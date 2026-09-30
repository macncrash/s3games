#include "game/art.h"

#include <initializer_list>

namespace bellmark {
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

gs::Bitmap towerArt() {
    gs::Bitmap b(96, 140);
    b.rect(8, 0, 80, 140, 2);
    b.rect(16, 8, 64, 124, 3);
    b.rect(28, 18, 16, 22, 4);
    b.rect(52, 18, 16, 22, 4);
    b.rect(28, 50, 16, 22, 4);
    b.rect(52, 50, 16, 22, 4);
    b.rect(36, 92, 24, 36, 1);
    b.rect(44, 100, 8, 16, 5);
    for (int i = 0; i < 8; i++) b.rect(10 + i * 10, 132, 6, 6, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(72, 64);
    b.rect(32, 0, 8, 8, 4);
    b.ellipse(36, 28, 22, 20, 2);
    b.ellipse(36, 26, 14, 12, 3);
    b.rect(16, 36, 40, 16, 2);
    b.rect(12, 48, 48, 8, 1);
    b.rect(18, 50, 36, 4, 5);
    b.ellipse(36, 18, 5, 4, 6);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 0, 2, 16, 1);
    b.ellipse(5, 22, 4, 5, 2);
    b.ellipse(4, 20, 2, 2, 3);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(8, 90);
    b.rect(3, 0, 2, 78, 2);
    b.ellipse(4, 82, 3, 5, 3);
    b.rect(2, 78, 4, 3, 1);
    return b;
}

gs::Bitmap notchArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 2);
    b.rect(6, 2, 2, 10, 1);
    b.rect(2, 6, 10, 2, 3);
    return b;
}

gs::Bitmap ringerArt() {
    gs::Bitmap b(36, 70);
    b.ellipse(18, 9, 7, 7, 2);
    b.rect(12, 17, 12, 20, 3);
    b.rect(4, 18, 8, 5, 4);
    b.rect(24, 20, 8, 14, 3);
    b.rect(12, 37, 5, 22, 1);
    b.rect(19, 37, 5, 22, 1);
    b.rect(10, 57, 8, 6, 4);
    b.rect(20, 57, 8, 6, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 10), gs::rgb4(5, 4, 6), gs::rgb4(1, 1, 2)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 3));
    setPal(vdp, PAL_STONE, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(1, 1, 2),
                            gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 4, 1), gs::rgb4(11, 8, 2), gs::rgb4(14, 11, 4), gs::rgb4(4, 3, 1),
                           gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 4), gs::rgb4(12, 9, 5)});
    setPal(vdp, PAL_CLAP, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(13, 13, 14)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(10, 7, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_RINGER, {0, gs::rgb4(3, 2, 2), gs::rgb4(11, 8, 6), gs::rgb4(3, 4, 8), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_YOKE, {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(3, 2, 1)});

    art.tower = gs::uploadMipped(vdp, towerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.notch = gs::uploadMipped(vdp, notchArt());
    art.ringer = gs::uploadMipped(vdp, ringerArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 3;
    st.outline = 1;
    st.shadow = 15;
    art.title = gs::uploadImage(vdp, gs::textBitmap("BELL", st));

    loadFont(vdp, art);
}

}  // namespace bellmark
