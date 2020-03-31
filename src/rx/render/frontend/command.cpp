#include "rx/render/frontend/command.h"

#include "rx/core/memory/allocator.h" // memory::allocator::round_to_alignment

namespace rx::render::frontend {

command_buffer::command_buffer(memory::allocator& _allocator, rx_size _size)
  : m_base_allocator{_allocator}
  , m_base_memory{m_base_allocator.allocate(_size)}
  , m_allocator{m_base_memory, _size}
{
}

command_buffer::~command_buffer() {
  m_base_allocator.deallocate(m_base_memory);
}

rx_byte* command_buffer::allocate(rx_size _size, command_type _command, const command_header::info& _info) {
  rx_byte* data{m_allocator.allocate(sizeof(command_header) + _size)};
  RX_ASSERT(data, "out of memory in command buffer");
  command_header* header{reinterpret_cast<command_header*>(data)};
  header->type = _command;
  header->tag = _info;
  return data;
}

void command_buffer::reset() {
  m_allocator.reset();
}

} // namespace rx::render::frontend
