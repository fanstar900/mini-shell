#pragma once
#include "command.h"
#include "job_manager.h"
#include "environment.h"
class Builtins{
    public:
        bool execute(
            const Command& command,
            bool& running,
            std::string& previousDirectory,
            JobManager& jobManager,
            Environment& environment
        );
};
