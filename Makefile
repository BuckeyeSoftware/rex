# Compilation options:
# * LTO   - Link time optimization
# * ASAN  - Address sanitizer
# * TSAN  - Thread sanitizer
# * UBSAN - Undefined behavior sanitizer
# * DEBUG - Debug builds
LTO ?= 0
ASAN ?= 0
TSAN ?= 0
UBSAN ?= 0
DEBUG ?= 0

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

# C compiler
CC := gcc
CC ?= clang

# C++ compiler
# We use the C frontend with -xcpp to avoid linking in C++ runtime library
CXX := g++
CXX ?= clang++

SRCS := $(call rwildcard, src/, *cpp) $(call rwildcard, src/, *c)
OBJS := $(filter %.o,$(SRCS:.cpp=.o)) $(filter %.o,$(SRCS:.c=.o))
DEPS := $(filter %.d,$(SRCS:.cpp=.d)) $(filter %.d,$(SRCS:.c=.d))

# Shared C and C++ compilation flags
CFLAGS := -Isrc
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += `sdl2-config --cflags`

# C compilation flags
CCFLAGS := -std=c11

# C++ compilation flags
CXXFLAGS := -std=c++17
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-rtti

ifeq ($(LTO),1)
	CFLAGS += -flto
endif

ifeq ($(DEBUG),1)
	CFLAGS += -DRX_DEBUG
	CFLAGS += -O0
	CFLAGS += -ggdb3
	CFLAGS += -fno-omit-frame-pointer
else
	# enable assertions for release builds temporarily
	CFLAGS += -DRX_DEBUG
	CFLAGS += -O3
	CFLAGS += -fno-stack-protector
	CFLAGS += -fno-asynchronous-unwind-tables
	CFLAGS += -fno-stack-check
	CFLAGS += -fomit-frame-pointer

	ifeq ($(CXX),g++)
		CFLAGS += -fno-stack-clash-protection
	endif
endif
ifeq ($(ASAN),1)
	CFLAGS += -fsanitize=address
endif
ifeq ($(TSAN),1)
	CFLAGS += -fsanitize=thread -DRX_TSAN
endif
ifeq ($(UBSAN),1)
	CFLAGS += -fsanitize=undefined
endif

# linker flags
LDFLAGS := -lpthread
LDFLAGS += `sdl2-config --libs`
ifeq ($(LTO),1)
	LDFLAGS += -flto
endif
ifeq ($(ASAN),1)
	LDFLAGS += -fsanitize=address
endif
ifeq ($(TSAN),1)
	LDFLAGS += -fsanitize=thread
endif
ifeq ($(UBSAN),1)
	LDFLAGS += -fsanitize=undefined
endif

BIN := rex

all: $(BIN)

%.o: %.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJS) $(DEPS) $(BIN)

.PHONY: clean

-include $(DEPS)
