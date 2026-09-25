NAME := s3therm
PREF := s3therm
# This cart's headers before the live tree (it also ships game/art.h and version.h).
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

# The live tree's pattern rule would compile its game/art.cpp over this cart.
$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
