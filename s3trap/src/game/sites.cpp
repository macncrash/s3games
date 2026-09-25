#include "game/sites.h"

#include "game/art.h"

namespace trap {
namespace {
uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }
}  // namespace

const Site& siteDef(int i) {
    static const Site k[NUM_SITES] = {
        {"THE BOAT", "CVN DECK", "DUSK. CALM SEA. THREE WIRE.", 70, 170, 20, 3.5f, 72, 8.0f, true, 4, 0, 0.35f, 9.0f, 0.4f, 0,
         0.6f, 1852, 10, 8, 1.0f, true, true, SEA},
        {"NIGHT BOAT", "CASE III", "PITCHING DECK. CROSSWIND.", 70, 170, 20, 3.5f, 72, 8.0f, true, 4, 0, 1.7f, 6.5f, 2.4f, 7,
         1.4f, 1852, -14, 4, 1.0f, true, true, NIGHT},
        {"CANYON LEDGE", "RED WALL", "SHORT DIRT. STOP BEFORE THE EDGE.", 24, 150, 11, 4.2f, 58, 9.0f, false, 0, 0, 0, 8, 0, 4,
         3.2f, 980, 6, 12, 1.0f, false, false, CANYON},
        {"ROOFTOP", "MERIDIAN TOWER", "ONE NET. DON'T MISS THE BUILDING.", 16, 28, 10, 5.0f, 56, 9.2f, true, 1, 0, 0.4f, 7.0f,
         1.1f, 8, 1.6f, 820, -8, 6, 1.0f, true, false, CITY},
        {"ICE SHELF", "WHITE DECK", "LONG SLIDE. BRAKES ARE A SUGGESTION.", 30, 360, 18, 3.0f, 66, 8.2f, false, 0, 0, 0.2f, 11,
         1.6f, 5, 0.8f, 1600, 12, 5, 0.38f, false, true, ICE},
        {"FLATCAR", "NIGHT FREIGHT", "THE PAD IS MOVING. THE NET IS NOT WIDE.", 14, 32, 8, 4.5f, 64, 8.6f, true, 1, 26, 0.6f,
         5.5f, 1.8f, 3, 1.8f, 760, 4, 8, 1.0f, true, false, HIGHWAY},
        {"CALDERA", "ASH STRIP", "UPDRAFTS OFF THE RIM. STOP ON THE ASH.", 22, 190, 12, 4.0f, 60, 9.0f, false, 0, 0, 0, 7, 0, 2,
         2.4f, 900, -6, 8, 0.9f, false, false, ASH},
        {"HIGHWAY", "EMPTY DAWN", "TWO LANES. THE OVERPASS ENDS IT.", 18, 210, 9, 2.6f, 64, 8.4f, false, 0, 0, 0, 9, 0.8f, 3,
         1.0f, 1200, 8, 6, 0.95f, true, false, HIGHWAY},
        {"THE RIG", "NORTH SPINE", "A TINY PAD ON A SWAYING LEG.", 12, 16, 8, 5.5f, 54, 10.0f, true, 1, 0, 2.0f, 5.2f, 3.2f,
         9, 2.0f, 640, 5, 4, 1.0f, true, true, RIG},
    };
    if (i < 0) i = 0;
    if (i >= NUM_SITES) i = NUM_SITES - 1;
    return k[i];
}

Sky themeSky(Theme t) {
    switch (t) {
        case NIGHT: return {C(0, 0, 3), C(1, 1, 4), C(1, 1, 3)};
        case CANYON: return {C(4, 5, 12), C(14, 8, 5), C(12, 7, 4)};
        case CITY: return {C(1, 1, 4), C(3, 2, 6), C(2, 2, 4)};
        case ICE: return {C(6, 8, 12), C(13, 14, 15), C(12, 13, 15)};
        case ASH: return {C(5, 3, 3), C(12, 6, 3), C(8, 4, 3)};
        case HIGHWAY: return {C(3, 5, 12), C(15, 10, 6), C(13, 9, 6)};
        case RIG: return {C(0, 1, 4), C(1, 2, 5), C(0, 1, 3)};
        case SEA:
        default: return {C(2, 4, 12), C(15, 9, 6), C(12, 8, 6)};
    }
}

void applyTheme(gs::VDP& vdp, const Site& s) {
    auto put = [&](int pal, const uint16_t* c) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
    };
    uint16_t deck[16] = {};
    uint16_t side[16] = {};
    auto sea = [&](uint16_t a, uint16_t b, uint16_t spark) {
        deck[11] = side[11] = a;
        deck[12] = side[12] = b;
        deck[13] = side[13] = spark;
    };
    if (s.theme == NIGHT || s.theme == RIG) {
        deck[6] = C(4, 4, 5);
        deck[7] = C(3, 3, 4);
        deck[14] = C(15, 13, 3);
        deck[15] = C(8, 8, 4);
        deck[4] = deck[5] = C(12, 12, 10);
        sea(C(0, 1, 3), C(0, 1, 4), C(2, 3, 6));
        for (int i = 1; i < 11; i++) side[i] = C(1, 1, 2);
        side[3] = C(8, 7, 2);
    } else if (s.theme == ICE) {
        deck[1] = C(14, 15, 15);
        deck[2] = C(11, 13, 15);
        deck[6] = C(15, 15, 15);
        deck[7] = C(13, 14, 15);
        deck[9] = C(10, 12, 14);
        deck[10] = C(8, 11, 14);
        deck[14] = C(4, 8, 14);
        deck[15] = C(12, 14, 15);
        deck[4] = deck[5] = C(15, 15, 15);
        sea(C(3, 7, 12), C(4, 8, 13), C(10, 13, 15));
    } else if (s.theme == CANYON || s.theme == ASH) {
        bool ash = s.theme == ASH;
        deck[6] = ash ? C(6, 5, 5) : C(12, 7, 4);
        deck[7] = ash ? C(4, 3, 3) : C(9, 5, 3);
        deck[9] = C(5, 3, 2);
        deck[10] = C(3, 2, 2);
        deck[8] = C(7, 6, 5);
        deck[15] = C(10, 8, 6);
        deck[4] = deck[5] = C(14, 12, 8);
        side[1] = ash ? C(5, 3, 2) : C(10, 5, 3);
        side[2] = ash ? C(3, 2, 2) : C(7, 3, 2);
        side[3] = C(4, 3, 2);
        side[6] = side[1];
        side[7] = side[2];
        side[9] = C(4, 2, 1);
        side[10] = C(3, 2, 1);
    } else if (s.theme == CITY) {
        deck[6] = C(5, 5, 6);
        deck[7] = C(3, 3, 4);
        deck[14] = C(15, 12, 3);
        deck[15] = C(8, 8, 6);
        deck[4] = C(14, 14, 12);
        deck[5] = C(10, 2, 2);
        side[1] = C(2, 2, 3);
        side[2] = C(1, 1, 2);
        side[3] = C(12, 10, 3);
        side[6] = side[1];
        side[7] = side[2];
    } else if (s.theme == HIGHWAY) {
        deck[6] = C(5, 5, 5);
        deck[7] = C(4, 4, 4);
        deck[14] = C(15, 14, 8);
        deck[15] = C(7, 7, 6);
        deck[4] = C(15, 15, 15);
        deck[5] = C(12, 12, 12);
        side[1] = C(5, 8, 3);
        side[2] = C(4, 6, 2);
        side[3] = C(3, 4, 2);
        side[6] = C(6, 6, 5);
        side[7] = C(5, 5, 4);
        side[9] = C(4, 4, 3);
        side[10] = C(3, 3, 2);
    } else {
        deck[6] = C(6, 6, 7);
        deck[7] = C(5, 5, 6);
        deck[14] = C(15, 14, 6);
        deck[15] = C(8, 8, 7);
        deck[4] = C(15, 15, 15);
        deck[5] = C(12, 12, 11);
        sea(C(2, 5, 10), C(3, 6, 12), C(8, 12, 15));
        side[1] = C(3, 6, 11);
        side[2] = C(2, 5, 9);
    }
    put(PAL_DECK, deck);
    put(PAL_SIDE, side);
    Sky sky = themeSky(s.theme);
    vdp.setFogColor(sky.fog);
}

}  // namespace trap
