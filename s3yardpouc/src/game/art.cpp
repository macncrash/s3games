#include "game/art.h"

#include <initializer_list>
#include <string>

namespace yardpouc {
namespace {

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(p * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(p * 16 + i++, 0);
    if (i == 15) vdp.setColor(p * 16 + 15, gs::rgb4(1, 1, 1));
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

void paintYard(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int wall = makeTile(tiles, vdp, {"11111111", "11611161", "11111111", "11111111", "22222222", "11161111",
                                     "11111111", "11111111"});
    int roof = makeTile(tiles, vdp, {"00000000", "00033000", "00333300", "33333333", "55555555", "22222222",
                                     "00000000", "00000000"});
    int star = makeTile(tiles, vdp, {"00000000", "00060000", "00000000", "00000000", "00000000", "00000000",
                                     "00000000", "00000060"});
    int gravel = makeTile(tiles, vdp, {"11211121", "12111211", "21112112", "11211131", "12121112", "11121211",
                                       "21111212", "12111121"});
    int gravelB = makeTile(tiles, vdp, {"21111211", "11212121", "12111211", "21121112", "11211121", "12131121",
                                        "11121211", "21211112"});
    int railA = makeTile(tiles, vdp, {"11211121", "77777777", "11211121", "44444444", "11211121", "12121112",
                                      "21111211", "12111211"});
    int railB = makeTile(tiles, vdp, {"12111211", "77777777", "21112112", "11211121", "12121112", "11212111",
                                      "21111212", "12111121"});
    vdp.A.clear();
    vdp.B.clear();
    for (int i = 0; i < 48; i++) {
        int cx = (i * 9 + 3) % 64;
        int cy = (i * 3) % 8;
        vdp.A.set(cx, cy, gs::entry(star, PAL_FX));
    }
    for (int cx = 0; cx < 64; cx++) {
        int n = (cx * 5 + 2) % 11;
        if (n < 6) {
            vdp.A.set(cx, 16, gs::entry(wall, PAL_IRON));
            vdp.A.set(cx, 15, gs::entry(wall, PAL_IRON));
            vdp.A.set(cx, 14, gs::entry(roof, PAL_IRON));
            if (n < 2) vdp.A.set(cx, 13, gs::entry(roof, PAL_IRON));
        }
        vdp.B.set(cx, 22, gs::entry((cx & 1) ? railB : railA, PAL_EARTH));
        for (int cy = 23; cy < 28; cy++) {
            int tile = ((cx + cy * 3) & 1) ? gravelB : gravel;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_EARTH));
        }
    }
}

struct Ink {
    gs::Bitmap b;
    explicit Ink(int w, int h) : b(w, h) {}
    void r(int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); }
    void p(int x, int y, int c) { b.set(x, y, c); }
    void el(float cx, float cy, float rx, float ry, int c) { b.ellipse(cx, cy, rx, ry, c); }
    void ln(float x0, float y0, float x1, float y1, int c, float t = 1.f) { b.line(x0, y0, x1, y1, c, t); }
    gs::Bitmap take(bool edge = true) {
        if (edge) b.outline(15, false);
        return b;
    }
};

gs::Mipped up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadMipped(vdp, b); }

// Cap, ochre vest, navy shirt. Feet sit on the bottom row of every pose.
gs::Bitmap hand(int pose) {
    Ink d(40, 56);
    if (pose == 3) {
        d.r(12, 24, 16, 4, 4);
        d.r(22, 27, 8, 2, 3);
        d.r(14, 28, 12, 6, 1);
        d.r(14, 28, 3, 5, 6);
        d.p(22, 30, 7);
        d.p(23, 30, 7);
        d.r(10, 34, 20, 10, 2);
        d.r(16, 35, 12, 8, 3);
        d.p(20, 38, 9);
        d.r(6, 38, 6, 4, 2);
        d.r(28, 40, 6, 3, 8);
        d.r(12, 44, 7, 6, 2);
        d.r(21, 44, 7, 6, 2);
        d.r(11, 50, 8, 5, 5);
        d.r(21, 50, 8, 5, 5);
        return d.take();
    }
    int y = 0;
    d.r(14, 3 + y, 13, 5, 4);
    d.r(13, 7 + y, 16, 3, 4);
    d.r(24, 8 + y, 7, 2, 3);
    d.r(15, 10 + y, 11, 8, 1);
    d.r(15, 10 + y, 3, 7, 6);
    d.p(22, 13 + y, 7);
    d.p(23, 13 + y, 7);
    d.r(19, 16 + y, 4, 1, 6);
    d.r(12, 18 + y, 16, 16, 2);
    d.r(18, 19 + y, 9, 13, 3);
    d.p(21, 23 + y, 9);
    d.p(21, 27 + y, 9);
    d.r(12, 31 + y, 16, 3, 8);
    d.p(25, 32 + y, 9);
    if (pose == 4) {
        d.r(8, 8 + y, 5, 12, 2);
        d.r(27, 8 + y, 5, 12, 2);
        d.r(8, 8 + y, 4, 3, 8);
        d.r(28, 8 + y, 4, 3, 8);
        d.r(14, 36 + y, 5, 10, 2);
        d.r(22, 36 + y, 5, 10, 2);
        d.r(13, 48 + y, 7, 6, 5);
        d.r(21, 48 + y, 7, 6, 5);
    } else if (pose == 1) {
        d.r(27, 20 + y, 5, 12, 2);
        d.r(29, 30 + y, 4, 3, 8);
        d.r(13, 34 + y, 5, 14, 2);
        d.r(22, 36 + y, 5, 10, 2);
        d.r(12, 48 + y, 7, 6, 5);
        d.r(21, 46 + y, 7, 6, 5);
    } else if (pose == 2) {
        d.r(27, 20 + y, 5, 12, 2);
        d.r(29, 30 + y, 4, 3, 8);
        d.r(14, 36 + y, 5, 10, 2);
        d.r(22, 34 + y, 5, 14, 2);
        d.r(13, 46 + y, 7, 6, 5);
        d.r(21, 48 + y, 7, 6, 5);
    } else {
        d.r(27, 20 + y, 5, 14, 2);
        d.r(29, 32 + y, 4, 3, 8);
        d.r(14, 34 + y, 5, 14, 2);
        d.r(22, 34 + y, 5, 14, 2);
        d.r(13, 48 + y, 7, 6, 5);
        d.r(21, 48 + y, 7, 6, 5);
    }
    return d.take();
}

gs::Bitmap satchel(int swing) {
    Ink d(30, 26);
    int s = swing ? 2 : 0;
    d.r(7, 8 + s, 16, 14, 1);
    d.r(7, 8 + s, 16, 4, 2);
    d.r(9, 5 + s, 12, 5, 2);
    d.r(12, 3 + s, 6, 4, 3);
    d.p(14, 5 + s, 4);
    d.p(15, 5 + s, 4);
    d.r(10, 14 + s, 10, 1, 5);
    d.el(18, 16 + s, 3, 3, 6);
    d.r(6, 11 + s, 3, 8, 3);
    d.p(11, 12 + s, 7);
    return d.take();
}

gs::Bitmap crateArt() {
    Ink d(32, 22);
    d.r(3, 4, 26, 14, 1);
    d.r(3, 4, 26, 3, 3);
    d.r(3, 10, 26, 2, 2);
    d.r(3, 15, 26, 3, 3);
    d.r(8, 4, 2, 14, 2);
    d.r(22, 4, 2, 14, 2);
    d.p(6, 7, 5);
    d.p(26, 7, 5);
    d.p(6, 16, 5);
    d.p(26, 16, 5);
    return d.take();
}

gs::Bitmap drumArt() {
    Ink d(28, 28);
    d.el(14, 14, 11, 11, 2);
    d.el(14, 14, 7, 7, 1);
    d.el(14, 14, 3, 3, 4);
    d.r(4, 12, 20, 2, 3);
    d.r(4, 16, 20, 2, 3);
    return d.take();
}

gs::Bitmap barrelArt() {
    Ink d(18, 24);
    d.r(3, 3, 12, 18, 1);
    d.r(3, 3, 12, 3, 3);
    d.r(3, 16, 12, 3, 2);
    d.r(2, 7, 14, 2, 4);
    d.r(2, 13, 14, 2, 4);
    d.el(9, 6, 3, 2, 5);
    return d.take();
}

gs::Bitmap plankArt() {
    Ink d(24, 12);
    d.r(1, 2, 22, 8, 1);
    d.r(1, 2, 22, 2, 3);
    d.r(6, 2, 2, 8, 2);
    d.r(14, 2, 2, 8, 2);
    d.p(4, 6, 5);
    d.p(18, 6, 5);
    return d.take();
}

gs::Bitmap boxcarArt() {
    Ink d(104, 58);
    d.r(6, 6, 92, 6, 3);
    d.r(8, 4, 88, 3, 9);
    d.r(4, 12, 96, 28, 1);
    d.r(4, 12, 8, 28, 2);
    d.r(84, 12, 16, 28, 2);
    d.r(8, 22, 80, 5, 7);
    d.r(40, 16, 24, 22, 4);
    d.r(40, 16, 3, 22, 2);
    d.r(61, 16, 3, 22, 2);
    d.r(48, 22, 8, 8, 8);
    d.r(6, 38, 92, 6, 9);
    d.el(24, 46, 8, 8, 5);
    d.el(24, 46, 3, 3, 7);
    d.el(78, 46, 8, 8, 5);
    d.el(78, 46, 3, 3, 7);
    d.r(2, 28, 4, 6, 9);
    d.r(96, 18, 3, 16, 6);
    d.p(18, 26, 7);
    d.p(22, 26, 7);
    d.p(28, 26, 7);
    return d.take();
}

gs::Bitmap hookArt() {
    Ink d(16, 28);
    d.r(6, 1, 4, 10, 1);
    d.r(6, 9, 8, 3, 4);
    d.r(10, 11, 4, 10, 1);
    d.r(4, 17, 10, 4, 2);
    d.r(4, 15, 4, 6, 1);
    d.p(7, 4, 4);
    return d.take();
}

gs::Bitmap chainArt() {
    Ink d(6, 16);
    d.r(1, 1, 4, 4, 1);
    d.r(1, 6, 4, 4, 4);
    d.r(1, 11, 4, 4, 1);
    return d.take();
}

gs::Bitmap mastArt() {
    Ink d(18, 96);
    d.r(2, 2, 4, 86, 1);
    d.r(12, 2, 4, 86, 1);
    for (int i = 0; i < 7; i++) {
        d.ln(3, 8 + i * 11, 15, 16 + i * 11, 2, 1.2f);
        d.ln(15, 8 + i * 11, 3, 16 + i * 11, 4, 1.2f);
    }
    d.r(0, 86, 18, 8, 5);
    return d.take();
}

gs::Bitmap beamArt() {
    Ink d(100, 14);
    d.r(1, 3, 98, 7, 1);
    d.r(1, 2, 98, 3, 4);
    for (int i = 0; i < 8; i++) d.r(6 + i * 12, 4, 2, 8, 2);
    return d.take();
}

gs::Bitmap houseArt() {
    Ink d(68, 100);
    d.r(4, 16, 60, 8, 5);
    d.r(6, 14, 56, 4, 6);
    d.r(8, 24, 52, 70, 1);
    d.r(8, 24, 6, 70, 2);
    for (int i = 0; i < 6; i++) d.r(8, 34 + i * 10, 52, 1, 3);
    d.r(12, 40, 16, 18, 7);
    d.r(12, 40, 16, 2, 2);
    d.r(12, 40, 2, 18, 2);
    d.r(14, 68, 16, 26, 4);
    d.p(26, 80, 7);
    d.r(30, 6, 8, 12, 9);
    d.r(8, 94, 28, 5, 8);
    d.r(46, 48, 8, 12, 7);
    return d.take();
}

gs::Bitmap shutterArt() {
    Ink d(20, 22);
    d.r(2, 2, 16, 18, 4);
    d.r(2, 6, 16, 2, 2);
    d.r(2, 11, 16, 2, 2);
    d.r(2, 16, 16, 2, 2);
    d.r(4, 2, 2, 18, 6);
    return d.take();
}

gs::Bitmap clerkArt() {
    Ink d(32, 36);
    d.r(16, 2, 11, 4, 4);
    d.r(16, 6, 12, 7, 1);
    d.r(16, 6, 3, 6, 6);
    d.p(24, 8, 7);
    d.r(14, 13, 14, 12, 2);
    d.r(18, 15, 8, 8, 3);
    d.p(22, 18, 9);
    d.r(2, 16, 14, 4, 1);
    d.r(1, 18, 5, 3, 8);
    d.r(15, 25, 5, 8, 2);
    d.r(21, 25, 5, 8, 2);
    d.r(14, 32, 6, 3, 5);
    d.r(21, 32, 6, 3, 5);
    return d.take();
}

gs::Bitmap buckArt() {
    Ink d(28, 46);
    d.r(12, 18, 4, 26, 2);
    d.ln(4, 4, 24, 20, 1, 2.2f);
    d.ln(24, 4, 4, 20, 1, 2.2f);
    d.r(10, 40, 8, 4, 2);
    return d.take();
}

gs::Bitmap lensArt(int c) {
    Ink d(12, 12);
    d.el(6, 6, 5, 5, c);
    d.el(5, 5, 2, 2, 3);
    return d.take();
}

gs::Bitmap lampArt() {
    Ink d(16, 52);
    d.r(6, 10, 4, 38, 2);
    d.r(3, 4, 10, 8, 1);
    d.r(4, 2, 8, 4, 4);
    d.r(2, 46, 12, 4, 5);
    return d.take();
}

gs::Bitmap flameArt(int frm) {
    Ink d(12, 14);
    int s = frm ? 1 : 0;
    d.el(6, 8, 4, 5, 1);
    d.el(6, 7 - s, 2, 3, 2);
    d.p(6, 4 - s, 3);
    return d.take();
}

gs::Bitmap chevArt() {
    Ink d(20, 12);
    d.ln(2, 9, 10, 2, 1, 2.f);
    d.ln(10, 2, 18, 9, 1, 2.f);
    return d.take();
}

gs::Bitmap towerArt() {
    Ink d(40, 90);
    d.r(16, 4, 8, 10, 2);
    d.r(12, 12, 16, 4, 3);
    d.el(20, 24, 14, 11, 1);
    d.el(20, 24, 8, 6, 6);
    d.r(6, 34, 4, 50, 2);
    d.r(30, 34, 4, 50, 2);
    d.r(18, 36, 3, 46, 4);
    d.ln(8, 42, 32, 54, 4, 1.2f);
    d.ln(32, 42, 8, 54, 4, 1.2f);
    d.ln(8, 58, 32, 70, 5, 1.2f);
    d.r(2, 82, 36, 6, 5);
    return d.take();
}

gs::Bitmap shedArt() {
    Ink d(40, 64);
    d.r(3, 12, 34, 7, 3);
    d.r(5, 10, 30, 4, 2);
    d.r(6, 19, 28, 40, 1);
    d.r(10, 24, 10, 10, 4);
    d.r(16, 40, 12, 19, 2);
    d.p(25, 50, 5);
    d.r(2, 58, 36, 4, 2);
    return d.take();
}

gs::Bitmap bellArt() {
    Ink d(14, 16);
    d.r(6, 1, 2, 3, 4);
    d.el(7, 8, 5, 5, 1);
    d.el(7, 9, 2, 2, 3);
    d.r(4, 12, 6, 2, 2);
    return d.take();
}

gs::Bitmap stakeArt() {
    Ink d(12, 36);
    d.r(4, 6, 4, 26, 1);
    d.r(2, 2, 8, 6, 4);
    d.r(1, 30, 10, 4, 2);
    return d.take();
}

gs::Bitmap leverArt() {
    Ink d(16, 36);
    d.r(6, 10, 4, 22, 2);
    d.r(2, 28, 12, 5, 5);
    d.el(8, 8, 5, 5, 3);
    d.el(8, 8, 2, 2, 1);
    d.ln(8, 8, 14, 2, 4, 1.4f);
    return d.take();
}

gs::Bitmap holeArt() {
    Ink d(8, 32);
    d.r(0, 0, 8, 32, 5);
    return d.take(false);
}

gs::Bitmap waterArt() {
    Ink d(8, 8);
    d.r(0, 1, 8, 6, 6);
    d.r(1, 3, 3, 1, 7);
    d.r(5, 5, 2, 1, 7);
    return d.take(false);
}

gs::Bitmap lipArt() {
    Ink d(14, 16);
    d.r(1, 4, 11, 10, 1);
    d.r(2, 8, 9, 5, 2);
    d.r(1, 4, 11, 3, 3);
    d.p(4, 6, 4);
    return d.take();
}

gs::Bitmap shadowArt() {
    Ink d(28, 8);
    d.el(14, 4, 12, 3, 1);
    return d.take(false);
}

gs::Bitmap sunArt() {
    Ink d(24, 24);
    d.el(12, 12, 9, 9, 4);
    d.el(12, 12, 5, 5, 1);
    return d.take();
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    pal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12), gs::rgb4(9, 8, 7), gs::rgb4(14, 11, 4)});
    pal(vdp, PAL_IRON, {0, gs::rgb4(7, 7, 9), gs::rgb4(4, 4, 6), gs::rgb4(11, 6, 3), gs::rgb4(12, 12, 14),
                        gs::rgb4(2, 2, 4), gs::rgb4(14, 12, 8)});
    pal(vdp, PAL_EARTH, {0, gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(10, 8, 6), gs::rgb4(6, 4, 2),
                         gs::rgb4(1, 1, 2), gs::rgb4(2, 4, 7), gs::rgb4(12, 12, 13)});
    pal(vdp, PAL_POUCH, {0, gs::rgb4(13, 10, 6), gs::rgb4(7, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(14, 12, 5),
                         gs::rgb4(9, 7, 4), gs::rgb4(13, 3, 2), gs::rgb4(15, 13, 9)});
    pal(vdp, PAL_PLAYER, {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 7), gs::rgb4(12, 8, 2), gs::rgb4(3, 3, 4),
                          gs::rgb4(3, 2, 1), gs::rgb4(4, 2, 1), gs::rgb4(1, 1, 2), gs::rgb4(8, 6, 3),
                          gs::rgb4(14, 11, 4)});
    pal(vdp, PAL_CUT, {0, gs::rgb4(11, 3, 2), gs::rgb4(7, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(13, 11, 8),
                       gs::rgb4(2, 2, 2), gs::rgb4(9, 5, 2), gs::rgb4(14, 13, 10), gs::rgb4(6, 8, 10),
                       gs::rgb4(3, 3, 4)});
    pal(vdp, PAL_WOOD, {0, gs::rgb4(11, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 3), gs::rgb4(12, 10, 6),
                        gs::rgb4(13, 13, 12)});
    pal(vdp, PAL_FX, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 6, 2), gs::rgb4(15, 14, 9), gs::rgb4(15, 12, 8),
                      gs::rgb4(4, 13, 5), gs::rgb4(14, 13, 11)});
    pal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(8, 1, 1), gs::rgb4(15, 12, 11)});
    pal(vdp, PAL_HOUSE, {0, gs::rgb4(10, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(9, 8, 7), gs::rgb4(3, 8, 4),
                         gs::rgb4(4, 4, 6), gs::rgb4(3, 3, 4), gs::rgb4(15, 13, 7), gs::rgb4(5, 4, 4),
                         gs::rgb4(8, 3, 2)});
    vdp.setFogColor(gs::rgb4(5, 3, 6));

    gs::TileAlloc tiles(vdp, 1);
    paintYard(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = up(vdp, hand(0));
    art.runA = up(vdp, hand(1));
    art.runB = up(vdp, hand(2));
    art.duck = up(vdp, hand(3));
    art.leap = up(vdp, hand(4));
    art.pouch[0] = up(vdp, satchel(0));
    art.pouch[1] = up(vdp, satchel(1));
    art.crate = up(vdp, crateArt());
    art.drum = up(vdp, drumArt());
    art.barrel = up(vdp, barrelArt());
    art.plank = up(vdp, plankArt());
    art.boxcar = up(vdp, boxcarArt());
    art.hook = up(vdp, hookArt());
    art.chain = up(vdp, chainArt());
    art.mast = up(vdp, mastArt());
    art.beam = up(vdp, beamArt());
    art.house = up(vdp, houseArt());
    art.shutter = up(vdp, shutterArt());
    art.clerk = up(vdp, clerkArt());
    art.buck = up(vdp, buckArt());
    art.lensR = up(vdp, lensArt(1));
    art.lensG = up(vdp, lensArt(5));
    art.lamp = up(vdp, lampArt());
    art.flame[0] = up(vdp, flameArt(0));
    art.flame[1] = up(vdp, flameArt(1));
    art.chev = up(vdp, chevArt());
    art.tower = up(vdp, towerArt());
    art.shed = up(vdp, shedArt());
    art.bell = up(vdp, bellArt());
    art.stake = up(vdp, stakeArt());
    art.lever = up(vdp, leverArt());
    art.hole = up(vdp, holeArt());
    art.water = up(vdp, waterArt());
    art.lip = up(vdp, lipArt());
    art.shadow = up(vdp, shadowArt());
    art.sun = up(vdp, sunArt());
}

}  // namespace yardpouc
