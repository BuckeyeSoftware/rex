#include "rx/hud/console.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

#include "rx/console/interface.h"

#include "rx/input/input.h"

#include "rx/core/algorithm/clamp.h"

namespace rx::hud {

RX_CONSOLE_SVAR(
  console_font_name,
  "console.font_name",
  "name of the font to use for the console (should be a monospaced font)",
  "Inconsolata-Regular");

RX_CONSOLE_IVAR(
  console_font_size,
  "console.font_size",
  "size of the font to use for the console",
  8,
  56,
  30);

RX_CONSOLE_IVAR(
  console_output_lines,
  "console.output_lines",
  "number of lines of visible output",
  5,
  50,
  10);

RX_CONSOLE_IVAR(
  console_suggestion_lines,
  "console.suggestion_lines",
  "number of lines of visible suggestions",
  5,
  20,
  8);

RX_CONSOLE_V4FVAR(
  console_background_color,
  "console.background_color",
  "background color of console",
  math::vec4f(0.0f, 0.0f, 0.0f, 0.0f),
  math::vec4f(1.0f, 1.0f, 1.0f, 1.0f),
  math::vec4f(0.1176f, 0.1176f, 0.1176f, 1.0f));

RX_CONSOLE_V4FVAR(
  console_text_input_bakckgound_color,
  "console.text_input_background_color",
  "background color for console text input",
  math::vec4f(0.0f, 0.0f, 0.0f, 0.0f),
  math::vec4f(1.0f, 1.0f, 1.0f, 1.0f),
  math::vec4f(0x29/255.0f, 0x29/255.0f, 0x29/255.0f, 1.0f));

RX_CONSOLE_V4FVAR(
  console_selection_highlight_background_color,
  "console.selection_highlight_background_color",
  "console text selection highlight color",
  math::vec4f(0.0f, 0.0f, 0.0f, 0.0f),
  math::vec4f(1.0f, 1.0f, 1.0f, 1.0f),
  math::vec4f(0.6784f, 0.8392f, 1.0f, 1.0f - 0.1490f));

console::console(render::immediate2D* _immediate)
  : m_immediate{_immediate}
{
}

void console::update(input::input& _input) {
  bool made_selection = false;
  if (_input.keyboard().is_pressed(input::scan_code::k_grave)) {
    _input.capture_text(_input.active_text() == &m_text ? nullptr : &m_text);
    _input.capture_mouse(_input.active_text() ? false : true);
  } else if (_input.keyboard().is_pressed(input::scan_code::k_down)) {
    m_selection++;
  } else if (_input.keyboard().is_pressed(input::scan_code::k_up)) {
    if (m_selection) {
      m_selection--;
    }
  } else if (_input.keyboard().is_pressed(input::scan_code::k_tab)) {
    made_selection = true;
  } else if (_input.keyboard().is_pressed(input::scan_code::k_return)) {
    rx::console::interface::execute(m_text.contents());
    m_text.clear();
  }

  m_suggestions  = rx::console::interface::auto_complete_variables(m_text.contents());
  m_suggestions += rx::console::interface::auto_complete_commands(m_text.contents());

  if (m_suggestions.is_empty()) {
    m_selection = 0;
  } else {
    m_selection = algorithm::clamp(m_selection, 0_z, m_suggestions.size() - 1);
    if (made_selection) {
      m_text.assign(m_suggestions[m_selection]);
    }
  }
}

void console::render() {
  if (!m_text.is_active()) {
    return;
  }

  const render::frontend::context& frontend{*m_immediate->frontend()};
  const math::vec2f& resolution{frontend.swapchain()->dimensions().cast<rx_f32>()};

  rx_f32 padding{10.0f};
  rx_f32 font_size{static_cast<rx_f32>(*console_font_size)};
  rx_size suggestion_lines{static_cast<rx_size>(*console_suggestion_lines)};
  rx_size console_lines{static_cast<rx_size>(*console_output_lines)};

  rx_f32 base_y{0.0f};
  rx_f32 console_h{console_lines*font_size+padding*2.0f};
  rx_f32 selection_h{suggestion_lines*font_size};

  // Draw rectangle across the top of the screen.
  m_immediate->frame_queue().record_rectangle(
    {0.0f,         resolution.h - console_h},
    {resolution.w, console_h},
    0,
    *console_background_color);
  base_y += console_h;

  // Scissor inside the rectangle, such that anything that goes outside it
  // is not rendered.
  m_immediate->frame_queue().record_scissor(
    {0.0f,         resolution.h - console_h},
    {resolution.w, console_h}
  );

  const vector<string>& lines{rx::console::interface::lines()};

  // Scrolling of the text by offsetting it inside the box. Scissor will remove
  // the text outside.
  rx_f32 text_y{padding+font_size*0.75f};
  if (lines.size() > console_lines) {
    text_y -= (lines.size() - console_lines) * font_size;
  }

  // Render every line for the console, lines outside will be scissored.
  lines.each_fwd([&](const string& _line) {
    m_immediate->frame_queue().record_text(
      *console_font_name,
      {padding, resolution.h - text_y},
      static_cast<rx_s32>(font_size),
      1.0f,
      render::immediate2D::text_align::k_left,
      _line,
      {1.0f, 1.0f, 1.0f, 1.0f});
    text_y += font_size;
  });

  // Disable scissoring.
  m_immediate->frame_queue().record_scissor(
    {-1.0f, -1.0f},
    {-1.0f, -1.0f});

  // Draw a solid 1px white line below the console.
  m_immediate->frame_queue().record_line(
    {0.0f,         resolution.h - base_y},
    {resolution.w, resolution.h - base_y},
    0.0f,
    1.0f,
    {1.0f, 1.0f, 1.0f, 1.0f});
  base_y += 1.0f;

  // Draw a box below the 1px line for text input, exactly 2x as large as the
  // font height, text will be centered inside.
  rx_f32 textbox_y{base_y};
  m_immediate->frame_queue().record_rectangle(
    {0.0f,         resolution.h - base_y - font_size * 2.0f},
    {resolution.w, font_size * 2.0f},
    0.0f,
    *console_text_input_bakckgound_color);
  base_y += font_size * 2.0f;

  // Draw a 1px white line line below that box.
  m_immediate->frame_queue().record_line(
    {0.0f,         resolution.h - base_y},
    {resolution.w, resolution.h - base_y},
    0.0f,
    1.0f,
    {1.0f, 1.0f, 1.0f, 1.0f});
  base_y += 1.0f;

  // Center the text inside the text box.
  textbox_y += font_size * 0.5f;

  // Render the current input text inside the box, centered.
  m_immediate->frame_queue().record_text(
    *console_font_name,
    {padding, resolution.h - textbox_y - font_size * 0.75f},
    static_cast<rx_s32>(font_size),
    1.0f,
    render::immediate2D::text_align::k_left,
    m_text.contents(),
    {1.0f, 1.0f, 1.0f, 1.0f});

  if (m_text.is_selected()) {
    const auto& selection{m_text.selection()};
    if (selection[0] != selection[1]) {
      // Measure the text up to selection[0] for start
      const rx_f32 skip{m_immediate->measure_text_length(
        *console_font_name,
        m_text.contents().data(),
        selection[0],
        font_size,
        1.0f)};

      // Measure text from selection[0] to selection[1]
      const rx_f32 size{m_immediate->measure_text_length(
        *console_font_name,
        m_text.contents().data() + selection[0],
        selection[1] - selection[0],
        font_size,
        1.0f)};

      m_immediate->frame_queue().record_rectangle(
        {padding + skip, resolution.h - textbox_y - font_size},
        {size, font_size},
        0.0f,
        *console_selection_highlight_background_color);
    }
  }

  // Draw vertical line for the blinking cursor.
  if (m_text.is_cursor_visible()) {
    // Determine where in the text the cursor begins by measuring the length of
    // the text up to the cursor position.
    const rx_f32 cursor{
      m_immediate->measure_text_length(
        *console_font_name,
        m_text.contents().data(),
        m_text.cursor(),
        font_size,
        1.0f)};

    // Render 1px wide, vertical white line that represents the cursor.
    m_immediate->frame_queue().record_line(
      {padding + cursor, resolution.h - textbox_y},
      {padding + cursor, resolution.h - textbox_y - font_size},
      0.0f,
      1.0f,
      {1.0f, 1.0f, 1.0f, 1.0f});
  }

  if (m_suggestions.is_empty() || m_text.contents().is_empty()) {
    return;
  }

  // Draw a box below everything else for suggestions.
  m_immediate->frame_queue().record_rectangle(
    {0.0f,                 resolution.h - base_y - selection_h},
    {resolution.w * 0.50f, selection_h},
    0.0f,
    *console_background_color);

  // Scissor inside the suggestions box so anything outside does not render.
  m_immediate->frame_queue().record_scissor(
    {0.0f,                 resolution.h - base_y - selection_h},
    {resolution.w * 0.50f, selection_h});

  // Scrolling of the text by offsetting it inside the box. Scissor will remove
  // the text outside.
  rx_f32 suggestion_y{base_y + font_size};
  if (m_selection >= suggestion_lines - 1) {
    suggestion_y -= ((m_selection + 1) - suggestion_lines) * font_size;
  }

  // Render a bar indicating which item is selected based on the selection index.
  m_immediate->frame_queue().record_rectangle(
    {0.0f,                 resolution.h - suggestion_y - font_size * m_selection},
    {resolution.w * 0.50f, font_size},
    0.0f,
    *console_selection_highlight_background_color);

  // Draw each suggestion now inside that box.
  m_suggestions.each_fwd([&](const string& _suggestion){
    m_immediate->frame_queue().record_text(
      *console_font_name,
      {padding, resolution.h - suggestion_y + font_size*0.15f},
      font_size,
      1.0f,
      render::immediate2D::text_align::k_left,
      _suggestion,
      {1.0f, 1.0f, 1.0f, 1.0f});
    suggestion_y += font_size;
  });

  // Disable scissoring.
  m_immediate->frame_queue().record_scissor(
    {-1.0f, -1.0f},
    {-1.0f, -1.0f});
}

} // namespace rx::hud
