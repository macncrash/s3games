NAME := s3clockseven
PREF := s3clockseven
CXXFLAGS := -O2 -g -Isrc
include ../common.mk

$(GAME_OBJ): build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
