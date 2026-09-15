#include "job_manager.h"

#include <iostream>
#include <algorithm>

int JobManager::addJob(
    pid_t pid,
    pid_t pgid,
    const std::string& command,
    JobStatus status)
{
    Job job;

    job.jobId = nextJobId++;

    job.pid = pid;

    job.processGroupId = pgid;

    job.command = command;

    job.status = status;

    jobs.push_back(job);

    return job.jobId;
}

void JobManager::markStopped(pid_t pid)
{
    for(auto& job : jobs)
    {
        if(job.pid == pid)
        {
            job.status = JobStatus::Stopped;
            return;
        }
    }
}

void JobManager::markRunning(pid_t pid)
{
    for(auto& job : jobs)
    {
        if(job.pid == pid)
        {
            job.status = JobStatus::Running;
            return;
        }
    }
}

void JobManager::removeJob(pid_t pid)
{
    jobs.erase(
        std::remove_if(
            jobs.begin(),
            jobs.end(),
            [pid](const Job& job)
            {
                return job.pid == pid;
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

void JobManager::printJobs() const
{
    for(const auto& job : jobs)
    {
        std::cout
            << "[" << job.jobId << "] ";

        if(job.status == JobStatus::Running)
        {
            std::cout << "Running ";
        }
        else
        {
            std::cout << "Stopped ";
        }

        std::cout
            << job.command
            << "\n";
    }
}

Job* JobManager::findJobByPid(pid_t pid)
{
    for(auto& job : jobs)
    {
        if(job.pid == pid)
        {
            return &job;
        }
    }

    return nullptr;
}
