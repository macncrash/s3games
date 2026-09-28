#include "game/art.h"

#include <initializer_list>

namespace jugglemark {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

gs::Bitmap jugglerArt() {
    gs::Bitmap b(40, 78);
    b.ellipse(20.f, 10.f, 7.f, 7.f, 1);
    b.ellipse(17.f, 9.f, 1.2f, 1.2f, 2);
    b.ellipse(23.f, 9.f, 1.2f, 1.2f, 2);
    b.rect(16.f, 17.f, 8.f, 6.f, 1);
    b.rect(12.f, 22.f, 16.f, 22.f, 3);
    b.rect(12.f, 22.f, 16.f, 4.f, 4);
    b.rect(14.f, 44.f, 5.f, 24.f, 5);
    b.rect(21.f, 44.f, 5.f, 24.f, 5);
    b.rect(11.f, 66.f, 9.f, 4.f, 6);
    b.rect(20.f, 66.f, 9.f, 4.f, 6);
    b.line(12.f, 26.f, 2.f, 40.f, 1, 3.f);
    b.line(28.f, 26.f, 38.f, 40.f, 1, 3.f);
    return b;
}

gs::Bitmap gloveArt() {
    gs::Bitmap b(18, 14);
    b.ellipse(9.f, 8.f, 7.f, 5.f, 1);
    b.rect(3.f, 2.f, 3.f, 6.f, 1);
    b.rect(7.f, 1.f, 3.f, 6.f, 1);
    b.rect(11.f, 2.f, 3.f, 6.f, 1);
    b.rect(3.f, 7.f, 12.f, 2.f, 2);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(6.f, 6.f, 2.2f, 2.f, 2);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 3);
    b.ellipse(8.f, 8.f, 5.5f, 5.5f, 1);
    b.ellipse(6.f, 6.f, 2.f, 1.6f, 2);
    return b;
}

gs::Bitmap goldArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(8.f, 8.f, 4.f, 4.f, 2);
    b.rect(7.f, 3.f, 2.f, 10.f, 3);
    b.rect(3.f, 7.f, 10.f, 2.f, 3);
    return b;
}

gs::Bitmap stageArt() {
    gs::Bitmap b(220, 28);
    b.rect(0, 6, 220, 16, 1);
    b.rect(0, 6, 220, 4, 2);
    b.rect(0, 18, 220, 4, 3);
    for (int i = 0; i < 8; i++) b.rect(8.f + i * 27.f, 10.f, 4.f, 12.f, 4);
    b.rect(0, 22, 220, 6, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 36);
    b.rect(6, 12, 2, 22, 1);
    b.ellipse(7.f, 8.f, 6.f, 6.f, 2);
    b.ellipse(6.f, 7.f, 2.f, 2.f, 3);
    return b;
}

gs::Bitmap chalkArt() {
    gs::Bitmap b(28, 8);
    b.rect(0, 3, 28, 2, 1);
    b.rect(4, 1, 2, 6, 2);
    b.rect(13, 1, 2, 6, 2);
    b.rect(22, 1, 2, 6, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BODY,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 1, 1), gs::rgb4(4, 2, 8), gs::rgb4(8, 3, 10), gs::rgb4(2, 2, 6),
            gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_GLOVE, {0, gs::rgb4(14, 12, 10), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 8, 7), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 7, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_STAGE,
           {0, gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(5, 2, 1), gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_MARK, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 4), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 8), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 12), gs::rgb4(1, 4, 1));

    loadFont(vdp, art);
    art.juggler = gs::uploadImage(vdp, jugglerArt());
    art.glove = gs::uploadImage(vdp, gloveArt());
    art.ball = gs::uploadImage(vdp, ballArt());
    art.gold = gs::uploadImage(vdp, goldArt());
    art.stage = gs::uploadImage(vdp, stageArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.chalk = gs::uploadImage(vdp, chalkArt());
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 JUGGLEMARK", {2, 1, 2, 0, 1}));
    art.win = gs::uploadImage(vdp, gs::textBitmap("FINISHED MARK", {2, 1, 2, 0, 1}));
}

}  // namespace jugglemark
