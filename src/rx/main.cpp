#include <SDL.h>

#include <rx/core/memory/system_allocator.h>

#include <rx/core/statics.h>
#include <rx/core/string.h>

#include <rx/core/log.h>
#include <rx/console/variable.h>
#include <rx/console/console.h>

#include <rx/math/vec3.h>

RX_LOG(hardware);

RX_CONSOLE_BVAR(b, "b", "a", true);
RX_CONSOLE_BVAR(c, "c", "a", true);
RX_CONSOLE_BVAR(d, "d", "a", true);
RX_CONSOLE_BVAR(f, "f", "a", true);
RX_CONSOLE_BVAR(g, "g", "a", true);
RX_CONSOLE_BVAR(h, "h", "a", true);
RX_CONSOLE_BVAR(i, "i", "a", true);
RX_CONSOLE_BVAR(j, "j", "a", true);
RX_CONSOLE_BVAR(a, "a", "a", true);
RX_CONSOLE_BVAR(e, "e", "a", true);

int entry(int argc, char **argv) {
  rx::console::console::load("");

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
  rx::static_globals::remove(system_alloc);

  // trigger log initialization
  auto log{rx::static_globals::find("log")};
  RX_ASSERT(log, "log missing");

  log->init();
  rx::static_globals::remove(log);

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
