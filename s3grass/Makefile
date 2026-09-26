NAME := s3grass
PREF := s3grass
# Cartridge headers before the live tree, which has its own version.h and game/.
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

# common.mk also matches the engine's main.cpp and game/art.cpp.
build/main.o: src/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/art.o: src/game/art.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/grass.o: src/game/grass.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
