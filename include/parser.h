#pragma once

#include<string>
#include "pipeline.h"

class Parser{
    public:
        Pipeline parse(const std::string& input);
    private:
        Command parseSingleCommand(const std::string& input);
};