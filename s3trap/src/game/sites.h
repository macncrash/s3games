// Landing sites for S3 TRAP. The first two are the boat. The rest are not.
#pragma once
#include <cstdint>

#include "console/vdp.h"

namespace trap {

enum Theme { SEA, NIGHT, CANYON, CITY, ICE, ASH, HIGHWAY, RIG };

struct Site {
    const char* name;
    const char* place;
    const char* blurb;
    float deckAft, deckFwd, halfW;
    float slopeDeg, vRef, aoaRef;
    bool wires;
    int wireCount;
    float padSpeed;
    float pitchAmp, period, swayAmp;
    float wind, gust;
    float startZ, startX, startHigh;
    float friction;
    bool hardDeck;
    bool sea;
    Theme theme;
};

constexpr int NUM_SITES = 9;
const Site& siteDef(int i);

struct Sky {
    uint16_t top, horizon, fog;
};
Sky themeSky(Theme t);
void applyTheme(gs::VDP& vdp, const Site& s);

}  // namespace trap
