#pragma once

#include <string>
#include <vector>
#include "command.h"

class Pipeline{
    public:
        std::vector<Command> commands;

        bool background = false;

        // The command line as typed (without a trailing '&'); shown by `jobs`.
        std::string text;
};
