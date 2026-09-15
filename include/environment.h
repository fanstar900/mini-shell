#pragma once

#include <string>
#include <unordered_map>

class Environment
{
public:

    std::string expand(
        const std::string& token
    ) const;

    void setVariable(
        const std::string& name,
        const std::string& value
    );

    void unsetVariable(
        const std::string& name
    );

private:

    std::unordered_map<
        std::string,
        std::string
    > variables;
};