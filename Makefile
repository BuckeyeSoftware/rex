# Compilation options:
# * LTO       - Link time optimization
# * ASAN      - Address sanitizer
# * TSAN      - Thread sanitizer
# * UBSAN     - Undefined behavior sanitizer
# * ESAN      - Engine sanitizer
# * DEBUG     - Debug build
# * PROFILE   - Profile build
# * SRCDIR    - Out of tree builds
# * UNUSED    - Removed unused references
LTO ?= 0
ASAN ?= 0
TSAN ?= 0
UBSAN ?= 0
ESAN ?= 0
DEBUG ?= 0
PROFILE ?= 0
SRCDIR ?= src
UNUSED ?= 1

#
# Some recursive make functions to avoid shelling out
#
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# C compiler
CC := gcc
CC ?= clang

# C++ compiler
# We use the C frontend with -xc++ to avoid linking in C++ runtime library.
CXX := $(CC) -xc++

# Determine if the C or C++ compiler should be used as the linker frontend.
ifneq (,$(findstring -xc++,$(CXX)))
	LD := $(CC)
else
	LD := $(CXX)
endif

# Determine build type.
ifeq ($(DEBUG), 1)
	TYPE := debug
else ifeq ($(PROFILE),1)
	TYPE := profile
else
	TYPE := release
endif

# Determine if Emscripten is being used.
ifneq (,$(findstring emcc,$(CC)))
	EMSCRIPTEN := 1
else
	EMSCRIPTEN := 0
endif

# Determine the binary output file name.
ifneq (,$(findstring mingw,$(CC)))
	BIN := rex.exe
else ifeq ($(EMSCRIPTEN), 1)
	BIN := rex.html
else
	BIN := rex
endif

# Artifacts generated and what to remove on clean.
ARTIFACTS := $(BIN)
ifeq ($(EMSCRIPTEN), 1)
	# Emscripten generates additional artifacts so list them here.
	ARTIFACTS += rex.js
	ARTIFACTS += rex.wasm
	ARTIFACTS += rex.wasm.map
	ARTIFACTS += rex.data
	ARTIFACTS += rex.worker.js
endif

# Build artifact directories.
OBJDIR := .build/$(TYPE)/objs
DEPDIR := .build/$(TYPE)/deps

# Collect all .cpp, .c and .S files for build in the source directory.
SRCS := $(call rwildcard, $(SRCDIR)/, *cpp)
SRCS += $(call rwildcard, $(SRCDIR)/, *c)
SRCS += $(call rwildcard, $(SRCDIR)/, *S)

# Generate object and dependency filenames.
OBJS := $(filter %.o,$(SRCS:%.cpp=$(OBJDIR)/%.o))
OBJS += $(filter %.o,$(SRCS:%.c=$(OBJDIR)/%.o))
OBJS += $(filter %.o,$(SRCS:%.S=$(OBJDIR)/%.o))

DEPS := $(filter %.d,$(SRCS:%.cpp=$(DEPDIR)/%.d))
DEPS += $(filter %.d,$(SRCS:%.c=$(DEPDIR)/%.d))

#
# Shared C and C++ compilation flags.
#
CFLAGS := -Isrc
CFLAGS += -Wall
CFLAGS += -Wextra

ifeq ($(EMSCRIPTEN), 1)
	# When building with Emscripten ensure we are generating code that is compiled
	# with support for atomics and bulk-memory features for threading support.
	CFLAGS += -s USE_PTHREADS=1
	CFLAGS += -s USE_SDL=2
else ifneq (,$(findstring mingw,$(CC)))
	# When building with mingw ensure we use a platform toolset that requires
	# Windows Vista as a minimum target.
	CFLAGS += -D_WIN32_WINNT=0x0600
	CFLAGS += -I/usr/x86_64-w64-mingw32/include/SDL2
else
	CFLAGS += `sdl2-config --cflags`
endif

# Give each function and data it's own section so the linker can remove unused
# references to each, producing smaller, tighter binaries.
ifeq ($(UNUSED), 1)
	CFLAGS += -ffunction-sections
	CFLAGS += -fdata-sections
endif

# Disable some unneeded features.
CFLAGS += -fno-unwind-tables
CFLAGS += -fno-asynchronous-unwind-tables

# Enable link-time optimizations if requested.
ifeq ($(LTO),1)
	CFLAGS += -flto
endif

# Enable engine sanitizer if requested.
ifeq ($(ESAN),1)
	CFLAGS += -DRX_ESAN
endif

ifeq ($(DEBUG),1)
	# Generate source maps with Emscripten.
	ifeq ($(EMSCRIPTEN),1)
		CFLAGS += -g4
	else
		CFLAGS += -g
	endif

	CFLAGS += -DRX_DEBUG

	# Optimize for debugging.
	CFLAGS += -O1

	# Ensure there's a frame pointer in debug builds.
	CFLAGS += -fno-omit-frame-pointer
else ifeq ($(PROFILE),1)
	# Enable profile options in profile builds.
	CFLAGS += -pg
	CFLAGS += -no-pie

	# Enable debug symbols and assertions in profile builds.
	CFLAGS += -g
	CFLAGS += -DRX_DEBUG

	# Use slightly less aggressive optimizations in profile builds.
	CFLAGS += -O2
	CFLAGS += -fno-inline-functions
	CFLAGS += -fno-inline-functions-called-once
	CFLAGS += -fno-optimize-sibling-calls
else
	# Enable assertions in release temporarily.
	CFLAGS += -DRX_DEBUG

	# Disable default C assertions.
	CFLAGS += -DNDEBUG

	# Highest optimization flag in release builds.
	CFLAGS += -O3

	# Disable all the stack protection features in release builds.
	CFLAGS += -fno-stack-protector
	CFLAGS += -fno-stack-check

	ifeq ($(CC),gcc)
		CFLAGS += -fno-stack-clash-protection
	endif

	# Disable frame pointer in release builds when AddressSanitizer isn't present.
	ifeq ($(ASAN),1)
		CFLAGS += -fno-omit-frame-pointer
	else
		CFLAGS += -fomit-frame-pointer
	endif
endif

# Sanitizer selection.
ifeq ($(ASAN),1)
	CFLAGS += -fsanitize=address
endif
ifeq ($(TSAN),1)
	CFLAGS += -fsanitize=thread -DRX_TSAN
endif
ifeq ($(UBSAN),1)
	CFLAGS += -fsanitize=undefined
endif

#
# C compilation flags.
#
CCFLAGS := $(CFLAGS)
CCFLAGS += -std=c11 -D_DEFAULT_SOURCE

#
# C++ compilation flags.
#
CXXFLAGS := $(CFLAGS)
CXXFLAGS += -std=c++20

# Disable some unneeded features.
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-rtti

#
# Dependency flags.
#
DEPFLAGS := -MMD
DEPFLAGS += -MP

#
# Output flags (the thing after -o bin)
#
ifeq ($(EMSCRIPTEN), 1)
	OFLAGS := --preload-file ./base/
endif

#
# Linker flags.
#
LDFLAGS := -ldl
LDFLAGS += -lm

# Emscripten specific linker flags.
ifeq ($(EMSCRIPTEN), 1)
	# Emscripten ports.
	LDFLAGS += -s USE_PTHREADS=1
	LDFLAGS += -s USE_SDL=2

	# Emulate full ES 3.0.
	LDFLAGS += -s FULL_ES3=1

	# Preinitialize a thread pool with four webworkers here to avoid blocking
	# in main waiting for our builtin thread pool to initialize since we depend
	# on synchronous thread construction of our thread pool.
	LDFLAGS += -s PTHREAD_POOL_SIZE=4

	# Allow the heap to grow when we run out of memory in the browser.
	LDFLAGS += -s ALLOW_MEMORY_GROWTH=1

	# Record all the Emscripten flags in the build.
	LDFLAGS += -s RETAIN_COMPILER_SETTINGS=1

	# Allocate 1 GiB for Rex.
	LDFLAGS += -s INITIAL_MEMORY=1073742000

	ifeq ($(DEBUG), 1)
		# When building debug builds with Emscripten the -g flag must also be given
		# to the linker. Similarly, a source map base must be given so that the
		# browser can find it when debugging.
		#
		# Note that the port 6931 is the default port for emrun.
		LDFLAGS += -g4 --source-map-base http://0.0.0.0:6931/

		# Catch threading bugs.
		LDFLAGS += -PTHREADS_DEBUG=1

		# Catch heap corruption issues. These are typically caused by misalignments
		# and pointer aliasing which tend to work on Desktop but won't on the Web.
		LDFLAGS += -s SAFE_HEAP=1
		LDFLAGS += -s SAFE_HEAP_LOG=1

		# Additional assertions and loggging.
		LDFLAGS += -s ASSERTIONS=2
		LDFLAGS += -s RUNTIME_LOGGING=1

		# OpenGL debug stuff.
		#
		# ES3 permits some things even FULL_ES3=1 does not support and we can catch
		# these with some additional assertions and debugging flags here.
		#
		# Typical issues that come up include draw buffer misuse and feedback loops
		# caused by render to texture.
		LDFLAGS += -s GL_ASSERTIONS=1
		LDFLAGS += -s GL_DEBUG=1
	else
		# Enable some potentially unsafe OpenGL optimizations for release builds.
		LDFLAGS += -s GL_UNSAFE_OPTS=1
		LDFLAGS += -s GL_TRACK_ERRORS=0
	endif
else
	LDFLAGS += -lpthread
	LDFLAGS += -lSDL2
endif

# Strip unused symbols if requested.
ifeq ($(UNUSED), 1)
	LDFLAGS += -Wl,--gc-sections
endif

# Enable profiling if requested.
ifeq ($(PROFILE),1)
	LDFLAGS += -pg
	LDFLAGS += -no-pie
endif

# Enable link-time optimizations if requested.
ifeq ($(LTO),1)
	LDFLAGS += -flto
endif

# Sanitizer selection.
ifeq ($(ASAN),1)
	LDFLAGS += -fsanitize=address
endif
ifeq ($(TSAN),1)
	LDFLAGS += -fsanitize=thread
endif
ifeq ($(UBSAN),1)
	LDFLAGS += -fsanitize=undefined
endif

# Strip the binary when not a debug build.
ifneq (,$(findstring RX_DEBUG,$(CFLAGS)))
	STRIP := true
else
	STRIP := strip
endif

all: $(BIN)

# Build artifact directories..
$(DEPDIR):
	@mkdir -p $(addprefix $(DEPDIR)/,$(call uniq,$(dir $(SRCS))))

$(OBJDIR):
	@mkdir -p $(addprefix $(OBJDIR)/,$(call uniq,$(dir $(SRCS))))

$(OBJDIR)/%.o: %.cpp $(DEPDIR)/%.d | $(OBJDIR) $(DEPDIR)
	$(CXX) -MT $@ $(DEPFLAGS) -MF $(DEPDIR)/$*.Td $(CXXFLAGS) -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

$(OBJDIR)/%.o: %.c $(DEPDIR)/%.d | $(OBJDIR) $(DEPDIR)
	$(CC) -MT $@ $(DEPFLAGS) -MF $(DEPDIR)/$*.Td $(CCFLAGS) -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	$(CC) -c -o $@ $<
	@mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

$(BIN): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@ $(OFLAGS)
	$(STRIP) $@

clean:
	rm -rf $(DEPDIR) $(OBJDIR) $(ARTIFACTS)

doc:
	doxygen $(SRCDIR)/rx/Doxyfile

.PHONY: clean doc $(DEPDIR) $(OBJDIR)

$(DEPS):
include $(wildcard $(DEPS))
