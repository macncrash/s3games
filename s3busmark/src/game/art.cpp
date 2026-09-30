#include "art.h"

#include <cmath>

namespace busmark {
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

Bitmap paintBus() {
    Bitmap b(28, 72);
    b.rect(5, 6, 18, 60, 1);
    b.rect(7, 2, 14, 8, 2);
    b.rect(8, 3, 12, 4, 7);
    b.rect(7, 12, 14, 6, 4);
    b.rect(8, 20, 5, 8, 4);
    b.rect(15, 20, 5, 8, 4);
    b.rect(8, 30, 5, 8, 4);
    b.rect(15, 30, 5, 8, 4);
    b.rect(8, 40, 5, 7, 4);
    b.rect(15, 40, 5, 7, 4);
    b.rect(9, 50, 10, 10, 5);
    b.rect(11, 53, 6, 6, 8);
    b.rect(2, 16, 4, 8, 3);
    b.rect(22, 16, 4, 8, 3);
    b.rect(2, 46, 4, 8, 3);
    b.rect(22, 46, 4, 8, 3);
    b.rect(3, 18, 2, 4, 6);
    b.rect(23, 18, 2, 4, 6);
    b.rect(3, 48, 2, 4, 6);
    b.rect(23, 48, 2, 4, 6);
    b.rect(6, 10, 16, 2, 9);
    b.rect(10, 62, 8, 3, 10);
    b.ellipse(14, 4, 4, 2, 11);
    b.outline(12, false);
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
    Bitmap b(24, 10);
    b.ellipse(12, 5, 11, 3, 1);
    return b;
}

Bitmap paintBlock(int kind) {
    Bitmap b(30, 44);
    int wall = kind == 0 ? 1 : kind == 1 ? 2 : 4;
    b.rect(2, 10, 26, 34, wall);
    b.poly({{2, 10}, {15, 1}, {28, 10}}, 5);
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            int lit = ((r * 2 + c + kind) % 3) == 0 ? 3 : 7;
            b.rect(5 + c * 8, 14 + r * 8, 5, 5, lit);
        }
    }
    b.rect(11, 34, 8, 10, 8);
    return b;
}

Bitmap paintShelter() {
    Bitmap b(36, 28);
    b.rect(2, 8, 32, 3, 1);
    b.rect(3, 11, 2, 14, 2);
    b.rect(31, 11, 2, 14, 2);
    b.rect(4, 12, 26, 10, 3);
    b.rect(6, 22, 24, 3, 4);
    b.rect(14, 4, 8, 4, 5);
    return b;
}

Bitmap paintBench() {
    Bitmap b(22, 10);
    b.rect(1, 3, 20, 3, 1);
    b.rect(2, 6, 2, 4, 2);
    b.rect(18, 6, 2, 4, 2);
    return b;
}

Bitmap paintAsphalt() {
    Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 7, 16, 2, 2);
    b.set(4, 3, 3);
    b.set(12, 13, 3);
    return b;
}

Bitmap paintWalk() {
    Bitmap b(12, 12);
    b.rect(0, 0, 12, 12, 1);
    b.rect(1, 1, 4, 4, 2);
    b.rect(7, 7, 4, 4, 2);
    return b;
}

Bitmap paintBar() {
    Bitmap b(20, 28);
    b.rect(2, 4, 16, 20, 1);
    b.rect(6, 8, 8, 12, 2);
    b.rect(8, 2, 4, 24, 3);
    return b;
}

Bitmap paintRing() {
    Bitmap b(32, 48);
    b.rect(2, 2, 28, 44, 1);
    b.rect(6, 6, 20, 36, 0);
    b.rect(2, 20, 28, 4, 2);
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
    Bitmap b(18, 26);
    b.rect(8, 14, 3, 12, 2);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(7, 7, 3, 2, 3);
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
    setPal(vdp, PAL_BUS,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(9, 7, 1), gs::rgb4(1, 1, 1), gs::rgb4(6, 10, 13), gs::rgb4(3, 5, 6),
            gs::rgb4(13, 13, 12), gs::rgb4(15, 15, 13), gs::rgb4(10, 8, 3), gs::rgb4(12, 2, 2), gs::rgb4(15, 12, 3),
            gs::rgb4(15, 15, 8), gs::rgb4(2, 2, 2), 0, 0, 0});
    setPal(vdp, PAL_BLOCK,
           {0, gs::rgb4(10, 6, 5), gs::rgb4(7, 8, 10), gs::rgb4(14, 12, 6), gs::rgb4(5, 6, 7), gs::rgb4(4, 3, 3),
            gs::rgb4(13, 11, 8), gs::rgb4(2, 2, 4), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_WALK, {0, gs::rgb4(7, 7, 6), gs::rgb4(11, 11, 9), gs::rgb4(5, 5, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 3), gs::rgb4(15, 9, 1), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_END, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 8), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 7), gs::rgb4(5, 5, 5)});
    setPal(vdp, PAL_STOP, {0, gs::rgb4(4, 8, 11), gs::rgb4(2, 3, 4), gs::rgb4(10, 13, 15), gs::rgb4(6, 5, 3),
                           gs::rgb4(14, 12, 2), gs::rgb4(2, 7, 3)});

    Bitmap bus = paintBus();
    const float step = 6.28318530718f / 16.f;
    for (int i = 0; i < 16; i++) art.bus[i] = gs::uploadMipped(vdp, i ? spin(bus, step * float(i)) : bus);
    art.shade = gs::uploadMipped(vdp, paintShade());
    for (int i = 0; i < 3; i++) art.block[i] = gs::uploadMipped(vdp, paintBlock(i));
    art.shelter = gs::uploadMipped(vdp, paintShelter());
    art.bench = gs::uploadMipped(vdp, paintBench());
    art.asphalt = gs::uploadMipped(vdp, paintAsphalt());
    art.walk = gs::uploadMipped(vdp, paintWalk());
    art.bar = gs::uploadMipped(vdp, paintBar());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.endbar = gs::uploadMipped(vdp, paintEnd());
    art.tree = gs::uploadMipped(vdp, paintTree());
    loadFont(vdp, art);
    art.title = words(vdp, "BUS MARK", 3, 1);
    art.set = words(vdp, "SET DOWN", 2, 1);
    art.missed = words(vdp, "MISSED THE END", 2, 1);
    art.hold = words(vdp, "HOLD", 2, 1);
    art.start = words(vdp, "START", 2, 1);
}

}  // namespace busmark
