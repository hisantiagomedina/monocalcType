#include "monocalc/limiter.h"
#include "monocalc/evaluator.h"
#include "monocalc/error.h"
#include <math.h>
#include <stdlib.h>

namespace monocalc {

// Numerical limit estimation using adaptive criteria.
// No fixed magnitude threshold for divergence.
// Compares successive values at multiple scales to detect convergence,
// divergence (monotonic growth), oscillation, or plateau from precision loss.
static LimitResult limitOneSide(const Arena& arena, NodeId root, MONOCALC_REAL x0, bool fromLeft) {
    const int maxIter = 60;
    const MONOCALC_REAL tol = 1e-8;

    MONOCALC_REAL h = 1e-1;
    MONOCALC_REAL sign = fromLeft ? -1.0 : 1.0;

    // Collect valid samples
    struct Sample { MONOCALC_REAL x; MONOCALC_REAL val; };
    Sample samples[64];
    int sampleCount = 0;

    for (int i = 0; i < maxIter && sampleCount < 64; i++) {
        MONOCALC_REAL x = x0 + sign * h;
        // Detect if x == x0 (precision loss)
        if (x == x0) {
            h *= 0.1;
            continue;
        }
        EvalResult r = evaluate(arena, root, x);
        if (!r.ok) {
            h *= 0.1;
            continue;
        }
        if (isnan(r.value) || isinf(r.value)) {
            // Non-finite value — may indicate divergence
            h *= 0.1;
            continue;
        }
        samples[sampleCount].x = x;
        samples[sampleCount].val = r.value;
        sampleCount++;
        h *= 0.1;
    }

    if (sampleCount < 3) {
        // Not enough valid samples
        LimitResult lr;
        lr.ok = false;
        lr.value = 0.0;
        lr.err = ErrorCode::NotConverged;
        return lr;
    }

    // Check for divergence: values grow monotonically in magnitude
    bool growing = true;
    for (int i = 1; i < sampleCount; i++) {
        if (fabs(samples[i].val) <= fabs(samples[i-1].val)) {
            growing = false;
            break;
        }
    }
    if (growing && sampleCount >= 5) {
        // Check if last values are very large compared to first
        MONOCALC_REAL ratio = fabs(samples[sampleCount-1].val) / (fabs(samples[0].val) < 1e-300 ? 1e-300 : fabs(samples[0].val));
        if (ratio > 1e6) {
            LimitResult lr;
            lr.ok = false;
            lr.value = samples[sampleCount-1].val > 0 ? (MONOCALC_REAL)INFINITY : -(MONOCALC_REAL)INFINITY;
            lr.err = ErrorCode::Diverged;
            return lr;
        }
    }

    // Check for convergence: last N samples agree within tolerance
    // Use last 5 samples (smallest h)
    int checkN = sampleCount >= 5 ? 5 : sampleCount;
    int startIdx = sampleCount - checkN;
    MONOCALC_REAL refVal = samples[startIdx].val;

    // Relative tolerance based on magnitude
    MONOCALC_REAL relTol = tol;
    if (fabs(refVal) > 1e6) {
        relTol = tol * fabs(refVal); // absolute tolerance for large values
    }

    bool converged = true;
    for (int i = startIdx + 1; i < sampleCount; i++) {
        MONOCALC_REAL diff = fabs(samples[i].val - refVal);
        MONOCALC_REAL threshold = relTol;
        if (fabs(refVal) > 1.0) {
            threshold = tol * fabs(refVal);
        }
        if (diff > threshold) {
            converged = false;
            break;
        }
    }

    if (converged) {
        LimitResult lr;
        lr.ok = true;
        lr.value = samples[sampleCount-1].val;
        lr.err = ErrorCode::OK;
        return lr;
    }

    // Check for oscillation: sign changes in last samples
    int signChanges = 0;
    for (int i = startIdx + 1; i < sampleCount; i++) {
        if ((samples[i].val > 0) != (samples[i-1].val > 0)) {
            signChanges++;
        }
    }
    if (signChanges >= 2) {
        LimitResult lr;
        lr.ok = false;
        lr.value = 0.0;
        lr.err = ErrorCode::NotConverged;
        return lr;
    }

    // Not converged
    LimitResult lr;
    lr.ok = false;
    lr.value = samples[sampleCount-1].val;
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
