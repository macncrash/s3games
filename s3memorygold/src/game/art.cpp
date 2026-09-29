#include "game/art.h"

#include <cmath>
#include <vector>

namespace memorygold {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PI = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, const uint16_t cs[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, cs[i]);
}

void plate(Bitmap& b, int rim) {
    b.rect(0, 0, CARD_W, CARD_H, rim);
    b.rect(2, 2, CARD_W - 4, CARD_H - 4, 1);
    b.rect(4, 4, CARD_W - 8, CARD_H - 8, 2);
}

void star(Bitmap& b, float cx, float cy, float r, int c) {
    std::vector<Pt> p;
    for (int i = 0; i < 10; i++) {
        float a = -PI * 0.5f + i * (PI / 5.f);
        float rr = (i & 1) ? r * 0.42f : r;
        p.push_back({cx + std::cos(a) * rr, cy + std::sin(a) * rr});
    }
    b.poly(p, c);
}

Bitmap faceOf(int i) {
    Bitmap b(CARD_W, CARD_H);
    int rim = goldFace(i) ? 7 : 8;
    plate(b, rim);
    float cx = CARD_W * 0.5f, cy = CARD_H * 0.5f + 1.f;
    if (i == 0) {
        star(b, cx, cy, 13, 7);
        star(b, cx, cy, 6, 11);
        b.ellipse(cx, cy, 2.2f, 2.2f, 12);
    } else if (i == 1) {
        b.ellipse(cx, cy, 11, 11, 7);
        b.ellipse(cx, cy, 8, 8, 11);
        b.rect(int(cx) - 1, int(cy) - 6, 2, 12, 5);
        b.rect(int(cx) - 6, int(cy) - 1, 12, 2, 5);
    } else if (i == 2) {
        b.poly({{cx, 8}, {cx + 12, 16}, {cx + 8, 30}, {cx - 8, 30}, {cx - 12, 16}}, 7);
        b.rect(int(cx) - 8, 28, 16, 4, 11);
        b.ellipse(cx, 18, 3, 3, 12);
    } else if (i == 3) {
        b.ellipse(cx, cy, 12, 12, 6);
        b.ellipse(cx + 4, cy - 2, 8, 8, 2);
        b.rect(12, 12, 2, 2, 12);
    } else if (i == 4) {
        b.ellipse(cx, cy, 12, 7, 6);
        b.poly({{32, cy}, {38, cy - 6}, {38, cy + 6}}, 6);
        b.ellipse(16, cy - 2, 2, 2, 12);
    } else if (i == 5) {
        b.poly({{cx, 8}, {cx + 10, 32}, {cx - 10, 32}}, 9);
        b.rect(int(cx) - 2, 6, 4, 4, 10);
    } else if (i == 6) {
        b.ellipse(cx, cy + 2, 8, 10, 9);
        b.ellipse(cx - 6, cy - 4, 5, 7, 9);
        b.ellipse(cx + 6, cy - 2, 4, 6, 10);
    } else {
        b.rect(int(cx) - 1, 8, 2, 4, 4);
        b.ellipse(cx, 16, 8, 6, 4);
        b.poly({{10, 16}, {32, 16}, {34, 28}, {8, 28}}, 11);
        b.ellipse(cx, 30, 3, 3, 4);
    }
    if (goldFace(i)) b.rect(6, 6, 6, 3, 7);
    return b;
}

Bitmap cardBack() {
    Bitmap b(CARD_W, CARD_H);
    b.rect(0, 0, CARD_W, CARD_H, 4);
    b.rect(2, 2, CARD_W - 4, CARD_H - 4, 3);
    b.rect(5, 5, CARD_W - 10, CARD_H - 10, 2);
    for (int y = 8; y < CARD_H - 8; y += 6) {
        for (int x = 8; x < CARD_W - 8; x += 6) b.set(x, y, 5);
    }
    b.rect(CARD_W / 2 - 2, CARD_H / 2 - 2, 4, 4, 6);
    return b;
}

Bitmap cursorBmp() {
    Bitmap b(CARD_W + 8, CARD_H + 8);
    b.rect(0, 0, b.w, 2, 1);
    b.rect(0, b.h - 2, b.w, 2, 1);
    b.rect(0, 0, 2, b.h, 1);
    b.rect(b.w - 2, 0, 2, b.h, 1);
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
    const uint16_t ink[16] = {0, gs::rgb4(14, 13, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)};
    const uint16_t face[16] = {
        0,
        gs::rgb4(15, 14, 11),
        gs::rgb4(8, 6, 4),
        gs::rgb4(3, 5, 9),
        gs::rgb4(12, 9, 4),
        gs::rgb4(2, 2, 3),
        gs::rgb4(4, 9, 12),
        gs::rgb4(15, 12, 3),
        gs::rgb4(13, 11, 8),
        gs::rgb4(3, 11, 5),
        gs::rgb4(6, 13, 6),
        gs::rgb4(15, 14, 8),
        gs::rgb4(15, 15, 14),
        0,
        0,
        gs::rgb4(1, 1, 2),
    };
    const uint16_t back[16] = {
        0, gs::rgb4(12, 4, 4), gs::rgb4(6, 2, 3), gs::rgb4(9, 2, 3), gs::rgb4(14, 12, 8), gs::rgb4(15, 13, 6),
        gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2),
    };
    const uint16_t wood[16] = {0, gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(6, 3, 1), gs::rgb4(14, 11, 6)};
    const uint16_t mark[16] = {0, gs::rgb4(15, 14, 6)};
    const uint16_t gold[16] = {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)};
    const uint16_t alert[16] = {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)};
    const uint16_t ok[16] = {0, gs::rgb4(6, 14, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1)};
    const uint16_t dim[16] = {0, gs::rgb4(7, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)};
    setPal(vdp, PAL_INK, ink);
    setPal(vdp, PAL_FACE, face);
    setPal(vdp, PAL_BACK, back);
    setPal(vdp, PAL_WOOD, wood);
    setPal(vdp, PAL_MARK, mark);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_OK, ok);
    setPal(vdp, PAL_DIM, dim);

    loadFont(vdp, art);
    for (int i = 0; i < PAIRS; i++) art.face[i] = gs::uploadMipped(vdp, faceOf(i));
    art.back = gs::uploadMipped(vdp, cardBack());
    art.cursor = gs::uploadMipped(vdp, cursorBmp());
    Bitmap pip(10, 10);
    pip.ellipse(5, 5, 4, 4, 1);
    art.pip = gs::uploadMipped(vdp, pip);
    Bitmap bar(8, 4);
    bar.rect(0, 0, 8, 4, 1);
    art.bar = gs::uploadMipped(vdp, bar);
    Bitmap woodB(gs::SCREEN_W, 16);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < gs::SCREEN_W; x++) {
            int c = 2;
            if (((x / 5) + y) % 7 == 0) c = 1;
            if (y == 0 || y == 15) c = 4;
            woodB.set(x, y, c);
        }
    }
    art.wood = gs::uploadMipped(vdp, woodB);
    gs::TextStyle logo;
    logo.scale = 2;
    logo.color = 1;
    logo.shadow = 15;
    logo.spacing = 1;
    gs::TextStyle sub;
    sub.scale = 1;
    sub.color = 1;
    sub.shadow = 15;
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 MEMORY GOLD", logo));
    art.lineA = gs::uploadMipped(vdp, gs::textBitmap("ONLY THE GOLD COUNTS DOUBLE", sub));
    art.lineB = gs::uploadMipped(vdp, gs::textBitmap("LEAVE WHEN THAT IS TRUE", sub));
}

}  // namespace memorygold
