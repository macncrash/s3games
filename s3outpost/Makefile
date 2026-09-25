# S3 OUTPOST. Console sources are the live S3 tree, not a copy.
NAME := s3outpost
PREF := s3outpost
S3_ENGINE ?= $(abspath $(CURDIR)/../../csys/s3rally/src)
ENGINE := $(S3_ENGINE)

CXX      ?= clang++
SDL2_CONFIG ?= $(firstword $(wildcard /opt/homebrew/bin/sdl2-config) sdl2-config)
SDL_CFLAGS := $(shell $(SDL2_CONFIG) --cflags)
SDL_LIBS   := $(shell $(SDL2_CONFIG) --libs)
BUILD_ID := $(shell git rev-parse --short HEAD 2>/dev/null || echo dev)
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -MMD -MP $(SDL_CFLAGS)
CXXFLAGS += -Isrc -I$(ENGINE) -DS3_BUILD='"$(BUILD_ID)"' -DS3_ORG='"s3games"' -DS3_PREF='"$(PREF)"'

CON_SRC := $(wildcard $(ENGINE)/console/*.cpp)
GAME_SRC := $(wildcard src/game/*.cpp src/*.cpp)
CON_OBJ := $(patsubst $(ENGINE)/console/%.cpp,build/console/%.o,$(CON_SRC))
GAME_OBJ := $(patsubst src/%.cpp,build/%.o,$(GAME_SRC))
OBJ := $(CON_OBJ) $(GAME_OBJ)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(SDL_LIBS)

build/console/%.o: $(ENGINE)/console/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

run: $(NAME)
	./$(NAME)

sim: $(NAME)
	./$(NAME) --sim

clean:
	rm -rf build $(NAME)

.PHONY: all run sim clean
-include $(OBJ:.o=.d)
