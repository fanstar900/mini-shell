#pragma once

#include<string>
#include<vector>

class Command{
    public:
        std::string program;
        std::vector<std::string> arguments;

        std::string inputFile;
        std::string outputFile;

        bool append = false ; 
        bool background = false ;
};