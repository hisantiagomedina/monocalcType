#ifndef MONOCALC_LEXER_H
#define MONOCALC_LEXER_H

#include "monocalc/token.h"
#include "monocalc/error.h"

namespace monocalc {

struct LexerResult {
    bool ok;
    Token* tokens;
    size_t count;
    size_t capacity;
    ErrorCode err;
    size_t pos;
};

ErrorCode tokenize(const char* input, size_t len, Token* out, size_t capacity, size_t* outCount);

} // namespace monocalc

#endif // MONOCALC_LEXER_H
