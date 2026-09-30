#include "game/art.h"

#include <initializer_list>

namespace cuebell {
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
    gs::Bitmap b(240, 78);
    b.rect(0, 0, 240, 78, 1);
    b.rect(6, 6, 228, 66, 2);
    b.rect(12, 12, 216, 54, 3);
    b.rect(18, 36, 204, 2, 4);
    for (int i = 0; i < 5; i++) b.rect(36 + i * 36, 8, 3, 6, 5);
    b.ellipse(16, 14, 6, 6, 6);
    b.ellipse(224, 14, 6, 6, 6);
    b.ellipse(16, 64, 6, 6, 6);
    b.ellipse(224, 64, 6, 6, 6);
    b.ellipse(120, 14, 5, 5, 6);
    b.ellipse(120, 64, 5, 5, 6);
    return b;
}

gs::Bitmap cueArt() {
    gs::Bitmap b(110, 8);
    b.rect(0, 2, 86, 4, 2);
    b.rect(0, 3, 70, 2, 3);
    b.rect(82, 1, 14, 6, 4);
    b.rect(96, 2, 8, 4, 1);
    b.rect(104, 3, 6, 2, 5);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.ellipse(6, 6, 3, 2, 3);
    b.ellipse(10, 10, 2, 2, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(13, 0, 2, 6, 4);
    b.ellipse(14, 8, 3, 3, 2);
    b.poly({{4, 12}, {24, 12}, {27, 26}, {1, 26}}, 3);
    b.poly({{7, 13}, {21, 13}, {22, 20}, {6, 20}}, 2);
    b.rect(1, 24, 26, 4, 5);
    b.ellipse(14, 28, 3, 3, 1);
    return b;
}

gs::Bitmap playerArt() {
    gs::Bitmap b(36, 58);
    b.ellipse(16, 8, 6, 6, 2);
    b.rect(11, 15, 10, 16, 3);
    b.rect(4, 17, 8, 4, 4);
    b.rect(20, 18, 12, 3, 3);
    b.rect(11, 31, 4, 16, 1);
    b.rect(17, 31, 4, 16, 1);
    b.rect(9, 46, 7, 5, 5);
    b.rect(17, 46, 7, 5, 5);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 2);
    b.ellipse(5, 5, 2, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 13), gs::rgb4(3, 5, 4), gs::rgb4(1, 2, 1)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(1, 1, 1));
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 3, 1), gs::rgb4(9, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(4, 2, 1),
                           gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(4, 2, 1), gs::rgb4(1, 5, 2), gs::rgb4(2, 9, 4), gs::rgb4(8, 12, 6),
                            gs::rgb4(6, 4, 2), gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_CUE, {0, gs::rgb4(14, 13, 8), gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 4), gs::rgb4(6, 3, 1),
                          gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(7, 7, 8), gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 4, 1), gs::rgb4(12, 9, 2), gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 2),
                           gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(3, 2, 2), gs::rgb4(11, 8, 5), gs::rgb4(2, 3, 7), gs::rgb4(5, 3, 2),
                             gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_DEAD, {0, gs::rgb4(8, 2, 2), gs::rgb4(12, 4, 3), gs::rgb4(4, 1, 1)});

    art.table = gs::uploadMipped(vdp, tableArt());
    art.cue = gs::uploadMipped(vdp, cueArt());
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.player = gs::uploadMipped(vdp, playerArt());
    art.mark = gs::uploadMipped(vdp, markArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 3;
    st.outline = 1;
    st.shadow = 15;
    art.title = gs::uploadImage(vdp, gs::textBitmap("CUE BELL", st));
    loadFont(vdp, art);
}

}  // namespace cuebell
