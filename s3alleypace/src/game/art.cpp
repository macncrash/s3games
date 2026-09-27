#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace alley {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap courier(int step) {
    gs::Bitmap b(40, 78);
    b.ellipse(20, 12, 8, 9, 4);
    b.ellipse(20, 8, 9, 4, 5);
    b.rect(14, 10, 12, 3, 5);
    b.set(17, 13, 8);
    b.set(23, 13, 8);
    b.rect(17, 17, 6, 2, 9);
    b.poly({{10, 22}, {30, 22}, {34, 50}, {6, 50}}, 2);
    b.poly({{10, 22}, {20, 22}, {18, 50}, {6, 50}}, 1);
    b.rect(18, 24, 4, 22, 3);
    b.line(12, 26, 6, 40, 6, 3.f);
    b.line(28, 26, 34, 38, 6, 3.f);
    b.ellipse(6, 42, 4, 4, 4);
    if (step == 0) {
        b.rect(12, 48, 6, 20, 3);
        b.rect(22, 48, 6, 16, 7);
        b.rect(10, 66, 10, 4, 8);
        b.rect(21, 62, 8, 4, 8);
    } else {
        b.rect(12, 48, 6, 16, 7);
        b.rect(22, 48, 6, 20, 3);
        b.rect(11, 62, 8, 4, 8);
        b.rect(20, 66, 10, 4, 8);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap downed() {
    gs::Bitmap b(86, 32);
    b.ellipse(14, 16, 7, 6, 4);
    b.ellipse(14, 11, 8, 4, 5);
    b.poly({{22, 10}, {72, 14}, {76, 24}, {20, 22}}, 2);
    b.poly({{22, 10}, {44, 12}, {46, 22}, {20, 20}}, 1);
    b.rect(62, 18, 12, 5, 8);
    b.outline(8, false);
    return b;
}

gs::Bitmap brickWall() {
    gs::Bitmap b(36, 96);
    b.rect(2, 0, 32, 96, 2);
    for (int y = 0; y < 96; y += 8) {
        int off = ((y / 8) & 1) ? 8 : 0;
        b.rect(2, y, 32, 2, 1);
        for (int x = 2 + off; x < 34; x += 16) b.rect(x, y + 2, 2, 6, 3);
    }
    b.rect(8, 28, 10, 16, 4);
    b.rect(10, 30, 6, 12, 5);
    b.rect(22, 60, 8, 14, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(28, 48);
    b.rect(2, 4, 24, 44, 2);
    b.rect(4, 6, 20, 40, 1);
    b.rect(6, 8, 7, 12, 3);
    b.rect(15, 8, 7, 12, 4);
    b.rect(6, 24, 16, 16, 3);
    b.ellipse(20, 28, 2, 2, 5);
    b.rect(0, 0, 28, 6, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(28, 26);
    b.rect(2, 4, 24, 20, 2);
    b.rect(2, 4, 24, 4, 1);
    b.line(4, 8, 24, 22, 3, 2.f);
    b.line(24, 8, 4, 22, 3, 2.f);
    b.rect(12, 12, 4, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap pipeArt() {
    gs::Bitmap b(12, 72);
    b.rect(3, 0, 6, 72, 1);
    b.rect(4, 0, 2, 72, 2);
    for (int y = 8; y < 70; y += 16) b.rect(2, y, 8, 3, 3);
    b.ellipse(6, 70, 5, 3, 1);
    return b;
}

gs::Bitmap washArt() {
    gs::Bitmap b(40, 28);
    b.line(2, 2, 38, 4, 1, 1.f);
    b.poly({{6, 4}, {16, 4}, {18, 24}, {4, 22}}, 2);
    b.poly({{18, 5}, {30, 6}, {28, 22}, {16, 20}}, 3);
    b.rect(8, 8, 4, 8, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 22);
    b.rect(7, 0, 2, 6, 1);
    b.poly({{3, 8}, {13, 8}, {11, 16}, {5, 16}}, 2);
    b.ellipse(8, 18, 5, 3, 3);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(22, 22);
    for (int i = 0; i < 20; i++) {
        if ((i % 5) < 2) continue;
        float a = i * TAU / 20.f;
        int x = int(std::lround(11 + std::cos(a) * 8));
        int y = int(std::lround(11 + std::sin(a) * 8));
        b.set(x, y, 1);
    }
    b.rect(10, 2, 2, 4, 2);
    b.rect(10, 16, 2, 4, 2);
    b.rect(2, 10, 4, 2, 2);
    b.rect(16, 10, 4, 2, 2);
    b.set(11, 11, 1);
    return b;
}

gs::Bitmap pistolArt() {
    gs::Bitmap b(18, 52);
    b.rect(7, 0, 4, 30, 1);
    b.rect(8, 2, 2, 26, 2);
    b.rect(5, 28, 8, 8, 3);
    b.rect(6, 36, 6, 12, 1);
    b.rect(4, 46, 10, 5, 4);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap flareArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 2);
    b.ellipse(10, 10, 4, 4, 1);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(6, 6, 5, 3, 1);
    b.ellipse(13, 5, 4, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(9, 8, 8), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 10), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 10, 8), gs::rgb4(5, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 7), gs::rgb4(13, 15, 11), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(8, 4, 3), gs::rgb4(12, 6, 4), gs::rgb4(5, 3, 3), gs::rgb4(3, 5, 6), gs::rgb4(10, 12, 13),
                            gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(9, 7, 4), gs::rgb4(6, 4, 3), gs::rgb4(3, 2, 2), gs::rgb4(13, 10, 8), gs::rgb4(2, 2, 3),
                           gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 9), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(12, 15, 9), gs::rgb4(4, 11, 6), gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(14, 10, 4), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(6, 6, 8), gs::rgb4(14, 12, 6), gs::rgb4(10, 11, 12), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(4, 3, 3),
        gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 2),
        gs::rgb4(6, 5, 4), gs::rgb4(4, 4, 3),
        gs::rgb4(7, 6, 5), gs::rgb4(5, 4, 4), gs::rgb4(8, 7, 6),
        gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4),
        gs::rgb4(9, 7, 4), gs::rgb4(6, 6, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.step[0] = gs::uploadMipped(vdp, courier(0));
    art.step[1] = gs::uploadMipped(vdp, courier(1));
    art.down = gs::uploadMipped(vdp, downed());
    art.wall = gs::uploadMipped(vdp, brickWall());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.pipe = gs::uploadMipped(vdp, pipeArt());
    art.wash = gs::uploadMipped(vdp, washArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pistol = gs::uploadMipped(vdp, pistolArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(2, 2, 3));
}

}  // namespace alley
