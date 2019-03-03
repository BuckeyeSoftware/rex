#include <string.h> // strcmp

#include <rx/console/console.h>
#include <rx/console/variable.h>

#include <rx/core/concurrency/spin_lock.h>
#include <rx/core/concurrency/scope_lock.h>

namespace rx::console {

using rx::concurrency::spin_lock;
using rx::concurrency::scope_lock;

static spin_lock g_lock;
static variable_reference* g_head; // protected by |g_lock|

bool console::load(const char* file_name) {
  // sort references
  {
    scope_lock<spin_lock> locked(g_lock);
    g_head = sort(g_head);
  }

  return true;
}

bool console::save(const char* file_name) {
  return true;
}

variable_reference* console::add_variable_reference(variable_reference* reference) {
  scope_lock<spin_lock> locked(g_lock);
  variable_reference* next = g_head;
  g_head = reference;
  return next;
}

variable_reference* console::split(variable_reference* reference) {
  if (!reference || !reference->m_next) {
    return nullptr;
  }
  variable_reference* splitted = reference->m_next;
  reference->m_next = splitted->m_next;
  splitted->m_next = split(splitted->m_next);
  return splitted;
}

variable_reference* console::merge(variable_reference* lhs, variable_reference* rhs) {
  if (!lhs) {
    return rhs;
  }

  if (!rhs) {
    return lhs;
  }

  if (strcmp(lhs->m_name, rhs->m_name) > 0) {
    rhs->m_next = merge(lhs, rhs->m_next);
    return rhs;
  }

  lhs->m_next = merge(lhs->m_next, rhs);
  return lhs;
}

variable_reference* console::sort(variable_reference* reference) {
  if (!reference) {
    return nullptr;
  }

  if (!reference->m_next) {
    return reference;
  }

  variable_reference* splitted = split(reference);

  return merge(sort(reference), sort(splitted));
}

} // namespace rx::console
