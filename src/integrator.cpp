#include "monocalc/integrator.h"
#include "monocalc/ast.h"
#include "monocalc/error.h"

namespace monocalc {

ErrorCode integrate(const Arena& in, NodeId root, Arena& out, NodeId* outRoot) {
    // Placeholder — will be implemented in Stage 5
    (void)in; (void)root; (void)out; (void)outRoot;
    return ErrorCode::UnsupportedOperation;
}

} // namespace monocalc
