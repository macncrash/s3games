// S3 DEPOT BANN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace dbann {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_YARD = 2,
    PAL_BANN = 3,
    PAL_LOCO = 4,
    PAL_CAR = 5,
    PAL_TEAL = 6,
    PAL_LAMP = 7,
    PAL_WOOD = 8,
    PAL_ALERT = 9,
    PAL_TITLE = 10,
    PAL_NIGHT = 11,
    PAL_SMOKE = 12
};

struct Art {
    gs::Mipped loco;
    gs::Mipped car;
    gs::Mipped cloth[2];
    gs::Mipped mast;
    gs::Mipped flat;
    gs::Mipped lamp;
    gs::Mipped crate;
    gs::Mipped tower;
    gs::Mipped moon;
    gs::Mipped cloud;
    gs::Mipped puff;
    gs::Mipped shadow;
    gs::Mipped plate;
    gs::Mipped logo, tag, hint, hint2, win, lose;
    int font[96] = {};
    int ballast[3] = {};
    int rail = 1;
    int apron = 1;
    int hazard = 1;
    int brick = 1;
    int window = 1;
    int door = 1;
    int roof = 1;
    int shed = 1;
    int shedWin = 1;
    int star = 1;
    int hill = 1;
    int bar = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dbann
