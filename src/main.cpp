// MiniShell++ — Phase 2: REPL + tokenizer
//
// We still don't execute anything yet — Phase 3 brings fork/exec.
// For now, we tokenize the input and print the resulting argv array so you
// can see exactly what the shell parsed.

#include <iostream>
#include <string>

#include "parser.h"

static const char* PROMPT = "minish> ";

int main() {
    std::string line;

    while (true) {
        std::cout << PROMPT << std::flush;

        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        auto tokens = minish::tokenize(line);
        if (tokens.empty()) {
            continue;  // empty input OR tokenize error (already reported)
        }

        // Sneak "exit" in as a built-in. Phase 4 formalizes built-ins.
        if (tokens[0] == "exit") {
            break;
        }

        std::cout << "[" << tokens.size() << " token(s)]\n";
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::cout << "  argv[" << i << "] = \"" << tokens[i] << "\"\n";
        }
    }

    return 0;
}
