NAME := s3dune
PREF := s3dune
# Search this cart before the live tree so headers are ours.
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

# The live tree also has src/main.cpp and src/game/art.cpp. Those pattern
# rules would steal this cart's objects; bind the cart sources explicitly.
$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
