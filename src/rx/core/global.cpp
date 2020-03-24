#include <string.h> // strcmp
#include <stdlib.h> // malloc, free

#include "rx/core/global.h"
#include "rx/core/log.h"

#include "rx/core/concurrency/scope_lock.h"
#include "rx/core/concurrency/spin_lock.h"

namespace rx {

RX_LOG("global", logger);

static concurrency::spin_lock g_lock;

// global_node
void global_node::init_global() {
  const auto flags = m_argument_store.as_tag();
  if (!(flags & k_enabled)) {
    return;
  }

  RX_ASSERT(!(flags & k_initialized), "already initialized");
  logger->verbose("%p init: %s/%s", this, m_group, m_name);

  m_storage_dispatch(storage_mode::k_init_global, data(), m_argument_store.as_ptr());

  m_argument_store.retag(flags | k_initialized);
}

void global_node::fini_global() {
  const auto flags = m_argument_store.as_tag();

  if (!(flags & k_enabled)) {
    return;
  }

  RX_ASSERT(flags & k_initialized, "not initialized");
  logger->verbose("%p fini: %s/%s", this, m_group, m_name);

  m_storage_dispatch(storage_mode::k_fini_global, data(), nullptr);
  if (flags & k_arguments) {
    auto argument_store = m_argument_store.as_ptr();
    m_storage_dispatch(storage_mode::k_fini_arguments, nullptr, argument_store);
    reallocate_arguments(argument_store, 0);
  }

  m_argument_store.retag(flags & ~k_initialized);
}

void global_node::init() {
  const auto flags = m_argument_store.as_tag();
  RX_ASSERT(!(flags & k_initialized), "already initialized");

  m_storage_dispatch(storage_mode::k_init_global, data(), m_argument_store.as_ptr());

  m_argument_store.retag((flags & ~k_enabled) | k_initialized);
}

void global_node::fini() {
  const auto flags = m_argument_store.as_tag();
  RX_ASSERT(flags & k_initialized, "not initialized");

  m_storage_dispatch(storage_mode::k_fini_global, data(), nullptr);
  if (flags & k_arguments) {
    auto argument_store = m_argument_store.as_ptr();
    m_storage_dispatch(storage_mode::k_fini_arguments, nullptr, argument_store);
    reallocate_arguments(argument_store, 0);
  }

  m_argument_store.retag((flags & ~k_enabled) | k_initialized);
}

rx_byte* global_node::reallocate_arguments(rx_byte* _existing, rx_size _size) {
  if (_existing && _size == 0) {
    free(_existing);
    return nullptr;
  }
  return reinterpret_cast<rx_byte*>(realloc(nullptr, _size));
}

// global_group
global_node* global_group::find(const char* _name) {
  for (auto node = m_list.enumerate_head(&global_node::m_grouped); node; node.next()) {
    if (!strcmp(node->name(), _name)) {
      return node.data();
    }
  }
  return nullptr;
}

void global_group::init() {
  for (auto node = m_list.enumerate_head(&global_node::m_grouped); node; node.next()) {
    node->init();
  }
}

void global_group::fini() {
  for (auto node = m_list.enumerate_tail(&global_node::m_grouped); node; node.prev()) {
    node->fini();
  }
}

void global_group::init_global() {
  for (auto node = m_list.enumerate_head(&global_node::m_grouped); node; node.next()) {
    node->init_global();
  }
}

void global_group::fini_global() {
  for (auto node = m_list.enumerate_tail(&global_node::m_grouped); node; node.prev()) {
    node->fini_global();
  }
}

// globals
global_group* globals::find(const char* _name) {
  for (auto group = s_group_list.enumerate_head(&global_group::m_link); group; group.next()) {
    if (!strcmp(group->name(), _name)) {
      return group.data();
    }
  }
  return nullptr;
}

void globals::link() {
  // Link ungrouped globals from |s_node_list| managed by |global_node::m_ungrouped|
  // with the appropriate group given by |global_group::m_list|, which is managed
  // by |global_node::m_grouped| when the given global shares the same group
  // name as the group.
  concurrency::scope_lock lock{g_lock};
  for (auto node = s_node_list.enumerate_head(&global_node::m_ungrouped); node; node.next()) {
    bool unlinked = true;
    for (auto group = s_group_list.enumerate_head(&global_group::m_link); group; group.next()) {
      if (!strcmp(node->m_group, group->name())) {
        group->m_list.push(&node->m_grouped);
        unlinked = false;
        break;
      }
    }

    if (unlinked) {
      // NOTE(dweiler): If you've hit this code-enforced crash it means there
      // exists an rx::global<T> that is associated with a group by name which
      // doesn't exist. This can be caused by misnaming the group in the
      // global's constructor, or because the rx::global_group with that name
      // doesn't exist in any translation unit.
      *reinterpret_cast<volatile int*>(0) = 0;
    }
  }
}

void globals::init() {
  for (auto group = s_group_list.enumerate_head(&global_group::m_link); group; group.next()) {
    group->init_global();
  }
}

void globals::fini() {
  for (auto group = s_group_list.enumerate_tail(&global_group::m_link); group; group.prev()) {
    group->fini_global();
  }
}

void globals::link(global_node* _node) {
  concurrency::scope_lock lock{g_lock};
  s_node_list.push(&_node->m_ungrouped);
}

void globals::link(global_group* _group) {
  concurrency::scope_lock lock{g_lock};
  s_group_list.push(&_group->m_link);
}

static global_group g_group_system{"system"};

} // namespace rx
