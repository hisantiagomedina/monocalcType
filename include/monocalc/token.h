#ifndef MONOCALC_TOKEN_H
#define MONOCALC_TOKEN_H

#include <cstddef>

namespace monocalc {

enum class TokenType {
    EOF_,
    Number,
    Ident,
    Plus,
    Minus,
    Star,
    Slash,
    Caret,
    LParen,
    RParen,
    Comma,
    Unknown
};

struct Token {
    TokenType type;
    double number;      // valid if type == Number
    char ident[16];     // valid if type == Ident
    size_t pos;         // position in input string
};

} // namespace monocalc

#endif // MONOCALC_TOKEN_H
