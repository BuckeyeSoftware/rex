#include "rx/input/text.h"
#include "rx/core/hints/unreachable.h"
#include "rx/core/utility/swap.h"

namespace Rx::Input {

Text::Text(Memory::Allocator& _allocator)
  : m_allocator{_allocator}
  , m_contents{m_allocator}
  , m_cursor{0}
  , m_cursor_visible_time{CURSOR_VISIBLE_TIME}
  , m_flags{0}
{
  m_selection[0] = 0;
  m_selection[1] = 0;
}

void Text::update(Float32 _delta_time) {
  m_cursor_visible_time -= _delta_time;
  if (m_cursor_visible_time <= 0.0f) {
    m_cursor_visible_time = CURSOR_VISIBLE_TIME;
    m_flags ^= CURSOR_VISIBLE;
  }
}

Optional<String> Text::copy() const {
  if (m_flags & SELECTED) {
    return m_contents.substring(m_selection[0], m_selection[1] - m_selection[0]);
  }
  return String::copy(m_contents);
}

Optional<String> Text::cut() {
  Optional<String> contents;
  if (m_flags & SELECTED) {
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

void Text::paste(const StringView& _contents) {
  // Pasting when you have selected text replaces the selected text.
  if ((m_flags & SELECTED) && !m_contents.is_empty()) {
    // Remove the selected text and move the cursor to the beginning of
    // the selection for inserting the new text.
    m_contents.erase(m_selection[0], m_selection[1]);
    m_cursor = m_selection[0];
  }

  // Insert the new text at the cursor position and advance the cursor by the
  // length of the new text.
  if (m_contents.insert_at(m_cursor, _contents.data())) {
    m_cursor += _contents.size();
  }

  // When you paste any text, the selection is cancelled and the timer for
  // the blinking cursor is reset.
  reset_selection();
  reset_cursor();
}

void Text::erase() {
  if (m_flags & SELECTED) {
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
  if (_select && !(m_flags & SELECTING)) {
    // Start a new selection starting from the cursor only when a selection
    // hasn't already been started.
    m_flags |= SELECTING;
    m_selection[0] = m_cursor;
    m_selection[1] = m_cursor;
  } else if (!_select) {
    // No longer in a selecting state but there can still be selected text.
    m_flags &= ~SELECTING;
  }
}

void Text::select_all() {
  // Select all the text and indicate that we're selected. When you select
  // all text it doesn't mean you're in a selecting state however. So any
  // events that would modify a selection should start a new selection.
  m_selection[0] = 0;
  m_selection[1] = m_contents.size();
  m_flags |= SELECTED;
  m_flags &= ~(SELECTING | SELECT_LEFT | SELECT_RIGHT);

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
  if (m_flags & SELECTING) {
    m_flags |= SELECTED;
  } else {
    reset_selection();
  }

  switch (_position)
  {
  case Position::HOME:
    if (m_flags & SELECTING) {
      m_selection[0] = 0;
    }
    m_cursor = 0;
    break;
  case Position::END:
    if (m_flags & SELECTING) {
      m_selection[1] = m_contents.size();
    }
    m_cursor = m_contents.size();
    break;
  case Position::LEFT:
    if (m_cursor) {
      m_cursor--;
    }
    break;
  case Position::RIGHT:
    if (m_cursor < m_contents.size()) {
      m_cursor++;
    }
    break;
  }

  if (m_flags & SELECTING) {
    // The direction of the selection hasn't been recorded yet.
    if (!(m_flags & SELECT_LEFT) && !(m_flags & SELECT_RIGHT)) {
      // When going left and the start of the selection can go left further,
      // or when going right and the end of the selection can go right further,
      // record the direction of the selection and move the selection left or
      // right respectively.
      if (_position == Position::LEFT && m_selection[0]) {
        m_flags |= SELECT_LEFT;
        m_selection[0]--;
      } else if (_position == Position::RIGHT && m_selection[1] < m_contents.size()) {
        m_flags |= SELECT_RIGHT;
        m_selection[1]++;
      }
    } else {
      // Move the selection points in the appropriate directions based on their
      // starting directions and the current direction to move them in. The
      // selection coordinates given by |m_selection| are such that [0] is always
      // the start of the selection and [1] is always the end of the selection.
      //
      // This maintains the invariant |m_selection[0] <= m_selection[1]|.
      if (m_flags & SELECT_LEFT) {
        if (_position == Position::LEFT && m_selection[0]) {
          m_selection[0]--;
        } else if (_position == Position::RIGHT && m_selection[0] < m_contents.size()) {
          m_selection[0]++;
        }
      } else if (m_flags & SELECT_RIGHT) {
        if (_position == Position::LEFT && m_selection[1]) {
          m_selection[1]--;
        } else if (_position == Position::RIGHT && m_selection[1] < m_contents.size()) {
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

bool Text::assign(const StringView& _contents) {
  auto contents = _contents.to_string(m_allocator);
  if (!contents) {
    return false;
  }

  m_contents = Utility::move(*contents);
  m_cursor = m_contents.size();

  reset_selection();
  reset_cursor();

  return true;
}

void Text::clear() {
  m_contents.clear();
  m_cursor = 0;

  reset_selection();
  reset_cursor();
}

void Text::reset_selection() {
  m_flags &= ~(SELECTING | SELECTED | SELECT_LEFT | SELECT_RIGHT);
  m_selection[0] = m_cursor;
  m_selection[1] = m_cursor;
}

void Text::reset_cursor() {
  m_flags |= CURSOR_VISIBLE;
  m_cursor_visible_time = CURSOR_VISIBLE_TIME;
}

} // namespace Rx::Input
