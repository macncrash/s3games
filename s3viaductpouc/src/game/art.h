#pragma once
#include "console/gfx.h"

namespace pouch {

struct Art {
    gs::Image runner[2];
    gs::Image pouch;
    gs::Image deck;
    gs::Image pier;
    gs::Image moon;
    gs::Image cloud;
    gs::Image lamp;
    gs::Image flag;
    gs::Image gull;
    int font[96] = {};
};

void makeArt(gs::VDP& vdp, Art& art);

}  // namespace pouch
