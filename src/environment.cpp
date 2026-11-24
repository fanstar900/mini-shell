#include "environment.h"

#include <cctype>
#include <cstdlib>

namespace
{

bool isNameStart(char c)
{
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool isNameChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

} // namespace

std::string Environment::expand(
    const std::string& token
) const
{
    std::string result;
    result.reserve(token.size());

    size_t i = 0;

    while(i < token.size())
    {
        if(token[i] != '$' || i + 1 >= token.size())
        {
            result += token[i++];
            continue;
        }

        size_t start = i + 1;

        // ${NAME}
        if(token[start] == '{')
        {
            size_t close = token.find('}', start + 1);

            if(close == std::string::npos)
            {
                // Unterminated: keep the rest as typed.
                result += token.substr(i);
                break;
            }

            result += lookup(token.substr(start + 1, close - start - 1));
            i = close + 1;
            continue;
        }

        // A '$' not followed by a name is kept literally, e.g. "cost$5".
        if(!isNameStart(token[start]))
        {
            result += token[i++];
            continue;
        }

        // $NAME
        size_t end = start;

        while(end < token.size() && isNameChar(token[end]))
        {
            end++;
        }

        result += lookup(token.substr(start, end - start));
        i = end;
    }

    return result;
}

std::string Environment::lookup(
    const std::string& name
) const
{
    auto it = variables.find(name);

    if(it != variables.end())
    {
        return it->second;
    }

    const char* value = std::getenv(name.c_str());

    return value ? value : "";
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

bool Environment::isValidName(
    const std::string& name)
{
    if(name.empty() || !isNameStart(name[0]))
    {
        return false;
    }

    for(char c : name)
    {
        if(!isNameChar(c))
        {
            return false;
        }
    }

    return true;
}
