#include "game/art.h"

namespace culvert {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, c[i]);
}

gs::Bitmap sentryArt(bool duck) {
    gs::Bitmap b(40, duck ? 28 : 48);
    int y0 = duck ? 0 : 8;
    b.ellipse(20, y0 + 6, 6, 6, 3);
    b.rect(16, y0 + 2, 8, 4, 4);
    b.rect(12, y0 + 12, 16, duck ? 12 : 18, 5);
    b.rect(14, y0 + 14, 6, 8, 8);
    b.rect(26, y0 + 13, 10, 3, 2);
    b.rect(34, y0 + 12, 4, 3, 6);
    if (!duck) {
        b.rect(14, 32, 5, 14, 7);
        b.rect(22, 32, 5, 14, 7);
        b.rect(13, 44, 7, 3, 1);
        b.rect(21, 44, 7, 3, 1);
    } else {
        b.rect(8, 18, 10, 6, 7);
    }
    b.rect(18, y0 + 6, 2, 2, 1);
    return b;
}

gs::Bitmap foeArt(int kind) {
    gs::Bitmap b(36, 46);
    b.ellipse(18, 8, 6, 6, 4);
    b.rect(11, 14, 14, 16, kind == 2 ? 6 : 3);
    b.rect(8, 16, 8, 3, 2);
    b.rect(20, 18, 12, 3, 8);
    b.rect(12, 30, 5, 14, 5);
    b.rect(20, 30, 5, 14, 5);
    if (kind == 1) {
        b.rect(6, 20, 8, 3, 7);
        b.ellipse(8, 28, 4, 4, 9);
    } else if (kind == 2) {
        b.ellipse(28, 22, 5, 5, 10);
        b.rect(26, 12, 3, 8, 1);
    } else {
        b.rect(24, 16, 10, 2, 1);
    }
    b.rect(14, 8, 3, 2, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(13, 2, 2, 6, 2);
    b.poly({{6, 10}, {22, 10}, {24, 22}, {4, 22}}, 3);
    b.ellipse(14, 22, 10, 4, 4);
    b.ellipse(14, 16, 3, 3, 5);
    b.rect(12, 24, 4, 6, 2);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(16, 6);
    b.rect(0, 2, 12, 2, 3);
    b.poly({{10, 0}, {16, 3}, {10, 6}}, 5);
    return b;
}

gs::Bitmap shotArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 3);
    b.ellipse(4, 4, 1, 1, 5);
    return b;
}

gs::Bitmap grenadeArt() {
    gs::Bitmap b(12, 14);
    b.ellipse(6, 8, 5, 5, 3);
    b.rect(5, 2, 2, 5, 2);
    b.rect(4, 1, 4, 2, 5);
    return b;
}

gs::Bitmap archSide(bool right) {
    gs::Bitmap b(78, 200);
    b.rect(0, 0, 78, 200, 2);
    for (int y = 0; y < 200; y++) {
        int inset = 8 + (y * y) / 1800;
        if (!right) b.rect(78 - inset, y, inset, 1, 0);
        else b.rect(0, y, inset, 1, 0);
        if (y % 18 < 2) {
            int x0 = right ? inset : 0;
            int x1 = right ? 78 : 78 - inset;
            for (int x = x0; x < x1; x++)
                if (b.get(x, y)) b.set(x, y, 4);
        }
    }
    b.rect(right ? 60 : 4, 40, 12, 28, 6);
    b.rect(right ? 63 : 7, 44, 6, 8, 8);
    return b;
}

gs::Bitmap lintelArt() {
    gs::Bitmap b(320, 36);
    b.rect(0, 0, 320, 36, 2);
    b.rect(0, 30, 320, 4, 4);
    for (int x = 16; x < 320; x += 28) b.rect(x, 8, 10, 6, 5);
    b.rect(148, 6, 24, 16, 3);
    b.rect(154, 10, 12, 8, 8);
    return b;
}

gs::Bitmap lipArt() {
    gs::Bitmap b(220, 16);
    b.poly({{0, 0}, {220, 0}, {200, 16}, {20, 16}}, 3);
    b.rect(30, 6, 160, 3, 5);
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

void buildArt(gs::VDP& vdp, Art& a) {
    const uint16_t hud[16] = {
        gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 13), gs::rgb4(8, 8, 7), gs::rgb4(15, 12, 4),
        gs::rgb4(4, 4, 4), gs::rgb4(12, 4, 3), gs::rgb4(6, 10, 6), gs::rgb4(3, 3, 3),
        gs::rgb4(15, 15, 15), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0),
        gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0), gs::rgb4(2, 2, 2)};
    const uint16_t stone[16] = {
        0, gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(3, 3, 4),
        gs::rgb4(5, 5, 4), gs::rgb4(12, 10, 4), gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 8),
        gs::rgb4(8, 8, 8), 0, 0, 0, 0, 0, 0};
    const uint16_t raid[16] = {
        0, gs::rgb4(2, 2, 2), gs::rgb4(8, 3, 2), gs::rgb4(10, 6, 4), gs::rgb4(5, 4, 3),
        gs::rgb4(4, 4, 5), gs::rgb4(7, 8, 5), gs::rgb4(12, 8, 3), gs::rgb4(3, 3, 3),
        gs::rgb4(6, 7, 4), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0};
    const uint16_t sentry[16] = {
        0, gs::rgb4(2, 2, 2), gs::rgb4(6, 7, 4), gs::rgb4(12, 9, 6), gs::rgb4(4, 5, 3),
        gs::rgb4(5, 7, 4), gs::rgb4(14, 12, 5), gs::rgb4(3, 4, 5), gs::rgb4(8, 10, 6),
        0, 0, 0, 0, 0, 0, 0};
    const uint16_t bell[16] = {
        0, gs::rgb4(6, 4, 1), gs::rgb4(10, 7, 2), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 6),
        gs::rgb4(8, 8, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t bolt[16] = {
        0, gs::rgb4(8, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(15, 14, 6), gs::rgb4(10, 8, 3),
        gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t lamp[16] = {
        0, gs::rgb4(8, 6, 2), gs::rgb4(14, 10, 3), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t water[16] = {
        0, gs::rgb4(2, 5, 7), gs::rgb4(3, 7, 8), gs::rgb4(6, 10, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t road[16] = {
        0,
        gs::rgb4(4, 6, 3), gs::rgb4(3, 5, 2), gs::rgb4(6, 7, 4),
        gs::rgb4(5, 5, 3), gs::rgb4(3, 4, 2),
        gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5),
        gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(7, 7, 6),
        gs::rgb4(2, 4, 6), gs::rgb4(3, 6, 7), gs::rgb4(5, 8, 8),
        gs::rgb4(12, 11, 6), gs::rgb4(9, 9, 8)};
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_STONE, stone);
    pal(vdp, PAL_RAID, raid);
    pal(vdp, PAL_SENTRY, sentry);
    pal(vdp, PAL_BELL, bell);
    pal(vdp, PAL_BOLT, bolt);
    pal(vdp, PAL_LAMP, lamp);
    pal(vdp, PAL_WATER, water);
    pal(vdp, PAL_ROAD, road);
    vdp.setFogColor(gs::rgb4(3, 4, 6));

    a.sentry = gs::uploadMipped(vdp, sentryArt(false));
    a.sentryDuck = gs::uploadMipped(vdp, sentryArt(true));
    a.raider = gs::uploadMipped(vdp, foeArt(0));
    a.runner = gs::uploadMipped(vdp, foeArt(1));
    a.grenadier = gs::uploadMipped(vdp, foeArt(2));
    a.bell = gs::uploadMipped(vdp, bellArt());
    a.bolt = gs::uploadMipped(vdp, boltArt());
    a.shot = gs::uploadMipped(vdp, shotArt());
    a.grenade = gs::uploadMipped(vdp, grenadeArt());
    a.archL = gs::uploadMipped(vdp, archSide(false));
    a.archR = gs::uploadMipped(vdp, archSide(true));
    a.lintel = gs::uploadMipped(vdp, lintelArt());
    a.lip = gs::uploadMipped(vdp, lipArt());
    loadFont(vdp, a);
}

}  // namespace culvert
