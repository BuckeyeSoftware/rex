#include <SDL.h>

#include <rx/core/memory/system_allocator.h>

#include <rx/core/statics.h>
#include <rx/core/string.h>

#include <rx/console/console.h>

#include <rx/input/input.h>

int entry(int argc, char **argv) {
  (void)argc;
  (void)argv;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window =
    SDL_CreateWindow("rex", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      1600, 900, SDL_WINDOW_OPENGL);

  SDL_GL_SwapWindow(window);

  if (!rx::console::console::load("config.cfg")) {
    // immediately save the default options on load failure
    rx::console::console::save("config.cfg");
  }

  rx::input::input input;
  while (!input.get_keyboard().is_released(SDLK_ESCAPE, false)) {
    input.update(0);

    // translate SDL events
    for (SDL_Event event; SDL_PollEvent(&event); ) {
      rx::input::event ievent;
      switch (event.type) {
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        ievent.type = rx::input::event_type::k_keyboard;
        ievent.as_keyboard.down = event.type == SDL_KEYDOWN;
        ievent.as_keyboard.scan_code = event.key.keysym.scancode;
        ievent.as_keyboard.symbol = event.key.keysym.sym;
        input.handle_event(rx::move(ievent));
        break;
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        ievent.type = rx::input::event_type::k_mouse_button;
        ievent.as_mouse_button.down = event.type == SDL_MOUSEBUTTONDOWN;
        ievent.as_mouse_button.button = event.button.button;
        input.handle_event(rx::move(ievent));
        break;
      case SDL_MOUSEMOTION:
        ievent.type = rx::input::event_type::k_mouse_motion;
        ievent.as_mouse_motion.value = { event.motion.x, event.motion.y };
        input.handle_event(rx::move(ievent));
        break;
      case SDL_MOUSEWHEEL:
        ievent.type = rx::input::event_type::k_mouse_scroll;
        ievent.as_mouse_scroll.value = { event.wheel.x, event.wheel.y };
        input.handle_event(rx::move(ievent));
        break;
      }
    }
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

#if 0
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    return -1;
  }

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    return -1;
  }

  const char* platform{SDL_GetPlatform()};
  hardware(rx::log::level::k_info, "platform: %s", platform);

  // enumerate displays and display info
  int displays{SDL_GetNumVideoDisplays()};
  hardware(rx::log::level::k_info, "discovered: %d displays", displays);
  for (int i{0}; i < displays; i++) {
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(i, &mode) == 0) {
      const char* name{SDL_GetDisplayName(i)};
      hardware(rx::log::level::k_info, "  display %d (%s):", i, name ? name : "unknown");
      hardware(rx::log::level::k_info, "    current mode:");
      hardware(rx::log::level::k_info, "      %d x %d @ %dHz", mode.w, mode.h, mode.refresh_rate);
      hardware(rx::log::level::k_info, "    modes:");
      int modes{SDL_GetNumDisplayModes(i)};
      for (int j{0}; j < modes; j++) {
        if (SDL_GetDisplayMode(i, j, &mode) == 0) {
          hardware(rx::log::level::k_info, "      %d x %d @ %dHz", mode.w, mode.h, mode.refresh_rate);
        }
      }
    }
  }

  // enumerate audio drivers
  int audio_drivers{SDL_GetNumAudioDrivers()};
  hardware(rx::log::level::k_info, "discovered: %d audio drivers", audio_drivers);
  for (int i{0}; i < audio_drivers; i++) {
    const char* name{SDL_GetAudioDriver(i)};
    if (!strcmp(name, "dummy")) {
      continue;
    }
    // hardware(rx::log::level::k_info, "  driver %s: ", name);
    if (SDL_AudioInit(name) == 0) {
      hardware(rx::log::level::k_info, "  driver %s: usable", name);
      int audio_devices{SDL_GetNumAudioDevices(0)};
      if (audio_devices > 0) {
        hardware(rx::log::level::k_info, "    discovered: %d audio devices", audio_devices);
        for (int j{0}; j < audio_devices; j++) {
          const char* device_name{SDL_GetAudioDeviceName(j, 0)};
          hardware(rx::log::level::k_info, "      device %s", device_name ? device_name : "unknown");
        }
      }
    } else {
      hardware(rx::log::level::k_info, "  driver %s: unusable", name);
    }
    SDL_AudioQuit();
  }
#endif
  return 0;
}

int main(int argc, char **argv) {
  // trigger system allocator initialization first
  auto system_alloc{rx::static_globals::find("system_allocator")};
  RX_ASSERT(system_alloc, "system allocator missing");

  // initialize system allocator and remove from static globals list
  system_alloc->init();

  // trigger log initialization
  auto log{rx::static_globals::find("log")};
  RX_ASSERT(log, "log missing");

  log->init();

  // initialize all other static globals
  rx::static_globals::init();

  // run the engine
  const auto result{entry(argc, argv)};

  // finalize all other static globals
  rx::static_globals::fini();

  // finalize log
  log->fini();

  // finalize system allocator
  system_alloc->fini();

  return result;
}
