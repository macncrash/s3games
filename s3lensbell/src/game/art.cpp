#include "game/art.h"

namespace lensbell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap bellArt() {
    gs::Bitmap b(48, 64);
    b.rect(22, 0, 4, 14, 5);
    b.ellipse(24, 30, 18, 16, 2);
    b.ellipse(24, 28, 12, 10, 1);
    b.ellipse(18, 26, 4, 3, 1);
    b.rect(8, 42, 32, 7, 2);
    b.rect(6, 48, 36, 5, 3);
    b.rect(10, 47, 28, 2, 1);
    b.ellipse(24, 40, 3, 5, 4);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(10, 16);
    b.rect(4, 0, 2, 6, 3);
    b.ellipse(5, 11, 4, 4, 4);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(6, 28);
    b.rect(2, 0, 2, 28, 5);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(120, 14);
    b.rect(0, 4, 120, 8, 2);
    b.rect(0, 4, 120, 2, 1);
    b.rect(8, 0, 6, 14, 3);
    b.rect(106, 0, 6, 14, 3);
    return b;
}

gs::Bitmap bodyArt() {
    gs::Bitmap b(140, 52);
    b.rect(4, 10, 100, 34, 2);
    b.rect(4, 10, 100, 4, 1);
    b.rect(8, 18, 46, 18, 3);
    b.rect(12, 22, 18, 10, 4);
    b.ellipse(86, 26, 16, 16, 3);
    b.rect(118, 20, 16, 10, 2);
    b.rect(122, 22, 8, 6, 1);
    b.outline(3, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 2);
    b.ellipse(18, 18, 12, 12, 3);
    b.ellipse(18, 18, 7, 7, 1);
    return b;
}

gs::Bitmap glassArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(10, 10, 4, 3, 2);
    return b;
}

gs::Bitmap cornerArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 12, 2, 2);
    b.rect(0, 0, 2, 12, 2);
    b.rect(0, 0, 8, 1, 1);
    return b;
}

gs::Bitmap caretArt() {
    gs::Bitmap b(8, 14);
    b.rect(3, 0, 2, 14, 1);
    b.rect(1, 6, 6, 2, 2);
    return b;
}

gs::Bitmap notchArt() {
    gs::Bitmap b(6, 18);
    b.rect(2, 0, 2, 18, 1);
    return b;
}

gs::Bitmap plateArt() {
    gs::Bitmap b(18, 14);
    b.rect(1, 1, 16, 12, 1);
    b.rect(3, 3, 12, 8, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap hazeArt() {
    gs::Bitmap b(40, 24);
    b.ellipse(20, 12, 18, 10, 1);
    b.ellipse(14, 10, 6, 4, 2);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 9), gs::rgb4(15, 12, 4), shadow});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 3),
                            gs::rgb4(8, 12, 14), shadow});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(15, 14, 7), gs::rgb4(13, 10, 3), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 3),
                           gs::rgb4(10, 7, 4), shadow});
    setPal(vdp, PAL_GLASS, {0, gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15), gs::rgb4(4, 8, 12), shadow});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), shadow});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 15, 7), gs::rgb4(2, 8, 3), shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(12, 11, 9), shadow});

    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.body = gs::uploadMipped(vdp, bodyArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.glass = gs::uploadMipped(vdp, glassArt());
    art.corner = gs::uploadMipped(vdp, cornerArt());
    art.caret = gs::uploadMipped(vdp, caretArt());
    art.notch = gs::uploadMipped(vdp, notchArt());
    art.plate = gs::uploadMipped(vdp, plateArt());
    art.haze = gs::uploadMipped(vdp, hazeArt());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(11, 12, 14));
}

}  // namespace lensbell
