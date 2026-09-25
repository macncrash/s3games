NAME := s3oven
PREF := s3oven
# This cart's headers before the live tree (it also ships game/ and version.h).
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

# The live tree's pattern rule would compile its src/main.cpp over this cart.
$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
