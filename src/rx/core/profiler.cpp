#include "rx/core/profiler.h"

namespace rx {

global<profiler> profiler::s_profiler{"system", "profiler"};

}
