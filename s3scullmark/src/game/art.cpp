#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace scull {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
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
    gs::TextStyle st{scale, fill, edge, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

gs::Bitmap hullArt() {
    gs::Bitmap b(140, 26);
    b.poly({{2.f, 14.f}, {22.f, 9.f}, {128.f, 8.f}, {138.f, 13.f}, {128.f, 19.f}, {18.f, 20.f}}, 1);
    b.poly({{28.f, 10.f}, {124.f, 9.f}, {126.f, 14.f}, {28.f, 16.f}}, 2);
    b.rect(62, 4, 10, 12, 4);
    b.line(48, 8, 90, 6, 4, 2.f);
    b.ellipse(134, 13, 3.f, 2.2f, 3);
    b.rect(10, 14, 8, 3, 5);
    b.rect(58, 12, 18, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap rowerArt() {
    gs::Bitmap b(28, 32);
    b.ellipse(14, 7, 5.2f, 5.f, 8);
    b.rect(11, 12, 8, 10, 9);
    b.line(12, 16, 4, 22, 8, 2.2f);
    b.line(17, 16, 24, 22, 8, 2.2f);
    b.line(12, 21, 8, 30, 9, 2.4f);
    b.line(17, 21, 20, 30, 9, 2.4f);
    b.set(12, 6, 5);
    b.set(16, 6, 5);
    return b;
}

gs::Bitmap oarArt(float ang) {
    gs::Bitmap b(72, 72);
    const float c = std::cos(ang), s = std::sin(ang);
    auto rot = [&](float x, float y) {
        float lx = x - 10.f, ly = y - 36.f;
        return gs::Pt{10.f + lx * c - ly * s, 36.f + lx * s + ly * c};
    };
    gs::Pt a = rot(10, 36), d = rot(62, 36);
    b.line(a.first, a.second, d.first, d.second, 1, 2.4f);
    gs::Pt p0 = rot(54, 30), p1 = rot(70, 30), p2 = rot(70, 42), p3 = rot(54, 42);
    b.poly({p0, p1, p2, p3}, 2);
    b.ellipse(a.first, a.second, 2.2f, 2.2f, 3);
    return b;
}

gs::Bitmap paintArt() {
    gs::Bitmap b(36, 16);
    for (int x = 0; x < 36; x += 6) b.rect(x, 2, 3, 12, (x / 6) & 1 ? 1 : 2);
    b.rect(0, 2, 36, 2, 3);
    b.rect(0, 12, 36, 2, 3);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(16, 36);
    b.rect(7, 14, 2, 22, 1);
    b.ellipse(8, 10, 6.f, 7.f, 2);
    b.rect(6, 4, 4, 4, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(36, 52);
    b.rect(16, 28, 5, 22, 1);
    b.ellipse(18, 18, 14, 16, 2);
    b.ellipse(12, 16, 6, 5, 3);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(16, 28);
    b.line(4, 26, 6, 4, 1, 1.6f);
    b.line(8, 26, 7, 8, 2, 1.4f);
    b.line(12, 26, 11, 2, 1, 1.6f);
    b.ellipse(6, 4, 2.2f, 3.f, 3);
    return b;
}

gs::Bitmap rippleArt() {
    gs::Bitmap b(24, 8);
    b.line(1, 4, 8, 2, 1, 1.2f);
    b.line(8, 2, 16, 5, 1, 1.2f);
    b.line(16, 5, 23, 3, 2, 1.2f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 12, 6), gs::rgb4(8, 14, 10), gs::rgb4(14, 5, 4),
                          gs::rgb4(6, 10, 14), gs::rgb4(10, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(6, 10, 14), gs::rgb4(10, 13, 15), gs::rgb4(14, 14, 12), 0});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 6), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(14, 14, 13), gs::rgb4(11, 4, 3), gs::rgb4(4, 6, 8), gs::rgb4(2, 2, 3),
                           gs::rgb4(8, 8, 9), gs::rgb4(15, 12, 3), gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6),
                           gs::rgb4(6, 8, 12)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(4, 3, 1), gs::rgb4(3, 8, 3), gs::rgb4(6, 11, 4), gs::rgb4(8, 10, 5)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 12), gs::rgb4(13, 3, 2), gs::rgb4(15, 12, 2), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14), gs::rgb4(15, 15, 15)});

    art.hull = gs::uploadMipped(vdp, hullArt());
    art.rower = gs::uploadMipped(vdp, rowerArt());
    for (int i = 0; i < 9; i++) {
        float ang = -0.9f + float(i) * 0.22f;
        art.oar[i] = gs::uploadMipped(vdp, oarArt(ang));
    }
    art.paint = gs::uploadMipped(vdp, paintArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.ripple = gs::uploadMipped(vdp, rippleArt());
    art.setWord = words(vdp, "MARK", 2, 3, 4);
    art.endWord = words(vdp, "END", 2, 2, 4);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 8, 10));
}

}  // namespace scull
