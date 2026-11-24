# Mini Unix Shell

A small interactive Unix-like shell written in C++20. It uses POSIX process and file-descriptor APIs directly to explore how a shell reads commands, starts programs, connects pipelines, redirects I/O, tracks jobs, and handles signals.

The executable is called `mini-shell` and displays the prompt `mysh> `.

## What It Supports

### Commands and builtins

Commands found through `PATH` are started with `fork()` and `execvp()`. When a builtin is the only command on the line and runs in the foreground, it runs in the shell process so it can change shell state. Builtins inside a pipeline or started with `&` run in a child process, as in other shells, so `history | grep ls` works but `cd /tmp | cat` does not change the shell's directory.

| Command | Behavior |
| --- | --- |
| `cd [directory]` | Change directory; with no argument, uses `$HOME`. |
| `cd -` | Return to the previous directory. |
| `pwd` | Print the current working directory. |
| `jobs` | List background and stopped jobs. |
| `fg [job-id]` | Give a job the terminal, continue it, and wait for it. Accepts `1` or `%1`; defaults to the most recent job. |
| `bg [job-id]` | Continue a stopped job in the background. |
| `export NAME=value` | Set a variable for the shell and the commands it starts. |
| `unset NAME` | Remove a variable from the shell and from the environment. |
| `history` | Print the current readline history. |
| `exit` | Leave the shell. |

### Redirection

The parser recognizes these redirection operators, with or without surrounding spaces:

```bash
ls > files.txt
echo hello >> log.txt
cat<input.txt
grep main < source.cpp > matches.txt
pwd > where.txt
```

`>` truncates or creates a file, while `>>` appends to it. Redirection is implemented with `open()`, `dup2()`, and `close()`. Redirection also works for builtins that run in the shell: the shell temporarily points its own stdin/stdout at the files and restores them afterwards.

### Pipelines

Pipelines can contain multiple commands. Each stage runs in its own child process, and adjacent stages are connected with POSIX pipes:

```bash
cat README.md | grep shell | wc -l
seq 3 -1 | sort -n
```

### Background jobs, job control and signals

Appending `&` starts a command or a whole pipeline without waiting for it:

```bash
sleep 30 &
cat README.md | grep shell | sort &
jobs
fg 1
```

Each command line runs as one job in its own process group. A foreground job is given the terminal with `tcsetpgrp()`, so Ctrl+C and Ctrl+Z reach only that job, never the shell or background jobs. When the job exits or stops, the shell takes the terminal back and restores its terminal settings. A stopped job keeps its own terminal settings, which are restored when it is resumed with `fg`. A background job that tries to read from the terminal is stopped by the kernel (`SIGTTIN`) and can be brought to the foreground with `fg`.

The shell ignores `SIGINT`, `SIGQUIT`, `SIGTSTP`, `SIGTTIN`, and `SIGTTOU`, and child processes restore the defaults before `exec`. The shell does not use a `SIGCHLD` handler: updating the job list inside a signal handler is not async-signal-safe. Foreground jobs are waited for directly. Before each prompt, background processes are collected with a non-blocking `waitpid()` and state changes are reported (`[1] Done sleep 30`, `[2] Stopped cat`).

### Environment expansion and history

`$NAME` and `${NAME}` are expanded anywhere in an argument or redirection target, from shell variables first and then from the inherited process environment. A word that expands to nothing is dropped, as in other shells:

```bash
export PROJECT=custom-shell
echo $PROJECT
echo $HOME/projects ${PROJECT}-build
```

GNU Readline provides line editing and Up/Down history navigation. History is loaded from and saved to `~/.mysh_history`; history is limited to the most recent 1000 entries, both in memory and in the saved file.

## Parsing Boundaries

The parser is intentionally small. It splits input on whitespace and on the operators `|`, `&`, `<`, `>`, and `>>`. Malformed lines such as `ls |`, `| ls`, `ls >`, or `a & b` are rejected with a syntax error and nothing is run. `&` is only allowed at the end of a line, so `&&`, `||` and `;` are not supported. The current implementation does not provide shell quoting or escaping, wildcard expansion, command substitution, here-documents, stderr redirection (`2>`), or shell scripting syntax.

## Architecture

```text
readline
    |
 Shell: history, expansion, builtin dispatch
    |
 Parser: input -> Pipeline -> Command objects
    |
 Executor: fork, execvp, redirection, pipelines, waiting
    |
 POSIX processes and file descriptors

 Shell state
    +-- Environment
    +-- JobManager
    +-- JobManager: process groups, terminal handoff, job reaping
```

### Source layout

```text
include/                 Public headers
   builtins.h             Builtin command dispatch
   command.h              Parsed command representation
   environment.h          Variable expansion and storage
   executor.h             Process execution interface
   job_manager.h          Job records, job control and terminal ownership
   parser.h               Command-line parsing
   pipeline.h             Pipeline representation
   shell.h                REPL and shell state
   signals.h              Signal setup
src/
   main.cpp               Program entry point
   shell.cpp              Read-eval-print loop and history
   parser.cpp             Tokenizing and syntax checking
   executor.cpp           Process creation and execution
   builtins.cpp           Builtin implementations
   environment.cpp        Variable expansion
   job_manager.cpp        Job bookkeeping, foreground waits, background reaping
   signals.cpp            Shell and child signal dispositions
```

## Build and Run

### Requirements

- Linux or another POSIX-like system
- A C++20 compiler, such as GCC or Clang
- CMake 3.16 or newer
- GNU Readline development files

On Debian or Ubuntu, the Readline dependency can be installed with:

```bash
sudo apt install build-essential cmake libreadline-dev
```

### Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build
```

Run the shell with:

```bash
./build/mini-shell
```

The existing `build/` directory can also be built with `make`.

## POSIX APIs in the Implementation

- **Process execution:** `fork()`, `execvp()`, `waitpid()`
- **Pipelines and descriptors:** `pipe()`, `open()`, `dup2()`, `close()`
- **Directories:** `getcwd()`, `chdir()`
- **Signals and jobs:** `signal()`, `kill()`, `waitpid()` with `WUNTRACED`/`WCONTINUED`/`WNOHANG`
- **Process groups and terminals:** `setpgid()`, `getpgrp()`, `tcsetpgrp()`, `tcgetpgrp()`, `tcgetattr()`, `tcsetattr()`

## Learning Goals

This project is an exercise in POSIX systems programming and operating-system concepts, including process creation, parent-child synchronization, file descriptors, inter-process communication, signal delivery, environment state, and the boundary between shell builtins and external programs.

## Possible Next Steps

- Track exit statuses and support `$?`, `&&`, `||`, and `;`.
- Add stderr redirection (`2>`, `2>&1`).
- Add quoted strings, escaping, and more precise syntax errors.
- Add wildcard expansion, command substitution, and here-documents.
- Add tests for parsing, redirection, pipelines, and job state transitions.