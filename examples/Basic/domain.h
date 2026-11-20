#ifndef MONOCALC_DOMAIN_H
#define MONOCALC_DOMAIN_H

#include "error.h"

#ifndef MONOCALC_REAL
#define MONOCALC_REAL double
#endif

namespace monocalc {

enum class DomainType : unsigned char {
    All,        // no restriction
    NotZero,    // x != 0
    Positive,   // x > 0
    Negative    // x < 0
};

struct Domain {
    DomainType type;
    MONOCALC_REAL excludedPoint; // for NotZero
};

ErrorCode domainCheck(const Domain& d, MONOCALC_REAL x);

} // namespace monocalc

#endif // MONOCALC_DOMAIN_H
