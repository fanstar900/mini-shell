#include "shell.h"
#include "pipeline.h"

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
    
    
    Pipeline pipeline = parser.parse(input);

    // for(auto& cmd : pipeline.commands){
    //     std::cout << cmd.program << std::endl;
    // }

    // loop over pipeline for command and connect their file descriptors



    if(pipeline.commands.empty()) return ;

    if(pipeline.commands.size() == 1){
        if(builtins.execute(pipeline.commands[0],
            running,
            previousDirectory))
        {
            return; 
        }
    }

    executor.execute(pipeline);

}