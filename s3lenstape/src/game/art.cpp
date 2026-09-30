#include "game/art.h"

namespace lenstape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, int* font) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 0; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
        font[c] = tiles.shared(px);
    }
}

gs::Image disc(gs::VDP& vdp, int rx, int ry, bool hole, bool wedge) {
    gs::Bitmap b(rx * 2 + 4, ry * 2 + 4);
    b.ellipse(float(rx + 2), float(ry + 2), float(rx), float(ry), 2);
    b.ellipse(float(rx + 2), float(ry + 1), float(rx) * 0.55f, float(ry) * 0.45f, 3);
    if (hole) b.ellipse(float(rx + 2), float(ry + 2), float(rx) * 0.22f, float(ry) * 0.28f, 1);
    if (wedge) {
        b.poly({{2.f, float(ry)}, {float(rx), 2.f}, {float(rx + 4), float(ry + 2)}, {float(rx), float(ry * 2)}}, 1);
    }
    return gs::uploadImage(vdp, b);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    loadFont(vdp, art.font);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(13, 14, 15)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(5, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(14, 3, 4)});
    setPal(vdp, PAL_BENCH, {0, gs::rgb4(3, 2, 2), gs::rgb4(7, 5, 3), gs::rgb4(11, 8, 5), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_CROWN, {0, gs::rgb4(2, 4, 8), gs::rgb4(6, 10, 14), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_FLINT, {0, gs::rgb4(6, 3, 1), gs::rgb4(13, 8, 2), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_MENISCUS, {0, gs::rgb4(1, 5, 3), gs::rgb4(4, 11, 7), gs::rgb4(10, 15, 11)});
    setPal(vdp, PAL_PRISM, {0, gs::rgb4(5, 2, 6), gs::rgb4(11, 4, 12), gs::rgb4(15, 10, 14)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(8, 7, 5), gs::rgb4(14, 13, 10), gs::rgb4(4, 3, 2)});

    {
        gs::Bitmap b(220, 28);
        b.rect(4, 6, 212, 16, 2);
        b.rect(8, 8, 204, 8, 3);
        b.rect(0, 18, 16, 8, 1);
        b.rect(204, 18, 16, 8, 1);
        b.line(20, 12, 200, 12, 1, 1.f);
        art.bench = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(8, 36);
        b.rect(3, 0, 2, 36, 3);
        b.rect(1, 14, 6, 8, 2);
        art.gate = gs::uploadImage(vdp, b);
    }
    art.crown = disc(vdp, 16, 16, false, false);
    art.flint = disc(vdp, 12, 14, true, false);
    art.meniscus = disc(vdp, 18, 8, false, false);
    art.prism = disc(vdp, 14, 14, false, true);
    {
        gs::Bitmap b(48, 22);
        b.rect(2, 2, 44, 16, 2);
        b.rect(6, 6, 36, 8, 1);
        b.rect(2, 16, 44, 4, 3);
        art.tray = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(10, 14);
        b.rect(3, 2, 4, 6, 2);
        b.poly({{1, 8}, {9, 8}, {7, 13}, {3, 13}}, 1);
        art.lamp = gs::uploadImage(vdp, b);
    }
}

}  // namespace lenstape
