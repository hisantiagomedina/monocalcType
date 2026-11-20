#ifndef MONOCALC_PARSER_H
#define MONOCALC_PARSER_H

#include "token.h"
#include "ast.h"
#include "error.h"

namespace monocalc {

ErrorCode parse(const Token* tokens, size_t count, Arena& arena, NodeId* outRoot);

} // namespace monocalc

#endif // MONOCALC_PARSER_H
