#include "environment.h"

#include <cstdlib>

std::string Environment::expand(
    const std::string& token
) const
{
    if(token.empty())
    {
        return token;
    }

    if(token[0] != '$')
    {
        return token;
    }

    std::string name = token.substr(1);

    auto it = variables.find(name);

    if(it != variables.end())
    {
        return it->second;
    }

    const char* value = getenv(name.c_str());

    if(value)
    {
        return value;
    }

    return "";
}

void Environment::setVariable(
    const std::string& name,
    const std::string& value)
{
    variables[name] = value;
}

void Environment::unsetVariable(
    const std::string& name)
{
    variables.erase(name);
}