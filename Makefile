rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

CXX ?= clang++

SRCS := $(call rwildcard, src/, *cpp)
OBJS := $(filter %.o,$(SRCS:.cpp=.o))
DEPS := $(filter %.d,$(SRCS:.cpp=.d))

CXXFLAGS := -O3
CXXFLAGS += -std=c++17
CXXFLAGS += -Iinc
CXXFLAGS += -flto
CXXFLAGS += -DRX_DEBUG

# Disable a bunch of stuff we don't want
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-rtti
CXXFLAGS += -fno-stack-protector
CXXFLAGS += -fno-stack-clash-protection
CXXFLAGS += -fno-asynchronous-unwind-tables
CXXFLAGS += -fno-stack-check

LDFLAGS := -lpthread
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
