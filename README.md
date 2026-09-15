# Mini Unix Shell

A small interactive Unix-like shell written in C++20. It uses POSIX process and file-descriptor APIs directly to explore how a shell reads commands, starts programs, connects pipelines, redirects I/O, tracks jobs, and handles signals.

The executable is called `mini-shell` and displays the prompt `mysh> `.

## What It Supports

### Commands and builtins

Commands found through `PATH` are started with `fork()` and `execvp()`. The following builtins run in the shell process so they can change shell state:

| Command | Behavior |
| --- | --- |
| `cd [directory]` | Change directory; with no argument, uses `$HOME`. |
| `cd -` | Return to the previous directory. |
| `pwd` | Print the current working directory. |
| `jobs` | List tracked background and stopped jobs. |
| `fg <job-id>` | Continue a job and wait for it in the foreground. |
| `bg <job-id>` | Continue a stopped job in the background. |
| `export NAME=value` | Set a shell variable and update the process environment. |
| `unset NAME` | Remove a variable from the shell's custom variable map. |
| `history` | Print the current readline history. |
| `exit` | Leave the shell. |

### Redirection

The parser recognizes space-separated redirection operators:

```bash
ls > files.txt
echo hello >> log.txt
cat < input.txt
grep main < source.cpp > matches.txt
```

`>` truncates or creates a file, while `>>` appends to it. Redirection is implemented with `open()`, `dup2()`, and `close()`.

### Pipelines

Pipelines can contain multiple commands. Each stage runs in its own child process, and adjacent stages are connected with POSIX pipes:

```bash
cat README.md | grep shell | wc -l
seq 3 -1 | sort -n
```

### Background jobs and signals

Appending `&` starts a single command without waiting for it:

```bash
sleep 30 &
jobs
fg 1
```

The shell ignores `SIGINT`, `SIGTSTP`, and `SIGQUIT` for itself, restores the default behavior in child processes, and uses `SIGCHLD` to reap completed children. Stopped and running jobs are stored by job ID.

### Environment expansion and history

Arguments beginning with `$` are expanded from shell variables first and then from the inherited process environment:

```bash
export PROJECT=custom-shell
echo $PROJECT
echo $HOME
```

GNU Readline provides line editing and Up/Down history navigation. History is loaded from and saved to `~/.mysh_history`; the in-memory history is stifled to 1000 entries when the shell exits.

## Parsing Boundaries

The parser is intentionally small and tokenizes input on whitespace. Redirection operators must be separate tokens, as in `echo hi > out.txt`. The current implementation does not provide shell quoting or escaping, wildcard expansion, command substitution, here-documents, or shell scripting syntax.

Pipelines run in the foreground. Background execution is currently handled for a single command, not for an entire pipeline. Terminal process-group handoff is also not enabled yet; job tracking and `fg`/`bg` are implemented around the tracked child process.

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
    +-- Signal handlers
```

### Source layout

```text
include/                 Public headers
   builtins.h             Builtin command dispatch
   command.h              Parsed command representation
   environment.h          Variable expansion and storage
   executor.h             Process execution interface
   job_manager.h          Job records and state
   parser.h               Command-line parsing
   pipeline.h             Pipeline representation
   shell.h                REPL and shell state
   signals.h              Signal setup
src/
   main.cpp               Program entry point
   shell.cpp              Read-eval-print loop and history
   parser.cpp             Redirection, pipe, and background parsing
   executor.cpp           Process creation and execution
   buitins.cpp             Builtin implementations
   environment.cpp        Variable expansion
   job_manager.cpp        Job bookkeeping
   signals.cpp            SIGCHLD and interactive signal setup
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
- **Signals and jobs:** `signal()`, `sigaction()`, `kill()`, `SIGCHLD`
- **Terminal-related groundwork:** process-group fields and APIs are present in the design, but terminal ownership transfer is not currently active

## Learning Goals

This project is an exercise in POSIX systems programming and operating-system concepts, including process creation, parent-child synchronization, file descriptors, inter-process communication, signal delivery, environment state, and the boundary between shell builtins and external programs.

## Possible Next Steps

- Enable process groups and terminal ownership transfer with `setpgid()` and `tcsetpgrp()`.
- Make job control operate on complete pipelines.
- Add quoted strings, escaping, and more precise syntax errors.
- Add wildcard expansion, command substitution, and here-documents.
- Add tests for parsing, redirection, pipelines, and job state transitions.