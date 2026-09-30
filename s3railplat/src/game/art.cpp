#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace railplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void bogie(gs::Bitmap& b, float cx, float phase) {
    b.rect(int(cx) - 14, 50, 28, 4, 2);
    for (int k = 0; k < 2; k++) {
        float wx = cx - 7.f + k * 14.f;
        b.ellipse(wx, 56, 6, 6, 8);
        b.ellipse(wx, 56, 2, 2, 1);
        for (int s = 0; s < 3; s++) {
            float a = phase + s * 1.0472f;
            b.line(wx, 56, wx + std::cos(a) * 5.f, 56 + std::sin(a) * 5.f, 9, 1.f);
        }
    }
}

gs::Bitmap coachArt(int phase) {
    gs::Bitmap b(148, 64);
    b.rect(8, 48, 132, 4, 2);
    b.rect(6, 16, 136, 34, 3);
    b.rect(6, 16, 136, 4, 4);
    b.rect(6, 46, 136, 4, 5);
    // door the driver has to put on the stripe
    b.rect(62, 22, 22, 26, 6);
    b.rect(64, 24, 8, 16, 10);
    b.rect(74, 24, 8, 16, 11);
    b.rect(71, 34, 2, 4, 12);
    b.rect(14, 22, 16, 12, 7);
    b.rect(34, 22, 16, 12, 7);
    b.rect(96, 22, 16, 12, 7);
    b.rect(118, 22, 16, 12, 7);
    b.rect(16, 24, 6, 6, 10);
    b.rect(36, 24, 6, 6, 10);
    b.rect(98, 24, 6, 6, 10);
    b.rect(120, 24, 6, 6, 10);
    b.rect(4, 28, 4, 16, 8);
    b.rect(140, 28, 4, 16, 8);
    b.ellipse(74, 12, 3, 3, 12);
    bogie(b, 36, phase * 1.1f);
    bogie(b, 112, phase * 1.1f + 0.4f);
    b.outline(1, false);
    return b;
}

gs::Bitmap platArt() {
    gs::Bitmap b(168, 72);
    b.rect(0, 40, 168, 28, 3);
    b.rect(0, 40, 168, 3, 4);
    b.rect(0, 66, 168, 4, 2);
    for (int x = 6; x < 160; x += 16) b.rect(x, 48, 8, 3, 5);
    // shelter and the door the stripe belongs to
    b.rect(18, 18, 130, 24, 6);
    b.rect(18, 18, 130, 3, 7);
    b.poly({{10, 20}, {84, 4}, {156, 20}}, 8);
    b.rect(70, 22, 22, 20, 9);
    b.rect(72, 24, 8, 14, 10);
    b.rect(82, 24, 8, 14, 11);
    b.rect(28, 24, 14, 10, 12);
    b.rect(120, 24, 14, 10, 12);
    // yellow level stripe under the platform door
    b.rect(68, 42, 26, 4, 13);
    b.rect(68, 40, 2, 8, 13);
    b.rect(92, 40, 2, 8, 13);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(32, 18);
    b.rect(0, 4, 32, 12, 3);
    for (int x = 2; x < 32; x += 8) b.rect(x, 6, 4, 8, 2);
    b.rect(0, 5, 32, 2, 4);
    b.rect(0, 13, 32, 2, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 46);
    b.rect(7, 16, 4, 28, 2);
    b.rect(3, 4, 12, 14, 3);
    b.rect(5, 6, 8, 8, 4);
    b.ellipse(9, 10, 2, 2, 5);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 48);
    b.rect(12, 26, 4, 20, 5);
    b.ellipse(14, 16, 12, 13, 2);
    b.ellipse(10, 14, 6, 6, 3);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(96, 36);
    b.poly({{0, 35}, {18, 18}, {40, 26}, {62, 6}, {84, 20}, {95, 35}}, 6);
    b.poly({{34, 35}, {62, 12}, {90, 35}}, 7);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 18);
    b.ellipse(12, 11, 10, 6, 2);
    b.ellipse(26, 9, 12, 7, 1);
    b.ellipse(40, 11, 7, 5, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 5, 1);
    b.ellipse(7, 7, 2, 2, 2);
    return b;
}

gs::Bitmap bufferArt() {
    gs::Bitmap b(22, 28);
    b.rect(4, 8, 6, 18, 2);
    b.rect(12, 8, 6, 18, 2);
    b.rect(2, 6, 18, 6, 3);
    b.rect(6, 8, 10, 3, 4);
    return b;
}

gs::Image words(gs::VDP& vdp, const char* s, int scale, int color) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, color, 0, 15, 1}));
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 96; c++) {
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
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 10, 12));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(4, 15, 8));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));

    setPal(vdp, PAL_COACH,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 3, 3), gs::rgb4(15, 6, 5), gs::rgb4(8, 2, 2),
            gs::rgb4(4, 5, 7), gs::rgb4(9, 13, 15), gs::rgb4(2, 2, 3), gs::rgb4(10, 10, 11), gs::rgb4(13, 15, 15),
            gs::rgb4(7, 11, 13), gs::rgb4(15, 13, 3), 0, 0, 0});
    setPal(vdp, PAL_PLAT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(5, 5, 6),
            gs::rgb4(9, 8, 6), gs::rgb4(12, 11, 8), gs::rgb4(6, 5, 4), gs::rgb4(3, 4, 6), gs::rgb4(12, 14, 15),
            gs::rgb4(6, 9, 11), gs::rgb4(14, 14, 12), gs::rgb4(15, 12, 2), 0, 0});
    setPal(vdp, PAL_LAND,
           {0, gs::rgb4(14, 14, 15), gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4), gs::rgb4(8, 8, 9), gs::rgb4(6, 4, 2),
            gs::rgb4(3, 6, 5), gs::rgb4(5, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 6), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 14, 15), gs::rgb4(9, 9, 11), gs::rgb4(15, 8, 3), gs::rgb4(15, 14, 6), 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0});

    vdp.setFogColor(gs::rgb4(6, 8, 11));
    art.coach[0] = gs::uploadImage(vdp, coachArt(0));
    art.coach[1] = gs::uploadImage(vdp, coachArt(1));
    art.plat = gs::uploadImage(vdp, platArt());
    art.rail = gs::uploadImage(vdp, railArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.tree = gs::uploadImage(vdp, treeArt());
    art.hill = gs::uploadImage(vdp, hillArt());
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.puff = gs::uploadImage(vdp, puffArt());
    art.buffer = gs::uploadImage(vdp, bufferArt());
    art.logo = words(vdp, "RAIL PLAT", 3, 1);
    art.tag = words(vdp, "LEVEL WITH THE PLATFORM", 1, 1);
    art.banLevel = words(vdp, "LEVEL", 3, 1);
    art.banShort = words(vdp, "SHORT", 3, 1);
    art.banPast = words(vdp, "PAST", 3, 1);
    art.banCrew = words(vdp, "BERTH TAKEN", 2, 1);
    loadFont(vdp, art);
}

}  // namespace railplat
