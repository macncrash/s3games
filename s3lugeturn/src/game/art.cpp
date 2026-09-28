#include "game/art.h"

#include <cmath>

namespace luge {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap podArt(int lean) {
    Bitmap b(80, 48);
    const float k = float(lean);
    b.ellipse(40, 40, 30, 6, 3);
    b.poly({{12, 34}, {68, 34}, {72 + k * 2.f, 42}, {8 + k * 2.f, 42}}, 4);
    b.poly({{18, 28}, {62, 28}, {66, 36}, {14, 36}}, 2);
    b.ellipse(28 + k * 4.f, 20, 10, 8, 6);
    b.ellipse(30 + k * 4.f, 19, 6, 4, 7);
    b.ellipse(48 + k * 3.f, 22, 8, 5, 5);
    b.line(20 + k, 26, 58 + k * 2.f, 18, 8, 2.2f);
    b.line(22, 40, 22, 34, 5, 1.6f);
    b.line(58, 40, 58, 34, 5, 1.6f);
    b.outline(1, false);
    return b;
}

Bitmap gateArt() {
    Bitmap b(28, 64);
    b.rect(12, 8, 4, 52, 2);
    b.poly({{4, 6}, {24, 6}, {22, 22}, {6, 22}}, 4);
    b.poly({{6, 8}, {16, 8}, {15, 20}, {7, 20}}, 5);
    b.ellipse(14, 60, 6, 3, 3);
    b.outline(1, false);
    return b;
}

Bitmap pineArt() {
    Bitmap b(36, 56);
    b.rect(16, 40, 4, 14, 2);
    b.poly({{18, 6}, {34, 42}, {2, 42}}, 3);
    b.poly({{18, 16}, {30, 36}, {6, 36}}, 4);
    b.outline(1, false);
    return b;
}

Bitmap sprayArt() {
    Bitmap b(20, 12);
    b.ellipse(8, 7, 6, 3, 1);
    b.ellipse(14, 5, 4, 2, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
            }
        }
        a.font[c - 32] = tiles.shared(px);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.reset();
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(2, 3, 6), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(10, 13, 15), gs::rgb4(6, 9, 13), gs::rgb4(14, 15, 15), gs::rgb4(3, 5, 9)});
    setPal(vdp, PAL_POD, {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 2, 2), gs::rgb4(4, 1, 1), gs::rgb4(8, 8, 9),
                          gs::rgb4(14, 12, 8), gs::rgb4(13, 8, 5), gs::rgb4(15, 13, 11), gs::rgb4(6, 2, 2)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(1, 1, 2), gs::rgb4(9, 9, 10), gs::rgb4(3, 3, 4), gs::rgb4(14, 3, 2),
                           gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 2, 1), gs::rgb4(4, 3, 2), gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 5)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(13, 14, 15), gs::rgb4(8, 11, 14)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(11, 13, 15), gs::rgb4(7, 10, 13), gs::rgb4(14, 15, 15), gs::rgb4(4, 6, 10),
                           gs::rgb4(13, 14, 15), gs::rgb4(9, 12, 14), gs::rgb4(15, 15, 15), gs::rgb4(5, 8, 12)});
    vdp.setFogColor(gs::rgb4(12, 13, 14));

    art.pod[0] = gs::uploadMipped(vdp, podArt(-1));
    art.pod[1] = gs::uploadMipped(vdp, podArt(0));
    art.pod[2] = gs::uploadMipped(vdp, podArt(1));
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    gs::TextStyle st{3, 1, 0, 0, 1};
    art.title = gs::uploadMipped(vdp, gs::textBitmap("LUGE TURN", st));
    loadFont(vdp, art);
}

}  // namespace luge
