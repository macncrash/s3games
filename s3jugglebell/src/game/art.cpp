#include "game/art.h"

#include <initializer_list>

namespace jugglebell {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

gs::Bitmap bellArt() {
    gs::Bitmap b(36, 32);
    b.ellipse(18.f, 16.f, 15.f, 13.f, 3);
    b.ellipse(18.f, 15.f, 11.f, 9.5f, 2);
    b.ellipse(18.f, 16.f, 6.2f, 5.4f, 1);
    b.ellipse(18.f, 8.f, 3.2f, 2.6f, 5);
    b.line(8.f, 10.f, 12.f, 16.f, 5, 1.2f);
    b.ellipse(18.f, 24.f, 13.f, 3.2f, 4);
    b.rect(16, 1, 4, 5, 6);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(7, 12);
    b.line(3, 0, 3, 6, 1, 1.2f);
    b.ellipse(3.f, 9.f, 2.4f, 2.4f, 2);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(3, 28);
    b.rect(1, 0, 1, 28, 1);
    b.set(0, 6, 2);
    b.set(2, 14, 2);
    b.set(0, 22, 2);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7.f, 7.f, 6.2f, 6.2f, 1);
    b.ellipse(7.f, 7.f, 4.4f, 4.4f, 2);
    b.ellipse(5.f, 5.f, 1.6f, 1.4f, 3);
    return b;
}

gs::Bitmap jugglerArt() {
    gs::Bitmap b(48, 72);
    b.ellipse(24.f, 12.f, 8.f, 8.f, 1);
    b.ellipse(24.f, 11.f, 7.2f, 4.f, 2);
    b.rect(18, 8, 3, 2, 3);
    b.rect(27, 8, 3, 2, 3);
    b.ellipse(24.f, 16.f, 2.2f, 1.2f, 4);
    b.rect(16, 20, 16, 22, 5);
    b.rect(18, 22, 4, 16, 6);
    b.rect(26, 22, 4, 16, 6);
    b.line(16, 24, 4, 40, 5, 3.2f);
    b.line(32, 24, 44, 38, 5, 3.2f);
    b.rect(18, 42, 5, 22, 7);
    b.rect(25, 42, 5, 22, 7);
    b.rect(16, 62, 8, 4, 8);
    b.rect(24, 62, 8, 4, 8);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(12, 10);
    b.ellipse(6.f, 5.f, 5.f, 4.f, 1);
    b.ellipse(6.f, 4.f, 2.2f, 1.4f, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 96);
    b.rect(2, 0, 4, 96, 1);
    b.rect(3, 0, 1, 96, 2);
    b.rect(0, 88, 8, 8, 3);
    return b;
}

gs::Bitmap meterArt() {
    gs::Bitmap b(6, 8);
    b.rect(0, 0, 6, 8, 1);
    b.rect(1, 1, 4, 6, 2);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(5, 8);
    b.rect(1, 0, 3, 8, 1);
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 13), gs::rgb4(2, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 8), gs::rgb4(4, 1, 2));
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(4, 2, 0), gs::rgb4(12, 8, 2), gs::rgb4(8, 5, 1), gs::rgb4(14, 11, 4), gs::rgb4(13, 9, 3),
            gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(10, 1, 2), gs::rgb4(15, 3, 3), gs::rgb4(15, 12, 10)});
    setPal(vdp, PAL_GOLD_BALL, {0, gs::rgb4(10, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(1, 3, 10), gs::rgb4(3, 7, 15), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_BODY,
           {0, gs::rgb4(12, 8, 5), gs::rgb4(6, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 8, 7), gs::rgb4(4, 2, 6),
            gs::rgb4(8, 3, 8), gs::rgb4(3, 2, 5), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(14, 10, 7), gs::rgb4(15, 13, 10)});

    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.rope = gs::uploadImage(vdp, ropeArt());
    art.ball = gs::uploadImage(vdp, ballArt());
    art.juggler = gs::uploadImage(vdp, jugglerArt());
    art.hand = gs::uploadImage(vdp, handArt());
    art.post = gs::uploadImage(vdp, postArt());
    art.meter = gs::uploadImage(vdp, meterArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    loadFont(vdp, art);
}

}  // namespace jugglebell
