#include "game/art.h"

#include "console/gfx.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace mazetape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, int base) {
    for (int c = 32; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[(y + 1) * 8 + (x + 1)] = 1;
        vdp.loadTile(base + (c - 32), px);
    }
}

gs::Image say(gs::VDP& vdp, const std::string& s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.shadow = 2;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

bool isWall(const uint8_t* h, int x, int y) {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return h[y * MW + x] != 0;
}

gs::Bitmap paintMaze(const uint8_t* h, int ex, int ey, int sx, int sy) {
    gs::Bitmap b(MW * CELL, MH * CELL);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int cx = x / CELL, cy = y / CELL;
            if (!isWall(h, cx, cy)) {
                int c = ((x + y) & 4) ? 3 : 4;
                b.set(x, y, c);
                continue;
            }
            int lx = x % CELL, ly = y % CELL;
            int c = 6;
            if (lx < 2 || ly < 2) c = 7;
            else if (lx > 12 || ly > 12) c = 8;
            else if (((x * 3 + y * 5) % 17) == 0) c = 9;
            b.set(x, y, c);
        }
    }
    b.rect(float(sx * CELL + 5), float(sy * CELL + 12), 6, 2, 5);
    b.rect(float(ex * CELL + 2), float(ey * CELL + 2), float(CELL - 4), float(CELL - 4), 2);
    b.rect(float(ex * CELL + 4), float(ey * CELL + 3), 2, float(CELL - 6), 10);
    b.rect(float(ex * CELL + 10), float(ey * CELL + 3), 2, float(CELL - 6), 10);
    b.rect(float(ex * CELL + 4), float(ey * CELL + 7), 8, 2, 10);
    return b;
}

gs::Bitmap clerk(int facing, int stride) {
    gs::Bitmap b(16, 22);
    int swing = stride ? 1 : 0;
    b.rect(5, 0, 6, 2, 3);
    b.rect(4, 2, 8, 3, 4);
    if (facing == 1) {
        b.rect(5, 3, 6, 2, 5);
    } else if (facing == 2) {
        b.rect(8, 3, 3, 2, 4);
        b.set(10, 3, 1);
    } else {
        b.set(6, 3, 1);
        b.set(9, 3, 1);
        b.rect(7, 4, 2, 1, 6);
    }
    b.rect(4, 5, 8, 8, 2);
    b.rect(5, 6, 6, 6, 7);
    b.rect(6, 10, 4, 2, 8);
    b.rect(11, 6, 3, 4, 2);
    b.rect(13, 9, 2, 2, 9);
    b.rect(3, 7 + swing, 2, 5, 2);
    b.rect(4, 16, 3, 5, 5);
    b.rect(9, 16, 3, 5, 5);
    b.rect(3, 20, 4, 2, 1);
    b.rect(9, 20 - swing, 4, 2, 1);
    return b;
}

gs::Bitmap tokenPic(int kind) {
    gs::Bitmap b(12, 12);
    if (kind == KEY || kind == LOCK) {
        if (kind == KEY) {
            b.ellipse(4, 4, 3, 3, 1);
            b.ellipse(4, 4, 1, 1, 0);
            b.rect(6, 4, 5, 2, 1);
            b.rect(9, 6, 2, 2, 1);
            b.set(11, 5, 2);
        } else {
            b.rect(3, 1, 6, 3, 1);
            b.rect(4, 0, 4, 2, 0);
            b.rect(2, 4, 8, 7, 1);
            b.rect(5, 6, 2, 3, 2);
        }
    } else if (kind == BELL || kind == CHIME) {
        if (kind == BELL) {
            b.ellipse(6, 5, 4, 4, 1);
            b.rect(2, 7, 8, 2, 1);
            b.set(6, 10, 2);
            b.rect(5, 1, 2, 2, 1);
        } else {
            b.rect(2, 1, 2, 8, 1);
            b.rect(5, 2, 2, 7, 1);
            b.rect(8, 1, 2, 8, 1);
            b.rect(1, 9, 10, 2, 2);
        }
    } else {
        if (kind == LAMP) {
            b.rect(3, 3, 6, 7, 1);
            b.rect(4, 4, 4, 4, 2);
            b.rect(5, 1, 2, 2, 1);
            b.rect(2, 10, 8, 1, 1);
        } else {
            b.rect(5, 6, 2, 5, 1);
            b.ellipse(6, 4, 3, 3, 2);
            b.ellipse(6, 3, 1, 2, 3);
        }
    }
    return b;
}

gs::Bitmap diorama() {
    gs::Bitmap t(160, 72);
    t.rect(0, 0, 160, 72, 6);
    for (int y = 8; y < 64; y += 16)
        for (int x = 8; x < 152; x += 16) t.rect(float(x), float(y), 14, 14, ((x + y) & 32) ? 4 : 3);
    t.rect(8, 24, 144, 14, 4);
    t.rect(72, 8, 14, 56, 4);
    t.rect(24, 28, 8, 6, 9);
    t.ellipse(88, 28, 4, 4, 10);
    t.rect(120, 28, 6, 8, 11);
    t.rect(136, 26, 8, 10, 2);
    t.rect(40, 22, 8, 14, 5);
    return t;
}

}  // namespace

void Art::bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey, int sx, int sy) {
    setPal(vdp, 0,
           {0, gs::rgb4(2, 2, 1), gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 5), gs::rgb4(12, 11, 8), gs::rgb4(8, 7, 4),
            gs::rgb4(1, 4, 2), gs::rgb4(2, 6, 3), gs::rgb4(1, 3, 1), gs::rgb4(4, 8, 3), gs::rgb4(9, 7, 4),
            gs::rgb4(14, 12, 6), 0, 0, 0, 0});
    setPal(vdp, 1,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(3, 5, 9), gs::rgb4(8, 5, 2), gs::rgb4(13, 10, 7), gs::rgb4(2, 2, 4),
            gs::rgb4(12, 6, 5), gs::rgb4(6, 8, 12), gs::rgb4(14, 12, 6), gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0});
    setPal(vdp, 2, {0, gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, 3, {0, gs::rgb4(14, 11, 3), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, 4, {0, gs::rgb4(9, 9, 10), gs::rgb4(14, 14, 15), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, 5, {0, gs::rgb4(1, 1, 1), gs::rgb4(4, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, fontBase);
    gs::TileAlloc tiles(vdp, 80);
    gs::bitmapToPlane(tiles, vdp.A, OX / 8, OY / 8, paintMaze(hedge, ex, ey, sx, sy), 0);

    for (int face = 0; face < 3; face++)
        for (int stride = 0; stride < 2; stride++) body[face][stride] = gs::uploadImage(vdp, clerk(face, stride));
    shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap s(12, 5);
        s.ellipse(6, 2, 5, 2, 1);
        return s;
    }());
    for (int k = 0; k < KIND_N; k++) token[k] = gs::uploadImage(vdp, tokenPic(k));
    gate = gs::uploadImage(vdp, [] {
        gs::Bitmap g(14, 14);
        g.rect(1, 1, 12, 12, 1);
        g.rect(3, 2, 2, 10, 2);
        g.rect(9, 2, 2, 10, 2);
        g.rect(3, 6, 8, 2, 2);
        return g;
    }());
    titlePic = gs::uploadImage(vdp, diorama());
    titleName = say(vdp, "S3 MAZETAPE", 2);
    titleSub = say(vdp, "THE DRAWER HAS TO MATCH THE TAPE", 1);
    titleRule = say(vdp, "KEY  BELL  LAMP", 1);
    titleMove = say(vdp, "ARROWS WALK   B DROPS", 1);
    titleGo = say(vdp, "PRESS ENTER", 1);
    sayFind = say(vdp, "TAKE WHAT THE TAPE NAMES", 1);
    sayWrong = say(vdp, "THAT PAYS THE SAME AND IS NOT THE TAPE", 1);
    sayMatch = say(vdp, "THE TILL MATCHES. LEAVE BY THE GATE", 1);
    sayOut = say(vdp, "OUT. THE DRAWER MATCHES THE TAPE", 1);
}

}  // namespace mazetape
