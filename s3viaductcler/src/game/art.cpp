#include "game/art.h"

#include <cstdint>
#include <vector>

namespace cler {
namespace {

void setPal(gs::VDP& vdp, int pal, const std::vector<uint16_t>& cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap walkerArt(int fr) {
    gs::Bitmap b(22, 36);
    b.ellipse(11, 6, 4, 4, 4);
    b.rect(9, 2, 4, 2, 8);
    b.rect(8, 11, 7, 10, 1);
    b.rect(9, 13, 5, 4, 7);
    b.rect(5, 12, 3, 7, 2);
    b.rect(14, 12, 3, 7, 2);
    if (fr == 0) {
        b.rect(8, 21, 3, 11, 3);
        b.rect(12, 21, 3, 9, 3);
    } else {
        b.rect(7, 21, 3, 9, 3);
        b.rect(13, 21, 3, 11, 3);
    }
    b.rect(7, 31, 5, 3, 5);
    b.rect(12, 31, 5, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(28, 8);
    b.rect(1, 3, 22, 2, 1);
    b.rect(20, 1, 6, 6, 2);
    b.rect(22, 2, 2, 4, 3);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(48, 72);
    b.rect(2, 8, 8, 62, 1);
    b.rect(38, 8, 8, 62, 1);
    b.rect(4, 10, 4, 56, 2);
    b.rect(40, 10, 4, 56, 2);
    for (int y = 0; y < 18; y++) {
        int inset = (y * y) / 22;
        b.rect(10 + inset, 8 + y, 28 - inset * 2, 1, 3);
    }
    b.rect(0, 0, 48, 10, 1);
    b.rect(2, 2, 44, 4, 4);
    return b;
}

gs::Bitmap deckArt() {
    gs::Bitmap b(80, 18);
    b.rect(0, 2, 80, 14, 1);
    b.rect(0, 0, 80, 4, 2);
    for (int x = 4; x < 80; x += 10) b.rect(x, 6, 1, 8, 3);
    b.rect(0, 14, 80, 3, 4);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(20, 16);
    b.rect(2, 2, 16, 12, 1);
    b.line(2, 2, 18, 14, 2, 1.f);
    b.line(18, 2, 2, 14, 2, 1.f);
    b.rect(9, 6, 3, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 9, 6, 7, 1);
    b.ellipse(8, 5, 5, 2, 2);
    b.rect(3, 7, 10, 2, 3);
    b.rect(3, 12, 10, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(28, 10);
    b.rect(1, 3, 26, 4, 1);
    b.rect(2, 4, 24, 2, 2);
    for (int x = 4; x < 26; x += 6) b.rect(x, 2, 2, 6, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 2, 2, 12, 1);
    b.ellipse(5, 16, 4, 4, 2);
    b.ellipse(5, 16, 2, 2, 3);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 8, 8, 2);
    b.rect(12, 6, 2, 7, 3);
    b.rect(12, 12, 6, 2, 4);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 3, 1);
    b.ellipse(5, 4, 2, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 6);
    b.ellipse(10, 3, 8, 2, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), gs::rgb4(9, 8, 7), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 6), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 15, 6), gs::rgb4(13, 15, 10), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 9, 8), gs::rgb4(6, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(13, 12, 10), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(3, 5, 9), gs::rgb4(2, 3, 6), gs::rgb4(5, 7, 11), gs::rgb4(12, 8, 5),
                             gs::rgb4(14, 11, 8), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 13, 8),
                             gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(11, 7, 3), gs::rgb4(8, 5, 2), gs::rgb4(14, 11, 5), gs::rgb4(4, 2, 1), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BARREL, {0, gs::rgb4(9, 6, 3), gs::rgb4(13, 9, 4), gs::rgb4(4, 3, 2), gs::rgb4(6, 4, 2), 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(9, 8, 6), gs::rgb4(14, 13, 11), gs::rgb4(5, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(6, 7, 8), gs::rgb4(10, 11, 12), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 6), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 11, 9), gs::rgb4(5, 5, 4), gs::rgb4(3, 3, 3), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(9, 8, 7), gs::rgb4(6, 5, 5), gs::rgb4(7, 6, 5), gs::rgb4(12, 11, 9), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(8, 7, 5), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 2), gs::rgb4(12, 3, 2),
                            gs::rgb4(14, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, ink});

    const uint16_t water[16] = {
        0,
        gs::rgb4(2, 5, 8),
        gs::rgb4(3, 7, 10),
        gs::rgb4(4, 8, 11),
        gs::rgb4(5, 9, 12),
        gs::rgb4(1, 4, 7),
        gs::rgb4(6, 10, 12),
        gs::rgb4(2, 6, 9),
        gs::rgb4(7, 11, 13),
        gs::rgb4(3, 6, 8),
        gs::rgb4(8, 12, 13),
        gs::rgb4(1, 3, 6),
        gs::rgb4(4, 9, 11),
        gs::rgb4(2, 7, 9),
        gs::rgb4(9, 13, 14),
        gs::rgb4(5, 8, 10),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, water[i]);

    loadFont(vdp, art);
    art.walker[0] = gs::uploadMipped(vdp, walkerArt(0));
    art.walker[1] = gs::uploadMipped(vdp, walkerArt(1));
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace cler
