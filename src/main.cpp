// MiniShell++ — Phase 1: REPL skeleton
//
// A shell is fundamentally a Read-Eval-Print Loop:
//   1. print a prompt
//   2. read a line of input
//   3. "evaluate" it (in this phase: just echo)
//   4. loop
//
// We exit on either:
//   - the user typing "exit"
//   - end-of-file on stdin (Ctrl+D in a terminal)

#include <iostream>
#include <string>

static const char* PROMPT = "minish> ";

int main() {
    std::string line;

    while (true) {
        // 1. Print the prompt.
        //    std::cout is line-buffered when connected to a terminal, so we
        //    must flush explicitly — otherwise the prompt appears AFTER the
        //    user's input on some setups.
        std::cout << PROMPT << std::flush;

        // 2. Read one line from stdin.
        //    std::getline returns the stream; the stream evaluates to false
        //    when it hits EOF (Ctrl+D) or a read error. That is how the
        //    loop terminates cleanly without an explicit "quit" command.
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';  // tidy newline so the next shell prompt is on its own line
            break;
        }

        // 3. "Evaluate" — for now, just echo. Later phases replace this
        //    with parsing + forking + executing.
        if (line == "exit") {
            break;
        }
        if (line.empty()) {
            continue;  // user just hit Enter — re-prompt
        }

        std::cout << "you said: " << line << '\n';
    }

    return 0;
}
