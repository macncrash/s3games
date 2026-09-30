#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace lenschime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap towerArt() {
    gs::Bitmap b(70, 120);
    b.rect(18, 8, 34, 100, 2);
    b.rect(18, 8, 34, 4, 1);
    b.rect(10, 104, 50, 12, 3);
    b.rect(14, 100, 42, 4, 2);
    b.rect(28, 0, 14, 12, 3);
    b.rect(22, 36, 10, 16, 4);
    b.rect(38, 36, 10, 16, 4);
    b.rect(22, 64, 10, 16, 4);
    b.rect(38, 64, 10, 16, 4);
    b.rect(30, 90, 10, 14, 5);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 24, 22, 22, 1);
    b.ellipse(24, 24, 18, 18, 2);
    b.ellipse(24, 24, 2, 2, 3);
    for (int i = 0; i < 12; i++) {
        float a = i * 3.14159265f / 6.f;
        int x = int(24 + std::sin(a) * 16);
        int y = int(24 - std::cos(a) * 16);
        b.set(x, y, 3);
        b.set(x + 1, y, 3);
    }
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(12, 0, 4, 8, 3);
    b.ellipse(14, 18, 12, 10, 2);
    b.ellipse(14, 16, 7, 6, 1);
    b.rect(4, 24, 20, 5, 3);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 12);
    b.rect(3, 0, 2, 5, 3);
    b.ellipse(4, 8, 3, 3, 1);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(90, 10);
    b.rect(0, 3, 90, 6, 2);
    b.rect(0, 3, 90, 2, 1);
    return b;
}

gs::Bitmap bodyArt() {
    gs::Bitmap b(120, 44);
    b.rect(2, 8, 86, 30, 2);
    b.rect(2, 8, 86, 4, 1);
    b.rect(8, 16, 36, 16, 3);
    b.ellipse(78, 22, 14, 14, 4);
    b.rect(96, 16, 18, 10, 1);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(30, 30);
    b.ellipse(15, 15, 14, 14, 2);
    b.ellipse(15, 15, 9, 9, 3);
    b.ellipse(15, 15, 4, 4, 1);
    return b;
}

gs::Bitmap glassArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(8, 8, 3, 2, 2);
    return b;
}

gs::Bitmap cornerArt() {
    gs::Bitmap b(14, 14);
    b.rect(0, 0, 12, 2, 1);
    b.rect(0, 0, 2, 12, 1);
    return b;
}

gs::Bitmap caretArt() {
    gs::Bitmap b(6, 16);
    b.rect(2, 0, 2, 16, 1);
    b.rect(0, 7, 6, 2, 2);
    return b;
}

gs::Bitmap plateArt() {
    gs::Bitmap b(16, 12);
    b.rect(1, 1, 14, 10, 1);
    b.rect(3, 3, 10, 6, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 2, 2, 1);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(4, 18);
    b.rect(1, 0, 2, 18, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t sh = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(7, 7, 8), gs::rgb4(15, 12, 4), sh});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(9, 12, 14),
                            gs::rgb4(3, 3, 4), sh});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 3), sh});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 13, 6), gs::rgb4(11, 8, 3), gs::rgb4(5, 4, 2), gs::rgb4(8, 10, 12), sh});
    setPal(vdp, PAL_GLASS, {0, gs::rgb4(12, 15, 15), gs::rgb4(15, 15, 15), sh});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 2), sh});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 15, 8), gs::rgb4(2, 8, 3), sh});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), sh});

    art.tower = gs::uploadMipped(vdp, towerArt());
    art.face = gs::uploadMipped(vdp, faceArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.body = gs::uploadMipped(vdp, bodyArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.glass = gs::uploadMipped(vdp, glassArt());
    art.corner = gs::uploadMipped(vdp, cornerArt());
    art.caret = gs::uploadMipped(vdp, caretArt());
    art.plate = gs::uploadMipped(vdp, plateArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.hand = gs::uploadMipped(vdp, handArt());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(10, 11, 13));
}

}  // namespace lenschime
