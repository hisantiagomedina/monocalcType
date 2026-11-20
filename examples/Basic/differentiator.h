#ifndef MONOCALC_DIFFERENTIATOR_H
#define MONOCALC_DIFFERENTIATOR_H

#include "ast.h"
#include "error.h"

namespace monocalc {

ErrorCode differentiate(const Arena& in, NodeId root, Arena& out, NodeId* outRoot);

} // namespace monocalc

#endif // MONOCALC_DIFFERENTIATOR_H
