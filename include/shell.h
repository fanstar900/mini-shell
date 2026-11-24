#pragma once

#include <string>
#include "executor.h"
#include "parser.h"
#include "builtins.h"
#include "job_manager.h"
#include "environment.h"

class Shell
{
    public:
        Shell();
        ~Shell();
        void run();

    private:

        Parser parser;
        Executor executor;
        Builtins builtins;
        JobManager jobManager;
        Environment environment;

        // States Management
        bool running;
        std::string previousDirectory;

        std::string readInput() const ;

        void processInput(const std::string& input) ;

        // Expands variables in every argument and redirection target.
        // Returns false (after printing an error) if the result is unusable.
        bool expandPipeline(Pipeline& pipeline) const;

        bool runBuiltin(const Command& command);

        // Runs a builtin in the shell process, applying its redirections
        // temporarily.
        void runBuiltinInShell(const Command& command);

        void loadHistory();

        void saveHistory();
};
