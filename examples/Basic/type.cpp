#include "type.h"
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include "differentiator.h"
#include "integrator.h"
#include "limiter.h"
#include "domain.h"
#include "error.h"
#include <string.h>
#include <stdlib.h>

namespace monocalc {

ParseResult parse(const char* input, size_t len, Function& out) {
    out.root = INVALID_ID;
    out.domain.type = DomainType::All;
    out.domain.excludedPoint = 0.0;
    out.inputLen = 0;
    out.input[0] = '\0';

    if (!input || len == 0) {
        ParseResult r;
        r.ok = false; r.func = out; r.err = ErrorCode::SyntaxError; r.pos = 0;
        return r;
    }

    size_t copyLen = len < sizeof(out.input) - 1 ? len : sizeof(out.input) - 1;
    memcpy(out.input, input, copyLen);
    out.input[copyLen] = '\0';
    out.inputLen = copyLen;

    Node* nodes = (Node*)malloc(sizeof(Node) * DEFAULT_ARENA_CAPACITY);
    if (!nodes) {
        ParseResult r;
        r.ok = false; r.func = out; r.err = ErrorCode::ResourceExhausted; r.pos = 0;
        return r;
    }
    out.arena.init(nodes, 0, DEFAULT_ARENA_CAPACITY);

    Token tokens[DEFAULT_TOKEN_CAPACITY];
    size_t tokenCount = 0;
    ErrorCode err = tokenize(input, len, tokens, DEFAULT_TOKEN_CAPACITY, &tokenCount);
    if (err != ErrorCode::OK) {
        free(nodes);
        out.arena.init(nullptr, 0, 0);
        ParseResult r;
        r.ok = false; r.func = out; r.err = err; r.pos = 0;
        return r;
    }

    // Check for implicit multiplication
    for (size_t i = 0; i + 1 < tokenCount; i++) {
        if (tokens[i].type == TokenType::Number && tokens[i+1].type == TokenType::Ident) {
            free(nodes);
            out.arena.init(nullptr, 0, 0);
            ParseResult r;
            r.ok = false; r.func = out; r.err = ErrorCode::SyntaxError; r.pos = tokens[i+1].pos;
            return r;
        }
        if (tokens[i].type == TokenType::RParen && tokens[i+1].type == TokenType::LParen) {
            free(nodes);
            out.arena.init(nullptr, 0, 0);
            ParseResult r;
            r.ok = false; r.func = out; r.err = ErrorCode::SyntaxError; r.pos = tokens[i+1].pos;
            return r;
        }
        if (tokens[i].type == TokenType::RParen && tokens[i+1].type == TokenType::Ident) {
            free(nodes);
            out.arena.init(nullptr, 0, 0);
            ParseResult r;
            r.ok = false; r.func = out; r.err = ErrorCode::SyntaxError; r.pos = tokens[i+1].pos;
            return r;
        }
    }

    NodeId root;
    err = parse(tokens, tokenCount, out.arena, &root);
    if (err != ErrorCode::OK) {
        free(nodes);
        out.arena.init(nullptr, 0, 0);
        ParseResult r;
        r.ok = false; r.func = out; r.err = err; r.pos = 0;
        return r;
    }
    out.root = root;

    ParseResult r;
    r.ok = true; r.func = out; r.err = ErrorCode::OK; r.pos = 0;
    return r;
}

EvalResult compute(const Function& f, MONOCALC_REAL x) {
    if (f.root == INVALID_ID) {
        EvalResult r; r.ok = false; r.value = 0.0; r.err = ErrorCode::SyntaxError;
        return r;
    }
    ErrorCode dc = domainCheck(f.domain, x);
    if (dc != ErrorCode::OK) {
        EvalResult r; r.ok = false; r.value = 0.0; r.err = dc;
        return r;
    }
    return evaluate(f.arena, f.root, x);
}

ErrorCode derive(const Function& f, Function& out) {
    out.root = INVALID_ID;
    out.domain.type = DomainType::All;
    out.domain.excludedPoint = 0.0;
    out.inputLen = 0;
    out.input[0] = '\0';

    if (f.root == INVALID_ID) return ErrorCode::SyntaxError;

    Node* nodes = (Node*)malloc(sizeof(Node) * DEFAULT_ARENA_CAPACITY);
    if (!nodes) return ErrorCode::ResourceExhausted;
    out.arena.init(nodes, 0, DEFAULT_ARENA_CAPACITY);

    NodeId root;
    ErrorCode err = differentiate(f.arena, f.root, out.arena, &root);
    if (err != ErrorCode::OK) {
        free(nodes);
        out.arena.init(nullptr, 0, 0);
        return err;
    }
    out.root = root;
    return ErrorCode::OK;
}

ErrorCode integrate(const Function& f, Function& out) {
    out.root = INVALID_ID;
    out.domain.type = DomainType::All;
    out.domain.excludedPoint = 0.0;
    out.inputLen = 0;
    out.input[0] = '\0';

    if (f.root == INVALID_ID) return ErrorCode::SyntaxError;

    Node* nodes = (Node*)malloc(sizeof(Node) * DEFAULT_ARENA_CAPACITY);
    if (!nodes) return ErrorCode::ResourceExhausted;
    out.arena.init(nodes, 0, DEFAULT_ARENA_CAPACITY);

    NodeId root;
    Domain d;
    ErrorCode err = integrateAst(f.arena, f.root, out.arena, &root, &d);
    if (err != ErrorCode::OK) {
        free(nodes);
        out.arena.init(nullptr, 0, 0);
        return err;
    }
    out.root = root;
    // Merge domain: integration may add constraints
    if (d.type != DomainType::All) {
        out.domain = d;
    } else {
        // Inherit original domain if integration doesn't add constraints
        out.domain = f.domain;
    }
    return ErrorCode::OK;
}

LimitResult lim(const Function& f, MONOCALC_REAL x0, LimitDirection dir) {
    if (f.root == INVALID_ID) {
        LimitResult r; r.ok = false; r.value = 0.0; r.err = ErrorCode::SyntaxError;
        return r;
    }
    return limit(f.arena, f.root, x0, dir);
}

} // namespace monocalc
