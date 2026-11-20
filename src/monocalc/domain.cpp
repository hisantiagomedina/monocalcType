#include "monocalc/domain.h"
#include "monocalc/error.h"
#include <math.h>

namespace monocalc {

ErrorCode domainCheck(const Domain& d, MONOCALC_REAL x) {
    // Check type-based restrictions
    switch (d.type) {
        case DomainType::All:
            break;
        case DomainType::NotZero:
            // NotZero with no excluded points means x != 0 (classic case)
            if (d.excludedCount == 0 && fabs(x) < 1e-15) return ErrorCode::DomainError;
            break;
        case DomainType::Positive:
            if (x <= 0.0) return ErrorCode::DomainError;
            break;
        case DomainType::Negative:
            if (x >= 0.0) return ErrorCode::DomainError;
            break;
    }
    // Check all excluded points
    for (int i = 0; i < d.excludedCount; i++) {
        if (fabs(x - d.excludedPoint[i]) < 1e-15) return ErrorCode::DomainError;
    }
    return ErrorCode::OK;
}

} // namespace monocalc
