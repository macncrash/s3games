NAME := s3cable
PREF := s3cable
include ../common.mk

# The engine tree also ships main.cpp and game/art.cpp. common.mk's pattern
# rule would compile those instead of this cartridge.
build/main.o: src/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/art.o: src/game/art.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/cable.o: src/game/cable.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

