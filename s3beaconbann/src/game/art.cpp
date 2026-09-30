#include "game/art.h"

#include <string>

namespace beaconbann {
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

gs::Bitmap keeper(int pose, int coat, int trim, int cap) {
    gs::Bitmap b(30, 46);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    int bob = pose == 2 ? 1 : 0;
    R(10, 4 + bob, 10, 6, 4);
    R(9, 1 + bob, 12, 4, cap);
    R(18, 5 + bob, 2, 2, 9);
    R(7, 10 + bob, 15, 13, coat);
    R(7, 10 + bob, 3, 13, trim);
    R(11, 18 + bob, 6, 2, 5);
    if (pose == 1) {
        R(8, 23, 5, 14, coat);
        R(16, 25, 5, 12, coat);
        R(7, 36, 7, 4, 6);
        R(15, 36, 7, 4, 6);
        R(21, 12, 3, 10, 4);
    } else if (pose == 2) {
        R(7, 25, 5, 12, coat);
        R(17, 23, 5, 14, coat);
        R(6, 36, 7, 4, 6);
        R(16, 36, 7, 4, 6);
        R(21, 13, 3, 10, 4);
    } else if (pose == 3) {
        R(10, 23, 4, 14, coat);
        R(16, 23, 4, 14, coat);
        R(9, 36, 6, 4, 6);
        R(16, 36, 6, 4, 6);
        R(20, 14, 8, 3, coat);
        R(26, 12, 3, 4, 7);
    } else {
        R(10, 23, 4, 14, coat);
        R(16, 23, 4, 14, coat);
        R(9, 36, 6, 4, 6);
        R(16, 36, 6, 4, 6);
        R(21, 12, 3, 10, 4);
    }
    return b;
}

gs::Bitmap flag() {
    gs::Bitmap b(30, 36);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(4, 2, 22, 5, 6);
    R(6, 7, 18, 20, 2);
    R(6, 7, 5, 20, 3);
    R(14, 11, 5, 12, 4);
    R(8, 27, 3, 6, 2);
    R(14, 28, 3, 5, 2);
    R(19, 27, 3, 6, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 15, 13), gs::rgb4(1, 2, 4), gs::rgb4(15, 12, 4),
                          gs::rgb4(12, 3, 2), gs::rgb4(6, 8, 9), gs::rgb4(0, 1, 2)});
    setPal(vdp, PAL_SEA, {gs::rgb4(0, 0, 0), gs::rgb4(1, 3, 6), gs::rgb4(2, 5, 8), gs::rgb4(3, 6, 9),
                          gs::rgb4(6, 9, 11), gs::rgb4(10, 12, 12), gs::rgb4(0, 2, 3), gs::rgb4(4, 7, 8)});
    setPal(vdp, PAL_COAT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(2, 4, 7), gs::rgb4(3, 6, 9),
                           gs::rgb4(12, 10, 7), gs::rgb4(8, 2, 2), gs::rgb4(2, 2, 1), gs::rgb4(14, 13, 9),
                           gs::rgb4(15, 14, 8), gs::rgb4(0, 2, 3)});
    setPal(vdp, PAL_BANNER, {gs::rgb4(0, 0, 0), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                             gs::rgb4(15, 14, 12), gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_KEEPER, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 1), gs::rgb4(3, 4, 2), gs::rgb4(6, 7, 3),
                             gs::rgb4(10, 8, 4), gs::rgb4(12, 9, 2), gs::rgb4(2, 2, 1), gs::rgb4(14, 12, 5),
                             gs::rgb4(15, 11, 3), gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_STONE, {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 5), gs::rgb4(7, 7, 6),
                            gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 3), gs::rgb4(11, 10, 8)});
    setPal(vdp, PAL_LAMP, {gs::rgb4(0, 0, 0), gs::rgb4(4, 3, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 6),
                           gs::rgb4(8, 5, 1), gs::rgb4(2, 1, 0), gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_NIGHT, {gs::rgb4(0, 0, 0), gs::rgb4(0, 1, 3), gs::rgb4(1, 2, 5), gs::rgb4(2, 3, 6),
                            gs::rgb4(8, 9, 11), gs::rgb4(13, 13, 12)});

    loadFont(vdp, art, tiles);

    int swell = makeTile(tiles, vdp, {"00011222", "00112233", "11223344", "22334411", "33441122", "44112233",
                                      "11223344", "00112233"});
    int foam = makeTile(tiles, vdp, {"55555555", "44554455", "33443344", "22332233", "11221122", "00110011",
                                     "00000000", "00000000"});
    int path = makeTile(tiles, vdp, {"22332233", "33223322", "66776677", "22332233", "33223322", "77667766",
                                     "22332233", "33223322"});
    int grit = makeTile(tiles, vdp, {"33223322", "22332233", "66776677", "33223322", "22332233", "77667766",
                                     "33223322", "22332233"});
    int cliff = makeTile(tiles, vdp, {"11112222", "11222233", "22233311", "23331122", "33112233", "11223311",
                                      "22331122", "33112233"});

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    for (int cy = 16; cy < 20; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = (cy == 16) ? foam : swell;
            vdp.A.set(cx, cy, gs::entry(tile, PAL_SEA));
        }
    }
    for (int cy = 20; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = cy < 22 ? cliff : (((cx + cy) & 1) ? path : grit);
            vdp.B.set(cx, cy, gs::entry(tile, PAL_STONE));
        }
    }

    art.stand = gs::uploadMipped(vdp, keeper(0, 2, 3, 7));
    art.walkA = gs::uploadMipped(vdp, keeper(1, 2, 3, 7));
    art.walkB = gs::uploadMipped(vdp, keeper(2, 2, 3, 7));
    art.shove = gs::uploadMipped(vdp, keeper(3, 2, 3, 7));
    art.keeper[0] = gs::uploadMipped(vdp, keeper(0, 2, 5, 6));
    art.keeper[1] = gs::uploadMipped(vdp, keeper(1, 2, 5, 6));
    art.banner = gs::uploadMipped(vdp, flag());

    {
        gs::Bitmap b(10, 58);
        b.rect(4, 0, 2, 58, 6);
        b.rect(2, 0, 6, 4, 7);
        art.pole = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 92);
        b.rect(8, 18, 12, 74, 2);
        b.rect(6, 16, 16, 6, 3);
        b.rect(10, 8, 8, 10, 4);
        b.rect(9, 4, 10, 6, 7);
        b.rect(11, 0, 6, 5, 3);
        b.rect(11, 36, 6, 8, 1);
        b.rect(11, 56, 6, 8, 1);
        art.tower = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(16, 16);
        b.ellipse(8, 8, 7, 7, 3);
        b.ellipse(8, 8, 4, 4, 2);
        art.lamp = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(26, 16);
        b.ellipse(13, 10, 12, 6, 2);
        b.ellipse(8, 9, 5, 4, 3);
        b.rect(4, 12, 18, 3, 5);
        art.rock = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 20);
        b.rect(2, 4, 2, 16, 4);
        b.rect(18, 4, 2, 16, 4);
        b.rect(2, 6, 18, 2, 6);
        b.rect(2, 12, 18, 2, 6);
        art.rail = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 8);
        b.ellipse(11, 4, 10, 3, 1);
        art.shadow = gs::uploadMipped(vdp, b);
    }
    vdp.setFogColor(gs::rgb4(1, 2, 4));
}

}  // namespace beaconbann
