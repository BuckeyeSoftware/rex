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
  if (auto name = Utility::copy(_name)) {
    m_name = Utility::move(*name);
    return true;
  }
  return false;
}

} // namespace Rx