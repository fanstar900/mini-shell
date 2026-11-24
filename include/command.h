#pragma once

#include <string>
#include <vector>

class Command{
    public:
        // arguments[0] after variable expansion; set by the shell.
        std::string program;
        std::vector<std::string> arguments;

        std::string inputFile;
        std::string outputFile;

        bool append = false ;

        bool hasRedirection() const
        {
            return !inputFile.empty() || !outputFile.empty();
        }
};
