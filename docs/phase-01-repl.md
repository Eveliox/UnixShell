# Phase 1 — The REPL Skeleton

## Goal

Get a shell prompt on screen, read a line of input, print it back, and exit cleanly. No processes yet — just the *loop* that every shell is built around.

By the end you'll be able to:

```text
minish> hello world
you said: hello world
minish> exit
$
```

---

## The concept: a shell is a REPL

Every interactive shell — `bash`, `zsh`, `fish`, `cmd.exe`, even Python's interpreter — is a **Read-Eval-Print Loop**:

```
┌─────────────────────────────────────────┐
│  print prompt   →   read line           │
│        ▲                  │             │
│        │                  ▼             │
│   loop forever  ←   evaluate + print    │
└─────────────────────────────────────────┘
```

In a real shell, "evaluate" means:

1. **Parse** the line into a command and arguments
2. **Fork** a child process
3. **Exec** the requested program in the child
4. **Wait** for the child to finish
5. Print any output the child produced

For Phase 1 we replace steps 1–5 with a placeholder (just echo the line). Later phases peel back each step one at a time.

---

## OS concepts you meet in this phase

### 1. Standard streams (stdin, stdout, stderr)

Every Unix process is born with three open **file descriptors**:

| fd | name   | C++ wrapper      | Default attached to |
|----|--------|------------------|---------------------|
| 0  | stdin  | `std::cin`       | the terminal's keyboard |
| 1  | stdout | `std::cout`      | the terminal's screen   |
| 2  | stderr | `std::cerr`      | the terminal's screen   |

A "file descriptor" is just a small integer the kernel uses to look up an entry in your process's open-files table. You'll meet this table in detail in Phase 5 (redirection). For now: writing to `std::cout` writes to fd 1, reading from `std::cin` reads from fd 0.

### 2. Line buffering

When `std::cout` is connected to a terminal, the C++ runtime **buffers** output and only flushes it when:

- you write `'\n'` or `std::endl`, OR
- you flush explicitly with `<< std::flush`, OR
- the program exits normally.

A prompt like `minish> ` has *no* newline at the end (you want the cursor on the same line as the prompt). Without an explicit flush, the prompt can sit invisibly in a buffer while `std::getline` is already waiting for input. That's why we write:

```cpp
std::cout << PROMPT << std::flush;
```

This is a real bug in beginner shells — try removing the `std::flush` and see what happens.

### 3. EOF (end-of-file)

In a terminal, pressing **Ctrl+D** on an empty line sends an EOF signal to the program reading stdin. There is no "EOF character" stored in the input — the kernel just tells the reader "this stream is done."

`std::getline(std::cin, line)` returns a reference to `std::cin`, which converts to `false` when the stream hits EOF or an error. That's how:

```cpp
if (!std::getline(std::cin, line)) { break; }
```

cleanly terminates the shell when the user presses Ctrl+D. Without this, your shell would loop forever reading empty strings.

---

## Walkthrough: `src/main.cpp`

```cpp
#include <iostream>
#include <string>

static const char* PROMPT = "minish> ";
```

`PROMPT` is just a constant we'll reuse. `static` here means *internal linkage* — the name doesn't escape this translation unit. It would matter if we had multiple `.cpp` files defining `PROMPT`; even without that risk, it's a good habit.

```cpp
int main() {
    std::string line;
```

One `std::string` reused across iterations. `std::getline` will reallocate it if needed.

```cpp
    while (true) {
        std::cout << PROMPT << std::flush;
```

Infinite loop. Print the prompt, then flush so it appears *before* we block on input.

```cpp
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }
```

Read one line. If the stream reports EOF (Ctrl+D) or an error, print a newline so the next thing the user's terminal shows (their actual shell's prompt) doesn't land mid-line, then exit the loop.

> **Note:** `std::getline` consumes the trailing `\n` but does not store it. `line` contains exactly what the user typed.

```cpp
        if (line == "exit") {
            break;
        }
        if (line.empty()) {
            continue;
        }
```

Two early-outs:
- typing `exit` is the polite way to leave;
- empty input (the user just hit Enter) re-prompts without printing anything.

```cpp
        std::cout << "you said: " << line << '\n';
    }
    return 0;
}
```

The "evaluate" step, placeholder version. Phase 3 replaces this with a fork + exec.

---

## The build system

The `Makefile` at the project root does three things:

1. **Compiles every `.cpp` in `src/`** to a `.o` in `build/`.
2. **Links** them into `build/minish`.
3. Provides `make run` (build and run) and `make clean` (delete `build/`).

The interesting flags:

| Flag | Why |
|------|-----|
| `-Wall -Wextra -Wpedantic` | Turn on warnings. Catches uninitialized variables, suspicious casts, etc. |
| `-std=c++17` | Modern C++ but still widely supported. |
| `-O0` | No optimization. Makes debugging in `gdb` predictable. |
| `-g` | Embed debug symbols so `gdb` can show source lines. |

---

## Try it

```bash
cd /mnt/c/Users/eveli/OneDrive/Desktop/miniUnixShell
make
./build/minish
```

Things to experiment with:

1. Type some text, press Enter — see the echo.
2. Press Enter on an empty line — should just re-prompt.
3. Type `exit` — should drop you back to your real shell.
4. Press **Ctrl+D** on an empty line — should also exit cleanly.
5. **Bug hunt:** delete `std::flush` from the prompt line, rebuild, run. What happens? (Behavior depends on your terminal, but the prompt may appear out of order.)
6. Pipe input in: `echo hello | ./build/minish`. The shell reads "hello", echoes it, then sees EOF and exits. This works because stdin is just an fd — it doesn't have to be a keyboard.

---

## What's NOT here yet

- No tokenization (the input is one opaque string).
- No process creation — we can't actually run `ls`.
- No file descriptors used directly.
- No signal handling — Ctrl+C kills the shell instead of the foreground command (we don't have one yet).

All of that comes next.

→ **Next:** [Phase 2 — Tokenizer](phase-02-parsing.md)
