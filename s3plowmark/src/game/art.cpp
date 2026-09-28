#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace plow {
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

gs::Bitmap truckArt() {
    gs::Bitmap b(110, 48);
    b.poly({{8.f, 30.f}, {18.f, 18.f}, {46.f, 16.f}, {58.f, 26.f}, {96.f, 26.f}, {100.f, 34.f}, {8.f, 36.f}}, 2);
    b.rect(8, 32, 92, 6, 1);
    b.poly({{20.f, 20.f}, {44.f, 18.f}, {52.f, 26.f}, {20.f, 28.f}}, 3);
    b.rect(62, 22, 8, 6, 6);
    b.rect(78, 22, 6, 5, 6);
    b.rect(4, 30, 6, 6, 8);
    b.ellipse(88, 12, 4.f, 4.f, 7);
    b.rect(86, 14, 4, 12, 1);
    b.rect(96, 28, 8, 4, 1);
    b.ellipse(28, 40, 8.f, 8.f, 4);
    b.ellipse(28, 40, 3.2f, 3.2f, 5);
    b.ellipse(78, 40, 8.f, 8.f, 4);
    b.ellipse(78, 40, 3.2f, 3.2f, 5);
    b.rect(14, 34, 70, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap bladeArt(int frame) {
    gs::Bitmap b(64, 56);
    float drop = float(frame) / 4.f;
    float ang = -1.15f + drop * 1.35f;
    float c = std::cos(ang), s = std::sin(ang);
    auto rot = [&](float x, float y) {
        float lx = x - 10.f, ly = y - 10.f;
        return gs::Pt{10.f + lx * c - ly * s, 10.f + lx * s + ly * c};
    };
    gs::Pt a = rot(10, 10), d = rot(52, 10);
    b.line(a.first, a.second, d.first, d.second, 1, 3.2f);
    gs::Pt p0 = rot(40, 2), p1 = rot(62, 2), p2 = rot(62, 20), p3 = rot(40, 20);
    b.poly({p0, p1, p2, p3}, 2);
    b.line(p0.first, p0.second, p1.first, p1.second, 6, 2.f);
    b.ellipse(a.first, a.second, 3.f, 3.f, 5);
    b.rect(6, 8, 6, 4, 1);
    return b;
}

gs::Bitmap paintArt() {
    gs::Bitmap b(48, 14);
    b.rect(0, 2, 48, 10, 1);
    b.rect(20, 0, 8, 14, 2);
    b.rect(0, 2, 48, 2, 3);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(18, 40);
    b.rect(8, 12, 2, 28, 1);
    b.poly({{10.f, 12.f}, {17.f, 16.f}, {10.f, 20.f}}, 2);
    b.ellipse(9, 8, 3.f, 3.f, 3);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(40, 64);
    b.rect(18, 40, 5, 22, 1);
    b.poly({{20.f, 2.f}, {36.f, 28.f}, {4.f, 28.f}}, 2);
    b.poly({{20.f, 14.f}, {38.f, 42.f}, {2.f, 42.f}}, 3);
    b.poly({{20.f, 4.f}, {28.f, 16.f}, {14.f, 14.f}}, 4);
    return b;
}

gs::Bitmap barnArt() {
    gs::Bitmap b(70, 48);
    b.rect(6, 18, 58, 28, 1);
    b.poly({{4.f, 20.f}, {35.f, 4.f}, {66.f, 20.f}}, 2);
    b.rect(30, 28, 12, 18, 4);
    b.rect(12, 24, 10, 8, 3);
    b.rect(48, 24, 10, 8, 3);
    b.rect(8, 42, 54, 3, 9);
    return b;
}

gs::Bitmap fenceArt() {
    gs::Bitmap b(16, 28);
    b.rect(6, 4, 3, 24, 1);
    b.rect(2, 8, 12, 2, 2);
    b.rect(2, 16, 12, 2, 2);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(28, 18);
    b.ellipse(8, 12, 6.f, 4.f, 1);
    b.ellipse(16, 8, 7.f, 5.f, 2);
    b.ellipse(22, 13, 4.f, 3.f, 3);
    return b;
}

gs::Bitmap flakeArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 11, 3), gs::rgb4(8, 14, 10), gs::rgb4(14, 4, 3),
                          gs::rgb4(8, 12, 15), gs::rgb4(10, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 5)});
    setPal(vdp, PAL_TRUCK,
           {0, gs::rgb4(3, 4, 5), gs::rgb4(14, 11, 1), gs::rgb4(6, 10, 13), gs::rgb4(1, 1, 2), gs::rgb4(8, 8, 7),
            gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 1), gs::rgb4(13, 2, 2), gs::rgb4(13, 13, 14), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 15), gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(5, 3, 1), gs::rgb4(2, 6, 2), gs::rgb4(3, 8, 3), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 7, 1), gs::rgb4(15, 15, 14), gs::rgb4(10, 4, 1), gs::rgb4(15, 12, 2)});
    setPal(vdp, PAL_BARN, {0, gs::rgb4(11, 3, 2), gs::rgb4(6, 2, 2), gs::rgb4(12, 13, 14), gs::rgb4(3, 2, 1),
                           gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_FLAKE, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(9, 11, 13)});

    art.truck = gs::uploadMipped(vdp, truckArt());
    for (int i = 0; i < 5; i++) art.blade[i] = gs::uploadMipped(vdp, bladeArt(i));
    art.paint = gs::uploadMipped(vdp, paintArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.barn = gs::uploadMipped(vdp, barnArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.flake = gs::uploadMipped(vdp, flakeArt());
    art.word = words(vdp, "MARK", 2, 2, 3);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(10, 12, 14));
}

}  // namespace plow
