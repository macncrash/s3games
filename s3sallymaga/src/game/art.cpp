#include "game/art.h"

#include <initializer_list>
#include <string>

namespace sallymaga {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap raiderArt() {
    gs::Bitmap b(20, 32);
    b.rect(8, 2, 5, 5, 3);
    b.rect(7, 6, 7, 2, 2);
    b.rect(8, 8, 5, 10, 1);
    b.rect(5, 9, 3, 7, 1);
    b.rect(13, 9, 3, 7, 1);
    b.rect(8, 18, 2, 10, 4);
    b.rect(11, 18, 2, 10, 4);
    b.line(4, 4, 16, 14, 5, 1.4f);
    b.rect(3, 3, 3, 3, 6);
    b.set(9, 4, 7);
    b.set(11, 4, 7);
    return b;
}

gs::Bitmap fallenArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(10, 6, 4, 3, 3);
    b.rect(13, 5, 12, 4, 1);
    b.rect(14, 9, 3, 3, 4);
    b.rect(20, 9, 3, 3, 4);
    b.line(22, 2, 30, 8, 5, 1.2f);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(36, 90);
    b.rect(4, 10, 28, 80, 1);
    b.rect(2, 6, 32, 8, 2);
    b.rect(8, 0, 20, 8, 3);
    for (int y = 22; y < 80; y += 16) {
        b.rect(8, y, 6, 8, 4);
        b.rect(22, y, 6, 8, 4);
        b.rect(10, y + 2, 2, 3, 5);
        b.rect(24, y + 2, 2, 3, 5);
    }
    b.rect(14, 62, 8, 18, 6);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(70, 36);
    b.rect(0, 8, 70, 10, 1);
    b.rect(4, 0, 62, 10, 2);
    b.rect(16, 16, 38, 20, 3);
    b.rect(22, 18, 26, 18, 0);
    return b;
}

gs::Bitmap torchArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 12, 2, 14, 1);
    b.ellipse(5, 8, 4, 6, 2);
    b.ellipse(5, 7, 2, 4, 3);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(16, 22);
    b.rect(7, 0, 2, 6, 1);
    b.poly({{2, 6}, {14, 6}, {12, 20}, {4, 20}}, 2);
    b.rect(6, 10, 4, 4, 3);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(16, 16);
    b.rect(7, 1, 2, 4, 1);
    b.rect(7, 11, 2, 4, 1);
    b.rect(1, 7, 4, 2, 1);
    b.rect(11, 7, 4, 2, 1);
    b.set(8, 8, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(18, 14);
    b.ellipse(9, 8, 7, 4, 1);
    b.ellipse(6, 5, 3, 3, 2);
    b.ellipse(12, 5, 3, 3, 3);
    return b;
}

gs::Bitmap roundArt(int fill) {
    gs::Bitmap b(8, 14);
    b.rect(2, 2, 4, 10, fill);
    b.rect(2, 1, 4, 2, 3);
    b.rect(1, 11, 6, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(13, 12, 8), gs::rgb4(4, 3, 3), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(7, 6, 5), gs::rgb4(10, 9, 7), gs::rgb4(5, 4, 4), gs::rgb4(3, 3, 4),
                            gs::rgb4(14, 12, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(6, 4, 3), gs::rgb4(4, 3, 3), gs::rgb4(11, 8, 6), gs::rgb4(3, 3, 4),
                          gs::rgb4(9, 9, 8), gs::rgb4(12, 4, 3), gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_FALL, {0, gs::rgb4(5, 3, 3), gs::rgb4(3, 2, 2), gs::rgb4(8, 6, 5), gs::rgb4(3, 3, 3),
                           gs::rgb4(7, 7, 6), gs::rgb4(8, 3, 2), gs::rgb4(10, 9, 8)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(13, 10, 3), gs::rgb4(6, 5, 3), gs::rgb4(15, 13, 6), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(6, 4, 2), gs::rgb4(14, 8, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 11), gs::rgb4(10, 9, 7), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(6, 5, 3), gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(9, 7, 4),
                           gs::rgb4(3, 4, 2), gs::rgb4(4, 5, 2), gs::rgb4(7, 6, 4), gs::rgb4(10, 8, 5)});
    vdp.setFogColor(gs::rgb4(4, 3, 5));

    art.raider = gs::uploadMipped(vdp, raiderArt());
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.torch = gs::uploadMipped(vdp, torchArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.round = gs::uploadMipped(vdp, roundArt(1));
    art.spent = gs::uploadMipped(vdp, roundArt(2));

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 1;
    for (int c = 32; c < 127; c++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        art.cellW = g.w;
        art.cellH = g.h;
        art.glyph[c - 32] = gs::uploadImage(vdp, g);
    }
}

}  // namespace sallymaga
