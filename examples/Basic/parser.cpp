#include "parser.h"
#include "token.h"
#include "ast.h"
#include "error.h"
#include <string.h>

namespace monocalc {

// Pratt parser
// Binding powers (higher = tighter):
//   ^ (right-associative): led=10
//   * /:                   led=8
//   + -:                   led=6
//   Unary +/-:             binds tighter than ^ (handled in parseUnary)

static const size_t MAX_DEPTH = 64;

struct Parser {
    const Token* tokens;
    size_t count;
    size_t pos;
    Arena& arena;
    size_t depth;

    Parser(const Token* t, size_t c, Arena& a) : tokens(t), count(c), pos(0), arena(a), depth(0) {}

    ErrorCode parse(NodeId* outRoot) {
        if (count == 0 || tokens[0].type == TokenType::EOF_) return ErrorCode::SyntaxError;
        ErrorCode err = parseExpr(0, outRoot);
        if (err != ErrorCode::OK) return err;
        if (pos < count && tokens[pos].type != TokenType::EOF_) {
            return ErrorCode::SyntaxError;
        }
        return ErrorCode::OK;
    }

    // Parse expression with minimum binding power
    ErrorCode parseExpr(unsigned minPrec, NodeId* out) {
        if (depth >= MAX_DEPTH) return ErrorCode::ResourceExhausted;
        depth++;

        // Parse the left-hand side (primary with possible unary operators)
        NodeId left;
        ErrorCode err = parseUnary(&left);
        if (err != ErrorCode::OK) { depth--; return err; }

        // Chain binary operators
        while (pos < count && tokens[pos].type != TokenType::EOF_ && tokens[pos].type != TokenType::RParen) {
            unsigned prec = ledPrec(tokens[pos].type);
            if (prec < minPrec) break;

            TokenType op = tokens[pos].type;
            pos++;

            unsigned nextPrec;
            if (op == TokenType::Caret) {
                // Right-associative: RHS uses prec-1 so x^2^3 = x^(2^3)
                nextPrec = prec - 1;
            } else {
                nextPrec = prec;
            }

            NodeId right;
            err = parseExpr(nextPrec, &right);
            if (err != ErrorCode::OK) { depth--; return err; }

            NodeId nid = arena.alloc();
            if (nid == INVALID_ID) { depth--; return ErrorCode::ResourceExhausted; }
            Node& n = arena.at(nid);
            n.type = NodeType::BinaryOp;
            switch (op) {
                case TokenType::Plus: n.op = OpType::Add; break;
                case TokenType::Minus: n.op = OpType::Sub; break;
                case TokenType::Star: n.op = OpType::Mul; break;
                case TokenType::Slash: n.op = OpType::Div; break;
                case TokenType::Caret: n.op = OpType::Pow; break;
                default: depth--; return ErrorCode::SyntaxError;
            }
            n.left = left;
            n.right = right;
            left = nid;
        }

        depth--;
        *out = left;
        return ErrorCode::OK;
    }

    // Parse unary operators and primaries
    // This handles -x, +x, --x, etc.
    // Key: unary binds tighter than ^, so -x^2 = -(x^2)
    ErrorCode parseUnary(NodeId* out) {
        if (pos >= count || tokens[pos].type == TokenType::EOF_) {
            return ErrorCode::SyntaxError;
        }

        if (tokens[pos].type == TokenType::Plus || tokens[pos].type == TokenType::Minus) {
            TokenType op = tokens[pos].type;
            pos++;

            // Parse the operand: allow ^ but not * / + -
            // This ensures -x^2 = -(x^2) because ^ binds tighter
            NodeId operand;
            ErrorCode err = parseUnaryOperand(&operand);
            if (err != ErrorCode::OK) return err;

            NodeId nid = arena.alloc();
            if (nid == INVALID_ID) return ErrorCode::ResourceExhausted;
            Node& n = arena.at(nid);
            n.type = NodeType::UnaryOp;
            n.op = (op == TokenType::Minus) ? OpType::Neg : OpType::Pos;
            n.left = operand;
            n.right = INVALID_ID;
            *out = nid;
            return ErrorCode::OK;
        }

        return parsePrimary(out);
    }

    // Parse the operand of a unary operator
    // Allows ^ (power) but not * / + -
    // This is what makes -x^2 = -(x^2)
    ErrorCode parseUnaryOperand(NodeId* out) {
        NodeId base;
        ErrorCode err = parsePrimary(&base);
        if (err != ErrorCode::OK) return err;

        // Allow ^ after the base (right-associative)
        while (pos < count && tokens[pos].type == TokenType::Caret) {
            pos++;
            NodeId exp;
            err = parseUnaryOperand(&exp);
            if (err != ErrorCode::OK) return err;

            NodeId nid = arena.alloc();
            if (nid == INVALID_ID) return ErrorCode::ResourceExhausted;
            Node& n = arena.at(nid);
            n.type = NodeType::BinaryOp;
            n.op = OpType::Pow;
            n.left = base;
            n.right = exp;
            base = nid;
        }

        *out = base;
        return ErrorCode::OK;
    }

    ErrorCode parsePrimary(NodeId* out) {
        if (pos >= count || tokens[pos].type == TokenType::EOF_) {
            return ErrorCode::SyntaxError;
        }

        if (tokens[pos].type == TokenType::Number) {
            NodeId nid = arena.alloc();
            if (nid == INVALID_ID) return ErrorCode::ResourceExhausted;
            Node& n = arena.at(nid);
            n.type = NodeType::Const;
            n.constant = tokens[pos].number;
            n.left = INVALID_ID;
            n.right = INVALID_ID;
            *out = nid;
            pos++;
            return ErrorCode::OK;
        }

        if (tokens[pos].type == TokenType::Ident) {
            if (pos + 1 < count && tokens[pos + 1].type == TokenType::LParen) {
                FuncType ft;
                if (memcmp(tokens[pos].ident, "sin", 4) == 0) ft = FuncType::Sin;
                else if (memcmp(tokens[pos].ident, "cos", 4) == 0) ft = FuncType::Cos;
                else if (memcmp(tokens[pos].ident, "exp", 4) == 0) ft = FuncType::Exp;
                else if (memcmp(tokens[pos].ident, "ln", 3) == 0) ft = FuncType::Ln;
                else if (memcmp(tokens[pos].ident, "abs", 4) == 0) ft = FuncType::Abs;
                else return ErrorCode::SyntaxError;

                pos++; // skip ident
                pos++; // skip (

                NodeId arg;
                ErrorCode err = parseExpr(0, &arg);
                if (err != ErrorCode::OK) return err;

                if (pos >= count || tokens[pos].type != TokenType::RParen) {
                    return ErrorCode::SyntaxError;
                }
                pos++; // skip )

                NodeId nid = arena.alloc();
                if (nid == INVALID_ID) return ErrorCode::ResourceExhausted;
                Node& n = arena.at(nid);
                n.type = NodeType::FuncCall;
                n.func = ft;
                n.left = arg;
                n.right = INVALID_ID;
                *out = nid;
                return ErrorCode::OK;
            }

            if (memcmp(tokens[pos].ident, "x", 2) == 0) {
                NodeId nid = arena.alloc();
                if (nid == INVALID_ID) return ErrorCode::ResourceExhausted;
                Node& n = arena.at(nid);
                n.type = NodeType::Var;
                n.left = INVALID_ID;
                n.right = INVALID_ID;
                *out = nid;
                pos++;
                return ErrorCode::OK;
            }

            return ErrorCode::SyntaxError;
        }

        if (tokens[pos].type == TokenType::LParen) {
            pos++;
            NodeId expr;
            ErrorCode err = parseExpr(0, &expr);
            if (err != ErrorCode::OK) return err;
            if (pos >= count || tokens[pos].type != TokenType::RParen) {
                return ErrorCode::SyntaxError;
            }
            pos++;
            *out = expr;
            return ErrorCode::OK;
        }

        return ErrorCode::SyntaxError;
    }

    static unsigned ledPrec(TokenType t) {
        switch (t) {
            case TokenType::Plus:
            case TokenType::Minus: return 6;
            case TokenType::Star:
            case TokenType::Slash: return 8;
            case TokenType::Caret: return 10;
            default: return 0;
        }
    }
};

ErrorCode parse(const Token* tokens, size_t count, Arena& arena, NodeId* outRoot) {
    Parser p(tokens, count, arena);
    return p.parse(outRoot);
}

} // namespace monocalc
