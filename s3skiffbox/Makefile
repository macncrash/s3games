NAME := s3skiffbox
PREF := s3skiffbox
# Cartridge headers before the live tree, which has its own version.h and game/.
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

build/main.o: src/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/art.o: src/game/art.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/skiff.o: src/game/skiff.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
