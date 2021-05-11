#include "rx/core/report.h"

namespace Rx {

bool Report::write(Log::Level _level, const char* _data) const {
  if (m_name.is_empty()) {
    return m_log->write(_level, "%s", _data);
  } else {
    return m_log->write(_level, "%s: %s", m_name, _data);
  }
}

bool Report::rename(const String& _name) {
  // TODO(dweiler): Utility::copy.
  m_name = _name;
  return true;
}

} // namespace Rx