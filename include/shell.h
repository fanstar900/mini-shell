#ifndef SHELL_H
#define SHELL.H

#include <string>
class Shell
{
    public:
        Shell();
        void run();
    private:
        bool running;
        
        void printPrompt() const; // prints mys>, this function can't modify the internal data members
        std::string readInput() const ; 

        void processInput(const std::string& input) ; 



};

#endif