#include <SDL_video.h>

#include "display.h"

#include "rx/core/map.h"

namespace rx {

vector<display> display::displays(memory::allocator& _allocator) {
  vector<display> displays{_allocator};

  for (int i{0}, n{SDL_GetNumVideoDisplays()}; i < n; i++) {
    display result{_allocator};
    if (const char* name{SDL_GetDisplayName(i)}) {
      // Copy the contents of the name for manipulation.
      const rx_size name_size{strlen(name) + 1};
      rx_byte* name_data{_allocator.allocate(name_size)};
      RX_ASSERT(name_data, "out of memory");
      memcpy(name_data, name, name_size);

      // SDL2 introduces the physical size of the display in inches as a suffix
      // to the display name. Strip this information from the name, as it's
      // rarely correct due to monitors having incorrect EDID information.
      auto is_numeric{[](int ch) {
        return (ch >= '0' && ch <= '9') || ch == '.';
      }};

      // When " is found at the end of the string.
      if (char* x = strrchr(reinterpret_cast<char*>(name_data), '"'); x && x[1] == '\0') {
        // Write zeros from the end until it's no longer numeric looking.
        char *y{x - 1};
        while (is_numeric(*y)) {
          *y-- = '\0';
        }

        // Remove trailing whitespace.
        while (*y == ' ') {
          *y-- = '\0';
        }
      }

      // The string now owns the memory, avoids a copy.
      result.m_name = memory::view{&_allocator, name_data, name_size};
    } else {
      result.m_name = string::format("Unknown (%d)", i);
    }

    rx_f32 dpi[3];
    if (SDL_GetDisplayDPI(i, dpi + 0, dpi + 1, dpi + 2) == 0) {
      result.m_diagonal_dpi = dpi[0];
      result.m_horizontal_dpi = dpi[1];
      result.m_vertical_dpi = dpi[2];
    }

    SDL_Rect bounds;
    if (SDL_GetDisplayBounds(i, &bounds) == 0) {
      result.m_bounds.offset = {bounds.x, bounds.y};
      result.m_bounds.dimensions = {bounds.w, bounds.h};
    }

    const int n_modes{SDL_GetNumDisplayModes(i)};
    for (int j{0}; j < n_modes; j++) {
      SDL_DisplayMode mode;
      if (SDL_GetDisplayMode(i, j, &mode) == 0) {
        const math::vec2i resolution{mode.w, mode.h};
        result.m_modes.emplace_back(resolution.cast<rx_size>(),
          static_cast<rx_f32>(mode.refresh_rate));
      }
    }
    displays.push_back(utility::move(result));
  }

  // Differentiate between displays with the same name by suffixing with " (%d)"
  const rx_size n_displays{displays.size()};
  for (rx_size i{0}; i < n_displays; i++) {
    auto& lhs{displays[i]};
    int count{0};
    for (rx_size j{i+1}; j < n_displays; j++) {
      auto& rhs{displays[j]};
      if (lhs.m_name == rhs.m_name) {
        count++;
        rhs.m_name += string::format(" (%d)", count);
      }
    }
    if (count) {
      lhs.m_name += " (0)";
    }
  }

  return displays;
}

} // namespace rx
