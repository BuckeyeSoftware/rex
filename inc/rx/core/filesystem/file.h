#ifndef RX_CORE_FILESYSTEM_FILE_H
#define RX_CORE_FILESYSTEM_FILE_H

#include <rx/core/concepts/no_copy.h> // no_copy
#include <rx/core/string.h> // string
#include <rx/core/optional.h> // optional

namespace rx::filesystem {

struct file : concepts::no_copy {
  file(const char* file_name, const char* mode);
  file(const string& _file_name, const char* _mode);
  file(file&& other);
  ~file();

  // read |size| bytes from file into |data|
  rx_u64 read(rx_byte* data, rx_u64 size);

  // write |size| bytes from |data| into file
  rx_u64 write(const rx_byte* data, rx_u64 size);

  // print |fmt| with |args| to file using |alloc| for formatting
  template<typename... Ts>
  bool print(memory::allocator* alloc, const char* fmt, Ts&&... args);

  // print |fmt| with |args| to file
  template<typename... Ts>
  bool print(const char* fmt, Ts&&... args);

  // seek to |where| in file
  bool seek(rx_u64 where);

  // get size of file, returns nullopt if operation not supported
  optional<rx_u64> size();

  // flush to disk
  bool flush();

  bool read_line(string& line);

  bool is_valid() const;

  operator bool() const;

private:
  bool print(string&& contents);

  void* m_impl;
  const char* m_file_name;
  const char* m_mode;
};

inline file::file(const string& _file_name, const char* _mode)
  : file{_file_name.data(), _mode}
{
}

inline file::operator bool() const {
  return is_valid();
}

template<typename... Ts>
inline bool file::print(memory::allocator* _allocator, const char* _format, Ts&&... _arguments) {
  return print(string::format(_allocator, _format, utility::forward<Ts>(_arguments)...));
}

template<typename... Ts>
inline bool file::print(const char* _format, Ts&&... _arguments) {
  return print(&memory::g_system_allocator, _format, utility::forward<Ts>(_arguments)...);
}

optional<array<rx_byte>> read_binary_file(memory::allocator* _allocator, const string& _file_name);

inline optional<array<rx_byte>> read_binary_file(const string& _file_name) {
  return read_binary_file(&memory::g_system_allocator, _file_name);
}

} // namespace rx::filesystem

#endif // RX_CORE_FILESYSTEM_FILE_H
