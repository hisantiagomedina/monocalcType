#include "monocalc/domain.h"
#include "monocalc/error.h"
#include <cmath>

namespace monocalc {

ErrorCode domainCheck(const Domain& d, MONOCALC_REAL x) {
    switch (d.type) {
        case DomainType::All:
            return ErrorCode::OK;
        case DomainType::NotZero:
            // excludedPoint holds the point that must be excluded
            if (std::abs(x - d.excludedPoint) < 1e-15) return ErrorCode::DomainError;
            return ErrorCode::OK;
        case DomainType::Positive:
            if (x <= 0.0) return ErrorCode::DomainError;
            return ErrorCode::OK;
        case DomainType::Negative:
            if (x >= 0.0) return ErrorCode::DomainError;
            return ErrorCode::OK;
    }
    return ErrorCode::OK;
}

} // namespace monocalc
