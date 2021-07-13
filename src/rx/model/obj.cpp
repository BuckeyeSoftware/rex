#include <stdlib.h> // strtof
#include <string.h> // strncmp

#include "rx/model/obj.h"

#include "rx/core/map.h"
#include "rx/core/hash/combine.h"
#include "rx/core/stream/context.h"
#include "rx/core/memory/temporary_allocator.h"

namespace Rx::Model {

struct Key {
  Array<Sint32[3]> attributes = { -1, -1, -1 };

  Size hash() const {
    const auto h0 = Hash::mix_sint32(attributes[0]);
    const auto h1 = Hash::mix_sint32(attributes[1]);
    const auto h2 = Hash::mix_sint32(attributes[2]);
    return Hash::combine(h0, Hash::combine(h1, h2));
  }

  bool operator==(const Key& _other) const {
    return attributes == _other.attributes;
  }
};

static bool is_space(int _ch) {
  return _ch == ' ' || _ch == '\t' || _ch == '\r';
}

static bool is_alpha(int _ch) {
  return (_ch >= 'a' && _ch <= 'z') || (_ch >= 'A' && _ch <= 'Z');
}

static bool skip_when(char*& string_, auto&& _filter) {
  while (*string_ && _filter(*string_)) {
    string_++;
  }
  return *string_ != '\0';
}

// Read up to a three value attribute into |out_| from |_string|.
static bool read_attribute(char* _string, Vector<Math::Vec3f>& out_) {
  if (!out_.emplace_back(0.0f, 0.0f, 0.0f)) {
    return false;
  }

  auto& vec = out_[out_.size() - 1];

  skip_when(_string, is_alpha);

  for (Size i = 0; i < 3; i++) {
    vec[i] = strtof(_string, &_string);
    if (!skip_when(_string, is_space)) {
      break;
    }
  }

  return true;
};

// Read a string literal trimming unnecessary characters.
static bool read_string(char* _string, String& out_) {
  skip_when(_string, is_alpha);
  skip_when(_string, is_space);
  auto len = strlen(_string);
  for (; len && is_space(_string[len - 1]); len--)
    ;
  out_.clear();
  return out_.append(_string, len);
};

bool OBJ::read(Stream::Context& _stream) {
  auto& allocator = Memory::SystemAllocator::instance();

  // Read the entire contents into memory.
  auto contents = _stream.read_text(allocator);
  if (!contents) {
    return m_report.error("out of memory");
  }

  // NOTE(dweiler): These need the system allocator since they're moved from.
  String mesh_name{allocator};
  String material_name{allocator};

  // 1MiB of scratch memory for temporaries used during loading below.
  Memory::TemporaryAllocator<1_MiB> scratch{allocator};

  Map<Key, Uint32> hash{scratch};
  Vector<Math::Vec3f> attributes[3]{scratch, scratch, scratch};

  // Process the content line-by-line.
  auto data = reinterpret_cast<char*>(contents->data());
  bool result = true;
  bool mesh = false;
  while (auto line = String::read_line(data)) {
    skip_when(line, is_space);
    switch (*line) {
    case '#':
      // Skip comment lines.
      continue;
    case 'v':
      switch (line[1]) {
      case ' ':
        result &= read_attribute(line, attributes[0]);
        break;
      case 't':
        result &= read_attribute(line, attributes[1]);
        break;
      case 'n':
        result &= read_attribute(line, attributes[2]);
        break;
      }
      break;
    case 'u':
      // Skip unless the line is "usemtl".
      if (strncmp(line, "usemtl", 6)) {
        continue;
      }
      result &= read_string(line, material_name);
      mesh = false;
      break;
    case 'g':
      result &= read_string(line, mesh_name);
      mesh = false;
      break;
    case 'f':
      if (!mesh) {
        result &= m_meshes.emplace_back(
          Utility::move(mesh_name),
          Utility::move(material_name),
          m_elements.size(),
          0_z,
          Vector<Vector<Math::AABB>>{});
        mesh = true;
        hash.clear();
      }
      skip_when(line, is_alpha);
      for (Uint32 indices[3], count = 0;;) {
        if (!skip_when(line, is_space)) {
          break;
        }
        Key key;
        for (Size i = 0; i < 3; i++) {
          const auto value = strtol(line, &line, 10);
          const auto index = value < 0 ? attributes[i].size() + value : value - 1;
          key.attributes[i] = attributes[i].in_range(index) ? index : -1;
          if (*line != '/') {
            break;
          }
          line++;
        }
        auto index = hash.find(key);
        if (!index) {
          index = hash.insert(key, m_positions.size());
          if (!index) {
            result = false;
            break;
          }
          if (key.attributes[0] < 0) {
            m_positions.emplace_back(0.0f, 0.0f, 0.0f);
          } else {
            m_positions.push_back(attributes[0][key.attributes[0]]);
          }
          if (key.attributes[1] < 0) {
            m_coordinates.emplace_back(0.0f, 0.0f);
          } else {
            const auto& coordinate = attributes[1][key.attributes[1]];
            m_coordinates.emplace_back(coordinate.x, coordinate.y);
          }
          // Special case for normals if present.
          if (key.attributes[2] >= 0) {
            m_normals.push_back(attributes[2][key.attributes[2]]);
          }
        }
        // Implicitly triangulates the input even for malformed OBJ.
        if (count < 2) {
          indices[count++] = *index;
        } else {
          result &= m_elements.push_back(indices[0]);
          result &= m_elements.push_back(indices[1]);
          result &= m_elements.push_back(*index);
          indices[1] = *index;
        }
      }
      break;
    }

    if (!result) {
      return m_report.error("out of memory");
    }
  }

  const auto n_meshes = m_meshes.size();
  const auto n_elements = m_elements.size();

  // TODO(dweiler): Consider moving this to the importer?
  if (n_meshes && n_elements) {
    // Calculate the counts for each mesh.
    for (Size i = 1; i < n_meshes; i++) {
      m_meshes[i - 1].count = m_meshes[i].offset - m_meshes[i - 1].offset;
    }

    // Last one is a special case.
    m_meshes[n_meshes - 1].count = n_elements - m_meshes[n_meshes - 1].offset;
  }

  return true;
}

} // namespace Rx::Model