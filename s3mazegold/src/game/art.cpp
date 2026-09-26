#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace mazegold {
namespace {

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 0x45D9F3Bu ^ uint32_t(y) * 0x119DE1F3u;
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    return h ^ (h >> 15);
}

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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

bool isWall(const uint8_t* h, int x, int y) {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return h[y * MW + x] != 0;
}

// Night gravel and a clipped yew. The exit court keeps its own lamp.
gs::Bitmap paintHedge(const uint8_t* h, int sx, int sy, int ex, int ey, const int* coinX, const int* coinY,
                      int nCoins) {
    gs::Bitmap b(MW * CELL, MH * CELL);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int cx = x / CELL, cy = y / CELL;
            int lx = x - cx * CELL, ly = y - cy * CELL;
            uint32_t hv = mix(x, y);
            if (isWall(h, cx, cy)) {
                bool rim = false;
                if (lx < 2 && !isWall(h, cx - 1, cy)) rim = true;
                if (lx >= CELL - 2 && !isWall(h, cx + 1, cy)) rim = true;
                if (ly < 2 && !isWall(h, cx, cy - 1)) rim = true;
                if (ly >= CELL - 2 && !isWall(h, cx, cy + 1)) rim = true;
                int c = 6;
                uint32_t n = hv % 11u;
                if (n < 2) c = 5;
                else if (n > 8) c = 7;
                if (rim) c = (hv & 1u) ? 9 : 8;
                if (!rim && (hv % 53u) == 0) c = 9;
                b.set(x, y, c);
                continue;
            }
            bool lip = lx < 2 || ly < 2 || lx >= CELL - 2 || ly >= CELL - 2;
            int c = 2;
            if (lip) c = 1;
            else if ((hv % 19u) == 0) c = 3;
            else if ((hv % 23u) == 0) c = 4;
            b.set(x, y, c);
        }
    }
    b.rect(float(sx * CELL + 3), float(sy * CELL + 11), 10, 3, 13);
    b.rect(float(ex * CELL), float(ey * CELL), float(CELL), float(CELL), 10);
    b.rect(float(ex * CELL + 1), float(ey * CELL + 1), 3, float(CELL - 2), 11);
    b.rect(float(ex * CELL + 12), float(ey * CELL + 1), 3, float(CELL - 2), 11);
    b.ellipse(float(ex * CELL + 8), float(ey * CELL + 8), 3, 3, 12);
    for (int i = 0; i < nCoins; i++) {
        float px = float(coinX[i] * CELL + 8);
        float py = float(coinY[i] * CELL + 9);
        b.ellipse(px, py, 5, 3, 14);
        b.ellipse(px, py - 1, 3, 2, 3);
    }
    return b;
}

gs::Bitmap keeper(int facing, int stride) {
    gs::Bitmap b(16, 24);
    int s = stride ? 1 : 0;
    b.rect(4, 0, 8, 3, 5);
    b.rect(5, 1, 6, 2, 3);
    b.rect(3, 3, 10, 2, 1);
    b.rect(4, 3, 8, 1, 5);
    if (facing == 1) {
        b.rect(5, 5, 6, 4, 5);
    } else if (facing == 2) {
        b.rect(6, 5, 5, 4, 4);
        b.rect(10, 6, 2, 2, 5);
        b.set(8, 6, 1);
    } else {
        b.rect(5, 5, 6, 4, 4);
        b.set(6, 6, 1);
        b.set(9, 6, 1);
        b.rect(6, 8, 4, 1, 1);
    }
    b.rect(3, 9, 10, 9, 1);
    b.rect(4, 10, 8, 7, 2);
    b.rect(5, 11, 3, 5, 3);
    b.rect(6, 9, 4, 2, 9);
    if (facing == 1) {
        b.rect(3, 12, 2, 5, 2);
    } else if (facing == 2) {
        b.rect(11, 11, 3, 5, 6);
        b.rect(12, 12, 1, 2, 7);
        b.set(12, 10, 7);
    } else {
        b.rect(11, 12, 3, 5, 6);
        b.set(12, 13, 7);
        b.rect(2, 12, 2, 5, 2);
    }
    b.rect(4, 17, 3, 5, 8);
    b.rect(9, 17, 3, 5 - s, 8);
    b.rect(4, 21, 4, 2, 1);
    b.rect(8, 20 + s, 4, 2, 1);
    if (stride) b.rect(9, 16, 3, 2, 8);
    return b;
}

gs::Bitmap drawGold() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 4, 4, 2);
    b.rect(5, 2, 2, 8, 3);
    b.rect(2, 5, 8, 2, 3);
    b.set(4, 4, 4);
    return b;
}

gs::Bitmap creamToken() {
    gs::Bitmap b(12, 12);
    b.rect(2, 1, 8, 10, 1);
    b.rect(1, 2, 10, 8, 1);
    b.rect(3, 2, 6, 8, 2);
    b.rect(2, 3, 8, 6, 2);
    b.rect(4, 4, 4, 4, 3);
    b.set(5, 4, 4);
    return b;
}

gs::Bitmap barsArt() {
    gs::Bitmap b(16, 20);
    b.rect(1, 0, 14, 3, 2);
    b.rect(2, 0, 12, 1, 3);
    for (int i = 0; i < 4; i++) {
        b.rect(float(2 + i * 3), 3, 2, 13, 1);
        b.set(2 + i * 3, 7, 3);
    }
    b.rect(1, 16, 14, 3, 2);
    b.rect(0, 2, 2, 16, 4);
    b.rect(14, 2, 2, 16, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 14);
    b.rect(5, 0, 2, 3, 1);
    b.ellipse(6, 8, 5, 5, 2);
    b.ellipse(6, 8, 2, 2, 3);
    b.set(5, 7, 4);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 2);
    b.ellipse(5, 6, 2, 2, 3);
    b.ellipse(9, 8, 1, 1, 3);
    b.set(8, 4, 3);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 1);
    b.set(0, 1, 1);
    b.set(1, 1, 1);
    b.set(2, 1, 1);
    b.set(1, 2, 1);
    return b;
}

// The title card: gold and cream on the path, the gate still barred.
gs::Bitmap diorama() {
    gs::Bitmap b(180, 100);
    b.rect(0, 0, 180, 100, 1);
    for (int i = 0; i < 8; i++) b.set(12 + i * 20, 6 + (i & 1) * 4, 3);
    b.ellipse(150, 16, 8, 8, 3);
    b.ellipse(148, 15, 3, 3, 4);
    b.rect(10, 28, 160, 64, 13);
    b.rect(16, 34, 148, 52, 5);
    b.rect(16, 34, 148, 4, 6);
    b.rect(16, 82, 148, 4, 6);
    b.rect(16, 34, 8, 52, 6);
    b.rect(156, 34, 8, 52, 6);
    b.rect(70, 34, 40, 52, 7);
    b.rect(16, 52, 148, 16, 7);
    b.rect(24, 38, 40, 10, 6);
    b.rect(116, 38, 36, 10, 6);
    b.rect(28, 70, 36, 10, 6);
    b.rect(118, 70, 32, 10, 6);
    b.ellipse(46, 60, 7, 7, 9);
    b.ellipse(46, 60, 4, 4, 10);
    b.rect(44, 55, 4, 10, 10);
    b.rect(41, 58, 10, 4, 10);
    b.ellipse(132, 60, 7, 7, 11);
    b.ellipse(132, 60, 4, 4, 12);
    b.rect(78, 34, 24, 3, 13);
    for (int i = 0; i < 4; i++) b.rect(float(80 + i * 5), 37, 2, 12, 13);
    b.ellipse(90, 30, 4, 4, 14);
    b.rect(84, 72, 8, 10, 15);
    b.rect(86, 68, 4, 4, 4);
    b.rect(92, 76, 3, 4, 14);
    b.rect(8, 26, 164, 2, 13);
    b.rect(8, 92, 164, 2, 13);
    b.rect(8, 26, 2, 68, 13);
    b.rect(170, 26, 2, 68, 13);
    return b;
}

}  // namespace

void Art::bake(gs::VDP& vdp, const uint8_t* hedge, int sx, int sy, int ex, int ey, const int* coinX, const int* coinY,
               int nCoins) {
    setPal(vdp, 0,
           {0, gs::rgb4(2, 2, 4), gs::rgb4(4, 4, 6), gs::rgb4(6, 6, 8), gs::rgb4(2, 5, 3), gs::rgb4(0, 2, 1),
            gs::rgb4(1, 4, 2), gs::rgb4(2, 6, 2), gs::rgb4(3, 8, 3), gs::rgb4(8, 7, 2), gs::rgb4(7, 6, 5),
            gs::rgb4(12, 9, 3), gs::rgb4(15, 14, 8), gs::rgb4(6, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(12, 10, 4)});
    setPal(vdp, 1,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 6), gs::rgb4(4, 4, 9), gs::rgb4(12, 8, 6), gs::rgb4(6, 3, 2),
            gs::rgb4(10, 8, 3), gs::rgb4(15, 12, 4), gs::rgb4(3, 2, 2), gs::rgb4(13, 12, 10)});
    setPal(vdp, 2,
           {0, gs::rgb4(8, 5, 1), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 13)});
    setPal(vdp, 3,
           {0, gs::rgb4(8, 7, 5), gs::rgb4(14, 12, 9), gs::rgb4(15, 14, 12), gs::rgb4(6, 5, 4)});
    setPal(vdp, 4,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 7), gs::rgb4(8, 8, 10), gs::rgb4(7, 3, 2)});
    setPal(vdp, 5,
           {0, gs::rgb4(1, 1, 4), gs::rgb4(2, 2, 6), gs::rgb4(14, 14, 10), gs::rgb4(10, 10, 8), gs::rgb4(1, 4, 2),
            gs::rgb4(3, 7, 3), gs::rgb4(4, 4, 6), gs::rgb4(6, 6, 8), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8),
            gs::rgb4(14, 12, 9), gs::rgb4(15, 15, 13), gs::rgb4(3, 3, 5), gs::rgb4(15, 12, 4), gs::rgb4(5, 5, 11)});
    setPal(vdp, 6, {0, gs::rgb4(15, 14, 10), gs::rgb4(2, 1, 3)});
    setPal(vdp, 7, {0, gs::rgb4(15, 14, 11), gs::rgb4(4, 3, 1)});
    setPal(vdp, 8, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 13, 9), gs::rgb4(11, 10, 7), gs::rgb4(6, 6, 10)});

    loadFont(vdp, fontBase);
    gs::TileAlloc tiles(vdp, 96);
    gs::bitmapToPlane(tiles, vdp.A, OX / 8, OY / 8, paintHedge(hedge, sx, sy, ex, ey, coinX, coinY, nCoins), 0);

    for (int face = 0; face < 3; face++)
        for (int stride = 0; stride < 2; stride++) body[face][stride] = gs::uploadImage(vdp, keeper(face, stride));
    shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap s(12, 5);
        s.ellipse(6, 2, 5, 2, 1);
        return s;
    }());
    goldCoin = gs::uploadImage(vdp, drawGold());
    creamCoin = gs::uploadImage(vdp, creamToken());
    bars = gs::uploadImage(vdp, barsArt());
    lamp = gs::uploadImage(vdp, lampArt());
    moon = gs::uploadImage(vdp, moonArt());
    star = gs::uploadImage(vdp, starArt());
    titlePic = gs::uploadImage(vdp, diorama());
    titleName = phrase(vdp, "S3 MAZE GOLD", 2);
    titleRule = phrase(vdp, "ONLY THE GOLD COUNTS DOUBLE", 1);
    titleLeave = phrase(vdp, "LEAVE WHEN THAT IS TRUE", 1);
    titleMove = phrase(vdp, "ARROWS WALK    ENTER STARTS", 1);
    vdp.setFogColor(gs::rgb4(1, 1, 3));
}

}  // namespace mazegold
