// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace duel {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_RIVAL = 2,
    PAL_WOOD = 3,
    PAL_PLANT = 4,
    PAL_HORSE = 5,
    PAL_LAND = 6,
    PAL_SUN = 7,
    PAL_ALERT = 8,
    PAL_DUST = 9,
    PAL_CLOUD = 10
};

enum Pose {
    POSE_FACE = 0,
    POSE_BACK,
    POSE_STEP,
    POSE_AIM,
    POSE_FIRE,
    POSE_HURT,
    POSE_DOWN,
    POSE_N
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    int ground[4] = {};
    gs::Mipped pose[POSE_N];
    gs::Mipped saloon, store, tower, hill, cactus, horse, fence, barrel, wheel;
    gs::Mipped sun, cloud, dust, shadow, flash, print, arrow, bird, weed;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace duel
