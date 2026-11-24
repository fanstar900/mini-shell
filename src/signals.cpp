#include "signals.h"

#include <signal.h>

/*
The shell no longer installs a SIGCHLD handler. Updating the job list from a
signal handler is not async-signal-safe (it allocates memory, which can
deadlock or corrupt the heap if the signal arrives while readline is
allocating). Instead, finished background jobs are collected with a
non-blocking waitpid() before each prompt (JobManager::reapBackgroundJobs),
and foreground jobs are waited for directly.
*/

static const int jobControlSignals[] = {
    SIGINT,
    SIGQUIT,
    SIGTSTP,
    SIGTTIN,
    SIGTTOU
};

void setupSignalHandlers()
{
    for(int sig : jobControlSignals)
    {
        signal(sig, SIG_IGN);
    }
}

void resetChildSignals()
{
    for(int sig : jobControlSignals)
    {
        signal(sig, SIG_DFL);
    }
}
