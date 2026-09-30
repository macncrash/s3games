#include "game/art.h"

#include <string>

namespace sally {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap wallArt() {
    Bitmap b(320, 96);
    b.rect(0, 28, 320, 42, 2);
    b.rect(0, 34, 320, 8, 3);
    for (int x = 0; x < 320; x += 16) {
        b.rect(x, 8, 10, 22, 2);
        b.rect(x, 8, 10, 3, 3);
        b.rect(x + 2, 70, 12, 26, (x / 16) % 2 ? 1 : 2);
    }
    b.rect(0, 68, 320, 4, 4);
    b.rect(132, 36, 56, 34, 1);
    b.rect(148, 44, 24, 26, 5);
    b.rect(140, 40, 6, 30, 3);
    b.rect(174, 40, 6, 30, 3);
    for (int y = 40; y < 68; y += 6) b.rect(146, y, 28, 1, 3);
    return b;
}

Bitmap moonArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 1);
    b.ellipse(18, 12, 8, 8, 0);
    b.ellipse(10, 16, 2, 2, 2);
    b.ellipse(15, 18, 1, 1, 2);
    return b;
}

Bitmap starArt() {
    Bitmap b(5, 5);
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

Bitmap potArt() {
    Bitmap b(36, 28);
    b.poly({{6, 6}, {30, 6}, {32, 26}, {4, 26}}, 2);
    b.poly({{8, 8}, {28, 8}, {29, 24}, {7, 24}}, 3);
    b.rect(4, 4, 28, 5, 4);
    b.rect(10, 12, 4, 8, 1);
    b.rect(22, 14, 3, 6, 1);
    b.ellipse(18, 10, 6, 3, 5);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(24, 40);
    float lean = frame == 0 ? -2.f : frame == 2 ? 2.f : 0.f;
    b.poly({{12 + lean, 2}, {20, 16}, {16, 22}, {18, 36}, {6, 36}, {8, 20}, {4, 14}}, 1);
    b.poly({{12 + lean * 0.6f, 8}, {16, 18}, {14, 34}, {9, 34}, {8, 18}}, 2);
    b.poly({{12 + lean * 0.3f, 14}, {14, 22}, {13, 32}, {11, 32}, {10, 22}}, 3);
    b.ellipse(12 + lean * 0.2f, 18, 2, 3, 4);
    return b;
}

Bitmap sentryArt(bool pikeLeft) {
    Bitmap b(32, 48);
    b.rect(12, 6, 8, 8, 3);
    b.rect(11, 4, 10, 3, 4);
    b.rect(13, 9, 2, 2, 1);
    b.rect(17, 9, 2, 2, 1);
    b.rect(10, 14, 12, 16, 2);
    b.rect(12, 16, 8, 8, 5);
    b.rect(8, 16, 4, 12, 2);
    b.rect(20, 16, 4, 12, 2);
    b.rect(11, 30, 4, 12, 6);
    b.rect(17, 30, 4, 12, 6);
    b.rect(10, 40, 5, 4, 1);
    b.rect(17, 40, 5, 4, 1);
    if (pikeLeft) {
        b.line(6, 40, 4, 2, 4, 1.6f);
        b.poly({{2, 6}, {8, 4}, {5, 0}}, 4);
    } else {
        b.line(26, 40, 28, 2, 4, 1.6f);
        b.poly({{24, 4}, {30, 6}, {27, 0}}, 4);
    }
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t z = 0;
    setPal(vdp, PAL_HUD, {z, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 4, 3), gs::rgb4(8, 14, 8), gs::rgb4(6, 8, 10), z, z, z, z, z, z, z, z, z, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_AMBER, {z, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 8), z, z, z, z, z, z, z, z, z, z, z, z, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_FLAME, {z, gs::rgb4(8, 1, 0), gs::rgb4(13, 4, 0), gs::rgb4(15, 9, 1), gs::rgb4(15, 14, 6), gs::rgb4(15, 12, 4), z, z, z, z, z, z, z, z, z, gs::rgb4(3, 1, 0)});
    setPal(vdp, PAL_MAN, {z, gs::rgb4(2, 2, 2), gs::rgb4(2, 3, 6), gs::rgb4(12, 8, 6), gs::rgb4(9, 10, 11), gs::rgb4(5, 3, 2), gs::rgb4(1, 1, 3), z, z, z, z, z, z, z, z, gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_WOOD, {z, gs::rgb4(3, 2, 1), gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(6, 6, 7), gs::rgb4(12, 6, 1), z, z, z, z, z, z, z, z, z, z, gs::rgb4(1, 1, 0)});
    setPal(vdp, PAL_STONE, {z, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(3, 4, 3), gs::rgb4(1, 1, 2), z, z, z, z, z, z, z, z, z, z, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_RED, {z, gs::rgb4(15, 3, 2), z, z, z, z, z, z, z, z, z, z, z, z, z, z, gs::rgb4(3, 0, 0)});

    art.wall = gs::uploadMipped(vdp, wallArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.pot = gs::uploadMipped(vdp, potArt());
    for (int i = 0; i < 3; i++) art.flame[i] = gs::uploadMipped(vdp, flameArt(i));
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(true));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(false));
    art.bar = gs::uploadMipped(vdp, barArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 1, 2));
}

}  // namespace sally
