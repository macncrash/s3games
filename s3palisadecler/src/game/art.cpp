#include "game/art.h"

#include <initializer_list>
#include <string>

namespace palcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap wardArt(int step) {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 7, 6, 5, 2);
    b.rect(10, 5, 8, 3, 3);
    b.set(12, 8, 1);
    b.set(16, 8, 1);
    b.poly({{8, 14}, {20, 14}, {22, 32}, {6, 32}}, 4);
    b.rect(12, 16, 4, 8, 5);
    b.rect(11, 32, 3, step ? 12 : 10, 6);
    b.rect(15, 32, 3, step ? 10 : 12, 6);
    b.rect(9, step ? 42 : 40, 6, 3, 7);
    b.rect(15, step ? 40 : 42, 6, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap rakeArt() {
    gs::Bitmap b(36, 18);
    b.line(2, 9, 22, 9, 3, 2.f);
    b.rect(20, 3, 4, 12, 4);
    for (int i = 0; i < 5; i++) b.line(float(22 + i * 3), 4.f, float(24 + i * 3), 16.f, 2, 1.2f);
    b.outline(1, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(16, 72);
    b.poly({{8, 2}, {14, 16}, {13, 70}, {3, 70}, {2, 16}}, 1);
    b.line(8, 8, 8, 68, 2, 1.2f);
    b.rect(3, 58, 10, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(48, 80);
    b.rect(4, 8, 8, 70, 1);
    b.rect(36, 8, 8, 70, 1);
    b.poly({{4, 8}, {24, 2}, {44, 8}, {40, 16}, {24, 10}, {8, 16}}, 2);
    for (int y = 22; y < 72; y += 10) b.rect(12, y, 24, 3, 3);
    b.rect(22, 40, 4, 28, 5);
    b.outline(4, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(26, 16);
    b.poly({{3, 10}, {8, 3}, {18, 2}, {24, 8}, {20, 14}, {6, 14}}, 1);
    b.line(8, 6, 16, 5, 2, 1.f);
    b.outline(3, false);
    return b;
}

gs::Bitmap branchArt() {
    gs::Bitmap b(40, 14);
    b.line(2, 8, 36, 5, 1, 2.4f);
    b.line(12, 7, 8, 2, 2, 1.4f);
    b.line(22, 6, 26, 2, 2, 1.4f);
    b.ellipse(34, 4, 3, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap shieldArt() {
    gs::Bitmap b(22, 28);
    b.poly({{11, 2}, {20, 8}, {18, 20}, {11, 26}, {4, 20}, {2, 8}}, 1);
    b.line(11, 4, 11, 22, 2, 1.4f);
    b.line(6, 12, 16, 12, 3, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap brushArt() {
    gs::Bitmap b(30, 22);
    b.ellipse(10, 12, 8, 7, 1);
    b.ellipse(18, 10, 9, 6, 2);
    b.ellipse(14, 16, 10, 4, 3);
    b.line(6, 8, 4, 3, 4, 1.2f);
    b.line(20, 6, 24, 2, 4, 1.2f);
    b.outline(5, false);
    return b;
}

gs::Bitmap moteArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap ditchArt() {
    gs::Bitmap b(48, 10);
    b.ellipse(24, 5, 22, 4, 1);
    b.ellipse(24, 5, 14, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 8), gs::rgb4(8, 10, 6), gs::rgb4(15, 15, 12), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(2, 1, 0),
                             gs::rgb4(9, 8, 4), gs::rgb4(6, 6, 5)});
    setPal(vdp, PAL_WARD, {0, gs::rgb4(1, 1, 1), gs::rgb4(12, 8, 5), gs::rgb4(4, 3, 2), gs::rgb4(3, 5, 8),
                           gs::rgb4(6, 7, 9), gs::rgb4(7, 5, 3), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 11), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 4),
                           gs::rgb4(10, 8, 4)});
    setPal(vdp, PAL_BRUSH, {0, gs::rgb4(3, 7, 2), gs::rgb4(5, 9, 3), gs::rgb4(2, 5, 2), gs::rgb4(8, 10, 4),
                            gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_DUSK, {0, gs::rgb4(12, 7, 3), gs::rgb4(14, 10, 4), gs::rgb4(6, 4, 8), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(7, 7, 6), gs::rgb4(10, 10, 9), gs::rgb4(4, 4, 4), gs::rgb4(9, 6, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 4, 2), gs::rgb4(15, 8, 3), gs::rgb4(8, 2, 1), gs::rgb4(15, 14, 8)});

    a.ward[0] = gs::uploadMipped(vdp, wardArt(0));
    a.ward[1] = gs::uploadMipped(vdp, wardArt(1));
    a.rake = gs::uploadMipped(vdp, rakeArt());
    a.stake = gs::uploadMipped(vdp, stakeArt());
    a.gate = gs::uploadMipped(vdp, gateArt());
    a.stone = gs::uploadMipped(vdp, stoneArt());
    a.branch = gs::uploadMipped(vdp, branchArt());
    a.shield = gs::uploadMipped(vdp, shieldArt());
    a.brush = gs::uploadMipped(vdp, brushArt());
    a.mote = gs::uploadMipped(vdp, moteArt());
    a.ditch = gs::uploadMipped(vdp, ditchArt());

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    big.outline = 4;
    for (int c = 32; c < 127; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace palcler
