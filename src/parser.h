// MiniShell++ — Phase 2: Tokenizer
//
// Turns a raw input line into a list of shell-style tokens.
//
// Rules supported in this phase:
//   * Whitespace (space, tab) separates tokens
//   * 'single quotes' preserve everything literally until the next '
//   * "double quotes" preserve everything literally, except:
//       \"   -> a literal "
//       \\   -> a literal backslash
//   * \X outside quotes -> a literal X (lets you escape a space, etc.)
//
// NOT yet handled (later phases):
//   * variable expansion ($HOME, $?)
//   * special operators (| < > >> & ;) — those become their own token type in Phase 5/6
//   * comments (#)
//
// On error (unterminated quote / dangling escape), prints a diagnostic to
// stderr and returns an empty vector.

#pragma once

#include <string>
#include <vector>

namespace minish {

std::vector<std::string> tokenize(const std::string& line);

}  // namespace minish
