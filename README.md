# MiniShell++

A lightweight Unix-style shell written in C++

## Build & run

Build inside **WSL Ubuntu** (POSIX system calls don't exist on native Windows).

```bash
# from WSL, navigate to the project (your Windows drive is mounted at /mnt/c)
cd /mnt/c/Users/eveli/OneDrive/Desktop/miniUnixShell

# build
make

# run
./build/minish
```

To exit the shell: type `exit` or press `Ctrl+D` (EOF).

## Project layout

```
miniUnixShell/
├── Makefile
├── README.md
├── src/         # C++ source
└── docs/        # one markdown per phase: concept + code walkthrough
```

## Phases

| # | Phase | New concepts |
|---|-------|--------------|
| 1 | REPL skeleton | stdin/stdout, EOF, line buffering |
| 2 | Tokenizer | Lexing, `argv` conventions, quoting |
| 3 | Process execution | `fork`, `execvp`, `waitpid` |
| 4 | Built-ins | Why `cd` must run in the parent |
| 5 | I/O redirection | File descriptors, `dup2` |
| 6 | Pipelines | `pipe`, descriptor inheritance |
| 7 | Signals | `sigaction`, async-signal-safe code |
| 8 | Background jobs | Process groups, `SIGCHLD`, `tcsetpgrp` |

Start reading at [docs/phase-01-repl.md](docs/phase-01-repl.md).
