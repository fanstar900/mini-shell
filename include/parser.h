#pragma once

#include<string>
#include "command.h"

class Parser{
    public:
        Command parse(const std::string& input);
};