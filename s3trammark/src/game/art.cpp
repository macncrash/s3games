#include "art.h"

#include <cmath>

namespace trammark {
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

Bitmap paintTram() {
    Bitmap b(36, 72);
    b.rect(8, 6, 20, 60, 1);
    b.rect(10, 2, 16, 8, 2);
    b.rect(12, 0, 12, 4, 7);
    b.rect(16, 0, 4, 3, 8);
    b.rect(11, 12, 14, 10, 4);
    b.rect(11, 26, 14, 8, 5);
    b.rect(11, 38, 14, 10, 4);
    b.rect(11, 52, 14, 8, 4);
    b.rect(6, 16, 3, 8, 3);
    b.rect(27, 16, 3, 8, 3);
    b.rect(6, 44, 3, 8, 3);
    b.rect(27, 44, 3, 8, 3);
    b.rect(10, 18, 4, 4, 9);
    b.rect(22, 46, 4, 4, 9);
    b.rect(14, 62, 8, 4, 6);
    b.rect(15, 66, 6, 3, 10);
    b.ellipse(18, 8, 4, 2, 11);
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
    Bitmap b(40, 14);
    b.ellipse(20, 7, 18, 5, 1);
    return b;
}

Bitmap paintBlock(int kind) {
    Bitmap b(32, 44);
    int wall = kind == 0 ? 1 : kind == 1 ? 2 : 4;
    b.rect(2, 10, 28, 34, wall);
    b.poly({{2, 10}, {16, 1}, {30, 10}}, 5);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int lit = ((r + c + kind) & 1) ? 3 : 6;
            b.rect(5 + c * 8, 14 + r * 8, 5, 5, lit);
        }
    }
    return b;
}

Bitmap paintPole() {
    Bitmap b(16, 40);
    b.rect(7, 8, 2, 28, 2);
    b.rect(2, 8, 12, 2, 1);
    b.ellipse(8, 6, 3, 2, 3);
    b.rect(4, 36, 8, 3, 2);
    return b;
}

Bitmap paintBallast() {
    Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.set(2, 4, 2);
    b.set(9, 11, 2);
    b.set(13, 3, 3);
    b.set(5, 13, 3);
    return b;
}

Bitmap paintSleeper() {
    Bitmap b(16, 6);
    b.rect(0, 1, 16, 4, 1);
    b.rect(0, 2, 16, 1, 2);
    return b;
}

Bitmap paintRail() {
    Bitmap b(4, 16);
    b.rect(1, 0, 2, 16, 1);
    return b;
}

Bitmap paintMark() {
    Bitmap b(24, 24);
    b.poly({{12, 1}, {23, 12}, {12, 23}, {1, 12}}, 1);
    b.poly({{12, 6}, {18, 12}, {12, 18}, {6, 12}}, 2);
    b.rect(10, 4, 4, 16, 3);
    return b;
}

Bitmap paintRing() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 1);
    b.ellipse(18, 18, 11, 11, 0);
    return b;
}

Bitmap paintBuffer() {
    Bitmap b(16, 10);
    b.rect(0, 0, 16, 10, 1);
    b.rect(0, 0, 8, 5, 2);
    b.rect(8, 5, 8, 5, 2);
    return b;
}

Bitmap paintTree() {
    Bitmap b(18, 26);
    b.rect(8, 14, 3, 12, 2);
    b.ellipse(9, 9, 8, 8, 1);
    return b;
}

Bitmap paintPlat() {
    Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 0, 16, 3, 2);
    b.rect(2, 7, 5, 5, 3);
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
    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 2), gs::rgb4(6, 10, 13), gs::rgb4(3, 5, 7),
            gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 9), gs::rgb4(14, 12, 3), gs::rgb4(15, 12, 2), gs::rgb4(11, 11, 12),
            gs::rgb4(15, 15, 8), 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TOWN,
           {0, gs::rgb4(10, 7, 5), gs::rgb4(7, 8, 10), gs::rgb4(15, 13, 6), gs::rgb4(6, 6, 8), gs::rgb4(4, 3, 3),
            gs::rgb4(12, 11, 9), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_BALLAST, {0, gs::rgb4(7, 7, 6), gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 3), gs::rgb4(15, 9, 1), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_END, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(10, 10, 11), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(3, 8, 4), gs::rgb4(5, 5, 6), gs::rgb4(14, 14, 8)});

    Bitmap body = paintTram();
    const float step = 6.28318530718f / 16.f;
    for (int i = 0; i < 16; i++) art.tram[i] = gs::uploadMipped(vdp, i ? spin(body, step * float(i)) : body);
    art.shade = gs::uploadMipped(vdp, paintShade());
    for (int i = 0; i < 3; i++) art.block[i] = gs::uploadMipped(vdp, paintBlock(i));
    art.pole = gs::uploadMipped(vdp, paintPole());
    art.ballast = gs::uploadMipped(vdp, paintBallast());
    art.sleeper = gs::uploadMipped(vdp, paintSleeper());
    art.rail = gs::uploadMipped(vdp, paintRail());
    art.mark = gs::uploadMipped(vdp, paintMark());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.buffer = gs::uploadMipped(vdp, paintBuffer());
    art.tree = gs::uploadMipped(vdp, paintTree());
    art.plat = gs::uploadMipped(vdp, paintPlat());
    loadFont(vdp, art);
    art.title = words(vdp, "TRAM MARK", 3, 1);
    art.set = words(vdp, "SET DOWN", 2, 1);
    art.missed = words(vdp, "MISSED THE END", 2, 1);
    art.hold = words(vdp, "HOLD", 2, 1);
    art.start = words(vdp, "START", 2, 1);
}

}  // namespace trammark
