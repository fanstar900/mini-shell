#include<iostream>
#include<string>
#include<sstream>

#include "parser.h"


Command Parser::parse(const std::string& input){
    Command command ;

    std::istringstream iss(input);

    std::string token;

    while(iss >> token){
        command.arguments.push_back(token);
    }

    if(!command.arguments.empty()){
        command.program = command.arguments[0];
    }
    

    return command ;
}