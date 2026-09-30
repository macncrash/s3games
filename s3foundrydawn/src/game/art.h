// S3 FOUNDRY DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace foundrydawn {

enum Pal {
    PAL_HUD = 0,
    PAL_SOOT = 1,
    PAL_FLOOR = 2,
    PAL_MAN = 3,
    PAL_FIRE = 4,
    PAL_EMBER = 5,
    PAL_IRON = 6,
    PAL_SLAG = 7,
    PAL_QUENCH = 8,
    PAL_SMOKE = 9,
    PAL_MOON = 10,
    PAL_SUN = 11,
    PAL_GOLD = 12,
    PAL_ALERT = 13,
    PAL_PIP = 14,
    PAL_COAL = 15
};

struct Art {
    gs::Mipped man[2];
    gs::Mipped crucible;
    gs::Mipped flame[2];
    gs::Mipped slag;
    gs::Mipped quench;
    gs::Mipped coal;
    gs::Mipped spark;
    gs::Mipped moon;
    gs::Mipped glyph[96];
    int font[96] = {};
    int soot = 0;
    int beam = 0;
    int plate = 0;
    int grate = 0;
    int dark = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundrydawn
