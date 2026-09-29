#include "game/art.h"

#include <cmath>
#include <initializer_list>

#include "console/gfx.h"

namespace fishchime {
namespace {

constexpr float kPi = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap fishBmp(int wag) {
    gs::Bitmap b(40, 22);
    float tail = wag ? 4.f : -3.f;
    b.poly({{4, 11 + tail}, {14, 6}, {14, 16}}, 1);
    b.ellipse(24, 11, 12, 7, 1);
    b.ellipse(26, 10, 8, 4, 2);
    b.poly({{18, 8}, {24, 3}, {26, 9}}, 3);
    b.poly({{16, 14}, {22, 18}, {24, 13}}, 3);
    b.ellipse(32, 9, 2, 2, 4);
    b.ellipse(33, 9, 1, 1, 5);
    b.rect(36, 10, 3, 1, 5);
    return b;
}

gs::Bitmap bellBmp() {
    gs::Bitmap b(28, 26);
    b.poly({{14, 1}, {12, 6}, {16, 6}}, 3);
    b.poly({{5, 7}, {23, 7}, {25, 18}, {3, 18}}, 1);
    b.poly({{8, 9}, {14, 9}, {13, 16}, {6, 16}}, 2);
    b.ellipse(14, 19, 12, 3, 1);
    b.ellipse(14, 21, 2, 2, 4);
    return b;
}

gs::Bitmap buoyBmp() {
    gs::Bitmap b(22, 36);
    b.rect(10, 0, 2, 14, 3);
    b.ellipse(11, 20, 9, 7, 1);
    b.ellipse(11, 19, 6, 4, 2);
    b.rect(10, 26, 2, 8, 3);
    return b;
}

gs::Bitmap kelpBmp() {
    gs::Bitmap b(16, 48);
    b.poly({{8, 0}, {4, 16}, {10, 28}, {3, 47}, {7, 47}, {13, 26}, {8, 14}, {12, 2}}, 1);
    b.poly({{9, 8}, {12, 20}, {8, 36}}, 2);
    return b;
}

gs::Bitmap pierBmp() {
    gs::Bitmap b(18, 70);
    b.rect(2, 0, 14, 8, 1);
    b.rect(4, 8, 3, 60, 2);
    b.rect(11, 8, 3, 60, 2);
    b.rect(4, 28, 10, 3, 1);
    b.rect(4, 48, 10, 3, 1);
    return b;
}

gs::Bitmap bubbleBmp() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(3, 3, 1, 1, 2);
    return b;
}

gs::Bitmap faceBmp() {
    gs::Bitmap b(kFace, kFace);
    float c = kFace * 0.5f;
    b.ellipse(c, c, c - 1.f, c - 1.f, 1);
    b.ellipse(c, c, c - 4.f, c - 4.f, 2);
    for (int i = 0; i < 12; i++) {
        float rad = float(i) * 30.f * kPi / 180.f;
        float dx = std::sin(rad);
        float dy = -std::cos(rad);
        b.line(c + dx * (c - 9.f), c + dy * (c - 9.f), c + dx * (c - 5.f), c + dy * (c - 5.f), 3, 1.4f);
    }
    b.ellipse(c, c, 2.f, 2.f, 3);
    return b;
}

gs::Bitmap handBmp(int which, int step) {
    gs::Bitmap b(kPivot * 2, kPivot * 2);
    float rad = float(step) * 30.f * kPi / 180.f;
    float dx = std::sin(rad);
    float dy = -std::cos(rad);
    float len = which == 0 ? 8.f : 12.f;
    float cx = float(kPivot);
    float cy = float(kPivot);
    b.line(cx, cy, cx + dx * len, cy + dy * len, 1, which == 0 ? 2.2f : 1.2f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_FISH, {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 12, 5), gs::rgb4(13, 5, 1), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(14, 11, 3), gs::rgb4(10, 7, 2), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_KELP, {0, gs::rgb4(1, 8, 3), gs::rgb4(3, 12, 5)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 8, 6), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(14, 13, 10), gs::rgb4(4, 5, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 9, 10)});
    setPal(vdp, PAL_BUB, {0, gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15)});

    art.fish[0] = gs::uploadImage(vdp, fishBmp(0));
    art.fish[1] = gs::uploadImage(vdp, fishBmp(1));
    art.bell = gs::uploadImage(vdp, bellBmp());
    art.buoy = gs::uploadImage(vdp, buoyBmp());
    art.kelp = gs::uploadImage(vdp, kelpBmp());
    art.pier = gs::uploadImage(vdp, pierBmp());
    art.bubble = gs::uploadImage(vdp, bubbleBmp());
    art.face = gs::uploadImage(vdp, faceBmp());
    for (int which = 0; which < 2; which++)
        for (int step = 0; step < kHandN; step++) art.hand[which][step] = gs::uploadImage(vdp, handBmp(which, step));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
}

}  // namespace fishchime
