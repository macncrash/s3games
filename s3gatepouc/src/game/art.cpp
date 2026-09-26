#include "game/art.h"

#include <initializer_list>
#include <string>

namespace pouc {
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

void paintRoad(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int dirt = makeTile(tiles, vdp, {"22332233", "33223322", "22332232", "32243223", "23322332", "32233223",
                                     "22333222", "33222332"});
    int crust = makeTile(tiles, vdp, {"55555555", "52555255", "22332233", "33223322", "22332232", "32233223",
                                      "23322332", "32233223"});
    int pebble = makeTile(tiles, vdp, {"22332233", "33224322", "22332232", "42233223", "23322332", "32233223",
                                       "22333222", "33222342"});
    int ridge = makeTile(tiles, vdp, {"00000000", "00010000", "00111000", "01111100", "11111111", "22222222",
                                      "22222222", "33333333"});
    int star = makeTile(tiles, vdp, {"00000000", "00010000", "00000000", "00000000", "00000000", "00000000",
                                     "00000000", "00000010"});
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            vdp.A.set(cx, cy, 0);
            vdp.B.set(cx, cy, 0);
        }
    }
    for (int i = 0; i < 48; i++) {
        int cx = (i * 17) % 64;
        int cy = 1 + (i * 5) % 6;
        vdp.A.set(cx, cy, gs::entry(star, PAL_FX));
    }
    for (int cx = 0; cx < 64; cx++) {
        vdp.A.set(cx, 10, gs::entry(ridge, PAL_STONE));
        vdp.B.set(cx, 23, gs::entry(crust, PAL_EARTH));
        for (int cy = 24; cy < 32; cy++) {
            int tile = ((cx * 3 + cy) % 7 == 0) ? pebble : dirt;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_EARTH));
        }
    }
}

struct Ink {
    gs::Bitmap b;
    explicit Ink(int w, int h) : b(w, h) {}
    void r(int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }
    void p(int x, int y, int c) { b.set(x, y, c); }
    gs::Bitmap take() {
        b.outline(15, false);
        return b;
    }
};

// Green watch coat, cap, brass badge. Poses share a feet line at the bottom.
gs::Bitmap runner(int pose) {
    Ink d(40, 56);
    if (pose == 3) {
        d.r(13, 28, 14, 5, 2);
        d.r(12, 32, 16, 3, 1);
        d.p(24, 30, 7);
        d.r(14, 33, 12, 6, 4);
        d.p(22, 35, 9);
        d.r(10, 38, 20, 8, 2);
        d.r(10, 38, 4, 8, 3);
        d.r(4, 40, 8, 4, 2);
        d.r(28, 40, 8, 4, 2);
        d.r(30, 41, 4, 3, 4);
        d.r(12, 46, 7, 4, 1);
        d.r(21, 46, 7, 4, 1);
        d.r(11, 50, 9, 4, 6);
        d.r(20, 50, 9, 4, 6);
        return d.take();
    }
    int y0 = 0;
    d.r(14, 6 + y0, 12, 6, 2);
    d.r(13, 10 + y0, 14, 3, 1);
    d.p(22, 8 + y0, 7);
    d.p(23, 8 + y0, 7);
    d.r(15, 12 + y0, 11, 8, 4);
    d.p(22, 15 + y0, 9);
    d.p(23, 15 + y0, 1);
    d.r(12, 20 + y0, 16, 16, 2);
    d.r(12, 20 + y0, 4, 16, 3);
    d.r(14, 32 + y0, 12, 3, 8);
    d.p(19, 24 + y0, 7);
    d.p(19, 28 + y0, 7);
    d.p(19, 31 + y0, 7);
    if (pose == 3) {
        d.r(8, 24 + y0, 6, 4, 2);
        d.r(26, 26 + y0, 8, 4, 2);
        d.r(30, 28 + y0, 4, 3, 4);
        d.r(13, 38 + y0, 6, 6, 1);
        d.r(21, 38 + y0, 6, 6, 1);
        d.r(12, 44 + y0, 8, 4, 6);
        d.r(22, 44 + y0, 8, 4, 6);
    } else if (pose == 4) {
        d.r(8, 16 + y0, 5, 8, 2);
        d.r(28, 14 + y0, 5, 8, 2);
        d.r(30, 14 + y0, 3, 3, 4);
        d.r(14, 36 + y0, 5, 8, 1);
        d.r(22, 36 + y0, 5, 8, 1);
        d.r(13, 46 + y0, 7, 5, 6);
        d.r(21, 46 + y0, 7, 5, 6);
    } else if (pose == 1) {
        d.r(26, 22 + y0, 5, 10, 2);
        d.r(28, 30 + y0, 4, 4, 4);
        d.r(13, 36 + y0, 5, 12, 1);
        d.r(23, 38 + y0, 5, 8, 1);
        d.r(12, 47 + y0, 7, 5, 6);
        d.r(22, 45 + y0, 7, 5, 6);
    } else if (pose == 2) {
        d.r(26, 22 + y0, 5, 10, 2);
        d.r(28, 30 + y0, 4, 4, 4);
        d.r(14, 38 + y0, 5, 8, 1);
        d.r(22, 36 + y0, 5, 12, 1);
        d.r(13, 45 + y0, 7, 5, 6);
        d.r(21, 47 + y0, 7, 5, 6);
    } else {
        d.r(26, 22 + y0, 5, 12, 2);
        d.r(28, 32 + y0, 4, 4, 4);
        d.r(14, 36 + y0, 5, 12, 1);
        d.r(22, 36 + y0, 5, 12, 1);
        d.r(13, 47 + y0, 7, 5, 6);
        d.r(21, 47 + y0, 7, 5, 6);
    }
    return d.take();
}

gs::Bitmap patrolMan(int pose) {
    Ink d(36, 58);
    int bob = pose == 0 ? 0 : (pose == 1 ? 0 : 1);
    d.r(12, 4 + bob, 12, 5, 5);
    d.r(11, 8 + bob, 14, 3, 6);
    d.p(20, 6, 7);
    d.r(13, 11 + bob, 10, 7, 4);
    d.p(20, 14 + bob, 1);
    d.r(10, 18 + bob, 16, 16, 2);
    d.r(10, 18 + bob, 4, 16, 3);
    d.r(12, 30 + bob, 12, 3, 1);
    d.r(22, 20 + bob, 8, 4, 2);
    d.r(28, 16 + bob, 5, 6, 7);
    d.r(29, 17 + bob, 3, 3, 8);
    if (pose == 0) {
        d.r(12, 34, 5, 14, 1);
        d.r(19, 34, 5, 14, 1);
        d.r(11, 50, 7, 6, 9);
        d.r(18, 50, 7, 6, 9);
    } else if (pose == 1) {
        d.r(11, 34, 5, 14, 1);
        d.r(20, 36, 5, 10, 1);
        d.r(10, 50, 7, 6, 9);
        d.r(19, 48, 7, 6, 9);
    } else {
        d.r(13, 36, 5, 10, 1);
        d.r(19, 34, 5, 14, 1);
        d.r(12, 48, 7, 6, 9);
        d.r(18, 50, 7, 6, 9);
    }
    return d.take();
}

gs::Bitmap pouchBag(int swing) {
    Ink d(28, 24);
    int s = swing ? 1 : 0;
    d.r(6, 6 + s, 16, 14, 2);
    d.r(6, 6 + s, 16, 3, 3);
    d.r(8, 4 + s, 12, 4, 1);
    d.r(11, 3 + s, 6, 3, 4);
    d.p(14, 4 + s, 6);
    d.r(8, 12 + s, 12, 1, 5);
    d.r(18, 10 + s, 4, 5, 6);
    d.p(19, 12 + s, 4);
    d.r(7, 8 + s, 2, 8, 5);
    return d.take();
}

gs::Bitmap stoolArt() {
    Ink d(22, 16);
    d.r(3, 2, 16, 4, 2);
    d.r(3, 2, 16, 2, 3);
    d.r(4, 6, 3, 8, 1);
    d.r(15, 6, 3, 8, 1);
    d.r(6, 9, 10, 2, 2);
    return d.take();
}

gs::Bitmap towerArt() {
    Ink d(44, 112);
    d.r(8, 18, 28, 90, 2);
    d.r(30, 18, 6, 90, 1);
    d.r(8, 18, 4, 90, 3);
    for (int i = 0; i < 7; i++) d.r(8, 28 + i * 11, 28, 2, 4);
    d.r(4, 8, 36, 12, 2);
    d.r(2, 4, 10, 10, 3);
    d.r(16, 2, 12, 12, 3);
    d.r(32, 4, 10, 10, 1);
    d.r(6, 0, 6, 6, 2);
    d.r(18, 0, 8, 4, 2);
    d.r(34, 0, 6, 6, 2);
    d.r(16, 48, 10, 16, 6);
    d.r(18, 50, 6, 8, 7);
    d.r(14, 96, 16, 12, 1);
    d.r(12, 70, 8, 10, 5);
    return d.take();
}

gs::Bitmap archArt() {
    Ink d(88, 36);
    d.r(2, 4, 84, 10, 2);
    d.r(2, 4, 84, 3, 3);
    d.r(2, 12, 84, 3, 1);
    for (int i = 0; i < 6; i++) d.r(8 + i * 13, 6, 4, 4, 4);
    d.r(8, 16, 8, 18, 2);
    d.r(72, 16, 8, 18, 2);
    d.r(16, 22, 56, 4, 7);
    return d.take();
}

gs::Bitmap doorArt() {
    Ink d(28, 48);
    d.r(4, 4, 20, 40, 2);
    d.r(4, 4, 20, 4, 3);
    d.r(6, 12, 16, 14, 1);
    d.r(6, 28, 16, 12, 1);
    d.r(18, 24, 3, 3, 4);
    d.r(8, 8, 4, 2, 5);
    return d.take();
}

gs::Bitmap lampArt() {
    Ink d(16, 36);
    d.r(6, 14, 4, 20, 1);
    d.r(4, 8, 8, 8, 2);
    d.r(5, 9, 6, 4, 3);
    d.r(2, 6, 12, 3, 4);
    return d.take();
}

gs::Bitmap flameArt(int hot) {
    Ink d(10, 12);
    d.r(3, 4, 4, 6, hot ? 3 : 4);
    d.r(4, 2, 2, 4, 1);
    d.p(4, 6, 8);
    return d.take();
}

gs::Bitmap postArt() {
    Ink d(26, 78);
    d.r(10, 8, 8, 66, 2);
    d.r(10, 8, 3, 66, 3);
    d.r(4, 4, 18, 8, 4);
    d.r(6, 2, 14, 4, 7);
    d.r(8, 18, 10, 3, 1);
    d.r(8, 40, 10, 3, 1);
    return d.take();
}

gs::Bitmap bellArt() {
    Ink d(14, 14);
    d.r(3, 4, 8, 7, 4);
    d.r(4, 5, 6, 3, 3);
    d.p(6, 2, 2);
    d.p(6, 11, 7);
    return d.take();
}

// Far-post receiver. Shut keeps the pouch out. Open is the only handoff.
gs::Bitmap guardArt(bool open) {
    Ink d(48, 58);
    d.r(18, 4, 12, 5, 5);
    d.r(17, 8, 14, 3, 6);
    d.r(19, 11, 10, 7, 4);
    d.p(26, 14, 1);
    d.r(16, 18, 16, 16, 2);
    d.r(16, 18, 4, 16, 3);
    d.r(18, 30, 12, 3, 8);
    d.p(23, 8, 7);
    d.r(18, 34, 5, 14, 1);
    d.r(25, 34, 5, 14, 1);
    d.r(17, 47, 7, 5, 6);
    d.r(24, 47, 7, 5, 6);
    if (open) {
        d.r(2, 20, 14, 5, 2);
        d.r(32, 20, 14, 5, 2);
        d.r(1, 21, 4, 4, 4);
        d.r(43, 21, 4, 4, 4);
    } else {
        d.r(10, 22, 6, 5, 2);
        d.r(32, 22, 6, 5, 2);
        d.r(10, 24, 4, 3, 4);
        d.r(34, 24, 4, 3, 4);
    }
    return d.take();
}

gs::Bitmap barArt() {
    Ink d(64, 14);
    d.r(2, 3, 60, 8, 4);
    d.r(2, 3, 60, 2, 3);
    d.r(2, 9, 60, 2, 1);
    for (int i = 0; i < 5; i++) d.r(6 + i * 11, 4, 3, 6, 7);
    return d.take();
}

gs::Bitmap beamPostArt() {
    Ink d(12, 70);
    d.r(3, 4, 6, 62, 2);
    d.r(3, 4, 2, 62, 3);
    d.r(1, 2, 10, 6, 4);
    d.r(2, 60, 8, 6, 1);
    return d.take();
}

gs::Bitmap holeArt() {
    gs::Bitmap b(16, 16);
    b.rect(0, 0, 16, 16, 1);
    b.rect(0, 0, 16, 3, 7);
    return b;
}

gs::Bitmap lipArt() {
    Ink d(18, 20);
    d.r(2, 4, 14, 8, 2);
    d.r(2, 4, 14, 3, 3);
    d.r(2, 12, 14, 6, 1);
    d.r(4, 8, 3, 3, 4);
    d.r(10, 7, 3, 3, 5);
    return d.take();
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(10, 11, 8, 8, 1);
    b.ellipse(14, 9, 7, 7, 0);
    b.set(7, 12, 2);
    b.set(8, 15, 2);
    b.set(6, 14, 2);
    return b;
}

gs::Bitmap stoneArt() {
    Ink d(12, 8);
    d.r(2, 2, 8, 4, 4);
    d.r(2, 2, 8, 2, 3);
    return d.take();
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 8, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(14, 11, 4), gs::rgb4(6, 6, 8), gs::rgb4(14, 4, 3),
                          gs::rgb4(4, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(1, 1, 3), gs::rgb4(5, 6, 8), gs::rgb4(9, 10, 12), gs::rgb4(3, 3, 5),
                            gs::rgb4(3, 5, 4), gs::rgb4(12, 9, 4), gs::rgb4(2, 2, 4), 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_EARTH, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(5, 5, 4),
                            gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_POUCH, {0, gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 2), gs::rgb4(12, 8, 3), gs::rgb4(13, 10, 4),
                            gs::rgb4(3, 2, 1), gs::rgb4(14, 13, 10), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(1, 2, 1), gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 5), gs::rgb4(13, 9, 6),
                             gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2), gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 2),
                             gs::rgb4(14, 14, 11), 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PATROL, {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 2, 2), gs::rgb4(11, 4, 3), gs::rgb4(13, 9, 6),
                             gs::rgb4(4, 4, 6), gs::rgb4(8, 8, 10), gs::rgb4(15, 12, 5), gs::rgb4(15, 15, 12),
                             gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, gs::rgb4(1, 0, 0)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(4, 4, 5),
                           gs::rgb4(3, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 14, 11), gs::rgb4(8, 8, 12), gs::rgb4(15, 12, 5), gs::rgb4(15, 8, 2),
                         0, 0, 0, gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(6, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(1, 2, 3), gs::rgb4(2, 6, 8), gs::rgb4(4, 9, 11), gs::rgb4(13, 9, 6),
                           gs::rgb4(3, 3, 5), gs::rgb4(7, 7, 9), gs::rgb4(13, 10, 4), gs::rgb4(5, 4, 2),
                           0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    paintRoad(vdp, tiles);

    art.stand = gs::uploadMipped(vdp, runner(0));
    art.runA = gs::uploadMipped(vdp, runner(1));
    art.runB = gs::uploadMipped(vdp, runner(2));
    art.duck = gs::uploadMipped(vdp, runner(3));
    art.leap = gs::uploadMipped(vdp, runner(4));
    for (int i = 0; i < 3; i++) art.patrol[i] = gs::uploadMipped(vdp, patrolMan(i));
    art.pouch[0] = gs::uploadMipped(vdp, pouchBag(0));
    art.pouch[1] = gs::uploadMipped(vdp, pouchBag(1));
    art.stool = gs::uploadMipped(vdp, stoolArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.post = gs::uploadMipped(vdp, postArt());
    art.guardShut = gs::uploadMipped(vdp, guardArt(false));
    art.guardOpen = gs::uploadMipped(vdp, guardArt(true));
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.beamPost = gs::uploadMipped(vdp, beamPostArt());
    art.hole = gs::uploadMipped(vdp, holeArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace pouc
