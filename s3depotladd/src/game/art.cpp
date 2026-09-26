#include "game/art.h"

#include <initializer_list>
#include <string>

namespace depotladd {
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

// Yard hand: navy coat, cap lamp, lantern. Feet sit on the last row.
gs::Bitmap hand(int pose) {
    gs::Bitmap b(30, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    auto D = [&](int x, int y, int c) { b.set(x, y, c); };
    R(10, 1, 11, 4, 4);
    R(8, 4, 15, 2, 4);
    R(19, 4, 5, 2, 3);
    D(12, 2, 10);
    D(13, 2, 10);
    R(11, 6, 9, 7, 5);
    D(13, 8, 14);
    D(16, 8, 14);
    R(12, 11, 5, 1, 6);
    if (pose >= 4) {
        bool lift = pose == 4;
        R(lift ? 5 : 21, 3, 4, 12, 2);
        R(lift ? 6 : 22, 3, 2, 4, 11);
        R(lift ? 21 : 5, 15, 4, 10, 2);
        R(lift ? 22 : 6, 21, 2, 4, 11);
        R(10, 13, 10, 15, 2);
        R(11, 13, 3, 15, 3);
        R(12, 25, 6, 2, 9);
        D(18, 23, 10);
        D(19, 23, 10);
        int lk = pose == 4 ? 1 : -1;
        R(11, 28, 4, 11 + lk, 7);
        R(16, 28, 4, 11 - lk, 7);
        R(10, 39, 5, 7, 8);
        R(16, 39, 5, 7, 8);
        b.outline(1, false);
        return b;
    }
    int la = 0, ra = 0, ll = 0, rl = 0;
    if (pose == 1) {
        la = -2;
        ra = 2;
        ll = 2;
        rl = -3;
    } else if (pose == 2) {
        la = 2;
        ra = -2;
        ll = -3;
        rl = 2;
    } else if (pose == 3) {
        la = -4;
        ra = -3;
        ll = 2;
        rl = 3;
    }
    R(6, 16 + la, 4, 11, 2);
    R(7, 24 + la, 3, 3, 11);
    R(9, 14, 13, 16, 2);
    R(9, 14, 4, 16, 3);
    R(10, 14, 8, 2, 12);
    R(12, 27, 7, 2, 9);
    D(14, 20, 13);
    D(14, 23, 13);
    R(8, 18, 3, 7, 8);
    R(20, 15 + ra, 4, 10, 2);
    R(23, 17 + ra, 5, 7, 9);
    R(24, 18 + ra, 3, 4, 10);
    R(21, 24 + ra, 3, 3, 11);
    if (pose == 3) {
        R(10, 30, 5, 8, 7);
        R(16, 31, 5, 7, 7);
        R(9, 37, 6, 6, 8);
        R(16, 38, 6, 6, 8);
    } else {
        R(10 + ll, 30, 4, 10, 7);
        R(16 + rl, 30, 4, 10, 7);
        R(9 + ll, 39, 6, 7, 8);
        R(15 + rl, 39, 6, 7, 8);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(12, 28);
    b.rect(1, 0, 2, 28, 2);
    b.rect(9, 0, 2, 28, 2);
    b.rect(1, 0, 1, 28, 4);
    b.rect(10, 0, 1, 28, 5);
    for (int y = 3; y < 28; y += 7) {
        b.rect(2, y, 8, 2, 3);
        b.rect(2, y, 8, 1, 5);
    }
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 8, 8, 6, 2);
    b.ellipse(9, 8, 6, 4, 3);
    b.rect(1, 3, 16, 2, 5);
    b.rect(1, 11, 16, 2, 5);
    b.rect(8, 2, 2, 12, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 0, 3, 6, 3);
    b.rect(4, 5, 7, 3, 4);
    b.rect(3, 8, 3, 7, 2);
    b.rect(3, 13, 8, 3, 3);
    b.rect(9, 10, 3, 5, 4);
    b.rect(5, 1, 1, 4, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap cableArt() {
    gs::Bitmap b(3, 20);
    b.rect(1, 0, 1, 20, 3);
    b.rect(0, 0, 1, 20, 2);
    return b;
}

gs::Bitmap steamArt(int hot) {
    gs::Bitmap b(14, 18);
    b.ellipse(7, 12, 4, 3, 2);
    b.ellipse(6, 8, 3, 3, hot ? 1 : 2);
    b.ellipse(8, 5, 2, 2, hot ? 4 : 3);
    b.set(7, 3, hot ? 4 : 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 22);
    b.rect(3, 6, 2, 14, 5);
    b.rect(2, 18, 4, 3, 2);
    b.rect(1, 2, 6, 5, 3);
    b.rect(2, 3, 4, 3, 4);
    b.set(3, 4, 6);
    b.rect(1, 1, 6, 1, 7);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 4);
    return b;
}

gs::Bitmap signalArt() {
    gs::Bitmap b(12, 14);
    b.rect(2, 1, 8, 10, 3);
    b.rect(3, 2, 6, 8, 2);
    b.ellipse(6, 6, 3, 3, 1);
    b.rect(5, 11, 2, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(16, 14);
    b.rect(1, 2, 14, 11, 2);
    b.rect(1, 2, 14, 2, 3);
    b.rect(2, 6, 12, 1, 1);
    b.rect(7, 3, 2, 9, 4);
    b.rect(1, 11, 14, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 4, 4, 3);
    b.ellipse(7, 7, 2, 2, 5);
    b.rect(6, 1, 2, 12, 2);
    b.rect(1, 6, 12, 2, 2);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(22, 64);
    b.rect(4, 6, 14, 16, 2);
    b.ellipse(11, 14, 8, 7, 3);
    b.ellipse(11, 13, 5, 4, 4);
    b.rect(3, 20, 16, 3, 1);
    b.rect(2, 22, 18, 2, 5);
    b.rect(4, 24, 3, 36, 2);
    b.rect(15, 24, 3, 36, 2);
    b.rect(4, 40, 14, 2, 1);
    b.rect(10, 18, 6, 2, 6);
    b.rect(16, 18, 4, 2, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 2);
    b.ellipse(9, 9, 6, 6, 4);
    b.rect(8, 4, 2, 6, 5);
    b.rect(8, 8, 5, 2, 5);
    b.set(9, 9, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(10, 11, 8, 8, 3);
    b.ellipse(9, 10, 6, 6, 4);
    b.ellipse(12, 8, 3, 3, 2);
    b.ellipse(7, 13, 2, 2, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 6, 2, 2);
    b.ellipse(4, 4, 2, 1, 3);
    b.ellipse(10, 5, 2, 1, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

gs::Bitmap whistleArt() {
    gs::Bitmap b(14, 16);
    b.rect(6, 0, 2, 4, 5);
    b.rect(3, 4, 8, 6, 3);
    b.rect(2, 8, 10, 3, 2);
    b.rect(4, 11, 3, 4, 5);
    b.rect(9, 6, 4, 2, 4);
    b.set(12, 6, 6);
    b.outline(1, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t sh = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), gs::rgb4(12, 3, 2), gs::rgb4(15, 8, 6),
                            gs::rgb4(4, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 0, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(4, 15, 8), gs::rgb4(1, 6, 3), gs::rgb4(8, 15, 11), gs::rgb4(13, 15, 14), 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_AMBER,
           {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 4, 1), gs::rgb4(14, 8, 2), gs::rgb4(15, 15, 11), gs::rgb4(4, 3, 1),
            gs::rgb4(15, 12, 5), gs::rgb4(12, 6, 1), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 3, 6), gs::rgb4(4, 6, 10), gs::rgb4(1, 2, 4), gs::rgb4(13, 10, 7),
            gs::rgb4(2, 1, 1), gs::rgb4(3, 4, 6), gs::rgb4(4, 2, 1), gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 6),
            gs::rgb4(5, 3, 2), gs::rgb4(13, 12, 11), gs::rgb4(14, 12, 7), gs::rgb4(1, 1, 1), sh});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 5), gs::rgb4(6, 6, 8), gs::rgb4(10, 11, 13),
                           gs::rgb4(13, 14, 15), gs::rgb4(8, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(3, 1, 1), gs::rgb4(8, 2, 1), gs::rgb4(12, 3, 2), gs::rgb4(14, 13, 12),
                          gs::rgb4(1, 1, 1), gs::rgb4(5, 4, 3), gs::rgb4(7, 5, 4), gs::rgb4(15, 14, 13), 0, 0, 0, 0, 0,
                          0, sh});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 4), gs::rgb4(12, 9, 6),
                           gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_STEAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 12, 13), gs::rgb4(8, 9, 10), gs::rgb4(15, 15, 15), 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_TANK, {0, gs::rgb4(1, 2, 1), gs::rgb4(2, 4, 3), gs::rgb4(4, 6, 5), gs::rgb4(8, 10, 9),
                           gs::rgb4(12, 12, 12), gs::rgb4(6, 3, 3), gs::rgb4(13, 14, 13), 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_CONC,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(13, 12, 4), gs::rgb4(1, 1, 2),
            gs::rgb4(14, 10, 4), gs::rgb4(13, 13, 12), gs::rgb4(10, 9, 8), gs::rgb4(6, 4, 3), gs::rgb4(8, 5, 4),
            gs::rgb4(1, 1, 1), gs::rgb4(15, 14, 13), 0, 0, sh});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 4), gs::rgb4(3, 3, 5), gs::rgb4(14, 12, 7),
                            gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0,
                            0, 0, sh});
    setPal(vdp, PAL_DRUM, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 3), gs::rgb4(12, 8, 4),
                           gs::rgb4(2, 2, 3), gs::rgb4(13, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0, sh});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    art.clap = makeTile(tiles, vdp, {"77777777", "88888888", "77777777", "77777777", "77777777", "88888888", "77777777",
                                     "77777777"});
    art.window = makeTile(tiles, vdp, {"77777777", "75555557", "75555557", "75555557", "78888887", "75555557",
                                       "75555557", "77777777"});
    art.winLit = makeTile(tiles, vdp, {"77777777", "76666667", "76666667", "76666667", "78888887", "76666667",
                                       "76666667", "77777777"});
    art.conc = makeTile(tiles, vdp, {"22222222", "22322232", "22232222", "33333333", "22223222", "23222223", "22222222",
                                     "22222222"});
    art.safety = makeTile(tiles, vdp, {"44444444", "44444444", "33333333", "22222222", "22232222", "23222222",
                                       "22222222", "11111111"});
    art.brick = makeTile(tiles, vdp, {"99999999", "9a999a99", "11111111", "99999999", "999a999a", "11111111", "9a999999",
                                      "99999999"});
    art.car = makeTile(tiles, vdp, {"22222222", "23333332", "22242222", "55555555", "22222222", "22322232", "22222222",
                                    "22222222"});
    art.carDoor = makeTile(tiles, vdp, {"22222222", "21111112", "21444412", "21555512", "21444412", "21111112",
                                        "22222222", "55555555"});
    art.roof = makeTile(tiles, vdp, {"11111111", "66666666", "77777777", "66666666", "77767777", "66666666", "55555555",
                                     "11111111"});
    art.wood = makeTile(tiles, vdp, {"22222222", "23333332", "22222222", "44424444", "22232222", "23333332", "22222222",
                                     "55555555"});
    art.grate = makeTile(tiles, vdp, {"44444444", "45555554", "34343434", "11111111", "43434343", "11111111", "33333333",
                                      "22222222"});
    art.beam = makeTile(tiles, vdp, {"33333333", "45555554", "33333333", "22222222", "11111111", "00000000", "00000000",
                                     "00000000"});
    art.post = makeTile(tiles, vdp, {"00333300", "00333300", "00444400", "00333300", "00333300", "00222200", "00333300",
                                     "00333300"});
    art.ballast = makeTile(tiles, vdp, {"11121111", "21111212", "11211121", "11121111", "21112111", "12111112",
                                        "11121111", "11211121"});
    art.rail = makeTile(tiles, vdp, {"00055000", "00055000", "44444444", "33333333", "22222222", "11111111", "11111111",
                                     "11111111"});
    art.tie = makeTile(tiles, vdp, {"11111111", "22252222", "55555555", "22222252", "11111111", "11121111", "21111211",
                                    "11111121"});
    art.gold = makeTile(tiles, vdp, {"33333333", "44444444", "22222222", "11111111", "22222222", "11111111", "11111111",
                                     "55555555"});
    art.tank = makeTile(tiles, vdp, {"22222222", "23333332", "34444443", "33333333", "22222222", "33333333", "11111111",
                                     "22222222"});
    art.tankBand = makeTile(tiles, vdp, {"55555555", "23333332", "34444443", "55555555", "22222222", "66666666",
                                         "11111111", "22222222"});
    art.star = makeTile(tiles, vdp, {"00000000", "00005000", "00000000", "00000000", "00000000", "00000000", "00000000",
                                     "00000000"});
    art.starB = makeTile(tiles, vdp, {"00000000", "00005000", "00055000", "00005000", "00000000", "00000000", "00000000",
                                      "00000000"});
    art.shed = makeTile(tiles, vdp, {"66666666", "33333333", "32222322", "32242322", "32222322", "33333333", "22222222",
                                     "11111111"});
    art.shedB = makeTile(tiles, vdp, {"66666666", "22222222", "22322322", "22232222", "22222322", "33333333", "22222222",
                                      "11111111"});
    art.skyWin = makeTile(tiles, vdp, {"22222222", "22422422", "22222222", "24224222", "22222222", "22242222",
                                       "33333333", "11111111"});

    art.stand = gs::uploadMipped(vdp, hand(0));
    art.walkA = gs::uploadMipped(vdp, hand(1));
    art.walkB = gs::uploadMipped(vdp, hand(2));
    art.jump = gs::uploadMipped(vdp, hand(3));
    art.climbA = gs::uploadMipped(vdp, hand(4));
    art.climbB = gs::uploadMipped(vdp, hand(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.steam[0] = gs::uploadMipped(vdp, steamArt(0));
    art.steam[1] = gs::uploadMipped(vdp, steamArt(1));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.whistle = gs::uploadMipped(vdp, whistleArt());
}

}  // namespace depotladd
