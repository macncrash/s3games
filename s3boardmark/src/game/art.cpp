#include "game/art.h"

#include <cmath>
#include <string>

namespace boardmark {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void ink(gs::VDP& vdp, int pal, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
    uint16_t p[16] = {};
    p[1] = gs::rgb4(15, 15, 14);
    p[2] = a;
    p[3] = b;
    p[4] = c;
    p[5] = d;
    p[15] = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, p);
}

gs::Bitmap lampArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 9, 9, 5);
    b.ellipse(10, 10, 6.5f, 6.5f, 4);
    b.ellipse(10, 10, 4.2f, 4.2f, 3);
    b.ellipse(10, 10, 2.2f, 2.2f, 2);
    b.ellipse(8, 8, 1.2f, 1.0f, 1);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 4);
    b.ellipse(9, 9, 6, 6, 3);
    b.ellipse(9, 9, 3.2f, 3.2f, 2);
    b.rect(8, 5, 2, 8, 1);
    b.rect(6, 8, 6, 2, 1);
    return b;
}

gs::Bitmap plugArt() {
    gs::Bitmap b(14, 22);
    b.rect(4, 2, 6, 8, 3);
    b.rect(5, 3, 4, 5, 2);
    b.rect(3, 10, 8, 6, 4);
    b.rect(6, 16, 2, 5, 5);
    b.rect(2, 4, 2, 3, 1);
    b.rect(10, 4, 2, 3, 1);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 3);
    b.ellipse(3, 3, 1, 1, 1);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(26, 26);
    for (int y = 0; y < 26; y++) {
        for (int x = 0; x < 26; x++) {
            float d = std::hypot(x - 12.5f, y - 12.5f);
            if (d > 8.4f && d < 11.6f) b.set(x, y, 1);
        }
    }
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 2);
    return b;
}

gs::Bitmap opArt() {
    gs::Bitmap b(22, 36);
    b.ellipse(11, 7, 5, 5, 2);
    b.ellipse(11, 6, 2, 2, 1);
    b.rect(7, 13, 8, 12, 3);
    b.rect(8, 14, 6, 8, 4);
    b.rect(5, 14, 3, 9, 3);
    b.rect(14, 14, 3, 9, 3);
    b.rect(8, 25, 3, 9, 5);
    b.rect(12, 25, 3, 9, 5);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(28, 48);
    b.rect(0, 0, 28, 48, 4);
    b.rect(3, 3, 22, 42, 3);
    b.rect(6, 8, 7, 8, 5);
    b.rect(15, 8, 7, 8, 5);
    b.ellipse(20, 26, 1.4f, 1.4f, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int y = 0; y < 7; y++) {
                for (int x = 0; x < 5; x++) {
                    if (!g[y * 5 + x]) continue;
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_INK, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2));
    ink(vdp, PAL_WOOD, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(12, 8, 2), gs::rgb4(7, 4, 1), gs::rgb4(3, 2, 1));
    ink(vdp, PAL_TEAL, gs::rgb4(6, 14, 12), gs::rgb4(2, 9, 9), gs::rgb4(1, 5, 6), gs::rgb4(1, 2, 3));
    ink(vdp, PAL_ROSE, gs::rgb4(15, 8, 8), gs::rgb4(11, 3, 5), gs::rgb4(6, 1, 3), gs::rgb4(2, 1, 2));
    ink(vdp, PAL_CORD, gs::rgb4(12, 10, 6), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1));
    ink(vdp, PAL_OK, gs::rgb4(8, 15, 8), gs::rgb4(3, 10, 4), gs::rgb4(1, 5, 2), gs::rgb4(1, 2, 1));
    ink(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(10, 2, 2), gs::rgb4(5, 1, 1), gs::rgb4(2, 0, 0));
    ink(vdp, PAL_NIGHT, gs::rgb4(6, 7, 12), gs::rgb4(3, 3, 7), gs::rgb4(1, 1, 3), gs::rgb4(0, 0, 1));
    vdp.setFogColor(gs::rgb4(1, 1, 3));

    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.coin = gs::uploadMipped(vdp, coinArt());
    art.plug = gs::uploadMipped(vdp, plugArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.op = gs::uploadMipped(vdp, opArt());
    art.door = gs::uploadMipped(vdp, doorArt());

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
}

}  // namespace boardmark
