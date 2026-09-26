NAME := s3solitaire
PREF := s3solitaire
# Cartridge headers before the live tree, which also ships game/art.h and version.h.
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

# The live tree's pattern rule would compile its game/art.cpp and main.cpp over this cart.
$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
