#include "job_manager.h"
#include "signals.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

void JobManager::initTerminal()
{
    interactive = isatty(STDIN_FILENO);

    if(interactive)
    {
        // If the shell was started in the background, stop until it is
        // brought to the foreground, as other shells do.
        while(true)
        {
            pid_t foregroundGroup = tcgetpgrp(STDIN_FILENO);

            if(foregroundGroup < 0)
            {
                // A terminal, but not our controlling terminal.
                interactive = false;
                break;
            }

            pid_t ownGroup = getpgrp();

            if(foregroundGroup == ownGroup)
            {
                break;
            }

            kill(-ownGroup, SIGTTIN);
        }
    }

    setupSignalHandlers();

    shellProcessGroup = getpgrp();

    if(!interactive)
    {
        return;
    }

    // Put the shell in its own process group and take the terminal.
    shellProcessGroup = getpid();

    if(getpgrp() != shellProcessGroup &&
       setpgid(0, shellProcessGroup) < 0)
    {
        perror("mysh: setpgid");
        interactive = false;
        return;
    }

    if(tcsetpgrp(STDIN_FILENO, shellProcessGroup) < 0)
    {
        perror("mysh: tcsetpgrp");
    }

    tcgetattr(STDIN_FILENO, &shellTerminalModes);
}

bool JobManager::isInteractive() const
{
    return interactive;
}

int JobManager::addJob(Job job)
{
    // Like other shells, reuse ids once jobs finish: next id is max + 1.
    int maxId = 0;

    for(const auto& existing : jobs)
    {
        maxId = std::max(maxId, existing.jobId);
    }

    job.jobId = maxId + 1;

    jobs.push_back(std::move(job));

    return jobs.back().jobId;
}

void JobManager::removeJob(int jobId)
{
    jobs.erase(
        std::remove_if(
            jobs.begin(),
            jobs.end(),
            [jobId](const Job& job)
            {
                return job.jobId == jobId;
            }),
        jobs.end());
}

Job* JobManager::findJob(int jobId)
{
    for(auto& job : jobs)
    {
        if(job.jobId == jobId)
        {
            return &job;
        }
    }

    return nullptr;
}

Job* JobManager::currentJob()
{
    return jobs.empty() ? nullptr : &jobs.back();
}

Job* JobManager::findJobByPid(pid_t pid)
{
    for(auto& job : jobs)
    {
        if(std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end())
        {
            return &job;
        }
    }

    return nullptr;
}

void JobManager::printJobs() const
{
    for(const auto& job : jobs)
    {
        std::cout
            << "[" << job.jobId << "] "
            << (job.status == JobStatus::Running ? "Running " : "Stopped ")
            << job.command
            << "\n";
    }
}

bool JobManager::waitInForeground(Job& job, bool resume)
{
    if(interactive)
    {
        tcsetpgrp(STDIN_FILENO, job.processGroupId);

        if(resume && job.hasTerminalModes)
        {
            tcsetattr(STDIN_FILENO, TCSADRAIN, &job.terminalModes);
        }
    }

    if(resume && kill(-job.processGroupId, SIGCONT) < 0)
    {
        perror("mysh: kill");
    }

    job.status = JobStatus::Running;

    bool stopped = false;
    bool interrupted = false;

    while(!job.pids.empty())
    {
        int status = 0;

        pid_t pid = waitpid(-job.processGroupId, &status, WUNTRACED);

        if(pid < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }

            // ECHILD: nothing left to wait for in this group.
            job.pids.clear();
            break;
        }

        if(WIFSTOPPED(status))
        {
            job.status = JobStatus::Stopped;
            stopped = true;
            break;
        }

        if(WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
        {
            interrupted = true;
        }

        job.pids.erase(
            std::remove(job.pids.begin(), job.pids.end(), pid),
            job.pids.end());
    }

    if(interactive)
    {
        // Take the terminal back and restore the shell's terminal settings,
        // keeping the job's settings in case it is resumed later.
        tcsetpgrp(STDIN_FILENO, shellProcessGroup);

        if(stopped)
        {
            tcgetattr(STDIN_FILENO, &job.terminalModes);
            job.hasTerminalModes = true;
        }

        tcsetattr(STDIN_FILENO, TCSADRAIN, &shellTerminalModes);
    }

    // The terminal echoed ^C or ^Z without a newline.
    if(stopped || interrupted)
    {
        std::cout << "\n";
    }

    return stopped;
}

bool JobManager::continueInBackground(Job& job)
{
    if(kill(-job.processGroupId, SIGCONT) < 0)
    {
        perror("mysh: kill");
        return false;
    }

    job.status = JobStatus::Running;

    return true;
}

void JobManager::reapBackgroundJobs()
{
    int status = 0;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        Job* job = findJobByPid(pid);

        if(job == nullptr)
        {
            continue;
        }

        if(WIFSTOPPED(status))
        {
            if(job->status != JobStatus::Stopped)
            {
                job->status = JobStatus::Stopped;

                std::cout
                    << "[" << job->jobId << "] Stopped "
                    << job->command << "\n";
            }
        }
        else if(WIFCONTINUED(status))
        {
            job->status = JobStatus::Running;
        }
        else
        {
            job->pids.erase(
                std::remove(job->pids.begin(), job->pids.end(), pid),
                job->pids.end());
        }
    }

    for(const auto& job : jobs)
    {
        if(job.pids.empty())
        {
            std::cout
                << "[" << job.jobId << "] Done "
                << job.command << "\n";
        }
    }

    jobs.erase(
        std::remove_if(
            jobs.begin(),
            jobs.end(),
            [](const Job& job)
            {
                return job.pids.empty();
            }),
        jobs.end());
}
