#ifdef XBOX_PLATFORM
#include "lwjgl/GLContext.h"

namespace lwjgl {
namespace GLContext {

void setRequestedSamples(int samples) {}
int getRequestedSamples() { return 1; }
void instantiate() {}
const detail::GLCapabilities& getCapabilities() {
    static detail::GLCapabilities caps;
    return caps;
}

} // namespace GLContext
} // namespace lwjgl
#endif
