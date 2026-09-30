#include "game/art.h"

#include <initializer_list>

namespace cuemark {
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

gs::Bitmap tableArt() {
    gs::Bitmap b(220, 86);
    b.rect(0, 0, 220, 86, 1);
    b.rect(8, 8, 204, 70, 2);
    b.rect(14, 14, 192, 58, 3);
    for (int i = 0; i < 6; i++) {
        b.rect(28 + i * 28, 10, 4, 4, 4);
        b.rect(28 + i * 28, 72, 4, 4, 4);
    }
    b.ellipse(18, 16, 7, 7, 5);
    b.ellipse(202, 16, 7, 7, 5);
    b.ellipse(18, 70, 7, 7, 5);
    b.ellipse(202, 70, 7, 7, 5);
    b.ellipse(110, 16, 7, 7, 5);
    b.ellipse(110, 70, 7, 7, 5);
    return b;
}

gs::Bitmap cueArt() {
    gs::Bitmap b(120, 10);
    b.rect(0, 3, 96, 4, 2);
    b.rect(0, 4, 80, 2, 3);
    b.rect(90, 2, 16, 6, 4);
    b.rect(106, 3, 10, 4, 1);
    b.rect(114, 4, 6, 2, 5);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 2);
    b.ellipse(7, 7, 3, 3, 3);
    b.ellipse(11, 11, 2, 2, 1);
    return b;
}

gs::Bitmap spotArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.ellipse(8, 8, 3, 3, 3);
    b.rect(7, 2, 2, 12, 1);
    b.rect(2, 7, 12, 2, 1);
    return b;
}

gs::Bitmap chalkArt() {
    gs::Bitmap b(16, 14);
    b.rect(2, 2, 12, 10, 2);
    b.rect(4, 4, 8, 4, 3);
    b.rect(3, 10, 10, 2, 1);
    return b;
}

gs::Bitmap playerArt() {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 10, 7, 7, 2);
    b.rect(14, 18, 12, 18, 3);
    b.rect(6, 20, 8, 5, 4);
    b.rect(26, 22, 10, 4, 3);
    b.rect(14, 36, 5, 20, 1);
    b.rect(21, 36, 5, 20, 1);
    b.rect(12, 54, 8, 6, 4);
    b.rect(22, 54, 8, 6, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 12), gs::rgb4(4, 6, 4), gs::rgb4(1, 2, 1)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(1, 1, 1));
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(3, 2, 1),
                           gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(1, 4, 2), gs::rgb4(2, 8, 3), gs::rgb4(4, 12, 5), gs::rgb4(8, 7, 3),
                            gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_CUE, {0, gs::rgb4(12, 10, 6), gs::rgb4(9, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(6, 3, 1),
                          gs::rgb4(14, 14, 10)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(6, 6, 7), gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(10, 7, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(3, 2, 2), gs::rgb4(10, 7, 5), gs::rgb4(2, 3, 6), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_CHALK, {0, gs::rgb4(1, 3, 8), gs::rgb4(2, 6, 13), gs::rgb4(6, 10, 15)});

    art.table = gs::uploadMipped(vdp, tableArt());
    art.cue = gs::uploadMipped(vdp, cueArt());
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.spot = gs::uploadMipped(vdp, spotArt());
    art.chalk = gs::uploadMipped(vdp, chalkArt());
    art.player = gs::uploadMipped(vdp, playerArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 3;
    st.outline = 1;
    st.shadow = 15;
    art.title = gs::uploadImage(vdp, gs::textBitmap("CUE", st));

    loadFont(vdp, art);
}

}  // namespace cuemark
