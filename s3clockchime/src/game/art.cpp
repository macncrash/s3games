#include "game/art.h"

#include <cmath>
#include <initializer_list>

#include "console/gfx.h"

namespace clockchime {
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

gs::Bitmap handBmp(int which, int step) {
    gs::Bitmap b(kPivot * 2, kPivot * 2);
    float rad = float(step) * 6.f * kPi / 180.f;
    float dx = std::sin(rad);
    float dy = -std::cos(rad);
    float len = which == 0 ? 22.f : which == 1 ? 30.f : 34.f;
    float thick = which == 0 ? 3.2f : which == 1 ? 2.1f : 1.15f;
    float cx = float(kPivot);
    float cy = float(kPivot);
    b.line(cx - dx * 6.f, cy - dy * 6.f, cx + dx * len, cy + dy * len, 1, thick);
    b.line(cx, cy, cx + dx * (len - 2.f), cy + dy * (len - 2.f), 2, which == 2 ? 0.6f : 1.f);
    b.ellipse(cx, cy, which == 2 ? 2.f : 3.f, which == 2 ? 2.f : 3.f, 1);
    return b;
}

gs::Bitmap faceBmp() {
    gs::Bitmap b(kFace, kFace);
    float c = kFace * 0.5f;
    b.ellipse(c, c, c - 1.f, c - 1.f, 1);
    b.ellipse(c, c, c - 6.f, c - 6.f, 2);
    b.ellipse(c, c, c - 8.f, c - 8.f, 3);
    for (int i = 0; i < 60; i++) {
        float rad = float(i) * 6.f * kPi / 180.f;
        float dx = std::sin(rad);
        float dy = -std::cos(rad);
        bool hour = (i % 5) == 0;
        float inner = hour ? c - 16.f : c - 12.f;
        float outer = c - 8.f;
        b.line(c + dx * inner, c + dy * inner, c + dx * outer, c + dy * outer, hour ? 5 : 4, hour ? 2.2f : 1.f);
    }
    b.ellipse(c, c, 4.f, 4.f, 5);
    return b;
}

gs::Bitmap bellBmp() {
    gs::Bitmap b(36, 32);
    b.poly({{18, 2}, {16, 8}, {20, 8}}, 3);
    b.poly({{6, 10}, {30, 10}, {32, 22}, {4, 22}}, 1);
    b.poly({{10, 12}, {18, 12}, {16, 20}, {8, 20}}, 2);
    b.ellipse(18, 24, 14, 4, 1);
    b.ellipse(18, 26, 3, 3, 4);
    return b;
}

gs::Bitmap ropeBmp() {
    gs::Bitmap b(6, 48);
    b.rect(2, 0, 2, 40, 1);
    b.ellipse(3, 42, 3, 4, 2);
    return b;
}

gs::Bitmap capBmp() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap keeperBmp() {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 6, 4, 4, 1);
    b.rect(6, 4, 4, 2, 2);
    b.rect(4, 11, 8, 10, 3);
    b.rect(5, 12, 3, 8, 4);
    b.rect(3, 12, 2, 7, 3);
    b.rect(11, 12, 2, 7, 3);
    b.rect(5, 21, 2, 6, 5);
    b.rect(9, 21, 2, 6, 5);
    b.rect(4, 26, 3, 2, 6);
    b.rect(9, 26, 3, 2, 6);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(11, 6, 4, 4, 0);
    return b;
}

gs::Bitmap starBmp() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(2, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 7, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(14, 12, 8), gs::rgb4(12, 10, 7), gs::rgb4(5, 4, 3), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_HOUR, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_MIN, {0, gs::rgb4(1, 3, 6), gs::rgb4(6, 9, 12)});
    setPal(vdp, PAL_SEC, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(13, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(6, 5, 3), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_KEEP,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 3, 6), gs::rgb4(1, 2, 4), gs::rgb4(3, 2, 2),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(14, 13, 9)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 12, 4)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    art.face = gs::uploadImage(vdp, faceBmp());
    for (int w = 0; w < 3; w++)
        for (int s = 0; s < kHandN; s++) art.hand[w][s] = gs::uploadImage(vdp, handBmp(w, s));
    art.bell = gs::uploadImage(vdp, bellBmp());
    art.rope = gs::uploadImage(vdp, ropeBmp());
    art.cap = gs::uploadImage(vdp, capBmp());
    art.keeper = gs::uploadImage(vdp, keeperBmp());
    art.moon = gs::uploadImage(vdp, moonBmp());
    art.star = gs::uploadImage(vdp, starBmp());
}

}  // namespace clockchime
