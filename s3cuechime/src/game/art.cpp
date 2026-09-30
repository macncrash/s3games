#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace cuechime {
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
    gs::Bitmap b(248, 86);
    b.rect(0, 0, 248, 86, 1);
    b.rect(8, 8, 232, 70, 2);
    b.rect(16, 16, 216, 54, 3);
    b.line(28, 42, 220, 42, 4, 1);
    b.ellipse(22, 18, 8, 8, 5);
    b.ellipse(226, 18, 8, 8, 5);
    b.ellipse(22, 68, 8, 8, 5);
    b.ellipse(226, 68, 8, 8, 5);
    b.ellipse(124, 16, 7, 6, 5);
    b.ellipse(124, 70, 7, 6, 5);
    return b;
}

gs::Bitmap cueArt() {
    gs::Bitmap b(96, 8);
    b.rect(0, 2, 72, 4, 2);
    b.rect(0, 3, 48, 2, 3);
    b.rect(70, 1, 12, 6, 4);
    b.rect(82, 2, 8, 4, 1);
    b.rect(90, 3, 6, 2, 5);
    return b;
}

gs::Bitmap cueBallArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.ellipse(6, 6, 3, 2, 3);
    b.ellipse(10, 11, 2, 2, 1);
    return b;
}

gs::Bitmap objectArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.ellipse(8, 8, 3, 3, 4);
    b.ellipse(6, 6, 2, 1, 3);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(52, 52);
    b.ellipse(26, 26, 24, 24, 2);
    b.ellipse(26, 26, 20, 20, 3);
    b.ellipse(26, 26, 2, 2, 4);
    for (int i = 0; i < 12; i++) {
        float a = -1.5708f + float(i) * 0.5236f;
        int x = int(26 + std::cos(a) * 16);
        int y = int(26 + std::sin(a) * 16);
        b.rect(x - 1, y - 1, 2, 2, i % 3 == 0 ? 5 : 1);
    }
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 2, 2, 1);
    return b;
}

gs::Bitmap playerArt() {
    gs::Bitmap b(34, 56);
    b.ellipse(14, 8, 6, 6, 2);
    b.rect(9, 15, 10, 16, 3);
    b.rect(2, 17, 8, 4, 4);
    b.rect(18, 18, 12, 3, 5);
    b.rect(9, 31, 4, 16, 1);
    b.rect(15, 31, 4, 16, 1);
    b.rect(7, 46, 7, 5, 6);
    b.rect(15, 46, 7, 5, 6);
    return b;
}

gs::Bitmap pocketArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 5, 5, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 11), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 1));
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(5, 3, 1), gs::rgb4(9, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(3, 2, 1),
                           gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(4, 2, 1), gs::rgb4(0, 4, 2), gs::rgb4(1, 8, 3), gs::rgb4(6, 11, 5),
                            gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_CUE, {0, gs::rgb4(7, 4, 1), gs::rgb4(11, 7, 2), gs::rgb4(14, 10, 4), gs::rgb4(5, 2, 1),
                          gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(8, 8, 7), gs::rgb4(15, 15, 13), gs::rgb4(13, 13, 11), gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(8, 6, 3), gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 8), gs::rgb4(4, 3, 2),
                            gs::rgb4(14, 12, 6), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(3, 2, 2), gs::rgb4(12, 9, 6), gs::rgb4(2, 3, 6), gs::rgb4(6, 4, 2),
                             gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_POCKET, {0, gs::rgb4(1, 1, 1), gs::rgb4(0, 0, 0), gs::rgb4(3, 2, 1)});

    art.table = gs::uploadMipped(vdp, tableArt());
    art.cue = gs::uploadMipped(vdp, cueArt());
    art.cueBall = gs::uploadMipped(vdp, cueBallArt());
    art.object = gs::uploadMipped(vdp, objectArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.hand = gs::uploadMipped(vdp, handArt());
    art.player = gs::uploadMipped(vdp, playerArt());
    art.pocket = gs::uploadMipped(vdp, pocketArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 4;
    st.outline = 1;
    st.shadow = 15;
    art.title = gs::uploadImage(vdp, gs::textBitmap("CUE CHIME", st));
    loadFont(vdp, art);
}

}  // namespace cuechime
