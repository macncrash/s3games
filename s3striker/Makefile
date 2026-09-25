NAME := s3striker
PREF := s3striker
include ../common.mk

# The live tree also ships main.cpp and game/art.cpp. These explicit rules
# keep the cartridge objects on this game; the pattern rule would take the engine copies.
build/main.o: src/main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/art.o: src/game/art.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/game/striker.o: src/game/striker.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<
