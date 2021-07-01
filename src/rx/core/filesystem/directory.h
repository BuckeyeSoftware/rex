#ifndef RX_CORE_FILESYSTEM_DIRECTORY_H
#define RX_CORE_FILESYSTEM_DIRECTORY_H
#include "rx/core/string.h"
#include "rx/core/function.h"
#include "rx/core/optional.h"

/// \file directory.h

namespace Rx::Filesystem {

/// \brief Represents a directory.
struct RX_API Directory {
  RX_MARK_NO_COPY(Directory);

  /// Default construct a directory.
  constexpr Directory();

  /// \brief Move constructor.
  ///
  /// Constructs the directory object to represent the directory that was
  /// represented by \p directory_. After this call, \p directory_ no longer
  /// represents a valid directory object (calls to is_valid() yield \c false .)
  ///
  /// \param directory_ Another directory object to construct this directory
  /// object with.
  Directory(Directory&& directory_);

  /// \brief Closes the directory.
  ~Directory();

  /// \brief Moves the directory object.
  ///
  /// If \c *this still has valid directory, closes it. Then, or otherwise,
  /// assigns the state of \p directory_ to \c *this and sets \p directory_
  /// to a default constructed state.
  ///
  /// \param directory_ Another directory object to construct this directory
  /// object with.
  Directory& operator=(Directory&& directory_);

  /// \brief Open a directory.
  ///
  /// \param _allocator The allocator to use for directory operations.
  /// \param _path The path to the directory to open.
  /// \returns On success, the Directory. Otherwise, \c nullopt.
  static Optional<Directory> open(Memory::Allocator& _allocator, const StringView& _path);

  /// @{
  /// Check if the directory is valid.
  bool is_valid() const;
  explicit operator bool() const;
  /// @}

  /// \brief Enumerate directory items.
  ///
  /// Enumerates the directory calling an invocable for each item. The invocable
  /// should have one of the following signatures:
  /// \code{.cpp}
  /// void each(Item&& item_);
  /// // Or
  /// bool each(Item&& item_);
  /// \endcode
  ///
  /// Where the \c bool specialication version should return \c true for
  /// continued enumeration or \c false to stop enumeration.
  ///
  /// \param function_ The invocable.
  /// \returns When the whole directory is enumerated (the invocable returns
  /// \c void, or returns \c true always), this function returns \c true.
  /// Otherwise, returns \c false.
  template<typename F>
  bool each(F&& function_);

  /// \brief An item in a directory.
  ///
  /// Directory items may be files or other directories.
  ///
  /// \warning The Item has the same lifetime as the Directory. It's not valid
  /// to refer to an Item after the Directory has gone out of scope.
  struct Item {
    RX_MARK_NO_COPY(Item);
    RX_MARK_NO_MOVE(Item);

    /// Test if an item is a file.
    bool is_file() const;
    /// Test if an item is a directory.
    bool is_directory() const;

    /// Get the name of the item.
    /// \note The name does not include the full path name.
    const String& name() const;

    /// Reference to the directory \c *this is associated with.
    const Directory& directory() const &;

    /// \brief Attempt to open the item as a Directory itself.
    /// \return On success, the Directory. Otherwise, \c nullopt.
    /// \note This can fail if the Item is not a directory or was just deleted
    /// between the time of recieving this item from enumerate() and the call to
    /// as_directory().
    Optional<Directory> as_directory() const;

    /// Get full name of the item.
    Optional<String> full_name() const;

  private:
    friend struct Directory;

    enum class Type : Uint8 {
      FILE,
      DIRECTORY
    };

    Item(const Directory* _directory, String&& name_, Type _type);

    const Directory* m_directory;
    String m_name;
    Type m_type;
  };

  /// Recieve the path passed to open().
  const String& path() const &;

  /// Recieve the allocator passed to open().
  constexpr Memory::Allocator& allocator() const;

private:
  Directory(Memory::Allocator& _allocator, String&& path_, void* _impl)
    : m_allocator{&_allocator}
    , m_path{Utility::move(path_)}
    , m_impl{_impl}
  {
  }

  void release();

  using Enumerator = Function<bool(Item&&)>;

  // enumerate directory with |_function| being called for each item
  // NOTE: does not consider hidden files, symbolic links, block devices, or ..
  bool enumerate(Enumerator&& _function);

  Memory::Allocator* m_allocator;
  String m_path;
  void* m_impl;
};

inline constexpr Directory::Directory()
  : m_allocator{nullptr}
  , m_impl{nullptr}
{
}

inline Directory::Directory(Directory&& directory_)
  : m_allocator{Utility::exchange(directory_.m_allocator, &Memory::NullAllocator::instance())}
  , m_path{Utility::move(directory_.m_path)}
  , m_impl{Utility::exchange(directory_.m_impl, nullptr)}
{
}

inline Directory::~Directory() {
  release();
}

inline Directory& Directory::operator=(Directory&& directory_) {
  if (this != &directory_) {
    release();
    m_allocator = Utility::exchange(directory_.m_allocator, nullptr);
    m_path = Utility::move(directory_.m_path);
    m_impl = Utility::exchange(directory_.m_impl, nullptr);
  }
  return *this;
}

RX_HINT_FORCE_INLINE bool Directory::is_valid() const {
  return m_impl != nullptr;
}

RX_HINT_FORCE_INLINE Directory::operator bool() const {
  return is_valid();
}

template<typename F>
bool Directory::each(F&& function_) {
  // When |function_| doesn't return boolean result.
  if constexpr (!Traits::IS_SAME<Traits::InvokeResult<F, Item&&>, bool>) {
    // Wrap the function dispatch since |enumerate| expects something that
    // returns boolean to indicate continued enumeration.
    auto wrap = [function = Utility::move(function_)](Item&& item_) {
      function(Utility::move(item_));
      return true;
    };
    if (auto fun = Enumerator::create(Utility::move(wrap))) {
      return enumerate(Utility::move(*fun));
    }
  } else if (auto fun = Enumerator::create(Utility::move(function_))) {
    return enumerate(Utility::move(*fun));
  }
  return false;
}

RX_HINT_FORCE_INLINE const String& Directory::path() const & {
  return m_path;
}

RX_HINT_FORCE_INLINE constexpr Memory::Allocator& Directory::allocator() const {
  return *m_allocator;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_file() const {
  return m_type == Type::FILE;
}

RX_HINT_FORCE_INLINE bool Directory::Item::is_directory() const {
  return m_type == Type::DIRECTORY;
}

RX_HINT_FORCE_INLINE const Directory& Directory::Item::directory() const & {
  return *m_directory;
}

RX_HINT_FORCE_INLINE const String& Directory::Item::name() const {
  return m_name;
}

RX_HINT_FORCE_INLINE Directory::Item::Item(const Directory* _directory, String&& name_, Type _type)
  : m_directory{_directory}
  , m_name{Utility::move(name_)}
  , m_type{_type}
{
}

bool create_directory(const StringView& _path);

} // namespace Rx::Filesystem

#endif // RX_CORE_FILESYSTEM_DIRECTORY_H
