NAME := s3depotcolu
PREF := s3depotcolu
include ../common.mk

# This cartridge's headers must beat the live tree's game/ and version.h.
CXXFLAGS := -Isrc $(filter-out -Isrc,$(CXXFLAGS))

# The live tree also has main.cpp and game/art.cpp. A static pattern
# keeps this cartridge's sources.
$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
