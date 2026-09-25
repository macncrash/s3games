#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace tug {
namespace {

constexpr float kPi = 3.14159265f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

Bitmap rotateUpright(const Bitmap& src, float heading) {
    Bitmap dst(src.w, src.h);
    const float theta = kPi * 0.5f - heading;
    const float c = std::cos(theta), s = std::sin(theta);
    const float cx = (src.w - 1) * 0.5f, cy = (src.h - 1) * 0.5f;
    for (int y = 0; y < dst.h; y++) {
        for (int x = 0; x < dst.w; x++) {
            float dx = x - cx, dy = y - cy;
            int sx = int(std::lround(cx + dx * c + dy * s));
            int sy = int(std::lround(cy - dx * s + dy * c));
            int p = src.get(sx, sy);
            if (p) dst.set(x, y, p);
        }
    }
    dst.outline(15, false);
    return dst;
}

Bitmap paintShip() {
    Bitmap b(120, 120);
    b.poly({{60, 16}, {76, 38}, {78, 62}, {74, 98}, {60, 106}, {46, 98}, {42, 62}, {44, 38}}, 1);
    b.poly({{60, 24}, {70, 40}, {72, 96}, {60, 100}, {48, 96}, {50, 40}}, 2);
    b.poly({{60, 32}, {66, 44}, {66, 90}, {60, 94}, {54, 90}, {54, 44}}, 3);
    b.rect(55, 46, 10, 12, 13);
    b.rect(55, 60, 10, 12, 14);
    b.rect(55, 74, 10, 10, 10);
    b.rect(53, 84, 14, 12, 5);
    b.rect(55, 86, 10, 5, 6);
    b.poly({{60, 18}, {67, 32}, {53, 32}}, 12);
    b.ellipse(60, 88, 2, 2, 12);
    b.poly({{42, 68}, {48, 64}, {48, 100}, {36, 100}, {33, 92}, {33, 76}}, 7);
    b.rect(35, 76, 10, 10, 8);
    b.rect(37, 78, 6, 5, 6);
    b.rect(34, 96, 12, 3, 9);
    b.rect(46, 72, 4, 3, 9);
    b.rect(46, 90, 4, 3, 9);
    b.line(39, 70, 50, 70, 11, 1.4f);
    gs::TextStyle st{1, 12, 2, 0, 1};
    Bitmap label = gs::textBitmap("S3", st);
    b.blit(label, 60 - label.w / 2, 50);
    b.outline(15, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(28, 28);
    b.ellipse(10, 17, 6, 6, 1);
    b.ellipse(18, 17, 6, 6, 1);
    b.ellipse(14, 11, 6, 6, 1);
    b.ellipse(10, 16, 3, 3, 2);
    b.ellipse(18, 16, 3, 3, 2);
    b.ellipse(14, 10, 3, 3, 3);
    b.ellipse(14, 8, 2, 2, 4);
    b.outline(2, false);
    return b;
}

Bitmap dockArt() {
    Bitmap b(48, 36);
    b.rect(0, 0, 48, 36, 1);
    b.rect(0, 0, 6, 36, 8);
    for (int i = 0; i < 4; i++) b.ellipse(3, 5 + i * 8, 2.2f, 2.2f, 7);
    b.rect(14, 5, 16, 11, 3);
    b.rect(16, 7, 12, 7, 4);
    b.rect(30, 18, 12, 12, 3);
    b.rect(32, 20, 8, 8, 4);
    b.rect(16, 20, 10, 10, 6);
    b.line(10, 2, 10, 34, 2, 1.2f);
    b.outline(2, false);
    return b;
}

Bitmap craneArt() {
    Bitmap b(44, 52);
    b.rect(18, 28, 10, 18, 1);
    b.rect(4, 18, 34, 6, 5);
    b.rect(8, 24, 3, 16, 7);
    b.poly({{6, 40}, {14, 40}, {10, 48}}, 3);
    b.rect(14, 42, 18, 6, 2);
    b.rect(30, 16, 4, 8, 5);
    b.outline(2, false);
    return b;
}

Bitmap dashArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 2.6f, 2.6f, 1);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 3.5f, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 3.5f, 2, 1);
    float tip = up ? 2.f : 13.f;
    b.line(14, 8, 2, tip, 1, 1.5f);
    b.line(14, 8, 26, tip, 2, 1.5f);
    b.set(18, 8, 3);
    return b;
}

Bitmap dotArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 2.4f, 2.4f, 1);
    return b;
}

Bitmap panelArt() {
    Bitmap b(86, 80);
    for (int x = 0; x < 86; x++) {
        b.set(x, 0, 1);
        b.set(x, 1, 2);
        b.set(x, 78, 2);
        b.set(x, 79, 1);
    }
    for (int y = 0; y < 80; y++) {
        b.set(0, y, 1);
        b.set(1, y, 2);
        b.set(84, y, 2);
        b.set(85, y, 1);
    }
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
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12), gs::rgb4(15, 13, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHIP,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(7, 2, 2), gs::rgb4(13, 11, 8), gs::rgb4(6, 6, 7), gs::rgb4(14, 14, 15),
            gs::rgb4(3, 8, 12), gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(10, 6, 3),
            gs::rgb4(2, 6, 8), gs::rgb4(15, 15, 15), gs::rgb4(3, 10, 5), gs::rgb4(3, 6, 12), shadow});
    setPal(vdp, PAL_PILE, {0, gs::rgb4(11, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 8), gs::rgb4(13, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(11, 7, 3), gs::rgb4(13, 10, 5), gs::rgb4(14, 11, 2),
                           gs::rgb4(3, 5, 8), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(6, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LANE, {0, gs::rgb4(8, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 7), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(12, 14, 15), gs::rgb4(3, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CRANE, {0, gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 6), gs::rgb4(15, 12, 2),
                            0, gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    Bitmap upright = paintShip();
    for (int i = 0; i < 16; i++) {
        float h = i * (kPi * 2.f) / 16.f;
        art.ship[i] = gs::uploadMipped(vdp, rotateUpright(upright, h));
    }
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.dock = gs::uploadMipped(vdp, dockArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.dash = gs::uploadMipped(vdp, dashArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(false));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(true));
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.panel = gs::uploadMipped(vdp, panelArt());
    art.title = words(vdp, "S3 TUG", 4, 1, 2);
    art.berthed = words(vdp, "BERTHED", 3, 1, 2);
    art.piling = words(vdp, "PILING", 3, 1, 2);
}

}  // namespace tug
