NAME := s3gatereli
PREF := s3gatereli
include ../common.mk

# The live tree also ships rally's main.cpp and game/art.cpp. A static pattern
# keeps this cartridge's sources, and -Isrc is first so its headers win.
CXXFLAGS := -Isrc $(filter-out -Isrc,$(CXXFLAGS))

$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
