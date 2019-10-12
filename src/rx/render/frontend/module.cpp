#include "rx/core/filesystem/file.h"
#include "rx/core/json.h"

#include "rx/render/frontend/module.h"

namespace rx::render::frontend {

RX_LOG("render/module", logger);

bool module::load(const string& _file_name) {
  auto data{filesystem::read_binary_file(_file_name)};
  if (!data) {
    return false;
  }

  if (!parse({data->disown()})) {
    return false;
  }

  return true;
}

bool module::parse(const json& _description) {
  const json& name{_description["name"]};
  const json& source{_description["source"]};

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

  m_source = source.as_string();

  if (const json& imports{_description["imports"]}) {
    if (!imports.is_array_of(json::type::k_string)) {
      return error("expected Array[String] for 'imports'");
    }

    imports.each([&](const json& _import) {
      m_dependencies.push_back(_import.as_string());
    });
  }

  return true;
}

bool resolve_module_dependencies(
  const map<string, module>& _modules,
  const module& _current_module,
  set<string>& visited_,
  algorithm::topological_sort<string>& sorter_)
{
  sorter_.add(_current_module.name());

  // For each dependency of this module.
  return _current_module.dependencies().each_fwd([&](const string& _dependency) {
    // Add the dependency to the topological sorter.
    if (!sorter_.add(_current_module.name(), _dependency)) {
      return false;
    }

    // Break cycles in the visitation.
    if (visited_.find(_dependency)) {
      return true;
    }

    visited_.insert(_dependency);

    // Recursively apply dependencies.
    if (auto find{_modules.find(_dependency)}) {
      return resolve_module_dependencies(_modules, *find, visited_, sorter_);
    }

    return true;
  });
}

void module::write_log(log::level _level, string&& message_) const {
  if (m_name.is_empty()) {
    logger(_level, "%s", utility::move(message_));
  } else {
    logger(_level, "module '%s': %s", m_name, utility::move(message_));
  }
}

} // namespace rx::render::frontend
