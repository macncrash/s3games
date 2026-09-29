#include "game/art.h"

#include <initializer_list>
#include <string>

namespace palisadeladd {
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
            if (c >= '1' && c <= '9') v = c - '0';
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

gs::Bitmap sentry(int pose) {
    gs::Bitmap b(24, 40);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(8, 1, 8, 3, 5);
    R(7, 3, 10, 6, 5);
    R(9, 5, 2, 2, 4);
    R(13, 5, 2, 2, 4);
    R(10, 8, 4, 1, 6);
    int la = 0, ra = 0, ls = 0, rs = 0;
    if (pose == 1) {
        la = -2;
        ra = 2;
        ls = 2;
        rs = -2;
    } else if (pose == 2) {
        la = 2;
        ra = -2;
        ls = -2;
        rs = 2;
    }
    if (pose == 4 || pose == 5) {
        int lift = pose == 4 ? 0 : 2;
        R(4, 12 + lift, 4, 10, 2);
        R(16, 14, 4, 9, 2);
        R(7, 11, 10, 14, 2);
        R(8, 12, 8, 4, 3);
        R(9, 24, 3, 10, 7);
        R(13, 24, 3, 9, 7);
        R(8, 33, 5, 6, 8);
        R(13, 33, 5, 6, 8);
    } else if (pose == 3) {
        R(3, 12, 4, 9, 2);
        R(16, 10, 4, 8, 2);
        R(7, 11, 10, 12, 2);
        R(8, 12, 8, 3, 3);
        R(8, 22, 4, 8, 7);
        R(13, 23, 4, 7, 7);
        R(7, 29, 6, 4, 8);
        R(12, 30, 6, 4, 8);
    } else {
        R(3, 13 + la, 4, 10, 2);
        R(17, 13 + ra, 4, 10, 2);
        R(7, 11, 10, 14, 2);
        R(8, 12, 8, 5, 3);
        R(6, 22, 3, 4, 6);
        R(8 + ls, 24, 4, 10, 7);
        R(13 + rs, 24, 4, 10, 7);
        R(7 + ls, 33, 6, 6, 8);
        R(12 + rs, 33, 6, 6, 8);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(14, 64);
    b.rect(1, 0, 3, 64, 2);
    b.rect(10, 0, 3, 64, 2);
    b.rect(2, 0, 1, 64, 3);
    for (int y = 4; y < 62; y += 8) b.rect(2, y, 10, 2, 4);
    return b;
}

gs::Bitmap logArt() {
    gs::Bitmap b(40, 26);
    b.ellipse(20, 13, 18, 11, 2);
    b.ellipse(20, 13, 10, 7, 3);
    b.ellipse(20, 13, 4, 3, 4);
    b.rect(2, 11, 36, 2, 1);
    b.rect(4, 16, 32, 1, 1);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 10, 3, 5, 2);
    b.ellipse(5, 7, 2, 4, 3);
    b.ellipse(5, 5, 1, 2, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 16);
    b.rect(8, 0, 2, 3, 1);
    b.poly({{3, 4}, {15, 4}, {16, 12}, {2, 12}}, 3);
    b.rect(2, 12, 14, 3, 2);
    b.ellipse(9, 10, 2, 2, 4);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(22, 18);
    b.rect(1, 1, 3, 16, 1);
    b.rect(4, 2, 16, 10, 3);
    b.rect(6, 4, 12, 2, 4);
    b.poly({{4, 12}, {20, 12}, {16, 17}, {8, 17}}, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 11), gs::rgb4(6, 7, 9)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(1, 1, 2));
    setPal(vdp, PAL_SKY, {0, gs::rgb4(6, 7, 10), gs::rgb4(3, 3, 6)});
    setPal(vdp, PAL_STAKE, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5),
                            gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_HERO, {0, gs::rgb4(1, 1, 2), gs::rgb4(4, 5, 8), gs::rgb4(7, 8, 11), gs::rgb4(12, 9, 6),
                           gs::rgb4(8, 8, 9), gs::rgb4(11, 8, 3), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_LOG, {0, gs::rgb4(2, 1, 1), gs::rgb4(7, 4, 2), gs::rgb4(10, 6, 3), gs::rgb4(13, 9, 5)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(6, 2, 1), gs::rgb4(12, 5, 1), gs::rgb4(15, 10, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_LADDER, {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(14, 11, 6)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(3, 3, 4), gs::rgb4(10, 10, 8), gs::rgb4(14, 13, 8), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 6, 2)});
    vdp.setColor(PAL_GOLD * 16 + 15, gs::rgb4(3, 2, 1));
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)});
    vdp.setColor(PAL_ALERT * 16 + 15, gs::rgb4(3, 0, 0));
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 14, 8), gs::rgb4(2, 6, 3)});
    vdp.setColor(PAL_OK * 16 + 15, gs::rgb4(0, 2, 1));
    setPal(vdp, PAL_FAR, {0, gs::rgb4(2, 2, 4), gs::rgb4(4, 4, 6), gs::rgb4(3, 3, 4)});

    art.stand = gs::uploadMipped(vdp, sentry(0));
    art.walkA = gs::uploadMipped(vdp, sentry(1));
    art.walkB = gs::uploadMipped(vdp, sentry(2));
    art.jump = gs::uploadMipped(vdp, sentry(3));
    art.climbA = gs::uploadMipped(vdp, sentry(4));
    art.climbB = gs::uploadMipped(vdp, sentry(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.log = gs::uploadMipped(vdp, logArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    art.plank = makeTile(tiles, vdp, {"22222222", "34343434", "22222222", "55555555", "34343434", "22222222", "55555555", "22222222"});
    art.plankEnd = makeTile(tiles, vdp, {"12222221", "13434341", "12222221", "15555551", "13434341", "12222221", "15555551", "12222221"});
    art.stake = makeTile(tiles, vdp, {"11222211", "11233211", "11233211", "11222211", "11233211", "11233211", "11222211", "11111111"});
    art.tip = makeTile(tiles, vdp, {"00011000", "00122100", "00133100", "01233210", "01233210", "11222211", "11233211", "11222211"});
    art.star = makeTile(tiles, vdp, {"00000000", "00010000", "00000000", "00111000", "00010000", "00000000", "00000000", "00001000"});
    art.ridge = makeTile(tiles, vdp, {"00111100", "01122110", "01233210", "11222211", "11233211", "11222211", "11111111", "11111111"});
    art.ridgeTip = makeTile(tiles, vdp, {"00011000", "00122100", "01233210", "01233210", "11222211", "00000000", "00000000", "00000000"});
}

}  // namespace palisadeladd
