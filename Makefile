# ============================================================================
#  Blacksmith -- live, dependency-free system monitor for Apple Silicon.
#
#      make                 debug build   (ASan + UBSan, -O0, every warning on)
#      make test            build + run every */tests/*_test.cc
#      make test T=mach_cpu build + run just sample_mach_cpu_test.cc
#      make lint            enforce the module dependency rules (see below)
#      make run             build + run the monitor (needs main/main.cc)
#      make release         optimised, no sanitizers -> build/release/blacksmith
#      make extern          fetch metal-cpp into extern/
#      make check           compile every source file, link nothing
#      make clean
#
#  Toolchain: Apple clang (Xcode 17+), -std=c++20. No package manager, no
#  third-party libraries. metal-cpp is header-only and vendored so the repo
#  builds offline; it is the only thing under extern/.
#
#  Layout:
#      source/blacksmith/<module>/x.hh          public header     namespace blacksmith::<module>
#      source/blacksmith/<module>/intern/x.cc   implementation, private headers
#      source/blacksmith/<module>/tests/<module>_x_test.cc
#      intern/check/check.hh                    in-house test harness
#      extern/metal-cpp/                        third-party
#      #include "model/cpu.hh"   "sample/sample.hh"   "check/check.hh"
# ============================================================================

# make has a built-in CXX=c++, so ?= would never win. Override only that.
ifeq ($(origin CXX),default)
CXX := clang++
endif

ROOT   := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SRC    := $(ROOT)/source/blacksmith
INTERN := $(ROOT)/intern
EXTERN := $(ROOT)/extern

# ---------------------------------------------------------------------------
#  Warnings. Strict on purpose: the classic mistakes around Mach and sysctl
#  are all things a warning can catch before the terminal does.
#      -Wconversion -Wsign-conversion   signed maths on 32-bit tick counters
#      -Wshadow                         a loop variable hiding a member
#      -Wold-style-cast                 C casts around Mach out-parameters
#  The two -Wno-gnu flags exist only because metal-cpp uses anonymous structs.
# ---------------------------------------------------------------------------
WARN := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
        -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual \
        -Wno-unused-parameter -Wno-gnu-anonymous-struct -Wno-nested-anon-types

# -ffile-prefix-map so __FILE__ in test output is repo-relative, not absolute.
CXXFLAGS := -std=c++20 $(WARN) -I $(SRC) -I $(INTERN) -I $(EXTERN)/metal-cpp \
            -ffile-prefix-map=$(ROOT)/= -DBLACKSMITH_ROOT=\"$(ROOT)\" -MMD -MP

# Debug is the default: a monitor that leaks a Mach buffer once a second is
# exactly what ASan exists for. Release is for measurements worth quoting.
DEBUG_FLAGS   := -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer
RELEASE_FLAGS := -O2 -DNDEBUG

# Foundation + Metal for the GPU ceiling, IOKit + CoreFoundation for GPU
# utilisation and power. Mach and sysctl live in libSystem and need nothing.
FRAMEWORKS := -framework Foundation -framework Metal \
              -framework IOKit -framework CoreFoundation

# Sanitizer runtime options. halt_on_error so the terminal is restored by the
# RawMode destructor before the report scrolls past.
ASAN_ENV := ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
            UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1

# Library = every module's intern/. main/ is the only module with no intern/.
MODULES := $(notdir $(patsubst %/,%,$(dir $(wildcard $(SRC)/*/))))
LIB_CC  := $(wildcard $(SRC)/*/intern/*.cc)
MAIN_CC := $(wildcard $(SRC)/main/main.cc)
TEST_CC := $(wildcard $(SRC)/*/tests/*_test.cc)

LIB_OBJS  := $(patsubst $(SRC)/%.cc,build/debug/%.o,$(LIB_CC))
RLIB_OBJS := $(patsubst $(SRC)/%.cc,build/release/%.o,$(LIB_CC))
MAIN_OBJ  := $(patsubst $(SRC)/%.cc,build/debug/%.o,$(MAIN_CC))
RMAIN_OBJ := $(patsubst $(SRC)/%.cc,build/release/%.o,$(MAIN_CC))

# T=mach_cpu selects *_mach_cpu_test.cc. T=cpu selects every *_cpu_test.cc.
TESTS     := $(if $(T),$(filter %_$(T)_test.cc,$(TEST_CC)),$(TEST_CC))
TEST_BINS := $(patsubst $(SRC)/%_test.cc,build/tests/%,$(TESTS))

.PHONY: all run release extern check test lint clean help

ifneq ($(MAIN_CC),)
all: build/debug/blacksmith
else
all: check
endif

build/debug/%.o: $(SRC)/%.cc
	@mkdir -p $(@D)
	@echo "  CXX  $*.cc"
	@$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) -c $< -o $@

build/release/%.o: $(SRC)/%.cc
	@mkdir -p $(@D)
	@echo "  CXX  $*.cc  [release]"
	@$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) -c $< -o $@

build/debug/blacksmith: $(LIB_OBJS) $(MAIN_OBJ)
	@echo "  LNK  build/debug/blacksmith"
	@$(CXX) $(DEBUG_FLAGS) $^ -o $@ $(FRAMEWORKS)

build/release/blacksmith: $(RLIB_OBJS) $(RMAIN_OBJ)
	@echo "  LNK  build/release/blacksmith"
	@$(CXX) $(RELEASE_FLAGS) $^ -o $@ $(FRAMEWORKS)

run: build/debug/blacksmith
	@$(ASAN_ENV) ./build/debug/blacksmith

release: build/release/blacksmith

check: $(LIB_OBJS)
	@if [ -z "$(LIB_CC)" ]; then echo "  no sources under source/blacksmith/*/intern/"; \
	 else echo "  ok   $(words $(LIB_OBJS)) object(s)"; fi

# A test is its own program linked against every intern/ object, so it sees
# exactly what the monitor sees.
build/tests/%: $(SRC)/%_test.cc $(LIB_OBJS) $(INTERN)/check/check.hh
	@mkdir -p $(@D)
	@echo "  CXX  $*_test.cc"
	@$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $< $(LIB_OBJS) -o $@ $(FRAMEWORKS)

test: lint $(TEST_BINS)
	@if [ -z "$(TEST_BINS)" ]; then echo "  no test matches T=$(T)"; exit 1; fi
	@fail=0; for t in $(TEST_BINS); do \
	   printf '\n  ---- %s ----\n' "$$(basename $$t)"; \
	   $(ASAN_ENV) $$t || fail=1; done; \
	 printf '\n'; [ $$fail -eq 0 ] && echo "  all tests passed" || { echo "  FAILURES above"; exit 1; }

# ---------------------------------------------------------------------------
#  Module dependency rules, enforced mechanically.
#    * model/ depends on nothing: no OS header, no other module.
#    * sample/ depends on model/ only; ui/, record/, inox/ depend on model/.
#    * only sample/intern/ includes Mach, sysctl, Metal, IOKit.
#    * a module's intern/ is included only from inside that module.
#    * headers never `using namespace`.
# ---------------------------------------------------------------------------
OS_HDR := (mach|sys|Metal|IOKit|Foundation|CoreFoundation|libproc)
lint:
	@bad=0; \
	 if [ -d $(SRC)/model ] && grep -rEn '#include [<"]($(OS_HDR)/|(sample|ui|record|inox|main)/)' $(SRC)/model; then bad=1; fi; \
	 for f in $$(find $(SRC) -name '*.cc' -o -name '*.hh' 2>/dev/null); do \
	   case "$$f" in */sample/intern/*|*/sample/tests/*) ;; *) \
	     if grep -En '#include [<"]$(OS_HDR)/' "$$f"; then bad=1; fi;; esac; \
	   mod=$${f#$(SRC)/}; mod=$${mod%%/*}; \
	   if grep -En '#include "[a-z]+/intern/' "$$f" | grep -v "\"$$mod/intern/"; then bad=1; fi; \
	 done; \
	 if grep -rEn --include='*.hh' '^\s*using namespace' $(SRC) $(INTERN); then bad=1; fi; \
	 if [ $$bad -ne 0 ]; then echo "  lint: the lines above break the module dependency rules"; exit 1; \
	 else echo "  lint ok"; fi

# metal-cpp is Apple's, Apache 2.0. The mirror tracks the zip Apple publishes
# at developer.apple.com/metal/cpp. Shallow clone, then the .git is dropped so
# extern/ is plain files committed with the repo.
extern:
	@if [ -d $(EXTERN)/metal-cpp/Metal ]; then echo "  extern/metal-cpp already present"; exit 0; fi
	@echo "  GET  metal-cpp"
	@git clone --depth 1 https://github.com/bkaradzic/metal-cpp $(EXTERN)/metal-cpp
	@rm -rf $(EXTERN)/metal-cpp/.git
	@echo "  ok   extern/metal-cpp -- commit it"

clean:
	@rm -rf build
	@echo "  cleaned"

help:
	@sed -n '2,13p' $(ROOT)/Makefile | sed 's/^# \{0,1\}//'

-include $(LIB_OBJS:.o=.d) $(RLIB_OBJS:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_BINS:=.d)
