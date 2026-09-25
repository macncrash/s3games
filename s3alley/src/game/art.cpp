#include "game/art.h"

#include <cstdint>

namespace alley {
namespace {

void setPal(gs::VDP& v, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) v.setColor(pal * 16 + i, c[i]);
}

void brick(gs::Bitmap& b, int x0, int y0, int x1, int y1) {
    for (int y = y0; y < y1; y++) {
        int course = y / 5;
        for (int x = x0; x < x1; x++) {
            bool mortar = (y % 5) == 0 || ((x + (course & 1) * 6) % 11) == 0;
            int h = (x * 13 + course * 7) & 7;
            int c = mortar ? 2 : (h < 2 ? 3 : (h > 5 ? 9 : 1));
            b.set(x, y, c);
        }
    }
}

gs::Bitmap wallArt(int variant) {
    gs::Bitmap b(48, 112);
    brick(b, 0, 0, 48, 112);
    for (int y = 0; y < 112; y++) b.set(0, y, 10);
    if (variant == 0 || variant == 1) {
        int glass = variant == 0 ? 6 : 5;
        b.rect(12, 22, 24, 32, 4);
        b.rect(14, 24, 20, 28, glass);
        b.rect(22, 24, 3, 28, 4);
        b.rect(14, 36, 20, 3, 4);
        if (variant == 0) {
            b.rect(16, 28, 6, 10, 8);
            b.rect(18, 40, 10, 10, 7);
        } else {
            b.rect(15, 26, 4, 24, 8);
        }
        b.rect(10, 54, 28, 3, 3);
    } else if (variant == 2) {
        b.rect(30, 0, 6, 112, 7);
        b.rect(26, 78, 12, 4, 7);
        b.rect(14, 18, 12, 16, 4);
        b.rect(15, 19, 10, 14, 5);
    } else {
        b.rect(10, 26, 28, 36, 4);
        for (int i = 0; i < 5; i++) b.rect(12, 30 + i * 6, 24, 3, 7);
        b.rect(10, 62, 28, 3, 3);
    }
    return b;
}

gs::Bitmap walkerArt(int frame) {
    gs::Bitmap b(40, 64);
    b.rect(14, 3, 12, 7, 2);
    b.rect(9, 9, 22, 3, 1);
    b.rect(15, 12, 10, 8, 5);
    b.rect(13, 20, 14, 4, 4);
    b.rect(frame ? 22 : 11, 23, 7, 5, 4);
    b.poly({{8, 26}, {32, 26}, {35, 48}, {5, 48}}, 2);
    b.rect(19, 28, 2, 16, 1);
    b.rect(frame ? 30 : 4, 28, 5, 15, 2);
    b.rect(frame ? 4 : 31, 30, 5, 13, 2);
    if (frame == 0) {
        b.rect(11, 48, 6, 11, 1);
        b.rect(23, 48, 6, 11, 1);
        b.rect(10, 57, 8, 4, 7);
        b.rect(22, 57, 8, 4, 7);
    } else {
        b.rect(14, 48, 5, 11, 1);
        b.rect(21, 48, 5, 11, 1);
        b.rect(13, 57, 7, 4, 7);
        b.rect(20, 57, 7, 4, 7);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap crouchArt() {
    gs::Bitmap b(44, 40);
    b.rect(14, 2, 14, 5, 2);
    b.rect(8, 7, 26, 3, 1);
    b.rect(16, 10, 10, 6, 5);
    b.rect(12, 16, 18, 3, 4);
    b.poly({{5, 20}, {38, 20}, {40, 32}, {3, 32}}, 2);
    b.rect(6, 32, 12, 4, 1);
    b.rect(24, 32, 12, 4, 1);
    b.rect(4, 35, 14, 3, 7);
    b.rect(24, 35, 14, 3, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 32);
    b.rect(2, 4, 32, 26, 2);
    b.rect(4, 6, 28, 22, 1);
    b.line(6, 8, 30, 26, 3, 1.2f);
    b.line(30, 8, 6, 26, 3, 1.2f);
    b.rect(2, 4, 32, 3, 3);
    b.rect(2, 15, 32, 2, 3);
    b.rect(2, 27, 32, 3, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(28, 36);
    b.ellipse(14, 18, 11, 14, 2);
    b.ellipse(14, 8, 9, 4, 1);
    b.ellipse(14, 8, 5, 2, 3);
    b.rect(4, 12, 20, 2.2f, 4);
    b.rect(4, 22, 20, 2.2f, 4);
    b.rect(12, 10, 2, 16, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(20, 44);
    b.rect(9, 0, 2, 16, 4);
    b.rect(2, 16, 16, 24, 1);
    b.rect(4, 18, 12, 20, 2);
    b.rect(6, 22, 8, 3, 5);
    b.rect(8, 28, 4, 6, 5);
    b.outline(3, false);
    return b;
}

gs::Bitmap laundryArt() {
    gs::Bitmap b(72, 28);
    b.rect(0, 2, 72, 2.2f, 3);
    b.rect(4, 8, 64, 16, 1);
    for (int i = 0; i < 4; i++) b.rect(8 + i * 16, 8, 3, 16, 2);
    b.rect(4, 20, 64, 3, 2);
    b.line(2, 4, 10, 10, 3, 1);
    b.line(62, 4, 54, 10, 3, 1);
    return b;
}

gs::Bitmap ventArt() {
    gs::Bitmap b(24, 32);
    b.rect(4, 22, 16, 8, 4);
    b.rect(6, 20, 12, 3, 3);
    for (int i = 0; i < 4; i++) b.rect(7, 23 + i * 2, 10, 1, 5);
    b.ellipse(12, 12, 6, 8, 1);
    b.ellipse(12, 10, 3, 4, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 32);
    b.rect(7, 0, 2, 14, 5);
    b.rect(4, 14, 8, 3, 4);
    b.ellipse(8, 22, 5, 6, 3);
    b.ellipse(8, 21, 3, 4, 2);
    b.ellipse(8, 20, 1.6f, 2, 1);
    return b;
}

gs::Bitmap endWall() {
    gs::Bitmap b(96, 120);
    brick(b, 0, 0, 96, 120);
    b.rect(30, 28, 36, 12, 4);
    b.rect(32, 30, 32, 8, 6);
    b.rect(46, 30, 3, 8, 4);
    b.rect(26, 44, 44, 70, 4);
    b.rect(30, 48, 36, 66, 10);
    b.rect(22, 112, 52, 6, 3);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(22, 72);
    b.rect(1, 2, 20, 68, 2);
    b.rect(3, 4, 16, 28, 1);
    b.rect(3, 36, 16, 28, 1);
    b.rect(18, 4, 2.2f, 64, 5);
    b.ellipse(16, 40, 2.2f, 2.2f, 7);
    b.rect(4, 66, 14, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(32, 72);
    b.rect(8, 4, 16, 64, 2);
    b.rect(12, 8, 8, 56, 1);
    b.ellipse(16, 20, 6, 10, 3);
    return b;
}

gs::Bitmap rainArt() {
    gs::Bitmap b(6, 14);
    b.line(1, 0, 4, 13, 1, 1);
    b.set(3, 12, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t dim = gs::rgb4(10, 10, 12);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t hud[16] = {0, ink, dim, gs::rgb4(15, 14, 12), gs::rgb4(12, 3, 3), gs::rgb4(8, 14, 8),
                              gs::rgb4(15, 12, 4), gs::rgb4(6, 8, 12), 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_HUD, hud);
    const uint16_t coat[16] = {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 7), gs::rgb4(8, 8, 10), gs::rgb4(12, 2, 3),
                               gs::rgb4(13, 9, 7), gs::rgb4(6, 6, 8), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_COAT, coat);
    const uint16_t brickp[16] = {0, gs::rgb4(7, 2, 2), gs::rgb4(4, 3, 3), gs::rgb4(4, 1, 1), gs::rgb4(2, 2, 3),
                                 gs::rgb4(2, 3, 6), gs::rgb4(15, 12, 5), gs::rgb4(5, 5, 6), gs::rgb4(12, 8, 6),
                                 gs::rgb4(9, 3, 3), gs::rgb4(2, 1, 1), 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_BRICK, brickp);
    const uint16_t warm[16] = {0, gs::rgb4(9, 3, 2), gs::rgb4(5, 2, 2), gs::rgb4(6, 2, 1), gs::rgb4(3, 2, 2),
                               gs::rgb4(3, 3, 6), gs::rgb4(15, 11, 4), gs::rgb4(6, 5, 5), gs::rgb4(13, 9, 6),
                               gs::rgb4(11, 4, 3), gs::rgb4(3, 1, 1), 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_WARM, warm);
    const uint16_t wood[16] = {0, gs::rgb4(10, 6, 2), gs::rgb4(7, 4, 1), gs::rgb4(4, 2, 1), gs::rgb4(3, 2, 2),
                               gs::rgb4(15, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(12, 9, 3), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0,
                               shadow};
    setPal(vdp, PAL_WOOD, wood);
    const uint16_t lamp[16] = {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 12, 3), gs::rgb4(15, 7, 1), gs::rgb4(10, 7, 2),
                               gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 5), 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_LAMP, lamp);
    const uint16_t cloth[16] = {0, gs::rgb4(11, 11, 12), gs::rgb4(7, 7, 9), gs::rgb4(4, 3, 2), gs::rgb4(6, 6, 7),
                                gs::rgb4(13, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_CLOTH, cloth);
    const uint16_t shade[16] = {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 5), gs::rgb4(3, 1, 2),
                                gs::rgb4(4, 3, 3), gs::rgb4(2, 2, 3), gs::rgb4(0, 0, 1), 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_SHADE, shade);
    const uint16_t rain[16] = {0, gs::rgb4(11, 12, 14), gs::rgb4(7, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_RAIN, rain);
    const uint16_t amber[16] = {0, gs::rgb4(15, 12, 4), gs::rgb4(12, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_AMBER, amber);
    const uint16_t alert[16] = {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow};
    setPal(vdp, PAL_ALERT, alert);

    const uint16_t field[16] = {
        0,       gs::rgb4(5, 2, 2), gs::rgb4(3, 1, 1), gs::rgb4(7, 3, 3), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 10), gs::rgb4(2, 2, 3), gs::rgb4(2, 3, 5), gs::rgb4(2, 2, 4),
        gs::rgb4(4, 4, 6), gs::rgb4(8, 8, 10), gs::rgb4(10, 9, 6), gs::rgb4(7, 7, 8)};
    setPal(vdp, PAL_FIELD, field);
    vdp.setFogColor(gs::rgb4(5, 4, 6));

    loadFont(vdp, art);
    art.walker[0] = gs::uploadMipped(vdp, walkerArt(0));
    art.walker[1] = gs::uploadMipped(vdp, walkerArt(1));
    art.crouch = gs::uploadMipped(vdp, crouchArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.laundry = gs::uploadMipped(vdp, laundryArt());
    art.vent = gs::uploadMipped(vdp, ventArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    for (int i = 0; i < 4; i++) art.wall[i] = gs::uploadMipped(vdp, wallArt(i));
    art.endwall = gs::uploadMipped(vdp, endWall());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.rain = gs::uploadMipped(vdp, rainArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace alley
