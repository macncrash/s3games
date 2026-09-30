// S3 CULVERT CLER pictures. Drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ccler {

enum Pal {
    PAL_TEXT = 0,
    PAL_PIPE = 1,
    PAL_MOSS = 2,
    PAL_WATER = 3,
    PAL_WADER = 4,
    PAL_SILT = 5,
    PAL_BRICK = 6,
    PAL_TIN = 7,
    PAL_GOOD = 8,
    PAL_ALERT = 9,
    PAL_CLOCK = 10
};

struct Art {
    gs::Mipped ring;
    gs::Mipped mouth;
    gs::Mipped wader[2];
    gs::Mipped shovel;
    gs::Mipped silt;
    gs::Mipped brick;
    gs::Mipped branch;
    gs::Mipped tin;
    gs::Mipped clod;
    gs::Mipped wheel;
    gs::Mipped drip;
    gs::Mipped puff;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ccler
