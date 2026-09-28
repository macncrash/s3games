#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace tower {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap sentry(int step) {
    gs::Bitmap b(28, 56);
    b.ellipse(14, 8, 6, 6, 4);
    b.ellipse(14, 5, 7, 3, 5);
    b.rect(9, 13, 10, 18, 2);
    b.rect(11, 15, 3, 12, 1);
    b.rect(8, 18, 4, 3, 6);
    b.line(8, 16, 3, 28, 6, 2.f);
    b.line(20, 16, 25, 26, 3, 2.f);
    if (step == 0) {
        b.rect(9, 30, 5, 18, 3);
        b.rect(16, 30, 5, 14, 7);
        b.rect(8, 46, 7, 3, 8);
        b.rect(15, 42, 7, 3, 8);
    } else {
        b.rect(9, 30, 5, 14, 7);
        b.rect(16, 30, 5, 18, 3);
        b.rect(8, 42, 7, 3, 8);
        b.rect(15, 46, 7, 3, 8);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap downed() {
    gs::Bitmap b(64, 22);
    b.ellipse(10, 11, 6, 5, 4);
    b.ellipse(10, 8, 7, 3, 5);
    b.poly({{16, 6}, {54, 8}, {58, 16}, {16, 16}}, 2);
    b.rect(48, 12, 10, 4, 8);
    b.outline(8, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(72, 140);
    b.poly({{16, 8}, {56, 8}, {64, 136}, {8, 136}}, 2);
    b.poly({{16, 8}, {36, 8}, {36, 136}, {10, 136}}, 1);
    for (int y = 16; y < 132; y += 10) {
        b.rect(12, y, 48, 2, 3);
        int off = ((y / 10) & 1) ? 8 : 0;
        for (int x = 14 + off; x < 58; x += 16) b.rect(x, y + 2, 2, 7, 4);
    }
    b.rect(30, 48, 12, 18, 5);
    b.rect(32, 50, 8, 8, 6);
    b.rect(32, 60, 8, 4, 6);
    b.rect(28, 96, 16, 28, 5);
    b.rect(31, 100, 10, 16, 6);
    b.rect(14, 0, 44, 10, 3);
    for (int x = 14; x < 58; x += 10) b.rect(x, 0, 6, 8, 1);
    b.outline(7, false);
    return b;
}

gs::Bitmap merlonArt() {
    gs::Bitmap b(18, 16);
    b.rect(1, 4, 16, 12, 2);
    b.rect(2, 0, 6, 8, 1);
    b.rect(10, 0, 6, 8, 1);
    b.rect(1, 14, 16, 2, 3);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(22, 36);
    b.rect(2, 6, 18, 30, 2);
    b.poly({{2, 8}, {11, 0}, {20, 8}}, 1);
    b.rect(6, 12, 10, 18, 3);
    b.ellipse(15, 22, 1, 1, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(28, 20);
    b.rect(2, 0, 2, 20, 1);
    b.poly({{4, 2}, {26, 6}, {22, 12}, {4, 10}}, 2);
    b.poly({{6, 4}, {16, 6}, {14, 10}, {6, 8}}, 3);
    return b;
}

gs::Bitmap stairArt() {
    gs::Bitmap b(20, 48);
    for (int i = 0; i < 6; i++) b.rect(2, 4 + i * 7, 16 - i, 5, (i & 1) ? 2 : 1);
    b.outline(3, false);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(22, 22);
    for (int i = 0; i < 20; i++) {
        if ((i % 5) < 2) continue;
        float a = i * TAU / 20.f;
        b.set(int(std::lround(11 + std::cos(a) * 8)), int(std::lround(11 + std::sin(a) * 8)), 1);
    }
    b.rect(10, 2, 2, 4, 2);
    b.rect(10, 16, 2, 4, 2);
    b.rect(2, 10, 4, 2, 2);
    b.rect(16, 10, 4, 2, 2);
    b.set(11, 11, 1);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(16, 48);
    b.rect(6, 0, 4, 28, 1);
    b.rect(7, 2, 2, 22, 2);
    b.rect(4, 26, 8, 6, 3);
    b.rect(5, 32, 5, 12, 1);
    b.rect(3, 42, 9, 5, 4);
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
    setPal(vdp, PAL_STONE, {0, gs::rgb4(7, 7, 8), gs::rgb4(10, 10, 11), gs::rgb4(4, 4, 5), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 3),
                            gs::rgb4(13, 12, 8), gs::rgb4(3, 4, 6), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(5, 6, 8), gs::rgb4(8, 8, 6), gs::rgb4(3, 3, 4), gs::rgb4(12, 10, 8), gs::rgb4(2, 2, 3),
                           gs::rgb4(9, 7, 4), gs::rgb4(4, 3, 3), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 9), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(12, 15, 9), gs::rgb4(4, 11, 6), gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(14, 10, 4), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(6, 5, 4), gs::rgb4(12, 3, 2), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0, gs::rgb4(3, 4, 2), gs::rgb4(2, 3, 2), gs::rgb4(4, 5, 3), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 2),
        gs::rgb4(6, 5, 3), gs::rgb4(4, 4, 3), gs::rgb4(7, 6, 4), gs::rgb4(5, 5, 3), gs::rgb4(8, 7, 4),
        gs::rgb4(2, 2, 2), gs::rgb4(1, 2, 1), gs::rgb4(3, 3, 2), gs::rgb4(9, 8, 5), gs::rgb4(6, 6, 5),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.step[0] = gs::uploadMipped(vdp, sentry(0));
    art.step[1] = gs::uploadMipped(vdp, sentry(1));
    art.down = gs::uploadMipped(vdp, downed());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.merlon = gs::uploadMipped(vdp, merlonArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.stair = gs::uploadMipped(vdp, stairArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(3, 4, 6));
}

}  // namespace tower
