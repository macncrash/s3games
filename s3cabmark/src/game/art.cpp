#include "art.h"

#include <cmath>

namespace cabmark {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintCab() {
    Bitmap b(40, 56);
    b.rect(8, 8, 24, 40, 1);
    b.rect(10, 4, 20, 10, 2);
    b.rect(12, 1, 16, 5, 7);
    b.rect(11, 14, 8, 8, 4);
    b.rect(21, 14, 8, 8, 4);
    b.rect(16, 16, 8, 6, 5);
    b.rect(6, 36, 6, 12, 3);
    b.rect(28, 36, 6, 12, 3);
    b.rect(7, 40, 4, 6, 6);
    b.rect(29, 40, 4, 6, 6);
    b.rect(4, 30, 5, 4, 9);
    b.rect(31, 30, 5, 4, 9);
    b.rect(12, 30, 16, 3, 3);
    b.rect(14, 44, 12, 3, 8);
    b.ellipse(20, 6, 5, 2, 10);
    b.outline(15, false);
    return b;
}

Bitmap spin(const Bitmap& src, float a) {
    Bitmap o(src.w, src.h);
    const float c = std::cos(a), s = std::sin(a);
    const float cx = src.w * 0.5f, cy = src.h * 0.5f;
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            int sx = int(std::floor(cx + dx * c + dy * s));
            int sy = int(std::floor(cy - dx * s + dy * c));
            int p = src.get(sx, sy);
            if (p) o.set(x, y, p);
        }
    }
    return o;
}

Bitmap paintShade() {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
    return b;
}

Bitmap paintBlock(int kind) {
    Bitmap b(32, 48);
    int wall = kind == 0 ? 1 : kind == 1 ? 2 : 4;
    int win = kind == 2 ? 6 : 3;
    b.rect(2, 8, 28, 40, wall);
    b.poly({{2, 8}, {16, 1}, {30, 8}}, 5);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int lit = ((r + c + kind) % 3) == 0 ? win : 7;
            b.rect(5 + c * 8, 14 + r * 9, 5, 6, lit);
        }
    }
    b.rect(12, 38, 8, 10, 8);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 32);
    b.rect(5, 8, 2, 20, 2);
    b.ellipse(6, 5, 4, 3, 1);
    b.ellipse(6, 5, 2, 1.4f, 3);
    b.rect(2, 28, 8, 3, 2);
    return b;
}

Bitmap paintAsphalt() {
    Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 7, 16, 2, 2);
    b.set(3, 3, 3);
    b.set(11, 12, 3);
    return b;
}

Bitmap paintWalk() {
    Bitmap b(12, 12);
    b.rect(0, 0, 12, 12, 1);
    b.rect(1, 1, 4, 4, 2);
    b.rect(7, 7, 4, 4, 2);
    return b;
}

Bitmap paintMark() {
    Bitmap b(28, 28);
    b.poly({{14, 2}, {26, 14}, {14, 26}, {2, 14}}, 1);
    b.poly({{14, 7}, {21, 14}, {14, 21}, {7, 14}}, 2);
    b.rect(12, 6, 4, 16, 3);
    b.rect(6, 12, 16, 4, 3);
    return b;
}

Bitmap paintRing() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 18, 18, 1);
    b.ellipse(20, 20, 13, 13, 0);
    return b;
}

Bitmap paintEnd() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 4, 4, 2);
    b.rect(4, 4, 4, 4, 2);
    return b;
}

Bitmap paintTree() {
    Bitmap b(20, 28);
    b.rect(9, 16, 3, 12, 2);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(8, 8, 3, 3, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int color) {
    gs::TextStyle st{scale, color, 2, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(15, 13, 2), gs::rgb4(12, 8, 1), gs::rgb4(1, 1, 1), gs::rgb4(7, 11, 14), gs::rgb4(3, 5, 7),
            gs::rgb4(14, 14, 12), gs::rgb4(15, 15, 14), gs::rgb4(4, 4, 5), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 8), 0,
            0, 0, 0, gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_BLOCK,
           {0, gs::rgb4(11, 6, 4), gs::rgb4(8, 8, 10), gs::rgb4(15, 13, 6), gs::rgb4(6, 7, 8), gs::rgb4(5, 3, 3),
            gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 4), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_WALK, {0, gs::rgb4(8, 8, 7), gs::rgb4(11, 11, 10), gs::rgb4(5, 5, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 4), gs::rgb4(15, 10, 1), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_END, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 6), gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(3, 9, 4), gs::rgb4(2, 4, 2), gs::rgb4(12, 14, 8)});

    Bitmap cab = paintCab();
    const float step = 6.28318530718f / 16.f;
    for (int i = 0; i < 16; i++) art.cab[i] = gs::uploadMipped(vdp, i ? spin(cab, step * float(i)) : cab);
    art.shade = gs::uploadMipped(vdp, paintShade());
    for (int i = 0; i < 3; i++) art.block[i] = gs::uploadMipped(vdp, paintBlock(i));
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.asphalt = gs::uploadMipped(vdp, paintAsphalt());
    art.walk = gs::uploadMipped(vdp, paintWalk());
    art.mark = gs::uploadMipped(vdp, paintMark());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.endbar = gs::uploadMipped(vdp, paintEnd());
    art.tree = gs::uploadMipped(vdp, paintTree());
    loadFont(vdp, art);
    art.title = words(vdp, "CAB MARK", 3, 1);
    art.set = words(vdp, "SET DOWN", 2, 1);
    art.missed = words(vdp, "MISSED THE END", 2, 1);
    art.hold = words(vdp, "HOLD", 2, 1);
    art.start = words(vdp, "START", 2, 1);
}

}  // namespace cabmark
