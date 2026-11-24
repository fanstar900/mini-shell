#pragma once

#include <string>
#include "pipeline.h"

class Parser{
    public:
        // Parses `input` into `pipeline`.
        // Returns false and sets `error` when the input has a syntax error.
        bool parse(
            const std::string& input,
            Pipeline& pipeline,
            std::string& error
        ) const;
};
