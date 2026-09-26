NAME := s3ridgecolu
PREF := s3ridgecolu
include ../common.mk

# This cartridge's headers must beat the live tree's game/ and version.h.
CXXFLAGS := -Isrc $(filter-out -Isrc,$(CXXFLAGS))
