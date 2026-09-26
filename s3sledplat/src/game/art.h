// S3 SLED PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SLED = 4,
    PAL_TIMBER = 5,
    PAL_SNOW = 6,
    PAL_PINE = 7,
    PAL_HOUSE = 8,
    PAL_SKY = 9,
    PAL_MARK = 10,
    PAL_ROCK = 11,
    PAL_SIGN = 12,
    PAL_DUST = 13,
    PAL_FAR = 14,
    PAL_LAMP = 15
};

constexpr int kPoses = 7;
// Nose-up radians. The middle pose is sitting flat on the runners.
constexpr float kPoseAtt[kPoses] = {-0.50f, -0.24f, -0.12f, 0.f, 0.12f, 0.24f, 0.42f};

struct SledImg {
    gs::Mipped img;
    float ax = 0, ay = 0;  // runner contact under the cargo door
    float ppm = 20;
};

struct Art {
    SledImg sled[kPoses];
    gs::Mipped plank, fascia, riser, trestle;
    gs::Mipped house, post, board, chev, lamp;
    gs::Mipped word;
    gs::Mipped pine, rock, shed;
    gs::Mipped snow, puff, shade, flake;
    gs::Mipped cloud, ridge, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledplat
