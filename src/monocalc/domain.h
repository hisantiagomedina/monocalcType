#ifndef MONOCALC_DOMAIN_H
#define MONOCALC_DOMAIN_H

#include "monocalc/error.h"

#ifndef MONOCALC_REAL
#define MONOCALC_REAL double
#endif

#if defined(__AVR__)
#define DOMAIN_MAX_EXCLUDED 4
#else
#define DOMAIN_MAX_EXCLUDED 8
#endif

namespace monocalc {

enum class DomainType : unsigned char {
    All,        // no restriction
    NotZero,    // x != 0 (or x != excludedPoint[i])
    Positive,   // x > 0
    Negative    // x < 0
};

struct Domain {
    DomainType type;
    MONOCALC_REAL excludedPoint[DOMAIN_MAX_EXCLUDED];
    unsigned char excludedCount;

    Domain() : type(DomainType::All), excludedCount(0) {
        for (int i = 0; i < DOMAIN_MAX_EXCLUDED; i++) excludedPoint[i] = 0;
    }

    void addExcluded(MONOCALC_REAL pt) {
        // Check for duplicates
        for (int i = 0; i < excludedCount; i++) {
            if (excludedPoint[i] == pt) return;
        }
        if (excludedCount < DOMAIN_MAX_EXCLUDED) {
            excludedPoint[excludedCount++] = pt;
        }
    }

    bool hasRestrictions() const {
        return (type != DomainType::All && excludedCount == 0) || excludedCount > 0;
    }
};

ErrorCode domainCheck(const Domain& d, MONOCALC_REAL x);

} // namespace monocalc

#endif // MONOCALC_DOMAIN_H
