#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace keys {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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

gs::Bitmap noteArt() {
    gs::Bitmap b(20, 22);
    b.rect(13, 1, 2, 12, 1);
    b.poly({{15, 1}, {19, 5}, {15, 8}}, 1);
    b.ellipse(9, 14, 7, 5, 1);
    b.ellipse(8, 13, 3, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap keyArt(char letter) {
    gs::Bitmap b(34, 24);
    b.rect(1, 1, 32, 22, 1);
    b.rect(1, 16, 32, 7, 2);
    b.rect(2, 2, 8, 4, 5);
    gs::Bitmap glyph = gs::textBitmap(std::string(1, letter), gs::TextStyle{2, 3, 0, 0, 1});
    b.blit(glyph, 17 - glyph.w / 2, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(32, 16);
    b.rect(0, 0, 32, 2, 1);
    b.rect(0, 14, 32, 2, 1);
    b.rect(0, 0, 2, 16, 1);
    b.rect(30, 0, 2, 16, 1);
    b.rect(4, 2, 2, 3, 1);
    b.rect(26, 2, 2, 3, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 16);
    b.rect(5, 10, 2, 6, 3);
    b.ellipse(6, 7, 4, 5, 1);
    b.ellipse(6, 6, 2, 3, 2);
    b.set(6, 5, 4);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(26, 34);
    b.rect(12, 0, 2, 6, 3);
    b.poly({{5, 8}, {21, 8}, {18, 22}, {8, 22}}, 3);
    b.poly({{7, 10}, {19, 10}, {16, 20}, {10, 20}}, 1);
    b.ellipse(13, 15, 3, 4, 2);
    b.rect(8, 22, 10, 3, 3);
    b.rect(11, 25, 4, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 10, 10, 1);
    b.ellipse(12, 12, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785398f;
        b.line(12, 12, 12 + std::cos(a) * 11, 12 + std::sin(a) * 11, 2, 1.5f);
    }
    return b;
}

// Hall: curtains, moon, window, lane carpets, piano. Sky stays transparent.
void paintHall(gs::Bitmap& b) {
    b.ellipse(58, 40, 13, 13, 14);
    b.ellipse(64, 36, 11, 11, 0);

    b.rect(248, 36, 24, 70, 15);
    b.rect(251, 39, 18, 64, 8);
    b.rect(259, 39, 2, 64, 15);
    b.rect(251, 68, 18, 2, 15);
    b.rect(256, 44, 4, 6, 14);

    for (int x = 0; x < 46; x++) {
        int fold = (x % 9 < 2) ? 3 : (x % 9 > 6) ? 2 : 1;
        for (int y = 0; y < 176; y++) b.set(x, y, fold);
        for (int y = 0; y < 176; y++) b.set(gs::SCREEN_W - 1 - x, y, fold);
    }
    for (int y = 0; y < 18; y++) {
        for (int x = 0; x < gs::SCREEN_W; x++) {
            int scallop = 14 + int(std::sin(x * 0.08f) * 4);
            if (y > scallop && x > 46 && x < gs::SCREEN_W - 46) continue;
            int fold = ((x / 8) % 3 == 0) ? 3 : ((x / 8) % 3 == 1) ? 1 : 2;
            b.set(x, y, fold);
        }
    }
    b.rect(40, 16, 8, 6, 12);
    b.rect(gs::SCREEN_W - 48, 16, 8, 6, 12);

    for (int lane = 0; lane < kLanes; lane++) {
        int x0 = kLaneX[lane] - 10;
        for (int y = 28; y < 168; y++) {
            for (int x = x0; x < x0 + 20; x++) b.set(x, y, (x == x0 || x == x0 + 19) ? 8 : 7);
        }
        for (int x = kLaneX[lane] - 16; x < kLaneX[lane] + 16; x++) {
            b.set(x, kHitY - 2, 12);
            b.set(x, kHitY - 1, 12);
            b.set(x, kHitY, 14);
            b.set(x, kHitY + 1, 13);
        }
    }

    b.rect(50, 168, 220, 36, 5);
    b.rect(50, 168, 220, 4, 6);
    b.rect(58, 174, 204, 22, 4);
    for (int lane = 0; lane < kLanes; lane++) {
        int x0 = kLaneX[lane] - 14;
        b.rect(float(x0), 176, 28, 18, 9);
        b.rect(float(x0), 188, 28, 6, 10);
        if (lane + 1 < kLanes) {
            int bx = (kLaneX[lane] + kLaneX[lane + 1]) / 2 - 4;
            b.rect(float(bx), 174, 8, 12, 11);
        }
    }
    for (int i = 0; i < 8; i++) b.ellipse(70.0f + i * 24.0f, 204, 3, 2, i % 2 ? 12 : 14);

    for (int y = 206; y < gs::SCREEN_H; y++) {
        int plank = ((y / 5) & 1) ? 4 : 5;
        for (int x = 0; x < gs::SCREEN_W; x++) {
            if (b.get(x, y)) continue;
            b.set(x, y, (x % 40 == 0) ? 6 : plank);
        }
    }
    for (int i = 0; i < kLamps; i++) b.rect(float(kLampX[i] - 1), 16, 3, 8, 13);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 0, 2);
    const uint16_t ink = gs::rgb4(15, 15, 15);
    auto textPal = [&](int pal, uint16_t c) {
        setPal(vdp, pal, {0, c, gs::rgb4(8, 8, 10), gs::rgb4(15, 14, 8), gs::rgb4(15, 4, 3), gs::rgb4(6, 15, 8),
                          gs::rgb4(8, 12, 15), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    };
    textPal(PAL_WHITE, ink);
    textPal(PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(PAL_GOOD, gs::rgb4(8, 15, 7));

    auto notePal = [&](int pal, uint16_t body, uint16_t hi, uint16_t edge) {
        setPal(vdp, pal, {0, body, hi, edge, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    };
    notePal(PAL_N0, gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 8), gs::rgb4(6, 3, 0));
    notePal(PAL_N1, gs::rgb4(15, 4, 6), gs::rgb4(15, 12, 12), gs::rgb4(6, 1, 2));
    notePal(PAL_N2, gs::rgb4(4, 14, 8), gs::rgb4(12, 15, 12), gs::rgb4(1, 5, 3));
    notePal(PAL_N3, gs::rgb4(5, 10, 15), gs::rgb4(13, 15, 15), gs::rgb4(1, 3, 7));

    setPal(vdp, PAL_KEY, {0, gs::rgb4(14, 13, 11), gs::rgb4(8, 7, 6), gs::rgb4(2, 1, 3), gs::rgb4(3, 2, 4),
                          gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_KEYLIT, {0, gs::rgb4(15, 15, 12), gs::rgb4(12, 10, 6), gs::rgb4(4, 2, 1), gs::rgb4(8, 6, 1),
                             gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 15), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 10), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 5, 4), gs::rgb4(8, 7, 5), gs::rgb4(3, 2, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_STAGE,
           {0, gs::rgb4(9, 1, 2), gs::rgb4(14, 4, 4), gs::rgb4(4, 0, 1), gs::rgb4(9, 5, 2), gs::rgb4(5, 3, 1),
            gs::rgb4(13, 9, 5), gs::rgb4(3, 3, 8), gs::rgb4(6, 6, 12), gs::rgb4(14, 13, 11), gs::rgb4(8, 7, 6),
            gs::rgb4(1, 1, 2), gs::rgb4(14, 11, 3), gs::rgb4(8, 6, 1), gs::rgb4(15, 14, 9), gs::rgb4(2, 1, 3)});

    loadFont(vdp, art);
    art.note = gs::uploadMipped(vdp, noteArt());
    static const char kLetter[kLanes] = {'L', 'D', 'U', 'R'};
    for (int i = 0; i < kLanes; i++) art.key[i] = gs::uploadMipped(vdp, keyArt(kLetter[i]));
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.flash = gs::uploadMipped(vdp, flashArt());

    gs::Bitmap hall(gs::SCREEN_W, gs::SCREEN_H);
    paintHall(hall);
    gs::TileAlloc stage(vdp, 128);
    vdp.B.clear();
    gs::bitmapToPlane(stage, vdp.B, 0, 0, hall, PAL_STAGE);
    vdp.A.enabled = false;
    vdp.B.scroll(0, 0);
    vdp.setFogColor(gs::rgb4(2, 1, 4));
}

}  // namespace keys
