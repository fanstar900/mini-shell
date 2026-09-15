#pragma once

#include <vector>
#include <string>
#include <sys/types.h>

enum class JobStatus
{
    Running,
    Stopped
};

struct Job
{
    int jobId;

    pid_t pid;

    pid_t processGroupId;

    std::string command;

    JobStatus status;
};

class JobManager
{
public:

    int addJob(
        pid_t pid,
        pid_t pgid,
        const std::string& command,
        JobStatus status
    );

    void markStopped(pid_t pid);

    void markRunning(pid_t pid);

    void removeJob(pid_t pid);

    Job* findJob(int jobId);

    Job* findJobByPid(pid_t pid);

    void printJobs() const;

private:

    int nextJobId = 1;

    std::vector<Job> jobs;
};