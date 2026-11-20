#include "monocalc/limiter.h"
#include "monocalc/evaluator.h"
#include "monocalc/error.h"
#include <math.h>
#include <stdlib.h>

namespace monocalc {

// Numerical limit estimation using progressively smaller steps.
// Returns the limit value if convergent, or indicates divergence/non-convergence.
static LimitResult limitOneSide(const Arena& arena, NodeId root, MONOCALC_REAL x0, bool fromLeft) {
    const MONOCALC_REAL h0 = 1e-2;
    const int maxIter = 50;
    const MONOCALC_REAL tol = 1e-10;
    const MONOCALC_REAL divergeThreshold = 1e15;

    MONOCALC_REAL h = h0;
    MONOCALC_REAL sign = fromLeft ? -1.0 : 1.0;
    MONOCALC_REAL prevVal = 0.0;
    bool first = true;

    for (int i = 0; i < maxIter; i++) {
        MONOCALC_REAL x = x0 + sign * h;
        EvalResult r = evaluate(arena, root, x);
        if (!r.ok) {
            // Domain error at this point — try smaller step
            h *= 0.1;
            continue;
        }

        // Check for divergence: value grows without bound
        if (fabs(r.value) > divergeThreshold) {
            LimitResult lr;
            lr.ok = false;
            lr.value = r.value > 0 ? (MONOCALC_REAL)INFINITY : -(MONOCALC_REAL)INFINITY;
            lr.err = ErrorCode::Diverged;
            return lr;
        }

        if (!first) {
            // Check convergence: successive values are close
            MONOCALC_REAL diff = fabs(r.value - prevVal);
            if (diff < tol * fabs(prevVal) || diff < tol) {
                // Converged
                LimitResult lr;
                lr.ok = true;
                lr.value = r.value;
                lr.err = ErrorCode::OK;
                return lr;
            }
        }

        prevVal = r.value;
        first = false;
        h *= 0.1;
    }

    // Did not converge within iterations
    LimitResult lr;
    lr.ok = false;
    lr.value = prevVal;
    lr.err = ErrorCode::NotConverged;
    return lr;
}

LimitResult limit(const Arena& arena, NodeId root, MONOCALC_REAL x0, LimitDirection dir) {
    switch (dir) {
        case LimitDirection::Left:
            return limitOneSide(arena, root, x0, true);
        case LimitDirection::Right:
            return limitOneSide(arena, root, x0, false);
        case LimitDirection::Both: {
            LimitResult left = limitOneSide(arena, root, x0, true);
            LimitResult right = limitOneSide(arena, root, x0, false);

            // Both must converge and agree
            if (!left.ok || !right.ok) {
                // If both diverge to the same infinity, that's a valid result
                if (left.err == ErrorCode::Diverged && right.err == ErrorCode::Diverged) {
                    if ((left.value > 0 && right.value > 0) || (left.value < 0 && right.value < 0)) {
                        return left; // same sign infinity → Diverged
                    }
                    // Opposite sign infinities → no limit exists
                    LimitResult lr;
                    lr.ok = false;
                    lr.value = 0.0;
                    lr.err = ErrorCode::NotConverged;
                    return lr;
                }
                // Different errors → no limit
                LimitResult lr;
                lr.ok = false;
                lr.value = 0.0;
                lr.err = ErrorCode::NotConverged;
                return lr;
            }

            // Both converged — check they agree
            MONOCALC_REAL diff = fabs(left.value - right.value);
            if (diff < 1e-8 * fabs(left.value) || diff < 1e-8) {
                LimitResult lr;
                lr.ok = true;
                lr.value = (left.value + right.value) / 2.0;
                lr.err = ErrorCode::OK;
                return lr;
            }

            // Left and right limits differ → no limit
            LimitResult lr;
            lr.ok = false;
            lr.value = 0.0;
            lr.err = ErrorCode::NotConverged;
            return lr;
        }
    }

    LimitResult lr;
    lr.ok = false;
    lr.value = 0.0;
    lr.err = ErrorCode::NotConverged;
    return lr;
}

} // namespace monocalc
