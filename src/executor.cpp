#include <iostream>
#include <vector>
#include <unistd.h>   // for POSIX calls
#include <sys/wait.h> // for waitpid() , wait()
#include <fcntl.h>    // for file descriptors

#include "executor.h"

void Executor::execute(const Pipeline &pipeline)
{
    if (pipeline.commands.empty())
        return;

    if (pipeline.commands.size() == 1)
    {
        executeSingleCommand(pipeline.commands[0]);
    }
    else
    {
        executeMultiPipeline(pipeline);
    }
}

void Executor::executeMultiPipeline(const Pipeline& pipeline){

    int previousRead = -1 ;

    std::vector<pid_t> children;

    for(size_t i = 0 ; i < pipeline.commands.size() ; i++){

        bool first = (i == 0);
        bool last = (i == pipeline.commands.size() - 1) ;

        int pipefd[2];

        if(!last){
            if(pipe(pipefd) < 0){
                perror("pipe");

                if(previousRead != -1){
                    close(previousRead);
                }
                return;
            }
        }


        pid_t pid = fork();

        if(pid < 0){
            perror("fork");
            
            if(!last){
                close(pipefd[0]);
                close(pipefd[1]);
            }

            if(previousRead != -1){
                close(previousRead);
            }

            for(pid_t child : children){
                waitpid(child, nullptr, 0);
            }

            return ;
        }


        if(pid == 0){

            // - ---------- CHILD ----------------

            if(!first){
                if(dup2(previousRead, STDIN_FILENO) < 0){
                    perror("dup2");
                    std::exit(EXIT_FAILURE);
                }
            }

            if(!last){
                if(dup2( pipefd[1] , STDOUT_FILENO) < 0){
                    perror("dup2");
                    std::exit(EXIT_FAILURE);
                }
            }

            if(previousRead != -1){
                close(previousRead);
            }


            if(!last){
                close(pipefd[0]);
                close(pipefd[1]);                
            }



            executeSingleCommandHelper(pipeline.commands[i]);

            std::exit(EXIT_FAILURE);


        }

        children.push_back(pid);

        if(previousRead != -1){
            close(previousRead);
        }

        if(!last){
            close(pipefd[1]);
            previousRead = pipefd[0];
        }


    }

    if(previousRead != -1){
        close(previousRead);
    }

    for(pid_t child : children){
        waitpid(child, nullptr, 0) ;
    }
}

void Executor::executePipeline(const Pipeline &pipeline)
{

    if(pipeline.commands.size() != 2){
        std::cerr << "Only one pipe supported currently\n";
        return ;
    }

    int pipefd[2];

    if(pipe(pipefd) < 0){
        perror("pipe");
        return ;
    }

    // LEFT CHILD
    pid_t leftChild = fork();

    if(leftChild < 0){
        close(pipefd[0]);
        close(pipefd[1]);
        perror("fork");
        return;
    }

    if(leftChild == 0){
        if(dup2(pipefd[1], STDOUT_FILENO) < 0){

            close(pipefd[0]);
            close(pipefd[1]);

            perror("dup2");
            std::exit(EXIT_FAILURE);
        }
        
        close(pipefd[0]);
        close(pipefd[1]);

        executeSingleCommandHelper(pipeline.commands[0]);
    }

    // RIGHT CHILD
    pid_t rightChild = fork();

    if(rightChild < 0){
        close(pipefd[0]);
        close(pipefd[1]);

        waitpid(leftChild,nullptr,0);

        perror("fork");
        return;
    }

    if(rightChild == 0){
        if(dup2(pipefd[0], STDIN_FILENO) < 0){
            close(pipefd[0]);
            close(pipefd[1]);

            perror("dup2");
            std::exit(EXIT_FAILURE);
        }
        
        close(pipefd[0]);
        close(pipefd[1]);

        executeSingleCommandHelper(pipeline.commands[1]);
    }

    // PARENT 
    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(leftChild,nullptr,0);
    waitpid(rightChild,nullptr,0);        





}

void Executor::executeSingleCommand(const Command &command)
{

    pid_t pid = fork();

    if (pid == 0)
    {
        executeSingleCommandHelper(command);
    }
    else if (pid > 0)
    {
        // parent process
        waitpid(pid, nullptr, 0);
    }
    else
    {
        perror("fork");
    }
}

void Executor::executeSingleCommandHelper(const Command &command)
{
    // child process

    if (command.arguments.empty())
    {
        std::exit(EXIT_FAILURE);
    }

    std::vector<char *> argv;

    for (const auto &arg : command.arguments)
    {
        argv.push_back(const_cast<char *>(arg.c_str())); // stripping away the constness as the execvp is an older API and expects char*
    }
    argv.push_back(nullptr);
    if (!command.outputFile.empty())
    {
        int flags = O_WRONLY | O_CREAT;

        if (command.append)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(command.outputFile.c_str(),
                      flags,
                      0644);

        if (fd < 0)
        {
            perror("open");
            std::exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");
            std::exit(EXIT_FAILURE);
        }
        close(fd); // closing the file descriptor
    }

    if (!command.inputFile.empty())
    {

        int fd = open(command.inputFile.c_str(), O_RDONLY);

        if (fd < 0)
        {
            perror("open");
            std::exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            std::exit(EXIT_FAILURE);
        }

        close(fd);
    }

    execvp(argv[0], argv.data()); // string,vector<char*>

    perror("Execvp Failed in the Child Process");

    std::exit(EXIT_FAILURE);
}