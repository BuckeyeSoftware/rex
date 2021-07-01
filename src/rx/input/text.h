#ifndef RX_INPUT_TEXT_H
#define RX_INPUT_TEXT_H
#include "rx/core/string.h"
#include "rx/math/vec2.h"

namespace Rx::Input {

struct Text {
  Text();

  static inline constexpr const auto CURSOR_VISIBLE_TIME = 0.50f;

  enum class Position : Uint8 {
    LEFT,
    RIGHT,
    HOME,
    END
  };

  void update(Float32 _delta_time);

  Optional<String> copy() const;
  Optional<String> cut();
  void paste(const StringView& _contents);
  void select(bool _select);
  void select_all();

  void move_cursor(Position _position);
  void erase();
  bool assign(const StringView& _contents);
  void clear();

  bool is_cursor_visible() const;
  bool is_selecting() const;
  bool is_selected() const;
  bool is_active() const;
  const String& contents() const;
  Size cursor() const;
  const Size (&selection() const)[2];

private:
  friend struct Layer;

  void reset_selection();
  void reset_cursor();

  enum : Uint8 {
    SELECTING      = 1 << 0,
    SELECTED       = 1 << 1,
    SELECT_LEFT    = 1 << 2,
    SELECT_RIGHT   = 1 << 3,
    CURSOR_VISIBLE = 1 << 4,
    ACTIVE         = 1 << 5
  };

  String m_contents;
  Size m_selection[2];
  Size m_cursor;
  Float32 m_cursor_visible_time;
  int m_flags;
};

inline bool Text::is_cursor_visible() const {
  return m_flags & CURSOR_VISIBLE;
}

inline bool Text::is_selecting() const {
  return m_flags & SELECTING;
}

inline bool Text::is_selected() const {
  return m_flags & SELECTED;
}

inline bool Text::is_active() const {
  return m_flags & ACTIVE;
}

inline const String& Text::contents() const {
  return m_contents;
}

inline Size Text::cursor() const {
  return m_cursor;
}

inline const Size (&Text::selection() const)[2] {
  return m_selection;
}

} // namespace Rx::Input

#endif // RX_INPUT_TEXT
