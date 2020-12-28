#ifndef RX_MODEL_IQM_H
#define RX_MODEL_IQM_H
#include "rx/model/importer.h"

namespace Rx::Model {

struct IQM
  : Importer
{
  IQM();
  IQM(Memory::Allocator& _allocator);

  struct Header;

  virtual bool read(Stream* _stream);

private:
  bool read_meshes(const Header& _header, const Vector<Byte>& _data);
  bool read_animations(const Header& _header, const Vector<Byte>& _data);
};

inline IQM::IQM(Memory::Allocator& _allocator)
  : Importer{_allocator}
{
}

} // namespace Rx::Model

#endif // RX_MODEL_IQM_H
