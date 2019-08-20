#include "rx/render/frontend/command.h"

#include "rx/core/memory/allocator.h" // memory::allocator::round_to_alignment

namespace rx::render::frontend {

command_buffer::command_buffer(memory::allocator* _allocator, rx_size _size)
  : m_allocator{_allocator, _size}
{
}

command_buffer::~command_buffer() {
  // ?
}

rx_byte* command_buffer::allocate(rx_size _size, command_type _command, const command_header::info& _info) {
  _size = memory::allocator::round_to_alignment(_size + sizeof(command_header));
  rx_byte* data{m_allocator.allocate(_size)};
  command_header* header{reinterpret_cast<command_header*>(data)};
  header->type = _command;
  header->tag = _info;
  return data;
}

void command_buffer::reset() {
  m_allocator.reset();
}

} // namespace rx::render::frontend
