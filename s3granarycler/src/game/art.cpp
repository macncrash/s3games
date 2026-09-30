#include "game/art.h"

#include <initializer_list>
#include <string>

namespace gcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(40, 60);
    b.ellipse(20, 8, 10, 5, 6);          // straw hat
    b.ellipse(20, 16, 7, 8, 5);          // face
    b.rect(16, 13, 8, 3, 7);
    b.set(17, 16, 8);
    b.set(23, 16, 8);
    b.poly({{12, 24}, {28, 24}, {30, 44}, {10, 44}}, 1);  // smock
    b.rect(16, 26, 8, 10, 3);            // apron
    b.line(28, 32, 36, 46, 2, 2.f);
    if (step == 0) {
        b.rect(13, 44, 5, 12, 4);
        b.rect(22, 44, 5, 10, 4);
        b.rect(11, 54, 8, 3, 2);
        b.rect(20, 52, 8, 3, 2);
    } else {
        b.rect(13, 44, 5, 10, 4);
        b.rect(22, 44, 5, 12, 4);
        b.rect(12, 52, 8, 3, 2);
        b.rect(20, 54, 8, 3, 2);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap scoopArt() {
    gs::Bitmap b(30, 42);
    b.line(6, 4, 18, 26, 3, 2.2f);
    b.poly({{14, 22}, {28, 24}, {26, 36}, {12, 32}}, 1);
    b.line(16, 26, 24, 30, 4, 1.2f);
    b.outline(5, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(26, 30);
    b.poly({{6, 8}, {20, 6}, {23, 26}, {4, 27}}, 1);
    b.rect(8, 4, 10, 6, 2);
    b.line(9, 14, 20, 13, 3, 1.2f);
    b.ellipse(13, 18, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap spillArt() {
    gs::Bitmap b(32, 16);
    b.ellipse(16, 9, 14, 5, 1);
    b.ellipse(10, 8, 4, 2, 2);
    b.ellipse(20, 10, 5, 2, 3);
    b.ellipse(15, 7, 2, 1, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap baleArt() {
    gs::Bitmap b(34, 22);
    b.poly({{3, 6}, {30, 4}, {32, 16}, {5, 19}}, 1);
    b.line(4, 10, 31, 8, 2, 1.3f);
    b.line(8, 5, 10, 18, 3, 1.2f);
    b.line(18, 4, 19, 17, 3, 1.2f);
    b.line(26, 4, 27, 16, 3, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap chaffArt() {
    gs::Bitmap b(22, 18);
    b.line(4, 14, 10, 4, 1, 1.4f);
    b.line(10, 14, 14, 3, 2, 1.4f);
    b.line(16, 14, 18, 6, 3, 1.4f);
    b.ellipse(11, 15, 8, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(28, 120);
    b.rect(8, 0, 12, 120, 1);
    b.rect(8, 0, 4, 120, 2);
    for (int y = 8; y < 120; y += 16) b.rect(8, float(y), 12, 2, 3);
    b.rect(4, 18, 20, 6, 4);
    b.rect(2, 108, 24, 8, 5);
    return b;
}

gs::Bitmap loftArt() {
    gs::Bitmap b(80, 28);
    b.rect(0, 8, 80, 10, 1);
    b.rect(0, 8, 80, 3, 2);
    for (int x = 6; x < 80; x += 14) b.rect(float(x), 8, 2, 10, 3);
    b.poly({{0, 0}, {80, 0}, {76, 8}, {4, 8}}, 4);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(11, 11, 7, 7, 2);
    b.line(11, 11, 11, 5, 3, 1.4f);
    b.line(11, 11, 16, 13, 4, 1.4f);
    b.outline(5, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 11, 3, 1);
    return b;
}

gs::Bitmap moteArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(3, 3, 1, 1, 2);
    return b;
}

void glyphs(gs::VDP& vdp, Art& art) {
    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.outline = 0;
    st.spacing = 1;
    for (int c = 32; c <= 126; c++) {
        std::string s(1, char(c));
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(s, st));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(6, 4, 2));
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 10), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(11, 7, 3),
                             gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(12, 11, 8), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 6), gs::rgb4(4, 3, 2), gs::rgb4(13, 9, 6),
            gs::rgb4(14, 11, 3), gs::rgb4(9, 6, 3), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_SACK, {0, gs::rgb4(10, 8, 4), gs::rgb4(7, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(13, 10, 4),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GRAIN, {0, gs::rgb4(14, 12, 4), gs::rgb4(12, 9, 2), gs::rgb4(15, 14, 8), gs::rgb4(9, 6, 1),
                            gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_STRAW, {0, gs::rgb4(13, 11, 3), gs::rgb4(10, 8, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 13, 6),
                            gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_FLOOR, {0, gs::rgb4(2, 2, 1), gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BEAM, {0, gs::rgb4(7, 4, 1), gs::rgb4(10, 6, 2), gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 3),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 6), gs::rgb4(3, 8, 3), gs::rgb4(12, 15, 8)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(10, 2, 1), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_LOFT, {0, gs::rgb4(9, 6, 2), gs::rgb4(14, 10, 4), gs::rgb4(5, 3, 1)});

    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.scoop = gs::uploadMipped(vdp, scoopArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.spill = gs::uploadMipped(vdp, spillArt());
    art.bale = gs::uploadMipped(vdp, baleArt());
    art.chaff = gs::uploadMipped(vdp, chaffArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.loft = gs::uploadMipped(vdp, loftArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mote = gs::uploadMipped(vdp, moteArt());
    glyphs(vdp, art);
}

}  // namespace gcler
