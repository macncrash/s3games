NAME := s3shelvemark
PREF := s3shelvemark
# Cart headers (version.h, game/art.h) before the live tree.
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

# Bind cart sources so a live-tree src/main.cpp cannot steal the objects.
$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
