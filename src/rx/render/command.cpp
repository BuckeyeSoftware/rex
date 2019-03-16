#include <string.h> // mem{set,cpy}
#include <stddef.h> // offsetof

#include <rx/render/command.h>
#include <rx/core/memory/allocator.h>
#include <rx/core/utility/move.h>

namespace rx::render {

command_buffer::command_buffer(memory::allocator* _allocator, rx_size _size)
  : m_allocator{_allocator, _size}
{
}

command_buffer::~command_buffer() {
}

rx_byte* command_buffer::allocate(rx_size _size, command_type _command, const command_header::info& _info) {
  _size = memory::allocator::round_to_alignment(_size + sizeof(command_header));
  rx_byte* data{m_allocator.allocate(_size)};
  memset(data, 0, _size);
  memcpy(data + offsetof(command_header, type), &_command, sizeof _command);
  memcpy(data + offsetof(command_header, tag), &_info, sizeof _info);
  return data;
}

void command_buffer::reset() {
  m_allocator.reset();
}

} // namespace rx::render
