#ifndef SHELL_H
#define SHELL.H

#include <string>
#include "executor.h"
#include "parser.h"
#include "builtins.h"
#include "job_manager.h"
#include "environment.h"

class Shell;

extern Shell* gShell;

class Shell
{
    public:
        Shell();
        ~Shell();
        void run();
                JobManager& getJobManager();
    private:

        Parser parser;
        Executor executor;
        Builtins builtins;
        JobManager jobManager;
        Environment environment;

        // States Management
        bool running;
        std::string previousDirectory;
        
        // void printPrompt() const; // prints mys>, this function can't modify the internal data members
        std::string readInput() const ; 

        void processInput(const std::string& input) ; 

        void loadHistory();

        void saveHistory();



};

#endif