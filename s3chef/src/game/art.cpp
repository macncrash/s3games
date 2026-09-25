#include "game/art.h"

#include <initializer_list>

namespace chef {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void plate(gs::Bitmap& b, float cy) {
    b.ellipse(22, cy, 18, 6, 12);
    b.ellipse(22, cy - 1, 14, 4, 2);
}

gs::Bitmap burger() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.ellipse(22, 24, 13, 5, 3);
    b.ellipse(22, 22, 12, 3, 4);
    b.ellipse(22, 19, 13, 3, 5);
    b.ellipse(22, 17, 14, 3, 7);
    b.rect(10, 15, 24, 3, 8);
    b.ellipse(22, 14, 11, 2, 6);
    b.ellipse(22, 10, 13, 6, 3);
    b.ellipse(16, 8, 5, 2, 13);
    b.ellipse(15, 9, 1.2f, 1.0f, 2);
    b.ellipse(22, 7, 1.2f, 1.0f, 2);
    b.ellipse(29, 10, 1.2f, 1.0f, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap trout() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.poly({{6, 20}, {16, 14}, {16, 26}}, 10);
    b.ellipse(26, 20, 12, 6, 10);
    b.ellipse(24, 21, 8, 3, 2);
    b.ellipse(32, 18, 2, 2, 1);
    b.ellipse(33, 17, 1, 1, 2);
    b.poly({{28, 24}, {38, 30}, {24, 28}}, 13);
    b.outline(1, false);
    return b;
}

gs::Bitmap steak() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.ellipse(22, 18, 14, 8, 4);
    b.ellipse(20, 17, 10, 5, 5);
    b.rect(12, 15, 16, 2, 14);
    b.rect(14, 19, 14, 2, 14);
    b.ellipse(30, 14, 3, 2, 2);
    b.ellipse(16, 22, 3, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap omelet() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.ellipse(22, 18, 14, 8, 8);
    b.ellipse(18, 16, 8, 5, 13);
    b.ellipse(26, 20, 6, 3, 9);
    b.rect(14, 18, 6, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap bisque() {
    gs::Bitmap b(44, 36);
    b.ellipse(22, 24, 16, 8, 12);
    b.ellipse(22, 20, 13, 7, 9);
    b.ellipse(22, 19, 10, 5, 13);
    b.ellipse(18, 18, 4, 2, 2);
    b.ellipse(26, 16, 2, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap ribs() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.rect(10, 16, 22, 5, 6);
    b.rect(12, 14, 18, 4, 4);
    b.rect(14, 12, 3, 12, 2);
    b.rect(22, 12, 3, 12, 2);
    b.rect(30, 12, 3, 12, 2);
    b.ellipse(18, 15, 3, 2, 9);
    b.outline(1, false);
    return b;
}

gs::Bitmap taco() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.line(10, 12, 16, 26, 3, 5);
    b.line(34, 12, 28, 26, 3, 5);
    b.ellipse(22, 26, 10, 4, 3);
    b.ellipse(22, 18, 8, 5, 7);
    b.ellipse(22, 16, 6, 3, 4);
    b.rect(16, 14, 12, 2, 8);
    b.ellipse(18, 15, 2, 2, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap noodle() {
    gs::Bitmap b(44, 36);
    b.ellipse(22, 24, 16, 8, 12);
    b.ellipse(22, 20, 12, 6, 9);
    b.ellipse(16, 18, 5, 2, 8);
    b.ellipse(24, 17, 6, 2, 13);
    b.ellipse(30, 20, 4, 2, 8);
    b.ellipse(20, 16, 3, 2, 2);
    b.rect(26, 15, 5, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap pie() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.ellipse(22, 22, 14, 6, 12);
    b.ellipse(22, 18, 13, 7, 3);
    b.ellipse(22, 16, 10, 5, 13);
    b.rect(16, 14, 12, 5, 6);
    b.rect(18, 12, 8, 3, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap chops() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.ellipse(16, 18, 8, 6, 4);
    b.ellipse(28, 18, 8, 6, 5);
    b.rect(14, 10, 3, 10, 2);
    b.rect(26, 10, 3, 10, 2);
    b.rect(12, 16, 8, 2, 14);
    b.rect(24, 17, 8, 2, 14);
    b.ellipse(34, 14, 3, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap burntDish() {
    gs::Bitmap b(44, 36);
    plate(b, 30);
    b.ellipse(22, 18, 13, 7, 5);
    b.ellipse(18, 16, 7, 4, 14);
    b.ellipse(27, 17, 5, 3, 1);
    b.ellipse(22, 8, 4, 3, 12);
    b.ellipse(28, 6, 3, 2, 12);
    b.outline(1, false);
    return b;
}

gs::Bitmap chefArt(bool reach) {
    gs::Bitmap b(40, 48);
    b.ellipse(20, 8, 12, 6, 8);
    b.rect(10, 10, 20, 6, 8);
    b.rect(10, 15, 20, 3, 5);
    b.ellipse(20, 24, 8, 7, 2);
    b.rect(15, 22, 3, 3, 7);
    b.rect(22, 22, 3, 3, 7);
    b.rect(16, 28, 8, 1, 6);
    b.rect(10, 32, 20, 14, 4);
    b.rect(17, 34, 6, 10, 5);
    b.ellipse(20, 36, 1.2f, 1.2f, 10);
    b.ellipse(20, 40, 1.2f, 1.2f, 10);
    b.poly({{14, 32}, {26, 32}, {23, 38}, {17, 38}}, 6);
    if (!reach) {
        b.rect(3, 34, 7, 10, 4);
        b.rect(30, 34, 7, 10, 4);
        b.ellipse(6, 44, 4, 3, 2);
        b.ellipse(34, 44, 4, 3, 2);
    } else {
        b.rect(1, 28, 9, 6, 4);
        b.rect(30, 28, 9, 6, 4);
        b.ellipse(4, 28, 3, 3, 2);
        b.ellipse(36, 28, 3, 3, 2);
    }
    b.rect(14, 38, 12, 8, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap panArt() {
    gs::Bitmap b(64, 28);
    b.ellipse(26, 14, 22, 11, 3);
    b.ellipse(26, 14, 17, 8, 4);
    b.ellipse(26, 14, 13, 6, 5);
    b.ellipse(20, 11, 5, 2, 2);
    b.poly({{44, 10}, {62, 8}, {63, 16}, {60, 20}, {44, 18}}, 6);
    b.ellipse(58, 14, 3, 3, 7);
    b.ellipse(42, 13, 1.4f, 1.4f, 8);
    b.outline(1, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(28, 20);
    float h = frame == 0 ? 14.f : frame == 1 ? 17.f : 12.f;
    b.ellipse(14, 16, 10, 4, 4);
    b.ellipse(14, 18 - h * 0.35f, 7, h * 0.45f, 3);
    b.ellipse(10, 14, 4, h * 0.35f, 3);
    b.ellipse(18, 15, 4, h * 0.3f, 4);
    b.ellipse(14, 12, 3, h * 0.28f, 2);
    b.ellipse(14, 10, 1.5f, 2, 1);
    return b;
}

gs::Bitmap ticketArt() {
    gs::Bitmap b(56, 50);
    b.rect(4, 2, 48, 46, 2);
    b.rect(4, 2, 48, 14, 4);
    b.rect(8, 20, 40, 2, 3);
    b.rect(8, 26, 32, 2, 3);
    b.rect(8, 32, 36, 2, 3);
    b.rect(8, 38, 24, 2, 3);
    b.outline(1, false);
    for (int y = 42; y <= 48; y++)
        for (int x = 24; x <= 32; x++)
            if ((x - 28) * (x - 28) + (y - 45) * (y - 45) <= 8) b.set(x, y, 0);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 16);
    b.poly({{8, 2}, {13, 7}, {12, 12}, {4, 12}, {3, 7}}, 1);
    b.ellipse(8, 12, 6, 3, 1);
    b.ellipse(8, 6, 2, 2, 2);
    b.rect(7, 0, 2, 3, 1);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(72, 64);
    b.rect(2, 2, 16, 4, 1);
    b.rect(2, 2, 4, 16, 1);
    b.rect(54, 2, 16, 4, 1);
    b.rect(66, 2, 4, 16, 1);
    b.rect(2, 58, 16, 4, 1);
    b.rect(2, 46, 4, 16, 1);
    b.rect(54, 58, 16, 4, 1);
    b.rect(66, 46, 4, 16, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(40, 22);
    b.poly({{4, 4}, {36, 4}, {40, 18}, {0, 18}}, 4);
    b.poly({{8, 6}, {32, 6}, {35, 16}, {5, 16}}, 3);
    b.ellipse(20, 14, 7, 3, 2);
    b.rect(16, 0, 8, 5, 5);
    return b;
}

gs::Bitmap bottleArt() {
    gs::Bitmap b(14, 28);
    b.rect(4, 1, 6, 5, 3);
    b.rect(5, 6, 4, 4, 2);
    b.ellipse(7, 18, 5, 8, 2);
    b.rect(3, 13, 8, 5, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap shakerArt() {
    gs::Bitmap b(12, 24);
    b.rect(2, 1, 8, 4, 7);
    b.rect(3, 5, 6, 16, 6);
    b.set(4, 2, 1);
    b.set(7, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap steamArt() {
    gs::Bitmap b(12, 16);
    b.ellipse(6, 11, 4, 3, 1);
    b.ellipse(6, 6, 2, 3, 1);
    return b;
}

gs::Bitmap solidArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        a.font[c - 32] = tiles.shared(px);
    }
}

void stampKitchen(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t wall[64], steel[64], wood[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            wall[y * 8 + x] = (x == 7 || y == 7) ? 3 : 2;
            steel[y * 8 + x] = y == 0 ? 5 : y == 7 ? 6 : 4;
            wood[y * 8 + x] = (y % 4 == 3) ? 8 : 7;
        }
    int w = tiles.shared(wall);
    int s = tiles.shared(steel);
    int f = tiles.shared(wood);
    for (int cy = 2; cy <= 14; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(w, PAL_HUD));
    for (int cy = 15; cy <= 20; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(s, PAL_HUD));
    for (int cy = 21; cy <= 27; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(f, PAL_HUD));
}

gs::Mipped say(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st{scale, 1, 0, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[16] = {0,
                              gs::rgb4(15, 15, 15),
                              gs::rgb4(14, 11, 9),
                              gs::rgb4(6, 4, 3),
                              gs::rgb4(8, 9, 10),
                              gs::rgb4(13, 14, 15),
                              gs::rgb4(4, 5, 6),
                              gs::rgb4(6, 3, 2),
                              gs::rgb4(3, 2, 1),
                              0,
                              0,
                              0,
                              0,
                              0,
                              0,
                              gs::rgb4(1, 0, 0)};
    const uint16_t gold[16] = {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)};
    const uint16_t red[16] = {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 0, 0)};
    const uint16_t green[16] = {0, gs::rgb4(5, 14, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)};
    const uint16_t chef[16] = {0,
                               gs::rgb4(2, 1, 1),
                               gs::rgb4(15, 12, 9),
                               gs::rgb4(12, 8, 6),
                               gs::rgb4(15, 15, 15),
                               gs::rgb4(12, 13, 14),
                               gs::rgb4(13, 2, 2),
                               gs::rgb4(2, 1, 1),
                               gs::rgb4(14, 14, 15),
                               gs::rgb4(6, 4, 3),
                               gs::rgb4(15, 12, 3),
                               0,
                               0,
                               0,
                               0,
                               gs::rgb4(1, 1, 1)};
    const uint16_t food[16] = {0,
                               gs::rgb4(2, 1, 1),
                               gs::rgb4(15, 14, 11),
                               gs::rgb4(13, 8, 3),
                               gs::rgb4(9, 5, 2),
                               gs::rgb4(5, 2, 1),
                               gs::rgb4(13, 2, 1),
                               gs::rgb4(3, 11, 2),
                               gs::rgb4(15, 12, 2),
                               gs::rgb4(15, 7, 1),
                               gs::rgb4(15, 9, 8),
                               gs::rgb4(15, 15, 14),
                               gs::rgb4(10, 11, 12),
                               gs::rgb4(14, 10, 3),
                               gs::rgb4(3, 2, 1),
                               gs::rgb4(1, 0, 0)};
    const uint16_t steel[16] = {0,
                                gs::rgb4(1, 1, 1),
                                gs::rgb4(14, 15, 15),
                                gs::rgb4(9, 10, 11),
                                gs::rgb4(5, 6, 7),
                                gs::rgb4(2, 2, 3),
                                gs::rgb4(10, 6, 3),
                                gs::rgb4(6, 3, 2),
                                gs::rgb4(15, 14, 8),
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                gs::rgb4(1, 1, 2)};
    const uint16_t fire[16] = {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 2), gs::rgb4(15, 7, 1), gs::rgb4(13, 2, 1), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t paper[16] = {0, gs::rgb4(2, 1, 1), gs::rgb4(15, 14, 11), gs::rgb4(12, 10, 8), gs::rgb4(13, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(6, 4, 3)};
    const uint16_t ready[16] = {0, gs::rgb4(3, 1, 0), gs::rgb4(15, 13, 6), gs::rgb4(12, 8, 2), gs::rgb4(2, 11, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)};
    const uint16_t track[16] = {0, gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 0, 0)};
    const uint16_t zone[16] = {0, gs::rgb4(10, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t raw[16] = {0, gs::rgb4(3, 8, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t ok[16] = {0, gs::rgb4(15, 12, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t hot[16] = {0, gs::rgb4(15, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t decor[16] = {0,
                                gs::rgb4(2, 1, 1),
                                gs::rgb4(12, 1, 1),
                                gs::rgb4(15, 15, 15),
                                gs::rgb4(15, 14, 12),
                                gs::rgb4(15, 8, 8),
                                gs::rgb4(14, 14, 13),
                                gs::rgb4(4, 4, 5),
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                gs::rgb4(1, 1, 1)};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_RED, red);
    setPal(vdp, PAL_GREEN, green);
    setPal(vdp, PAL_CHEF, chef);
    setPal(vdp, PAL_FOOD, food);
    setPal(vdp, PAL_STEEL, steel);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_PAPER, paper);
    setPal(vdp, PAL_READY, ready);
    setPal(vdp, PAL_TRACK, track);
    setPal(vdp, PAL_ZONE, zone);
    setPal(vdp, PAL_RAW, raw);
    setPal(vdp, PAL_OK, ok);
    setPal(vdp, PAL_HOT, hot);
    setPal(vdp, PAL_DECOR, decor);

    gs::TileAlloc tiles(vdp);
    stampKitchen(vdp, tiles);
    loadFont(vdp, tiles, art);

    art.word[WORD_TITLE] = say(vdp, "S3 CHEF", 4);
    art.word[WORD_SUB] = say(vdp, "TEN TICKETS", 2);
    art.word[WORD_WIN] = say(vdp, "SERVICE", 3);
    art.word[WORD_BURN] = say(vdp, "BURNED", 3);

    using DishFn = gs::Bitmap (*)();
    const DishFn dishes[TICKETS] = {burger, trout, steak, omelet, bisque, ribs, taco, noodle, pie, chops};
    for (int i = 0; i < TICKETS; i++) art.dish[i] = gs::uploadMipped(vdp, dishes[i]());
    art.burnt = gs::uploadMipped(vdp, burntDish());
    art.chef[0] = gs::uploadMipped(vdp, chefArt(false));
    art.chef[1] = gs::uploadMipped(vdp, chefArt(true));
    art.pan = gs::uploadMipped(vdp, panArt());
    for (int i = 0; i < 3; i++) art.flame[i] = gs::uploadMipped(vdp, flameArt(i));
    art.ticket = gs::uploadMipped(vdp, ticketArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.bracket = gs::uploadMipped(vdp, bracketArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bottle = gs::uploadMipped(vdp, bottleArt());
    art.shaker = gs::uploadMipped(vdp, shakerArt());
    art.steam = gs::uploadMipped(vdp, steamArt());
    art.solid = gs::uploadMipped(vdp, solidArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());

    vdp.A.enabled = true;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace chef
