#include "game/art.h"

#include <initializer_list>
#include <string>

namespace cisternladd {
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

// Wader: oilskin hood, rubber boots. pose 0 stand, 1-2 walk, 3 jump, 4-5 climb.
gs::Bitmap wader(int pose) {
    gs::Bitmap b(32, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(11, 2, 11, 5, 5);
    R(10, 6, 13, 2, 6);
    R(13, 8, 8, 6, 4);
    b.set(15, 10, 10);
    b.set(18, 10, 10);
    R(12, 13, 10, 2, 13);
    if (pose >= 4) {
        int up = pose == 4 ? 1 : 0;
        R(up ? 5 : 21, 4, 5, 13, 3);
        R(up ? 6 : 22, 4, 3, 4, 9);
        R(up ? 21 : 5, 17, 5, 10, 3);
        R(up ? 22 : 6, 23, 3, 4, 9);
        R(10, 15, 12, 15, 3);
        R(11, 15, 3, 15, 7);
        int lk = pose == 4 ? 2 : -2;
        R(12, 30, 4, 12 + lk, 8);
        R(18, 30, 4, 12 - lk, 8);
        R(11, 41, 6, 7, 2);
        R(17, 42, 6, 6, 2);
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
    R(6, 16 + la, 5, 12, 3);
    R(7, 24 + la, 4, 3, 9);
    R(21, 16 + ra, 5, 12, 3);
    R(22, 24 + ra, 4, 3, 9);
    R(10, 15, 13, 15, 3);
    R(11, 15, 4, 15, 7);
    if (pose == 3) {
        R(12, 30, 5, 8, 8);
        R(18, 32, 5, 6, 8);
        R(11, 37, 7, 11, 2);
        R(17, 39, 6, 9, 2);
    } else {
        R(11 + ls, 30, 4, 11, 8);
        R(18 + rs, 30, 4, 11, 8);
        R(10 + ls, 40, 6, 8, 2);
        R(17 + rs, 40, 6, 8, 2);
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

gs::Bitmap sluiceArt() {
    gs::Bitmap b(24, 40);
    for (int y = 0; y < 40; y += 5) {
        b.rect(0, y, 24, 4, (y / 5) & 1 ? 2 : 3);
        b.rect(0, y, 24, 1, 4);
    }
    b.rect(2, 2, 3, 36, 5);
    b.rect(19, 2, 3, 36, 5);
    b.rect(10, 0, 4, 40, 1);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 64);
    b.rect(2, 0, 6, 64, 2);
    b.rect(2, 0, 2, 64, 3);
    b.rect(1, 0, 8, 4, 4);
    b.rect(0, 58, 10, 6, 1);
    return b;
}

gs::Bitmap motorArt() {
    gs::Bitmap b(22, 14);
    b.rect(1, 2, 20, 10, 2);
    b.rect(2, 3, 18, 8, 3);
    b.ellipse(11, 7, 4, 4, 4);
    b.ellipse(11, 7, 2, 2, 5);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(14, 16);
    b.rect(6, 0, 2, 3, 3);
    b.ellipse(7, 9, 6, 5, 4);
    b.ellipse(7, 8, 4, 3, 5);
    b.rect(6, 13, 2, 3, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 0, 2, 6, 2);
    b.rect(2, 6, 6, 8, 3);
    b.rect(3, 7, 4, 6, 4);
    b.rect(3, 14, 4, 8, 2);
    return b;
}

gs::Bitmap flameArt(int n) {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 6, 2, 3, 2);
    b.ellipse(4, n ? 4 : 5, 1, 2, 3);
    b.set(4, 2, 4);
    return b;
}

gs::Bitmap weedArt() {
    gs::Bitmap b(28, 20);
    b.rect(0, 8, 4, 12, 2);
    b.line(4, 16, 14, 4, 3, 2);
    b.line(14, 4, 26, 14, 4, 2);
    b.ellipse(14, 4, 3, 2, 5);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 8, 8, 6, 2);
    b.ellipse(9, 8, 6, 4, 3);
    b.rect(1, 4, 16, 2, 4);
    b.rect(1, 10, 16, 2, 4);
    return b;
}

gs::Bitmap dripArt(int n) {
    gs::Bitmap b(6, 10);
    b.ellipse(3, n ? 7 : 4, 2, 3, 2);
    b.set(3, n ? 3 : 1, 3);
    return b;
}

gs::Bitmap ventArt() {
    gs::Bitmap b(20, 12);
    b.rect(0, 2, 20, 8, 2);
    for (int x = 2; x < 18; x += 4) b.rect(x, 3, 2, 6, 1);
    b.rect(0, 2, 20, 1, 3);
    return b;
}

gs::Bitmap mistArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 7, 2, 3);
    b.ellipse(5, 4, 2, 1, 2);
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
    const uint16_t ink = gs::rgb4(0, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 1)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(5, 15, 10), gs::rgb4(12, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 14, 8), gs::rgb4(7, 8, 4), gs::rgb4(12, 13, 8), gs::rgb4(15, 15, 12), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(1, 2, 3), gs::rgb4(6, 7, 8), gs::rgb4(8, 9, 10), gs::rgb4(4, 5, 6), gs::rgb4(3, 5, 6),
            gs::rgb4(5, 8, 8), gs::rgb4(2, 6, 5), gs::rgb4(10, 11, 10), gs::rgb4(7, 8, 7), gs::rgb4(3, 4, 5),
            gs::rgb4(12, 13, 8), gs::rgb4(1, 3, 6), gs::rgb4(14, 15, 12), gs::rgb4(4, 6, 7), gs::rgb4(0, 1, 2)});
    setPal(vdp, PAL_WADER,
           {0, ink, gs::rgb4(2, 2, 3), gs::rgb4(8, 10, 3), gs::rgb4(13, 10, 7), gs::rgb4(4, 5, 2), gs::rgb4(6, 7, 2),
            gs::rgb4(3, 6, 4), gs::rgb4(1, 2, 3), gs::rgb4(11, 9, 5), gs::rgb4(15, 15, 13), gs::rgb4(14, 13, 6),
            gs::rgb4(2, 3, 5), gs::rgb4(12, 13, 8), gs::rgb4(15, 14, 6), ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(1, 2, 3), gs::rgb4(4, 5, 7), gs::rgb4(7, 8, 10), gs::rgb4(11, 12, 14),
                           gs::rgb4(8, 5, 3), gs::rgb4(0, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(3, 3, 1), gs::rgb4(8, 7, 2), gs::rgb4(12, 10, 4), gs::rgb4(15, 13, 6),
                            gs::rgb4(15, 15, 10), gs::rgb4(2, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 0)});
    setPal(vdp, PAL_MOSS,
           {0, gs::rgb4(0, 1, 1), gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 4), gs::rgb4(8, 12, 5), gs::rgb4(3, 4, 2),
            gs::rgb4(1, 2, 2), gs::rgb4(1, 3, 2), gs::rgb4(10, 12, 4), gs::rgb4(13, 14, 10), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4),
                           gs::rgb4(14, 11, 7), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(2, 4, 8), gs::rgb4(6, 12, 14), gs::rgb4(12, 15, 14), gs::rgb4(15, 15, 12),
                         gs::rgb4(4, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_VAULT, {0, gs::rgb4(1, 2, 4), gs::rgb4(2, 3, 5), gs::rgb4(3, 4, 4), gs::rgb4(5, 7, 8),
                            gs::rgb4(10, 12, 11), gs::rgb4(8, 10, 6), gs::rgb4(4, 5, 4), 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(0, 1, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    art.cope = makeTile(tiles, vdp,
                        {"dddddddd", "88888888", "99999999", "aaaaaaaa", "11111111", "99999999", "aaaaaaaa", "11111111"});
    art.block = makeTile(tiles, vdp,
                         {"23322332", "eeeeeeee", "32233223", "eeeeeeee", "23322332", "eeeeeeee", "32233223", "11111111"});
    art.blockB = makeTile(tiles, vdp,
                          {"32233223", "eeeeeeee", "23322332", "eeeeeeee", "32233223", "eeeeeeee", "23322332", "11111111"});
    art.pier = makeTile(tiles, vdp,
                        {"55555555", "66666666", "55aa5555", "66666666", "aaaaaaaa", "66666666", "55aa5555", "11111111"});
    art.pierB = makeTile(tiles, vdp,
                         {"66666666", "aaaaaaaa", "aa55aa55", "aaaaaaaa", "55555555", "aaaaaaaa", "aa55aaaa", "11111111"});
    art.water = makeTile(tiles, vdp,
                         {"cccccccc", "c33333cc", "333c3333", "c33333cc", "33c33333", "c33333cc", "333c3333", "cccccccc"});
    art.waterB = makeTile(tiles, vdp,
                          {"333c3333", "c33333cc", "cc3333c3", "333c3333", "c33333cc", "33c33333", "c33333cc", "333c3333"});
    art.deep = makeTile(tiles, vdp,
                        {"11111111", "1cccccc1", "cc1ccccc", "1cccccc1", "cccc1ccc", "1cccccc1", "cc1ccccc", "11111111"});
    art.slit = makeTile(tiles, vdp,
                        {"22222222", "22444222", "22444222", "22222222", "44444444", "22222222", "23322332", "11111111"});
    art.slitLit = makeTile(tiles, vdp,
                           {"22222222", "22bbb222", "22bbb222", "22222222", "44444444", "22222222", "32233223", "11111111"});
    art.dripT = makeTile(tiles, vdp, {"00002000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000",
                                      "00000000"});
    art.dripB = makeTile(tiles, vdp, {"00000000", "00002000", "00022000", "00002000", "00000000", "00000000", "00000000",
                                      "00000000"});
    art.rib = makeTile(tiles, vdp,
                       {"22222222", "21111112", "22222222", "21111112", "22222222", "21111112", "22222222", "11111111"});
    art.arch = makeTile(tiles, vdp,
                        {"77777777", "33333333", "77777777", "33333333", "11111111", "22222222", "22222222", "11111111"});
    art.niche = makeTile(tiles, vdp,
                         {"22222222", "24444442", "24444442", "24444442", "22222222", "21111112", "22222222", "11111111"});
    art.nicheLit = makeTile(tiles, vdp,
                            {"22222222", "26666642", "26666642", "24444442", "22222222", "21111112", "22222222", "11111111"});

    art.stand = gs::uploadMipped(vdp, wader(0));
    art.walkA = gs::uploadMipped(vdp, wader(1));
    art.walkB = gs::uploadMipped(vdp, wader(2));
    art.jump = gs::uploadMipped(vdp, wader(3));
    art.climbA = gs::uploadMipped(vdp, wader(4));
    art.climbB = gs::uploadMipped(vdp, wader(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.sluice = gs::uploadMipped(vdp, sluiceArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.motor = gs::uploadMipped(vdp, motorArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.weed = gs::uploadMipped(vdp, weedArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.drip[0] = gs::uploadMipped(vdp, dripArt(0));
    art.drip[1] = gs::uploadMipped(vdp, dripArt(1));
    art.vent = gs::uploadMipped(vdp, ventArt());
    art.mist = gs::uploadMipped(vdp, mistArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
}

}  // namespace cisternladd
