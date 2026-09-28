#include "game/art.h"

#include <string>

namespace lotladd {
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

gs::Bitmap hand(int pose) {
    gs::Bitmap b(28, 42);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(9, 1, 12, 4, 5);
    R(8, 4, 14, 2, 6);
    b.set(18, 2, 11);
    R(11, 6, 8, 6, 4);
    b.set(13, 8, 10);
    b.set(16, 8, 10);
    R(10, 11, 10, 2, 7);
    if (pose >= 4) {
        int up = pose == 4 ? 1 : 0;
        R(up ? 4 : 18, 2, 4, 12, 2);
        R(up ? 5 : 19, 2, 2, 3, 9);
        R(up ? 18 : 4, 14, 4, 9, 2);
        R(10, 13, 10, 14, 2);
        R(10, 13, 3, 14, 3);
        int lk = pose == 4 ? 1 : -1;
        R(10, 27, 4, 10 + lk, 8);
        R(16, 27, 4, 10 - lk, 8);
        R(9, 36, 5, 5, 12);
        R(16, 36, 5, 5, 12);
    } else {
        int swing = pose == 1 ? 2 : (pose == 2 ? -2 : 0);
        R(5, 14 + swing, 4, 10, 2);
        R(19, 14 - swing, 4, 10, 2);
        R(6, 22 + swing, 3, 3, 9);
        R(20, 22 - swing, 3, 3, 9);
        R(10, 13, 10, 14, 2);
        R(10, 13, 3, 8, 3);
        R(17, 18, 3, 3, 11);
        int lk = pose == 1 ? 2 : (pose == 2 ? -2 : 0);
        R(10, 27, 4, 10 + lk, 8);
        R(16, 27, 4, 10 - lk, 8);
        R(9, 36, 5, 5, 12);
        R(16, 36, 5, 5, 12);
        if (pose == 3) {
            R(18, 8, 6, 3, 2);
            b.set(22, 8, 11);
        }
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(12, 28);
    b.rect(1, 0, 2, 28, 3);
    b.rect(9, 0, 2, 28, 3);
    for (int y = 2; y < 26; y += 5) b.rect(1, float(y), 10, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap carArt(int tint) {
    gs::Bitmap b(52, 26);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(4, 8, 44, 10, tint);
    R(12, 3, 22, 7, tint);
    R(14, 4, 8, 5, 4);
    R(24, 4, 8, 5, 4);
    R(6, 16, 8, 8, 6);
    R(36, 16, 8, 8, 6);
    R(8, 18, 4, 4, 7);
    R(38, 18, 4, 4, 7);
    R(2, 12, 3, 3, 8);
    R(47, 11, 3, 4, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 32);
    b.rect(4, 8, 2, 24, 3);
    b.rect(1, 1, 8, 8, 5);
    b.rect(2, 2, 6, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap tireArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 2);
    b.ellipse(9, 9, 4, 4, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 3);
    b.ellipse(11, 6, 4, 4, 0);
    b.set(5, 6, 4);
    b.set(7, 10, 4);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 22);
    b.rect(2, 2, 1, 20, 3);
    b.rect(3, 2, 11, 8, 5);
    b.rect(5, 4, 7, 2, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap officeArt() {
    gs::Bitmap b(40, 36);
    b.rect(2, 8, 36, 28, 2);
    b.rect(6, 12, 8, 8, 4);
    b.rect(26, 12, 8, 8, 4);
    b.rect(16, 22, 8, 14, 3);
    b.rect(0, 4, 40, 5, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 6);
    b.ellipse(10, 3, 9, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_HUD, {0, rgb4(15, 15, 14), rgb4(8, 8, 7), rgb4(3, 3, 3)});
    setPal(vdp, PAL_LOT, {0, rgb4(1, 1, 1), rgb4(3, 3, 3), rgb4(5, 5, 4), rgb4(12, 10, 3), rgb4(2, 2, 2)});
    setPal(vdp, PAL_HAND, {0, rgb4(1, 1, 2), rgb4(2, 3, 7), rgb4(4, 5, 11), rgb4(13, 9, 6), rgb4(1, 2, 4),
                           rgb4(14, 12, 3), rgb4(8, 6, 2), rgb4(2, 2, 3), rgb4(14, 12, 8), rgb4(1, 1, 1),
                           rgb4(15, 13, 4), rgb4(1, 1, 2)});
    setPal(vdp, PAL_IRON, {0, rgb4(1, 1, 1), rgb4(4, 4, 5), rgb4(8, 8, 9), rgb4(12, 12, 11)});
    setPal(vdp, PAL_SODIUM, {0, rgb4(1, 1, 1), rgb4(6, 4, 1), rgb4(10, 7, 2), rgb4(14, 10, 3), rgb4(15, 14, 6),
                             rgb4(15, 15, 10)});
    setPal(vdp, PAL_CAR, {0, rgb4(1, 1, 1), rgb4(11, 2, 2), rgb4(6, 1, 1), rgb4(6, 9, 12), rgb4(12, 12, 11),
                          rgb4(2, 2, 2), rgb4(9, 9, 8), rgb4(15, 14, 6)});
    setPal(vdp, PAL_SIGN, {0, rgb4(1, 1, 1), rgb4(5, 3, 1), rgb4(8, 6, 3), rgb4(12, 10, 4), rgb4(14, 3, 2),
                           rgb4(15, 14, 8)});
    setPal(vdp, PAL_FX, {0, rgb4(1, 1, 1), rgb4(12, 10, 6), rgb4(15, 12, 4)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 6, 4), rgb4(8, 1, 1), rgb4(15, 12, 4)});
    setPal(vdp, PAL_OK, {0, rgb4(6, 15, 8), rgb4(2, 8, 3), rgb4(12, 15, 8)});
    setPal(vdp, PAL_NIGHT, {0, rgb4(8, 8, 12), rgb4(2, 2, 5), rgb4(14, 13, 8), rgb4(6, 6, 8)});
    setPal(vdp, PAL_GOLD, {0, rgb4(15, 13, 5), rgb4(8, 6, 1), rgb4(15, 15, 10), rgb4(4, 3, 1)});
    setPal(vdp, PAL_DIM, {0, rgb4(8, 8, 7), rgb4(3, 3, 3)});
    for (int p = 0; p < 13; p++) vdp.setColor(p * 16 + 15, rgb4(1, 1, 1));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    art.asphalt = makeTile(tiles, vdp, {"22222222", "23332332", "22222222", "32333233", "22222222", "23322332",
                                        "22222222", "33233323"});
    art.stall = makeTile(tiles, vdp, {"22242222", "22242222", "22242222", "22242222", "22242222", "22242222",
                                      "22242222", "22242222"});
    art.star = makeTile(tiles, vdp, {"00000000", "00010000", "00111000", "00010000", "00000000", "00000000",
                                     "00000000", "00000000"});
    art.brick = makeTile(tiles, vdp, {"33333333", "32223222", "33333333", "22322232", "33333333", "32223222",
                                      "33333333", "22322232"});
    art.win = makeTile(tiles, vdp, {"33333333", "34444443", "34444443", "34444443", "34444443", "34444443",
                                    "33333333", "33333333"});

    art.stand = gs::uploadMipped(vdp, hand(0));
    art.walkA = gs::uploadMipped(vdp, hand(1));
    art.walkB = gs::uploadMipped(vdp, hand(2));
    art.jump = gs::uploadMipped(vdp, hand(3));
    art.climbA = gs::uploadMipped(vdp, hand(4));
    art.climbB = gs::uploadMipped(vdp, hand(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.car = gs::uploadMipped(vdp, carArt(2));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.tire = gs::uploadMipped(vdp, tireArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.office = gs::uploadMipped(vdp, officeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace lotladd
