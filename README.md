# mini-shell

A small Unix shell written in C++20 on top of raw POSIX system calls. It runs programs, connects pipelines, redirects input and output, expands variables, and does real job control: process groups, terminal handoff, Ctrl+C / Ctrl+Z, `fg` and `bg`.

It's a learning project. The goal is to show clearly how a shell works underneath, not to replace bash.

```console
$ ./build/mini-shell
mysh> echo hello > greeting.txt
mysh> cat greeting.txt | tr a-z A-Z
HELLO
mysh> sleep 30 &
[1] 48211
mysh> vim notes.txt
^Z
[2] Stopped vim notes.txt
mysh> jobs
[1] Running sleep 30
[2] Stopped vim notes.txt
mysh> fg 2
```

## Contents

- [Build and run](#build-and-run)
- [Features](#features)
- [How it works](#how-it-works)
- [Project layout](#project-layout)
- [Limitations](#limitations)
- [Next steps](#next-steps)

## Build and run

You need Linux (or another POSIX system), a C++20 compiler, CMake 3.16 or newer, and the GNU Readline development headers. On Debian or Ubuntu:

```bash
sudo apt install build-essential cmake libreadline-dev
```

Build and start the shell from the repository root:

```bash
cmake -S . -B build
cmake --build build
./build/mini-shell
```

To leave, type `exit` or press Ctrl+D.

## Features

### Running commands

Any program on your `PATH` runs as you'd expect. If a command can't be found, the shell prints `mysh: <name>: command not found`.

### Builtins

| Command | What it does |
| --- | --- |
| `cd [dir]` | Change directory. With no argument, go to `$HOME`. |
| `cd -` | Go back to the previous directory and print it. |
| `pwd` | Print the current directory. |
| `export NAME=value` | Set a variable for the shell and for the programs it starts. |
| `unset NAME` | Remove a variable. |
| `jobs` | List background and stopped jobs. |
| `fg [id]` | Bring a job to the foreground. Accepts `2` or `%2`. With no id, uses the most recent job. |
| `bg [id]` | Resume a stopped job in the background. |
| `history` | Show the command history. |
| `exit` | Quit the shell. |

A builtin on its own line runs inside the shell itself, which is how `cd` and `export` can change the shell's state. In a pipeline or with `&`, a builtin runs in a child process like any other command. So `history | grep git` works, but `cd /tmp | cat` won't change the shell's directory. bash behaves the same way.

### Redirection

```bash
ls > files.txt                  # write to a file (create or overwrite)
echo done >> log.txt            # append to a file
sort < names.txt                # read from a file
grep main < app.cpp > hits.txt  # both at once
pwd>where.txt                   # spaces around operators are optional
```

Redirection works for builtins too: `pwd > where.txt` writes to the file.

### Pipelines

Pipelines can have any number of stages:

```bash
cat README.md | grep shell | wc -l
seq 3 -1 1 | sort -n
```

### Variables

`$NAME` and `${NAME}` are replaced anywhere in a word, including in redirection filenames:

```bash
export PROJECT=mini-shell
echo $HOME/code/$PROJECT
echo ${PROJECT}-backup > $HOME/name.txt
```

An undefined variable expands to nothing. A word that ends up empty is dropped, so `echo a $UNDEFINED b` prints `a b`.

### Job control

Add `&` to run a command, or a whole pipeline, in the background:

```bash
sleep 60 &
find /usr/include -name stdio.h | wc -l &
```

- **Ctrl+C** stops only the program in the foreground. It never kills the shell or your background jobs.
- **Ctrl+Z** pauses the foreground job and returns you to the prompt. `fg` resumes it, and `bg` lets it keep running in the background.
- **Notifications:** when a background job finishes or stops, the shell tells you before the next prompt, for example `[1] Done sleep 60`.
- **Terminal input:** a background job that tries to read from the terminal is paused automatically. Bring it back with `fg` to type into it.
- **Full-screen programs:** editors like `vim` can be paused and resumed. Their terminal settings are saved and restored each time.

Job control is on when the shell is started from a terminal. If you pipe commands into it, for example `echo ls | ./build/mini-shell`, commands still run normally but the shell doesn't take control of the terminal.

### Line editing and history

GNU Readline provides arrow-key editing and Up/Down history. History is saved to `~/.mysh_history` when you exit. Only the most recent 1000 entries are kept.

### Syntax errors

Malformed lines are rejected before anything runs:

```console
mysh> ls |
mysh: syntax error: missing command after '|'
mysh> cat <
mysh: syntax error: expected filename after '<'
mysh> | wc
mysh: syntax error near unexpected token `|'
```

## How it works

Each line goes through the same steps:

```text
 readline ──► Parser ──► variable expansion ──► builtin? ──yes──► run in the shell
                                                    │
                                                    no
                                                    ▼
                                               Executor ──► fork one child per stage
                                                    │       connect them with pipes
                                                    │       apply < > >>
                                                    │       execvp()
                                                    ▼
                                               JobManager ──► wait in the foreground
                                                              or track in the background
```

**Parsing.** `Parser` splits the line into words and the operators `|`, `&`, `<`, `>` and `>>`, then builds a `Pipeline` made of `Command`s. Syntax is checked at this point, so a bad line never starts any processes.

**Starting processes.** `Executor` forks a child for each stage of the pipeline and connects neighbouring stages with `pipe()` and `dup2()`. Redirections are applied with `open()` and `dup2()`, then the child calls `execvp()`. All stages join one process group, led by the first child.

**Job control.** For a foreground job, `JobManager` hands the terminal to the job's process group with `tcsetpgrp()`. Keyboard signals such as Ctrl+C and Ctrl+Z then go to that job and not to the shell. The shell waits with `waitpid(..., WUNTRACED)`, which returns when the job exits *or* stops. Afterwards the shell takes the terminal back and restores its terminal settings. `fg` and `bg` resume a job by sending `SIGCONT` to its whole process group.

**Signals.** The shell ignores `SIGINT`, `SIGQUIT`, `SIGTSTP`, `SIGTTIN` and `SIGTTOU`, and each child resets them to their defaults before `exec`. There's deliberately no `SIGCHLD` handler. Changing the job list inside a signal handler isn't async-signal-safe, so background jobs are collected instead with a non-blocking `waitpid(-1, ..., WNOHANG)` before each prompt.

**System calls used:** `fork`, `execvp`, `waitpid`, `pipe`, `dup`, `dup2`, `open`, `close`, `chdir`, `getcwd`, `kill`, `signal`, `setpgid`, `getpgrp`, `tcgetpgrp`, `tcsetpgrp`, `tcgetattr`, `tcsetattr`.

## Project layout

```text
include/
  command.h         One command: arguments and redirections
  pipeline.h        Commands joined by |, plus the background flag
  parser.h          Turns a line of text into a Pipeline
  environment.h     Variable storage and $NAME expansion
  builtins.h        Builtin commands
  executor.h        Starts processes for a pipeline
  job_manager.h     Jobs, process groups, terminal ownership
  signals.h         Signal setup for the shell and its children
  shell.h           The read-eval loop that ties it all together
src/
  main.cpp          Entry point
  shell.cpp         Main loop, expansion, builtin dispatch, history
  parser.cpp        Tokenizer and syntax checks
  environment.cpp   Variable expansion
  builtins.cpp      cd, pwd, export, unset, jobs, fg, bg, history, exit
  executor.cpp      fork/exec, pipes, redirection
  job_manager.cpp   Foreground waits, background reaping, fg/bg support
  signals.cpp       Ignoring and restoring job-control signals
```

## Limitations

This is a deliberately small shell. It doesn't support:

- quoting or escaping (`"a b"`, `'$HOME'`, `\ `)
- wildcards (`*.txt`), command substitution (`$(...)`) or here-documents
- `&&`, `||`, `;`, or `$?` (exit statuses aren't tracked)
- stderr redirection (`2>`, `2>&1`)
- scripts, functions, aliases or control flow

## Next steps

- Track exit statuses, then add `$?`, `&&`, `||` and `;`.
- Add stderr redirection.
- Add quoting and escaping.
- Add wildcard expansion and command substitution.
- Add automated tests for the parser, redirection, pipelines and job states.
