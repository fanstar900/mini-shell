#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include <unistd.h>   // for POSIX calls
#include <fcntl.h>    // for open()
#include <signal.h>

#include "executor.h"
#include "signals.h"

namespace
{

// Kills and collects the processes already started for a pipeline that
// could not be fully set up, and gives the terminal back to the shell.
void abandonJob(Job& job, JobManager& jobManager)
{
    if(job.pids.empty())
    {
        return;
    }

    kill(-job.processGroupId, SIGKILL);

    jobManager.waitInForeground(job, false);
}

} // namespace

void Executor::execute(
    const Pipeline& pipeline,
    JobManager& jobManager,
    const BuiltinRunner& runBuiltin)
{
    if(pipeline.commands.empty())
        return;

    const bool foreground = !pipeline.background;
    const bool interactive = jobManager.isInteractive();

    Job job;
    job.command = pipeline.text;

    // Anything still buffered would otherwise be written again by children.
    std::cout.flush();
    std::fflush(nullptr);

    int previousRead = -1 ;

    for(size_t i = 0 ; i < pipeline.commands.size() ; i++){

        bool last = (i == pipeline.commands.size() - 1) ;

        int pipefd[2] = {-1, -1};

        if(!last && pipe(pipefd) < 0){
            perror("pipe");

            if(previousRead != -1){
                close(previousRead);
            }

            abandonJob(job, jobManager);
            return;
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

            abandonJob(job, jobManager);
            return ;
        }

        if(pid == 0){
            // ---------- CHILD ----------
            if(!last){
                close(pipefd[0]); // read end belongs to the next stage
            }

            runChild(
                pipeline.commands[i],
                job.processGroupId,
                foreground,
                interactive,
                previousRead == -1 ? STDIN_FILENO : previousRead,
                last ? STDOUT_FILENO : pipefd[1],
                runBuiltin
            );
        }

        // ---------- PARENT ----------
        // The first stage leads the job's process group. Both parent and
        // child call setpgid() so the group exists whichever runs first.
        if(job.processGroupId == 0){
            job.processGroupId = pid;
        }

        setpgid(pid, job.processGroupId);

        job.pids.push_back(pid);

        if(previousRead != -1){
            close(previousRead);
        }

        if(!last){
            close(pipefd[1]);
            previousRead = pipefd[0];
        }
    }

    if(!foreground)
    {
        int jobId = jobManager.addJob(job);

        std::cout
            << "[" << jobId << "] "
            << job.processGroupId
            << "\n";

        return;
    }

    if(jobManager.waitInForeground(job, false))
    {
        int jobId = jobManager.addJob(job);

        std::cout
            << "[" << jobId << "] Stopped "
            << job.command
            << "\n";
    }
}

void Executor::runChild(
    const Command& command,
    pid_t processGroup,
    bool foreground,
    bool interactive,
    int inputFd,
    int outputFd,
    const BuiltinRunner& runBuiltin)
{
    pid_t self = getpid();

    if(processGroup == 0){
        processGroup = self;
    }

    setpgid(self, processGroup);

    // Must happen before SIGTTOU is reset to its default, or the call
    // would stop this process.
    if(foreground && interactive){
        tcsetpgrp(STDIN_FILENO, processGroup);
    }

    resetChildSignals();

    if(inputFd != STDIN_FILENO){
        if(dup2(inputFd, STDIN_FILENO) < 0){
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(inputFd);
    }

    if(outputFd != STDOUT_FILENO){
        if(dup2(outputFd, STDOUT_FILENO) < 0){
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(outputFd);
    }

    if(!applyRedirections(command)){
        _exit(EXIT_FAILURE);
    }

    // Builtins in a pipeline or in the background run in this child,
    // so e.g. `history | grep ls` works. State changes (cd, export)
    // only affect the child, as in other shells.
    if(runBuiltin(command)){
        std::cout.flush();
        std::fflush(nullptr);
        _exit(EXIT_SUCCESS);
    }

    std::vector<char *> argv;

    for (const auto &arg : command.arguments)
    {
        argv.push_back(const_cast<char *>(arg.c_str())); // stripping away the constness as the execvp is an older API and expects char*
    }
    argv.push_back(nullptr);

    execvp(argv[0], argv.data());

    int error = errno;

    if(error == ENOENT){
        std::cerr << "mysh: " << command.program << ": command not found\n";
    }else{
        std::cerr << "mysh: " << command.program << ": " << std::strerror(error) << "\n";
    }

    // _exit() rather than exit(): don't run the shell's atexit handlers
    // or flush stdio buffers copied from the parent.
    _exit(error == ENOENT ? 127 : 126);
}

bool Executor::applyRedirections(const Command &command)
{
    if (!command.inputFile.empty())
    {
        int fd = open(command.inputFile.c_str(), O_RDONLY);

        if (fd < 0)
        {
            perror(command.inputFile.c_str());
            return false;
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            return false;
        }

        close(fd);
    }

    if (!command.outputFile.empty())
    {
        int flags = O_WRONLY | O_CREAT;

        if (command.append)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(command.outputFile.c_str(), flags, 0644);

        if (fd < 0)
        {
            perror(command.outputFile.c_str());
            return false;
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            return false;
        }

        close(fd); // closing the file descriptor
    }

    return true;
}
