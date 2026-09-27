#include "game/art.h"

#include <string>

namespace ferrylane {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

gs::Bitmap ferryArt() {
    gs::Bitmap b(52, 86);
    b.poly({{26, 2}, {40, 22}, {38, 72}, {26, 82}, {14, 72}, {12, 22}}, 1);
    b.poly({{26, 8}, {34, 24}, {33, 40}, {19, 40}, {18, 24}}, 2);
    b.rect(20, 28, 12, 10, 4);
    b.rect(22, 44, 8, 16, 5);
    b.rect(16, 62, 20, 6, 3);
    b.ellipse(18, 18, 3, 3, 6);
    b.ellipse(34, 18, 3, 3, 6);
    b.line(26, 4, 26, 16, 7, 1.5f);
    b.outline(8, false);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(18, 28);
    b.rect(8, 12, 2, 14, 3);
    b.ellipse(9, 10, 7, 7, 1);
    b.ellipse(9, 10, 3, 3, 2);
    b.rect(7, 24, 4, 3, 4);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(64, 28);
    b.rect(0, 6, 64, 16, 1);
    for (int x = 2; x < 62; x += 8) b.rect(x, 8, 3, 12, 2);
    b.rect(0, 4, 64, 3, 3);
    b.rect(0, 21, 64, 3, 4);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(12, 9, 10, 6, 1);
    b.ellipse(24, 8, 12, 7, 1);
    b.ellipse(30, 10, 8, 5, 2);
    return b;
}

gs::Bitmap gullArt(int frame) {
    gs::Bitmap b(28, 12);
    float dip = frame ? 3.f : 0.f;
    b.poly({{1, 6}, {12, 4 + dip}, {14, 6}, {12, 7}}, 1);
    b.poly({{27, 6}, {16, 4 + dip}, {14, 6}, {16, 7}}, 1);
    b.ellipse(14, 6, 2, 2, 2);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(20, 10);
    b.ellipse(10, 5, 8, 3, 1);
    b.ellipse(6, 5, 3, 2, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 8));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(14, 14, 12), gs::rgb4(2, 8, 7), gs::rgb4(12, 3, 2), gs::rgb4(6, 12, 14),
                           gs::rgb4(4, 6, 5), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 14), gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(13, 8, 5), gs::rgb4(8, 3, 2), gs::rgb4(12, 4, 2), gs::rgb4(5, 8, 11),
                           gs::rgb4(4, 3, 2), gs::rgb4(15, 10, 3), gs::rgb4(14, 12, 9), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 8), gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(9, 8, 6), gs::rgb4(5, 4, 3), gs::rgb4(14, 12, 4), gs::rgb4(3, 5, 6)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 14, 15), gs::rgb4(9, 12, 14)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 14), gs::rgb4(6, 5, 4)});

    // Road bank 12: water channel, yellow verge, open water outside.
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(2, 6, 4), gs::rgb4(1, 4, 3), gs::rgb4(4, 8, 5), gs::rgb4(15, 12, 3), gs::rgb4(11, 8, 2),
            gs::rgb4(4, 8, 11), gs::rgb4(3, 6, 9), gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5), gs::rgb4(10, 10, 8),
            gs::rgb4(1, 4, 8), gs::rgb4(2, 6, 11), gs::rgb4(8, 12, 14), gs::rgb4(15, 14, 6), gs::rgb4(5, 9, 12)});

    loadFont(vdp, art);
    art.ferry = gs::uploadMipped(vdp, ferryArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.foam = gs::uploadMipped(vdp, foamArt());
}

}  // namespace ferrylane
