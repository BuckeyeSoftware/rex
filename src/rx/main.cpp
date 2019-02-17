#include <rx/core/statics.h>

int main() {
  rx::static_globals::init();
  rx::static_globals::fini();
  return 0;
}
