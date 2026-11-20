#include "monocalc/differentiator.h"
#include "monocalc/ast.h"
#include "monocalc/error.h"
#include <math.h>

namespace monocalc {

// Helper: check if a node represents an affine expression a*x + b
static bool isAffineDiff(const Arena& in, NodeId id, MONOCALC_REAL* aCoeff, MONOCALC_REAL* bConst) {
    if (id == INVALID_ID) return false;
    const Node& nd = in.at(id);
    switch (nd.type) {
        case NodeType::Var:
            *aCoeff = 1.0; *bConst = 0.0; return true;
        case NodeType::Const:
            *aCoeff = 0.0; *bConst = nd.constant; return true;
        case NodeType::BinaryOp:
            if (nd.op == OpType::Add || nd.op == OpType::Sub) {
                MONOCALC_REAL la, lb, ra, rb;
                if (isAffineDiff(in, nd.left, &la, &lb) && isAffineDiff(in, nd.right, &ra, &rb)) {
                    if (nd.op == OpType::Add) { *aCoeff = la + ra; *bConst = lb + rb; }
                    else { *aCoeff = la - ra; *bConst = lb - rb; }
                    return true;
                }
            }
            if (nd.op == OpType::Mul) {
                if (nd.left != INVALID_ID && nd.right != INVALID_ID) {
                    const Node& lnd = in.at(nd.left);
                    const Node& rnd = in.at(nd.right);
                    if (lnd.type == NodeType::Const && rnd.type == NodeType::Var) {
                        *aCoeff = lnd.constant; *bConst = 0.0; return true;
                    }
                    if (rnd.type == NodeType::Const && lnd.type == NodeType::Var) {
                        *aCoeff = rnd.constant; *bConst = 0.0; return true;
                    }
                }
            }
            break;
        default:
            break;
    }
    return false;
}

static ErrorCode diff(const Arena& in, NodeId id, Arena& out, NodeId* outRoot);
static ErrorCode copyNode(const Arena& in, NodeId id, Arena& out, NodeId* outId);

// Helper: create a constant node in output arena
static NodeId mkConst(Arena& out, MONOCALC_REAL v) {
    NodeId nid = out.alloc();
    if (nid == INVALID_ID) return INVALID_ID;
    Node& n = out.at(nid);
    n.type = NodeType::Const;
    n.constant = v;
    n.left = INVALID_ID;
    n.right = INVALID_ID;
    return nid;
}

// Helper: create a var node
static NodeId mkVar(Arena& out) {
    NodeId nid = out.alloc();
    if (nid == INVALID_ID) return INVALID_ID;
    Node& n = out.at(nid);
    n.type = NodeType::Var;
    n.left = INVALID_ID;
    n.right = INVALID_ID;
    return nid;
}

// Helper: create a binary op node
static NodeId mkBinOp(Arena& out, OpType op, NodeId l, NodeId r) {
    NodeId nid = out.alloc();
    if (nid == INVALID_ID) return INVALID_ID;
    Node& n = out.at(nid);
    n.type = NodeType::BinaryOp;
    n.op = op;
    n.left = l;
    n.right = r;
    return nid;
}

// Helper: create a unary op node
static NodeId mkUnaryOp(Arena& out, OpType op, NodeId child) {
    NodeId nid = out.alloc();
    if (nid == INVALID_ID) return INVALID_ID;
    Node& n = out.at(nid);
    n.type = NodeType::UnaryOp;
    n.op = op;
    n.left = child;
    n.right = INVALID_ID;
    return nid;
}

// Helper: create a func call node
static NodeId mkFunc(Arena& out, FuncType ft, NodeId arg) {
    NodeId nid = out.alloc();
    if (nid == INVALID_ID) return INVALID_ID;
    Node& n = out.at(nid);
    n.type = NodeType::FuncCall;
    n.func = ft;
    n.left = arg;
    n.right = INVALID_ID;
    return nid;
}

// Helper: check if a node is a constant integer
static bool isIntConst(const Arena& a, NodeId id, long long* val) {
    if (id == INVALID_ID) return false;
    const Node& n = a.at(id);
    if (n.type != NodeType::Const) return false;
    MONOCALC_REAL v = n.constant;
    if (v != (MONOCALC_REAL)(long long)v) return false;
    *val = (long long)v;
    return true;
}

static ErrorCode diff(const Arena& in, NodeId id, Arena& out, NodeId* outRoot) {
    const Node& n = in.at(id);

    switch (n.type) {
        case NodeType::Const: {
            // d/dx(c) = 0
            NodeId z = mkConst(out, 0.0);
            if (z == INVALID_ID) return ErrorCode::ResourceExhausted;
            *outRoot = z;
            return ErrorCode::OK;
        }

        case NodeType::Var: {
            // d/dx(x) = 1
            NodeId one = mkConst(out, 1.0);
            if (one == INVALID_ID) return ErrorCode::ResourceExhausted;
            *outRoot = one;
            return ErrorCode::OK;
        }

        case NodeType::UnaryOp: {
            if (n.op == OpType::Neg) {
                // d/dx(-f) = -(d/dx(f))
                NodeId df;
                ErrorCode err = diff(in, n.left, out, &df);
                if (err != ErrorCode::OK) return err;
                NodeId neg = mkUnaryOp(out, OpType::Neg, df);
                if (neg == INVALID_ID) return ErrorCode::ResourceExhausted;
                *outRoot = neg;
                return ErrorCode::OK;
            }
            if (n.op == OpType::Pos) {
                // d/dx(+f) = d/dx(f)
                return diff(in, n.left, out, outRoot);
            }
            return ErrorCode::UnsupportedOperation;
        }

        case NodeType::BinaryOp: {
            switch (n.op) {
                case OpType::Add: {
                    // d/dx(f+g) = d/dx(f) + d/dx(g)
                    NodeId df, dg;
                    ErrorCode err = diff(in, n.left, out, &df);
                    if (err != ErrorCode::OK) return err;
                    err = diff(in, n.right, out, &dg);
                    if (err != ErrorCode::OK) return err;
                    NodeId add = mkBinOp(out, OpType::Add, df, dg);
                    if (add == INVALID_ID) return ErrorCode::ResourceExhausted;
                    *outRoot = add;
                    return ErrorCode::OK;
                }

                case OpType::Sub: {
                    // d/dx(f-g) = d/dx(f) - d/dx(g)
                    NodeId df, dg;
                    ErrorCode err = diff(in, n.left, out, &df);
                    if (err != ErrorCode::OK) return err;
                    err = diff(in, n.right, out, &dg);
                    if (err != ErrorCode::OK) return err;
                    NodeId sub = mkBinOp(out, OpType::Sub, df, dg);
                    if (sub == INVALID_ID) return ErrorCode::ResourceExhausted;
                    *outRoot = sub;
                    return ErrorCode::OK;
                }

                case OpType::Mul: {
                    // d/dx(f*g) = f'*g + f*g'
                    NodeId df, dg, fCopy, gCopy;
                    ErrorCode err = diff(in, n.left, out, &df);
                    if (err != ErrorCode::OK) return err;
                    err = diff(in, n.right, out, &dg);
                    if (err != ErrorCode::OK) return err;
                    // Copy f and g to output arena
                    err = copyNode(in, n.left, out, &fCopy);
                    if (err != ErrorCode::OK) return err;
                    err = copyNode(in, n.right, out, &gCopy);
                    if (err != ErrorCode::OK) return err;
                    // f'*g
                    NodeId t1 = mkBinOp(out, OpType::Mul, df, gCopy);
                    if (t1 == INVALID_ID) return ErrorCode::ResourceExhausted;
                    // f*g'
                    NodeId t2 = mkBinOp(out, OpType::Mul, fCopy, dg);
                    if (t2 == INVALID_ID) return ErrorCode::ResourceExhausted;
                    // f'*g + f*g'
                    NodeId add = mkBinOp(out, OpType::Add, t1, t2);
                    if (add == INVALID_ID) return ErrorCode::ResourceExhausted;
                    *outRoot = add;
                    return ErrorCode::OK;
                }

                case OpType::Div: {
                    // d/dx(f/g) = (f'*g - f*g') / g^2
                    NodeId df, dg, fCopy, gCopy;
                    ErrorCode err = diff(in, n.left, out, &df);
                    if (err != ErrorCode::OK) return err;
                    err = diff(in, n.right, out, &dg);
                    if (err != ErrorCode::OK) return err;
                    err = copyNode(in, n.left, out, &fCopy);
                    if (err != ErrorCode::OK) return err;
                    err = copyNode(in, n.right, out, &gCopy);
                    if (err != ErrorCode::OK) return err;
                    // f'*g
                    NodeId t1 = mkBinOp(out, OpType::Mul, df, gCopy);
                    if (t1 == INVALID_ID) return ErrorCode::ResourceExhausted;
                    // f*g'
                    NodeId t2 = mkBinOp(out, OpType::Mul, fCopy, dg);
                    if (t2 == INVALID_ID) return ErrorCode::ResourceExhausted;
                    // f'*g - f*g'
                    NodeId num = mkBinOp(out, OpType::Sub, t1, t2);
                    if (num == INVALID_ID) return ErrorCode::ResourceExhausted;
                    // g^2
                    NodeId two = mkConst(out, 2.0);
                    if (two == INVALID_ID) return ErrorCode::ResourceExhausted;
                    NodeId g2 = mkBinOp(out, OpType::Pow, gCopy, two);
                    if (g2 == INVALID_ID) return ErrorCode::ResourceExhausted;
                    // (f'*g - f*g') / g^2
                    NodeId div = mkBinOp(out, OpType::Div, num, g2);
                    if (div == INVALID_ID) return ErrorCode::ResourceExhausted;
                    *outRoot = div;
                    return ErrorCode::OK;
                }

                case OpType::Pow: {
                    // Check for x^n (variable to constant integer power)
                    {
                        const Node& leftNode = in.at(n.left);
                        long long expVal;
                        if (leftNode.type == NodeType::Var && isIntConst(in, n.right, &expVal)) {
                            // d/dx(x^n) = n*x^(n-1)
                            if (expVal == 0) {
                                NodeId z = mkConst(out, 0.0);
                                if (z == INVALID_ID) return ErrorCode::ResourceExhausted;
                                *outRoot = z;
                                return ErrorCode::OK;
                            }
                            NodeId nConst = mkConst(out, (MONOCALC_REAL)expVal);
                            if (nConst == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId nMinus1 = mkConst(out, (MONOCALC_REAL)(expVal - 1));
                            if (nMinus1 == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId var = mkVar(out);
                            if (var == INVALID_ID) return ErrorCode::ResourceExhausted;
                            if (expVal == 1) {
                                // n*x^0 = n
                                *outRoot = nConst;
                                return ErrorCode::OK;
                            }
                            NodeId xPow = mkBinOp(out, OpType::Pow, var, nMinus1);
                            if (xPow == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId result = mkBinOp(out, OpType::Mul, nConst, xPow);
                            if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                            *outRoot = result;
                            return ErrorCode::OK;
                        }
                    }
                    // General power rule: d/dx(f^g) = f^g * (g'*ln(f) + g*f'/f)
                    // Simplified: f^g * (g'*ln(f)/f + g*f'/f) ... use chain rule
                    // d/dx(f^g) = f^g * (g' * ln(f) + g * f' / f)
                    {
                        NodeId fCopy, gCopy, df, dg;
                        ErrorCode err = copyNode(in, n.left, out, &fCopy);
                        if (err != ErrorCode::OK) return err;
                        err = copyNode(in, n.right, out, &gCopy);
                        if (err != ErrorCode::OK) return err;
                        err = diff(in, n.left, out, &df);
                        if (err != ErrorCode::OK) return err;
                        err = diff(in, n.right, out, &dg);
                        if (err != ErrorCode::OK) return err;
                        // ln(f)
                        NodeId lnF = mkFunc(out, FuncType::Ln, fCopy);
                        if (lnF == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // g' * ln(f)
                        NodeId t1 = mkBinOp(out, OpType::Mul, dg, lnF);
                        if (t1 == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // g * f'
                        NodeId t2 = mkBinOp(out, OpType::Mul, gCopy, df);
                        if (t2 == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // f' * g / f  = (g*f')/f
                        NodeId t3 = mkBinOp(out, OpType::Div, t2, fCopy);
                        if (t3 == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // g'*ln(f) + g*f'/f
                        NodeId inner = mkBinOp(out, OpType::Add, t1, t3);
                        if (inner == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // f^g
                        NodeId fPowG = mkBinOp(out, OpType::Pow, fCopy, gCopy);
                        if (fPowG == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // f^g * (...)
                        NodeId result = mkBinOp(out, OpType::Mul, fPowG, inner);
                        if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                        *outRoot = result;
                        return ErrorCode::OK;
                    }
                }

                default:
                    return ErrorCode::UnsupportedOperation;
            }
        }

        case NodeType::FuncCall: {
            // Special case: d/dx(ln(abs(x))) = 1/x
            // Extended: d/dx(ln(abs(ax+b))) = a/(ax+b)
            // Must check BEFORE the generic chain rule, since abs is not differentiable
            if (n.func == FuncType::Ln) {
                const Node& argNode = in.at(n.left);
                if (argNode.type == NodeType::FuncCall && argNode.func == FuncType::Abs) {
                    const Node& innerNode = in.at(argNode.left);
                    if (innerNode.type == NodeType::Var) {
                        // d/dx(ln(abs(x))) = 1/x
                        NodeId one = mkConst(out, 1.0);
                        if (one == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId v = mkVar(out);
                        if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId invX = mkBinOp(out, OpType::Div, one, v);
                        if (invX == INVALID_ID) return ErrorCode::ResourceExhausted;
                        *outRoot = invX;
                        return ErrorCode::OK;
                    }
                    // Check for affine: ax+b inside abs
                    MONOCALC_REAL aCoeff, bConst;
                    if (isAffineDiff(in, argNode.left, &aCoeff, &bConst)) {
                        if (fabs(aCoeff) > 1e-15) {
                            // d/dx(ln(abs(ax+b))) = a/(ax+b)
                            NodeId aNode = mkConst(out, aCoeff);
                            if (aNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId v = mkVar(out);
                            if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId ax = mkBinOp(out, OpType::Mul, aNode, v);
                            if (ax == INVALID_ID) return ErrorCode::ResourceExhausted;
                            if (fabs(bConst) > 1e-15) {
                                NodeId bNode = mkConst(out, bConst);
                                if (bNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId axb = mkBinOp(out, OpType::Add, ax, bNode);
                                if (axb == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId result = mkBinOp(out, OpType::Div, aNode, axb);
                                if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                                *outRoot = result;
                                return ErrorCode::OK;
                            } else {
                                // a / (ax) = 1/x
                                NodeId one = mkConst(out, 1.0);
                                if (one == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId result = mkBinOp(out, OpType::Div, one, v);
                                if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                                *outRoot = result;
                                return ErrorCode::OK;
                            }
                        }
                    }
                }
            }

            // Chain rule: d/dx(f(g(x))) = f'(g(x)) * g'(x)
            NodeId argCopy, dArg;
            ErrorCode err = copyNode(in, n.left, out, &argCopy);
            if (err != ErrorCode::OK) return err;
            err = diff(in, n.left, out, &dArg);
            if (err != ErrorCode::OK) return err;

            NodeId fPrime;
            switch (n.func) {
                case FuncType::Sin: {
                    // d/dx(sin(u)) = cos(u) * u'
                    NodeId cosU = mkFunc(out, FuncType::Cos, argCopy);
                    if (cosU == INVALID_ID) return ErrorCode::ResourceExhausted;
                    fPrime = cosU;
                    break;
                }
                case FuncType::Cos: {
                    // d/dx(cos(u)) = -sin(u) * u'
                    NodeId sinU = mkFunc(out, FuncType::Sin, argCopy);
                    if (sinU == INVALID_ID) return ErrorCode::ResourceExhausted;
                    NodeId negSinU = mkUnaryOp(out, OpType::Neg, sinU);
                    if (negSinU == INVALID_ID) return ErrorCode::ResourceExhausted;
                    fPrime = negSinU;
                    break;
                }
                case FuncType::Exp: {
                    // d/dx(exp(u)) = exp(u) * u'
                    NodeId expU = mkFunc(out, FuncType::Exp, argCopy);
                    if (expU == INVALID_ID) return ErrorCode::ResourceExhausted;
                    fPrime = expU;
                    break;
                }
                case FuncType::Ln: {
                    // d/dx(ln(u)) = 1/u * u'
                    NodeId one = mkConst(out, 1.0);
                    if (one == INVALID_ID) return ErrorCode::ResourceExhausted;
                    NodeId invU = mkBinOp(out, OpType::Div, one, argCopy);
                    if (invU == INVALID_ID) return ErrorCode::ResourceExhausted;
                    fPrime = invU;
                    break;
                }
                case FuncType::Abs: {
                    // d/dx(abs(u)) = u * u' / abs(u)
                    // fPrime = u / abs(u)
                    NodeId absU = mkFunc(out, FuncType::Abs, argCopy);
                    if (absU == INVALID_ID) return ErrorCode::ResourceExhausted;
                    NodeId uOverAbsU = mkBinOp(out, OpType::Div, argCopy, absU);
                    if (uOverAbsU == INVALID_ID) return ErrorCode::ResourceExhausted;
                    fPrime = uOverAbsU;
                    break;
                }
                default:
                    return ErrorCode::UnsupportedOperation;
            }

            // f'(u) * u'
            NodeId result = mkBinOp(out, OpType::Mul, fPrime, dArg);
            if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
            *outRoot = result;
            return ErrorCode::OK;
        }

        default:
            return ErrorCode::UnsupportedOperation;
    }
}

// Copy a node from input arena to output arena (deep copy)
static ErrorCode copyNode(const Arena& in, NodeId id, Arena& out, NodeId* outId) {
    const Node& n = in.at(id);
    NodeId nid = out.alloc();
    if (nid == INVALID_ID) return ErrorCode::ResourceExhausted;
    Node& nOut = out.at(nid);

    nOut.type = n.type;
    switch (n.type) {
        case NodeType::Const:
            nOut.constant = n.constant;
            break;
        case NodeType::UnaryOp:
        case NodeType::BinaryOp:
            nOut.op = n.op;
            break;
        case NodeType::FuncCall:
            nOut.func = n.func;
            break;
        case NodeType::Var:
            break;
    }

    if (n.type == NodeType::UnaryOp || n.type == NodeType::FuncCall) {
        NodeId copied;
        ErrorCode err = copyNode(in, n.left, out, &copied);
        if (err != ErrorCode::OK) return err;
        nOut.left = copied;
        nOut.right = INVALID_ID;
    } else if (n.type == NodeType::BinaryOp) {
        NodeId lCopy, rCopy;
        ErrorCode err = copyNode(in, n.left, out, &lCopy);
        if (err != ErrorCode::OK) return err;
        err = copyNode(in, n.right, out, &rCopy);
        if (err != ErrorCode::OK) return err;
        nOut.left = lCopy;
        nOut.right = rCopy;
    } else {
        nOut.left = INVALID_ID;
        nOut.right = INVALID_ID;
    }

    *outId = nid;
    return ErrorCode::OK;
}

ErrorCode differentiate(const Arena& in, NodeId root, Arena& out, NodeId* outRoot) {
    return diff(in, root, out, outRoot);
}

} // namespace monocalc
