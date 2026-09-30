#include "game/art.h"

#include <cmath>

namespace culvertdawn {
namespace {

void putPal(gs::VDP& v, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) v.setColor(pal * 16 + i, i < n ? c[i] : 0);
}

void loadFont(gs::VDP& v, Art& art) {
    gs::TileAlloc tiles(v, 1);
    for (int ch = 0; ch < 96; ch++) {
        const uint8_t* g = gs::glyph(char(32 + ch));
        uint8_t px[64] = {};
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
        art.font[ch] = tiles.shared(px);
    }
}

gs::Bitmap culvertBmp() {
    gs::Bitmap b(320, 224);
    auto stone = [&](int x, int y) {
        int rib = ((x / 8) + (y / 10)) & 1;
        int c = rib ? 2 : 1;
        if (((x * 3 + y * 5) % 37) == 0) c = 3;
        if ((y % 18) == 0 || (x % 22) == 0) c = 4;
        return c;
    };
    for (int y = 0; y < 48; y++)
        for (int x = 0; x < 320; x++) b.set(x, y, stone(x, y));
    for (int y = 176; y < 224; y++)
        for (int x = 0; x < 320; x++) {
            int c = stone(x, y);
            if (y < 182) c = 5;
            b.set(x, y, c);
        }
    for (int y = 48; y < 176; y++) {
        for (int x = 0; x < 22; x++) b.set(x, y, stone(x, y));
        for (int x = 298; x < 320; x++) b.set(x, y, stone(x, y));
    }
    // Barrel ribs across the opening, thin so the night still shows.
    for (int i = 0; i < 5; i++) {
        int y = 52 + i * 24;
        for (int x = 22; x < 298; x++) {
            b.set(x, y, 4);
            if (i == 0) b.set(x, y + 1, 3);
        }
    }
    // A seam of moss where the pipe meets the water table.
    for (int x = 22; x < 298; x++)
        if ((x % 5) != 0) b.set(x, 175, 6);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 11.f, 11.f, 1);
    b.ellipse(18, 12, 8.f, 8.f, 0);
    b.set(8, 16, 2);
    b.set(11, 18, 2);
    b.set(9, 11, 2);
    return b;
}

gs::Bitmap starBmp() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

gs::Bitmap potBmp() {
    gs::Bitmap b(16, 18);
    b.rect(4, 2, 8, 3, 1);
    b.rect(3, 5, 10, 10, 2);
    b.rect(4, 6, 8, 8, 3);
    b.rect(5, 14, 6, 3, 4);
    b.set(7, 1, 5);
    b.set(8, 1, 5);
    return b;
}

gs::Bitmap flameBmp() {
    gs::Bitmap b(12, 28);
    b.ellipse(6, 16, 4.2f, 9.f, 1);
    b.ellipse(6, 18, 2.4f, 6.f, 2);
    b.ellipse(6, 20, 1.2f, 3.2f, 3);
    b.line(6, 4, 6, 12, 4, 1);
    b.set(6, 3, 4);
    return b;
}

gs::Bitmap ashBmp() {
    gs::Bitmap b(12, 16);
    b.line(6, 2, 5, 14, 1, 1);
    b.line(6, 2, 8, 13, 2, 1);
    b.set(6, 2, 1);
    return b;
}

gs::Bitmap watchBmp(int frame) {
    gs::Bitmap b(20, 36);
    b.ellipse(10, 6, 4.f, 4.4f, 3);
    b.rect(8, 9, 4, 2, 3);
    b.rect(6, 12, 8, 12, 1);
    b.rect(5, 14, 2, 8, 2);
    int arm = frame ? 3 : 0;
    b.rect(13, 13 + arm, 2, 8, 2);
    b.rect(7, 23, 3, 9, 4);
    b.rect(11, 23, 3, 9, 4);
    b.rect(6, 31, 4, 3, 5);
    b.rect(11, 31, 4, 3, 5);
    // A short striker in the right hand.
    b.line(15, 16 + arm, 18, 12 + arm, 6, 1);
    b.set(9, 5, 7);
    b.set(12, 5, 7);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 14, 10), gs::rgb4(4, 5, 6)};
    const uint16_t stone[] = {0,
                              gs::rgb4(5, 5, 5),
                              gs::rgb4(7, 7, 6),
                              gs::rgb4(3, 3, 3),
                              gs::rgb4(9, 9, 8),
                              gs::rgb4(4, 4, 3),
                              gs::rgb4(3, 6, 3)};
    const uint16_t watch[] = {0,
                              gs::rgb4(3, 4, 5),
                              gs::rgb4(2, 3, 4),
                              gs::rgb4(12, 9, 7),
                              gs::rgb4(2, 2, 3),
                              gs::rgb4(1, 1, 1),
                              gs::rgb4(10, 9, 6),
                              gs::rgb4(2, 2, 2)};
    const uint16_t flare[] = {0, gs::rgb4(14, 4, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 8), gs::rgb4(15, 15, 14)};
    const uint16_t night[] = {0, gs::rgb4(14, 14, 11), gs::rgb4(8, 8, 7)};
    const uint16_t ash[] = {0, gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 3)};
    const uint16_t pot[] = {0,
                            gs::rgb4(8, 7, 5),
                            gs::rgb4(5, 4, 3),
                            gs::rgb4(3, 3, 2),
                            gs::rgb4(2, 2, 2),
                            gs::rgb4(12, 8, 3)};
    putPal(vdp, PAL_HUD, hud, 3);
    putPal(vdp, PAL_STONE, stone, 7);
    putPal(vdp, PAL_WATCH, watch, 8);
    putPal(vdp, PAL_FLARE, flare, 5);
    putPal(vdp, PAL_NIGHT, night, 3);
    putPal(vdp, PAL_ASH, ash, 3);
    // Pots share the flare bank's spare slots — they are drawn with PAL_FLARE
    // only for the wick. The pot itself uses a dedicated upload palette.
    putPal(vdp, 6, pot, 6);

    loadFont(vdp, art);
    art.culvert = gs::uploadImage(vdp, culvertBmp());
    art.moon = gs::uploadImage(vdp, moonBmp());
    art.star = gs::uploadImage(vdp, starBmp());
    art.pot = gs::uploadImage(vdp, potBmp());
    art.flame = gs::uploadImage(vdp, flameBmp());
    art.ash = gs::uploadImage(vdp, ashBmp());
    art.watch[0] = gs::uploadImage(vdp, watchBmp(0));
    art.watch[1] = gs::uploadImage(vdp, watchBmp(1));

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    art.title = gs::uploadImage(vdp, gs::textBitmap("UNTIL DAWN", st));
}

}  // namespace culvertdawn
