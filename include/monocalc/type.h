#ifndef MONOCALC_TYPE_H
#define MONOCALC_TYPE_H

#include "error.h"
#include "ast.h"
#include "evaluator.h"
#include "domain.h"
#include "limiter.h"

#ifndef MONOCALC_REAL
#define MONOCALC_REAL double
#endif

namespace monocalc {

static const size_t DEFAULT_ARENA_CAPACITY = 256;
static const size_t DEFAULT_TOKEN_CAPACITY = 128;

struct Function {
    Arena arena;
    NodeId root;
    Domain domain;
    char input[256]; // store original input for debugging
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
LimitResult lim(const Function& f, MONOCALC_REAL x0, LimitDirection dir);

} // namespace monocalc

#endif // MONOCALC_TYPE_H
