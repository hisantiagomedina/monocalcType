#ifndef MONOCALC_INTEGRATOR_H
#define MONOCALC_INTEGRATOR_H

#include "ast.h"
#include "error.h"
#include "domain.h"

namespace monocalc {

/* Integrate an AST node. Returns OK on success with outRoot pointing to the
   antiderivative root, and fills outDomain with any domain constraints.
   Returns UnsupportedOperation for patterns not in the integration catalog. */
ErrorCode integrateAst(const Arena& in, NodeId root, Arena& out, NodeId* outRoot, Domain* outDomain);

} // namespace monocalc

#endif // MONOCALC_INTEGRATOR_H
