NAME := s3torpedo
PREF := s3torpedo
include ../common.mk

# This cartridge's headers must beat the live tree's game/ and version.h.
CXXFLAGS := -Isrc $(CXXFLAGS)

# The live tree also has src/main.cpp and src/game/art.cpp. The shared
# pattern would compile those instead of this cartridge.
build/main.o: src/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/art.o: src/game/art.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
