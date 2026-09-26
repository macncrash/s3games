#include "game/art.h"

#include <string>

namespace breach {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

int makeTile(gs::TileAlloc& al, gs::VDP& vdp, std::initializer_list<const char*> rows) {
    uint8_t px[64] = {};
    int y = 0;
    for (const char* row : rows) {
        if (y >= 8) break;
        for (int x = 0; x < 8 && row[x]; x++) {
            char c = row[x];
            int v = 0;
            if (c >= '0' && c <= '9') v = c - '0';
            else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
            px[y * 8 + x] = uint8_t(v);
        }
        y++;
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
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

// 1 ink, 2 coat, 3 hi, 4 skin, 5 hair, 6 boot, 7 shirt, 8 belt, 9 eye, 10 gold
gs::Bitmap thief(int pose) {
    gs::Bitmap b(28, 42);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    if (pose == 4) {
        R(3, 20, 20, 11, 2);
        R(3, 20, 6, 11, 3);
        R(8, 22, 8, 5, 7);
        R(9, 14, 10, 8, 4);
        R(8, 11, 13, 5, 5);
        R(16, 16, 2, 2, 9);
        R(17, 17, 1, 1, 1);
        R(1, 24, 8, 5, 2);
        R(16, 28, 9, 4, 6);
        R(6, 29, 8, 4, 6);
        b.outline(1, false);
        return b;
    }
    int leg = pose == 1 ? 3 : pose == 2 ? -2 : 0;
    int arm = pose == 1 ? -2 : pose == 2 ? 3 : pose == 3 ? 2 : 0;
    int lift = pose == 3 ? -3 : 0;
    R(9, 27 + lift, 4, 9, 2);
    R(15, 27 + lift, 4, 9 + (leg > 0 ? 0 : 0), 2);
    if (pose == 1) {
        R(8, 30, 4, 7, 2);
        R(16, 28, 4, 8, 2);
        R(7, 36, 6, 4, 6);
        R(16, 35, 6, 4, 6);
    } else if (pose == 2) {
        R(10, 28, 4, 8, 2);
        R(15, 31, 4, 6, 2);
        R(9, 35, 6, 4, 6);
        R(15, 36, 6, 4, 6);
    } else if (pose == 3) {
        R(8, 28, 4, 7, 2);
        R(16, 26, 4, 8, 2);
        R(7, 34, 6, 3, 6);
        R(16, 33, 6, 3, 6);
    } else {
        R(9, 28, 4, 8, 2);
        R(15, 28, 4, 8, 2);
        R(8, 35, 6, 4, 6);
        R(15, 35, 6, 4, 6);
    }
    R(8, 15 + lift, 12, 14, 2);
    R(8, 15 + lift, 4, 14, 3);
    R(11, 17 + lift, 6, 6, 7);
    R(10, 26 + lift, 8, 2, 8);
    R(13, 26 + lift, 2, 2, 10);
    R(17 + arm, 17 + lift, 4, 10, 2);
    R(17 + arm, 25 + lift, 4, 3, 4);
    R(10, 7 + lift, 9, 8, 4);
    R(9, 5 + lift, 12, 5, 5);
    R(8, 9 + lift, 3, 4, 5);
    R(16, 10 + lift, 2, 2, 9);
    R(17, 11 + lift, 1, 1, 1);
    (void)leg;
    b.outline(1, false);
    return b;
}

// 1 ink, 2 steel, 3 steel hi, 4 tabard, 5 tabard dark, 6 skin, 7 pike, 8 boot, 9 plume
gs::Bitmap sentry(int pose) {
    gs::Bitmap b(30, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int s = pose == 1 ? 2 : pose == 2 ? -2 : 0;
    R(14, 4, 2, 30, 7);
    R(11, 2, 8, 3, 7);
    R(11, 5, 4, 5, 2);
    R(12, 0, 3, 5, 9);
    R(7, 8, 12, 7, 2);
    R(7, 8, 12, 3, 3);
    R(9, 13, 8, 3, 6);
    R(15, 14, 2, 2, 1);
    R(6, 16, 14, 15, 4);
    R(6, 16, 4, 15, 5);
    R(9, 18, 7, 8, 2);
    R(7, 31, 5, 9, 5);
    R(15, 31, 5, 9, 5);
    R(6 + s, 39, 6, 4, 8);
    R(15 - s, 39, 6, 4, 8);
    b.outline(1, false);
    return b;
}

// 1 red, 2 dark, 3 gold, 4 cream, 5 navy, 6 pole, 7 fringe
gs::Bitmap bannerCloth(int flutter) {
    gs::Bitmap b(22, 58);
    int sway = flutter ? 1 : 0;
    b.rect(2, 1, 3, 56, 6);
    b.rect(2, 1, 1, 56, 3);
    b.ellipse(3.5f, 3, 3.2f, 3.2f, 3);
    b.rect(6, 8, 14, 38, 1);
    b.rect(6, 8, 14, 5, 2);
    b.rect(6, 20, 3, 26, 2);
    b.rect(7 + sway, 40, 12, 6, 2);
    b.rect(11, 18, 4, 12, 3);
    b.rect(8, 22, 10, 4, 3);
    b.set(12, 23, 4);
    b.set(13, 24, 4);
    for (int i = 0; i < 6; i++) b.rect(7 + i * 2 + sway, 46, 1, 5, 7);
    b.outline(5, false);
    return b;
}

gs::Bitmap standBase() {
    gs::Bitmap b(28, 16);
    b.rect(12, 0, 4, 10, 6);
    b.rect(12, 0, 1, 10, 3);
    b.rect(4, 9, 20, 3, 6);
    b.rect(2, 12, 24, 3, 3);
    b.rect(6, 12, 16, 1, 4);
    return b;
}

gs::Bitmap columnArt() {
    gs::Bitmap b(20, 96);
    b.rect(5, 10, 10, 74, 2);
    b.rect(5, 10, 3, 74, 4);
    b.rect(12, 10, 3, 74, 3);
    b.rect(9, 14, 1, 66, 3);
    b.rect(2, 4, 16, 7, 3);
    b.rect(1, 2, 18, 3, 4);
    b.rect(3, 82, 14, 6, 3);
    b.rect(1, 88, 18, 6, 1);
    b.rect(0, 92, 20, 4, 3);
    return b;
}

gs::Bitmap windowArt() {
    gs::Bitmap b(30, 44);
    b.rect(2, 8, 26, 34, 5);
    b.rect(4, 6, 22, 6, 6);
    b.ellipse(15, 16, 11, 10, 5);
    b.rect(6, 16, 18, 22, 1);
    b.rect(7, 18, 16, 18, 2);
    b.rect(14, 8, 2, 28, 6);
    b.rect(6, 24, 18, 2, 5);
    b.ellipse(18, 22, 3, 3, 3);
    b.rect(2, 36, 26, 5, 6);
    return b;
}

gs::Bitmap sconceArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 0, 2, 6, 4);
    b.rect(2, 5, 10, 3, 2);
    b.rect(4, 8, 6, 7, 3);
    b.rect(5, 9, 4, 5, 1);
    b.rect(3, 15, 8, 2, 2);
    return b;
}

gs::Bitmap flameArt(int hot) {
    gs::Bitmap b(10, 14);
    b.ellipse(5, 8, 3.2f, 4.5f, 2);
    b.ellipse(5, 9, 2.1f, 3.2f, hot ? 4 : 3);
    b.rect(4, 2, 2, 5, 3);
    b.set(5, 3, 4);
    if (hot) b.set(6, 6, 5);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 28);
    b.rect(2, 4, 32, 22, 2);
    b.rect(2, 4, 32, 4, 3);
    b.rect(4, 8, 28, 14, 1);
    b.rect(6, 10, 24, 2, 3);
    b.rect(6, 16, 24, 2, 3);
    b.rect(16, 8, 3, 14, 3);
    b.rect(2, 22, 32, 4, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(40, 18);
    b.rect(0, 3, 40, 12, 2);
    b.rect(0, 3, 40, 3, 3);
    b.rect(0, 12, 40, 3, 1);
    for (int x = 4; x < 40; x += 8) b.rect(x, 6, 2, 6, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap chainArt() {
    gs::Bitmap b(6, 16);
    for (int y = 0; y < 16; y += 4) {
        b.rect(1, y, 4, 3, 4);
        b.rect(2, y, 2, 2, 5);
    }
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(48, 96);
    b.rect(0, 8, 10, 84, 2);
    b.rect(38, 10, 10, 82, 2);
    b.rect(0, 8, 10, 8, 4);
    b.rect(38, 10, 10, 6, 4);
    b.rect(8, 18, 8, 70, 3);
    b.rect(30, 24, 10, 64, 3);
    b.rect(14, 30, 18, 48, 6);
    b.rect(18, 36, 8, 16, 7);
    b.rect(16, 70, 14, 10, 8);
    b.rect(4, 4, 6, 16, 5);
    b.rect(40, 16, 5, 10, 5);
    b.rect(0, 88, 48, 8, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap grateArt() {
    gs::Bitmap b(28, 80);
    b.rect(2, 0, 4, 80, 4);
    b.rect(22, 0, 4, 80, 4);
    b.rect(2, 0, 24, 4, 5);
    for (int y = 6; y < 78; y += 8) {
        b.rect(2, y, 24, 2, 4);
        b.rect(12, y, 2, 8, 5);
    }
    return b;
}

gs::Bitmap tapestryArt() {
    gs::Bitmap b(34, 26);
    b.rect(2, 2, 30, 18, 2);
    b.rect(2, 2, 30, 4, 1);
    b.rect(6, 8, 8, 8, 5);
    b.rect(16, 8, 10, 8, 3);
    b.rect(4, 20, 26, 3, 7);
    for (int x = 4; x < 30; x += 4) b.rect(x, 22, 2, 3, 3);
    b.rect(16, 0, 2, 4, 6);
    return b;
}

gs::Bitmap daisArt() {
    gs::Bitmap b(64, 22);
    b.rect(8, 8, 48, 8, 2);
    b.rect(4, 14, 56, 6, 3);
    b.rect(0, 18, 64, 4, 1);
    b.rect(10, 8, 44, 2, 4);
    b.rect(20, 4, 24, 5, 3);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    b.rect(3, 0, 1, 7, 3);
    b.rect(0, 3, 7, 1, 3);
    b.set(3, 3, 4);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(10, 6);
    b.ellipse(5, 3, 4, 2, 1);
    b.ellipse(3, 3, 1.4f, 1, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(22, 8);
    b.ellipse(11, 4, 9, 2.4f, 1);
    return b;
}

void paintHall(gs::VDP& vdp, gs::TileAlloc& tiles) {
    const int ceilT = makeTile(tiles, vdp, {"11111111", "23333332", "21111112", "23333332", "21111112", "23333332", "21111112", "11111111"});
    const int beamT = makeTile(tiles, vdp, {"11111111", "13333331", "13111131", "13333331", "11111111", "33333333", "31111113", "33333333"});
    const int stoneA = makeTile(tiles, vdp, {"11111111", "14444443", "14222223", "14222223", "14222223", "14222223", "14333333", "11111111"});
    const int stoneB = makeTile(tiles, vdp, {"11111111", "14333334", "14222623", "14222223", "14222223", "14232223", "14444443", "11111111"});
    const int stoneC = makeTile(tiles, vdp, {"11111111", "14444443", "14222223", "14222523", "14222223", "14222223", "14333334", "11111111"});
    const int wain = makeTile(tiles, vdp, {"44444444", "21111112", "23333332", "21111112", "23333332", "21111112", "44444444", "11111111"});
    const int floorT = makeTile(tiles, vdp, {"55555555", "56666665", "56666665", "56555565", "56666665", "56666665", "55555555", "11111111"});
    const int carpet = makeTile(tiles, vdp, {"33333333", "21111112", "21111112", "21333312", "21111112", "21111112", "21111112", "33333333"});
    const int carpetEdge = makeTile(tiles, vdp, {"44444444", "43333334", "42111124", "42111124", "42111124", "43333334", "44444444", "55555555"});

    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = 0;
            int pal = PAL_STONE;
            if (cy <= 1) {
                tile = (cy == 0 && (cx % 5) == 0) ? beamT : ceilT;
                pal = PAL_WOOD;
            } else if (cy <= 19) {
                int kind = (cx / 2 + cy / 2) % 3;
                tile = kind == 0 ? stoneA : kind == 1 ? stoneB : stoneC;
                pal = PAL_STONE;
            } else if (cy <= 21) {
                tile = wain;
                pal = PAL_WOOD;
            } else if (cy == 22) {
                tile = floorT;
                pal = PAL_CARPET;
            } else if (cy == 23) {
                tile = carpetEdge;
                pal = PAL_CARPET;
            } else if (cy <= 25) {
                tile = carpet;
                pal = PAL_CLOTH;
            } else {
                tile = floorT;
                pal = PAL_CARPET;
            }
            vdp.B.set(cx, cy, gs::entry(tile, pal));
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(12, 9, 3), gs::rgb4(6, 5, 4), gs::rgb4(13, 3, 3),
                          gs::rgb4(4, 12, 6), gs::rgb4(14, 6, 2), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(4, 3, 3), gs::rgb4(8, 7, 6), gs::rgb4(5, 4, 4), gs::rgb4(11, 10, 8),
                            gs::rgb4(4, 6, 3), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(12, 2, 2), gs::rgb4(7, 1, 2), gs::rgb4(13, 10, 3), gs::rgb4(14, 12, 8),
                            gs::rgb4(2, 2, 6), gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 2)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 3, 6), gs::rgb4(4, 6, 10), gs::rgb4(13, 9, 6),
                             gs::rgb4(2, 1, 1), gs::rgb4(3, 2, 2), gs::rgb4(11, 10, 8), gs::rgb4(8, 5, 2),
                             gs::rgb4(14, 14, 12), gs::rgb4(12, 9, 3)});
    setPal(vdp, PAL_GUARD, {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 7), gs::rgb4(10, 10, 11), gs::rgb4(8, 1, 2),
                            gs::rgb4(4, 1, 1), gs::rgb4(12, 8, 6), gs::rgb4(9, 9, 8), gs::rgb4(3, 2, 2), gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(3, 2, 1), gs::rgb4(10, 3, 1), gs::rgb4(14, 8, 1), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 11)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 6), gs::rgb4(12, 12, 14), gs::rgb4(4, 5, 8),
                            gs::rgb4(4, 3, 3), gs::rgb4(7, 6, 5), gs::rgb4(10, 9, 8)});
    setPal(vdp, PAL_CARPET, {0, gs::rgb4(3, 2, 2), gs::rgb4(6, 5, 4), gs::rgb4(5, 4, 3), gs::rgb4(8, 7, 6),
                             gs::rgb4(5, 4, 4), gs::rgb4(7, 6, 5)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(8, 7, 5), gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(1, 1, 1), gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(10, 7, 3),
                           gs::rgb4(5, 5, 6), gs::rgb4(1, 1, 4), gs::rgb4(13, 13, 15), gs::rgb4(8, 5, 2)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    paintHall(vdp, tiles);

    art.stand = gs::uploadMipped(vdp, thief(0));
    art.runA = gs::uploadMipped(vdp, thief(1));
    art.runB = gs::uploadMipped(vdp, thief(2));
    art.air = gs::uploadMipped(vdp, thief(3));
    art.duck = gs::uploadMipped(vdp, thief(4));
    for (int i = 0; i < 3; i++) art.guard[i] = gs::uploadMipped(vdp, sentry(i));
    art.banner[0] = gs::uploadMipped(vdp, bannerCloth(0));
    art.banner[1] = gs::uploadMipped(vdp, bannerCloth(1));
    art.standBase = gs::uploadMipped(vdp, standBase());
    art.column = gs::uploadMipped(vdp, columnArt());
    art.window = gs::uploadMipped(vdp, windowArt());
    art.sconce = gs::uploadMipped(vdp, sconceArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.grate = gs::uploadMipped(vdp, grateArt());
    art.tapestry = gs::uploadMipped(vdp, tapestryArt());
    art.dais = gs::uploadMipped(vdp, daisArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.setFogColor(gs::rgb4(2, 1, 1));
}

}  // namespace breach
