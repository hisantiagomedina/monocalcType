#ifndef MONOCALC_TYPE_H
#define MONOCALC_TYPE_H

#include "monocalc/error.h"
#include "monocalc/ast.h"
#include "monocalc/evaluator.h"
#include "monocalc/domain.h"
#include "monocalc/limiter.h"

#ifndef MONOCALC_REAL
#define MONOCALC_REAL double
#endif

namespace monocalc {

// Platform-aware defaults: smaller for AVR, larger for PC
#if defined(__AVR__)
static const size_t DEFAULT_ARENA_CAPACITY = 64;
static const size_t DEFAULT_TOKEN_CAPACITY = 32;
static const size_t MAX_INPUT_LEN = 64;
#else
static const size_t DEFAULT_ARENA_CAPACITY = 256;
static const size_t DEFAULT_TOKEN_CAPACITY = 128;
static const size_t MAX_INPUT_LEN = 256;
#endif

struct Function {
    Arena arena;
    NodeId root;
    Domain domain;
    char input[MAX_INPUT_LEN]; // store original input for debugging
    size_t inputLen;
};

struct ParseResult {
    bool ok;
    Function func;
    ErrorCode err;
    size_t pos;
};

ParseResult parse(const char* input, size_t len, Function& out);
EvalResult compute(const Function& f, MONOCALC_REAL x);
ErrorCode derive(const Function& f, Function& out);
ErrorCode integrate(const Function& f, Function& out);
EvalResult integrateDefinite(const Function& f, MONOCALC_REAL a, MONOCALC_REAL b);
LimitResult lim(const Function& f, MONOCALC_REAL x0, LimitDirection dir);

} // namespace monocalc

#endif // MONOCALC_TYPE_H
