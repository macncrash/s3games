#include "game/art.h"

#include <initializer_list>
#include <string>

namespace yardladd {
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

// Night yard hand: cap lamp, blue coat, boots on the last row.
gs::Bitmap hand(int pose) {
    gs::Bitmap b(32, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    auto P = [&](int x, int y, int c) { b.set(x, y, c); };
    R(11, 2, 11, 4, 5);
    R(10, 5, 14, 2, 6);
    P(20, 3, 11);
    P(21, 3, 4);
    R(13, 7, 8, 6, 4);
    P(15, 9, 10);
    P(18, 9, 10);
    R(12, 12, 10, 2, 13);
    if (pose >= 4) {
        int up = pose == 4 ? 1 : 0;
        R(up ? 6 : 20, 3, 5, 12, 2);
        R(up ? 7 : 21, 3, 3, 4, 9);
        R(up ? 20 : 6, 16, 5, 10, 2);
        R(up ? 21 : 7, 22, 3, 4, 9);
        R(11, 14, 11, 16, 2);
        R(11, 14, 3, 16, 3);
        R(12, 27, 9, 2, 7);
        P(18, 27, 11);
        int lk = pose == 4 ? 1 : -2;
        R(12, 30, 4, 11 + lk, 12);
        R(18, 30, 4, 11 - lk, 12);
        R(11, 40, 6, 8, 8);
        R(17, 41, 6, 7, 8);
        b.outline(1, false);
        return b;
    }
    int ls = 0, rs = 0, la = 0, ra = 0;
    if (pose == 1) {
        ls = 2;
        rs = -3;
        la = -1;
        ra = 2;
    } else if (pose == 2) {
        ls = -3;
        rs = 2;
        la = 2;
        ra = -1;
    }
    R(7, 16 + la, 5, 11, 2);
    R(8, 24 + la, 4, 3, 9);
    R(20, 16 + ra, 5, 11, 2);
    R(21, 24 + ra, 4, 3, 9);
    R(10, 14, 13, 16, 2);
    R(10, 14, 4, 16, 3);
    R(11, 27, 11, 3, 7);
    P(19, 28, 11);
    if (pose == 3) {
        R(12, 30, 5, 8, 12);
        R(18, 31, 5, 7, 12);
        R(11, 38, 7, 10, 8);
        R(17, 40, 6, 8, 8);
    } else {
        R(11 + ls, 30, 4, 10, 12);
        R(18 + rs, 30, 4, 10, 12);
        R(10 + ls, 40, 6, 8, 8);
        R(17 + rs, 40, 6, 8, 8);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(12, 32);
    b.rect(1, 0, 3, 32, 2);
    b.rect(8, 0, 3, 32, 2);
    b.rect(1, 0, 1, 32, 3);
    b.rect(9, 0, 1, 32, 4);
    for (int y = 3; y < 32; y += 8) {
        b.rect(2, y, 8, 2, 4);
        b.rect(2, y, 8, 1, 5);
    }
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(22, 40);
    for (int y = 0; y < 36; y += 6) {
        b.rect(0, y, 22, 2, 2);
        b.rect(0, y, 22, 1, 3);
    }
    for (int x = 1; x < 20; x += 5) {
        b.rect(x, 0, 2, 36, 2);
        b.rect(x, 0, 1, 36, 4);
    }
    for (int y = 2; y < 34; y += 6) {
        b.line(2, float(y), 19, float(y + 5), 5, 1);
        b.line(19, float(y), 2, float(y + 5), 1, 1);
    }
    b.rect(0, 33, 22, 7, 1);
    b.rect(0, 34, 22, 3, 5);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 64);
    b.rect(2, 0, 6, 64, 2);
    b.rect(3, 0, 2, 64, 3);
    b.rect(1, 0, 8, 5, 1);
    b.rect(2, 5, 6, 2, 4);
    for (int y = 12; y < 60; y += 10) b.rect(2, y, 6, 1, 1);
    return b;
}

gs::Bitmap motorArt() {
    gs::Bitmap b(18, 14);
    b.rect(1, 3, 16, 9, 2);
    b.rect(2, 4, 14, 7, 3);
    b.rect(6, 1, 6, 4, 1);
    b.rect(7, 5, 4, 5, 4);
    b.rect(0, 11, 18, 3, 1);
    b.set(8, 7, 5);
    b.set(10, 7, 5);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 18);
    b.rect(7, 0, 2, 3, 1);
    b.rect(4, 3, 8, 3, 2);
    b.rect(2, 6, 12, 7, 3);
    b.rect(3, 7, 5, 5, 4);
    b.rect(1, 12, 14, 3, 2);
    b.rect(6, 15, 4, 3, 1);
    b.set(8, 16, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 8, 2, 16, 1);
    b.rect(2, 3, 6, 6, 2);
    b.rect(3, 4, 4, 4, 4);
    b.rect(3, 23, 4, 4, 5);
    b.rect(1, 8, 8, 2, 3);
    return b;
}

gs::Bitmap flameArt(int hot) {
    gs::Bitmap b(8, 12);
    b.rect(3, 8, 2, 4, 1);
    b.rect(2, 5, 4, 5, 2);
    b.rect(3, 3, 2, 4, 3);
    if (hot) {
        b.rect(3, 1, 2, 3, 4);
        b.set(4, 0, 4);
    } else {
        b.set(4, 2, 4);
        b.rect(5, 4, 1, 3, 3);
    }
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(14, 36);
    b.rect(2, 0, 10, 3, 5);
    b.rect(3, 3, 8, 28, 2);
    b.rect(3, 3, 3, 28, 3);
    b.rect(6, 8, 2, 16, 8);
    for (int y = 6; y < 30; y += 5) b.rect(3, y, 8, 1, 1);
    b.rect(4, 30, 6, 4, 2);
    b.set(5, 33, 3);
    b.set(8, 33, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 2);
    b.ellipse(8, 8, 5, 4, 3);
    b.rect(2, 3, 12, 2, 4);
    b.rect(2, 11, 12, 2, 4);
    b.rect(3, 4, 10, 1, 5);
    b.rect(3, 11, 10, 1, 5);
    return b;
}

gs::Bitmap sentryArt(int step) {
    gs::Bitmap b(22, 40);
    int bob = step ? 1 : 0;
    b.rect(7, 1 + bob, 8, 4, 5);
    b.rect(6, 4 + bob, 10, 2, 6);
    b.rect(8, 5 + bob, 6, 5, 4);
    b.set(10, 7 + bob, 9);
    b.set(13, 7 + bob, 9);
    b.rect(6, 10 + bob, 10, 12, 2);
    b.rect(6, 10 + bob, 3, 12, 3);
    b.rect(7, 20 + bob, 8, 2, 7);
    b.set(13, 20 + bob, 8);
    b.rect(14, 8 + bob, 2, 12, 1);
    if (step) {
        b.rect(7, 22, 4, 12, 6);
        b.rect(12, 23, 4, 11, 6);
        b.rect(6, 33, 5, 6, 6);
        b.rect(12, 32, 5, 7, 6);
    } else {
        b.rect(7, 23, 4, 11, 6);
        b.rect(12, 22, 4, 12, 6);
        b.rect(6, 32, 5, 7, 6);
        b.rect(12, 33, 5, 6, 6);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(10, 11, 8, 8, 3);
    b.ellipse(9, 10, 6, 6, 4);
    b.ellipse(7, 9, 2, 2, 2);
    b.ellipse(12, 13, 1, 1, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 6, 2, 6);
    b.ellipse(4, 4, 2, 1, 5);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

gs::Bitmap signalArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.set(3, 3, 2);
    b.set(4, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 15, 8), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(9, 4, 3), gs::rgb4(12, 6, 3), gs::rgb4(5, 2, 2), gs::rgb4(5, 4, 3),
            gs::rgb4(8, 7, 5), gs::rgb4(3, 5, 2), gs::rgb4(13, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2),
            gs::rgb4(13, 10, 4), gs::rgb4(2, 2, 4), gs::rgb4(15, 13, 9), gs::rgb4(6, 5, 5), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_HAND,
           {0, ink, gs::rgb4(3, 4, 8), gs::rgb4(5, 7, 12), gs::rgb4(13, 9, 6), gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2),
            gs::rgb4(6, 4, 1), gs::rgb4(2, 1, 1), gs::rgb4(10, 7, 4), gs::rgb4(15, 15, 13), gs::rgb4(14, 12, 5),
            gs::rgb4(2, 2, 4), gs::rgb4(7, 8, 11), gs::rgb4(15, 12, 4), ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 7), gs::rgb4(8, 8, 10), gs::rgb4(12, 12, 14),
                           gs::rgb4(8, 4, 3), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(4, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(13, 10, 4), gs::rgb4(15, 13, 6),
                            gs::rgb4(15, 15, 10), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_WATCH,
           {0, gs::rgb4(1, 0, 1), gs::rgb4(10, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(12, 8, 5), gs::rgb4(5, 3, 2),
            gs::rgb4(2, 1, 1), gs::rgb4(3, 2, 1), gs::rgb4(12, 9, 3), gs::rgb4(14, 14, 12), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4),
                           gs::rgb4(14, 11, 7), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(8, 2, 1), gs::rgb4(15, 7, 1), gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 12),
                         gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(2, 2, 5), gs::rgb4(3, 3, 6), gs::rgb4(4, 3, 3), gs::rgb4(6, 7, 9),
                          gs::rgb4(14, 14, 12), gs::rgb4(12, 10, 5), gs::rgb4(6, 4, 4), 0, 0, 0, 0, 0, 0, 0,
                          gs::rgb4(0, 0, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    art.cope = makeTile(tiles, vdp,
                        {"dddddddd", "88888888", "99999999", "aaaaaaaa", "11111111", "99999999", "aaaaaaaa", "11111111"});
    art.brick = makeTile(tiles, vdp,
                         {"23322332", "eeeeeeee", "32233223", "eeeeeeee", "23322332", "eeeeeeee", "32233223", "11111111"});
    art.brickB = makeTile(tiles, vdp,
                          {"32233223", "eeeeeeee", "23322332", "eeeeeeee", "32233223", "eeeeeeee", "23322332", "11111111"});
    art.crate = makeTile(tiles, vdp,
                         {"88888888", "99999999", "99aa9999", "99999999", "aaaaaaaa", "99999999", "99aa9999", "11111111"});
    art.crateB = makeTile(tiles, vdp,
                          {"99999999", "aaaaaaaa", "aa99aa99", "aaaaaaaa", "99999999", "aaaaaaaa", "aa99aaaa", "11111111"});
    art.gravel = makeTile(tiles, vdp,
                          {"55565556", "65655565", "55567555", "56555556", "55565565", "67555555", "55565655", "56555565"});
    art.gravelB = makeTile(tiles, vdp,
                           {"65555565", "55565555", "56555765", "55565556", "65655555", "55556565", "75555556", "55565555"});
    art.pit = makeTile(tiles, vdp,
                       {"cccccccc", "cffffffc", "fffcffff", "cffffffc", "ffffcfff", "cffffffc", "fffcffff", "cccccccc"});
    art.slit = makeTile(tiles, vdp,
                        {"22222222", "22444222", "22444222", "22222222", "44444444", "22222222", "23322332", "11111111"});
    art.slitLit = makeTile(tiles, vdp,
                           {"22222222", "22bbb222", "22bbb222", "22222222", "44444444", "22222222", "32233223", "11111111"});
    art.star = makeTile(tiles, vdp, {"00000000", "00005000", "00000000", "00000000", "00000000", "00000000", "00000000",
                                     "00000000"});
    art.starB = makeTile(tiles, vdp, {"00000000", "00005000", "00055000", "00005000", "00000000", "00000000", "00000000",
                                      "00000000"});
    art.shed = makeTile(tiles, vdp,
                        {"22222222", "21111112", "22222222", "21111112", "22222222", "21111112", "22222222", "11111111"});
    art.roof = makeTile(tiles, vdp,
                        {"77777777", "33333333", "77777777", "33333333", "11111111", "22222222", "22222222", "11111111"});
    art.win = makeTile(tiles, vdp,
                       {"22222222", "24444442", "24444442", "24444442", "22222222", "21111112", "22222222", "11111111"});
    art.winLit = makeTile(tiles, vdp,
                          {"22222222", "26666642", "26666642", "24444442", "22222222", "21111112", "22222222", "11111111"});

    art.stand = gs::uploadMipped(vdp, hand(0));
    art.walkA = gs::uploadMipped(vdp, hand(1));
    art.walkB = gs::uploadMipped(vdp, hand(2));
    art.jump = gs::uploadMipped(vdp, hand(3));
    art.climbA = gs::uploadMipped(vdp, hand(4));
    art.climbB = gs::uploadMipped(vdp, hand(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.motor = gs::uploadMipped(vdp, motorArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
}

}  // namespace yardladd
