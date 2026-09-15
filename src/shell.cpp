#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>

#include "shell.h"
#include "pipeline.h"

Shell* gShell = nullptr;

Shell::Shell()
    : running(true)
{
    loadHistory();

    gShell = this;
}

Shell::~Shell()
{
    saveHistory();

    stifle_history(1000);

}

void Shell::loadHistory()
{
    const char* home =
        getenv("HOME");

    if(home)
    {
        std::string historyFile =
            std::string(home)
            + "/.mysh_history";

        read_history(
            historyFile.c_str()
        );
    }
}

void Shell::saveHistory()
{
    const char* home =
        getenv("HOME");

    if(home)
    {
        std::string historyFile =
            std::string(home)
            + "/.mysh_history";

        write_history(
            historyFile.c_str()
        );
    }
}

void Shell::run()
{

    while(running)
    {
        // printPrompt(); // readline already prints the prompt

        std::string input = readInput() ; 

        processInput(input) ;

    }
}

// void Shell::printPrompt() const{
//     std::cout << "mysh>";
// }


std::string Shell::readInput() const
{
    char* line =
        readline("mysh> ");

    if(line == nullptr)
    {
        return "exit";
    }

    std::string input(line);

    if(!input.empty())
    {
        add_history(line);
    }

    free(line);

    return input;
}


void Shell::processInput(const std::string&input){
    
    /*
    Inefficient => parser and executor are constructed for each processInput call, we should have them as internal data member for this shell
    Parser parser ; 
    Executor executor ;
    */
    
    
    Pipeline pipeline = parser.parse(input);

    // Expand environment variables
    for(auto& command : pipeline.commands)
    {
        for(auto& arg : command.arguments)
        {
            arg = environment.expand(arg);
        }
    }

    // std::cout
    // << "Background = "
    // << pipeline.background
    // << std::endl;

    // for(const auto& cmd : pipeline.commands)
    // {
    //     std::cout << "Program: "
    //             << cmd.program
    //             << "\n";

    //     for(const auto& arg : cmd.arguments)
    //     {
    //         std::cout
    //             << "Arg: "
    //             << arg
    //             << "\n";
    //     }
    // }

    // for(auto& cmd : pipeline.commands){
    //     std::cout << cmd.program << std::endl;
    // }

    // loop over pipeline for command and connect their file descriptors



    if(pipeline.commands.empty()) return ;

    if(pipeline.commands.size() == 1){
        if(builtins.execute(
                pipeline.commands[0],
                running,
                previousDirectory,
                jobManager,
                environment))
        {
            return;
        }
    }

    executor.execute(
        pipeline,
        jobManager
    );

}

JobManager& Shell::getJobManager()
{
    return jobManager;
}