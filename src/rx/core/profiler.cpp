#include "rx/core/profiler.h"

namespace rx {

global<profiler> profiler::s_instance{"system", "profiler"};

} // namespace rx
