#include "monocalc/lexer.h"
#include "monocalc/error.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

namespace monocalc {

static bool isIdentChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static void copyIdent(const char* s, size_t len, char* out, size_t outCap) {
    size_t n = len < outCap - 1 ? len : outCap - 1;
    memcpy(out, s, n);
    out[n] = '\0';
}

static bool isKnownFunc(const char* name, size_t len) {
    // sin, cos, exp, ln, abs
    if (len == 3) {
        if (memcmp(name, "sin", 3) == 0) return true;
        if (memcmp(name, "cos", 3) == 0) return true;
        if (memcmp(name, "exp", 3) == 0) return true;
        if (memcmp(name, "abs", 3) == 0) return true;
    }
    if (len == 2 && memcmp(name, "ln", 2) == 0) return true;
    return false;
}

ErrorCode tokenize(const char* input, size_t len, Token* out, size_t capacity, size_t* outCount) {
    size_t i = 0;
    size_t count = 0;

    while (i < len) {
        // Skip whitespace
        if (input[i] == ' ' || input[i] == '\t' || input[i] == '\n' || input[i] == '\r') {
            i++;
            continue;
        }

        if (count >= capacity) {
            if (outCount) *outCount = 0;
            return ErrorCode::ResourceExhausted;
        }

        Token& tok = out[count++];
        tok.pos = i;
        tok.number = 0.0;
        tok.ident[0] = '\0';

        // Number: digits, optional decimal, optional exponent
        if ((input[i] >= '0' && input[i] <= '9') || (input[i] == '.' && i + 1 < len && input[i+1] >= '0' && input[i+1] <= '9')) {
            tok.type = TokenType::Number;
            size_t start = i;
            // Integer part
            while (i < len && input[i] >= '0' && input[i] <= '9') i++;
            // Decimal part
            if (i < len && input[i] == '.') {
                i++;
                while (i < len && input[i] >= '0' && input[i] <= '9') i++;
            }
            // Exponent: e/E followed by optional +/- and at least one digit
            if (i < len && (input[i] == 'e' || input[i] == 'E')) {
                size_t epos = i;
                i++;
                if (i < len && (input[i] == '+' || input[i] == '-')) i++;
                if (i < len && input[i] >= '0' && input[i] <= '9') {
                    // Valid scientific notation — consume exponent digits
                    while (i < len && input[i] >= '0' && input[i] <= '9') i++;
                } else {
                    // Not scientific notation; back up and treat 'e' as identifier
                    i = epos;
                    // Fall through to identifier handling below
                    goto handle_ident;
                }
            }
            tok.number = strtod(input + start, nullptr);
            continue;
        }

handle_ident:
        // Identifier or function name
        if ((input[i] >= 'a' && input[i] <= 'z') || (input[i] >= 'A' && input[i] <= 'Z') || input[i] == '_') {
            size_t start = i;
            while (i < len && isIdentChar(input[i])) i++;
            size_t identLen = i - start;

            // Check for implicit multiplication: number followed by ident without *
            // This is detected by checking if previous token was a number and there was no operator
            if (identLen == 1 && input[start] == 'x') {
                tok.type = TokenType::Ident;
                copyIdent(input + start, identLen, tok.ident, sizeof(tok.ident));
                continue;
            }

            if (isKnownFunc(input + start, identLen)) {
                tok.type = TokenType::Ident;
                copyIdent(input + start, identLen, tok.ident, sizeof(tok.ident));
                continue;
            }

            // Unknown identifier
            tok.type = TokenType::Unknown;
            copyIdent(input + start, identLen, tok.ident, sizeof(tok.ident));
            if (outCount) *outCount = 0;
            return ErrorCode::SyntaxError;
        }

        // Operators and punctuation
        switch (input[i]) {
            case '+': tok.type = TokenType::Plus; i++; break;
            case '-': tok.type = TokenType::Minus; i++; break;
            case '*': tok.type = TokenType::Star; i++; break;
            case '/': tok.type = TokenType::Slash; i++; break;
            case '^': tok.type = TokenType::Caret; i++; break;
            case '(': tok.type = TokenType::LParen; i++; break;
            case ')': tok.type = TokenType::RParen; i++; break;
            case ',': tok.type = TokenType::Comma; i++; break;
            default:
                tok.type = TokenType::Unknown;
                i++;
                if (outCount) *outCount = 0;
                return ErrorCode::SyntaxError;
        }
    }

    // Add EOF token
    if (count >= capacity) {
        if (outCount) *outCount = 0;
        return ErrorCode::ResourceExhausted;
    }
    Token& eofTok = out[count];
    eofTok.type = TokenType::EOF_;
    eofTok.pos = i;
    eofTok.number = 0.0;
    eofTok.ident[0] = '\0';
    count++;

    if (outCount) *outCount = count;
    return ErrorCode::OK;
}

} // namespace monocalc
