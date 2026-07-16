# Mini Unix Shell

A lightweight Unix shell built from scratch in **C++** using **POSIX system calls**. This project was developed to understand how Unix shells work internally by implementing process management, command execution, file descriptor manipulation, I/O redirection, and pipelines.

Unlike a toy shell that simply invokes `system()`, this shell directly interacts with the Linux kernel using `fork()`, `execvp()`, `waitpid()`, `pipe()`, `dup2()`, `open()`, and related system calls.

---

## Features

### Built-in Commands

- `cd <directory>`
- `cd -`
- `cd`
- `pwd`
- `exit`

---

### External Commands

Supports execution of all commands available in the system PATH.

Examples:

```bash
ls
ls -la
echo Hello
cat file.txt
grep main *.cpp
```

---

### Input / Output Redirection

Supports:

```bash
ls > files.txt
echo Hello >> log.txt
cat < input.txt
cat < input.txt > output.txt
```

Implemented using:

- `open()`
- `dup2()`
- `close()`

---

### Pipelines

Supports multi-stage pipelines.

Examples:

```bash
ls | grep cpp

cat README.md | grep Shell | wc

cat file.txt | grep hello | sort | uniq
```

Implemented using:

- `pipe()`
- `fork()`
- `dup2()`

---

## Architecture

```
                +------------------+
                |      Shell       |
                +---------+--------+
                          |
                          v
                +------------------+
                |      Parser      |
                +---------+--------+
                          |
                          v
                +------------------+
                |    Pipeline      |
                +---------+--------+
                          |
                          v
                +------------------+
                |    Executor      |
                +----+--------+----+
                     |        |
                     |        |
          +----------+        +----------+
          |                              |
          v                              v
   Built-in Commands             External Commands
```

---

## Project Structure

```
mini-shell/
│
├── include/
│   ├── shell.h
│   ├── parser.h
│   ├── pipeline.h
│   ├── command.h
│   ├── executor.h
│   └── builtins.h
│
├── src/
│   ├── main.cpp
│   ├── shell.cpp
│   ├── parser.cpp
│   ├── executor.cpp
│   └── builtins.cpp
│
├── CMakeLists.txt
└── README.md
```

---

## How It Works

### 1. Read-Eval-Print Loop (REPL)

The shell continuously:

1. Reads user input
2. Parses the command
3. Executes built-in commands directly
4. Executes external commands using `fork()` and `execvp()`

---

### 2. Command Parsing

The parser converts user input into an internal representation.

Example:

Input:

```bash
grep hello < input.txt > output.txt
```

Internal representation:

```
Command
├── program      : grep
├── arguments    : ["grep", "hello"]
├── inputFile    : input.txt
├── outputFile   : output.txt
└── append       : false
```

Multiple commands are represented as a `Pipeline`.

Example:

```bash
cat file.txt | grep hello | sort
```

becomes

```
Pipeline
├── Command(cat)
├── Command(grep)
└── Command(sort)
```

---

## Process Creation

External commands are executed using:

```
fork()
        |
        +------ Parent
        |
        +------ Child
                    |
                 execvp()
```

The parent waits for the child using:

```
waitpid()
```

---

## I/O Redirection

Output redirection works by changing the process' file descriptor table.

Normal execution:

```
stdout
   |
Terminal
```

After:

```bash
ls > files.txt
```

```
stdout
   |
files.txt
```

Implemented using:

```cpp
open()
dup2()
close()
```

---

## Pipelines

A command such as

```bash
cat file.txt | grep hello | sort
```

is executed as

```
cat ---> grep ---> sort
       pipe      pipe
```

Each stage runs in a separate child process.

The shell:

1. Creates a pipe
2. Forks a child
3. Redirects stdin/stdout using `dup2()`
4. Executes the command with `execvp()`

---

## POSIX APIs Used

### Process Management

- `fork()`
- `execvp()`
- `waitpid()`

### File Descriptors

- `dup2()`
- `open()`
- `close()`

### Inter-process Communication

- `pipe()`

### Filesystem

- `getcwd()`
- `chdir()`

---

## Build

Requirements:

- Linux
- GCC / Clang
- CMake ≥ 3.16

Build:

```bash
mkdir build
cd build
cmake ..
make
```

Run:

```bash
./mini-shell
```

---

## Example Session

```bash
mysh> pwd
/home/nishant/projects

mysh> ls

CMakeLists.txt
README.md
src

mysh> ls > files.txt

mysh> cat files.txt

CMakeLists.txt
README.md
src

mysh> cat README.md | grep Shell

Mini Unix Shell

mysh> echo Hello >> log.txt

mysh> cat log.txt

Hello
```

---

## Concepts Demonstrated

- Unix process model
- Process creation (`fork`)
- Program execution (`execvp`)
- Parent-child synchronization (`waitpid`)
- File descriptor manipulation
- Input/output redirection
- Inter-process communication using pipes
- Modular parser and executor design
- POSIX systems programming

---

## Future Improvements

Planned features:

- [ ] Background processes (`&`)
- [ ] Signal handling (`Ctrl+C`, `Ctrl+Z`)
- [ ] Job control (`jobs`, `fg`, `bg`)
- [ ] Command history
- [ ] Tab completion
- [ ] Environment variables (`export`, `$PATH`)
- [ ] Wildcard expansion (`*.cpp`)
- [ ] Quoted strings and escaping
- [ ] Here documents (`<<`)
- [ ] Proper shell scripting support

---

## Motivation

The objective of this project is to gain a deeper understanding of Unix internals by implementing a shell from first principles instead of relying on high-level abstractions. It serves as a practical exploration of operating systems concepts such as process management, file descriptors, and inter-process communication.

---

## License

This project is intended for educational purposes.