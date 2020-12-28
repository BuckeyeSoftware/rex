#include "rx/hud/console.h"

#include "rx/render/frontend/context.h"
#include "rx/render/frontend/target.h"

#include "rx/render/immediate2D.h"

#include "rx/console/context.h"

#include "rx/input/context.h"

#include "rx/core/algorithm/clamp.h"

namespace Rx::hud {

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
  Math::Vec4f(0.0f, 0.0f, 0.0f, 0.0f),
  Math::Vec4f(1.0f, 1.0f, 1.0f, 1.0f),
  Math::Vec4f(0.1176f, 0.1176f, 0.1176f, 1.0f));

RX_CONSOLE_V4FVAR(
  console_text_input_bakckgound_color,
  "console.text_input_background_color",
  "background color for console text input",
  Math::Vec4f(0.0f, 0.0f, 0.0f, 0.0f),
  Math::Vec4f(1.0f, 1.0f, 1.0f, 1.0f),
  Math::Vec4f(0x29 / 255.0f, 0x29 / 255.0f, 0x29 / 255.0f, 1.0f));

RX_CONSOLE_V4FVAR(
  console_selection_highlight_background_color,
  "console.selection_highlight_background_color",
  "console text selection highlight color",
  Math::Vec4f(0.0f, 0.0f, 0.0f, 0.0f),
  Math::Vec4f(1.0f, 1.0f, 1.0f, 1.0f),
  Math::Vec4f(0.6784f, 0.8392f, 1.0f, 1.0f - 0.1490f));

Console::Console(Render::Immediate2D* _immediate, Input::Context& input_)
  : m_immediate{_immediate}
  , m_input_context{input_}
  , m_input_layer{&m_input_context}
  , m_selection{0}
{
  m_input_layer.capture_text(&m_text);
}

void Console::update(Rx::Console::Context& console_) {
  const auto& dimensions = m_immediate->frontend()->swapchain()->dimensions();

  const Float32 padding = 10.0f;
  const Float32 font_size = static_cast<Float32>(*console_font_size);
  const Size console_lines = static_cast<Size>(*console_output_lines);

  Float32 height = 0.0f;
  height += console_lines * font_size + padding * 2.0f; // Printable area
  height += 1.0f;                                       // Single pixel line
  height += font_size * 2.0f;                           // Text box
  height += 1.0f;                                       // Singlle pixel line

  // Resize and move the input layer accordingly.
  m_input_layer.resize({static_cast<Float32>(dimensions.w), height});
  m_input_layer.move({0.0f, dimensions.h - height});

  // Make a copy of the console line output for rendering.
  m_lines = console_.lines();

  bool made_selection = false;
  if (m_input_layer.keyboard().is_pressed(Input::ScanCode::k_grave)) {
    // The grave character will be given during text events. Just erase it.
    m_text.erase();
    m_input_context.root_layer().raise();
  } else if (m_input_layer.keyboard().is_pressed(Input::ScanCode::k_down)) {
    m_selection++;
  } else if (m_input_layer.keyboard().is_pressed(Input::ScanCode::k_up)) {
    if (m_selection) {
      m_selection--;
    }
  } else if (m_input_layer.keyboard().is_pressed(Input::ScanCode::k_tab)) {
    made_selection = true;
  } else if (m_input_layer.keyboard().is_pressed(Input::ScanCode::k_return)) {
    console_.execute(m_text.contents());
    m_text.clear();
  }

  if (m_text.contents().is_empty()) {
    return;
  }

  auto complete_variables = console_.auto_complete_variables(m_text.contents());
  auto complete_commands = console_.auto_complete_commands(m_text.contents());

  m_suggestions = Utility::move(complete_variables);
  m_suggestions.append(complete_commands);

  if (m_suggestions.is_empty()) {
    m_selection = 0;
  } else {
    m_selection = Algorithm::clamp(m_selection, 0_z, m_suggestions.size() - 1);
    if (made_selection) {
      m_text.assign(m_suggestions[m_selection]);
    }
  }
}

void Console::render() {
  if (!m_input_layer.is_active()) {
    return;
  }

  const Render::Frontend::Context& frontend{*m_immediate->frontend()};
  const Math::Vec2f& resolution{frontend.swapchain()->dimensions().cast<Float32>()};

  Float32 padding{10.0f};
  Float32 font_size{static_cast<Float32>(*console_font_size)};
  Size suggestion_lines{static_cast<Size>(*console_suggestion_lines)};
  Size console_lines{static_cast<Size>(*console_output_lines)};

  Float32 base_y{0.0f};
  Float32 console_h{console_lines * font_size + padding * 2.0f};
  Float32 selection_h{suggestion_lines * font_size};

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

  // Scrolling of the text by offsetting it inside the box. Scissor will remove
  // the text outside.
  Float32 text_y{padding + font_size * 0.75f};
  if (m_lines.size() > console_lines) {
    text_y -= (m_lines.size() - console_lines) * font_size;
  }

  // Render every line for the console, lines outside will be scissored.
  m_lines.each_fwd([&](const String& _line) {
    m_immediate->frame_queue().record_text(
      *console_font_name,
      {padding, resolution.h - text_y},
      static_cast<Sint32>(font_size),
      1.0f,
      Render::Immediate2D::TextAlign::k_left,
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
  Float32 textbox_y{base_y};
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
    static_cast<Sint32>(font_size),
    1.0f,
    Render::Immediate2D::TextAlign::k_left,
    m_text.contents(),
    {1.0f, 1.0f, 1.0f, 1.0f});

  if (m_text.is_selected()) {
    const auto& selection{m_text.selection()};
    if (selection[0] != selection[1]) {
      // Measure the text up to selection[0] for start
      const Float32 skip{m_immediate->measure_text_length(
        *console_font_name,
        m_text.contents().data(),
        selection[0],
        static_cast<Sint32>(font_size),
        1.0f)};

      // Measure text from selection[0] to selection[1]
      const Float32 size{m_immediate->measure_text_length(
        *console_font_name,
        m_text.contents().data() + selection[0],
        selection[1] - selection[0],
        static_cast<Sint32>(font_size),
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
    const Float32 cursor{
      m_immediate->measure_text_length(
        *console_font_name,
        m_text.contents().data(),
        m_text.cursor(),
        static_cast<Sint32>(font_size),
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
  Float32 suggestion_y{base_y + font_size};
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
  m_suggestions.each_fwd([&](const String& _suggestion){
    m_immediate->frame_queue().record_text(
      *console_font_name,
      {padding, resolution.h - suggestion_y + font_size*0.15f},
      static_cast<Sint32>(font_size),
      1.0f,
      Render::Immediate2D::TextAlign::k_left,
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
