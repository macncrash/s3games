#include "game/art.h"

#include "console/gfx.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace mazechime {
namespace {

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 0x9E3779B1u ^ uint32_t(y) * 0x85EBCA77u;
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

gs::Image say(gs::VDP& vdp, const std::string& s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.shadow = 2;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

bool wall(const uint8_t* h, int x, int y) {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return h[y * MW + x] != 0;
}

gs::Bitmap coat(int facing, int stride) {
    gs::Bitmap b(16, 24);
    int s = stride ? 1 : 0;
    b.ellipse(8, 3, 4, 3, 4);
    b.rect(5, 6, 6, 3, 5);
    if (facing == 1) {
        b.rect(6, 7, 4, 2, 4);
    } else if (facing == 2) {
        b.set(11, 7, 8);
        b.set(12, 8, 3);
    } else {
        b.set(6, 7, 8);
        b.set(9, 7, 8);
        b.set(7, 9, 6);
        b.set(8, 9, 6);
    }
    b.rect(4, 10, 8, 6, 2);
    b.rect(6, 11, 4, 3, 3);
    b.rect(11, 10, 3, 3, 7);
    b.rect(3, 12 + s, 2, 4, 2);
    b.rect(11, 12, 2, 4, 2);
    b.rect(5, 16, 2, 5, 1);
    b.rect(9, 16 - s, 2, 5, 1);
    b.rect(4, 20, 3, 2, 9);
    b.rect(9, 20, 3, 2, 9);
    b.outline(1, false);
    return b;
}

gs::Bitmap gatePic() {
    gs::Bitmap b(14, 16);
    b.rect(2, 1, 10, 13, 3);
    b.rect(4, 3, 6, 9, 4);
    b.rect(6, 1, 2, 3, 2);
    b.ellipse(7, 8, 2, 2, 5);
    b.rect(1, 13, 12, 2, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap clockPic() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 14, 14, 2);
    b.ellipse(16, 16, 11, 11, 3);
    b.rect(15, 6, 2, 10, 4);
    b.rect(15, 15, 8, 2, 5);
    b.ellipse(16, 16, 2, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap titlePoster() {
    gs::Bitmap t(168, 72);
    t.rect(0, 0, 168, 72, 2);
    t.rect(4, 4, 160, 64, 3);
    t.rect(8, 8, 152, 20, 4);
    for (int i = 0; i < 6; i++) t.rect(16 + i * 24, 40, 14, 18, 5 + (i & 1));
    t.ellipse(84, 28, 6, 6, 6);
    t.outline(1, false);
    return t;
}

gs::Bitmap paintMaze(const uint8_t* h, int ex, int ey) {
    gs::Bitmap b(MW * CELL, MH * CELL);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int cx = x / CELL;
            int cy = y / CELL;
            int lx = x % CELL;
            int ly = y % CELL;
            int c = 4;
            if (wall(h, cx, cy)) {
                c = ((mix(x, y) >> 4) & 3) ? 2 : 3;
                if ((lx + ly) % 9 == 0) c = 6;
            } else {
                c = (mix(x / 2, y / 2) & 7) == 0 ? 5 : 4;
            }
            bool lip = false;
            if (lx < 2 && wall(h, cx - 1, cy)) lip = true;
            if (lx >= CELL - 2 && wall(h, cx + 1, cy)) lip = true;
            if (ly < 2 && wall(h, cx, cy - 1)) lip = true;
            if (ly >= CELL - 2 && wall(h, cx, cy + 1)) lip = true;
            if (lip && !wall(h, cx, cy)) c = 7;
            if (wall(h, cx, cy) && lip) c = 1;
            b.set(x, y, c);
        }
    }
    b.ellipse(float(ex * CELL + 8), float(ey * CELL + 8), 6, 6, 8);
    return b;
}

}  // namespace

void Art::bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey) {
    setPal(vdp, PAL_MAZE,
           {0, gs::rgb4(1, 2, 1), gs::rgb4(1, 5, 2), gs::rgb4(2, 8, 3), gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4),
            gs::rgb4(3, 6, 2), gs::rgb4(4, 4, 2), gs::rgb4(12, 9, 3), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(4, 6, 12), gs::rgb4(8, 10, 15), gs::rgb4(12, 8, 5), gs::rgb4(14, 11, 7),
            gs::rgb4(3, 2, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 15), gs::rgb4(3, 2, 4), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(15, 12, 5), gs::rgb4(15, 14, 8), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(10, 8, 5), gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 4), gs::rgb4(12, 3, 3), 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_TITLE,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(1, 2, 5), gs::rgb4(2, 4, 8), gs::rgb4(8, 6, 3), gs::rgb4(3, 7, 3),
            gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 13, 11), gs::rgb4(1, 1, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 3, 3), gs::rgb4(2, 0, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, fontBase);
    gs::TileAlloc tiles(vdp, 80);
    gs::bitmapToPlane(tiles, vdp.A, OX / 8, OY / 8, paintMaze(hedge, ex, ey), PAL_MAZE);

    for (int face = 0; face < 3; face++)
        for (int stride = 0; stride < 2; stride++) body[face][stride] = gs::uploadImage(vdp, coat(face, stride));
    shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap s(12, 5);
        s.ellipse(6, 2, 5, 2, 1);
        return s;
    }());
    gate = gs::uploadImage(vdp, gatePic());
    clock = gs::uploadImage(vdp, clockPic());
    poster = gs::uploadImage(vdp, titlePoster());
    wordTitle = say(vdp, "S3 MAZE CHIME", 2);
    wordRule = say(vdp, "LEAVE WHEN THE HOUR CHIMES", 1);
    wordMove = say(vdp, "ARROWS WALK   THE GATE OPENS AT TWELVE", 1);
    wordGo = say(vdp, "PRESS ANY KEY", 1);
}

}  // namespace mazechime
