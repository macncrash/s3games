#include "art.h"

namespace mushlane {
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

Bitmap paintDogs() {
    Bitmap b(40, 52);
    for (int i = 0; i < 3; i++) {
        int y = 4 + i * 15;
        int x = 8 + (i == 1 ? 6 : 0);
        b.ellipse(x + 10, y + 8, 9, 5, 1);
        b.ellipse(x + 16, y + 5, 4, 3, 2);
        b.rect(x + 2, y + 10, 3, 6, 3);
        b.rect(x + 14, y + 10, 3, 6, 3);
        b.rect(x + 18, y + 4, 5, 2, 4);
    }
    b.line(18, 8, 22, 22, 5, 1.2f);
    b.line(22, 22, 20, 38, 5, 1.2f);
    return b;
}

Bitmap paintSled() {
    Bitmap b(36, 28);
    b.poly({{6, 6}, {30, 6}, {26, 16}, {10, 16}}, 1);
    b.rect(12, 8, 12, 5, 2);
    b.rect(8, 16, 3, 8, 3);
    b.rect(25, 16, 3, 8, 3);
    b.ellipse(9, 24, 3, 2, 4);
    b.ellipse(26, 24, 3, 2, 4);
    b.rect(4, 4, 4, 8, 5);
    return b;
}

Bitmap paintShade() {
    Bitmap b(34, 10);
    b.ellipse(17, 5, 15, 3.5f, 1);
    return b;
}

Bitmap paintPine() {
    Bitmap b(28, 48);
    b.poly({{14, 1}, {26, 18}, {2, 18}}, 1);
    b.poly({{14, 10}, {27, 28}, {1, 28}}, 2);
    b.poly({{14, 20}, {28, 38}, {0, 38}}, 3);
    b.rect(12, 36, 4, 10, 4);
    b.rect(10, 44, 8, 3, 5);
    return b;
}

Bitmap paintStake() {
    Bitmap b(10, 28);
    b.rect(4, 6, 2, 20, 1);
    b.poly({{1, 8}, {5, 1}, {9, 8}}, 2);
    b.rect(2, 24, 6, 3, 3);
    return b;
}

Bitmap paintArch() {
    Bitmap b(72, 32);
    b.rect(2, 6, 6, 24, 1);
    b.rect(64, 6, 6, 24, 1);
    b.rect(4, 4, 64, 8, 2);
    b.rect(16, 6, 8, 4, 3);
    b.rect(32, 6, 8, 4, 4);
    b.rect(48, 6, 8, 4, 3);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(3, 4, 6)});
    setPal(vdp, PAL_TEAM,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(12, 6, 3),
            gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_PINE,
           {0, gs::rgb4(2, 7, 3), gs::rgb4(1, 5, 2), gs::rgb4(3, 9, 4), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_STAKE, {0, gs::rgb4(8, 5, 3), gs::rgb4(14, 4, 3), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(6, 4, 3), gs::rgb4(14, 13, 12), gs::rgb4(12, 3, 3), gs::rgb4(3, 8, 12)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 9), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(13, 14, 15), gs::rgb4(11, 12, 13), gs::rgb4(14, 15, 15), gs::rgb4(8, 9, 10),
            gs::rgb4(6, 7, 8), gs::rgb4(12, 13, 14), gs::rgb4(10, 11, 12), gs::rgb4(9, 10, 11), gs::rgb4(7, 8, 9),
            gs::rgb4(15, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(6, 8, 10), gs::rgb4(9, 11, 13), gs::rgb4(15, 15, 14),
            gs::rgb4(15, 15, 15)});

    art.dogs = gs::uploadMipped(vdp, paintDogs());
    art.sled = gs::uploadMipped(vdp, paintSled());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.pine = gs::uploadMipped(vdp, paintPine());
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.arch = gs::uploadMipped(vdp, paintArch());
    art.title = words(vdp, "MUSH LANE", 3, 1);
    art.stay = words(vdp, "STAY IN THE LANE", 1, 1);
    art.held = words(vdp, "LANE HELD", 2, 1);
    art.left = words(vdp, "LEFT THE LANE", 2, 1);
    art.late = words(vdp, "CREW BEAT YOU", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(12, 13, 15));
}

}  // namespace mushlane
