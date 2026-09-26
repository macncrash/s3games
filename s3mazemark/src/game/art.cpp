#include "game/art.h"

#include "console/gfx.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace mazemark {
namespace {

constexpr int PAL_MAZE = 0;
constexpr int PAL_BODY = 1;
constexpr int PAL_INK = 2;
constexpr int PAL_COIN = 4;
constexpr int PAL_GOLD = 5;
constexpr int PAL_GREEN = 6;
constexpr int PAL_ALERT = 7;
constexpr int PAL_LOGO = 8;
constexpr int PAL_TREE = 9;
constexpr int PAL_MOON = 10;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, int* font) {
    for (int c = 32; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[(y + 1) * 8 + (x + 1)] = 1;
        int tile = 1 + (c - 32);
        vdp.loadTile(tile, px);
        font[c - 32] = tile;
    }
}

gs::Image phrase(gs::VDP& vdp, const std::string& s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.shadow = 3;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 0x9E3779B1u ^ uint32_t(y) * 0x85EBCA77u;
    h ^= h >> 16;
    h *= 0xC2B2AE35u;
    return h ^ (h >> 13);
}

bool wallAt(const uint8_t* h, int x, int y) {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return h[y * MW + x] != 0;
}

// Night hedge, warm path, a gold ring, and a cold gate that is not the mark.
gs::Bitmap paintMaze(const uint8_t* h, int ex, int ey, int mx, int my, int sx, int sy) {
    gs::Bitmap b(MW * CELL, MH * CELL);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int cx = x / CELL, cy = y / CELL;
            int lx = x - cx * CELL, ly = y - cy * CELL;
            uint32_t hv = mix(x, y);
            if (wallAt(h, cx, cy)) {
                bool border = cx == 0 || cy == 0 || cx == MW - 1 || cy == MH - 1;
                bool rim = false;
                if (lx < 3 && !wallAt(h, cx - 1, cy)) rim = true;
                if (lx >= CELL - 3 && !wallAt(h, cx + 1, cy)) rim = true;
                if (ly < 3 && !wallAt(h, cx, cy - 1)) rim = true;
                if (ly >= CELL - 3 && !wallAt(h, cx, cy + 1)) rim = true;
                int c = border ? 5 : 6;
                if (!border && (mix(x >> 2, y >> 2) % 5u) == 0) c = 7;
                if (!border && (hv % 17u) == 0) c = 5;
                if (rim) c = (hv & 1u) ? 8 : 9;
                if (!rim && !border && (hv % 53u) == 0) c = 10;
                b.set(x, y, c);
            } else {
                bool lip = lx < 2 || lx >= CELL - 2 || ly < 2 || ly >= CELL - 2;
                int c = 2;
                if ((mix(x >> 1, y >> 1) % 6u) == 0) c = 3;
                if ((hv % 13u) == 0) c = 1;
                if (lip) c = 4;
                b.set(x, y, c);
            }
        }
    }
    b.ellipse(float(mx * CELL + 8), float(my * CELL + 8), 6, 6, 11);
    b.ellipse(float(mx * CELL + 8), float(my * CELL + 8), 3, 3, 2);
    b.rect(float(mx * CELL + 7), float(my * CELL + 2), 2, 2, 12);
    b.rect(float(ex * CELL), float(ey * CELL), float(CELL), float(CELL), 13);
    b.rect(float(ex * CELL + 2), float(ey * CELL + 1), 3, float(CELL - 2), 14);
    b.rect(float(ex * CELL + 11), float(ey * CELL + 1), 3, float(CELL - 2), 14);
    b.rect(float(ex * CELL + 2), float(ey * CELL + 1), 12, 3, 15);
    b.rect(float(ex * CELL + 6), float(ey * CELL + 6), 4, 6, 15);
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int wx = ex + dx, wy = ey + dy;
            if (!wallAt(h, wx, wy)) continue;
            if (wx != 0 && wy != 0 && wx != MW - 1 && wy != MH - 1) continue;
            b.rect(float(wx * CELL), float(wy * CELL), float(CELL), float(CELL), 14);
            b.rect(float(wx * CELL + 3), float(wy * CELL + 3), float(CELL - 6), float(CELL - 6), 13);
        }
    }
    b.rect(float(sx * CELL + 4), float(sy * CELL - 5), 8, 4, 11);
    b.rect(float(sx * CELL + 6), float(sy * CELL - 4), 4, 2, 12);
    b.rect(float(sx * CELL + 3), float(sy * CELL + 12), 10, 2, 3);
    return b;
}

// Hat, indigo coat, and a lantern. 0 faces the camera, 1 the back, 2 the right.
gs::Bitmap person(int facing, int stride) {
    gs::Bitmap b(16, 24);
    int step = stride ? 1 : 0;
    b.rect(2, 1, 12, 2, 1);
    b.rect(3, 1, 10, 2, 6);
    b.rect(5, 0, 6, 2, 5);
    b.rect(5, 2, 6, 1, 9);
    if (facing == 1) {
        b.rect(5, 3, 6, 4, 5);
        b.rect(6, 4, 4, 2, 1);
    } else if (facing == 2) {
        b.rect(6, 3, 6, 5, 1);
        b.rect(7, 3, 4, 4, 4);
        b.set(10, 4, 12);
        b.set(10, 5, 1);
        b.set(11, 6, 11);
    } else {
        b.rect(5, 3, 6, 5, 1);
        b.rect(6, 3, 4, 4, 4);
        b.set(6, 4, 12);
        b.set(9, 4, 12);
        b.set(6, 5, 1);
        b.set(9, 5, 1);
        b.rect(7, 6, 2, 1, 11);
    }
    b.rect(3, 8, 10, 8, 1);
    b.rect(4, 9, 8, 6, 2);
    b.rect(7, 9, 2, 5, 3);
    b.set(7, 11, 9);
    b.set(8, 13, 9);
    if (facing == 1) {
        b.rect(1, 10, 3, 4, 1);
        b.rect(1, 11, 3, 2, 7);
        b.rect(12, 10 + step, 2, 5, 2);
    } else if (facing == 2) {
        b.rect(12, 10, 3, 5, 1);
        b.rect(12, 11, 3, 3, 10);
        b.set(13, 10, 9);
        b.rect(2, 10, 2, 5, 2);
    } else {
        b.rect(2, 9, 2, 6, 1);
        b.rect(2, 10, 2, 4, 2);
        b.rect(12, 10, 3, 5, 1);
        b.rect(12, 11, 3, 3, 10);
        b.set(13, 10, 9);
    }
    b.rect(4, 16, 3, 5, 7);
    b.rect(9, 16, 3, 5, 7);
    if (stride) {
        b.rect(4, 16, 3, 4, 7);
        b.rect(9, 17, 3, 4, 7);
    }
    b.rect(4, 21, 3, 2, 8);
    b.rect(9, 21 - step, 3, 2, 8);
    return b;
}

gs::Bitmap coinPic() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 5, 5, 2);
    b.ellipse(5, 5, 2, 1, 3);
    b.rect(6, 5, 2, 4, 4);
    b.rect(5, 6, 4, 2, 4);
    b.set(6, 6, 3);
    return b;
}

gs::Bitmap treePic() {
    gs::Bitmap b(22, 30);
    b.rect(9, 18, 4, 10, 4);
    b.rect(8, 26, 6, 2, 1);
    b.ellipse(11, 14, 9, 8, 1);
    b.ellipse(11, 13, 7, 6, 2);
    b.ellipse(8, 11, 3, 3, 3);
    return b;
}

}  // namespace

void Art::bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey, int mx, int my, int sx, int sy) {
    setPal(vdp, PAL_MAZE,
           {0, gs::rgb4(4, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(10, 8, 5), gs::rgb4(5, 4, 3), gs::rgb4(1, 3, 3),
            gs::rgb4(1, 5, 4), gs::rgb4(2, 6, 4), gs::rgb4(3, 8, 4), gs::rgb4(5, 11, 5), gs::rgb4(8, 2, 3),
            gs::rgb4(10, 7, 2), gs::rgb4(15, 12, 4), gs::rgb4(2, 5, 9), gs::rgb4(4, 9, 13), gs::rgb4(10, 14, 15)});
    setPal(vdp, PAL_BODY,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 8), gs::rgb4(5, 6, 13), gs::rgb4(13, 9, 6), gs::rgb4(3, 2, 1),
            gs::rgb4(12, 10, 7), gs::rgb4(8, 8, 10), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 6),
            gs::rgb4(12, 6, 5), gs::rgb4(15, 15, 14), 0, 0, 0});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 13, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_COIN,
           {0, gs::rgb4(8, 5, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 11), gs::rgb4(9, 6, 1), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(10, 15, 9), gs::rgb4(1, 3, 2), gs::rgb4(2, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 8, 6), gs::rgb4(3, 1, 1), gs::rgb4(7, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 7), gs::rgb4(1, 1, 4), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(1, 2, 1), gs::rgb4(1, 5, 3), gs::rgb4(3, 8, 4), gs::rgb4(5, 3, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(14, 14, 10), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, font);
    gs::TileAlloc tiles(vdp, 80);
    gs::Bitmap maze = paintMaze(hedge, ex, ey, mx, my, sx, sy);
    gs::bitmapToPlane(tiles, vdp.A, OX / 8, OY / 8, maze, PAL_MAZE);
    card = gs::uploadImage(vdp, maze.resample(160, 96));

    for (int face = 0; face < 3; face++)
        for (int stride = 0; stride < 2; stride++) body[face][stride] = gs::uploadImage(vdp, person(face, stride));
    shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap s(12, 5);
        s.ellipse(6, 2, 5, 2, 1);
        return s;
    }());
    coin = gs::uploadImage(vdp, coinPic());
    dot = gs::uploadImage(vdp, [] {
        gs::Bitmap d(3, 3);
        d.ellipse(1.5f, 1.5f, 1.4f, 1.4f, 2);
        return d;
    }());
    tree = gs::uploadImage(vdp, treePic());
    moon = gs::uploadImage(vdp, [] {
        gs::Bitmap m(14, 14);
        m.ellipse(7, 7, 6, 6, 1);
        m.ellipse(5, 5, 2, 2, 2);
        m.set(9, 8, 2);
        return m;
    }());
    star = gs::uploadImage(vdp, [] {
        gs::Bitmap s(3, 3);
        s.set(1, 0, 2);
        s.set(0, 1, 1);
        s.set(1, 1, 2);
        s.set(2, 1, 1);
        s.set(1, 2, 2);
        return s;
    }());
    logo = phrase(vdp, "S3 MAZEMARK", 3);
    finished = phrase(vdp, "FINISHED MARK", 2);
    open = phrase(vdp, "MARK STILL OPEN", 2);
}

}  // namespace mazemark
