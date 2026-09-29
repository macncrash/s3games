#include "game/art.h"

#include <string>

namespace mushboom {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap mushBody(int step) {
    Bitmap b(80, 44);
    int bob = step ? 2 : 0;
    b.line(10, 34, 62, 34, 4, 2);
    b.line(8, 36, 22, 40, 5, 2);
    b.line(48, 36, 66, 40, 5, 2);
    b.rect(28, 20 + bob, 30, 14, 4);
    b.rect(30, 22 + bob, 26, 8, 1);
    b.rect(54, 18 + bob, 6, 10, 4);
    b.ellipse(18, 24 + bob, 10, 7, 6);
    b.ellipse(36, 22 + (step ? 0 : 2), 9, 6, 6);
    b.ellipse(12, 22 + bob, 4, 3, 3);
    b.ellipse(30, 18 + bob, 3, 3, 3);
    b.rect(8, 28, 3, 8 - bob, 6);
    b.rect(16, 28, 3, 6 + bob, 6);
    b.rect(32, 28, 3, 8 - bob, 6);
    b.rect(40, 28, 3, 6 + bob, 6);
    b.ellipse(58, 16 + bob, 5, 5, 2);
    b.rect(56, 12 + bob, 8, 3, 2);
    b.outline(7, false);
    return b;
}

Bitmap driveArt() {
    Bitmap b(30, 16);
    b.ellipse(8, 8, 6, 6, 3);
    b.rect(8, 2, 16, 12, 1);
    b.ellipse(24, 8, 6, 6, 1);
    b.ellipse(24, 8, 3, 3, 2);
    b.rect(12, 4, 8, 2, 2);
    b.outline(4, false);
    return b;
}

Bitmap plankArt() {
    Bitmap b(48, 14);
    b.rect(0, 1, 48, 12, 1);
    b.rect(0, 1, 48, 3, 2);
    for (int x = 6; x < 48; x += 12) b.rect(x, 3, 2, 9, 3);
    b.outline(4, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 72);
    b.rect(5, 4, 6, 66, 1);
    b.rect(2, 0, 12, 8, 3);
    b.rect(6, 8, 2, 60, 2);
    b.outline(4, false);
    return b;
}

Bitmap waterArt() {
    Bitmap b(36, 22);
    b.rect(0, 6, 36, 16, 1);
    b.ellipse(8, 8, 8, 3, 3);
    b.ellipse(24, 12, 10, 3, 2);
    b.ellipse(16, 16, 7, 2, 3);
    return b;
}

Bitmap snowArt() {
    Bitmap b(40, 16);
    b.rect(0, 6, 40, 10, 1);
    b.ellipse(8, 6, 8, 4, 2);
    b.ellipse(24, 7, 12, 4, 1);
    b.ellipse(34, 8, 6, 3, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 10);
    b.ellipse(18, 5, 16, 4, 1);
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
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 12, 14), gs::rgb4(15, 12, 6), gs::rgb4(15, 4, 3), gs::rgb4(4, 14, 8),
                          gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MUSH, {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(15, 14, 10), gs::rgb4(10, 6, 2),
                           gs::rgb4(8, 9, 11), gs::rgb4(12, 8, 4), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DRIVE, {0, gs::rgb4(11, 12, 13), gs::rgb4(15, 15, 14), gs::rgb4(6, 7, 8), gs::rgb4(2, 2, 3),
                            gs::rgb4(13, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(4, 4, 5), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 10), gs::rgb4(3, 8, 12), gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, shadow});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 14), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, shadow});
    setPal(vdp, 7, {0, gs::rgb4(0, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, art);
    art.mush[0] = gs::uploadMipped(vdp, mushBody(0));
    art.mush[1] = gs::uploadMipped(vdp, mushBody(1));
    art.drive = gs::uploadMipped(vdp, driveArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.snow = gs::uploadMipped(vdp, snowArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace mushboom
