#include "game/art.h"

#include <initializer_list>

namespace ovenbell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap archArt() {
    gs::Bitmap b(240, 132);
    b.rect(0, 36, 240, 96, 2);
    b.rect(28, 52, 184, 80, 0);
    b.ellipse(120, 62, 86, 40, 0);
    b.rect(6, 22, 228, 18, 3);
    b.rect(0, 112, 240, 20, 4);
    for (int x = 8; x < 232; x += 24) b.rect(float(x), 26, 16, 8, 1);
    for (int y = 56; y < 112; y += 16) {
        b.rect(6, float(y), 14, 10, 1);
        b.rect(220, float(y), 14, 10, 1);
    }
    b.rect(108, 8, 4, 18, 5);
    b.rect(104, 118, 32, 8, 5);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(80, 42);
    b.ellipse(40, 26, 36, 14, 2);
    b.ellipse(40, 24, 28, 10, 3);
    b.ellipse(30, 18, 9, 4, 4);
    for (int i = 0; i < 4; i++) b.line(18.f + float(i) * 12.f, 14.f, 22.f + float(i) * 12.f, 28.f, 1, 1.5f);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(36, 32);
    b.rect(16, 0, 4, 6, 1);
    b.ellipse(18, 18, 16, 12, 2);
    b.ellipse(18, 16, 10, 7, 3);
    b.rect(4, 26, 28, 4, 1);
    b.rect(15, 22, 6, 8, 4);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 14);
    b.rect(3, 0, 2, 8, 1);
    b.ellipse(4, 10, 3, 3, 2);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(16, 26);
    b.ellipse(8, 16, 6, 9, 2);
    b.ellipse(8, 18, 3, 6, 3);
    b.ellipse(8, 11, 2, 5, 1);
    return b;
}

gs::Bitmap peelArt() {
    gs::Bitmap b(160, 14);
    b.ellipse(30, 7, 28, 6, 2);
    b.rect(54, 5, 96, 4, 3);
    b.rect(146, 3, 10, 8, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 13, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(8, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_OVEN,
           {0, gs::rgb4(8, 3, 2), gs::rgb4(6, 3, 2), gs::rgb4(10, 5, 3), gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 4)});
    setPal(vdp, PAL_LOAF, {0, gs::rgb4(8, 5, 2), gs::rgb4(13, 10, 6), gs::rgb4(15, 13, 8), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_PALE, {0, gs::rgb4(10, 8, 5), gs::rgb4(14, 12, 8), gs::rgb4(15, 14, 10), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_CRUST, {0, gs::rgb4(8, 4, 1), gs::rgb4(13, 8, 2), gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_CHAR, {0, gs::rgb4(2, 1, 1), gs::rgb4(4, 2, 2), gs::rgb4(6, 3, 2), gs::rgb4(8, 4, 3)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 14, 4), gs::rgb4(13, 4, 1), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_PEEL, {0, gs::rgb4(5, 3, 1), gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(9, 8, 6), gs::rgb4(14, 12, 8), gs::rgb4(15, 14, 10), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_CLAP, {0, gs::rgb4(6, 5, 4), gs::rgb4(10, 8, 5)});
    loadFont(vdp, art);
    art.arch = gs::uploadImage(vdp, archArt());
    art.loaf = gs::uploadImage(vdp, loafArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.flame = gs::uploadImage(vdp, flameArt());
    art.peel = gs::uploadImage(vdp, peelArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.bar = gs::uploadImage(vdp, barArt());
}

}  // namespace ovenbell
