#include "shell.h"
#include "command.h"

#include <iostream>

Shell::Shell(): running(true){
    
}

void Shell::run()
{
    while(running)
    {
        printPrompt();

        std::string input = readInput() ; 

        processInput(input) ;

    }
}

void Shell::printPrompt() const{
    std::cout << "mysh>";
}


std::string Shell::readInput() const 
{
    std::string input;
    std::getline(std::cin,input); // takes in the line an put into input string
    return input ; 
}


void Shell::processInput(const std::string&input){
    
    /*
    Inefficient => parser and executor are constructed for each processInput call, we should have them as internal data member for this shell
    Parser parser ; 
    Executor executor ;
    */
    
    
    Command command = parser.parse(input);

    if(command.arguments.empty()) return ;

    if(builtins.execute(command, running, previousDirectory)){
        return ;
    } 

    executor.execute(command);

}