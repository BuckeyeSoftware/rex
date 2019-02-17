#include <SDL.h>

#include <rx/core/memory/system_allocator.h>
#include <rx/core/statics.h>
#include <rx/core/map.h>

struct A {
  A(int m) :m{m}, p{malloc(1)} { }
  ~A() { free(p); }
  A(A&& a) : m{a.m}, p{a.p} { a.p = nullptr; }
  A(const A& a) : m{a.m}, p{malloc(1)} { }
  bool operator==(const A& o) { return o.m == m; }
  A& operator=(A&& a) { m=a.m; return *this; }
  int m;

  struct hasher {
    rx_size operator()(const A& a) const {
      return rx::hash<rx_s32>{}(a.m);
    }
  };

  void *p;
};

int entry(int argc, char **argv) {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    return -1;
  }

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    return -1;
  }

  const char* platform{SDL_GetPlatform()};
  printf("platform: %s\n", platform);

  // enumerate displays and display info
  int displays{SDL_GetNumVideoDisplays()};
  printf("discovered: %d displays\n", displays);
  for (int i{0}; i < displays; i++) {
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(i, &mode) == 0) {
      const char* name{SDL_GetDisplayName(i)};
      printf("  display %d (%s):\n", i, name ? name : "unknown");
      printf("    current mode:\n");
      printf("      %d x %d @ %dHz\n", mode.w, mode.h, mode.refresh_rate);
      printf("    modes:\n");
      int modes{SDL_GetNumDisplayModes(i)};
      for (int j{0}; j < modes; j++) {
        if (SDL_GetDisplayMode(i, j, &mode) == 0) {
          printf("      %d x %d @ %dHz\n", mode.w, mode.h, mode.refresh_rate);
        }
      }
    }
  }

  // enumerate audio drivers
  int audio_drivers{SDL_GetNumAudioDrivers()};
  printf("discovered: %d audio drivers\n", audio_drivers);
  for (int i{0}; i < audio_drivers; i++) {
    const char* name{SDL_GetAudioDriver(i)};
    if (!strcmp(name, "dummy")) {
      continue;
    }
    printf("  driver %s: ", name);
    if (SDL_AudioInit(name) == 0) {
      printf("usable\n");
      int audio_devices{SDL_GetNumAudioDevices(0)};
      if (audio_devices > 0) {
        printf("    discovered: %d audio devices\n", audio_devices);
        for (int j{0}; j < audio_devices; j++) {
          const char* device_name{SDL_GetAudioDeviceName(j, 0)};
          printf("      device %s\n", device_name ? device_name : "unknown");
        }
      }
    } else {
      printf("unusable\n");
    }
    SDL_AudioQuit();
  }

  return 0;
}

int main(int argc, char **argv) {
  rx::static_globals::init();
  const auto result{entry(argc, argv)};
  rx::static_globals::fini();
  return result;
}
