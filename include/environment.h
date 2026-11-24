#pragma once

#include <string>
#include <unordered_map>

class Environment
{
public:

    // Replaces every $NAME and ${NAME} in `token` with the variable's value.
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

    // True if `name` is a valid variable name ([A-Za-z_][A-Za-z0-9_]*).
    static bool isValidName(
        const std::string& name
    );

private:

    std::string lookup(
        const std::string& name
    ) const;

    std::unordered_map<
        std::string,
        std::string
    > variables;
};
