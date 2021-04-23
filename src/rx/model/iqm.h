#ifndef RX_MODEL_IQM_H
#define RX_MODEL_IQM_H
#include "rx/model/importer.h"
#include "rx/core/linear_buffer.h"

namespace Rx::Model {

struct IQM
  : Importer
{
  IQM();
  IQM(Memory::Allocator& _allocator);

  struct Header;

  [[nodiscard]] virtual bool read(Stream::UntrackedStream& _stream);

private:
  bool read_meshes(const Header& _header, const LinearBuffer& _data);
  bool read_animations(const Header& _header, const LinearBuffer& _data);
};

inline IQM::IQM(Memory::Allocator& _allocator)
  : Importer{_allocator}
{
}

} // namespace Rx::Model

#endif // RX_MODEL_IQM_H
