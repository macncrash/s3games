#include "game/art.h"

#include <string>

namespace tramboom {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap tramArt() {
    Bitmap b(168, 72);
    b.rect(8, 22, 148, 34, 1);
    b.rect(8, 40, 148, 8, 2);
    b.rect(8, 22, 148, 4, 3);
    b.poly({{18, 22}, {36, 8}, {132, 8}, {148, 22}}, 1);
    b.poly({{40, 10}, {128, 10}, {140, 20}, {28, 20}}, 4);
    b.rect(22, 28, 22, 14, 4);
    b.rect(48, 28, 22, 14, 4);
    b.rect(96, 28, 22, 14, 4);
    b.rect(122, 28, 22, 14, 4);
    b.rect(74, 26, 18, 22, 6);
    b.rect(76, 28, 14, 12, 5);
    b.rect(6, 48, 154, 6, 7);
    b.rect(14, 18, 6, 10, 7);
    b.line(20, 6, 84, 4, 7, 2.0f);
    b.line(84, 4, 150, 14, 7, 2.0f);
    b.rect(80, 2, 6, 8, 7);
    b.rect(4, 30, 8, 16, 3);
    b.rect(154, 32, 8, 14, 3);
    b.ellipse(36, 58, 12, 12, 6);
    b.ellipse(128, 58, 12, 12, 6);
    b.outline(5, false);
    return b;
}

Bitmap wheelArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 5, 5, 2);
    b.rect(12, 3, 4, 22, 2);
    b.rect(3, 12, 22, 4, 2);
    b.outline(3, false);
    return b;
}

Bitmap driveArt() {
    Bitmap b(40, 30);
    b.rect(2, 6, 36, 20, 1);
    b.rect(2, 6, 36, 4, 4);
    b.rect(6, 12, 10, 10, 3);
    b.rect(16, 12, 8, 10, 2);
    b.rect(26, 12, 8, 10, 3);
    b.ellipse(11, 17, 3, 3, 5);
    b.ellipse(30, 17, 3, 3, 5);
    b.rect(8, 2, 24, 5, 2);
    b.outline(5, false);
    return b;
}

Bitmap boomArt() {
    Bitmap b(120, 96);
    b.rect(86, 18, 14, 70, 4);
    b.rect(90, 10, 6, 12, 2);
    b.rect(18, 28, 78, 8, 1);
    b.rect(18, 44, 70, 6, 2);
    for (int i = 0; i < 6; i++) {
        float x = 22.f + i * 12.f;
        b.line(x, 28, x + 12, 44, 3, 2.0f);
        b.line(x + 12, 28, x, 44, 3, 2.0f);
    }
    b.rect(8, 24, 16, 18, 1);
    b.rect(10, 40, 4, 28, 5);
    b.rect(18, 40, 4, 28, 5);
    b.rect(6, 66, 20, 6, 2);
    b.rect(78, 18, 8, 22, 3);
    b.outline(3, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(72, 88);
    b.rect(4, 28, 64, 56, 1);
    b.poly({{2, 28}, {36, 6}, {70, 28}}, 4);
    b.rect(14, 40, 14, 18, 3);
    b.rect(40, 40, 14, 18, 3);
    b.rect(28, 62, 16, 22, 2);
    b.rect(8, 24, 8, 10, 5);
    b.outline(2, false);
    return b;
}

Bitmap blockArt() {
    Bitmap b(48, 80);
    b.rect(2, 8, 44, 70, 1);
    b.rect(2, 8, 44, 6, 4);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 3; x++) b.rect(8 + x * 12, 20 + y * 14, 8, 8, (x + y) & 1 ? 3 : 5);
    b.outline(2, false);
    return b;
}

Bitmap poleArt() {
    Bitmap b(16, 88);
    b.rect(6, 4, 4, 80, 1);
    b.rect(2, 6, 12, 4, 2);
    b.rect(4, 78, 8, 6, 2);
    return b;
}

Bitmap wireArt() {
    Bitmap b(64, 4);
    b.rect(0, 1, 64, 2, 1);
    return b;
}

Bitmap railArt() {
    Bitmap b(48, 16);
    b.rect(0, 4, 48, 3, 1);
    b.rect(0, 10, 48, 3, 2);
    for (int x = 4; x < 48; x += 12) b.rect(x, 6, 3, 6, 3);
    return b;
}

Bitmap bumperArt() {
    Bitmap b(20, 36);
    b.rect(4, 4, 8, 28, 1);
    b.rect(8, 8, 8, 20, 2);
    b.rect(2, 2, 16, 4, 3);
    b.outline(3, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big{3, 1, 0, 0, 1};
    for (int c = 32; c < 128; c++)
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(12, 2, 2), gs::rgb4(7, 1, 1), gs::rgb4(6, 10, 13), gs::rgb4(1, 1, 2),
            gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(4, 8, 3), gs::rgb4(2, 3, 2), gs::rgb4(8, 9, 8), gs::rgb4(10, 12, 6), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(12, 8, 1), gs::rgb4(1, 1, 1), gs::rgb4(6, 5, 4), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_CITY,
           {0, gs::rgb4(7, 4, 4), gs::rgb4(3, 2, 3), gs::rgb4(14, 11, 4), gs::rgb4(4, 3, 4), gs::rgb4(9, 8, 6)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 15, 7), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_DUSK,
           {0, gs::rgb4(10, 10, 11), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2)});

    art.tram = gs::uploadMipped(vdp, tramArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.drive = gs::uploadMipped(vdp, driveArt());
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.wire = gs::uploadMipped(vdp, wireArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.bumper = gs::uploadMipped(vdp, bumperArt());
    loadFont(vdp, art);

    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace tramboom
