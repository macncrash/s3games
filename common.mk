# Shared build for a S3-16 cartridge in this directory.
# The title Makefile sets NAME and PREF, then includes this file.

CXX      ?= clang++
SDL2_CONFIG ?= $(firstword $(wildcard /opt/homebrew/bin/sdl2-config) sdl2-config)
SDL_CFLAGS := $(shell $(SDL2_CONFIG) --cflags)
SDL_LIBS   := $(shell $(SDL2_CONFIG) --libs)
# The S3-16 sources. Override when the checkout is not beside this repo.
S3_ENGINE ?= $(abspath $(CURDIR)/../../csys/s3rally/src)
ENGINE := $(S3_ENGINE)
BUILD_ID := $(shell git rev-parse --short HEAD 2>/dev/null || echo dev)
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -MMD -MP $(SDL_CFLAGS)
CXXFLAGS += -I$(ENGINE) -Isrc -DS3_BUILD='"$(BUILD_ID)"' -DS3_ORG='"s3games"' -DS3_PREF='"$(PREF)"'

CON_SRC := $(wildcard $(ENGINE)/console/*.cpp)
GAME_SRC := $(wildcard src/game/*.cpp src/*.cpp)
CON_OBJ := $(patsubst $(ENGINE)/%.cpp,build/%.o,$(CON_SRC))
GAME_OBJ := $(patsubst src/%.cpp,build/%.o,$(GAME_SRC))
OBJ := $(CON_OBJ) $(GAME_OBJ)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LIBS)

build/%.o: $(ENGINE)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(NAME)
	./$(NAME)

sim: $(NAME)
	./$(NAME) --sim

web:
	@mkdir -p build-web
	em++ -std=c++17 -O2 -I$(ENGINE) -Isrc -DS3_BUILD='"$(BUILD_ID)"' -DS3_ORG='"s3games"' -DS3_PREF='"$(PREF)"' \
		-sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=134217728 -sSTACK_SIZE=1048576 \
		-sENVIRONMENT=web --shell-file web/shell.html $(CON_SRC) $(GAME_SRC) -o build-web/index.html

clean:
	rm -rf build build-web $(NAME)

.PHONY: all run sim web clean
-include $(OBJ:.o=.d)
