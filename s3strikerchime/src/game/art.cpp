#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace strikerchime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

gs::Bitmap towerArt() {
    gs::Bitmap b(36, 150);
    b.rect(4, 0, 28, 150, 1);
    b.rect(8, 6, 6, 138, 2);
    b.rect(22, 6, 6, 138, 2);
    for (int y = 12; y < 140; y += 16) b.rect(10, y, 16, 3, 3);
    b.rect(2, 142, 32, 8, 4);
    b.rect(14, 2, 8, 6, 5);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(40, 36);
    b.rect(18, 0, 4, 6, 3);
    b.poly({{8, 8}, {32, 8}, {38, 24}, {2, 24}}, 1);
    b.ellipse(20, 24, 18.f, 7.f, 1);
    b.ellipse(20, 24, 7.f, 3.f, 2);
    b.ellipse(20, 16, 3.f, 4.f, 3);
    b.rect(0, 30, 40, 3, 4);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 6, 7.f, 5.f, 1);
    b.ellipse(6, 5, 3.f, 2.f, 2);
    b.rect(3, 8, 10, 2, 3);
    return b;
}

gs::Bitmap manArt() {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 8, 7.f, 7.f, 1);
    b.rect(16, 6, 8, 4, 2);
    b.rect(12, 16, 16, 22, 3);
    b.rect(8, 18, 6, 16, 1);
    b.rect(26, 18, 6, 16, 1);
    b.rect(14, 38, 5, 20, 4);
    b.rect(21, 38, 5, 20, 4);
    b.rect(12, 56, 8, 5, 5);
    b.rect(20, 56, 8, 5, 5);
    return b;
}

gs::Bitmap malletArt() {
    gs::Bitmap b(46, 14);
    b.rect(0, 5, 30, 4, 1);
    b.rect(28, 1, 16, 12, 2);
    b.rect(30, 3, 12, 3, 3);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 30.f, 30.f, 1);
    b.ellipse(32, 32, 24.f, 24.f, 2);
    b.ellipse(32, 32, 3.f, 3.f, 3);
    b.rect(31, 8, 2, 6, 3);
    b.rect(31, 50, 2, 6, 3);
    b.rect(8, 31, 6, 2, 3);
    b.rect(50, 31, 6, 2, 3);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(4, 22);
    b.rect(1, 0, 2, 20, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.f, 4.f, 1);
    b.ellipse(4, 4, 1.6f, 1.6f, 2);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12.f, 12.f, 1);
    b.ellipse(18, 12, 8.f, 8.f, 2);
    return b;
}

gs::Bitmap groundArt() {
    gs::Bitmap b(80, 16);
    b.rect(0, 0, 80, 16, 1);
    b.rect(0, 0, 80, 3, 2);
    for (int x = 4; x < 78; x += 14) b.rect(x, 6, 6, 3, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 5)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 6, 2), gs::rgb4(6, 3, 1), gs::rgb4(14, 11, 5), gs::rgb4(4, 2, 1),
                           gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 12, 4), gs::rgb4(10, 7, 2), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(13, 8, 5), gs::rgb4(3, 2, 2), gs::rgb4(12, 2, 3), gs::rgb4(2, 2, 6),
                          gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(14, 13, 8), gs::rgb4(1, 1, 4), gs::rgb4(8, 10, 4)});
    setPal(vdp, PAL_PUCK, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 10, 8), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(12, 10, 7), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 3)});

    art.tower = gs::uploadMipped(vdp, towerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.man = gs::uploadMipped(vdp, manArt());
    art.mallet = gs::uploadMipped(vdp, malletArt());
    art.face = gs::uploadMipped(vdp, faceArt());
    art.hand = gs::uploadMipped(vdp, handArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.ground = gs::uploadMipped(vdp, groundArt());

    gs::TextStyle st;
    st.scale = 3;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    art.title = gs::uploadImage(vdp, gs::textBitmap("STRIKER", st));
    st.scale = 1;
    st.outline = 0;
    art.rule = gs::uploadImage(vdp, gs::textBitmap("THE HOUR HAS TO CHIME", st));
    loadFont(vdp, art);
}

}  // namespace strikerchime
