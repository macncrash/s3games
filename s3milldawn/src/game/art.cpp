#include "game/art.h"

#include <string>

namespace mill {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap millArt(int turn) {
    Bitmap b(88, 110);
    b.rect(30, 48, 28, 54, 3);
    b.rect(32, 50, 24, 50, 2);
    b.rect(40, 78, 8, 20, 5);
    b.rect(36, 58, 6, 7, 6);
    b.rect(50, 58, 6, 7, 4);
    b.poly({{26, 50}, {44, 28}, {62, 50}}, 7);
    b.poly({{30, 48}, {44, 32}, {58, 48}}, 8);
    b.ellipse(44, 36, 5, 4, 4);
    const int a = turn ? 9 : 10;
    const int c = turn ? 10 : 9;
    b.rect(40, 4, 8, 30, a);
    b.rect(40, 40, 8, 28, a);
    b.rect(6, 32, 32, 7, c);
    b.rect(50, 32, 32, 7, c);
    b.rect(16, 14, 18, 5, a);
    b.rect(52, 46, 18, 5, c);
    b.outline(15, false);
    return b;
}

Bitmap manArt(bool stoke) {
    Bitmap b(36, 48);
    b.ellipse(16, 7, 5, 5, 4);
    b.rect(12, 4, 8, 3, 6);
    b.rect(13, 12, 7, 11, 2);
    b.rect(11, 23, 12, 8, 3);
    b.rect(12, 31, 4, 11, 5);
    b.rect(18, 31, 4, 11, 5);
    b.rect(11, 41, 6, 3, 7);
    b.rect(18, 41, 6, 3, 7);
    if (stoke) {
        b.rect(20, 16, 12, 3, 3);
        b.rect(30, 12, 3, 10, 8);
        b.ellipse(31, 10, 3, 3, 9);
    } else {
        b.rect(8, 16, 4, 10, 3);
        b.rect(21, 16, 4, 10, 3);
        b.rect(22, 24, 3, 8, 8);
    }
    b.outline(15, false);
    return b;
}

Bitmap flameArt(int low) {
    Bitmap b(20, 28);
    if (low) {
        b.ellipse(10, 18, 4, 5, 3);
        b.ellipse(10, 17, 2, 3, 1);
        b.rect(9, 22, 2, 4, 4);
    } else {
        b.poly({{10, 2}, {16, 16}, {10, 14}, {4, 16}}, 2);
        b.poly({{10, 6}, {14, 16}, {10, 14}, {6, 16}}, 3);
        b.ellipse(10, 16, 3, 5, 1);
        b.rect(9, 20, 2, 6, 4);
    }
    return b;
}

Bitmap stakeArt() {
    Bitmap b(16, 28);
    b.rect(7, 4, 3, 20, 2);
    b.poly({{3, 26}, {8, 16}, {13, 26}}, 3);
    b.rect(6, 2, 5, 4, 4);
    b.outline(15, false);
    return b;
}

Bitmap moonArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 1);
    b.ellipse(18, 12, 8, 8, 0);
    b.ellipse(10, 10, 2, 2, 2);
    b.ellipse(12, 16, 1, 1, 2);
    return b;
}

Bitmap starArt() {
    Bitmap b(7, 7);
    b.rect(3, 0, 1, 7, 1);
    b.rect(0, 3, 7, 1, 1);
    b.set(2, 2, 1);
    b.set(4, 2, 1);
    b.set(2, 4, 1);
    b.set(4, 4, 1);
    return b;
}

Bitmap gustArt() {
    Bitmap b(28, 10);
    b.rect(1, 2, 10, 2, 1);
    b.rect(8, 5, 12, 2, 2);
    b.rect(16, 2, 10, 2, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (!g) continue;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    const auto C = gs::rgb4;
    setPal(vdp, PAL_HUD, {0, C(15, 14, 10), C(15, 10, 3), C(8, 5, 2), C(4, 10, 6), C(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, C(2, 1, 2)});
    setPal(vdp, PAL_MILL,
           {0, C(12, 11, 9), C(8, 7, 6), C(5, 4, 4), C(9, 10, 12), C(3, 2, 2), C(6, 8, 11), C(7, 4, 3), C(10, 6, 4),
            C(13, 12, 9), C(9, 8, 6), 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_MAN,
           {0, C(14, 13, 11), C(4, 5, 9), C(11, 8, 5), C(13, 10, 7), C(3, 3, 5), C(2, 2, 3), C(1, 1, 2), C(6, 4, 2),
            C(15, 12, 4), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_FLAME, {0, C(15, 15, 12), C(15, 10, 2), C(14, 6, 1), C(12, 4, 1), C(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, C(2, 1, 0)});
    setPal(vdp, PAL_YARD, {0, C(10, 12, 14), C(7, 8, 10), C(5, 4, 3), C(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_MOON, {0, C(14, 14, 12), C(10, 10, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    a.mill[0] = gs::uploadMipped(vdp, millArt(0));
    a.mill[1] = gs::uploadMipped(vdp, millArt(1));
    a.man[0] = gs::uploadMipped(vdp, manArt(false));
    a.man[1] = gs::uploadMipped(vdp, manArt(true));
    a.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    a.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    a.stake = gs::uploadMipped(vdp, stakeArt());
    a.moon = gs::uploadMipped(vdp, moonArt());
    a.star = gs::uploadMipped(vdp, starArt());
    a.gust = gs::uploadMipped(vdp, gustArt());
    loadFont(vdp, a);
    vdp.setFogColor(C(2, 2, 6));
}

}  // namespace mill
