#ifndef MONOCALC_PARSER_H
#define MONOCALC_PARSER_H

#include "monocalc/token.h"
#include "monocalc/ast.h"
#include "monocalc/error.h"

namespace monocalc {

ErrorCode parse(const Token* tokens, size_t count, Arena& arena, NodeId* outRoot);

} // namespace monocalc

#endif // MONOCALC_PARSER_H
