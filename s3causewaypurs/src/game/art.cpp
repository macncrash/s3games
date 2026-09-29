#include "game/art.h"

namespace cwpurs {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap youArt() {
    gs::Bitmap b(72, 40);
    b.rect(6, 16, 48, 16, 1);
    b.rect(36, 6, 22, 14, 2);
    b.rect(40, 9, 8, 6, 4);
    b.rect(50, 9, 6, 6, 4);
    b.rect(4, 28, 12, 8, 3);
    b.rect(22, 28, 12, 8, 3);
    b.rect(44, 28, 12, 8, 3);
    b.rect(58, 18, 10, 6, 5);
    b.rect(8, 18, 8, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(64, 48);
    b.rect(8, 22, 40, 16, 1);
    b.rect(28, 4, 8, 28, 2);
    b.rect(22, 2, 20, 6, 3);
    b.rect(6, 34, 12, 8, 4);
    b.rect(30, 34, 12, 8, 4);
    b.rect(14, 24, 10, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap gradeArt() {
    gs::Bitmap b(70, 36);
    b.rect(16, 14, 40, 14, 1);
    b.poly({{4, 28}, {20, 12}, {20, 28}}, 2);
    b.rect(38, 6, 16, 12, 3);
    b.rect(42, 9, 8, 5, 5);
    b.rect(18, 26, 10, 8, 4);
    b.rect(40, 26, 10, 8, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap pumpArt() {
    gs::Bitmap b(56, 44);
    b.rect(10, 18, 32, 16, 1);
    b.ellipse(26, 14, 10, 10, 2);
    b.ellipse(26, 14, 4, 4, 5);
    b.rect(36, 8, 6, 16, 3);
    b.rect(8, 30, 10, 8, 4);
    b.rect(28, 30, 10, 8, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap lorryArt() {
    gs::Bitmap b(76, 40);
    b.rect(8, 16, 28, 14, 2);
    b.rect(34, 12, 32, 18, 1);
    b.rect(12, 8, 18, 10, 3);
    b.rect(16, 11, 8, 5, 5);
    b.rect(10, 28, 10, 8, 4);
    b.rect(26, 28, 10, 8, 4);
    b.rect(46, 28, 10, 8, 4);
    b.outline(6, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 40);
    b.rect(5, 4, 4, 32, 1);
    b.rect(2, 2, 10, 5, 2);
    b.rect(3, 34, 8, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 36);
    b.rect(6, 12, 4, 20, 2);
    b.ellipse(8, 8, 6, 5, 1);
    b.ellipse(8, 8, 2, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap boatArt() {
    gs::Bitmap b(36, 16);
    b.poly({{2, 8}, {8, 14}, {28, 14}, {34, 8}, {28, 10}, {8, 10}}, 1);
    b.rect(14, 4, 8, 6, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(32, 24);
    b.ellipse(16, 14, 12, 6, 1);
    b.ellipse(8, 8, 3, 5, 2);
    b.ellipse(24, 7, 3, 6, 2);
    b.ellipse(16, 4, 2, 4, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    big.outline = 2;
    for (int c = 32; c < 127; c++) {
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 13), gs::rgb4(3, 3, 4)};
    const uint16_t you[] = {0, gs::rgb4(12, 10, 4), gs::rgb4(8, 7, 3), gs::rgb4(2, 2, 2), gs::rgb4(10, 13, 14),
                            gs::rgb4(14, 6, 3), gs::rgb4(15, 13, 5), gs::rgb4(1, 1, 1)};
    const uint16_t pile[] = {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(12, 10, 4), gs::rgb4(2, 2, 2),
                             gs::rgb4(14, 8, 3), gs::rgb4(1, 1, 1)};
    const uint16_t grade[] = {0, gs::rgb4(10, 8, 4), gs::rgb4(6, 6, 5), gs::rgb4(4, 5, 6), gs::rgb4(2, 2, 2),
                              gs::rgb4(12, 14, 14), gs::rgb4(1, 1, 1)};
    const uint16_t pump[] = {0, gs::rgb4(4, 7, 8), gs::rgb4(3, 5, 6), gs::rgb4(8, 8, 6), gs::rgb4(2, 2, 2),
                             gs::rgb4(14, 14, 8), gs::rgb4(1, 1, 1)};
    const uint16_t lorry[] = {0, gs::rgb4(8, 4, 3), gs::rgb4(5, 5, 6), gs::rgb4(10, 9, 7), gs::rgb4(2, 2, 2),
                              gs::rgb4(12, 13, 14), gs::rgb4(1, 1, 1)};
    const uint16_t stone[] = {0, gs::rgb4(8, 8, 7), gs::rgb4(11, 10, 8), gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2)};
    const uint16_t lamp[] = {0, gs::rgb4(15, 14, 7), gs::rgb4(5, 5, 5), gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 2)};
    const uint16_t fx[] = {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14), gs::rgb4(15, 15, 14)};
    const uint16_t boat[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 4), gs::rgb4(2, 2, 2)};
    const uint16_t alert[] = {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 2, 2)};
    const uint16_t good[] = {0, gs::rgb4(8, 15, 7), gs::rgb4(2, 5, 3)};
    const uint16_t title[] = {0, gs::rgb4(15, 13, 6), gs::rgb4(6, 4, 2)};
    const uint16_t dead[] = {0, gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 3)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_YOU, you, 8);
    setPal(vdp, PAL_PILE, pile, 7);
    setPal(vdp, PAL_GRADE, grade, 7);
    setPal(vdp, PAL_PUMP, pump, 7);
    setPal(vdp, PAL_LORRY, lorry, 7);
    setPal(vdp, PAL_STONE, stone, 5);
    setPal(vdp, PAL_LAMP, lamp, 5);
    setPal(vdp, PAL_FX, fx, 4);
    setPal(vdp, PAL_BOAT, boat, 4);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_GOOD, good, 3);
    setPal(vdp, PAL_TITLE, title, 3);
    setPal(vdp, PAL_DEAD, dead, 3);

    const uint16_t field[16] = {
        0,
        gs::rgb4(5, 5, 4), gs::rgb4(4, 4, 3), gs::rgb4(7, 7, 6),
        gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5),
        gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4),
        gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(10, 10, 9),
        gs::rgb4(1, 3, 6), gs::rgb4(2, 5, 8), gs::rgb4(4, 8, 10),
        gs::rgb4(14, 13, 8), gs::rgb4(9, 9, 8),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, field[i]);
    const uint16_t mist[] = {0, gs::rgb4(8, 9, 11), gs::rgb4(4, 5, 7)};
    setPal(vdp, PAL_MIST, mist, 3);
    vdp.setFogColor(gs::rgb4(5, 7, 10));

    loadFont(vdp, art);
    art.you = gs::uploadMipped(vdp, youArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.grade = gs::uploadMipped(vdp, gradeArt());
    art.pump = gs::uploadMipped(vdp, pumpArt());
    art.lorry = gs::uploadMipped(vdp, lorryArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.boat = gs::uploadMipped(vdp, boatArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
}

}  // namespace cwpurs
