#ifndef RX_INPUT_TEXT_H
#define RX_INPUT_TEXT_H
#include "rx/core/string.h"
#include "rx/math/vec2.h"

namespace Rx::Input {

struct Text {
  Text();

  static constexpr const Float32 k_cursor_visible_time = 0.50f;

  enum class Position {
    k_left,
    k_right,
    k_home,
    k_end
  };

  void update(Float32 _delta_time);

  String copy() const;
  String cut();
  void paste(const String& _contents);
  void select(bool _select);
  void select_all();

  void move_cursor(Position _position);
  void erase();
  void assign(const String& _contents);
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

  enum {
    k_selecting      = 1 << 0,
    k_selected       = 1 << 1,
    k_select_left    = 1 << 2,
    k_select_right   = 1 << 3,
    k_cursor_visible = 1 << 4,
    k_active         = 1 << 5
  };

  String m_contents;
  Size m_selection[2];
  Size m_cursor;
  Float32 m_cursor_visible_time;
  int m_flags;
};

inline bool Text::is_cursor_visible() const {
  return m_flags & k_cursor_visible;
}

inline bool Text::is_selecting() const {
  return m_flags & k_selecting;
}

inline bool Text::is_selected() const {
  return m_flags & k_selected;
}

inline bool Text::is_active() const {
  return m_flags & k_active;
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

} // namespace rx::input

#endif // RX_INPUT_TEXT
