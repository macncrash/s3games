#include "game/art.h"

#include <initializer_list>
#include <string>

namespace alleyladd {
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
    gs::Bitmap b(32, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    auto P = [&](int x, int y, int c) { b.set(x, y, c); };
    R(12, 2, 9, 3, 5);
    R(11, 4, 11, 2, 6);
    R(13, 6, 8, 6, 4);
    P(15, 8, 10);
    P(18, 8, 10);
    R(14, 11, 6, 1, 13);
    if (pose >= 4) {
        int up = pose == 4 ? 1 : 0;
        R(up ? 6 : 20, 4, 4, 12, 2);
        R(up ? 20 : 6, 16, 4, 10, 2);
        R(11, 13, 11, 16, 2);
        R(12, 26, 9, 2, 7);
        int lk = pose == 4 ? 1 : -2;
        R(12, 28, 4, 12 + lk, 12);
        R(18, 28, 4, 12 - lk, 12);
        R(11, 39, 6, 8, 8);
        R(17, 40, 6, 7, 8);
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
    R(7, 15 + la, 4, 11, 2);
    R(20, 15 + ra, 4, 11, 2);
    R(10, 13, 13, 15, 2);
    R(11, 25, 10, 2, 7);
    if (pose == 3) {
        R(12, 28, 5, 8, 12);
        R(18, 29, 5, 7, 12);
        R(11, 36, 7, 11, 8);
        R(17, 38, 6, 9, 8);
    } else {
        R(11 + ls, 28, 4, 11, 12);
        R(18 + rs, 28, 4, 11, 12);
        R(10 + ls, 39, 6, 8, 8);
        R(17 + rs, 39, 6, 8, 8);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(12, 32);
    b.rect(1, 0, 3, 32, 2);
    b.rect(8, 0, 3, 32, 2);
    b.rect(1, 0, 1, 32, 3);
    for (int y = 3; y < 32; y += 8) b.rect(2, y, 8, 2, 4);
    return b;
}

gs::Bitmap binArt() {
    gs::Bitmap b(28, 26);
    b.rect(2, 6, 24, 16, 2);
    b.rect(3, 7, 22, 14, 3);
    b.rect(4, 2, 20, 5, 4);
    b.rect(6, 0, 16, 3, 5);
    b.rect(1, 20, 26, 5, 1);
    b.rect(3, 21, 6, 4, 6);
    b.rect(19, 21, 6, 4, 6);
    b.rect(10, 10, 8, 6, 7);
    return b;
}

gs::Bitmap steamArt() {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 18, 6, 8, 2);
    b.ellipse(7, 10, 4, 6, 3);
    b.ellipse(9, 4, 3, 4, 4);
    return b;
}

gs::Bitmap lidArt() {
    gs::Bitmap b(18, 8);
    b.rect(1, 3, 16, 4, 2);
    b.rect(2, 2, 14, 2, 3);
    b.rect(7, 0, 4, 3, 4);
    return b;
}

gs::Bitmap bulbArt() {
    gs::Bitmap b(10, 16);
    b.rect(4, 10, 2, 6, 1);
    b.ellipse(5, 6, 4, 5, 3);
    b.ellipse(5, 6, 2, 3, 4);
    return b;
}

gs::Bitmap pipeArt() {
    gs::Bitmap b(10, 40);
    b.rect(3, 0, 4, 40, 2);
    b.rect(4, 0, 1, 40, 3);
    for (int y = 6; y < 38; y += 8) b.rect(2, y, 6, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 11), gs::rgb4(8, 7, 6)});
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 1));
    setPal(vdp, PAL_ALLEY, {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 3, 2), gs::rgb4(5, 2, 2), gs::rgb4(10, 8, 6),
                            gs::rgb4(3, 3, 4), gs::rgb4(12, 9, 3), gs::rgb4(4, 4, 5), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(1, 1, 1), gs::rgb4(9, 3, 2), gs::rgb4(12, 5, 3), gs::rgb4(13, 9, 6),
                           gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 2), gs::rgb4(3, 2, 2),
                           gs::rgb4(11, 10, 8), gs::rgb4(1, 1, 2), gs::rgb4(15, 12, 4), gs::rgb4(4, 3, 6),
                           gs::rgb4(10, 4, 4)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(12, 11, 8),
                           gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 2), gs::rgb4(13, 10, 3)});
    setPal(vdp, PAL_NEON, {0, gs::rgb4(1, 2, 2), gs::rgb4(2, 10, 8), gs::rgb4(6, 14, 11), gs::rgb4(12, 15, 13),
                           gs::rgb4(1, 4, 3)});
    setPal(vdp, PAL_STEAM, {0, gs::rgb4(8, 8, 9), gs::rgb4(11, 11, 12), gs::rgb4(13, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 3, 1), gs::rgb4(9, 5, 2), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(8, 4, 1), gs::rgb4(13, 7, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 4, 3), gs::rgb4(15, 8, 4)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(4, 13, 6), gs::rgb4(8, 15, 9)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(2, 2, 4), gs::rgb4(3, 3, 6), gs::rgb4(5, 5, 8), gs::rgb4(12, 10, 4),
                          gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(13, 10, 3), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 6, 6)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    art.cope = makeTile(tiles, vdp, {"44444444", "32323232", "22222222", "22222222", "22222222", "22222222", "22222222", "22222222"});
    art.brick = makeTile(tiles, vdp, {"22232223", "22232223", "33333333", "23222322", "23222322", "33333333", "22232223", "22232223"});
    art.brickB = makeTile(tiles, vdp, {"32232232", "32232232", "33333333", "22322232", "22322232", "33333333", "32232232", "32232232"});
    art.sill = makeTile(tiles, vdp, {"55555555", "66666666", "55555555", "56565656", "55555555", "65656565", "55555555", "55555555"});
    art.sillB = makeTile(tiles, vdp, {"55555555", "66666666", "55555555", "65656565", "55555555", "56565656", "55555555", "55555555"});
    art.asphalt = makeTile(tiles, vdp, {"77777777", "78777877", "77777777", "77787777", "77777777", "77877787", "77777777", "77777787"});
    art.asphaltB = makeTile(tiles, vdp, {"77777777", "77787778", "77777777", "78777777", "77777777", "77778777", "77777777", "87777777"});
    art.pit = makeTile(tiles, vdp, {"88888888", "81888188", "88888888", "88818888", "88888888", "88188818", "88888888", "88888818"});
    art.pane = makeTile(tiles, vdp, {"11111111", "15555551", "15555551", "15555551", "11111111", "15555551", "15555551", "11111111"});
    art.paneLit = makeTile(tiles, vdp, {"11111111", "16666661", "16666661", "16666661", "11111111", "16666661", "16666661", "11111111"});
    art.star = makeTile(tiles, vdp, {"00040000", "00040000", "00444000", "00040000", "00040000", "00000000", "00000000", "00000000"});
    art.starB = makeTile(tiles, vdp, {"00000000", "00400000", "00000040", "00000000", "40000000", "00004000", "00000000", "00000000"});
    art.wall = makeTile(tiles, vdp, {"22222222", "22222222", "21222122", "22222222", "22222222", "22122212", "22222222", "22222222"});
    art.cornice = makeTile(tiles, vdp, {"33333333", "22222222", "11111111", "22222222", "22222222", "22222222", "22222222", "22222222"});

    art.stand = gs::uploadMipped(vdp, hand(0));
    art.walkA = gs::uploadMipped(vdp, hand(1));
    art.walkB = gs::uploadMipped(vdp, hand(2));
    art.jump = gs::uploadMipped(vdp, hand(3));
    art.climbA = gs::uploadMipped(vdp, hand(4));
    art.climbB = gs::uploadMipped(vdp, hand(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.bin = gs::uploadMipped(vdp, binArt());
    art.steam = gs::uploadMipped(vdp, steamArt());
    art.lid = gs::uploadMipped(vdp, lidArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.pipe = gs::uploadMipped(vdp, pipeArt());
}

}  // namespace alleyladd
