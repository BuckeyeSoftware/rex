LTO ?= 0
ASAN ?= 0
TSAN ?= 0
UBSAN ?= 0
DEBUG ?= 0

PLATFORM := RX_PLATFORM_POSIX

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

CXX ?= clang++

SRCS := $(call rwildcard, src/, *cpp)
OBJS := $(filter %.o,$(SRCS:.cpp=.o))
DEPS := $(filter %.d,$(SRCS:.cpp=.d))

# compilation flags
CXXFLAGS := -std=c++17
CXXFLAGS += -Iinc
CXXFLAGS += `sdl2-config --cflags`
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -D$(PLATFORM)

ifeq ($(LTO),1)
	CXXFLAGS += -flto
endif

ifeq ($(DEBUG),1)
	CXXFLAGS += -DRX_DEBUG
	CXXFLAGS += -O0
	CXXFLAGS += -ggdb3
	CXXFLAGS += -fno-omit-frame-pointer
	CXXFLAGS += -fno-exceptions
	CXXFLAGS += -fno-rtti
else
	# enable assertions for release builds temporarily
	CXXFLAGS += -DRX_DEBUG
	CXXFLAGS += -O3

	# disable things we don't want in release
	CXXFLAGS += -fno-exceptions
	CXXFLAGS += -fno-rtti
	CXXFLAGS += -fno-stack-protector
	CXXFLAGS += -fno-asynchronous-unwind-tables
	CXXFLAGS += -fno-stack-check
	CXXFLAGS += -fomit-frame-pointer

	ifeq ($(CXX),g++)
		CXXFLAGS += -fno-stack-clash-protection
	endif
endif
ifeq ($(ASAN),1)
	CXXFLAGS += -fsanitize=address
endif
ifeq ($(TSAN),1)
	CXXFLAGS += -fsanitize=thread -DRX_TSAN
endif
ifeq ($(UBSAN),1)
	CXXFLAGS += -fsanitize=undefined
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
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BIN): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJS) $(DEPS) $(BIN)

.PHONY: clean

-include $(DEPS)
