#include "game/art.h"

#include <initializer_list>

namespace inkwellmark {
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

gs::Bitmap deskArt() {
    gs::Bitmap b(200, 48);
    b.rect(0, 8, 200, 36, 2);
    b.rect(0, 8, 200, 6, 3);
    b.rect(8, 16, 184, 4, 4);
    b.line(20, 22, 180, 40, 1, 1);
    b.line(40, 22, 160, 42, 5, 1);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(56, 48);
    b.ellipse(28, 16, 22, 10, 3);
    b.ellipse(28, 16, 16, 7, 2);
    b.ellipse(28, 16, 10, 4, 1);
    b.rect(10, 16, 36, 18, 4);
    b.ellipse(28, 34, 22, 10, 3);
    b.ellipse(28, 34, 16, 6, 5);
    b.rect(22, 4, 4, 8, 4);
    return b;
}

gs::Bitmap quillArt() {
    gs::Bitmap b(18, 96);
    b.line(9, 4, 8, 78, 2, 3);
    b.line(9, 8, 12, 40, 3, 2);
    b.ellipse(9, 8, 5, 7, 4);
    b.rect(7, 78, 4, 10, 1);
    b.rect(8, 86, 2, 8, 5);
    return b;
}

gs::Bitmap pageArt() {
    gs::Bitmap b(120, 90);
    b.rect(4, 2, 108, 84, 1);
    b.rect(8, 6, 96, 76, 2);
    b.line(16, 18, 96, 18, 3, 1);
    b.line(16, 30, 90, 30, 3, 1);
    b.line(16, 42, 92, 42, 3, 1);
    b.line(16, 54, 80, 54, 3, 1);
    b.rect(100, 8, 8, 70, 4);
    return b;
}

gs::Bitmap strokeArt() {
    gs::Bitmap b(70, 16);
    b.line(4, 10, 22, 4, 1, 3);
    b.line(22, 4, 40, 12, 1, 3);
    b.line(40, 12, 64, 3, 2, 2);
    b.ellipse(64, 4, 3, 3, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 5, 1);
    b.ellipse(5, 6, 3, 3, 2);
    b.ellipse(11, 10, 2, 2, 2);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(40, 36);
    b.ellipse(16, 18, 12, 10, 2);
    b.rect(22, 8, 8, 16, 3);
    b.rect(26, 4, 6, 12, 2);
    b.rect(8, 22, 18, 8, 1);
    b.rect(4, 26, 8, 6, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 11), gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 3)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(1, 1, 2));
    setPal(vdp, PAL_DESK, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(11, 8, 4),
                           gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 6), gs::rgb4(4, 4, 8), gs::rgb4(8, 8, 10),
                          gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_QUILL, {0, gs::rgb4(1, 1, 2), gs::rgb4(14, 13, 10), gs::rgb4(11, 10, 7), gs::rgb4(15, 15, 13),
                            gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_PAGE, {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 6), gs::rgb4(10, 8, 6)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(8, 5, 3), gs::rgb4(11, 7, 4), gs::rgb4(7, 4, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(8, 6, 2), gs::rgb4(12, 10, 3), gs::rgb4(14, 12, 5), gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_BLOT, {0, gs::rgb4(1, 1, 4), gs::rgb4(3, 3, 8), gs::rgb4(6, 6, 10)});

    art.desk = gs::uploadMipped(vdp, deskArt());
    art.well = gs::uploadMipped(vdp, wellArt());
    art.quill = gs::uploadMipped(vdp, quillArt());
    art.page = gs::uploadMipped(vdp, pageArt());
    art.stroke = gs::uploadMipped(vdp, strokeArt());
    art.blot = gs::uploadMipped(vdp, blotArt());
    art.hand = gs::uploadMipped(vdp, handArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.shadow = 15;
    art.title = gs::uploadImage(vdp, gs::textBitmap("INKWELL", st));

    loadFont(vdp, art);
}

}  // namespace inkwellmark
