#include "rx/core/filesystem/unbuffered_file.h"
#include "rx/core/serialize/json.h"

#include "rx/render/frontend/module.h"

namespace Rx::Render::Frontend {

RX_LOG("render/module", logger);

Module::Module(Memory::Allocator& _allocator)
  : m_allocator{&_allocator}
  , m_name{allocator()}
  , m_source{allocator()}
  , m_dependencies{allocator()}
  , m_report{allocator(), *logger}
{
}

Module::Module(Module&& module_)
  : m_allocator{module_.m_allocator}
  , m_name{Utility::move(module_.m_name)}
  , m_source{Utility::move(module_.m_source)}
  , m_dependencies{Utility::move(module_.m_dependencies)}
  , m_report{Utility::move(module_.m_report)}
{
}

Module& Module::operator=(Module&& module_) {
  m_allocator = module_.m_allocator;
  m_name = Utility::move(module_.m_name);
  m_source = Utility::move(module_.m_source);
  m_dependencies = Utility::move(module_.m_dependencies);
  m_report = Utility::move(module_.m_report);
  return *this;
}

bool Module::load(Stream::Context& _stream) {
  if (auto data = _stream.read_text(allocator())) {
    if (auto disown = data->disown()) {
      if (auto json = Serialize::JSON::parse(allocator(), String{*disown})) {
        return parse(*json);
      }
    }
  }
  return false;
}

bool Module::load(const StringView& _file_name) {
  if (auto file = Filesystem::UnbufferedFile::open(allocator(), _file_name, "r")) {
    return load(*file);
  }
  return false;
}

bool Module::parse(const Serialize::JSON& _description) {
  auto& allocator = *m_allocator;

  if (!_description) {
    const auto json_error{_description.error()};
    if (json_error) {
      return m_report.error("%s", *json_error);
    } else {
      return m_report.error("empty description");
    }
  }

  const auto& name = _description["name"];
  const auto& source = _description["source"];

  if (!name) {
    return m_report.error("missing 'name'");
  }

  if (!name.is_string()) {
    return m_report.error("expected String for 'name'");
  }

  auto name_string = name.as_string(allocator);
  if (!name_string) {
    return false;
  }

  m_name = Utility::move(*name_string);

  if (!m_report.rename(m_name)) {
    return false;
  }

  if (!source) {
    return m_report.error("missing 'source'");
  }

  if (!source.is_string()) {
    return m_report.error("expected String for 'source'");
  }

  auto source_string = source.as_string(allocator);
  if (!source_string) {
    return false;
  }

  // Trim any leading and trailing whitespace characters from the contents too.
  m_source = Utility::move(*source_string); //.strip("\t\r\n ");

  const auto& imports = _description["imports"];
  if (!imports) {
    return true;
  }

  if (!imports.is_array_of(Serialize::JSON::Type::STRING)) {
    return m_report.error("expected Array[String] for 'imports'");
  }

  return imports.each([&](const Serialize::JSON& _import) {
    auto dependency = _import.as_string(allocator);
    return dependency && m_dependencies.push_back(Utility::move(*dependency));
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

} // namespace Rx::Render::Frontend
