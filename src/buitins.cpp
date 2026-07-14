#include "builtins.h"
#include <iostream>
#include <unistd.h>
#include <cstdlib> // used for getenv() function

bool Builtins::execute(const Command &command,
                       bool &running,
                       std::string &previousDirectory)
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

    return false;
}