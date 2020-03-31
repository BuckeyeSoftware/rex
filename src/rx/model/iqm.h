#ifndef RX_MODEL_IQM_H
#define RX_MODEL_IQM_H
#include "rx/model/importer.h"

namespace rx::model {

struct iqm
  : importer
{
  iqm();
  iqm(memory::allocator& _allocator);

  struct header;

  virtual bool read(stream* _stream);

private:
  bool read_meshes(const header& _header, const vector<rx_byte>& _data);
  bool read_animations(const header& _header, const vector<rx_byte>& _data);
};

inline iqm::iqm(memory::allocator& _allocator)
  : importer{_allocator}
{
}

} // namespace rx::model

#endif // RX_MODEL_IQM_H
