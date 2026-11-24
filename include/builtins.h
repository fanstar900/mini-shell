#pragma once

#include <string>
#include "command.h"
#include "job_manager.h"
#include "environment.h"

class Builtins{
    public:
        static bool isBuiltin(const std::string& name);

        // Returns true if `command` was a builtin (and has been run).
        bool execute(
            const Command& command,
            bool& running,
            std::string& previousDirectory,
            JobManager& jobManager,
            Environment& environment
        );
};
