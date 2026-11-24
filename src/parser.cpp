#include <cctype>
#include <string>
#include <vector>

#include "parser.h"

namespace
{

enum class TokenType
{
    Word,
    Pipe,
    Background,
    RedirectIn,
    RedirectOut,
    RedirectAppend
};

struct Token
{
    TokenType type;
    std::string text;
};

// Splits the input on whitespace and on the operators | & < > >>,
// so "ls>out.txt" and "ls > out.txt" give the same tokens.
std::vector<Token> tokenize(const std::string& input)
{
    std::vector<Token> tokens;
    std::string word;

    auto finishWord = [&]()
    {
        if(!word.empty())
        {
            tokens.push_back({TokenType::Word, word});
            word.clear();
        }
    };

    for(size_t i = 0; i < input.size(); i++)
    {
        char c = input[i];

        if(std::isspace(static_cast<unsigned char>(c)))
        {
            finishWord();
            continue;
        }

        switch(c)
        {
            case '|':
                finishWord();
                tokens.push_back({TokenType::Pipe, "|"});
                break;

            case '&':
                finishWord();
                tokens.push_back({TokenType::Background, "&"});
                break;

            case '<':
                finishWord();
                tokens.push_back({TokenType::RedirectIn, "<"});
                break;

            case '>':
                finishWord();
                if(i + 1 < input.size() && input[i + 1] == '>')
                {
                    tokens.push_back({TokenType::RedirectAppend, ">>"});
                    i++;
                }
                else
                {
                    tokens.push_back({TokenType::RedirectOut, ">"});
                }
                break;

            default:
                word += c;
        }
    }

    finishWord();

    return tokens;
}

std::string unexpectedToken(const Token& token)
{
    return "syntax error near unexpected token `" + token.text + "'";
}

std::string trim(const std::string& text)
{
    size_t begin = 0;
    size_t end = text.size();

    while(begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
        begin++;

    while(end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
        end--;

    return text.substr(begin, end - begin);
}

} // namespace

bool Parser::parse(
    const std::string& input,
    Pipeline& pipeline,
    std::string& error) const
{
    pipeline = Pipeline{};

    std::vector<Token> tokens = tokenize(input);

    if(tokens.empty())
    {
        return true;
    }

    Command current;

    for(size_t i = 0; i < tokens.size(); i++)
    {
        const Token& token = tokens[i];

        switch(token.type)
        {
            case TokenType::Word:
                current.arguments.push_back(token.text);
                break;

            case TokenType::RedirectIn:
            case TokenType::RedirectOut:
            case TokenType::RedirectAppend:
            {
                if(i + 1 >= tokens.size())
                {
                    error = "syntax error: expected filename after '"
                            + token.text + "'";
                    return false;
                }

                const Token& target = tokens[++i];

                if(target.type != TokenType::Word)
                {
                    error = unexpectedToken(target);
                    return false;
                }

                if(token.type == TokenType::RedirectIn)
                {
                    current.inputFile = target.text;
                }
                else
                {
                    current.outputFile = target.text;
                    current.append = (token.type == TokenType::RedirectAppend);
                }
                break;
            }

            case TokenType::Pipe:
                if(current.arguments.empty())
                {
                    error = unexpectedToken(token);
                    return false;
                }

                pipeline.commands.push_back(std::move(current));
                current = Command{};
                break;

            case TokenType::Background:
                if(current.arguments.empty())
                {
                    error = unexpectedToken(token);
                    return false;
                }

                if(i + 1 != tokens.size())
                {
                    error = "syntax error: '&' is only supported at the end of a command";
                    return false;
                }

                pipeline.background = true;
                break;
        }
    }

    if(current.arguments.empty())
    {
        // "ls |" or a redirection with no command, such as "> out.txt"
        error = pipeline.commands.empty()
                    ? "syntax error: missing command"
                    : "syntax error: missing command after '|'";
        return false;
    }

    pipeline.commands.push_back(std::move(current));

    pipeline.text = trim(input);

    if(pipeline.background)
    {
        pipeline.text.pop_back(); // the trailing '&'
        pipeline.text = trim(pipeline.text);
    }

    return true;
}
