#include "rx/render/frontend/command.h"

namespace Rx::Render::Frontend {

CommandBuffer::CommandBuffer(Memory::Allocator& _allocator, Size _size)
  : m_base_allocator{_allocator}
  , m_base_memory{m_base_allocator.allocate(_size)}
  , m_allocator{m_base_memory, _size}
{
}

CommandBuffer::~CommandBuffer() {
  m_base_allocator.deallocate(m_base_memory);
}

Byte* CommandBuffer::allocate(Size _size, CommandType _command, const CommandHeader::Info& _info) {
  Byte* data = m_allocator.allocate(sizeof(CommandHeader) + _size);
  RX_ASSERT(data, "Out of memory");

  auto* header = reinterpret_cast<CommandHeader*>(data);
  header->type = _command;
  header->tag = _info;

  return data;
}

void CommandBuffer::reset() {
  m_allocator.reset();
}

} // namespace rx::render::frontend
