# Phase 2 — The Tokenizer

## Goal

Take the raw line the user typed and chop it into the `argv`-style array that `execvp` will demand in Phase 3:

```text
minish> ls -la /tmp
[3 token(s)]
  argv[0] = "ls"
  argv[1] = "-la"
  argv[2] = "/tmp"
```

And do it correctly when the user uses quotes or escapes:

```text
minish> echo 'hello   world'   "with spaces"   no\ split
[4 token(s)]
  argv[0] = "echo"
  argv[1] = "hello   world"
  argv[2] = "with spaces"
  argv[3] = "no split"
```

---

## Why tokenization is its own phase

When you eventually call:

```cpp
execvp("ls", argv);
```

`argv` must be an array of C strings: `{"ls", "-la", "/tmp", nullptr}`. The user, however, typed a single string with embedded spaces. Splitting that string into the right pieces is **not** as simple as `string.split(" ")` because:

| Input | Naïve split | What you actually want |
|-------|-------------|------------------------|
| `ls   /tmp` | `["ls", "", "", "/tmp"]` | `["ls", "/tmp"]` (collapse runs of spaces) |
| `echo "hello world"` | `["echo", "\"hello", "world\""]` | `["echo", "hello world"]` (quotes group) |
| `echo no\ split` | `["echo", "no\\", "split"]` | `["echo", "no split"]` (backslash escapes the space) |
| `echo ''` | `["echo", "''"]` | `["echo", ""]` (empty quoted string is still an arg) |

These are exactly the cases the tokenizer in `src/parser.cpp` handles.

---

## The concept: tokenization is a state machine

A character means different things depending on what state you're in. Hitting a space is a token boundary when you're outside quotes — but it's just a literal character when you're inside a quoted string. The same goes for `'`, `"`, and `\`.

The cleanest way to encode this is as a tiny state machine:

```
                  ┌───── ' ─────→ ┌────────────┐ ── ' ──┐
                  │               │ IN_SINGLE  │        │
                  │               └────────────┘        │
                  │                                     │
   ┌──────────┐ ──┤                                     ↓
   │ UNQUOTED │   │               ┌────────────┐    ┌──────────┐
   │ (collect │ ──┴──── " ──────→ │ IN_DOUBLE  │ ── │ UNQUOTED │
   │   chars) │                   └────────────┘ "  └──────────┘
   └──────────┘
         │
         │   space/tab outside quotes  →  push current token, return to start
         │   backslash                  →  set "escape pending"; next char is literal
```

State variables in the code:

| Variable          | Meaning |
|-------------------|---------|
| `current`         | The token we're currently building |
| `in_token`        | True if we've started building a token (separate from `current.empty()` so that `""` still pushes an empty token) |
| `escape_pending`  | True if the previous unescaped char was `\` |
| `quote`           | One of `None` / `Single` / `Double` |

At end-of-input we validate: a still-pending escape or an unclosed quote is an error.

---

## OS concepts you meet in this phase

There's actually no new OS concept here — tokenization is pure C++ string manipulation. But it's the bridge between "user typed something" and "we can talk to the kernel." Two ideas you should internalize:

### 1. The `argv` convention

Every Unix program receives its arguments as `(int argc, char** argv)`. Conventions:

- `argv[0]` is the program name (often, but not always, the path used to invoke it).
- `argv[1..argc-1]` are the actual arguments.
- `argv[argc]` is `nullptr` — that terminator is mandatory for `execvp`, and forgetting it is a classic shell bug.

Right now we store tokens in a `std::vector<std::string>`. Phase 3 will convert that into a `char* const argv[]` (with the trailing `nullptr`) right before calling `execvp`.

### 2. Why single quotes can't contain anything special

In POSIX shells, single quotes are *completely* literal — no escape, no variable expansion, nothing. That's so users have an escape hatch when they want a string with backslashes, dollar signs, double quotes, etc. We honor that: even `\` is literal inside `'...'`.

Try it once we're done:

```text
minish> echo 'a\nb'
  argv[1] = "a\nb"     ← literal four characters, not a newline
```

---

## Walkthrough: `src/parser.h`

```cpp
namespace minish {
    std::vector<std::string> tokenize(const std::string& line);
}
```

A single free function returning a vector of tokens. We put it in the `minish` namespace so that as we add more shell modules (`executor`, `builtins`, ...) they don't pollute each other.

The header comment explicitly lists what's supported and what's not — that's worth doing for every module so future-you (or a reader) knows the contract.

---

## Walkthrough: `src/parser.cpp`

### The anonymous namespace

```cpp
namespace {
    bool is_shell_space(char c) { return c == ' ' || c == '\t'; }
    enum class QuoteState { None, Single, Double };
}
```

`namespace { ... }` at file scope is the modern C++ replacement for `static`. Nothing inside leaks to other translation units. `enum class` gives us a strongly-typed enum so we can't accidentally compare a `QuoteState` to an int.

### State setup

```cpp
std::vector<std::string> tokens;
std::string current;
bool in_token = false;
bool escape_pending = false;
QuoteState quote = QuoteState::None;
```

Five state variables. The split between `in_token` and `current.empty()` is the subtle one — it's what lets `""` become a real (empty) token instead of being dropped.

### Branch 1: a pending escape consumes the current char

```cpp
if (escape_pending) {
    current += c;
    in_token = true;
    escape_pending = false;
    continue;
}
```

Whatever this character is — space, quote, newline-as-typed — append it literally. Note we set `in_token = true` even if the escape sequence ends up adding a "weird" character; the user clearly meant a token here.

### Branch 2: inside single quotes — literally everything

```cpp
if (quote == QuoteState::Single) {
    if (c == '\'') quote = QuoteState::None;
    else            current += c;
    continue;
}
```

The *only* character that means anything inside single quotes is the closing `'`. There's no way to put a literal `'` inside single quotes in POSIX shells — you have to close them, escape it, and reopen: `'it'\''s'`. We inherit that limitation, and it's correct.

### Branch 3: inside double quotes — almost literal

```cpp
if (quote == QuoteState::Double) {
    if (c == '"')                                   quote = QuoteState::None;
    else if (c == '\\' && i + 1 < line.size()
             && (line[i+1] == '"' || line[i+1] == '\\'))
        escape_pending = true;
    else
        current += c;
    continue;
}
```

Two characters are special inside `"..."`: the closing `"` and a backslash *if* it's about to escape a `"` or another `\`. Any other backslash is kept literal — that's POSIX behavior: `"\n"` is a backslash followed by an `n`, not a newline.

### Branch 4: the unquoted state

```cpp
if (c == '\\')   { escape_pending = true; continue; }
if (c == '\'')   { quote = QuoteState::Single; in_token = true; continue; }
if (c == '"')    { quote = QuoteState::Double; in_token = true; continue; }
if (is_shell_space(c)) {
    if (in_token) {
        tokens.push_back(std::move(current));
        current.clear();
        in_token = false;
    }
    continue;
}
current += c;
in_token = true;
```

Whitespace is the *only* thing that ends a token. We use `std::move(current)` to hand the storage to the vector without copying — a micro-optimization but also good habit.

### End-of-input validation

```cpp
if (escape_pending) { /* error */ return {}; }
if (quote != QuoteState::None) { /* error */ return {}; }
if (in_token) tokens.push_back(std::move(current));
```

Three terminal conditions:

1. The line ended with `\` and nothing to escape — error.
2. The line ended inside a quote — error.
3. Otherwise, push whatever token we were building.

Returning an empty vector on error lets the REPL handle errors uniformly with "empty input" — no special exception machinery.

---

## Walkthrough: `src/main.cpp` updates

```cpp
auto tokens = minish::tokenize(line);
if (tokens.empty()) continue;
if (tokens[0] == "exit") break;

std::cout << "[" << tokens.size() << " token(s)]\n";
for (size_t i = 0; i < tokens.size(); ++i) {
    std::cout << "  argv[" << i << "] = \"" << tokens[i] << "\"\n";
}
```

Two changes from Phase 1:

1. **`exit` check moved** until after tokenization, so `  exit  ` (with spaces) still exits.
2. **Echo** the parsed tokens with their indices, formatted to look like an `argv` array. This is the "see what the shell understood" output that helps you debug the tokenizer interactively.

---

## Try it

```bash
make
./build/minish
```

Try each of these and look at the output:

| Input | What you should see |
|-------|---------------------|
| `ls -la /tmp` | 3 tokens |
| `   ls    -la   ` (extra whitespace) | still 2 tokens, no empties |
| `echo "hello world"` | 2 tokens; the second is `hello world` |
| `echo 'a   b'` | 2 tokens; the second is `a   b` |
| `echo no\ split` | 2 tokens; the second is `no split` |
| `echo "she said \"hi\""` | 2 tokens; the second is `she said "hi"` |
| `echo 'a\nb'` | 2 tokens; the second is the 4 literal chars `a\nb` |
| `echo ""` | 2 tokens; the second is the empty string |
| `echo "unclosed` (intentional) | error: unterminated double quote, no tokens |
| `echo trailing\` (intentional) | error: dangling backslash, no tokens |

### Why typing `ls` doesn't actually list anything

We just *parsed* the command — we didn't run it. That's Phase 3.

### Things that look like operators but currently get swallowed into a token

Type:

```text
minish> echo hi > out.txt
```

You'll see four tokens: `echo`, `hi`, `>`, `out.txt`. The `>` is just an ordinary character to the tokenizer right now. In Phase 5 we'll add a *second* pass over the token list to recognize `>` `<` `>>` `|` `&` `;` as separate syntactic elements and strip them out of `argv`.

That two-pass design (lex → syntactic recognition) mirrors how real compilers and shells are structured.

---

## What's NOT here yet

- No variable expansion (`$HOME`, `$?`).
- No globbing (`*.txt`).
- No tilde expansion (`~`).
- No recognition of `|`, `<`, `>`, `>>`, `&`, `;` as operators — they're just characters.
- No execution. Type `ls` and nothing runs.

---

→ **Next:** Phase 3 — `fork`, `execvp`, `waitpid`. We finally make the shell *do* something.
