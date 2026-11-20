#ifndef MONOCALC_ERROR_H
#define MONOCALC_ERROR_H

#if defined(__AVR__)
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif
#else
#include <cstddef>
#endif

namespace monocalc {

enum class ErrorCode {
    OK,
    SyntaxError,
    UnsupportedOperation,
    DomainError,
    ResourceExhausted,
    NotConverged,
    Diverged
};

inline const char* errorMessage(ErrorCode err) {
    switch (err) {
        case ErrorCode::OK: return "OK";
        case ErrorCode::SyntaxError: return "Syntax error";
        case ErrorCode::UnsupportedOperation: return "Unsupported operation";
        case ErrorCode::DomainError: return "Domain error";
        case ErrorCode::ResourceExhausted: return "Resource exhausted";
        case ErrorCode::NotConverged: return "Not converged";
        case ErrorCode::Diverged: return "Diverged";
        default: return "Unknown error";
    }
}

} // namespace monocalc

#endif // MONOCALC_ERROR_H
