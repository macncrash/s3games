#include "game/art.h"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace sled {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void paintArch(Bitmap& b, bool lamp) {
    const float s = b.w / float(GATE_W);
    const float m = GATE_POST_M * s;
    const float pw = GATE_POST_W * s;
    const float top = 16.f * s;
    const float barH = 9.f * s;
    b.rect(m, top, pw, b.h - top - 1.f, 1);
    b.rect(m + pw * 0.28f, top, std::max(2.f, pw * 0.28f), b.h - top - 1.f, 2);
    b.rect(b.w - m - pw, top, pw, b.h - top - 1.f, 1);
    b.rect(b.w - m - pw + pw * 0.5f, top, std::max(2.f, pw * 0.22f), b.h - top - 1.f, 2);
    b.rect(m - 2.f * s, top - 2.f * s, b.w - 2.f * m + 4.f * s, barH, 1);
    b.rect(m - 2.f * s, top - 2.f * s, b.w - 2.f * m + 4.f * s, 3.f * s, 2);
    const float innerL = m + pw;
    const float innerR = b.w - m - pw;
    b.rect(innerL, top + barH, innerR - innerL, 12.f * s, 3);
    b.rect(innerL, top + barH, innerR - innerL, 3.f * s, 7);
    b.rect(innerL, top + barH + 9.f * s, innerR - innerL, 3.f * s, 4);
    const float midL = m + pw * 0.5f;
    const float midR = b.w - m - pw * 0.5f;
    b.poly({{midL - pw * 0.55f, top - 2.f * s}, {midL + pw * 0.55f, top - 2.f * s}, {midL, 3.f * s}}, 3);
    b.poly({{midR - pw * 0.55f, top - 2.f * s}, {midR + pw * 0.55f, top - 2.f * s}, {midR, 3.f * s}}, 3);
    b.rect(m - s, top - s, pw + 2.f * s, 3.f * s, 5);
    b.rect(b.w - m - pw - s, top - s, pw + 2.f * s, 3.f * s, 5);
    b.ellipse(m + pw * 0.5f, b.h - 3.f * s, pw * 0.75f, 3.2f * s, 5);
    b.ellipse(b.w - m - pw * 0.5f, b.h - 3.f * s, pw * 0.75f, 3.2f * s, 5);
    if (!lamp) return;
    const float cx = b.w * 0.5f;
    const float ly = top + barH + 6.f * s;
    b.rect(cx - s, top - 4.f * s, 2.2f * s, barH + 8.f * s, 9);
    b.ellipse(cx, ly + 6.f * s, 8.f * s, 9.f * s, 8);
    b.rect(cx - 3.2f * s, ly + 1.f * s, 6.4f * s, 10.f * s, 9);
    b.ellipse(cx, ly + 6.f * s, 2.6f * s, 3.6f * s, 8);
}

Bitmap archArt(bool lamp) {
    Bitmap b(lamp ? 120 : GATE_W, lamp ? 140 : 112);
    paintArch(b, lamp);
    return b;
}

Bitmap dogArt(int frame) {
    Bitmap b(56, 50);
    b.ellipse(22, 11, 4.2f, 6.f, 3);
    b.ellipse(21, 8, 2.2f, 3.f, 2);
    b.ellipse(28, 17, 7.f, 6.f, 2);
    b.ellipse(28, 16, 4.f, 3.f, 4);
    b.poly({{20, 16}, {23, 5}, {26, 16}}, 1);
    b.poly({{30, 16}, {33, 5}, {37, 16}}, 1);
    b.poly({{21.5f, 15}, {23.2f, 8}, {25, 15}}, 8);
    b.poly({{31.5f, 15}, {33.2f, 8}, {35.2f, 15}}, 8);
    b.ellipse(28, 25, 12.f, 9.f, 1);
    b.ellipse(28, 27, 8.f, 6.f, 8);
    b.rect(15, 23, 26, 3, 5);
    b.rect(26, 23, 4, 7, 5);
    static const int lift[3][4] = {{0, 5, 1, 6}, {6, 1, 5, 0}, {2, 6, 0, 4}};
    const int lx[4] = {14, 22, 32, 40};
    for (int i = 0; i < 4; i++) {
        const int up = lift[frame][i];
        b.rect(float(lx[i]), 31.f, 4.f, up > 3 ? 7.f : 11.f, up > 3 ? 4 : 2);
        b.rect(float(lx[i] - 1), up > 3 ? 37.f : 41.f, 6.f, 3.f, 1);
    }
    b.outline(7, false);
    return b;
}

Bitmap sledArt(int lean) {
    Bitmap b(84, 96);
    const float L = float(lean) * 7.f;
    b.line(24, 74, 34 + L * 0.35f, 16, 6, 3.2f);
    b.line(60, 74, 50 + L * 0.35f, 16, 6, 3.2f);
    b.line(26, 72, 35 + L * 0.35f, 18, 7, 1.4f);
    b.line(58, 72, 49 + L * 0.35f, 18, 7, 1.4f);
    b.line(34 + L * 0.35f, 16, 42 + L * 0.15f, 8, 4, 3.f);
    b.line(50 + L * 0.35f, 16, 42 + L * 0.15f, 8, 4, 3.f);
    b.line(38 + L * 0.2f, 12, 34 + L, 1, 5, 1.6f);
    b.line(46 + L * 0.2f, 12, 50 + L, 1, 5, 1.6f);
    b.poly({{30 + L * 0.25f, 46}, {54 + L * 0.25f, 46}, {50 + L * 0.2f, 26}, {34 + L * 0.2f, 26}}, 8);
    b.poly({{34 + L * 0.22f, 42}, {50 + L * 0.22f, 42}, {48 + L * 0.18f, 30}, {36 + L * 0.18f, 30}}, 5);
    b.line(16 + L, 54, 68 + L, 54, 6, 3.5f);
    b.line(20 + L, 54, 26 + L, 66, 6, 2.6f);
    b.line(64 + L, 54, 58 + L, 66, 6, 2.6f);
    b.ellipse(20 + L, 54, 5.f, 3.6f, 9);
    b.ellipse(64 + L, 54, 5.f, 3.6f, 9);
    b.poly({{24 + L, 60}, {60 + L, 60}, {66 + L, 90}, {18 + L, 90}}, 1);
    b.poly({{30 + L, 64}, {54 + L, 64}, {56 + L * 0.8f, 84}, {28 + L * 0.8f, 84}}, 2);
    b.ellipse(42 + L, 56, 13.f, 11.f, 1);
    b.ellipse(42 + L, 58, 8.f, 7.f, 3);
    b.ellipse(42 + L, 60, 3.6f, 3.2f, 10);
    b.ellipse(42 + L * 0.4f, 90, 20.f, 4.f, 3);
    return b;
}

Bitmap spruceArt() {
    Bitmap b(40, 78);
    b.rect(17, 54, 6, 18, 5);
    b.rect(18, 54, 2, 18, 6);
    b.poly({{2, 58}, {38, 58}, {20, 36}}, 1);
    b.poly({{7, 46}, {33, 46}, {20, 26}}, 2);
    b.poly({{11, 34}, {29, 34}, {20, 16}}, 1);
    b.poly({{15, 24}, {25, 24}, {20, 8}}, 2);
    b.poly({{10, 48}, {20, 38}, {18, 46}}, 4);
    b.poly({{16, 28}, {22, 18}, {21, 26}}, 4);
    b.poly({{18, 14}, {23, 9}, {21, 15}}, 4);
    b.ellipse(20, 72, 9, 3, 4);
    b.outline(7, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(32, 18);
    b.ellipse(16, 12, 13, 6, 7);
    b.ellipse(12, 12, 6, 4, 6);
    b.ellipse(16, 8, 9, 3, 4);
    return b;
}

Bitmap cabinArt() {
    Bitmap b(72, 58);
    b.poly({{6, 26}, {36, 6}, {66, 26}}, 3);
    b.rect(10, 24, 52, 28, 1);
    b.rect(12, 26, 48, 24, 2);
    b.rect(8, 23, 56, 4, 3);
    b.rect(32, 34, 10, 18, 5);
    b.rect(18, 32, 10, 8, 4);
    b.rect(46, 32, 10, 8, 4);
    b.rect(20, 34, 6, 4, 7);
    b.rect(48, 34, 6, 4, 7);
    b.rect(52, 12, 6, 14, 6);
    b.ellipse(60, 10, 6, 3, 7);
    b.ellipse(66, 8, 4, 2, 7);
    return b;
}

Bitmap hareArt() {
    Bitmap b(28, 22);
    b.ellipse(13, 14, 8, 5, 1);
    b.ellipse(20, 13, 4, 3, 1);
    b.rect(17, 3, 2, 9, 1);
    b.rect(22, 4, 2, 8, 1);
    b.rect(17, 3, 2, 3, 3);
    b.rect(22, 4, 2, 3, 3);
    b.set(21, 12, 4);
    b.ellipse(8, 16, 3, 2, 2);
    b.outline(4, false);
    return b;
}

Bitmap moonArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 13, 13, 3);
    b.ellipse(11, 12, 3, 2.4f, 4);
    b.ellipse(18, 18, 2.2f, 2.f, 4);
    b.ellipse(20, 11, 1.6f, 1.4f, 4);
    return b;
}

Bitmap auroraArt() {
    Bitmap b(16, 90);
    b.line(8, 86, 11, 58, 2, 2.2f);
    b.line(11, 58, 6, 28, 1, 2.f);
    b.line(6, 28, 9, 4, 3, 1.8f);
    b.line(4, 78, 5, 36, 3, 1.5f);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 4, 1);
    b.ellipse(6, 8, 3, 2, 5);
    b.ellipse(11, 8, 3, 2, 1);
    return b;
}

Bitmap flakeArt() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(64, 14);
    b.ellipse(32, 8, 26, 4, 1);
    return b;
}

void loadFontAndStars(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        const int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
    uint8_t star[64] = {};
    star[3 * 8 + 4] = 1;
    uint8_t dim[64] = {};
    dim[2 * 8 + 2] = 2;
    const int bright = tiles.alloc(1);
    const int faint = tiles.alloc(1);
    vdp.loadTile(bright, star);
    vdp.loadTile(faint, dim);
    a.starTile = bright;
    uint32_t h = 0xC0FFEEu;
    for (int i = 0; i < 42; i++) {
        h = h * 1664525u + 1013904223u;
        const int x = int(h >> 3) & 63;
        const int y = int(h >> 11) % 10;
        const int tile = (h & 2) ? faint : bright;
        const int hf = (h >> 6) & 1;
        vdp.B.set(x, y, gs::entry(tile, PAL_SKY, hf, 0));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 11, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 9, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_DOG, {0, gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), gs::rgb4(13, 12, 9),
                          gs::rgb4(13, 2, 2), gs::rgb4(14, 6, 5), gs::rgb4(1, 1, 1), gs::rgb4(11, 8, 4), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LEAD, {0, gs::rgb4(6, 5, 4), gs::rgb4(15, 15, 14), gs::rgb4(11, 10, 9), gs::rgb4(14, 12, 8),
                           gs::rgb4(3, 6, 13), gs::rgb4(14, 6, 5), gs::rgb4(1, 1, 1), gs::rgb4(12, 9, 5), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SLED, {0, gs::rgb4(2, 3, 8), gs::rgb4(4, 6, 12), gs::rgb4(14, 12, 8), gs::rgb4(8, 5, 2),
                           gs::rgb4(11, 7, 3), gs::rgb4(2, 2, 3), gs::rgb4(10, 11, 13), gs::rgb4(12, 10, 7),
                           gs::rgb4(13, 2, 2), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(1, 5, 2), gs::rgb4(2, 8, 3), gs::rgb4(5, 12, 5), gs::rgb4(14, 15, 15),
                           gs::rgb4(7, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(1, 2, 1), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                           gs::rgb4(15, 15, 15), gs::rgb4(12, 10, 6), gs::rgb4(14, 11, 3), gs::rgb4(15, 13, 4),
                           gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(15, 14, 10), gs::rgb4(11, 11, 8),
                         gs::rgb4(13, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CABIN, {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3), gs::rgb4(14, 15, 15), gs::rgb4(15, 12, 3),
                            gs::rgb4(3, 2, 1), gs::rgb4(4, 4, 5), gs::rgb4(11, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_AURORA, {0, gs::rgb4(8, 15, 10), gs::rgb4(3, 9, 7), gs::rgb4(6, 14, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_HARE, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 11), gs::rgb4(13, 8, 8), gs::rgb4(2, 2, 3), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0});

    const uint16_t snow[16] = {
        0,
        gs::rgb4(13, 14, 15), gs::rgb4(10, 12, 14), gs::rgb4(15, 15, 15),
        gs::rgb4(9, 10, 12), gs::rgb4(7, 8, 11),
        gs::rgb4(12, 13, 14), gs::rgb4(11, 12, 13),
        gs::rgb4(8, 9, 11), gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 14),
        gs::rgb4(11, 12, 14), gs::rgb4(10, 12, 13), gs::rgb4(14, 15, 15),
        gs::rgb4(15, 15, 15), gs::rgb4(14, 15, 15),
    };
    const uint16_t ice[16] = {
        0,
        gs::rgb4(13, 14, 15), gs::rgb4(10, 12, 14), gs::rgb4(15, 15, 15),
        gs::rgb4(8, 12, 14), gs::rgb4(6, 9, 12),
        gs::rgb4(9, 12, 14), gs::rgb4(7, 10, 13),
        gs::rgb4(14, 15, 15), gs::rgb4(6, 9, 12), gs::rgb4(10, 13, 15),
        gs::rgb4(8, 12, 14), gs::rgb4(7, 11, 14), gs::rgb4(12, 15, 15),
        gs::rgb4(12, 15, 15), gs::rgb4(15, 15, 15),
    };
    for (int i = 0; i < 16; i++) {
        vdp.setColor(PAL_SNOW * 16 + i, snow[i]);
        vdp.setColor(PAL_ICE * 16 + i, ice[i]);
    }
    vdp.setFogColor(gs::rgb4(8, 7, 10));
    vdp.A.enabled = false;

    loadFontAndStars(vdp, art);
    for (int i = 0; i < 3; i++) {
        art.dog[i] = gs::uploadMipped(vdp, dogArt(i));
        art.sled[i] = gs::uploadMipped(vdp, sledArt(i - 1));
    }
    art.spruce = gs::uploadMipped(vdp, spruceArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.gate = gs::uploadMipped(vdp, archArt(false));
    art.lantern = gs::uploadMipped(vdp, archArt(true));
    art.cabin = gs::uploadMipped(vdp, cabinArt());
    art.hare = gs::uploadMipped(vdp, hareArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.aurora = gs::uploadMipped(vdp, auroraArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.flake = gs::uploadMipped(vdp, flakeArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace sled
