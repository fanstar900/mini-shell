#include <executor.h>

#include <iostream>
#include <sstream>
#include <vector>

#include <unistd.h>   // for POSIX calls
#include <sys/wait.h> // for waitpid() , wait()

void Executor::execute(const std::string &command)
{
    std::istringstream iss(command);

    std::vector<std::string> args; // dismantle the input into various arguments

    std::string token;

    while (iss >> token)
    {
        args.push_back(token);
    }

    // std::cout << "Parsed Arguments:\n" ;

    // for(const auto& arg : args){
    //     std::cout << arg << '\n' ;
    // }

    if (args.empty())
    {
        return;
    }

    std::vector<char *> argv;

    for (auto &arg : args)
    {
        argv.push_back(arg.data());
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
    else if (pid < 0)
    {

        perror("Child Process can't be created");

        exit(EXIT_FAILURE);
    }
    else
    {
        // parent process
        waitpid(pid, nullptr, 0);
    }
}