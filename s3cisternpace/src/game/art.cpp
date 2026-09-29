#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace cistern {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap walker(int step) {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 8, 6, 6, 4);
    b.ellipse(14, 5, 6, 3, 5);
    b.set(12, 8, 8);
    b.set(16, 8, 8);
    b.poly({{8, 14}, {20, 14}, {22, 32}, {6, 32}}, 2);
    b.rect(12, 16, 4, 12, 3);
    b.line(10, 16, 4, 28, 6, 2.f);
    b.line(18, 16, 24, 26, 6, 2.f);
    if (step == 0) {
        b.rect(8, 32, 5, 12, 3);
        b.rect(16, 32, 5, 9, 7);
    } else {
        b.rect(8, 32, 5, 9, 7);
        b.rect(16, 32, 5, 12, 3);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap sunkFig() {
    gs::Bitmap b(40, 18);
    b.ellipse(12, 9, 6, 5, 4);
    b.poly({{16, 6}, {36, 8}, {34, 14}, {16, 13}}, 2);
    b.ellipse(30, 12, 4, 2, 1);
    return b;
}

gs::Bitmap stoneBlock() {
    gs::Bitmap b(26, 18);
    b.poly({{2, 4}, {22, 2}, {24, 14}, {4, 16}}, 2);
    b.poly({{2, 4}, {12, 3}, {13, 15}, {4, 16}}, 1);
    b.line(6, 6, 18, 5, 3, 1.f);
    b.line(8, 10, 20, 9, 3, 1.f);
    b.rect(16, 7, 3, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap waterDisc() {
    gs::Bitmap b(64, 36);
    b.ellipse(32, 18, 30, 15, 1);
    b.ellipse(32, 16, 22, 10, 2);
    b.ellipse(26, 14, 8, 3, 3);
    b.ellipse(40, 20, 6, 2, 4);
    return b;
}

gs::Bitmap bucketArt() {
    gs::Bitmap b(16, 18);
    b.poly({{3, 4}, {13, 4}, {11, 16}, {5, 16}}, 1);
    b.rect(3, 3, 10, 3, 2);
    b.line(5, 4, 8, 0, 3, 1.f);
    b.line(11, 4, 8, 0, 3, 1.f);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(6, 40);
    b.line(3, 0, 3, 39, 1, 2.f);
    b.line(2, 8, 4, 12, 2, 1.f);
    b.line(4, 20, 2, 24, 2, 1.f);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 0, 2, 4, 1);
    b.poly({{2, 6}, {12, 6}, {10, 14}, {4, 14}}, 2);
    b.ellipse(7, 15, 4, 2, 3);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(18, 18);
    for (int i = 0; i < 16; i++) {
        if ((i % 4) == 0) continue;
        float a = i * 6.2831853f / 16.f;
        b.set(int(std::lround(9 + std::cos(a) * 6)), int(std::lround(9 + std::sin(a) * 6)), 1);
    }
    b.rect(8, 2, 2, 3, 2);
    b.rect(8, 13, 2, 3, 2);
    b.rect(2, 8, 3, 2, 2);
    b.rect(13, 8, 3, 2, 2);
    b.set(9, 9, 1);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(14, 56);
    b.rect(5, 0, 4, 34, 1);
    b.rect(6, 2, 2, 28, 2);
    b.rect(3, 30, 8, 6, 3);
    b.rect(4, 36, 5, 14, 1);
    b.rect(2, 48, 9, 6, 4);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap flareArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 2);
    b.ellipse(9, 9, 3, 3, 1);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(22, 12);
    b.ellipse(11, 7, 9, 3, 1);
    b.ellipse(6, 5, 3, 2, 2);
    b.ellipse(16, 4, 3, 2, 3);
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
    const uint16_t ink = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(13, 14, 15), gs::rgb4(8, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(5, 14, 8), gs::rgb4(12, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5), gs::rgb4(7, 8, 6), gs::rgb4(3, 3, 4),
                            0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(7, 8, 9), gs::rgb4(4, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(12, 10, 8), gs::rgb4(2, 2, 3),
                           gs::rgb4(8, 7, 6), gs::rgb4(5, 4, 4), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(10, 15, 8), gs::rgb4(3, 10, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 8), gs::rgb4(3, 9, 11), gs::rgb4(6, 12, 13), gs::rgb4(1, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 14, 10), gs::rgb4(15, 9, 3), gs::rgb4(8, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MOSS, {0, gs::rgb4(3, 7, 3), gs::rgb4(6, 10, 4), gs::rgb4(2, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(7, 8, 8), gs::rgb4(4, 5, 5), gs::rgb4(10, 9, 6), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    art.step[0] = gs::uploadMipped(vdp, walker(0));
    art.step[1] = gs::uploadMipped(vdp, walker(1));
    art.sunk = gs::uploadMipped(vdp, sunkFig());
    art.block = gs::uploadMipped(vdp, stoneBlock());
    art.water = gs::uploadMipped(vdp, waterDisc());
    art.bucket = gs::uploadMipped(vdp, bucketArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.splash = gs::uploadMipped(vdp, splashArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(1, 2, 3));
}

}  // namespace cistern
