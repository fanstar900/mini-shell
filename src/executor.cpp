#include <iostream>
#include <vector>
#include <unistd.h>   // for POSIX calls
#include <sys/wait.h> // for waitpid() , wait()

#include "executor.h"

void Executor::execute(const Command &command)
{

    if (command.arguments.empty())
    {
        return;
    }

    std::vector<char *> argv;

    for (const auto &arg : command.arguments)
    {
        argv.push_back(const_cast<char *>(arg.c_str())); // stripping away the constness as the execvp is an older API and expects char*
    }
    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid == 0)
    {
        // child process
        execvp(argv[0], argv.data()); // string,vector<char*>

        perror("Execvp Failed in the Child Process");

        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        // parent process
        waitpid(pid, nullptr, 0);
    }
    else
    {
        perror("pid");

    }
}