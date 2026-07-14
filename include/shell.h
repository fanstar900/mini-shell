#ifndef SHELL_H
#define SHELL.H

#include <string>
#include "executor.h"
#include "parser.h"
#include "builtins.h"

class Shell
{
    public:
        Shell();
        void run();
    private:

        Parser parser;
        Executor executor;
        Builtins builtins;

        // States Management
        bool running;
        std::string previousDirectory;
        
        void printPrompt() const; // prints mys>, this function can't modify the internal data members
        std::string readInput() const ; 

        void processInput(const std::string& input) ; 



};

#endif