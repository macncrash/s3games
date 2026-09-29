#include "game/art.h"

#include <initializer_list>

namespace anvilmark {
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

gs::Bitmap anvilArt() {
    gs::Bitmap b(180, 64);
    b.rect(28, 8, 124, 22, 2);
    b.rect(36, 10, 108, 8, 3);
    b.ellipse(28, 18, 26, 12, 2);
    b.ellipse(22, 16, 10, 6, 4);
    b.rect(48, 28, 84, 10, 1);
    b.rect(62, 38, 56, 22, 2);
    b.rect(70, 42, 40, 14, 5);
    b.rect(40, 30, 14, 6, 4);
    b.rect(126, 30, 14, 6, 4);
    return b;
}

gs::Bitmap hammerArt() {
    gs::Bitmap b(36, 78);
    b.rect(4, 4, 28, 16, 2);
    b.rect(6, 6, 24, 6, 3);
    b.rect(2, 18, 32, 6, 1);
    b.rect(14, 24, 8, 50, 4);
    b.rect(16, 28, 3, 42, 5);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(110, 22);
    b.rect(4, 4, 102, 14, 2);
    b.rect(8, 6, 94, 6, 3);
    b.rect(0, 8, 8, 8, 1);
    b.rect(102, 8, 8, 8, 4);
    return b;
}

gs::Bitmap notchArt() {
    gs::Bitmap b(16, 16);
    b.rect(2, 2, 12, 12, 2);
    b.rect(5, 4, 6, 8, 3);
    b.rect(7, 2, 2, 12, 1);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 2);
    b.ellipse(7, 7, 3, 3, 1);
    b.rect(6, 1, 2, 12, 3);
    b.rect(1, 6, 12, 2, 3);
    return b;
}

gs::Bitmap smithArt() {
    gs::Bitmap b(48, 72);
    b.ellipse(24, 12, 8, 8, 2);
    b.rect(18, 20, 14, 22, 3);
    b.rect(8, 24, 10, 6, 2);
    b.rect(32, 22, 12, 6, 2);
    b.rect(16, 42, 7, 26, 4);
    b.rect(26, 42, 7, 26, 4);
    b.rect(14, 64, 10, 6, 1);
    b.rect(26, 64, 10, 6, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 10), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 3)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 1));
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 10), gs::rgb4(12, 11, 8),
                           gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(8, 2, 1), gs::rgb4(14, 5, 1), gs::rgb4(15, 11, 3), gs::rgb4(6, 2, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(10, 7, 1), gs::rgb4(14, 11, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(10, 6, 3), gs::rgb4(5, 3, 2),
                           gs::rgb4(12, 8, 4)});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(15, 15, 8), gs::rgb4(15, 8, 2), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(3, 2, 4), gs::rgb4(6, 3, 2)});
    setPal(vdp, PAL_SMITH, {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 5, 3), gs::rgb4(4, 3, 5), gs::rgb4(3, 2, 2)});

    art.anvil = gs::uploadMipped(vdp, anvilArt());
    art.hammer = gs::uploadMipped(vdp, hammerArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.notch = gs::uploadMipped(vdp, notchArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.smith = gs::uploadMipped(vdp, smithArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 3;
    st.outline = 1;
    st.shadow = 15;
    art.title = gs::uploadImage(vdp, gs::textBitmap("ANVIL", st));

    loadFont(vdp, art);
}

}  // namespace anvilmark
