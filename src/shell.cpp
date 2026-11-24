#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "shell.h"
#include "pipeline.h"

namespace
{

const int historySize = 1000;

std::string historyFilePath()
{
    const char* home = std::getenv("HOME");

    return home ? std::string(home) + "/.mysh_history" : "";
}

} // namespace

Shell::Shell()
    : running(true)
{
    jobManager.initTerminal();

    loadHistory();
}

Shell::~Shell()
{
    saveHistory();
}

void Shell::loadHistory()
{
    // Limit the history before loading so both the in-memory list and the
    // file written on exit stay at most `historySize` entries.
    stifle_history(historySize);

    std::string historyFile = historyFilePath();

    if(!historyFile.empty())
    {
        read_history(historyFile.c_str());
    }
}

void Shell::saveHistory()
{
    std::string historyFile = historyFilePath();

    if(!historyFile.empty())
    {
        write_history(historyFile.c_str());
    }
}

void Shell::run()
{
    while(running)
    {
        // Report background jobs that finished or stopped since the last
        // prompt, and collect their processes.
        jobManager.reapBackgroundJobs();

        std::string input = readInput() ;

        processInput(input) ;
    }
}

std::string Shell::readInput() const
{
    char* line =
        readline("mysh> ");

    if(line == nullptr)
    {
        return "exit";
    }

    std::string input(line);

    if(!input.empty())
    {
        add_history(line);
    }

    free(line);

    return input;
}

void Shell::processInput(const std::string&input){

    Pipeline pipeline;
    std::string error;

    if(!parser.parse(input, pipeline, error))
    {
        std::cerr << "mysh: " << error << "\n";
        return;
    }

    if(pipeline.commands.empty()) return ;

    if(!expandPipeline(pipeline)) return ;

    const Command& first = pipeline.commands[0];

    // A lone foreground builtin runs in the shell itself so it can change
    // shell state (cd, export, exit, ...). Builtins in pipelines or in the
    // background run in a child process, like external commands.
    if(pipeline.commands.size() == 1 &&
       !pipeline.background &&
       Builtins::isBuiltin(first.program))
    {
        runBuiltinInShell(first);
        return;
    }

    executor.execute(
        pipeline,
        jobManager,
        [this](const Command& command)
        {
            return runBuiltin(command);
        }
    );
}

bool Shell::expandPipeline(Pipeline& pipeline) const
{
    for(auto& command : pipeline.commands)
    {
        std::vector<std::string> expanded;

        for(const auto& arg : command.arguments)
        {
            std::string value = environment.expand(arg);

            // Without quoting, a word that expands to nothing disappears,
            // as in other shells: `echo a $UNSET b` prints "a b".
            if(!value.empty())
            {
                expanded.push_back(value);
            }
        }

        command.arguments = std::move(expanded);

        for(std::string* file : {&command.inputFile, &command.outputFile})
        {
            if(file->empty())
            {
                continue;
            }

            std::string value = environment.expand(*file);

            if(value.empty())
            {
                std::cerr << "mysh: " << *file << ": ambiguous redirect\n";
                return false;
            }

            *file = value;
        }

        command.program =
            command.arguments.empty() ? "" : command.arguments[0];
    }

    for(const auto& command : pipeline.commands)
    {
        if(command.arguments.empty())
        {
            // A line such as "$UNSET" alone does nothing; inside a pipeline
            // or with a redirection there is no command to run.
            if(pipeline.commands.size() > 1 || command.hasRedirection())
            {
                std::cerr << "mysh: empty command after variable expansion\n";
            }

            return false;
        }
    }

    return true;
}

bool Shell::runBuiltin(const Command& command)
{
    return builtins.execute(
        command,
        running,
        previousDirectory,
        jobManager,
        environment
    );
}

void Shell::runBuiltinInShell(const Command& command)
{
    if(!command.hasRedirection())
    {
        runBuiltin(command);
        return;
    }

    // Point stdin/stdout at the redirection targets for the duration of the
    // builtin, then put the shell's own descriptors back.
    std::cout.flush();
    std::fflush(nullptr);

    int savedInput = dup(STDIN_FILENO);
    int savedOutput = dup(STDOUT_FILENO);

    if(savedInput < 0 || savedOutput < 0)
    {
        perror("mysh: dup");

        if(savedInput >= 0) close(savedInput);
        if(savedOutput >= 0) close(savedOutput);

        return;
    }

    if(Executor::applyRedirections(command))
    {
        runBuiltin(command);
    }

    std::cout.flush();
    std::fflush(nullptr);

    dup2(savedInput, STDIN_FILENO);
    dup2(savedOutput, STDOUT_FILENO);

    close(savedInput);
    close(savedOutput);
}
