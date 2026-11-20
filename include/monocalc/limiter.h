#ifndef MONOCALC_LIMITER_H
#define MONOCALC_LIMITER_H

#include "ast.h"
#include "error.h"
#include "evaluator.h"

#ifndef MONOCALC_REAL
#define MONOCALC_REAL double
#endif

namespace monocalc {

enum class LimitDirection {
    Both,
    Left,
    Right
};

struct LimitResult {
    bool ok;
    MONOCALC_REAL value;
    ErrorCode err;
};

LimitResult limit(const Arena& arena, NodeId root, MONOCALC_REAL x0, LimitDirection dir);

} // namespace monocalc

#endif // MONOCALC_LIMITER_H
