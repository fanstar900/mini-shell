#include<iostream>
#include<string>
#include<sstream>

#include "parser.h"



Pipeline Parser::parse(const std::string& input){
    Pipeline pipeline;

    std::stringstream ss(input);

    std::string commandString;

    while(std::getline(ss, commandString, '|')){

        pipeline.commands.push_back(parseSingleCommand(commandString));


    }

    return pipeline ;
}

Command Parser::parseSingleCommand(const std::string& input){
    Command command ;

    std::istringstream iss(input);

    std::string token;

    while(iss >> token){
        if(token == ">"){
            if(!(iss >> command.outputFile)){
                std::cerr << "Syntax error: expected filename after '>'\n" ;
            }
            command.append = false ;
        }else if(token == ">>"){
            if(!(iss >> command.outputFile)){
                std::cerr << "Syntax error: expected filename after '>>'\n" ;
            }
            command.append = true ;
        }else if(token == "<"){
            if(!(iss >> command.inputFile)){
                std::cerr << "Syntax error: expected filename after '<'\n" ;
            }
        }else{
            command.arguments.push_back(token); 
        }
        
    }

    if(!command.arguments.empty()){
        command.program = command.arguments[0];
    }

    // std::cout << command.program << std::endl;
    // std::cout << command.inputFile << std::endl;
    // std::cout << command.outputFile << std::endl;
    // for(auto& arg : command.arguments){
    //     std::cout << arg << std::endl;
    // }
    

    return command ;
}