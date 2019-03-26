#ifndef RX_MODEL_IQM_H
#define RX_MODEL_IQM_H

#include <rx/model/loader.h>

namespace rx::model {

struct iqm : loader {
  iqm();
  iqm(memory::allocator* _allocator);

  struct header;

  virtual bool read(const array<rx_byte>& _data);

private:
  bool read_meshes(const header& _header, const array<rx_byte>& _data);
  bool read_animations(const header& _header, const array<rx_byte>& _data);
};

inline iqm::iqm()
  : iqm{&memory::g_system_allocator}
{
}

inline iqm::iqm(memory::allocator* _allocator)
  : loader{_allocator}
{
}

} // namespace rx::model

#endif // RX_MODEL_IQM_H