#pragma once
#include "command.h"
class Builtins{
    public:
        bool execute(const Command& command, bool& running, std::string& previousDirectory) ;
};
