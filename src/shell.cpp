#include "shell.h"
#include "executor.h"

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
    if(input == "exit"){
        running = false ; 
        return ;
    }

    // std::cout << "You Entered : " << input << std::endl;
    
    Executor executor;
    executor.execute(input) ;

}