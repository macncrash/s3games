#include "game/art.h"

#include <string>

namespace eaves {
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

gs::Bitmap climber(int pose) {
    gs::Bitmap b(30, 38);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    const int ink = 1, coat = 2, hi = 3, skin = 4, cap = 5, scarf = 6, pant = 7, boot = 8, glove = 9, eye = 10, bag = 11;
    if (pose < 4) {
        int ls = pose == 1 ? 2 : pose == 2 ? -1 : 0;
        int rs = pose == 1 ? -1 : pose == 2 ? 2 : 0;
        int leg = pose == 3 ? 6 : 8;
        R(11 + ls, 24, 4, leg, pant);
        R(17 + rs, 24, 4, leg, pant);
        R(10 + ls, 31, 6, 3, boot);
        R(16 + rs, 32, 6, 3, boot);
        R(10, 14, 13, 12, coat);
        R(10, 14, 4, 12, hi);
        R(11, 18, 4, 6, bag);
        R(12, 20, 2, 2, 12);
        int ax = pose == 1 ? 21 : pose == 2 ? 8 : pose == 3 ? 21 : 20;
        int ay = pose == 3 ? 12 : 16;
        R(ax, ay, 4, 9, coat);
        R(ax, ay + 8, 4, 3, glove);
        R(12, 12, 10, 3, scarf);
        R(20, 13, 3, 2, scarf);
        R(13, 6, 8, 7, skin);
        R(18, 8, 2, 2, eye);
        R(18, 9, 1, 1, ink);
        R(12, 2, 11, 5, cap);
        R(11, 6, 14, 2, cap);
    } else {
        int lift = pose == 4 ? 0 : 1;
        R(13, 24, 4, 8, pant);
        R(17, 25, 4, 7, pant);
        R(12, 31, 5, 3, boot);
        R(17, 32, 5, 3, boot);
        R(12, 13, 10, 13, coat);
        R(12, 13, 3, 12, hi);
        R(lift ? 7 : 20, 7, 4, 11, coat);
        R(lift ? 20 : 7, 12, 4, 8, coat);
        R(lift ? 7 : 20, 6, 4, 3, glove);
        R(13, 11, 8, 3, scarf);
        R(13, 5, 8, 7, skin);
        R(18, 7, 2, 2, eye);
        R(12, 1, 10, 5, cap);
    }
    b.outline(ink, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(16, 32);
    b.rect(1, 0, 3, 32, 1);
    b.rect(12, 0, 3, 32, 1);
    b.rect(2, 0, 1, 32, 2);
    b.rect(13, 0, 1, 32, 2);
    for (int y = 3; y < 32; y += 8) {
        b.rect(3, y, 10, 2, 3);
        b.rect(3, y, 10, 1, 4);
    }
    return b;
}

gs::Bitmap birdArt(int flap) {
    gs::Bitmap b(22, 12);
    b.ellipse(11, 7, 6, 3, 2);
    b.ellipse(16, 6, 3, 2, 2);
    b.rect(18, 6, 3, 2, 4);
    b.set(16, 5, 5);
    if (flap == 0) {
        b.line(2, 8, 10, 6, 3, 1.6f);
        b.line(10, 6, 18, 2, 3, 1.4f);
    } else {
        b.line(2, 3, 10, 6, 3, 1.6f);
        b.line(10, 6, 18, 9, 3, 1.4f);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(34, 34);
    b.ellipse(16, 17, 13, 13, 2);
    b.ellipse(15, 16, 11, 11, 3);
    b.ellipse(12, 13, 3, 2, 1);
    b.ellipse(18, 19, 2, 2.2f, 1);
    b.ellipse(20, 12, 1.6f, 1.4f, 4);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(44, 16);
    b.ellipse(14, 9, 10, 5, 6);
    b.ellipse(24, 8, 12, 6, 6);
    b.ellipse(33, 10, 8, 4, 6);
    return b;
}

gs::Bitmap hatchArt() {
    gs::Bitmap b(22, 18);
    b.rect(1, 3, 20, 14, 1);
    b.rect(3, 5, 16, 10, 2);
    b.rect(5, 7, 12, 6, 4);
    b.rect(7, 8, 8, 4, 3);
    b.rect(0, 1, 22, 4, 2);
    b.rect(4, 2, 3, 2, 3);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(12, 16);
    b.rect(5, 0, 2, 3, 1);
    b.rect(3, 3, 6, 8, 2);
    b.rect(4, 4, 4, 6, 3);
    b.rect(5, 5, 2, 4, 4);
    b.rect(2, 11, 8, 2, 1);
    b.rect(4, 13, 4, 2, 2);
    return b;
}

gs::Bitmap chimneyArt() {
    gs::Bitmap b(18, 26);
    b.rect(4, 8, 10, 18, 1);
    b.rect(5, 8, 4, 18, 2);
    b.rect(1, 5, 16, 4, 3);
    b.rect(7, 0, 4, 6, 1);
    b.ellipse(9, 2, 3, 2, 9);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 7, 3, 1);
    b.ellipse(5, 4, 2, 1, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 8, 2, 3);
    return b;
}

gs::Bitmap streakArt() {
    gs::Bitmap b(22, 8);
    b.line(0, 4, 20, 2, 2, 1.3f);
    b.line(6, 6, 18, 4, 4, 1.1f);
    return b;
}

gs::Bitmap wireArt() {
    gs::Bitmap b(36, 14);
    b.line(0, 2, 18, 11, 2, 1.2f);
    b.line(18, 11, 35, 2, 2, 1.2f);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 0, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(9, 9, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_CITY,
           {0, gs::rgb4(4, 2, 3), gs::rgb4(8, 4, 4), gs::rgb4(11, 6, 5), gs::rgb4(3, 3, 4), gs::rgb4(15, 11, 3),
            gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 3), gs::rgb4(4, 5, 7), gs::rgb4(10, 11, 13), gs::rgb4(9, 5, 3),
            gs::rgb4(2, 7, 4), gs::rgb4(1, 4, 3), gs::rgb4(13, 10, 4), gs::rgb4(1, 1, 2), shadow});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 5, 9), gs::rgb4(7, 10, 13), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 4),
            gs::rgb4(13, 3, 3), gs::rgb4(2, 2, 5), gs::rgb4(1, 1, 1), gs::rgb4(10, 8, 6), gs::rgb4(15, 15, 15),
            gs::rgb4(8, 5, 2), gs::rgb4(13, 11, 5), gs::rgb4(10, 7, 5), 0, shadow});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(3, 3, 5), gs::rgb4(8, 8, 10), gs::rgb4(12, 12, 13), gs::rgb4(15, 15, 15), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(6, 4, 2), gs::rgb4(12, 8, 3), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 11), 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(1, 1, 2), gs::rgb4(4, 4, 6), gs::rgb4(8, 8, 11), gs::rgb4(12, 8, 4),
                           gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(7, 7, 9), gs::rgb4(12, 13, 15), gs::rgb4(1, 1, 2), gs::rgb4(10, 12, 14), 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(1, 1, 4), gs::rgb4(2, 2, 6), gs::rgb4(3, 3, 8), gs::rgb4(7, 7, 10),
                          gs::rgb4(15, 15, 13), gs::rgb4(5, 5, 9), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(7, 7, 6), gs::rgb4(11, 11, 8), gs::rgb4(14, 14, 10), gs::rgb4(15, 15, 13), 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    art.slate = makeTile(tiles, vdp, {"99999999", "89888989", "98889889", "88998898", "99888988", "88989898", "98898889", "89888998"});
    art.slateB = makeTile(tiles, vdp, {"99999999", "98898988", "88998899", "99889889", "88998898", "98988988", "89889899", "98898988"});
    art.capL = makeTile(tiles, vdp, {"19999999", "18988898", "18898989", "18989898", "18899889", "18988988", "18898998", "18988899"});
    art.capR = makeTile(tiles, vdp, {"99999991", "89889891", "98989881", "89898981", "98899881", "88988981", "89898981", "99888891"});
    art.gutter = makeTile(tiles, vdp, {"aaaaaaaa", "aaaaaaaa", "11111111", "44444444", "22224222", "22224222", "22224222", "44444444"});
    art.brick = makeTile(tiles, vdp, {"22224222", "22224222", "32224222", "44444444", "42222224", "42222234", "42222224", "44444444"});
    art.brickB = makeTile(tiles, vdp, {"22234222", "22324222", "22224232", "44444444", "42222324", "43222224", "42222224", "44444444"});
    art.window = makeTile(tiles, vdp, {"77777777", "75555557", "75565557", "75555557", "75555557", "75655557", "75555557", "77777777"});
    art.windowD = makeTile(tiles, vdp, {"77777777", "71111117", "71141117", "71111117", "71111117", "71411117", "71111117", "77777777"});
    art.ivy = makeTile(tiles, vdp, {"222242b2", "22b24222", "b2224b22", "44444444", "4b222224", "4222b224", "422222b4", "44444444"});
    art.tower = makeTile(tiles, vdp, {"11114111", "11114111", "11114111", "44444444", "41111114", "41111114", "41111141", "44444444"});
    art.star = makeTile(tiles, vdp, {"00000000", "00000000", "00005000", "00000000", "00000000", "00000000", "00000000", "00000000"});
    art.starB = makeTile(tiles, vdp, {"00000000", "00040000", "00444000", "00040000", "00000000", "00000000", "00000000", "00000000"});
    art.farWall = makeTile(tiles, vdp, {"11111111", "11111111", "11111111", "11111111", "22222222", "11111111", "11111111", "11111111"});
    art.farRoof = makeTile(tiles, vdp, {"33333333", "22222222", "11111111", "11111111", "11111111", "11111111", "11111111", "22222222"});
    art.farChim = makeTile(tiles, vdp, {"00111100", "00111100", "00111100", "01111110", "01111110", "00111100", "00111100", "00000000"});

    art.stand = gs::uploadMipped(vdp, climber(0));
    art.walkA = gs::uploadMipped(vdp, climber(1));
    art.walkB = gs::uploadMipped(vdp, climber(2));
    art.jump = gs::uploadMipped(vdp, climber(3));
    art.climbA = gs::uploadMipped(vdp, climber(4));
    art.climbB = gs::uploadMipped(vdp, climber(5));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.hatch = gs::uploadMipped(vdp, hatchArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.chimney = gs::uploadMipped(vdp, chimneyArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.streak = gs::uploadMipped(vdp, streakArt());
    art.wire = gs::uploadMipped(vdp, wireArt());
}

}  // namespace eaves
