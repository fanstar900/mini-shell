#include<iostream>
#include "signals.h"

#include "shell.h"

#include <signal.h>
#include <sys/wait.h>

static void handleSigChild(int)
{
    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        // std::cout << "Reaped child "
        //           << pid
        //           << std::endl;

        if(gShell)
        {
            gShell->getJobManager()
                  .removeJob(pid);
        }
    }
}

void setupSignalHandlers()
{
    signal(SIGINT, SIG_IGN);

    signal(SIGTSTP, SIG_IGN);

    signal(SIGQUIT, SIG_IGN);

    struct sigaction sa;

    sa.sa_handler = handleSigChild;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART;

    sigaction(
        SIGCHLD,
        &sa,
        nullptr
    );
}