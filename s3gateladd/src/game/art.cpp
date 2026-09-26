#include "game/art.h"

#include <initializer_list>
#include <string>

namespace gateladd {
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

// Ochre cloak, nasal helm, horn on the hip. Feet sit on the last row.
gs::Bitmap sentry(int pose) {
    gs::Bitmap b(32, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    auto P = [&](int x, int y, int c) { b.set(x, y, c); };
    R(12, 1, 10, 7, 5);
    R(11, 6, 13, 3, 6);
    P(16, 2, 6);
    R(13, 9, 8, 7, 4);
    P(18, 11, 10);
    P(19, 12, 1);
    if (pose >= 4) {
        int lift = pose == 4 ? 0 : 1;
        R(lift ? 6 : 20, 2, 5, 12, 2);
        R(lift ? 7 : 21, 2, 3, 4, 9);
        R(lift ? 20 : 6, 14, 5, 10, 2);
        R(lift ? 21 : 7, 20, 3, 4, 9);
        R(10, 16, 13, 14, 2);
        R(10, 16, 4, 14, 3);
        R(12, 28, 9, 3, 7);
        R(20, 24, 4, 5, 11);
        P(21, 26, 14);
        int lk = pose == 4 ? 2 : -1;
        R(12, 30, 4, 10 + lk, 12);
        R(18, 30, 4, 10 - lk, 12);
        R(11, 40, 6, 8, 8);
        R(17, 41, 6, 7, 8);
        b.outline(1, false);
        return b;
    }
    int ls = 0, rs = 0, la = 0, ra = 0;
    if (pose == 1) {
        ls = 1;
        rs = -3;
        la = -1;
        ra = 2;
    } else if (pose == 2) {
        ls = -3;
        rs = 1;
        la = 2;
        ra = -1;
    }
    R(8, 18 + la, 5, 11, 2);
    R(8, 26 + la, 4, 3, 9);
    R(20, 17 + ra, 5, 12, 2);
    R(21, 26 + ra, 4, 3, 9);
    R(10, 16, 13, 16, 2);
    R(10, 16, 4, 16, 3);
    R(11, 29, 11, 3, 7);
    R(20, 24, 5, 5, 11);
    P(22, 26, 14);
    if (pose == 3) {
        R(14, 32, 5, 8, 12);
        R(19, 33, 5, 7, 12);
        R(18, 40, 7, 8, 8);
        R(13, 42, 6, 6, 8);
    } else {
        R(12 + ls, 32, 4, 10, 12);
        R(18 + rs, 32, 4, 10, 12);
        R(11 + ls, 42, 6, 6, 8);
        R(17 + rs, 42, 6, 6, 8);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(14, 32);
    b.rect(1, 0, 3, 32, 1);
    b.rect(10, 0, 3, 32, 2);
    b.rect(2, 0, 1, 32, 3);
    b.rect(11, 0, 1, 32, 4);
    for (int y = 2; y < 32; y += 8) {
        b.rect(3, y, 8, 2, 2);
        b.rect(3, y, 8, 1, 4);
    }
    return b;
}

gs::Bitmap grateArt() {
    gs::Bitmap b(24, 56);
    for (int x = 1; x <= 19; x += 6) {
        b.rect(x, 0, 4, 50, 2);
        b.rect(x, 0, 1, 50, 4);
        b.rect(x, 48, 4, 3, 1);
        b.set(x + 1, 52, 3);
        b.set(x + 2, 54, 2);
    }
    b.rect(0, 10, 24, 3, 3);
    b.rect(0, 28, 24, 3, 3);
    b.rect(0, 44, 24, 3, 1);
    b.rect(0, 11, 24, 1, 4);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 48);
    b.rect(1, 0, 8, 48, 2);
    b.rect(2, 0, 3, 48, 3);
    b.rect(0, 0, 10, 4, 8);
    b.rect(1, 4, 8, 2, 9);
    for (int y = 10; y < 46; y += 8) b.rect(1, y, 8, 1, 1);
    return b;
}

gs::Bitmap winchArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 2);
    b.ellipse(10, 10, 5, 5, 3);
    b.ellipse(10, 10, 2, 2, 4);
    b.rect(9, 0, 2, 20, 1);
    b.rect(0, 9, 20, 2, 1);
    return b;
}

gs::Bitmap gemArt() {
    gs::Bitmap b(8, 8);
    b.rect(2, 1, 4, 6, 1);
    b.rect(1, 2, 6, 4, 1);
    b.set(3, 3, 3);
    b.set(4, 3, 3);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(16, 18);
    b.rect(7, 0, 2, 3, 1);
    b.rect(4, 3, 8, 3, 2);
    b.rect(2, 6, 12, 7, 3);
    b.rect(3, 7, 4, 5, 4);
    b.rect(1, 12, 14, 3, 2);
    b.rect(6, 15, 4, 3, 1);
    b.set(8, 16, 4);
    return b;
}

gs::Bitmap torchArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 6, 2, 12, 1);
    b.rect(3, 16, 4, 3, 2);
    b.rect(2, 8, 6, 3, 4);
    b.rect(3, 9, 4, 1, 3);
    return b;
}

gs::Bitmap flameArt(int hot) {
    gs::Bitmap b(10, 12);
    b.rect(4, 8, 2, 4, 1);
    b.rect(3, 5, 4, 5, 2);
    b.rect(4, 3, 2, 4, 3);
    if (hot) {
        b.rect(3, 2, 3, 4, 3);
        b.set(4, 1, 4);
    } else {
        b.rect(5, 4, 2, 3, 3);
        b.set(5, 2, 4);
    }
    return b;
}

gs::Bitmap guardArt(int step) {
    gs::Bitmap b(26, 44);
    int bob = step ? 1 : 0;
    b.rect(10, 2 + bob, 8, 5, 4);
    b.rect(9, 6 + bob, 10, 2, 5);
    b.rect(11, 7 + bob, 6, 5, 6);
    b.set(15, 9 + bob, 1);
    b.rect(8, 12 + bob, 11, 12, 2);
    b.rect(8, 12 + bob, 3, 12, 3);
    b.rect(9, 22 + bob, 9, 2, 8);
    b.rect(17, 8 + bob, 3, 16, 4);
    b.rect(18, 6 + bob, 2, 4, 5);
    b.set(18, 4, 5);
    if (step) {
        b.rect(9, 24, 4, 14, 1);
        b.rect(14, 26, 4, 12, 1);
        b.rect(8, 37, 6, 6, 7);
        b.rect(14, 36, 6, 7, 7);
    } else {
        b.rect(9, 26, 4, 12, 1);
        b.rect(14, 24, 4, 14, 1);
        b.rect(8, 36, 6, 7, 7);
        b.rect(14, 37, 6, 6, 7);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(16, 34);
    b.rect(2, 0, 12, 3, 8);
    b.rect(3, 3, 10, 28, 2);
    b.rect(3, 3, 3, 28, 3);
    b.rect(7, 8, 2, 18, 8);
    for (int y = 6; y < 30; y += 6) b.rect(3, y, 10, 1, 1);
    b.rect(4, 30, 8, 3, 2);
    b.set(6, 32, 3);
    b.set(9, 32, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap keystoneArt() {
    gs::Bitmap b(18, 14);
    b.rect(2, 2, 14, 10, 2);
    b.rect(4, 1, 10, 4, 9);
    b.rect(6, 4, 6, 6, 3);
    b.rect(7, 6, 4, 3, 8);
    b.rect(1, 10, 16, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(12, 13, 10, 10, 3);
    b.ellipse(11, 12, 8, 8, 4);
    b.ellipse(8, 10, 2, 2, 2);
    b.ellipse(14, 15, 2, 1, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(7, 4, 6, 2, 2);
    b.ellipse(4, 4, 2, 1, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t sh = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 0, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(5, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 8, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 7, 4), gs::rgb4(13, 12, 8), gs::rgb4(15, 15, 12), 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4), gs::rgb4(2, 5, 2),
            gs::rgb4(10, 6, 2), gs::rgb4(1, 0, 1), gs::rgb4(9, 8, 7), gs::rgb4(13, 11, 8), gs::rgb4(2, 1, 2),
            gs::rgb4(3, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(7, 3, 2), gs::rgb4(4, 4, 6), sh});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(8, 4, 1), gs::rgb4(12, 8, 2), gs::rgb4(13, 9, 6), gs::rgb4(4, 5, 6),
            gs::rgb4(10, 11, 12), gs::rgb4(3, 2, 1), gs::rgb4(2, 1, 1), gs::rgb4(12, 8, 5), gs::rgb4(15, 15, 13),
            gs::rgb4(12, 9, 3), gs::rgb4(2, 2, 4), gs::rgb4(5, 2, 1), gs::rgb4(15, 12, 4), sh});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 4), gs::rgb4(5, 5, 7), gs::rgb4(9, 9, 11), gs::rgb4(13, 13, 15),
                           gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(5, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7),
                            gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_WARDEN,
           {0, gs::rgb4(1, 0, 1), gs::rgb4(8, 1, 1), gs::rgb4(13, 3, 2), gs::rgb4(6, 6, 8), gs::rgb4(11, 11, 13),
            gs::rgb4(13, 9, 6), gs::rgb4(2, 1, 1), gs::rgb4(12, 9, 3), 0, 0, 0, 0, 0, 0, gs::rgb4(1, 0, 0)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(5, 5, 7),
                           gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 2, 1), gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 12),
                         gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(1, 1, 4), gs::rgb4(2, 2, 6), gs::rgb4(3, 3, 7), gs::rgb4(8, 7, 3),
                          gs::rgb4(14, 14, 11), gs::rgb4(2, 2, 5), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 0, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    art.cope = makeTile(tiles, vdp, {"99999999", "88888888", "33333333", "22222222", "22232222", "22222222", "22222222",
                                     "11111111"});
    art.ash = makeTile(tiles, vdp, {"22232222", "22232222", "22222232", "11111111", "32222222", "22222223", "23222222",
                                    "11111111"});
    art.ashB = makeTile(tiles, vdp, {"23222222", "22223222", "22222222", "11111111", "22222322", "32222222", "22223222",
                                     "11111111"});
    art.slit = makeTile(tiles, vdp, {"22222222", "22111222", "22111222", "22111222", "22111222", "22222222", "22222222",
                                     "11111111"});
    art.slitLit = makeTile(tiles, vdp, {"22222222", "22666222", "22666222", "22666222", "22666222", "22222222",
                                        "22222222", "11111111"});
    art.voidT = makeTile(tiles, vdp, {"77777777", "77777777", "77777777", "77777777", "77777777", "77777777", "77777777",
                                      "77777777"});
    art.yard = makeTile(tiles, vdp, {"aaaaaaab", "baaaaaaa", "aaaaabaa", "abaaaaaa", "aaaaaaba", "baaaaaaa", "aaaaabaa",
                                     "abaaaaab"});
    art.yardB = makeTile(tiles, vdp, {"bbabbbab", "abbbabbb", "bbbabbba", "babbbabb", "bbbabbbb", "abbbabbb", "bbbabbba",
                                      "bbabbbab"});
    art.vous = makeTile(tiles, vdp, {"88888888", "32222223", "22222222", "22222222", "11111111", "77777777", "77777777",
                                     "77777777"});
    art.jamb = makeTile(tiles, vdp, {"22288888", "22288883", "22288882", "22288882", "11188881", "22277777", "22277777",
                                     "22277777"});
    art.door = makeTile(tiles, vdp, {"44424442", "22232222", "22222222", "44424442", "22322222", "22222232", "22222222",
                                     "44424442"});
    art.mer = makeTile(tiles, vdp, {"00088000", "00888800", "08888880", "08888880", "88888888", "33333333", "22222222",
                                    "22222222"});
    art.ivy = makeTile(tiles, vdp, {"22252222", "22522252", "22252222", "11111111", "52222222", "22252232", "22522222",
                                    "11111111"});
    art.star = makeTile(tiles, vdp, {"00000000", "00005000", "00000000", "00000000", "00000000", "00000000", "00000000",
                                     "00000000"});
    art.starB = makeTile(tiles, vdp, {"00000000", "00005000", "00055000", "00005000", "00000000", "00000000", "00000000",
                                      "00000000"});
    art.farW = makeTile(tiles, vdp, {"11111111", "11111111", "11141111", "11111111", "22222222", "11111111", "11111111",
                                     "11111111"});
    art.farR = makeTile(tiles, vdp, {"66666666", "33333333", "22222222", "11111111", "11111111", "11111111", "11111111",
                                     "22222222"});

    art.stand = gs::uploadMipped(vdp, sentry(0));
    art.walkA = gs::uploadMipped(vdp, sentry(1));
    art.walkB = gs::uploadMipped(vdp, sentry(2));
    art.jump = gs::uploadMipped(vdp, sentry(3));
    art.climbA = gs::uploadMipped(vdp, sentry(4));
    art.climbB = gs::uploadMipped(vdp, sentry(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.grate = gs::uploadMipped(vdp, grateArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.winch = gs::uploadMipped(vdp, winchArt());
    art.gem = gs::uploadMipped(vdp, gemArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.torch = gs::uploadMipped(vdp, torchArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.guard[0] = gs::uploadMipped(vdp, guardArt(0));
    art.guard[1] = gs::uploadMipped(vdp, guardArt(1));
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.keystone = gs::uploadMipped(vdp, keystoneArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace gateladd
