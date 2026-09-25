#include "game/art.h"

#include <initializer_list>
#include <string>

namespace tank {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// Rear view. Tracks sit on the bottom edge so the sprite can stand on the street.
gs::Bitmap tankRear(int phase) {
    gs::Bitmap b(96, 84);
    b.rect(6, 22, 22, 58, 4);
    b.rect(68, 22, 22, 58, 4);
    b.rect(8, 24, 18, 54, 3);
    b.rect(70, 24, 18, 54, 3);
    for (int i = 0; i < 4; i++) {
        int y = 28 + i * 13 + (phase ? 6 : 0);
        if (y > 68) y -= 52;
        b.ellipse(17, y, 7, 7, 8);
        b.ellipse(79, y, 7, 7, 8);
        b.ellipse(17, y, 2, 2, 5);
        b.ellipse(79, y, 2, 2, 5);
        b.rect(8, y + 8, 18, 3, 5);
        b.rect(70, y + 8, 18, 3, 5);
    }
    b.rect(26, 30, 44, 50, 2);
    b.rect(30, 34, 36, 40, 3);
    b.rect(28, 26, 40, 12, 1);
    b.ellipse(48, 46, 16, 14, 2);
    b.ellipse(48, 44, 11, 9, 1);
    b.ellipse(48, 42, 4, 3, 8);
    b.rect(43, 6, 10, 28, 8);
    b.rect(45, 6, 6, 22, 5);
    b.rect(34, 18, 6, 10, 8);
    b.rect(56, 18, 6, 10, 8);
    b.line(34, 70, 48, 56, 7, 4);
    b.line(48, 56, 62, 70, 7, 4);
    b.rect(32, 74, 8, 4, 9);
    b.rect(56, 74, 8, 4, 9);
    b.outline(15, false);
    return b;
}

// Head-on. The muzzle points at the camera, tracks on the ground.
gs::Bitmap tankFront() {
    gs::Bitmap b(96, 84);
    b.rect(4, 24, 24, 56, 4);
    b.rect(68, 24, 24, 56, 4);
    b.rect(7, 28, 18, 48, 3);
    b.rect(71, 28, 18, 48, 3);
    for (int i = 0; i < 3; i++) {
        int y = 34 + i * 14;
        b.ellipse(16, y, 6, 6, 8);
        b.ellipse(80, y, 6, 6, 8);
    }
    b.poly({{28, 78}, {68, 78}, {60, 40}, {36, 40}}, 2);
    b.poly({{34, 74}, {62, 74}, {56, 46}, {40, 46}}, 1);
    b.rect(34, 64, 28, 12, 3);
    b.ellipse(48, 48, 16, 13, 6);
    b.ellipse(48, 46, 10, 8, 2);
    b.rect(42, 28, 12, 18, 8);
    b.ellipse(48, 28, 8, 8, 8);
    b.ellipse(48, 28, 3, 3, 4);
    b.poly({{48, 58}, {58, 68}, {48, 78}, {38, 68}}, 7);
    b.poly({{48, 62}, {54, 68}, {48, 74}, {42, 68}}, 9);
    b.outline(15, false);
    return b;
}

void windows(gs::Bitmap& b, int x, int y, int cols, int rows, int lit) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int px = x + c * 14;
            int py = y + r * 16;
            bool on = ((r * 3 + c * 2 + lit) % 5) != 0;
            b.rect(px, py, 10, 12, on ? 4 : 5);
            b.rect(px + 2, py + 2, 6, 4, on ? 1 : 5);
        }
    }
}

gs::Bitmap building(int kind) {
    gs::Bitmap b(72, 112);
    b.rect(4, 10, 64, 100, 2);
    b.rect(6, 14, 60, 92, 3);
    b.rect(2, 6, 68, 8, 8);
    b.rect(8, 16, 56, 4, 7);
    if (kind == 0) {
        windows(b, 12, 26, 3, 4, 1);
        b.rect(28, 86, 16, 22, 6);
        b.rect(30, 90, 5, 6, 4);
        b.line(10, 40, 10, 96, 10, 2);
        b.line(10, 56, 22, 56, 10, 2);
        b.line(10, 74, 22, 74, 10, 2);
    } else if (kind == 1) {
        windows(b, 14, 24, 3, 3, 4);
        b.poly({{6, 78}, {66, 78}, {60, 66}, {12, 66}}, 9);
        b.rect(26, 80, 20, 28, 6);
        b.rect(30, 86, 12, 10, 5);
        b.rect(16, 84, 8, 14, 1);
        b.rect(48, 84, 8, 14, 1);
    } else {
        windows(b, 12, 22, 4, 2, 2);
        b.rect(14, 62, 44, 46, 6);
        b.rect(18, 66, 36, 38, 3);
        b.rect(22, 78, 28, 26, 5);
        b.rect(8, 28, 6, 70, 10);
        b.rect(58, 28, 6, 70, 10);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(24, 80);
    b.rect(10, 18, 4, 60, 3);
    b.rect(4, 6, 16, 12, 2);
    b.rect(6, 8, 12, 7, 1);
    b.rect(8, 16, 8, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap hydrantArt() {
    gs::Bitmap b(24, 32);
    b.rect(8, 18, 8, 12, 2);
    b.ellipse(12, 12, 7, 7, 9);
    b.ellipse(12, 11, 3, 3, 7);
    b.rect(2, 14, 6, 3, 2);
    b.rect(16, 14, 6, 3, 2);
    b.rect(6, 28, 12, 3, 8);
    b.outline(15, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 10, 9), gs::rgb4(15, 15, 14), gs::rgb4(15, 5, 3),
                          gs::rgb4(6, 14, 6), gs::rgb4(15, 12, 3), gs::rgb4(6, 8, 12), gs::rgb4(6, 6, 6),
                          0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 2), gs::rgb4(15, 10, 8), gs::rgb4(15, 14, 12), gs::rgb4(8, 1, 1),
                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 5), gs::rgb4(3, 10, 3), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t black = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(11, 13, 7), gs::rgb4(7, 9, 4), gs::rgb4(3, 4, 2), gs::rgb4(2, 2, 2),
                             gs::rgb4(9, 9, 8), gs::rgb4(8, 10, 5), gs::rgb4(15, 15, 13), gs::rgb4(4, 4, 4),
                             gs::rgb4(14, 2, 2), 0, 0, 0, 0, 0, black});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(14, 12, 7), gs::rgb4(11, 8, 4), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2),
                            gs::rgb4(9, 8, 7), gs::rgb4(10, 5, 3), gs::rgb4(15, 14, 12), gs::rgb4(3, 3, 3),
                            gs::rgb4(13, 2, 1), 0, 0, 0, 0, 0, black});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 13), gs::rgb4(15, 8, 2), gs::rgb4(14, 4, 1),
                         gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BLDG, {0, gs::rgb4(14, 12, 6), gs::rgb4(12, 6, 4), gs::rgb4(7, 3, 2), gs::rgb4(15, 13, 6),
                           gs::rgb4(2, 3, 6), gs::rgb4(4, 3, 2), gs::rgb4(13, 12, 10), gs::rgb4(9, 9, 8),
                           gs::rgb4(12, 2, 2), gs::rgb4(3, 3, 3), 0, 0, 0, 0, black});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(12, 11, 8), gs::rgb4(4, 4, 4), gs::rgb4(1, 1, 1),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(12, 11, 10), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(15, 8, 2),
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t street[16] = {
        0,
        gs::rgb4(11, 11, 10), gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5),
        gs::rgb4(5, 5, 4), gs::rgb4(3, 3, 3),
        gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4),
        gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(6, 6, 5),
        gs::rgb4(3, 4, 5), gs::rgb4(2, 3, 4), gs::rgb4(4, 5, 6),
        gs::rgb4(14, 12, 2), gs::rgb4(7, 7, 8),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, street[i]);
    vdp.setFogColor(gs::rgb4(12, 10, 8));

    loadFont(vdp, art);
    art.rear[0] = gs::uploadMipped(vdp, tankRear(0));
    art.rear[1] = gs::uploadMipped(vdp, tankRear(1));
    art.front = gs::uploadMipped(vdp, tankFront());
    for (int i = 0; i < 3; i++) art.bldg[i] = gs::uploadMipped(vdp, building(i));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.hydrant = gs::uploadMipped(vdp, hydrantArt());

    gs::Bitmap shell(8, 18);
    shell.rect(3, 2, 2, 14, 1);
    shell.rect(2, 1, 4, 4, 2);
    art.shell = gs::uploadMipped(vdp, shell);

    gs::Bitmap flash(20, 20);
    flash.ellipse(10, 10, 8, 6, 1);
    flash.ellipse(10, 10, 4, 3, 2);
    art.flash = gs::uploadMipped(vdp, flash);

    gs::Bitmap smoke(40, 40);
    smoke.ellipse(22, 22, 14, 12, 2);
    smoke.ellipse(16, 16, 10, 8, 1);
    smoke.ellipse(18, 18, 4, 3, 3);
    art.smoke = gs::uploadMipped(vdp, smoke);

    gs::Bitmap shade(64, 20);
    shade.ellipse(32, 10, 26, 7, 1);
    art.shadow = gs::uploadMipped(vdp, shade);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace tank
