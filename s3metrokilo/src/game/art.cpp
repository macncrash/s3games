#include "game/art.h"

#include <cmath>
#include <string>

namespace metro {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void tileSolid(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

gs::Bitmap carBody(bool ducked) {
    gs::Bitmap b(ducked ? 70 : 78, ducked ? 28 : 46);
    int h = b.h;
    b.rect(4, 6, b.w - 10, h - 10, 2);
    b.rect(2, 8, b.w - 6, h - 14, 1);
    b.rect(6, h - 8, b.w - 14, 4, 4);
    if (!ducked) {
        b.rect(10, 10, 14, 10, 3);
        b.rect(28, 10, 14, 10, 3);
        b.rect(46, 10, 14, 10, 3);
        b.rect(62, 12, 6, 16, 6);
        b.rect(36, 2, 3, 6, 5);
        b.rect(30, 0, 16, 3, 5);
    } else {
        b.rect(12, 8, 16, 6, 3);
        b.rect(34, 8, 16, 6, 3);
        b.rect(56, 9, 6, 8, 6);
    }
    b.rect(b.w - 10, h / 2 - 2, 6, 4, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap spillCar() {
    gs::Bitmap b(80, 40);
    b.poly({{8, 18}, {70, 8}, {74, 28}, {12, 34}}, 2);
    b.poly({{14, 16}, {48, 10}, {50, 20}, {16, 24}}, 3);
    b.rect(18, 28, 40, 4, 4);
    b.outline(8, false);
    return b;
}

gs::Bitmap bogie(int phase) {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 14, 14, 1);
    b.ellipse(18, 18, 10, 10, 2);
    b.ellipse(18, 18, 3, 3, 4);
    float a = phase ? 0.4f : 0.f;
    b.line(18, 18, 18 + 11 * std::cos(a), 18 + 11 * std::sin(a), 3, 2);
    b.line(18, 18, 18 - 11 * std::sin(a), 18 + 11 * std::cos(a), 3, 2);
    b.rect(2, 16, 6, 4, 5);
    b.rect(28, 16, 6, 4, 5);
    return b;
}

gs::Bitmap hangWheel() {
    gs::Bitmap b(34, 40);
    b.rect(15, 0, 4, 12, 3);
    b.rect(8, 0, 18, 3, 2);
    b.ellipse(17, 26, 12, 12, 1);
    b.ellipse(17, 26, 7, 7, 4);
    b.ellipse(17, 26, 2, 2, 5);
    return b;
}

gs::Bitmap crewCar() {
    gs::Bitmap b(48, 28);
    b.rect(2, 6, 44, 16, 1);
    b.rect(6, 8, 8, 6, 3);
    b.rect(18, 8, 8, 6, 3);
    b.rect(30, 8, 8, 6, 3);
    b.rect(4, 20, 40, 3, 2);
    b.rect(40, 10, 5, 4, 4);
    return b;
}

gs::Bitmap clockFace(int hand) {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 10, 10, 2);
    float ang = hand * 0.785398f - 1.5708f;
    b.line(14, 14, 14 + 8 * std::cos(ang), 14 + 8 * std::sin(ang), 3, 2);
    b.ellipse(14, 14, 2, 2, 4);
    return b;
}

gs::Bitmap lampGlow() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 4, 1);
    b.rect(6, 2, 4, 6, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 16);
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
        art.font[c - 32] = t;
    }
}

void layTunnel(gs::VDP& vdp, Art& art) {
    auto put = [&](int index, const uint8_t px[64]) { vdp.loadTile(index, px); };
    uint8_t wall[64], glow[64], sleeper[64], rail[64], pipe[64];
    tileSolid(wall, 1);
    for (int x = 0; x < 8; x++) wall[0 * 8 + x] = 2;
    for (int y = 0; y < 8; y++) wall[y * 8 + 0] = 2;
    tileSolid(glow, 1);
    glow[2 * 8 + 3] = 3;
    glow[2 * 8 + 4] = 3;
    glow[3 * 8 + 2] = 4;
    glow[3 * 8 + 3] = 4;
    glow[3 * 8 + 4] = 4;
    glow[3 * 8 + 5] = 4;
    glow[4 * 8 + 3] = 3;
    glow[4 * 8 + 4] = 3;
    tileSolid(sleeper, 0);
    for (int x = 0; x < 8; x++) sleeper[5 * 8 + x] = 5;
    tileSolid(rail, 0);
    for (int x = 0; x < 8; x++) {
        rail[2 * 8 + x] = 6;
        rail[6 * 8 + x] = 6;
    }
    rail[3 * 8 + 1] = 5;
    rail[4 * 8 + 3] = 5;
    rail[5 * 8 + 6] = 5;
    tileSolid(pipe, 1);
    for (int x = 0; x < 8; x++) pipe[6 * 8 + x] = 7;
    pipe[5 * 8 + 2] = 7;
    pipe[4 * 8 + 2] = 7;
    put(art.wall, wall);
    put(art.glow, glow);
    put(art.sleeper, sleeper);
    put(art.rail, rail);
    put(art.pipe, pipe);

    vdp.B.resize(64, 32);
    vdp.A.resize(64, 32);
    vdp.B.clear();
    vdp.A.clear();
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            int cell = ((x / 6) % 3 == 0 && y == 6) ? art.glow : (y < 3 ? art.pipe : art.wall);
            vdp.B.set(x, y, gs::entry(cell, PAL_TUNNEL));
            if (y >= 22 && y <= 26) {
                int track = (y == 23 || y == 25) ? art.rail : art.sleeper;
                vdp.A.set(x, y, gs::entry(track, PAL_TUNNEL));
            }
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(12, 9, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 10, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(11, 12, 13), gs::rgb4(6, 8, 11), gs::rgb4(8, 13, 15), gs::rgb4(13, 3, 3),
                          gs::rgb4(4, 4, 5), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 10), gs::rgb4(13, 12, 8), gs::rgb4(3, 3, 4), gs::rgb4(7, 4, 2)});
    setPal(vdp, PAL_HANG, {0, gs::rgb4(10, 8, 4), gs::rgb4(6, 5, 3), gs::rgb4(12, 10, 6), gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(4, 10, 6), gs::rgb4(2, 6, 4), gs::rgb4(10, 14, 12), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(12, 9, 4), gs::rgb4(15, 14, 10), gs::rgb4(3, 2, 2), gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_TUNNEL, {0, gs::rgb4(2, 3, 5), gs::rgb4(3, 4, 6), gs::rgb4(14, 12, 5), gs::rgb4(15, 14, 8),
                             gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 9), gs::rgb4(4, 5, 6)});

    art.car[0] = gs::uploadMipped(vdp, carBody(false));
    art.car[1] = gs::uploadMipped(vdp, carBody(false));
    art.duck = gs::uploadMipped(vdp, carBody(true));
    art.spill = gs::uploadMipped(vdp, spillCar());
    art.wheel[0] = gs::uploadMipped(vdp, bogie(0));
    art.wheel[1] = gs::uploadMipped(vdp, bogie(1));
    art.hang = gs::uploadMipped(vdp, hangWheel());
    art.crew = gs::uploadMipped(vdp, crewCar());
    for (int i = 0; i < 8; i++) art.clock[i] = gs::uploadMipped(vdp, clockFace(i));
    art.lamp = gs::uploadMipped(vdp, lampGlow());

    gs::TextStyle title{3, 1, 0, 15, 1};
    gs::TextStyle sub{1, 2, 0, 0, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 METROKILO", title));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("THE CLOCK IS THE OTHER CREW", sub));
    loadFont(vdp, art);
    layTunnel(vdp, art);
}

}  // namespace metro
