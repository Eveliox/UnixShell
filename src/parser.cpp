#include "parser.h"

#include <iostream>

namespace minish {

namespace {

bool is_shell_space(char c) {
    return c == ' ' || c == '\t';
}

enum class QuoteState { None, Single, Double };

}  // namespace

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;

    // in_token is separate from "current is non-empty" so that bare "" or ''
    // still produces one empty token, matching real shell behavior.
    bool in_token = false;
    bool escape_pending = false;
    QuoteState quote = QuoteState::None;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        // 1. If the previous char was an unescaped backslash, this char is literal.
        if (escape_pending) {
            current += c;
            in_token = true;
            escape_pending = false;
            continue;
        }

        // 2. Inside single quotes: everything is literal until the closing '.
        //    Not even backslash escapes — that matches POSIX shells.
        if (quote == QuoteState::Single) {
            if (c == '\'') {
                quote = QuoteState::None;
            } else {
                current += c;
            }
            continue;
        }

        // 3. Inside double quotes: literal, but \" and \\ are escapes.
        if (quote == QuoteState::Double) {
            if (c == '"') {
                quote = QuoteState::None;
            } else if (c == '\\' && i + 1 < line.size()
                       && (line[i + 1] == '"' || line[i + 1] == '\\')) {
                escape_pending = true;
            } else {
                current += c;
            }
            continue;
        }

        // 4. Unquoted state.
        if (c == '\\') {
            escape_pending = true;
            continue;
        }
        if (c == '\'') {
            quote = QuoteState::Single;
            in_token = true;
            continue;
        }
        if (c == '"') {
            quote = QuoteState::Double;
            in_token = true;
            continue;
        }
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
    }

    if (escape_pending) {
        std::cerr << "minish: dangling backslash at end of input\n";
        return {};
    }
    if (quote != QuoteState::None) {
        std::cerr << "minish: unterminated "
                  << (quote == QuoteState::Single ? "single" : "double")
                  << " quote\n";
        return {};
    }
    if (in_token) {
        tokens.push_back(std::move(current));
    }

    return tokens;
}

}  // namespace minish
