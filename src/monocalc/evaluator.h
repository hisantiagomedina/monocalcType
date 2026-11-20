#ifndef MONOCALC_EVALUATOR_H
#define MONOCALC_EVALUATOR_H

#include "monocalc/ast.h"
#include "monocalc/error.h"

namespace monocalc {

struct EvalResult {
    bool ok;
    MONOCALC_REAL value;
    ErrorCode err;
};

EvalResult evaluate(const Arena& arena, NodeId root, MONOCALC_REAL x);

} // namespace monocalc

#endif // MONOCALC_EVALUATOR_H
