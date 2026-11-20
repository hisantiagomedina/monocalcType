#include "monocalc/evaluator.h"
#include "monocalc/ast.h"
#include "monocalc/error.h"
#include <cmath>
#include <cfloat>

namespace monocalc {

static EvalResult evalNode(const Arena& arena, NodeId id, MONOCALC_REAL x) {
    const Node& n = arena.at(id);
    switch (n.type) {
        case NodeType::Const:
            return {true, n.constant, ErrorCode::OK};
        case NodeType::Var:
            return {true, x, ErrorCode::OK};
        case NodeType::UnaryOp: {
            EvalResult r = evalNode(arena, n.left, x);
            if (!r.ok) return r;
            if (n.op == OpType::Neg) r.value = -r.value;
            return r;
        }
        case NodeType::BinaryOp: {
            EvalResult l = evalNode(arena, n.left, x);
            if (!l.ok) return l;
            EvalResult rr = evalNode(arena, n.right, x);
            if (!rr.ok) return rr;
            switch (n.op) {
                case OpType::Add: return {true, l.value + rr.value, ErrorCode::OK};
                case OpType::Sub: return {true, l.value - rr.value, ErrorCode::OK};
                case OpType::Mul: return {true, l.value * rr.value, ErrorCode::OK};
                case OpType::Div:
                    if (rr.value == 0.0) return {false, 0.0, ErrorCode::DomainError};
                    return {true, l.value / rr.value, ErrorCode::OK};
                case OpType::Pow:
                    // Handle integer exponents for negative bases
                    if (rr.value == (MONOCALC_REAL)(long long)rr.value) {
                        long long exp = (long long)rr.value;
                        MONOCALC_REAL result = 1.0;
                        bool negative = exp < 0;
                        if (negative) exp = -exp;
                        for (long long e = 0; e < exp; e++) result *= l.value;
                        if (negative) result = 1.0 / result;
                        return {true, result, ErrorCode::OK};
                    }
                    {
                        MONOCALC_REAL result = std::pow(l.value, rr.value);
                        if (std::isnan(result) || std::isinf(result)) {
                            return {false, 0.0, ErrorCode::DomainError};
                        }
                        return {true, result, ErrorCode::OK};
                    }
                default: return {false, 0.0, ErrorCode::UnsupportedOperation};
            }
        }
        case NodeType::FuncCall: {
            EvalResult r = evalNode(arena, n.left, x);
            if (!r.ok) return r;
            switch (n.func) {
                case FuncType::Sin:
                    return {true, std::sin(r.value), ErrorCode::OK};
                case FuncType::Cos:
                    return {true, std::cos(r.value), ErrorCode::OK};
                case FuncType::Exp:
                    {
                        MONOCALC_REAL v = std::exp(r.value);
                        if (std::isinf(v)) return {false, 0.0, ErrorCode::DomainError};
                        return {true, v, ErrorCode::OK};
                    }
                case FuncType::Ln:
                    if (r.value <= 0.0) return {false, 0.0, ErrorCode::DomainError};
                    return {true, std::log(r.value), ErrorCode::OK};
                case FuncType::Abs:
                    return {true, std::abs(r.value), ErrorCode::OK};
                default: return {false, 0.0, ErrorCode::UnsupportedOperation};
            }
        }
        default:
            return {false, 0.0, ErrorCode::UnsupportedOperation};
    }
}

EvalResult evaluate(const Arena& arena, NodeId root, MONOCALC_REAL x) {
    return evalNode(arena, root, x);
}

} // namespace monocalc
