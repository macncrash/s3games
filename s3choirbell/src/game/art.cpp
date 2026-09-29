#include "game/art.h"

#include <initializer_list>

namespace choir {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap archArt() {
    Bitmap b(96, 160);
    b.rect(8, 28, 80, 132, 2);
    b.rect(16, 36, 64, 116, 1);
    b.ellipse(48, 48, 36, 28, 2);
    b.ellipse(48, 50, 28, 20, 3);
    b.rect(40, 20, 16, 22, 2);
    b.rect(44, 8, 8, 16, 4);
    b.rect(0, 148, 96, 12, 5);
    b.outline(6, false);
    return b;
}

Bitmap glassArt() {
    Bitmap b(48, 72);
    b.ellipse(24, 22, 20, 18, 1);
    b.rect(6, 22, 36, 46, 1);
    b.rect(22, 8, 4, 60, 2);
    b.rect(8, 40, 32, 3, 2);
    b.ellipse(24, 28, 8, 10, 3);
    b.ellipse(14, 50, 5, 7, 4);
    b.ellipse(34, 50, 5, 7, 5);
    b.rect(20, 48, 8, 16, 6);
    return b;
}

Bitmap columnArt() {
    Bitmap b(28, 150);
    b.rect(6, 16, 16, 120, 1);
    b.rect(8, 16, 6, 120, 2);
    b.rect(2, 8, 24, 12, 3);
    b.rect(2, 132, 24, 14, 3);
    b.rect(0, 140, 28, 8, 4);
    return b;
}

Bitmap pewArt() {
    Bitmap b(280, 28);
    b.rect(0, 8, 280, 14, 1);
    b.rect(0, 6, 280, 5, 2);
    for (int i = 0; i < 8; i++) b.rect(8 + i * 34, 14, 6, 12, 3);
    b.rect(0, 20, 280, 4, 4);
    return b;
}

Bitmap floorArt() {
    Bitmap b(320, 36);
    b.rect(0, 0, 320, 36, 1);
    for (int x = 0; x < 320; x += 20) b.rect(x, 0, 1, 36, 2);
    b.rect(140, 0, 40, 36, 3);
    return b;
}

Bitmap robeArt(bool alt) {
    Bitmap b(40, 78);
    b.poly({{20, 4}, {36, 74}, {4, 74}}, alt ? 2 : 1);
    b.poly({{20, 10}, {30, 70}, {10, 70}}, alt ? 3 : 4);
    b.rect(16, 8, 8, 16, 5);
    b.rect(8, 28, 10, 18, 6);
    b.rect(22, 30, 10, 16, 6);
    b.ellipse(14, 72, 6, 4, 7);
    b.ellipse(26, 72, 6, 4, 7);
    return b;
}

Bitmap headArt() {
    Bitmap b(22, 26);
    b.ellipse(11, 12, 9, 10, 1);
    b.ellipse(11, 8, 8, 5, 2);
    b.rect(6, 4, 10, 4, 2);
    b.ellipse(8, 12, 1, 1, 3);
    b.ellipse(14, 12, 1, 1, 3);
    b.rect(9, 16, 4, 1, 4);
    b.ellipse(11, 22, 5, 3, 1);
    return b;
}

Bitmap bookArt() {
    Bitmap b(18, 12);
    b.rect(0, 2, 18, 8, 1);
    b.rect(8, 2, 2, 8, 2);
    b.rect(1, 3, 6, 1, 3);
    b.rect(11, 3, 6, 1, 3);
    return b;
}

Bitmap bellArt() {
    Bitmap b(44, 40);
    b.poly({{8, 6}, {36, 6}, {40, 28}, {4, 28}}, 1);
    b.poly({{12, 8}, {32, 8}, {34, 24}, {10, 24}}, 2);
    b.ellipse(22, 30, 18, 8, 1);
    b.ellipse(22, 28, 12, 5, 3);
    b.rect(20, 0, 4, 8, 4);
    b.rect(10, 14, 4, 8, 5);
    return b;
}

Bitmap clapperArt() {
    Bitmap b(8, 16);
    b.rect(3, 0, 2, 8, 1);
    b.ellipse(4, 12, 3, 3, 2);
    return b;
}

Bitmap noteArt() {
    Bitmap b(16, 28);
    b.ellipse(5, 22, 5, 4, 1);
    b.rect(9, 4, 2, 18, 1);
    b.rect(9, 4, 6, 3, 1);
    b.rect(11, 2, 3, 2, 2);
    return b;
}

Bitmap candleArt() {
    Bitmap b(10, 24);
    b.rect(3, 10, 4, 12, 1);
    b.ellipse(5, 8, 2, 3, 2);
    b.ellipse(5, 6, 1, 2, 3);
    b.rect(2, 21, 6, 2, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(12, 3, 3), gs::rgb4(4, 8, 6),
                          gs::rgb4(8, 8, 10)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6),
                            gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_GLASS, {0, gs::rgb4(2, 4, 10), gs::rgb4(14, 12, 6), gs::rgb4(12, 4, 6), gs::rgb4(4, 10, 6),
                            gs::rgb4(10, 8, 2), gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_ROBE, {0, gs::rgb4(3, 3, 8), gs::rgb4(8, 2, 3), gs::rgb4(5, 5, 12), gs::rgb4(12, 3, 4),
                           gs::rgb4(14, 12, 8), gs::rgb4(10, 8, 5), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_ROBE2, {0, gs::rgb4(2, 6, 4), gs::rgb4(6, 3, 8), gs::rgb4(4, 9, 6), gs::rgb4(9, 5, 12),
                            gs::rgb4(14, 12, 8), gs::rgb4(10, 8, 5), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(12, 8, 2), gs::rgb4(15, 12, 4), gs::rgb4(8, 5, 1), gs::rgb4(5, 4, 3),
                           gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_SKIN, {0, gs::rgb4(13, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 3), gs::rgb4(10, 4, 4)});
    setPal(vdp, PAL_NOTE, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 3)});

    a.arch = gs::uploadMipped(vdp, archArt());
    a.glass = gs::uploadMipped(vdp, glassArt());
    a.column = gs::uploadMipped(vdp, columnArt());
    a.pew = gs::uploadMipped(vdp, pewArt());
    a.floor = gs::uploadMipped(vdp, floorArt());
    a.robe = gs::uploadMipped(vdp, robeArt(false));
    a.robeAlt = gs::uploadMipped(vdp, robeArt(true));
    a.head = gs::uploadMipped(vdp, headArt());
    a.book = gs::uploadMipped(vdp, bookArt());
    a.bell = gs::uploadMipped(vdp, bellArt());
    a.clapper = gs::uploadMipped(vdp, clapperArt());
    a.note = gs::uploadMipped(vdp, noteArt());
    a.candle = gs::uploadMipped(vdp, candleArt());

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    for (int c = 32; c < 128; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), st));

    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = y < 90 ? gs::rgb4(1, 1, 4) : gs::rgb4(2, 2, 3);
        if (y > 150) sky = gs::rgb4(3, 2, 2);
        vdp.lineBackdrop[y] = sky;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 1, 3));
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    vdp.hudEnabled = false;
}

}  // namespace choir
