NAME := s3battery
PREF := s3battery
# Search this cart before the live tree so game/art.h and version.h are ours.
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

# The live tree also has src/main.cpp and src/game/art.cpp. Those pattern
# rules would steal this cart's objects; bind the cart sources explicitly.
$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
