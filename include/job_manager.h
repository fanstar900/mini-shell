#pragma once

#include <string>
#include <vector>
#include <sys/types.h>
#include <termios.h>

enum class JobStatus
{
    Running,
    Stopped
};

struct Job
{
    int jobId = 0;

    pid_t processGroupId = 0;

    // Processes of this job that have not exited yet.
    std::vector<pid_t> pids;

    std::string command;

    JobStatus status = JobStatus::Running;

    // Terminal settings saved when the job was stopped, restored on `fg`.
    bool hasTerminalModes = false;
    struct termios terminalModes {};
};

class JobManager
{
public:

    // Puts the shell in its own process group and takes the terminal
    // (when stdin is a terminal), and installs the shell's signal setup.
    void initTerminal();

    bool isInteractive() const;

    // Stores the job, assigns it a job id and returns that id.
    int addJob(Job job);

    void removeJob(int jobId);

    Job* findJob(int jobId);

    // The most recently added job, or nullptr if there are none.
    Job* currentJob();

    void printJobs() const;

    // Gives the job the terminal and waits until all of its processes
    // exit or one of them stops. Continues the job first if `resume`.
    // Returns true if the job stopped.
    bool waitInForeground(Job& job, bool resume);

    // Sends SIGCONT to a stopped job without giving it the terminal.
    bool continueInBackground(Job& job);

    // Collects finished/stopped background processes without blocking
    // and reports jobs that changed state.
    void reapBackgroundJobs();

private:

    Job* findJobByPid(pid_t pid);

    std::vector<Job> jobs;

    bool interactive = false;

    pid_t shellProcessGroup = 0;

    struct termios shellTerminalModes {};
};
