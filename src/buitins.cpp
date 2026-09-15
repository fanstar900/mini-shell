#include "builtins.h"
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib> // used for getenv() function

#include <signal.h>
#include <readline/history.h>



bool Builtins::execute(
            const Command& command,
            bool& running,
            std::string& previousDirectory,
            JobManager& jobManager,
            Environment& environment
)
{
    /*
    If the return is true -> Built-in -> Executed in the Shell itself
    If the return is false ->External -> Forward to the executor

    check by which cd , which ls etc...

    pwd is external, but we'll implement it here

    */

    if (command.program == "exit")
    {
        running = false;
        return true;
    }

    if (command.program == "pwd")
    {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr)
        {
            std::cout << cwd << std::endl;
        }
        else
        {
            perror("pwd");
        }
        return true;
    }

    if (command.program == "cd")
    {

        const char *path = nullptr; // valid, blocks changing path characters, no problem with pointing string...

        // Handling the case to go to previous directory
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == nullptr)
        {
            perror("getcwd");
            return true;
        }
        std::string currentDirectory = cwd;

        if (command.arguments.size() == 2 &&
            command.arguments[1] == "-")
        {
            if (previousDirectory.empty())
            {
                std::cerr << "cd: OLDPWD not set\n";
                return true;
            }
            if (chdir(previousDirectory.c_str()) != 0)
            {
                perror("cd");
            }
            else
            {
                std::swap(previousDirectory, currentDirectory);
                std::cout << currentDirectory << std::endl;
            }
            return true;
        }

        if (command.arguments.size() == 1)
        {

            path = std::getenv("HOME");

            if (path == nullptr)
            {
                std::cerr << "cd: HOME not set\n";
                return true;
            }
        }
        else
        {
            path = command.arguments[1].c_str();
        }

        if (chdir(path) != 0)
        {
            perror("cd");
        }
        else
        {
            std::swap(previousDirectory, currentDirectory);
            std::cout << currentDirectory << std::endl;
        }

        return true;
    }

    if(command.program == "jobs")
    {
        jobManager.printJobs();

        return true;
    }

    if(command.program == "bg")
    {
        if(command.arguments.size() != 2)
        {
            std::cerr << "Usage: bg <job-id>\n";
            return true;
        }

        int jobId =
            std::stoi(command.arguments[1]);

        Job* job =
            jobManager.findJob(jobId);

        if(job == nullptr)
        {
            std::cerr
                << "bg: job not found\n";

            return true;
        }

        if(kill(job->pid, SIGCONT) < 0)
        {
            perror("bg");
            return true;
        }

        job->status =
            JobStatus::Running;

        std::cout
            << "[" << job->jobId
            << "] Running "
            << job->command
            << "\n";

        return true;
    }

    if(command.program == "fg")
    {
        if(command.arguments.size() != 2)
        {
            std::cerr
                << "Usage: fg <job-id>\n";

            return true;
        }

        int jobId =
            std::stoi(command.arguments[1]);

        Job* job =
            jobManager.findJob(jobId);

        if(job == nullptr)
        {
            std::cerr
                << "fg: job not found\n";

            return true;
        }

        if(kill(job->pid, SIGCONT) < 0)
        {
            perror("fg");
            return true;
        }

        job->status =
            JobStatus::Running;

        std::cout
            << job->command
            << std::endl;

        int status;

        pid_t result =
            waitpid(
                job->pid,
                &status,
                WUNTRACED
            );

        std::cout
            << "waitpid returned "
            << result
            << std::endl;

        if(WIFSTOPPED(status))
        {
            job->status =
                JobStatus::Stopped;

            std::cout
                << "[" << job->jobId
                << "] Stopped "
                << job->command
                << "\n";
        }
        else
        {
            jobManager.removeJob(
                job->pid
            );
        }

        return true;
    }

    if(command.program == "history")
    {
        HIST_ENTRY** entries =
            history_list();

        if(entries)
        {
            for(int i = 0;
                entries[i];
                i++)
            {
                std::cout
                    << i + 1
                    << " "
                    << entries[i]->line
                    << "\n";
            }
        }

        return true;
    }

    if(command.program == "export")
    {
        if(command.arguments.size() != 2)
        {
            std::cerr
                << "Usage: export NAME=value\n";

            return true;
        }

        std::string expr =
            command.arguments[1];

        size_t pos =
            expr.find('=');

        if(pos == std::string::npos)
        {
            std::cerr
                << "Invalid format\n";

            return true;
        }

        std::string name =
            expr.substr(0,pos);

        std::string value =
            expr.substr(pos+1);

        environment.setVariable(
            name,
            value
        );

        setenv(
            name.c_str(),
            value.c_str(),
            1
        );

        return true;
    }

    if(command.program == "unset")
    {
        if(command.arguments.size() != 2)
        {
            std::cerr
                << "Usage: unset NAME\n";

            return true;
        }

        environment.unsetVariable(
            command.arguments[1]
        );

        return true;
    }

    return false;
}