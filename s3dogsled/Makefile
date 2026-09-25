NAME := s3dogsled
PREF := s3dogsled
include ../common.mk

# The live tree also has main.cpp and game/art.cpp. The shared pattern rule
# tries that tree first, and -I lists it before src, so pin this cartridge.
CXXFLAGS := -Isrc $(CXXFLAGS)

build/main.o: src/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/art.o: src/game/art.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
