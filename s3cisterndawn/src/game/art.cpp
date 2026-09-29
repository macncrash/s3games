#include "art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace cdawn {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

gs::Bitmap cisternArt() {
    gs::Bitmap b(140, 90);
    b.ellipse(70, 48, 62, 34, 2);
    b.ellipse(70, 46, 50, 26, 3);
    b.ellipse(70, 44, 36, 16, 4);
    b.ellipse(62, 40, 10, 4, 5);
    b.rect(18, 70, 16, 10, 6);
    b.rect(106, 70, 16, 10, 6);
    b.rect(64, 76, 12, 8, 7);
    for (int i = 0; i < 8; i++) b.rect(22 + i * 13, 22, 3, 8, 8);
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(18, 28);
    b.rect(7, 6, 4, 18, 1);
    b.rect(4, 4, 10, 4, 2);
    b.rect(5, 22, 8, 4, 3);
    b.rect(8, 2, 2, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flameArt(int step) {
    gs::Bitmap b(18, 28);
    float lean = step ? 2.2f : -1.2f;
    b.ellipse(9 + lean, 18, 5.5f, 8, 1);
    b.ellipse(9 + lean * 0.35f, 12, 3.4f, 7, 2);
    b.ellipse(9, 8, 1.8f, 4, 3);
    return b;
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(30, 42);
    b.ellipse(14, 8, 6, 5, 3);
    b.rect(9, 5, 10, 3, 4);
    b.rect(8, 14, 12, 12, 1);
    b.rect(8, 14, 3, 12, 2);
    int leg = step ? 4 : 0;
    b.rect(8, 26, 4, 12, 5);
    b.rect(16 + leg, 26, 4, 12 - leg, 5);
    b.rect(18, 16, 10, 3, 6);
    b.ellipse(26, 16, 3, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(18, 12, 8, 8, 0);
    b.ellipse(10, 16, 2, 2, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 8, 8, 1);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        float c = std::cos(a), s = std::sin(a);
        b.line(16 + c * 10, 16 + s * 10, 16 + c * 15, 16 + s * 15, 2, 1.4f);
    }
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(9, 9);
    b.line(4, 0, 4, 8, 1, 1.f);
    b.line(0, 4, 8, 4, 1, 1.f);
    b.set(2, 2, 2);
    b.set(6, 2, 2);
    b.set(2, 6, 2);
    b.set(6, 6, 2);
    return b;
}

gs::Bitmap dropArt() {
    gs::Bitmap b(8, 14);
    b.ellipse(4, 8, 2.2f, 4, 1);
    b.line(4, 1, 4, 6, 2, 1.f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {0, rgb4(14, 13, 10), rgb4(4, 4, 6), rgb4(8, 8, 10)});
    setPal(vdp, PAL_YARD, {0, rgb4(2, 3, 2), rgb4(3, 4, 3), rgb4(5, 6, 4), rgb4(1, 2, 1)});
    setPal(vdp, PAL_STONE, {0, rgb4(6, 6, 7), rgb4(8, 8, 9), rgb4(4, 5, 6), rgb4(2, 4, 7), rgb4(6, 8, 12),
                            rgb4(5, 5, 5), rgb4(7, 7, 6), rgb4(9, 9, 8), rgb4(3, 3, 4)});
    setPal(vdp, PAL_YOU, {0, rgb4(3, 4, 6), rgb4(5, 6, 8), rgb4(12, 9, 6), rgb4(8, 6, 4), rgb4(2, 2, 3),
                          rgb4(10, 8, 3), rgb4(4, 6, 8), rgb4(1, 1, 2)});
    setPal(vdp, PAL_POST, {0, rgb4(5, 4, 3), rgb4(8, 6, 4), rgb4(3, 3, 3), rgb4(12, 8, 3), rgb4(2, 2, 2)});
    setPal(vdp, PAL_FIRE, {0, rgb4(12, 3, 1), rgb4(15, 9, 2), rgb4(15, 14, 6), rgb4(15, 12, 4)});
    setPal(vdp, PAL_MOON, {0, rgb4(12, 13, 14), rgb4(8, 9, 11)});
    setPal(vdp, PAL_STAR, {0, rgb4(14, 14, 12), rgb4(8, 8, 10)});
    setPal(vdp, PAL_WATER, {0, rgb4(2, 4, 8), rgb4(4, 7, 11), rgb4(1, 2, 5)});
    setPal(vdp, PAL_GOLD, {0, rgb4(15, 12, 4), rgb4(8, 6, 2)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 4, 3), rgb4(8, 2, 2)});
    setPal(vdp, PAL_OK, {0, rgb4(6, 14, 7), rgb4(2, 6, 3)});
    setPal(vdp, PAL_RAIN, {0, rgb4(8, 10, 14), rgb4(12, 13, 15)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.cistern = gs::uploadMipped(vdp, cisternArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.drop = gs::uploadMipped(vdp, dropArt());
}

}  // namespace cdawn
