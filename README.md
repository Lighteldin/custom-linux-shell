<a id="readme-top"></a>
<br />
<div align="center">
  <h3 align="center">CUSTOM LINUX SHELL</h3>
</div>

## Overview

A custom shell implemented in C that supports command execution, input/output redirection,
piping, background processes, and built-in commands such as: **cd**, **pwd**, and **history**.

## Features

- Execute external commands using fork and execvp
- Built-in commands:
  - cd (with support for `~` and `-`)
  - pwd
  - history
  - exit
- Input and output redirection:
  - `>` (output)
  - `<` (input)
- Single pipe support (`|`)
- Background execution using `&`
- Command history tracking
- Signal handling (Ctrl+C, Ctrl+Z behavior)

## Installation

### 1. Clone repository

```bash
git clone https://https://github.com/Lighteldin/custom-linux-shell
cd custom-linux-shell
```

### 2. Run the Makefile while inside the project directory

```bash
make
```

## Usage / Examples

### Run the shell

```bash
./myShell
```

### Run commands

```bash
ls
pwd
cd ..
cat file.txt
ls > output.txt
cat < input.txt
ls | grep .c
sleep 5 &
```

## Authors

- [@Lighteldin](https://github.com/Lighteldin)
- [@MohamedElsayed75](https://github.com/MohamedElsayed75)

Project Link: [https://github.com/Lighteldin/custom-linux-shell](https://github.com/Lighteldin/custom-linux-shell)
