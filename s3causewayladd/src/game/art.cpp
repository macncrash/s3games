#include "game/art.h"

#include <initializer_list>
#include <string>

namespace causewayladd {
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

gs::Bitmap walker(int pose) {
    gs::Bitmap b(24, 40);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(9, 1, 7, 6, 5);
    R(10, 3, 2, 2, 4);
    R(14, 3, 2, 2, 4);
    R(8, 7, 9, 3, 4);
    int ls = 0, rs = 0, la = 0, ra = 0;
    if (pose == 1) {
        ls = 2;
        rs = -2;
        la = -2;
        ra = 2;
    } else if (pose == 2) {
        ls = -2;
        rs = 2;
        la = 2;
        ra = -2;
    }
    if (pose == 4 || pose == 5) {
        R(6, 10, 3, 10, 2);
        R(16, 12, 3, 9, 3);
        R(8, 10, 9, 12, 2);
        R(9, 18, 7, 2, 7);
        int k = pose == 4 ? 0 : 2;
        R(8, 22, 4, 12, 6);
        R(13, 22 + k, 4, 12 - k, 6);
        R(7, 33, 6, 5, 1);
        R(13, 34, 6, 4, 1);
    } else if (pose == 3) {
        R(7, 11, 3, 8, 2);
        R(15, 12, 3, 8, 3);
        R(8, 10, 9, 11, 2);
        R(9, 28, 4, 6, 6);
        R(14, 26, 4, 5, 6);
        R(8, 34, 6, 5, 1);
        R(14, 31, 6, 5, 1);
    } else {
        R(5, 12 + la, 3, 9, 2);
        R(16, 12 + ra, 3, 9, 3);
        R(8, 10, 9, 12, 2);
        R(9, 19, 7, 2, 7);
        R(8 + ls, 22, 4, 11, 6);
        R(13 + rs, 22, 4, 11, 6);
        R(7 + ls, 33, 6, 5, 1);
        R(13 + rs, 33, 6, 5, 1);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(14, 32);
    b.rect(1, 0, 3, 32, 2);
    b.rect(10, 0, 3, 32, 3);
    for (int y = 2; y < 32; y += 8) b.rect(2, y, 10, 2, 4);
    return b;
}

gs::Bitmap gullArt(int flap) {
    gs::Bitmap b(28, 12);
    int y = flap ? 3 : 5;
    b.line(2, y + 3, 12, 6, 2, 2);
    b.line(26, y + 3, 16, 6, 2, 2);
    b.ellipse(14, 7, 3, 2, 3);
    b.set(15, 6, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 18);
    b.rect(4, 8, 2, 10, 1);
    b.ellipse(5, 5, 4, 4, 3);
    b.ellipse(5, 5, 2, 2, 4);
    return b;
}

gs::Bitmap beaconArt() {
    gs::Bitmap b(20, 28);
    b.rect(8, 10, 4, 18, 1);
    b.rect(4, 6, 12, 6, 2);
    b.rect(6, 2, 8, 5, 4);
    b.rect(8, 0, 4, 3, 5);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 4, 2, 24, 2);
    b.rect(1, 2, 6, 3, 3);
    b.rect(2, 0, 4, 2, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 7), gs::rgb4(10, 10, 9), gs::rgb4(13, 12, 10), gs::rgb4(5, 6, 6),
            gs::rgb4(2, 3, 4), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_SEA,
           {0, gs::rgb4(1, 3, 6), gs::rgb4(2, 6, 9), gs::rgb4(3, 8, 11), gs::rgb4(8, 12, 13), gs::rgb4(12, 14, 14),
            gs::rgb4(1, 4, 5)});
    setPal(vdp, PAL_COAT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 3, 7), gs::rgb4(4, 6, 11), gs::rgb4(13, 9, 6), gs::rgb4(4, 2, 1),
            gs::rgb4(2, 2, 3), gs::rgb4(12, 9, 3)});
    setPal(vdp, PAL_RUST,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 4, 2), gs::rgb4(11, 6, 3), gs::rgb4(14, 10, 4), gs::rgb4(6, 3, 2)});
    setPal(vdp, PAL_GOLD,
           {0, gs::rgb4(4, 3, 1), gs::rgb4(10, 8, 2), gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 8), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 5), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(2, 2, 3), gs::rgb4(13, 13, 14), gs::rgb4(8, 8, 9), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(8, 2, 2), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 14, 8), gs::rgb4(2, 6, 3), gs::rgb4(12, 15, 10)});
    setPal(vdp, PAL_FAR,
           {0, gs::rgb4(4, 5, 8), gs::rgb4(6, 7, 10), gs::rgb4(9, 9, 11), gs::rgb4(12, 12, 13), gs::rgb4(3, 4, 6)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(10, 13, 14), gs::rgb4(14, 15, 15), gs::rgb4(6, 10, 12)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 8, 8), gs::rgb4(4, 5, 5), gs::rgb4(10, 11, 10)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    art.stand = gs::uploadMipped(vdp, walker(0));
    art.walkA = gs::uploadMipped(vdp, walker(1));
    art.walkB = gs::uploadMipped(vdp, walker(2));
    art.jump = gs::uploadMipped(vdp, walker(3));
    art.climbA = gs::uploadMipped(vdp, walker(4));
    art.climbB = gs::uploadMipped(vdp, walker(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.gull = gs::uploadMipped(vdp, gullArt(0));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.beacon = gs::uploadMipped(vdp, beaconArt());
    art.post = gs::uploadMipped(vdp, postArt());

    art.sky = makeTile(tiles, vdp, {"00000000", "00000000", "00010000", "00000000", "00000000", "00000000", "00000010", "00000000"});
    art.haze = makeTile(tiles, vdp, {"22222222", "22232222", "22222222", "23222222", "22222232", "22222222", "22223222", "22222222"});
    art.stone = makeTile(tiles, vdp, {"33333333", "32222332", "32222332", "33333333", "23333223", "23333223", "23333223", "33333333"});
    art.stoneB = makeTile(tiles, vdp, {"33333333", "23332223", "23332223", "33333333", "32233332", "32233332", "32233332", "33333333"});
    art.cap = makeTile(tiles, vdp, {"44444444", "34444443", "33333333", "32222332", "32222332", "33333333", "23333223", "33333333"});
    art.joint = makeTile(tiles, vdp, {"44444444", "41111114", "33333333", "32222332", "33333333", "23333223", "33333333", "11111111"});
    art.pile = makeTile(tiles, vdp, {"00110000", "00122000", "00122000", "00122000", "00122000", "00122000", "01122100", "01122100"});
    art.water = makeTile(tiles, vdp, {"22222222", "22322232", "22222222", "23222322", "22232222", "22222223", "32222222", "22223222"});
    art.waterB = makeTile(tiles, vdp, {"23222223", "22232222", "22222232", "22322222", "22223222", "32222223", "22222322", "23222222"});
    art.foam = makeTile(tiles, vdp, {"45444544", "22253222", "22322232", "22222222", "23222223", "22232222", "22223222", "32222232"});
    art.rail = makeTile(tiles, vdp, {"00055000", "00055000", "00033000", "00033000", "00033000", "00022000", "00022000", "00000000"});
}

}  // namespace causewayladd
