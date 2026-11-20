#ifndef MONOCALC_AST_H
#define MONOCALC_AST_H

#include <cstddef>

#ifndef MONOCALC_REAL
#define MONOCALC_REAL double
#endif

namespace monocalc {

enum class NodeType : unsigned char { Const, Var, UnaryOp, BinaryOp, FuncCall };
enum class OpType : unsigned char { Add, Sub, Mul, Div, Pow, Neg, Pos };
enum class FuncType : unsigned char { Sin, Cos, Exp, Ln, Abs };

typedef size_t NodeId;
static const NodeId INVALID_ID = static_cast<NodeId>(-1);

struct Node {
    NodeType type;
    union {
        MONOCALC_REAL constant;
        OpType op;
        FuncType func;
    };
    NodeId left;
    NodeId right;
};

struct Arena {
    Node* nodes;
    size_t count;
    size_t capacity;

    void init(Node* n, size_t c, size_t cap) {
        nodes = n;
        count = c;
        capacity = cap;
    }

    NodeId alloc() {
        if (count >= capacity) return INVALID_ID;
        return count++;
    }

    Node& at(NodeId id) { return nodes[id]; }
    const Node& at(NodeId id) const { return nodes[id]; }
};

} // namespace monocalc

#endif // MONOCALC_AST_H
