#include "art.h"

#include <cmath>

namespace keelmark {
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

Bitmap paintHull() {
    Bitmap b(36, 64);
    b.ellipse(18, 34, 11, 26, 1);
    b.poly({{18, 2}, {26, 22}, {10, 22}}, 1);
    b.rect(12, 18, 12, 28, 2);
    b.poly({{18, 6}, {28, 28}, {18, 24}, {8, 28}}, 4);
    b.poly({{18, 8}, {24, 26}, {18, 22}, {12, 26}}, 5);
    b.rect(17, 10, 2, 36, 3);
    b.ellipse(18, 48, 4, 3, 6);
    b.rect(16, 46, 4, 6, 7);
    b.rect(8, 40, 4, 3, 8);
    b.rect(24, 40, 4, 3, 8);
    b.ellipse(18, 8, 2, 2, 9);
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

Bitmap paintBuoy() {
    Bitmap b(24, 32);
    b.rect(11, 16, 2, 14, 3);
    b.ellipse(12, 12, 8, 8, 1);
    b.ellipse(12, 11, 5, 5, 2);
    b.rect(11, 2, 2, 6, 4);
    b.poly({{11, 4}, {18, 6}, {11, 8}}, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(16, 28);
    b.rect(3, 4, 2, 22, 2);
    b.poly({{5, 5}, {14, 8}, {5, 12}}, 1);
    return b;
}

Bitmap paintWater() {
    Bitmap b(32, 32);
    b.rect(0, 0, 32, 32, 1);
    for (int y = 2; y < 32; y += 6) {
        for (int x = (y / 6) & 1 ? 2 : 8; x < 32; x += 14) b.rect(x, y, 6, 1, 2);
    }
    b.rect(4, 18, 3, 1, 3);
    b.rect(20, 10, 4, 1, 3);
    return b;
}

Bitmap paintBank() {
    Bitmap b(32, 32);
    b.rect(0, 0, 32, 32, 1);
    for (int i = 0; i < 12; i++) {
        int x = (i * 7) % 28;
        int y = (i * 11) % 28;
        b.ellipse(float(x + 2), float(y + 2), 3, 2, (i & 1) ? 2 : 3);
    }
    return b;
}

Bitmap paintRing() {
    Bitmap b(48, 48);
    b.ellipse(24, 24, 22, 22, 1);
    b.ellipse(24, 24, 16, 16, 0);
    b.ellipse(24, 24, 14, 14, 2);
    b.ellipse(24, 24, 10, 10, 0);
    return b;
}

Bitmap paintEnd() {
    Bitmap b(32, 8);
    for (int x = 0; x < 32; x += 8) {
        b.rect(x, 0, 4, 8, 1);
        b.rect(x + 4, 0, 4, 8, 2);
    }
    return b;
}

Bitmap paintReed() {
    Bitmap b(16, 28);
    b.line(4, 26, 3, 6, 1, 1.4f);
    b.line(8, 26, 9, 4, 2, 1.4f);
    b.line(12, 26, 14, 8, 1, 1.2f);
    b.ellipse(3, 6, 2, 3, 3);
    b.ellipse(10, 4, 2, 3, 3);
    return b;
}

Bitmap paintCommittee() {
    Bitmap b(28, 36);
    b.rect(2, 18, 24, 10, 1);
    b.poly({{2, 18}, {14, 8}, {26, 18}}, 2);
    b.rect(12, 22, 6, 8, 3);
    b.rect(13, 6, 2, 12, 4);
    b.poly({{15, 6}, {24, 9}, {15, 12}}, 5);
    b.rect(4, 26, 4, 8, 6);
    b.rect(20, 26, 4, 8, 6);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 4, 3), gs::rgb4(4, 3, 2), gs::rgb4(15, 15, 14), gs::rgb4(13, 14, 15),
            gs::rgb4(2, 5, 9), gs::rgb4(8, 10, 12), gs::rgb4(6, 3, 2), gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0,
            gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), gs::rgb4(2, 6, 2)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 11), gs::rgb4(4, 9, 14), gs::rgb4(8, 12, 15)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 2), gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 8),
                           gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_END, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 9), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(10, 13, 15), gs::rgb4(6, 9, 12)});
    setPal(vdp, PAL_REED,
           {0, gs::rgb4(4, 10, 3), gs::rgb4(7, 12, 4), gs::rgb4(10, 8, 3), gs::rgb4(6, 6, 5), gs::rgb4(14, 3, 2),
            gs::rgb4(3, 3, 2)});

    Bitmap hull = paintHull();
    const float step = 6.28318530718f / 16.f;
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, i ? spin(hull, step * float(i)) : hull);
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.water = gs::uploadMipped(vdp, paintWater());
    art.bank = gs::uploadMipped(vdp, paintBank());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.endbar = gs::uploadMipped(vdp, paintEnd());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.committee = gs::uploadMipped(vdp, paintCommittee());
    loadFont(vdp, art);
    art.title = words(vdp, "KEEL MARK", 3, 1);
    art.set = words(vdp, "SET DOWN", 2, 1);
    art.missed = words(vdp, "MISSED THE END", 2, 1);
    art.hold = words(vdp, "HOLD", 2, 1);
    art.start = words(vdp, "START", 2, 1);
}

}  // namespace keelmark
