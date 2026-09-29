#include "game/art.h"

#include <string>

namespace palbann {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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

void fillTile(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

gs::Bitmap bearer(int pose) {
    gs::Bitmap b(32, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(11, 2 + bob, 10, 8, 8);
    R(12, 4 + bob, 8, 5, 9);
    R(18, 5 + bob, 2, 2, 1);
    R(9, 10 + bob, 14, 15, 3);
    R(9, 10 + bob, 3, 15, 4);
    R(12, 16 + bob, 8, 2, 6);
    if (pose == 1) {
        R(8, 25, 5, 15, 3);
        R(17, 27, 5, 13, 3);
        R(7, 39, 7, 4, 5);
        R(16, 39, 7, 4, 5);
    } else if (pose == 2) {
        R(7, 27, 5, 13, 3);
        R(18, 25, 5, 15, 3);
        R(6, 39, 7, 4, 5);
        R(17, 39, 7, 4, 5);
    } else if (pose == 3) {
        R(11, 25, 4, 15, 3);
        R(17, 25, 4, 15, 3);
        R(10, 39, 6, 4, 5);
        R(17, 39, 6, 4, 5);
        R(20, 16, 10, 2, 7);
        R(28, 14, 3, 3, 10);
    } else {
        R(10, 25, 4, 15, 3);
        R(17, 25, 4, 15, 3);
        R(9, 39, 6, 4, 5);
        R(17, 39, 6, 4, 5);
        R(21, 18, 3, 14, 7);
    }
    if (pose != 3) R(22, 14 + bob, 2, 16, 7);
    return b;
}

gs::Bitmap foe(int pose) {
    gs::Bitmap b(28, 44);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose ? 1 : 0;
    R(9, 1 + bob, 10, 4, 6);
    R(10, 4 + bob, 8, 6, 8);
    R(8, 10 + bob, 12, 13, 3);
    R(8, 10 + bob, 12, 3, 5);
    if (pose) {
        R(6, 23, 5, 13, 3);
        R(16, 25, 5, 11, 3);
        R(5, 35, 7, 3, 2);
        R(15, 35, 7, 3, 2);
    } else {
        R(7, 23, 5, 13, 3);
        R(15, 23, 5, 13, 3);
        R(6, 35, 7, 3, 2);
        R(14, 35, 7, 3, 2);
    }
    R(19, 12 + bob, 2, 16, 7);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 5), gs::rgb4(4, 3, 2), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4), gs::rgb4(4, 3, 1),
                             gs::rgb4(8, 8, 6), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_CLOAK, {0, gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 3), gs::rgb4(3, 5, 9), gs::rgb4(2, 3, 6),
                            gs::rgb4(5, 4, 3), gs::rgb4(10, 8, 4), gs::rgb4(7, 7, 8), gs::rgb4(12, 9, 7),
                            gs::rgb4(14, 12, 9), gs::rgb4(13, 13, 12)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), gs::rgb4(15, 13, 6),
                            gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_RAID, {0, gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(10, 4, 2), gs::rgb4(6, 3, 2),
                           gs::rgb4(12, 6, 3), gs::rgb4(3, 2, 2), gs::rgb4(8, 7, 5), gs::rgb4(11, 8, 6),
                           gs::rgb4(13, 10, 8)});
    setPal(vdp, PAL_FIELD, {0, gs::rgb4(3, 6, 2), gs::rgb4(5, 8, 3), gs::rgb4(2, 4, 1), gs::rgb4(7, 9, 4),
                            gs::rgb4(4, 5, 2), gs::rgb4(8, 7, 3)});
    setPal(vdp, PAL_DITCH, {0, gs::rgb4(2, 3, 4), gs::rgb4(3, 5, 6), gs::rgb4(1, 2, 3), gs::rgb4(5, 7, 7),
                            gs::rgb4(8, 8, 6), gs::rgb4(4, 4, 3)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(6, 7, 8), gs::rgb4(10, 11, 12), gs::rgb4(4, 4, 5), gs::rgb4(13, 13, 12),
                           gs::rgb4(8, 6, 3)});

    gs::TileAlloc tiles(vdp, 1);
    uint8_t gpx[64];
    fillTile(gpx, 2);
    for (int i = 0; i < 8; i++) {
        gpx[i] = 1;
        gpx[56 + i] = 3;
        if (i % 3 == 0) gpx[16 + i] = 4;
    }
    art.grass = tiles.alloc(1);
    vdp.loadTile(art.grass, gpx);
    fillTile(gpx, 3);
    for (int i = 0; i < 64; i += 5) gpx[i] = 1;
    art.mud = tiles.alloc(1);
    vdp.loadTile(art.mud, gpx);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, bearer(0));
    art.walkA = gs::uploadMipped(vdp, bearer(1));
    art.walkB = gs::uploadMipped(vdp, bearer(2));
    art.thrust = gs::uploadMipped(vdp, bearer(3));
    art.raider[0] = gs::uploadMipped(vdp, foe(0));
    art.raider[1] = gs::uploadMipped(vdp, foe(1));

    {
        gs::Bitmap b(22, 28);
        b.rect(2, 2, 16, 22, 2);
        b.rect(4, 4, 12, 18, 1);
        b.poly({{4, 6}, {16, 13}, {4, 20}}, 4);
        b.rect(0, 8, 3, 10, 3);
        art.cloth = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(6, 40);
        b.rect(2, 0, 2, 40, 1);
        b.rect(1, 36, 4, 4, 2);
        art.pole = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 64);
        b.rect(4, 4, 6, 58, 2);
        b.rect(3, 8, 2, 50, 1);
        b.poly({{3, 4}, {7, 0}, {11, 4}}, 4);
        b.rect(2, 58, 10, 6, 5);
        art.stake = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(36, 48);
        b.rect(2, 8, 6, 40, 2);
        b.rect(28, 8, 6, 40, 2);
        b.rect(6, 10, 24, 6, 3);
        b.rect(8, 18, 20, 4, 1);
        b.rect(4, 0, 4, 10, 4);
        b.rect(28, 0, 4, 10, 4);
        art.gate = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(10, 18);
        b.line(2, 18, 4, 2, 4, 1.4f);
        b.line(6, 18, 5, 4, 2, 1.2f);
        b.line(8, 16, 7, 6, 1, 1.0f);
        art.reed = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(24, 8);
        b.ellipse(12, 4, 11, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }

    {
        gs::Bitmap b(48, 16);
        b.rect(0, 0, 48, 16, 1);
        b.rect(0, 0, 48, 3, 3);
        for (int x = 4; x < 48; x += 9) b.rect(float(x), 6, 4, 3, 5);
        art.mudpad = gs::uploadMipped(vdp, b);
    }

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int row = 22; row < 28; row++)
        for (int cx = 0; cx < 64; cx++) vdp.B.set(cx, row, gs::entry(art.grass, PAL_FIELD));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float k = y / float(gs::SCREEN_H - 1);
        int r = int(6 + (2 - 6) * k);
        int g = int(8 + (5 - 8) * k);
        int b = int(12 + (4 - 12) * k);
        if (y > 150) {
            r = 3;
            g = 5;
            b = 2;
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(6, 7, 8));
}

}  // namespace palbann
