# ============================================================================
#  Blacksmith -- live, dependency-free system monitor for Apple Silicon.
#
#      make            debug build   (ASan + UBSan, -O0, every warning on)
#      make run        build + run
#      make release    optimised build, no sanitizers  -> build/release/blacksmith
#      make vendor     fetch metal-cpp into vendor/     (needed from task 1.10)
#      make check      compile every source file, link nothing
#      make clean
#
#  Toolchain: Apple clang (Xcode 17+), -std=c++20. No package manager, no
#  third-party libraries. metal-cpp is header-only and vendored so the repo
#  builds offline; it is the only thing under vendor/.
#
#  Frameworks, and the phase that needs each:
#      Foundation, Metal          phase 1  recommendedMaxWorkingSetSize
#      IOKit, CoreFoundation      phase 4  accelerator statistics
#  Mach (host_statistics64, host_processor_info) and sysctl live in libSystem
#  and need nothing extra.
# ============================================================================

CXX ?= clang++

ROOT     := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SRC      := $(ROOT)/src
METALCPP := $(ROOT)/vendor/metal-cpp

# ---------------------------------------------------------------------------
#  Warnings. Strict on purpose: the traps in TODO.md are all things a warning
#  can catch before the terminal does.
#      -Wconversion -Wsign-conversion   signed maths on 32-bit tick counters
#      -Wshadow                         a loop variable hiding a member
#      -Wold-style-cast                 C casts around Mach out-parameters
#  The two -Wno-gnu flags exist only because metal-cpp uses anonymous structs.
# ---------------------------------------------------------------------------
WARN := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
        -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual \
        -Wno-unused-parameter -Wno-gnu-anonymous-struct -Wno-nested-anon-types

CXXFLAGS := -std=c++20 $(WARN) -I $(SRC) -I $(METALCPP) \
            -DBLACKSMITH_ROOT=\"$(ROOT)\" -MMD -MP

# Debug is the default: a monitor that leaks a Mach buffer once a second is
# exactly what ASan exists for. Release is for the numbers you quote.
DEBUG_FLAGS   := -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS := -O2 -DNDEBUG

FRAMEWORKS := -framework Foundation -framework Metal \
              -framework IOKit -framework CoreFoundation

# Sanitizer runtime options for `make run`. halt_on_error so the terminal is
# restored by the RawMode destructor before the report scrolls past.
ASAN_ENV := ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
            UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1

SOURCES := $(wildcard $(SRC)/*.cpp)
OBJS    := $(patsubst $(SRC)/%.cpp,build/debug/%.o,$(SOURCES))
ROBJS   := $(patsubst $(SRC)/%.cpp,build/release/%.o,$(SOURCES))

.PHONY: all run release vendor check clean help

all: build/debug/blacksmith

build/debug/%.o: $(SRC)/%.cpp
	@mkdir -p $(@D)
	@echo "  CXX  src/$*.cpp"
	@$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@

build/release/%.o: $(SRC)/%.cpp
	@mkdir -p $(@D)
	@echo "  CXX  src/$*.cpp  [release]"
	@$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -c $< -o $@

build/debug/blacksmith: $(OBJS)
	@if [ -z "$(SOURCES)" ]; then echo "  no sources in src/ yet -- see TODO.md task 1.1"; exit 1; fi
	@echo "  LNK  build/debug/blacksmith"
	@$(CXX) $(DEBUG_FLAGS) $(OBJS) -o $@ $(FRAMEWORKS)

build/release/blacksmith: $(ROBJS)
	@if [ -z "$(SOURCES)" ]; then echo "  no sources in src/ yet -- see TODO.md task 1.1"; exit 1; fi
	@echo "  LNK  build/release/blacksmith"
	@$(CXX) $(RELEASE_FLAGS) $(ROBJS) -o $@ $(FRAMEWORKS)

run: build/debug/blacksmith
	@$(ASAN_ENV) ./build/debug/blacksmith

release: build/release/blacksmith

check: $(OBJS)
	@echo "  ok   $(words $(OBJS)) object(s)"

# metal-cpp is Apple's, Apache 2.0. The mirror tracks the zip Apple publishes
# at developer.apple.com/metal/cpp. Shallow clone, then the .git is dropped so
# vendor/ is plain files committed with the repo.
vendor:
	@if [ -d $(METALCPP)/Metal ]; then echo "  vendor/metal-cpp already present"; exit 0; fi
	@echo "  GET  metal-cpp"
	@git clone --depth 1 https://github.com/bkaradzic/metal-cpp $(METALCPP)
	@rm -rf $(METALCPP)/.git
	@echo "  ok   vendor/metal-cpp -- commit it"

clean:
	@rm -rf build
	@echo "  cleaned"

help:
	@sed -n '2,12p' $(ROOT)/Makefile | sed 's/^# \{0,1\}//'

-include $(OBJS:.o=.d) $(ROBJS:.o=.d)
