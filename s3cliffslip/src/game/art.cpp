#include "game/art.h"

#include <cmath>
#include <cstdint>

namespace slip {

namespace {

constexpr float TAU = 6.28318530718f;
constexpr int SL = 128;
constexpr int SR = 192;
constexpr int BACK = 16;
constexpr int CLIFF = 86;

int noise(int x, int y) {
    uint32_t n = uint32_t(x) * 374761393u ^ uint32_t(y) * 668265263u;
    n = (n ^ (n >> 13)) * 1274126177u;
    return int(n >> 24);
}

bool water(int x, int y) {
    if (y >= CLIFF) return true;
    return x >= SL && x < SR && y >= BACK;
}

void paintHarbor(gs::Bitmap& b) {
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int n = noise(x, y);
            if (!water(x, y)) {
                int c = C_ROCK;
                if ((n & 7) == 0) c = C_STONE;
                if ((n % 11) == 0) c = C_LIT;
                if ((y + (n & 3)) % 9 == 0) c = C_SHADOW;
                if (y > CLIFF - 10) c = C_STONE;
                if ((n % 17) == 0 && y < CLIFF - 14) c = C_MOSS;
                if (y < 6) c = C_SHADOW;
                b.set(x, y, c);
                continue;
            }
            int c = C_DEEP;
            if (y > 150) c = (n & 3) == 0 ? C_MID : C_DEEP;
            if (y > CLIFF && y < CLIFF + 14) c = C_SHALLOW;
            if (x >= SL && x < SR && y < CLIFF + 6) c = C_MID;
            if ((n % 29) == 0) c = C_MID;
            if (y > 196 && (x % 23) < 2) c = C_FOAM;
            b.set(x, y, c);
        }
    }
    // Quay lip and timber fenders inside the notch.
    for (int x = 0; x < b.w; x++) {
        if (x >= SL - 2 && x < SR + 2) continue;
        b.set(x, CLIFF - 1, C_LIT);
        b.set(x, CLIFF, C_SAND);
    }
    for (int y = BACK; y < CLIFF + 4; y++) {
        b.set(SL, y, C_WOOD);
        b.set(SL + 1, y, (y & 3) == 0 ? C_ROPE : C_WOOD);
        b.set(SR - 1, y, C_WOOD);
        b.set(SR - 2, y, (y & 3) == 0 ? C_ROPE : C_WOOD);
    }
    for (int x = SL; x < SR; x++) {
        b.set(x, BACK, C_WOOD);
        b.set(x, BACK + 1, C_SHADOW);
        if ((x / 4) % 2 == 0) b.set(x, CLIFF - 8, C_MARK);
    }
    // Approach ticks down the fairway.
    for (int y = CLIFF + 16; y < 200; y += 14) {
        for (int k = -2; k <= 2; k++) b.set(160 + k, y, C_FOAM);
    }
    // A lamp and a buoy so the mouth reads.
    for (int y = CLIFF - 18; y < CLIFF - 4; y++) b.set(SL - 8, y, C_WOOD);
    b.set(SL - 9, CLIFF - 19, C_LAMP);
    b.set(SL - 8, CLIFF - 19, C_LAMP);
    b.set(SL - 7, CLIFF - 19, C_LAMP);
    for (int dy = -3; dy <= 3; dy++)
        for (int dx = -3; dx <= 3; dx++)
            if (dx * dx + dy * dy <= 7) b.set(210 + dx, CLIFF + 22 + dy, C_BUOY);
}

void paintBoat(gs::Bitmap& b, float a) {
    auto pt = [&](float bx, float by) -> gs::Pt {
        float c = std::cos(a), s = std::sin(a);
        return {16.f + c * bx - s * by, 16.f + s * bx + c * by};
    };
    b.poly({pt(12, 0), pt(-8, 6), pt(-10, 3), pt(-10, -3), pt(-8, -6)}, 1);
    b.poly({pt(2, 0), pt(-4, 3.2f), pt(-4, -3.2f)}, 2);
    b.poly({pt(-9, 2.2f), pt(-6, 2.2f), pt(-6, -2.2f), pt(-9, -2.2f)}, 3);
    b.line(pt(11, 0).first, pt(11, 0).second, pt(13.5f, 0).first, pt(13.5f, 0).second, 4, 1.2f);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto put = [&](int pal, int i, int r, int g, int b) { vdp.setColor(pal * 16 + i, gs::rgb4(r, g, b)); };
    put(PAL_TEXT, 1, 15, 14, 12);
    put(PAL_WARN, 1, 14, 4, 3);
    put(PAL_GOOD, 1, 8, 15, 9);
    put(PAL_DIM, 1, 8, 10, 12);

    put(PAL_MAP, C_DEEP, 1, 4, 8);
    put(PAL_MAP, C_MID, 2, 6, 10);
    put(PAL_MAP, C_SHALLOW, 3, 8, 11);
    put(PAL_MAP, C_FOAM, 10, 13, 13);
    put(PAL_MAP, C_ROCK, 4, 4, 5);
    put(PAL_MAP, C_STONE, 6, 6, 6);
    put(PAL_MAP, C_LIT, 9, 8, 7);
    put(PAL_MAP, C_WOOD, 8, 5, 2);
    put(PAL_MAP, C_ROPE, 12, 9, 4);
    put(PAL_MAP, C_MOSS, 3, 6, 3);
    put(PAL_MAP, C_SAND, 11, 9, 5);
    put(PAL_MAP, C_BUOY, 13, 3, 2);
    put(PAL_MAP, C_LAMP, 15, 13, 4);
    put(PAL_MAP, C_SHADOW, 2, 2, 3);
    put(PAL_MAP, C_MARK, 14, 12, 6);

    put(PAL_BOAT, 1, 13, 12, 10);
    put(PAL_BOAT, 2, 6, 8, 9);
    put(PAL_BOAT, 3, 12, 3, 2);
    put(PAL_BOAT, 4, 15, 14, 8);
    put(PAL_WAKE, 1, 12, 14, 14);
    put(PAL_WAKE, 2, 14, 14, 12);

    vdp.setFogColor(gs::rgb4(2, 5, 6));

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int i = 0; i < 96; i++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(32 + i)), st);
        art.gw[i] = g.w;
        art.gh = g.h;
        art.glyph[i] = gs::uploadImage(vdp, g);
    }

    for (int i = 0; i < BOAT_FRAMES; i++) {
        gs::Bitmap b(BOAT_PX, BOAT_PX);
        paintBoat(b, i * (TAU / BOAT_FRAMES));
        art.boat[i] = gs::uploadImage(vdp, b);
    }

    gs::Bitmap wake(10, 6);
    wake.ellipse(5, 3, 4.5f, 2.4f, 1);
    wake.ellipse(5, 3, 2.2f, 1.1f, 2);
    art.wake = gs::uploadImage(vdp, wake);

    gs::Bitmap gull(9, 5);
    gull.line(0, 3, 4, 1, 1, 1);
    gull.line(4, 1, 8, 3, 1, 1);
    art.gull = gs::uploadImage(vdp, gull);

    gs::Bitmap map(gs::SCREEN_W, gs::SCREEN_H);
    paintHarbor(map);
    gs::TileAlloc tiles(vdp, 1);
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, map, PAL_MAP);
    vdp.A.scroll(0, 0);
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    vdp.hudEnabled = false;
}

}  // namespace slip
