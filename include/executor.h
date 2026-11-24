#pragma once

#include <functional>
#include <sys/types.h>
#include "pipeline.h"
#include "job_manager.h"

class Executor{

    public:
        // Runs a builtin in the calling process.
        // Returns false if the command is not a builtin.
        using BuiltinRunner = std::function<bool(const Command&)>;

        // Starts every stage of the pipeline in one process group, then
        // either waits for it (foreground) or records it as a job (background).
        void execute(
            const Pipeline& pipeline,
            JobManager& jobManager,
            const BuiltinRunner& runBuiltin
        );

        // Applies the command's <, > and >> redirections to this process.
        // Prints an error and returns false on failure.
        static bool applyRedirections(const Command& command);

    private:
        [[noreturn]] static void runChild(
            const Command& command,
            pid_t processGroup,
            bool foreground,
            bool interactive,
            int inputFd,
            int outputFd,
            const BuiltinRunner& runBuiltin
        );
} ;
