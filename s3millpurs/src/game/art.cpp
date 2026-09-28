#include "game/art.h"

#include <initializer_list>
#include <string>

namespace millp {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

Bitmap millArt() {
    Bitmap b(40, 72);
    b.poly({{8, 70}, {12, 28}, {28, 28}, {32, 70}}, 1);
    b.rect(14, 18, 12, 14, 2);
    b.rect(16, 22, 8, 8, 6);
    b.rect(16, 40, 8, 16, 4);
    b.rect(10, 12, 20, 8, 3);
    b.rect(17, 2, 6, 12, 5);
    b.ellipse(20, 3, 5, 3, 2);
    b.rect(6, 66, 28, 4, 7);
    return b;
}

Bitmap sailA() {
    Bitmap b(64, 64);
    b.rect(30, 4, 4, 56, 1);
    b.rect(4, 30, 56, 4, 1);
    b.rect(28, 8, 8, 10, 2);
    b.rect(28, 46, 8, 10, 2);
    b.rect(8, 28, 10, 8, 2);
    b.rect(46, 28, 10, 8, 2);
    b.ellipse(32, 32, 4, 4, 3);
    return b;
}

Bitmap sailB() {
    Bitmap b(64, 64);
    b.line(10, 10, 54, 54, 1, 4);
    b.line(54, 10, 10, 54, 1, 4);
    b.rect(18, 16, 8, 8, 2);
    b.rect(38, 40, 8, 8, 2);
    b.rect(38, 16, 8, 8, 2);
    b.rect(18, 40, 8, 8, 2);
    b.ellipse(32, 32, 4, 4, 3);
    return b;
}

Bitmap thresherArt() {
    Bitmap b(48, 28);
    b.rect(6, 4, 28, 10, 1);
    b.rect(8, 6, 10, 5, 3);
    b.rect(20, 6, 10, 5, 6);
    b.rect(4, 14, 36, 8, 2);
    b.rect(34, 8, 10, 8, 4);
    b.ellipse(12, 24, 4, 4, 7);
    b.ellipse(28, 24, 4, 4, 7);
    b.rect(2, 12, 4, 8, 5);
    return b;
}

Bitmap tractorArt() {
    Bitmap b(44, 32);
    b.rect(14, 2, 14, 10, 2);
    b.rect(16, 4, 8, 6, 6);
    b.rect(6, 12, 30, 10, 1);
    b.rect(28, 8, 8, 6, 4);
    b.ellipse(12, 26, 6, 6, 7);
    b.ellipse(30, 26, 5, 5, 5);
    b.ellipse(12, 26, 2, 2, 3);
    b.rect(2, 14, 6, 4, 5);
    return b;
}

Bitmap stoneArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 3);
    return b;
}

Bitmap dustArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 3, 1);
    b.ellipse(4, 4, 2, 2, 2);
    return b;
}

Bitmap stalkArt() {
    Bitmap b(8, 20);
    b.line(4, 18, 4, 4, 1, 1.5f);
    b.ellipse(4, 3, 2, 3, 2);
    b.line(4, 10, 1, 6, 1, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WHEAT, {0, gs::rgb4(15, 13, 5), gs::rgb4(10, 8, 2), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 3), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_THRESH, {0, gs::rgb4(12, 8, 3), gs::rgb4(7, 5, 2), gs::rgb4(15, 12, 6), gs::rgb4(4, 3, 2),
                             gs::rgb4(9, 9, 8), gs::rgb4(3, 6, 10), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_TRACTOR, {0, gs::rgb4(6, 9, 4), gs::rgb4(3, 5, 2), gs::rgb4(12, 13, 8), gs::rgb4(8, 4, 2),
                              gs::rgb4(2, 2, 2), gs::rgb4(10, 14, 15), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(11, 8, 5), gs::rgb4(7, 5, 3), gs::rgb4(14, 12, 8), gs::rgb4(5, 3, 2),
                           gs::rgb4(9, 9, 8), gs::rgb4(4, 8, 12), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_SAIL, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 3), gs::rgb4(13, 12, 9)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(13, 11, 7), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(10, 10, 9), gs::rgb4(5, 5, 5), gs::rgb4(14, 6, 3)});
    vdp.setFogColor(gs::rgb4(8, 7, 5));
    loadFont(vdp, art);
    art.mill = gs::uploadMipped(vdp, millArt());
    art.sailA = gs::uploadMipped(vdp, sailA());
    art.sailB = gs::uploadMipped(vdp, sailB());
    art.thresher = gs::uploadMipped(vdp, thresherArt());
    art.tractor = gs::uploadMipped(vdp, tractorArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.stalk = gs::uploadMipped(vdp, stalkArt());
}

}  // namespace millp
