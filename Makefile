# Compilation options:
# * LTO       - Link time optimization
# * ASAN      - Address sanitizer
# * TSAN      - Thread sanitizer
# * UBSAN     - Undefined behavior sanitizer
# * DEBUG     - Debug build
# * PROFILE   - Profile build
# * SRCDIR    - Out of tree builds
# * UNUSED    - Removed unused references
LTO ?= 0
ASAN ?= 0
TSAN ?= 0
UBSAN ?= 0
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

# Build artifact directories
OBJDIR := .build/objs
DEPDIR := .build/deps

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
CFLAGS += `sdl2-config --cflags`

# Give each function and data it's own section so the linker can remove unused
# references to each, producing smaller, tighter binaries.
ifeq ($(UNUSED), 1)
	CFLAGS += -ffunction-sections
	CFLAGS += -fdata-sections
endif

# Disable some unneeded features.
CFLAGS += -fno-asynchronous-unwind-tables

# Enable link-time optimizations if requested.
ifeq ($(LTO),1)
	CFLAGS += -flto
endif

ifeq ($(DEBUG),1)
	CFLAGS += -g
	CFLAGS += -DRX_DEBUG

	# Disable all optimizations in debug builds.
	CFLAGS += -O0

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

	# Remotery
	CFLAGS += -DRMT_ENABLED=1
	CFLAGS += -DRMT_USE_OPENGL=1

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
CXXFLAGS += -std=c++17

# Disable some unneeded features.
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-rtti

#
# Dependency flags.
#
DEPFLAGS := -MMD
DEPFLAGS += -MP

#
# Linker flags.
#
LDFLAGS := -lpthread
LDFLAGS += `sdl2-config --libs`

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

BIN := rex

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
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(STRIP) $@

clean:
	rm -rf $(DEPDIR) $(OBJDIR) $(BIN)

.PHONY: clean $(DEPDIR) $(OBJDIR)

$(DEPS):
include $(wildcard $(DEPS))
