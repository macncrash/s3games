#include "game/art.h"

#include <initializer_list>
#include <string>

namespace keysgold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{2, 1, 0, 0, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap goldArt() {
    gs::Bitmap b(18, 22);
    b.rect(12, 1, 2, 11, 1);
    b.poly({{14, 1}, {17, 4}, {14, 7}}, 1);
    b.ellipse(8, 15, 6, 4, 1);
    b.ellipse(7, 14, 2, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap creamArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(9, 9, 3, 3, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap keyArt(char letter) {
    gs::Bitmap b(36, 22);
    b.rect(1, 1, 34, 20, 1);
    b.rect(1, 14, 34, 7, 2);
    gs::Bitmap glyph = gs::textBitmap(std::string(1, letter), gs::TextStyle{2, 3, 0, 0, 1});
    b.blit(glyph, 18 - glyph.w / 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(28, 6);
    b.rect(0, 2, 28, 2, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 14);
    b.rect(4, 9, 2, 5, 2);
    b.ellipse(5, 6, 4, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 8), gs::rgb4(8, 5, 1), gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 9), gs::rgb4(8, 7, 6), gs::rgb4(6, 5, 4), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_KEY, {0, gs::rgb4(6, 5, 8), gs::rgb4(3, 2, 4), gs::rgb4(12, 11, 10), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_KEYLIT, {0, gs::rgb4(15, 13, 6), gs::rgb4(10, 7, 2), gs::rgb4(15, 15, 12), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_LINE, {0, gs::rgb4(15, 11, 3), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(13, 3, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(14, 14, 13), gs::rgb4(7, 7, 8)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3)});
    loadFont(vdp, art);
    art.gold = gs::uploadMipped(vdp, goldArt());
    art.cream = gs::uploadMipped(vdp, creamArt());
    const char letters[kLanes] = {'Z', 'X', 'C', 'V'};
    for (int i = 0; i < kLanes; i++) art.key[i] = gs::uploadMipped(vdp, keyArt(letters[i]));
    art.rail = gs::uploadMipped(vdp, railArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace keysgold
