#include "rx/render/backend/null.h"

namespace Rx::Render::Backend {

AllocationInfo Null::query_allocation_info() const {
  return { 0, 0, 0, 0, 0, 0, 0 };
}

DeviceInfo Null::query_device_info() const {
  return { "", "", "" };
}

Null::Null(Memory::Allocator&, void*) {
  // {empty}
}

Null::~Null() {
  // {empty}
}

bool Null::init() {
  return true;
}

void Null::process(const Vector<Byte*>&) {
  // {empty}
}

void Null::swap() {
  // {empty}
}

} // namespace rx::render::backend
