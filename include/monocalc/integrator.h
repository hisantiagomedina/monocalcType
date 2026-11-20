#ifndef MONOCALC_INTEGRATOR_H
#define MONOCALC_INTEGRATOR_H

#include "ast.h"
#include "error.h"

namespace monocalc {

ErrorCode integrate(const Arena& in, NodeId root, Arena& out, NodeId* outRoot);

} // namespace monocalc

#endif // MONOCALC_INTEGRATOR_H
