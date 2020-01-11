#ifndef RX_INPUT_TEXT_H
#define RX_INPUT_TEXT_H
#include "rx/core/string.h"
#include "rx/math/vec2.h"

namespace rx::input {

struct input;

struct text {
  text();

  static constexpr const rx_f32 k_cursor_visible_time{0.50f};

  enum class position {
    k_left,
    k_right,
    k_home,
    k_end
  };

  void update(rx_f32 _delta_time);

  string copy() const;
  string cut();
  void paste(const string& _contents);
  void select(bool _select);
  void select_all();

  void move_cursor(position _position);
  void erase();
  void assign(const string& _contents);
  void clear();

  bool is_cursor_visible() const;
  bool is_selecting() const;
  bool is_selected() const;
  bool is_active() const;
  const string& contents() const;
  rx_size cursor() const;
  const rx_size (&selection() const)[2];

private:
  friend struct input;

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

  string m_contents;
  rx_size m_selection[2];
  rx_size m_cursor;
  rx_f32 m_cursor_visible_time;
  int m_flags;
};

inline bool text::is_cursor_visible() const {
  return m_flags & k_cursor_visible;
}

inline bool text::is_selecting() const {
  return m_flags & k_selecting;
}

inline bool text::is_selected() const {
  return m_flags & k_selected;
}

inline bool text::is_active() const {
  return m_flags & k_active;
}

inline const string& text::contents() const {
  return m_contents;
}

inline rx_size text::cursor() const {
  return m_cursor;
}

inline const rx_size (&text::selection() const)[2] {
  return m_selection;
}

} // namespace rx::input

#endif // RX_INPUT_TEXT
