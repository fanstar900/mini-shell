#include "builtins.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib> // used for getenv() function

#include <readline/history.h>
#include <charconv>
#include <unordered_set>

// Parses "<n>" or "%<n>" into a job id. Returns false on malformed input.
static bool parseJobId(const std::string& text, int& jobId)
{
    const char* begin = text.data();
    const char* end = text.data() + text.size();

    if (begin != end && *begin == '%')
        ++begin;

    if (begin == end)
        return false;

    auto [ptr, ec] = std::from_chars(begin, end, jobId);
    return ec == std::errc() && ptr == end;
}

// Finds the job named by `fg`/`bg`'s argument, or the most recent job when
// no argument is given. Prints an error and returns nullptr if none matches.
static Job* lookupJob(const Command& command, JobManager& jobManager)
{
    const std::string& name = command.program;

    if(command.arguments.size() > 2)
    {
        std::cerr << "Usage: " << name << " [job-id]\n";
        return nullptr;
    }

    if(command.arguments.size() == 1)
    {
        Job* job = jobManager.currentJob();

        if(job == nullptr)
        {
            std::cerr << name << ": no current job\n";
        }

        return job;
    }

    int jobId = 0;

    if(!parseJobId(command.arguments[1], jobId))
    {
        std::cerr << name << ": invalid job id: "
                  << command.arguments[1] << "\n";
        return nullptr;
    }

    Job* job = jobManager.findJob(jobId);

    if(job == nullptr)
    {
        std::cerr << name << ": job not found: "
                  << command.arguments[1] << "\n";
    }

    return job;
}

bool Builtins::isBuiltin(const std::string& name)
{
    static const std::unordered_set<std::string> names = {
        "exit", "pwd", "cd", "jobs", "bg", "fg",
        "history", "export", "unset"
    };

    return names.count(name) > 0;
}

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
            previousDirectory = currentDirectory;
        }

        return true;
    }

    if(command.program == "jobs")
    {
        // Report jobs that finished since the last prompt first.
        jobManager.reapBackgroundJobs();

        jobManager.printJobs();

        return true;
    }

    if(command.program == "bg")
    {
        Job* job = lookupJob(command, jobManager);

        if(job == nullptr)
        {
            return true;
        }

        if(!jobManager.continueInBackground(*job))
        {
            return true;
        }

        std::cout
            << "[" << job->jobId
            << "] "
            << job->command
            << " &\n";

        return true;
    }

    if(command.program == "fg")
    {
        Job* job = lookupJob(command, jobManager);

        if(job == nullptr)
        {
            return true;
        }

        std::cout
            << job->command
            << std::endl;

        if(jobManager.waitInForeground(*job, true))
        {
            std::cout
                << "[" << job->jobId
                << "] Stopped "
                << job->command
                << "\n";
        }
        else
        {
            jobManager.removeJob(job->jobId);
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

        if(!Environment::isValidName(name))
        {
            std::cerr
                << "export: not a valid identifier: "
                << name << "\n";

            return true;
        }

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

        unsetenv(
            command.arguments[1].c_str()
        );

        return true;
    }

    return false;
}