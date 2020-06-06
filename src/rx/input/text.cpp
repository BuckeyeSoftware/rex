#include "rx/input/text.h"
#include "rx/core/hints/unreachable.h"
#include "rx/core/utility/swap.h"

namespace Rx::Input {

Text::Text()
  : m_cursor{0}
  , m_cursor_visible_time{k_cursor_visible_time}
  , m_flags{0}
{
  m_selection[0] = 0;
  m_selection[1] = 0;
}

void Text::update(Float32 _delta_time) {
  m_cursor_visible_time -= _delta_time;
  if (m_cursor_visible_time <= 0.0f) {
    m_cursor_visible_time = k_cursor_visible_time;
    m_flags ^= k_cursor_visible;
  }
}

String Text::copy() const {
  return m_flags & k_selected
    ? m_contents.substring(m_selection[0], m_selection[1] - m_selection[0])
    : m_contents;
}

String Text::cut() {
  String contents;

  if (m_flags & k_selected) {
    // Take the substring defined by the selection region and remove that
    // substring from the contents.
    contents = m_contents.substring(m_selection[0],
      m_selection[1] - m_selection[0]);
    m_contents.erase(m_selection[0], m_selection[1]);

    // Then move the cursor to the beginning of the selection.
    m_cursor = m_selection[0];
  } else {
    // Cut with no selection behaves as if the entire text is selected.
    contents = Utility::move(m_contents);
    m_cursor = 0;
  }

  // When you cut any text, the selection is cancelled and the timer for
  // the blinking cursor is reset.
  reset_selection();
  reset_cursor();

  return contents;
}

void Text::paste(const String& _contents) {
  // Pasting when you have selected text replaces the selected text.
  if ((m_flags & k_selected) && !m_contents.is_empty()) {
    // Remove the selected text and move the cursor to the beginning of
    // the selection for inserting the new text.
    m_contents.erase(m_selection[0], m_selection[1]);
    m_cursor = m_selection[0];
  }

  // Insert the new text at the cursor position and advance the cursor by the
  // length of the new text.
  m_contents.insert_at(m_cursor, _contents);
  m_cursor += _contents.size();

  // When you paste any text, the selection is cancelled and the timer for
  // the blinking cursor is reset.
  reset_selection();
  reset_cursor();
}

void Text::erase() {
  if (m_flags & k_selected) {
    // Remove the text defined by the selection and move the cursor to the
    // beginning of the selection.
    m_contents.erase(m_selection[0], m_selection[1]);
    m_cursor = m_selection[0];
  } else if (m_cursor) {
    // Remove a single character at the cursor and move the cursor back.
    m_contents.erase(m_cursor - 1, m_cursor);
    m_cursor--;
  }

  // When you erase any text, the selection is cancelled and the timer for
  // the blinking cursor is reset.
  reset_selection();
  reset_cursor();
}

void Text::select(bool _select) {
  if (_select && !(m_flags & k_selecting)) {
    // Start a new selection starting from the cursor only when a selection
    // hasn't already been started.
    m_flags |= k_selecting;
    m_selection[0] = m_cursor;
    m_selection[1] = m_cursor;
  } else if (!_select) {
    // No longer in a selecting state but there can still be selected text.
    m_flags &= ~k_selecting;
  }
}

void Text::select_all() {
  // Select all the text and indicate that we're selected. When you select
  // all text it doesn't mean you're in a selecting state however. So any
  // events that would modify a selection should start a new selection.
  m_selection[0] = 0;
  m_selection[1] = m_contents.size();
  m_flags |= k_selected;
  m_flags &= ~(k_selecting | k_select_left | k_select_right);

  // When you select everything, the cursor is moved to the end of the
  // selection. You can't tell though because the cursor isn't typically
  // visible when all the contents are selected.
  m_cursor = m_contents.size();

  reset_cursor();
}

void Text::move_cursor(Position _position) {
  if (m_contents.is_empty()) {
    return;
  }

  // When selecting, any cursor movement should form a selection.
  // When not selecting, any cursor movement should reset any selection.
  if (m_flags & k_selecting) {
    m_flags |= k_selected;
  } else {
    reset_selection();
  }

  switch (_position)
  {
  case Position::k_home:
    if (m_flags & k_selecting) {
      m_selection[0] = 0;
    }
    m_cursor = 0;
    break;
  case Position::k_end:
    if (m_flags & k_selecting) {
      m_selection[1] = m_contents.size();
    }
    m_cursor = m_contents.size();
    break;
  case Position::k_left:
    if (m_cursor) {
      m_cursor--;
    }
    break;
  case Position::k_right:
    if (m_cursor < m_contents.size()) {
      m_cursor++;
    }
    break;
  }

  if (m_flags & k_selecting) {
    // The direction of the selection hasn't been recorded yet.
    if (!(m_flags & k_select_left) && !(m_flags & k_select_right)) {
      // When going left and the start of the selection can go left further,
      // or when going right and the end of the selection can go right further,
      // record the direction of the selection and move the selection left or
      // right respectively.
      if (_position == Position::k_left && m_selection[0]) {
        m_flags |= k_select_left;
        m_selection[0]--;
      } else if (_position == Position::k_right && m_selection[1] < m_contents.size()) {
        m_flags |= k_select_right;
        m_selection[1]++;
      }
    } else {
      // Move the selection points in the appropriate directions based on their
      // starting directions and the current direction to move them in. The
      // selection coordinates given by |m_selection| are such that [0] is always
      // the start of the selection and [1] is always the end of the selection.
      //
      // This maintains the invariant |m_selection[0] <= m_selection[1]|.
      if (m_flags & k_select_left) {
        if (_position == Position::k_left && m_selection[0]) {
          m_selection[0]--;
        } else if (_position == Position::k_right && m_selection[0] < m_contents.size()) {
          m_selection[0]++;
        }
      } else if (m_flags & k_select_right) {
        if (_position == Position::k_left && m_selection[1]) {
          m_selection[1]--;
        } else if (_position == Position::k_right && m_selection[1] < m_contents.size()) {
          m_selection[1]++;
        }
      }

      // When the selection meets itself, i.e a zero-character selection is
      // formed, reset the selection.
      if (m_selection[0] == m_selection[1]) {
        reset_selection();
      }
    }
  }

  reset_cursor();
}

void Text::assign(const String& _contents) {
  m_contents = _contents;
  m_cursor = m_contents.size();

  reset_selection();
  reset_cursor();
}

void Text::clear() {
  m_contents.clear();
  m_cursor = 0;

  reset_selection();
  reset_cursor();
}

void Text::reset_selection() {
  m_flags &= ~(k_selecting | k_selected | k_select_left | k_select_right);
  m_selection[0] = m_cursor;
  m_selection[1] = m_cursor;
}

void Text::reset_cursor() {
  m_flags |= k_cursor_visible;
  m_cursor_visible_time = k_cursor_visible_time;
}

} // namespace rx::input
