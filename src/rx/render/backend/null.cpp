#include "rx/render/backend/null.h"

namespace rx::render::backend {

allocation_info null::query_allocation_info() const {
  return { 0, 0, 0, 0, 0, 0, 0 };
}

device_info null::query_device_info() const {
  return { "", "", "" };
}

null::null(memory::allocator*, void*) {
  // {empty}
}

null::~null() {
  // {empty}
}

bool null::init() {
  return true;
}

void null::process(const vector<rx_byte*>& _commands) {
  // {empty}
}

void null::swap() {
  // {empty}
}

} // namespace rx::render::backend
