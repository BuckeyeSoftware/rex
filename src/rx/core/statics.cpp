#include <string.h> // strcmp

#include <rx/core/statics.h>
#include <rx/core/debug.h> // RX_MESSAGE

#include <rx/core/concurrency/spin_lock.h> // spin_lock
#include <rx/core/concurrency/scope_lock.h> // scope_lock
#include <rx/core/concurrency/scope_unlock.h> // scope_unlock

namespace rx {

using concurrency::scope_lock;
using concurrency::scope_unlock;
using concurrency::spin_lock;

static spin_lock g_lock;

static static_node* g_head; // protected by |g_lock|
static static_node* g_tail; // protected by |g_lock|

void static_node::link() {
  scope_lock<spin_lock> locked(g_lock);

  if (!g_head) {
    g_head = this;
  }

  if (g_tail) {
    m_prev = g_tail;
    m_prev->m_next = this;
  }

  g_tail = this;
}

void static_globals::init() {
  RX_MESSAGE("init static globals");
  scope_lock<spin_lock> locked(g_lock);
  for (auto node{g_head}; node; node = node->m_next) {
    scope_unlock<spin_lock> unlocked(g_lock);
    node->init_global();
  }
}

void static_globals::fini() {
  RX_MESSAGE("fini static globals");
  scope_lock<spin_lock> locked(g_lock);
  for (auto node{g_tail}; node; node = node->m_prev) {
    scope_unlock<spin_lock> unlocked(g_lock);
    node->fini_global();
  }
}

static_node* static_globals::find(const char* name) {
  scope_lock<spin_lock> locked(g_lock);
  for (auto node{g_head}; node; node = node->m_next) {
    if (!strcmp(node->m_name, name)) {
      return node;
    }
  }
  return nullptr;
}

#if 0
void static_globals::remove(static_node* node) {
  scope_lock<spin_lock> locked(g_lock);
  if (node->m_next) {
    node->m_next->m_prev = node->m_prev;
  }

  if (node->m_prev) {
    node->m_prev->m_next = node->m_next;
  }

  if (g_head == node) {
    g_head = node->m_next;
  }

  if (g_tail == node) {
    g_tail = node->m_prev;
  }
}
#endif

void static_globals::lock() {
  g_lock.lock();
}

void static_globals::unlock() {
  g_lock.unlock();
}

static_node* static_globals::head() {
  return g_head;
}

static_node* static_globals::tail() {
  return g_tail;
}

} // namespace rx
