#include "game/art.h"

#include <cmath>
#include <vector>

namespace memo {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PI = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, const uint16_t cs[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, cs[i]);
}

void ink(gs::VDP& vdp, int pal, uint16_t c) {
    uint16_t cs[16] = {};
    cs[1] = c;
    cs[15] = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, cs);
}

void plate(Bitmap& b, int rim, int edge, int paper) {
    b.rect(0, 0, CARD_W, CARD_H, rim);
    b.rect(1, 1, CARD_W - 2, CARD_H - 2, edge);
    b.rect(3, 3, CARD_W - 6, CARD_H - 6, paper);
}

void star(Bitmap& b, float cx, float cy, float r, int c) {
    std::vector<Pt> p;
    p.reserve(10);
    for (int i = 0; i < 10; i++) {
        float a = -PI * 0.5f + i * (PI / 5.f);
        float rr = (i & 1) ? r * 0.42f : r;
        p.push_back({cx + std::cos(a) * rr, cy + std::sin(a) * rr});
    }
    b.poly(p, c);
}

Bitmap faceMoon() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    b.ellipse(21, 20, 12, 12, 2);
    b.ellipse(21, 20, 9, 9, 11);
    b.ellipse(26, 17, 7.5f, 7.5f, 2);
    b.rect(11, 11, 2, 2, 12);
    b.rect(30, 14, 2, 2, 12);
    b.set(15, 28, 12);
    return b;
}

Bitmap faceStar() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    star(b, 21, 21, 13, 7);
    star(b, 21, 21, 8, 11);
    b.ellipse(21, 21, 2.2f, 2.2f, 12);
    return b;
}

Bitmap faceFish() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    b.poly({{30, 20}, {36, 14}, {36, 26}}, 6);
    b.ellipse(20, 20, 10, 6.5f, 6);
    b.ellipse(20, 22, 7, 3, 14);
    b.poly({{14, 20}, {20, 16}, {20, 24}}, 2);
    b.ellipse(14, 18, 2.2f, 2.2f, 12);
    b.rect(13, 18, 2, 2, 15);
    return b;
}

Bitmap faceBell() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    b.rect(20, 8, 2, 4, 4);
    b.ellipse(21, 15, 8, 7, 4);
    b.poly({{11, 16}, {31, 16}, {34, 27}, {8, 27}}, 11);
    b.rect(8, 27, 26, 3, 4);
    b.ellipse(21, 32, 2.4f, 2.4f, 15);
    b.ellipse(17, 14, 2, 3, 12);
    return b;
}

Bitmap faceHeart() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    b.ellipse(16, 17, 6.5f, 6.5f, 3);
    b.ellipse(26, 17, 6.5f, 6.5f, 3);
    b.poly({{10, 19}, {32, 19}, {21, 33}}, 3);
    b.ellipse(15, 15, 2, 2, 10);
    return b;
}

Bitmap faceKey() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    b.ellipse(14, 16, 7, 7, 4);
    b.ellipse(14, 16, 3, 3, 1);
    b.rect(19, 14, 15, 4, 4);
    b.rect(27, 18, 3, 6, 4);
    b.rect(32, 18, 3, 4, 4);
    b.rect(21, 15, 4, 1, 11);
    return b;
}

Bitmap faceLeaf() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    b.rect(20, 7, 2, 5, 9);
    b.ellipse(21, 22, 9, 12, 5);
    b.ellipse(17, 20, 4, 7, 13);
    b.line(21, 12, 21, 32, 13, 1.4f);
    b.line(21, 18, 27, 15, 13, 1.1f);
    b.line(21, 24, 15, 28, 13, 1.1f);
    return b;
}

Bitmap faceCrown() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 9, 1);
    b.rect(8, 26, 26, 7, 4);
    b.poly({{8, 26}, {13, 12}, {18, 26}}, 11);
    b.poly({{16, 26}, {21, 8}, {26, 26}}, 4);
    b.poly({{24, 26}, {29, 12}, {34, 26}}, 11);
    b.ellipse(13, 16, 2, 2, 3);
    b.ellipse(21, 12, 2.1f, 2.1f, 6);
    b.ellipse(29, 16, 2, 2, 8);
    b.rect(8, 31, 26, 2, 15);
    return b;
}

Bitmap cardBack() {
    Bitmap b(CARD_W, CARD_H);
    plate(b, 15, 1, 2);
    for (int y = 8; y <= 30; y += 6) {
        for (int x = 8; x <= 34; x += 6) {
            b.poly({{float(x), y - 2.f}, {x + 2.f, float(y)}, {float(x), y + 2.f}, {x - 2.f, float(y)}}, 1);
        }
    }
    b.ellipse(21, 20, 6, 6, 1);
    b.ellipse(21, 20, 3.6f, 3.6f, 2);
    b.ellipse(21, 20, 1.6f, 1.6f, 3);
    return b;
}

Bitmap cursorArt() {
    Bitmap b(50, 48);
    auto arm = [&](int x, int y, int sx, int sy) {
        for (int i = 0; i < 12; i++) {
            b.set(x + sx * i, y, 1);
            b.set(x + sx * i, y + sy, 2);
            b.set(x, y + sy * i, 1);
            b.set(x + sx, y + sy * i, 2);
        }
    };
    arm(1, 1, 1, 1);
    arm(48, 1, -1, 1);
    arm(1, 46, 1, -1);
    arm(48, 46, -1, -1);
    return b;
}

Bitmap pipArt() {
    Bitmap b(12, 12);
    b.poly({{0, 0}, {12, 0}, {12, 12}}, 1);
    b.line(3, 5, 5, 8, 15, 1.4f);
    b.line(5, 8, 10, 2, 15, 1.4f);
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 1);
    return b;
}

Bitmap wood(int w, int h, bool vert, bool nails) {
    Bitmap b(w, h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int u = vert ? y : x;
            int v = vert ? x : y;
            int c = 2;
            if (((u / 3) + v * 2) % 7 == 0) c = 1;
            if ((u / 5 + v) % 13 == 0) c = 3;
            if (x == 0 || y == 0 || x == w - 1 || y == h - 1) c = 4;
            if (!vert && (y == 1 || y == h - 2)) c = 3;
            if (vert && (x == 1 || x == w - 2)) c = 3;
            b.set(x, y, c);
        }
    }
    if (nails) {
        b.ellipse(14, h * 0.5f, 2.1f, 2.1f, 4);
        b.ellipse(w - 15.f, h * 0.5f, 2.1f, 2.1f, 4);
        b.set(14, h / 2, 1);
        b.set(w - 15, h / 2, 1);
    }
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t face[16] = {
        0,
        gs::rgb4(15, 14, 11),
        gs::rgb4(2, 3, 8),
        gs::rgb4(13, 2, 2),
        gs::rgb4(14, 11, 2),
        gs::rgb4(2, 11, 3),
        gs::rgb4(2, 6, 13),
        gs::rgb4(15, 8, 2),
        gs::rgb4(9, 3, 12),
        gs::rgb4(8, 5, 2),
        gs::rgb4(15, 10, 12),
        gs::rgb4(15, 14, 5),
        gs::rgb4(15, 15, 15),
        gs::rgb4(1, 7, 3),
        gs::rgb4(3, 12, 11),
        gs::rgb4(1, 1, 2),
    };
    const uint16_t back[16] = {
        0,
        gs::rgb4(13, 10, 3),
        gs::rgb4(1, 2, 7),
        gs::rgb4(15, 14, 11),
        gs::rgb4(0, 1, 4),
        gs::rgb4(6, 8, 12),
        0, 0, 0, 0, 0, 0, 0, 0, 0,
        gs::rgb4(0, 0, 1),
    };
    const uint16_t woodPal[16] = {
        0,
        gs::rgb4(12, 8, 3),
        gs::rgb4(9, 6, 2),
        gs::rgb4(6, 3, 1),
        gs::rgb4(3, 2, 1),
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        gs::rgb4(1, 1, 1),
    };
    const uint16_t mark[16] = {
        0,
        gs::rgb4(15, 14, 5),
        gs::rgb4(15, 15, 15),
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        gs::rgb4(4, 3, 1),
    };
    setPal(vdp, PAL_FACE, face);
    setPal(vdp, PAL_BACK, back);
    setPal(vdp, PAL_WOOD, woodPal);
    setPal(vdp, PAL_MARK, mark);
    ink(vdp, PAL_INK, gs::rgb4(15, 15, 15));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 5));
    ink(vdp, PAL_ALERT, gs::rgb4(15, 4, 3));
    ink(vdp, PAL_OK, gs::rgb4(6, 15, 7));
    ink(vdp, PAL_DIM, gs::rgb4(11, 10, 8));

    loadFont(vdp, art);
    Bitmap (*faces[PAIRS])() = {faceMoon, faceStar, faceFish, faceBell, faceHeart, faceKey, faceLeaf, faceCrown};
    for (int i = 0; i < PAIRS; i++) art.face[i] = gs::uploadMipped(vdp, faces[i]());
    art.back = gs::uploadMipped(vdp, cardBack());
    art.cursor = gs::uploadMipped(vdp, cursorArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.woodTop = gs::uploadMipped(vdp, wood(gs::SCREEN_W, WOOD_T, false, true));
    art.woodBot = gs::uploadMipped(vdp, wood(gs::SCREEN_W, WOOD_B, false, true));
    art.woodSide = gs::uploadMipped(vdp, wood(WOOD_SIDE, SIDE_H, true, false));

    gs::TextStyle logo{3, 1, 0, 15, 1};
    gs::TextStyle sub{2, 1, 0, 15, 1};
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 MEMORY", logo));
    art.lineA = gs::uploadMipped(vdp, gs::textBitmap("MATCH THE TABLE", sub));
    art.lineB = gs::uploadMipped(vdp, gs::textBitmap("BEFORE THE CLOCK", sub));
}

}  // namespace memo
