#include "game/art.h"

#include <cmath>
#include <string>
#include <vector>

namespace sledplat {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 20.f;

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

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

SledImg drawSled(gs::VDP& vdp, float att) {
    const float ox = 118.f, oy = 108.f;
    Bitmap b(250, 170);
    const float c = std::cos(att), s = std::sin(att);
    auto P = [&](float mx, float my) -> Pt {
        float wx = mx * c - my * s;
        float wy = mx * s + my * c;
        return {ox + wx * PPM, oy - wy * PPM};
    };
    auto quad = [&](float x0, float y0, float x1, float y1, int col) {
        b.poly({P(x0, y0), P(x1, y0), P(x1, y1), P(x0, y1)}, col);
    };
    auto line = [&](float x0, float y0, float x1, float y1, int col, float th) {
        Pt a = P(x0, y0), d = P(x1, y1);
        b.line(a.first, a.second, d.first, d.second, col, th);
    };
    auto blob = [&](float mx, float my, float rx, float ry, int col) {
        Pt p = P(mx, my);
        b.ellipse(p.first, p.second, rx * PPM, ry * PPM, col);
    };

    // Rear curl, runner, and the brass horn at the nose.
    line(-2.15f, 0.05f, -2.42f, 0.28f, 2, 3.2f);
    quad(-2.28f, 0.0f, 1.92f, 0.09f, 1);
    line(-2.05f, 0.07f, 1.75f, 0.07f, 2, 2.2f);
    line(1.72f, 0.06f, 2.18f, 0.62f, 2, 3.4f);
    line(1.78f, 0.10f, 2.08f, 0.52f, 13, 2.0f);
    blob(2.12f, 0.66f, 0.09f, 0.09f, 13);

    // Stanchions and the slat bed.
    for (float x : {-1.85f, -0.95f, 0.15f, 1.15f}) line(x, 0.08f, x, 0.34f, 4, 2.4f);
    quad(-2.02f, 0.22f, 1.58f, 0.38f, 3);
    for (float x = -1.7f; x < 1.4f; x += 0.34f) line(x, 0.24f, x, 0.36f, 4, 1.4f);

    // Freight box. The brass seam at x = 0 is the door that has to meet the bay.
    quad(-0.78f, 0.38f, 1.05f, 1.08f, 5);
    quad(-0.70f, 0.46f, 0.96f, 1.00f, 6);
    quad(-0.05f, 0.40f, 0.05f, 1.06f, 13);
    blob(0.0f, 0.72f, 0.045f, 0.045f, 14);
    line(-0.62f, 0.42f, 0.88f, 0.42f, 7, 1.6f);
    line(-0.55f, 1.02f, 0.78f, 0.48f, 7, 1.5f);
    line(-0.50f, 0.48f, 0.82f, 1.00f, 7, 1.5f);
    quad(-0.55f, 0.96f, 0.72f, 1.08f, 14);

    // Musher at the back, hands on the brake bar.
    quad(-1.78f, 0.36f, -1.58f, 0.78f, 11);
    quad(-1.52f, 0.36f, -1.32f, 0.78f, 11);
    quad(-1.86f, 0.74f, -1.24f, 1.22f, 8);
    quad(-1.78f, 0.82f, -1.32f, 1.14f, 12);
    line(-1.70f, 1.05f, -2.22f, 0.16f, 4, 2.6f);
    line(-2.28f, 0.10f, -2.08f, 0.18f, 11, 2.4f);
    blob(-1.55f, 1.36f, 0.15f, 0.15f, 9);
    blob(-1.50f, 1.40f, 0.035f, 0.035f, 11);
    quad(-1.74f, 1.42f, -1.36f, 1.58f, 10);
    blob(-1.55f, 1.52f, 0.16f, 0.08f, 10);
    line(-1.28f, 0.92f, -0.95f, 0.70f, 8, 2.2f);

    b.outline(11, false);

    int x0 = b.w, y0 = b.h, x1 = -1, y1 = -1;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            if (!b.get(x, y)) continue;
            x0 = std::min(x0, x);
            y0 = std::min(y0, y);
            x1 = std::max(x1, x);
            y1 = std::max(y1, y);
        }
    }
    Bitmap cropped(std::max(1, x1 - x0 + 1), std::max(1, y1 - y0 + 1));
    if (x1 >= x0) {
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    }
    SledImg sled;
    sled.img = gs::uploadMipped(vdp, cropped);
    sled.ax = ox - float(x0);
    sled.ay = oy - float(y0);
    sled.ppm = PPM;
    return sled;
}

Bitmap plankArt() {
    Bitmap b(48, 14);
    b.rect(0, 0, 48, 14, 1);
    for (int x = 0; x < 48; x++) {
        b.set(x, 0, 2);
        b.set(x, 13, 4);
        if (x % 12 == 0)
            for (int y = 0; y < 14; y++) b.set(x, y, 3);
    }
    for (int i = 0; i < 10; i++) b.set(int(hash2(i, 2) % 48), 4 + int(hash2(i, 4) % 7), 3);
    return b;
}

Bitmap fasciaArt() {
    Bitmap b(40, 16);
    for (int x = 0; x < 40; x++) {
        int col = ((x / 5) & 1) ? 1 : 2;
        for (int y = 0; y < 16; y++) b.set(x, y, col);
        b.set(x, 0, 3);
        b.set(x, 15, 4);
    }
    return b;
}

Bitmap riserArt() {
    Bitmap b(12, 20);
    b.rect(0, 0, 12, 20, 1);
    b.rect(0, 0, 3, 20, 2);
    b.rect(9, 0, 3, 20, 4);
    return b;
}

Bitmap trestleArt() {
    Bitmap b(18, 40);
    b.rect(6, 0, 6, 40, 1);
    b.rect(7, 0, 2, 40, 2);
    for (int y = 6; y < 40; y += 10) b.rect(1, y, 16, 3, 3);
    b.outline(4, false);
    return b;
}

Bitmap houseArt() {
    Bitmap b(96, 80);
    b.poly({{8, 34}, {48, 8}, {88, 34}}, 6);
    b.poly({{18, 30}, {48, 14}, {78, 30}}, 7);
    b.rect(14, 32, 68, 44, 1);
    b.rect(16, 34, 64, 40, 2);
    b.rect(40, 46, 16, 30, 4);
    b.rect(43, 50, 10, 18, 3);
    b.rect(22, 42, 12, 12, 5);
    b.rect(62, 42, 12, 12, 5);
    b.rect(24, 44, 4, 4, 8);
    b.rect(64, 44, 4, 4, 8);
    b.rect(70, 18, 8, 18, 3);
    b.rect(72, 10, 4, 10, 9);
    b.rect(12, 74, 72, 4, 3);
    b.outline(3, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(22, 72);
    b.rect(8, 16, 6, 56, 1);
    b.rect(9, 16, 2, 56, 2);
    b.rect(2, 14, 18, 5, 3);
    b.ellipse(11, 8, 6, 6, 4);
    b.ellipse(11, 8, 3, 3, 5);
    b.outline(3, false);
    return b;
}

Bitmap boardArt() {
    Bitmap b(70, 18);
    b.rect(0, 0, 70, 18, 1);
    b.rect(2, 2, 66, 14, 2);
    b.outline(3, false);
    return b;
}

Bitmap chevArt() {
    Bitmap b(28, 16);
    b.poly({{2, 2}, {16, 8}, {2, 14}, {6, 8}}, 1);
    b.poly({{10, 2}, {24, 8}, {10, 14}, {14, 8}}, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 28);
    b.rect(6, 10, 4, 18, 1);
    b.ellipse(8, 7, 5, 5, 2);
    b.ellipse(8, 7, 2, 2.2f, 3);
    return b;
}

Bitmap pineArt() {
    Bitmap b(52, 70);
    b.poly({{26, 2}, {48, 32}, {4, 32}}, 1);
    b.poly({{26, 16}, {44, 46}, {8, 46}}, 2);
    b.poly({{26, 30}, {40, 58}, {12, 58}}, 3);
    b.rect(23, 54, 6, 14, 4);
    for (int i = 0; i < 8; i++) b.set(14 + int(hash2(i, 7) % 24), 20 + int(hash2(i, 3) % 28), 5);
    b.outline(4, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(48, 28);
    b.poly({{4, 24}, {10, 10}, {24, 4}, {40, 12}, {46, 24}}, 1);
    b.poly({{14, 22}, {18, 14}, {28, 12}, {36, 20}}, 2);
    b.outline(3, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(44, 36);
    b.poly({{4, 16}, {22, 4}, {40, 16}}, 3);
    b.rect(8, 16, 28, 16, 1);
    b.rect(18, 22, 8, 10, 2);
    b.rect(6, 30, 32, 3, 4);
    return b;
}

Bitmap snowArt() {
    Bitmap b(32, 32);
    b.rect(0, 0, 32, 32, 1);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            uint32_t h = hash2(x, y);
            if (h % 11 == 0) b.set(x, y, 2);
            else if (h % 17 == 0) b.set(x, y, 3);
        }
        b.set(0, y, 4);
    }
    for (int x = 0; x < 32; x++) b.set(x, 0, 5);
    return b;
}

Bitmap puffArt() {
    Bitmap b(20, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(7, 6, 3, 2, 2);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(48, 12);
    b.ellipse(24, 6, 20, 4, 1);
    return b;
}

Bitmap flakeArt() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(2, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 24);
    b.ellipse(24, 14, 18, 8, 1);
    b.ellipse(40, 12, 16, 9, 1);
    b.ellipse(32, 10, 12, 7, 2);
    return b;
}

Bitmap ridgeArt() {
    Bitmap b(96, 36);
    b.poly({{0, 34}, {18, 16}, {34, 28}, {52, 8}, {74, 24}, {96, 34}}, 1);
    b.poly({{20, 34}, {34, 22}, {48, 32}}, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 8, 8, 1);
    b.ellipse(14, 14, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        b.line(14 + std::cos(a) * 8, 14 + std::sin(a) * 8, 14 + std::cos(a) * 12, 14 + std::sin(a) * 12, 1, 1.6f);
    }
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
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(8, 15, 9));

    setPal(vdp, PAL_SLED, {0, gs::rgb4(5, 6, 8), gs::rgb4(11, 13, 14), gs::rgb4(11, 7, 3), gs::rgb4(6, 3, 2),
                           gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(12, 10, 6), gs::rgb4(2, 5, 7),
                           gs::rgb4(13, 9, 6), gs::rgb4(12, 2, 2), gs::rgb4(1, 1, 2), gs::rgb4(14, 12, 9),
                           gs::rgb4(13, 10, 3), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(10, 6, 3), gs::rgb4(13, 9, 5), gs::rgb4(6, 3, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(13, 14, 15), gs::rgb4(9, 11, 14), gs::rgb4(15, 15, 15), gs::rgb4(7, 9, 12),
                           gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 5, 3), gs::rgb4(2, 7, 4), gs::rgb4(3, 9, 5), gs::rgb4(5, 3, 2),
                           gs::rgb4(8, 10, 6)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(12, 10, 8), gs::rgb4(9, 7, 6), gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 2),
                            gs::rgb4(14, 12, 6), gs::rgb4(8, 3, 2), gs::rgb4(15, 15, 15), gs::rgb4(14, 11, 4),
                            gs::rgb4(6, 6, 6)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), gs::rgb4(15, 15, 15), gs::rgb4(14, 12, 6), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 10), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 1),
                           gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(7, 7, 8), gs::rgb4(5, 5, 7), gs::rgb4(3, 3, 4), gs::rgb4(9, 8, 7)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(4, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(2, 1, 1), gs::rgb4(14, 11, 4)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(6, 7, 9), gs::rgb4(12, 13, 14)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(6, 7, 10), gs::rgb4(8, 9, 12), gs::rgb4(10, 11, 13)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 4, 3), gs::rgb4(14, 12, 5), gs::rgb4(15, 15, 12), gs::rgb4(3, 2, 1),
                           gs::rgb4(13, 4, 2)});

    loadFont(vdp, art);
    for (int i = 0; i < kPoses; i++) art.sled[i] = drawSled(vdp, kPoseAtt[i]);
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.fascia = gs::uploadMipped(vdp, fasciaArt());
    art.riser = gs::uploadMipped(vdp, riserArt());
    art.trestle = gs::uploadMipped(vdp, trestleArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    gs::TextStyle word{2, 1, 0, 15, 1};
    art.word = gs::uploadMipped(vdp, gs::textBitmap("LEVEL", word));
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.snow = gs::uploadMipped(vdp, snowArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.flake = gs::uploadMipped(vdp, flakeArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.ridge = gs::uploadMipped(vdp, ridgeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
}

}  // namespace sledplat
