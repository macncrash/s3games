NAME := s3skiffbuoy
PREF := s3skiffbuoy
include ../common.mk

# The live engine tree has its own main.cpp and game/art.cpp. These explicit
# rules keep the cartridge sources ahead of the shared pattern.
build/main.o: src/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/art.o: src/game/art.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
