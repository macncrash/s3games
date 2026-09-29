#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace bedsmark {
namespace {

uint16_t rgb(int r, int g, int b) { return gs::rgb4(r, g, b); }

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(p * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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

void paint(gs::VDP& vdp) {
    pal(vdp, PAL_HUD, {0, rgb(15, 15, 14), rgb(2, 3, 5)});
    pal(vdp, PAL_GOLD, {0, rgb(15, 13, 3), rgb(8, 5, 1), rgb(15, 15, 10)});
    pal(vdp, PAL_WARN, {0, rgb(15, 5, 3), rgb(6, 1, 1)});
    pal(vdp, PAL_GOOD, {0, rgb(6, 15, 6), rgb(1, 5, 1)});
    pal(vdp, PAL_WOOD, {0, rgb(4, 2, 1), rgb(12, 8, 3), rgb(8, 5, 2), rgb(15, 12, 6), rgb(6, 4, 2)});
    pal(vdp, PAL_DRY, {0, rgb(4, 3, 1), rgb(12, 9, 4), rgb(8, 6, 2), rgb(14, 11, 6)});
    pal(vdp, PAL_WET, {0, rgb(2, 3, 2), rgb(5, 8, 4), rgb(2, 5, 8), rgb(8, 12, 8)});
    pal(vdp, PAL_PLANT, {0, rgb(1, 3, 1), rgb(8, 14, 3), rgb(3, 9, 2), rgb(14, 4, 3), rgb(15, 12, 2), rgb(12, 6, 8)});
    pal(vdp, PAL_MAN, {0, rgb(3, 2, 1), rgb(14, 10, 6), rgb(8, 5, 3), rgb(3, 7, 13), rgb(2, 3, 6), rgb(14, 12, 4)});
    pal(vdp, PAL_SUN, {0, rgb(15, 15, 12), rgb(15, 11, 3), rgb(14, 14, 15), rgb(6, 6, 8)});
    pal(vdp, PAL_YARD, {0, rgb(4, 10, 3), rgb(2, 6, 2), rgb(9, 8, 5), rgb(6, 5, 3)});
    pal(vdp, PAL_WATER, {0, rgb(10, 15, 15), rgb(3, 9, 14), rgb(1, 4, 8)});
}

gs::Bitmap frameBmp() {
    gs::Bitmap b(60, 40);
    b.rect(1, 2, 58, 6, 2);
    b.rect(1, 2, 58, 2, 4);
    b.rect(1, 8, 5, 28, 3);
    b.rect(54, 8, 5, 28, 2);
    b.rect(1, 32, 58, 6, 3);
    b.rect(8, 10, 44, 3, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap soilBmp(bool wet) {
    gs::Bitmap b(44, 16);
    b.rect(1, 1, 42, 14, wet ? 2 : 2);
    b.ellipse(14, 8, 8, 4, wet ? 4 : 3);
    b.ellipse(30, 9, 6, 3, wet ? 3 : 4);
    b.rect(18, 5, 6, 2, wet ? 3 : 4);
    return b;
}

void plantOf(gs::Bitmap& b, int kind, int stage) {
    float s = stage == 0 ? 0.55f : stage == 1 ? 1.f : 0.7f;
    int leaf = stage == 2 ? 3 : 2;
    int dark = stage == 2 ? 1 : 3;
    float base = 28.f;
    float cx = 20.f;
    auto y = [&](float yy) { return base - (base - yy) * s; };
    if (kind == 0) {
        b.ellipse(cx, y(16), 10 * s, 6 * s, leaf);
        b.ellipse(cx - 7, y(20), 5 * s, 3 * s, dark);
        b.ellipse(cx + 7, y(20), 5 * s, 3 * s, dark);
        if (stage == 1) b.ellipse(cx, y(12), 3, 3, 5);
    } else if (kind == 1) {
        b.line(cx, base, cx, y(10), dark, 2.f);
        b.ellipse(cx - 6, y(16), 4 * s, 4 * s, stage == 2 ? 1 : 4);
        b.ellipse(cx + 6, y(18), 3.5f * s, 3.5f * s, stage == 2 ? 1 : 4);
        if (stage == 1) b.ellipse(cx, y(12), 3, 3, 5);
    } else {
        for (int i = 0; i < 5; i++) {
            float x = cx - 8 + i * 4;
            b.line(x, base, x, base - (10.f + (i % 3) * 3.f) * s, i & 1 ? leaf : dark, 1.5f);
        }
        if (stage == 1) b.ellipse(cx, y(12), 3, 2, 6);
    }
}

gs::Bitmap plantBmp(int kind, int stage) {
    gs::Bitmap b(40, 32);
    plantOf(b, kind, stage);
    return b;
}

gs::Bitmap manBmp(int step) {
    gs::Bitmap b(24, 26);
    b.ellipse(12, 6, 4, 4, 2);
    b.rect(9, 11, 6, 8, 4);
    b.rect(6, 12, 3, 6, 3);
    b.rect(15, 12, 3, 6, 3);
    int ox = step ? 1 : 0;
    b.rect(9, 19, 3, 6, 5);
    b.rect(13 + ox, 19, 3, 6, 6);
    b.rect(8, 24, 4, 2, 1);
    b.rect(13, 24, 4, 2, 1);
    return b;
}

gs::Bitmap stakeBmp(bool up) {
    gs::Bitmap b(10, up ? 28 : 18);
    int top = up ? 2 : 6;
    b.rect(4, top, 2, b.h - top - 1, 2);
    b.rect(2, top, 6, 3, 1);
    b.rect(3, top + 3, 4, 2, 3);
    return b;
}

gs::Bitmap dropBmp() {
    gs::Bitmap b(6, 8);
    b.ellipse(3, 4, 2, 3, 1);
    b.set(3, 1, 2);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1);
    b.line(0, 3, 6, 3, 1, 1);
    b.set(3, 3, 3);
    return b;
}

gs::Bitmap sunBmp() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 6, 6, 1);
    b.ellipse(11, 11, 4, 4, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        b.line(11 + std::cos(a) * 7, 11 + std::sin(a) * 7, 11 + std::cos(a) * 10, 11 + std::sin(a) * 10, 1, 1);
    }
    return b;
}

gs::Bitmap cloudBmp() {
    gs::Bitmap b(36, 14);
    b.ellipse(12, 8, 8, 5, 3);
    b.ellipse(22, 7, 10, 5, 1);
    b.ellipse(28, 9, 6, 4, 3);
    return b;
}

void grassTile(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) px[y * 8 + x] = ((x + y * 3) % 7 == 0) ? 2 : 1;
    px[3 * 8 + 5] = 3;
    a.grass = tiles.alloc(1);
    vdp.loadTile(a.grass, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paint(vdp);
    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    grassTile(vdp, art, tiles);
    art.frame = gs::uploadMipped(vdp, frameBmp());
    art.soil = gs::uploadMipped(vdp, soilBmp(false));
    for (int k = 0; k < 3; k++)
        for (int s = 0; s < 3; s++) art.plant[k][s] = gs::uploadMipped(vdp, plantBmp(k, s));
    art.man[0] = gs::uploadMipped(vdp, manBmp(0));
    art.man[1] = gs::uploadMipped(vdp, manBmp(1));
    art.stake = gs::uploadMipped(vdp, stakeBmp(false));
    art.stakeUp = gs::uploadMipped(vdp, stakeBmp(true));
    art.drop = gs::uploadMipped(vdp, dropBmp());
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.sun = gs::uploadMipped(vdp, sunBmp());
    art.cloud = gs::uploadMipped(vdp, cloudBmp());
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.shadow = 2;
    art.title = gs::uploadMipped(vdp, gs::textBitmap("BEDSMARK", st));
    st.color = 1;
    art.done = gs::uploadMipped(vdp, gs::textBitmap("FINISHED MARK", st));
}

}  // namespace bedsmark
