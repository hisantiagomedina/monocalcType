#include "monocalc/integrator.h"
#include "monocalc/ast.h"
#include "monocalc/error.h"
#include "monocalc/domain.h"
#include <math.h>

namespace monocalc {

/* Forward declarations */
static ErrorCode integrateNode(const Arena& in, NodeId id, Arena& out, NodeId* outRoot, Domain* outDomain);
static ErrorCode copyNode(const Arena& in, NodeId id, Arena& out, NodeId* outId);

/* Helper: create a constant node */
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

/* Helper: create a var node */
static NodeId mkVar(Arena& out) {
    NodeId nid = out.alloc();
    if (nid == INVALID_ID) return INVALID_ID;
    Node& n = out.at(nid);
    n.type = NodeType::Var;
    n.left = INVALID_ID;
    n.right = INVALID_ID;
    return nid;
}

/* Helper: create a binary op node */
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

/* Helper: create a unary op node */
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

/* Helper: create a func call node */
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

/* Check if a node is a constant integer, return value in *val */
static bool isIntConst(const Arena& a, NodeId id, long long* val) {
    if (id == INVALID_ID) return false;
    const Node& n = a.at(id);
    if (n.type != NodeType::Const) return false;
    MONOCALC_REAL v = n.constant;
    if (v != (MONOCALC_REAL)(long long)v) return false;
    *val = (long long)v;
    return true;
}

/* Check if a node is a constant (any value), return value in *val */
static bool isConst(const Arena& a, NodeId id, MONOCALC_REAL* val) {
    if (id == INVALID_ID) return false;
    const Node& n = a.at(id);
    if (n.type != NodeType::Const) return false;
    *val = n.constant;
    return true;
}

/* Check if a node is the variable x */
static bool isVar(const Arena& a, NodeId id) {
    if (id == INVALID_ID) return false;
    const Node& n = a.at(id);
    return n.type == NodeType::Var;
}

/* Check if a node is an affine expression a*x + b.
   Returns true and fills *aCoeff and *bConst.
   Handles: x, c*x, c*x+b, b+c*x, c, and -(a*x+b) patterns. */
static bool isAffine(const Arena& in, NodeId id, MONOCALC_REAL* aCoeff, MONOCALC_REAL* bConst) {
    if (id == INVALID_ID) return false;
    const Node& n = in.at(id);

    switch (n.type) {
        case NodeType::Var:
            *aCoeff = 1.0;
            *bConst = 0.0;
            return true;

        case NodeType::Const:
            *aCoeff = 0.0;
            *bConst = n.constant;
            return true;

        case NodeType::UnaryOp:
            if (n.op == OpType::Neg) {
                MONOCALC_REAL a, b;
                if (isAffine(in, n.left, &a, &b)) {
                    *aCoeff = -a;
                    *bConst = -b;
                    return true;
                }
            } else if (n.op == OpType::Pos) {
                return isAffine(in, n.left, aCoeff, bConst);
            }
            return false;

        case NodeType::BinaryOp: {
            if (n.op == OpType::Add || n.op == OpType::Sub) {
                MONOCALC_REAL la, lb, ra, rb;
                if (isAffine(in, n.left, &la, &lb) && isAffine(in, n.right, &ra, &rb)) {
                    if (n.op == OpType::Add) {
                        *aCoeff = la + ra;
                        *bConst = lb + rb;
                    } else {
                        *aCoeff = la - ra;
                        *bConst = lb - rb;
                    }
                    return true;
                }
            }
            if (n.op == OpType::Mul) {
                // c * x or x * c
                MONOCALC_REAL c;
                if (isConst(in, n.left, &c) && isVar(in, n.right)) {
                    *aCoeff = c;
                    *bConst = 0.0;
                    return true;
                }
                if (isVar(in, n.left) && isConst(in, n.right, &c)) {
                    *aCoeff = c;
                    *bConst = 0.0;
                    return true;
                }
                // c * (affine) — only if right is const * affine
                MONOCALC_REAL la, lb;
                if (isConst(in, n.left, &c) && isAffine(in, n.right, &la, &lb)) {
                    *aCoeff = c * la;
                    *bConst = c * lb;
                    return true;
                }
            }
            return false;
        }

        default:
            return false;
    }
}

/* Deep copy a node from input arena to output arena */
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

/* Main integration function */
static ErrorCode integrateNode(const Arena& in, NodeId id, Arena& out, NodeId* outRoot, Domain* outDomain) {
    const Node& n = in.at(id);

    switch (n.type) {
        case NodeType::Const: {
            // ∫c dx = c*x
            NodeId c = mkConst(out, n.constant);
            if (c == INVALID_ID) return ErrorCode::ResourceExhausted;
            NodeId v = mkVar(out);
            if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
            NodeId mul = mkBinOp(out, OpType::Mul, c, v);
            if (mul == INVALID_ID) return ErrorCode::ResourceExhausted;
            *outRoot = mul;
            if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
            return ErrorCode::OK;
        }

        case NodeType::Var: {
            // ∫x dx = x^2 / 2
            NodeId v = mkVar(out);
            if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
            NodeId two = mkConst(out, 2.0);
            if (two == INVALID_ID) return ErrorCode::ResourceExhausted;
            NodeId x2 = mkBinOp(out, OpType::Pow, v, two);
            if (x2 == INVALID_ID) return ErrorCode::ResourceExhausted;
            NodeId half = mkConst(out, 0.5);
            if (half == INVALID_ID) return ErrorCode::ResourceExhausted;
            NodeId result = mkBinOp(out, OpType::Mul, half, x2);
            if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
            *outRoot = result;
            if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
            return ErrorCode::OK;
        }

        case NodeType::UnaryOp: {
            if (n.op == OpType::Neg) {
                // ∫(-f) dx = -∫f dx
                NodeId intF;
                Domain dF;
                ErrorCode err = integrateNode(in, n.left, out, &intF, &dF);
                if (err != ErrorCode::OK) return err;
                NodeId neg = mkUnaryOp(out, OpType::Neg, intF);
                if (neg == INVALID_ID) return ErrorCode::ResourceExhausted;
                *outRoot = neg;
                if (outDomain) *outDomain = dF;
                return ErrorCode::OK;
            }
            if (n.op == OpType::Pos) {
                return integrateNode(in, n.left, out, outRoot, outDomain);
            }
            return ErrorCode::UnsupportedOperation;
        }

        case NodeType::BinaryOp: {
            switch (n.op) {
                case OpType::Add: {
                    // ∫(f+g) dx = ∫f dx + ∫g dx
                    NodeId intF, intG;
                    Domain dF, dG;
                    ErrorCode err = integrateNode(in, n.left, out, &intF, &dF);
                    if (err != ErrorCode::OK) return err;
                    err = integrateNode(in, n.right, out, &intG, &dG);
                    if (err != ErrorCode::OK) return err;
                    NodeId add = mkBinOp(out, OpType::Add, intF, intG);
                    if (add == INVALID_ID) return ErrorCode::ResourceExhausted;
                    *outRoot = add;
                    // Merge domains: take the more restrictive
                    if (outDomain) {
                        if (dG.type != DomainType::All) *outDomain = dG;
                        else *outDomain = dF;
                    }
                    return ErrorCode::OK;
                }

                case OpType::Sub: {
                    // ∫(f-g) dx = ∫f dx - ∫g dx
                    NodeId intF, intG;
                    Domain dF, dG;
                    ErrorCode err = integrateNode(in, n.left, out, &intF, &dF);
                    if (err != ErrorCode::OK) return err;
                    err = integrateNode(in, n.right, out, &intG, &dG);
                    if (err != ErrorCode::OK) return err;
                    NodeId sub = mkBinOp(out, OpType::Sub, intF, intG);
                    if (sub == INVALID_ID) return ErrorCode::ResourceExhausted;
                    *outRoot = sub;
                    if (outDomain) {
                        if (dG.type != DomainType::All) *outDomain = dG;
                        else *outDomain = dF;
                    }
                    return ErrorCode::OK;
                }

                case OpType::Mul: {
                    // Check for c * x^n pattern (constant * power)
                    {
                        MONOCALC_REAL c;
                        if (isConst(in, n.left, &c)) {
                            // c * f → c * ∫f
                            NodeId intF;
                            Domain dF;
                            ErrorCode err = integrateNode(in, n.right, out, &intF, &dF);
                            if (err != ErrorCode::OK) return err;
                            NodeId cNode = mkConst(out, c);
                            if (cNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId result = mkBinOp(out, OpType::Mul, cNode, intF);
                            if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                            *outRoot = result;
                            if (outDomain) *outDomain = dF;
                            return ErrorCode::OK;
                        }
                    }
                    // Check for c * sin(a*x+b) or c * cos(a*x+b) or c * exp(a*x+b)
                    {
                        MONOCALC_REAL c;
                        if (isConst(in, n.left, &c)) {
                            const Node& rightNode = in.at(n.right);
                            if (rightNode.type == NodeType::FuncCall) {
                                NodeId intF;
                                Domain dF;
                                ErrorCode err = integrateNode(in, n.right, out, &intF, &dF);
                                if (err != ErrorCode::OK) return err;
                                NodeId cNode = mkConst(out, c);
                                if (cNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId result = mkBinOp(out, OpType::Mul, cNode, intF);
                                if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                                *outRoot = result;
                                if (outDomain) *outDomain = dF;
                                return ErrorCode::OK;
                            }
                        }
                    }
                    // Check for 1 / (a*x + b) pattern: (1/(ax+b)) represented as Div
                    // This case is handled in Div below
                    return ErrorCode::UnsupportedOperation;
                }

                case OpType::Div: {
                    // Check for 1 / (a*x + b) → ln|a*x+b| / a
                    {
                        MONOCALC_REAL numVal;
                        if (isConst(in, n.left, &numVal) && fabs(numVal - 1.0) < 1e-15) {
                            MONOCALC_REAL aCoeff, bConst;
                            if (isAffine(in, n.right, &aCoeff, &bConst)) {
                                if (fabs(aCoeff) < 1e-15) {
                                    // 1 / constant → constant
                                    MONOCALC_REAL c = 1.0 / bConst;
                                    NodeId cNode = mkConst(out, c);
                                    if (cNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                                    NodeId v = mkVar(out);
                                    if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                                    NodeId result = mkBinOp(out, OpType::Mul, cNode, v);
                                    if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                                    *outRoot = result;
                                    if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
                                    return ErrorCode::OK;
                                }
                                // ∫1/(a*x+b) dx = ln|a*x+b| / a
                                // Build: a*x + b
                                NodeId aNode = mkConst(out, aCoeff);
                                if (aNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId v = mkVar(out);
                                if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId ax = mkBinOp(out, OpType::Mul, aNode, v);
                                if (ax == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId bNode = mkConst(out, bConst);
                                if (bNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId axb = mkBinOp(out, OpType::Add, ax, bNode);
                                if (axb == INVALID_ID) return ErrorCode::ResourceExhausted;
                                // abs(a*x+b)
                                NodeId absAxb = mkFunc(out, FuncType::Abs, axb);
                                if (absAxb == INVALID_ID) return ErrorCode::ResourceExhausted;
                                // ln(abs(a*x+b))
                                NodeId lnAbs = mkFunc(out, FuncType::Ln, absAxb);
                                if (lnAbs == INVALID_ID) return ErrorCode::ResourceExhausted;
                                // / a
                                NodeId invA = mkConst(out, 1.0 / aCoeff);
                                if (invA == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId result = mkBinOp(out, OpType::Mul, invA, lnAbs);
                                if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                                *outRoot = result;
                                if (outDomain) {
                                    outDomain->type = DomainType::NotZero;
                                    outDomain->excludedPoint = -bConst / aCoeff;
                                }
                                return ErrorCode::OK;
                            }
                        }
                    }
                    // General: f / g — check if f is 1 and g is x (1/x → ln|a|x|)
                    {
                        MONOCALC_REAL numVal;
                        if (isConst(in, n.left, &numVal) && fabs(numVal - 1.0) < 1e-15 && isVar(in, n.right)) {
                            // ∫1/x dx = ln|a|x|
                            NodeId v = mkVar(out);
                            if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId absV = mkFunc(out, FuncType::Abs, v);
                            if (absV == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId lnAbs = mkFunc(out, FuncType::Ln, absV);
                            if (lnAbs == INVALID_ID) return ErrorCode::ResourceExhausted;
                            *outRoot = lnAbs;
                            if (outDomain) { outDomain->type = DomainType::NotZero; outDomain->excludedPoint = 0.0; }
                            return ErrorCode::OK;
                        }
                    }
                    return ErrorCode::UnsupportedOperation;
                }

                case OpType::Pow: {
                    // x^n where n is integer constant
                    // Handle x^(-k) where the exponent is -Const (UnaryOp Neg)
                    {
                        if (isVar(in, n.left)) {
                            long long expVal;
                            bool found = isIntConst(in, n.right, &expVal);
                            // Also check for -Const pattern: UnaryOp(Neg, Const)
                            if (!found) {
                                const Node& rightNode = in.at(n.right);
                                if (rightNode.type == NodeType::UnaryOp && rightNode.op == OpType::Neg) {
                                    long long innerVal;
                                    if (isIntConst(in, rightNode.left, &innerVal)) {
                                        expVal = -innerVal;
                                        found = true;
                                    }
                                }
                            }
                            if (found) {
                                if (expVal == -1) {
                                    // ∫x^(-1) dx = ∫1/x dx = ln|a|x|
                                    NodeId v = mkVar(out);
                                    if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                                    NodeId absV = mkFunc(out, FuncType::Abs, v);
                                    if (absV == INVALID_ID) return ErrorCode::ResourceExhausted;
                                    NodeId lnAbs = mkFunc(out, FuncType::Ln, absV);
                                    if (lnAbs == INVALID_ID) return ErrorCode::ResourceExhausted;
                                    *outRoot = lnAbs;
                                    if (outDomain) { outDomain->type = DomainType::NotZero; outDomain->excludedPoint = 0.0; }
                                    return ErrorCode::OK;
                                }
                                // ∫x^n dx = x^(n+1) / (n+1)
                                long long nPlus1 = expVal + 1;
                                NodeId v = mkVar(out);
                                if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId expNode = mkConst(out, (MONOCALC_REAL)nPlus1);
                                if (expNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId xPow = mkBinOp(out, OpType::Pow, v, expNode);
                                if (xPow == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId invN = mkConst(out, 1.0 / (MONOCALC_REAL)nPlus1);
                                if (invN == INVALID_ID) return ErrorCode::ResourceExhausted;
                                NodeId result = mkBinOp(out, OpType::Mul, invN, xPow);
                                if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                                *outRoot = result;
                                if (outDomain) {
                                    if (nPlus1 < 0) {
                                        outDomain->type = DomainType::NotZero;
                                        outDomain->excludedPoint = 0.0;
                                    } else {
                                        outDomain->type = DomainType::All;
                                        outDomain->excludedPoint = 0.0;
                                    }
                                }
                                return ErrorCode::OK;
                            }
                        }
                    }
                    return ErrorCode::UnsupportedOperation;
                }

                default:
                    return ErrorCode::UnsupportedOperation;
            }
        }

        case NodeType::FuncCall: {
            // Check for f(a*x + b) pattern — affine composition
            MONOCALC_REAL aCoeff, bConst;
            bool affine = isAffine(in, n.left, &aCoeff, &bConst);

            switch (n.func) {
                case FuncType::Sin: {
                    if (affine) {
                        // ∫sin(a*x+b) dx = -cos(a*x+b) / a
                        if (fabs(aCoeff) < 1e-15) {
                            // sin(constant) → constant * x
                            MONOCALC_REAL c = sin(bConst);
                            NodeId cNode = mkConst(out, c);
                            if (cNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId v = mkVar(out);
                            if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId result = mkBinOp(out, OpType::Mul, cNode, v);
                            if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                            *outRoot = result;
                            if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
                            return ErrorCode::OK;
                        }
                        // Build: a*x + b
                        NodeId aNode = mkConst(out, aCoeff);
                        if (aNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId v = mkVar(out);
                        if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId ax = mkBinOp(out, OpType::Mul, aNode, v);
                        if (ax == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId bNode = mkConst(out, bConst);
                        if (bNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId axb = mkBinOp(out, OpType::Add, ax, bNode);
                        if (axb == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // cos(a*x+b)
                        NodeId cosAxb = mkFunc(out, FuncType::Cos, axb);
                        if (cosAxb == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // -cos(a*x+b)
                        NodeId negCos = mkUnaryOp(out, OpType::Neg, cosAxb);
                        if (negCos == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // / a
                        NodeId invA = mkConst(out, 1.0 / aCoeff);
                        if (invA == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId result = mkBinOp(out, OpType::Mul, invA, negCos);
                        if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                        *outRoot = result;
                        if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
                        return ErrorCode::OK;
                    }
                    return ErrorCode::UnsupportedOperation;
                }

                case FuncType::Cos: {
                    if (affine) {
                        // ∫cos(a*x+b) dx = sin(a*x+b) / a
                        if (fabs(aCoeff) < 1e-15) {
                            MONOCALC_REAL c = cos(bConst);
                            NodeId cNode = mkConst(out, c);
                            if (cNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId v = mkVar(out);
                            if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId result = mkBinOp(out, OpType::Mul, cNode, v);
                            if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                            *outRoot = result;
                            if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
                            return ErrorCode::OK;
                        }
                        // Build: a*x + b
                        NodeId aNode = mkConst(out, aCoeff);
                        if (aNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId v = mkVar(out);
                        if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId ax = mkBinOp(out, OpType::Mul, aNode, v);
                        if (ax == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId bNode = mkConst(out, bConst);
                        if (bNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId axb = mkBinOp(out, OpType::Add, ax, bNode);
                        if (axb == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // sin(a*x+b)
                        NodeId sinAxb = mkFunc(out, FuncType::Sin, axb);
                        if (sinAxb == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // / a
                        NodeId invA = mkConst(out, 1.0 / aCoeff);
                        if (invA == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId result = mkBinOp(out, OpType::Mul, invA, sinAxb);
                        if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                        *outRoot = result;
                        if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
                        return ErrorCode::OK;
                    }
                    return ErrorCode::UnsupportedOperation;
                }

                case FuncType::Exp: {
                    if (affine) {
                        // ∫exp(a*x+b) dx = exp(a*x+b) / a
                        if (fabs(aCoeff) < 1e-15) {
                            MONOCALC_REAL c = exp(bConst);
                            NodeId cNode = mkConst(out, c);
                            if (cNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId v = mkVar(out);
                            if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                            NodeId result = mkBinOp(out, OpType::Mul, cNode, v);
                            if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                            *outRoot = result;
                            if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
                            return ErrorCode::OK;
                        }
                        // Build: a*x + b
                        NodeId aNode = mkConst(out, aCoeff);
                        if (aNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId v = mkVar(out);
                        if (v == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId ax = mkBinOp(out, OpType::Mul, aNode, v);
                        if (ax == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId bNode = mkConst(out, bConst);
                        if (bNode == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId axb = mkBinOp(out, OpType::Add, ax, bNode);
                        if (axb == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // exp(a*x+b)
                        NodeId expAxb = mkFunc(out, FuncType::Exp, axb);
                        if (expAxb == INVALID_ID) return ErrorCode::ResourceExhausted;
                        // / a
                        NodeId invA = mkConst(out, 1.0 / aCoeff);
                        if (invA == INVALID_ID) return ErrorCode::ResourceExhausted;
                        NodeId result = mkBinOp(out, OpType::Mul, invA, expAxb);
                        if (result == INVALID_ID) return ErrorCode::ResourceExhausted;
                        *outRoot = result;
                        if (outDomain) { outDomain->type = DomainType::All; outDomain->excludedPoint = 0.0; }
                        return ErrorCode::OK;
                    }
                    // exp(non-affine) → not supported
                    return ErrorCode::UnsupportedOperation;
                }

                case FuncType::Ln: {
                    // ∫ln(x) dx is not in the catalog — reject
                    // ∫ln(a*x+b) is also not supported
                    return ErrorCode::UnsupportedOperation;
                }

                case FuncType::Abs: {
                    // ∫abs(x) not in catalog
                    return ErrorCode::UnsupportedOperation;
                }

                default:
                    return ErrorCode::UnsupportedOperation;
            }
        }

        default:
            return ErrorCode::UnsupportedOperation;
    }
}

ErrorCode integrateAst(const Arena& in, NodeId root, Arena& out, NodeId* outRoot, Domain* outDomain) {
    if (outDomain) {
        outDomain->type = DomainType::All;
        outDomain->excludedPoint = 0.0;
    }
    return integrateNode(in, root, out, outRoot, outDomain);
}

} // namespace monocalc
