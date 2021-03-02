#include "rx/core/filesystem/file.h"
#include "rx/core/json.h"

#include "rx/render/frontend/module.h"

namespace Rx::Render::Frontend {

RX_LOG("render/module", logger);

Module::Module(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_name{allocator()}
  , m_source{allocator()}
  , m_dependencies{allocator()}
{
}

Module::Module(Module&& module_)
  : m_allocator{module_.m_allocator}
  , m_name{Utility::move(module_.m_name)}
  , m_source{Utility::move(module_.m_source)}
  , m_dependencies{Utility::move(module_.m_dependencies)}
{
}

Module& Module::operator=(Module&& module_) {
  m_allocator = module_.m_allocator;
  m_name = Utility::move(module_.m_name);
  m_source = Utility::move(module_.m_source);
  m_dependencies = Utility::move(module_.m_dependencies);

  return *this;
}

bool Module::load(Stream* _stream) {
  if (auto data = read_text_stream(allocator(), _stream)) {
    if (auto disown = data->disown()) {
      if (auto json = JSON::parse(allocator(), *disown)) {
        return parse(*json);
      }
    }
  }
  return false;
}

bool Module::load(const String& _file_name) {
  if (Filesystem::File file{_file_name, "rb"}) {
    return load(&file);
  }
  return false;
}

bool Module::parse(const JSON& _description) {
  if (!_description) {
    const auto json_error{_description.error()};
    if (json_error) {
      return error("%s", *json_error);
    } else {
      return error("empty description");
    }
  }

  const JSON& name{_description["name"]};
  const JSON& source{_description["source"]};

  if (!name) {
    return error("missing 'name'");
  }

  if (!name.is_string()) {
    return error("expected String for 'name'");
  }

  m_name = name.as_string();

  if (!source) {
    return error("missing 'source'");
  }

  if (!source.is_string()) {
    return error("expected String for 'source'");
  }

  // Trim any leading and trailing whitespace characters from the contents too.
  m_source = source.as_string().strip("\t\r\n ");

  const JSON& imports = _description["imports"];
  if (!imports) {
    return true;
  }

  if (!imports.is_array_of(JSON::Type::STRING)) {
    return error("expected Array[String] for 'imports'");
  }

  return imports.each([&](const JSON& _import) {
    return m_dependencies.push_back(_import.as_string());
  });
}

bool resolve_module_dependencies(
  const Map<String, Module>& _modules,
  const Module& _current_module,
  Set<String>& visited_,
  Algorithm::TopologicalSort<String>& sorter_)
{
  if (!sorter_.add(_current_module.name())) {
    return false;
  }

  // For each dependency of this module.
  return _current_module.dependencies().each_fwd([&](const String& _dependency) {
    // Add the dependency to the topological sorter.
    if (!sorter_.add(_current_module.name(), _dependency)) {
      return false;
    }

    // Break cycles in the visitation.
    if (visited_.find(_dependency)) {
      return true;
    }

    if (!visited_.insert(_dependency)) {
      return false;
    }

    // Recursively apply dependencies.
    if (auto find = _modules.find(_dependency)) {
      return resolve_module_dependencies(_modules, *find, visited_, sorter_);
    }

    return true;
  });
}

void Module::write_log(Log::Level _level, String&& message_) const {
  if (m_name.is_empty()) {
    logger->write(_level, Utility::move(message_));
  } else {
    logger->write(_level, "module '%s': %s", m_name, Utility::move(message_));
  }
}

} // namespace Rx::Render::Frontend
