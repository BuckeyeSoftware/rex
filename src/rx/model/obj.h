#ifndef RX_MODEL_OBJ
#define RX_MODEL_OBJ
#include "rx/model/importer.h"

namespace Rx::Model {

struct OBJ
  : Importer
{
  OBJ(Memory::Allocator& _allocator);

  [[nodiscard]] virtual bool read(Stream::Context& _stream);
};

inline OBJ::OBJ(Memory::Allocator& _allocator)
  : Importer{_allocator}
{
}

} // namespace Rx::Model

#endif // RX_MODEL_OBJ