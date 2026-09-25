#include "game/pics.h"

#include "game/view.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gig {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float HH = 12.f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap rasterGround() {
    const float lat0 = -9.2f, lat1 = 9.2f;
    const float z0 = -SEG * 0.5f - 0.16f, z1 = SEG * 0.5f + 0.16f;
    float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
    for (float lat : {lat0, lat1}) {
        for (float z : {z0, z1}) {
            float sx = lat * LX + z * ZX;
            float sy = lat * LY + z * ZY;
            minx = std::min(minx, sx);
            miny = std::min(miny, sy);
            maxx = std::max(maxx, sx);
            maxy = std::max(maxy, sy);
        }
    }
    const int pad = 2;
    const int W = std::max(8, int(std::ceil(maxx - minx)) + pad * 2);
    const int H = std::max(8, int(std::ceil(maxy - miny)) + pad * 2);
    const float ox = -minx + pad;
    const float oy = -miny + pad;
    Bitmap b(W, H);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float lat, z;
            unproject(x + 0.5f - ox, y + 0.5f - oy, lat, z);
            if (lat < lat0 || lat > lat1 || z < z0 || z > z1) continue;
            float a = std::fabs(lat);
            float phase = std::fmod(z + SEG * 0.5f, 2.f);
            if (phase < 0) phase += 2.f;
            int c;
            if (a < ROAD_HALF) {
                int q = int(std::floor(lat * 4.f)) * 17 + int(std::floor((z + 3.f) * 4.f));
                c = (hash01(q) > 0.86f) ? 4 : 3;
                if (a > ROAD_HALF - 0.16f) c = 6;
                else if (a < 0.12f && phase < 0.92f) c = 5;
            } else if (a < ROAD_HALF + WALK) {
                int seam = int(std::floor((z + SEG) * 1.6f)) & 1;
                c = seam ? 8 : 7;
                if (a < ROAD_HALF + 0.10f) c = 10;
            } else {
                int q = int(std::floor(lat * 3.f)) * 13 + int(std::floor(phase * 4.f));
                c = hash01(q + 9) > 0.82f ? 2 : 1;
            }
            b.set(x, y, c);
        }
    }
    return b;
}

Bitmap rasterDisc(float rOuter, float rInner, int fill, int rim) {
    float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
    for (int i = 0; i < 16; i++) {
        float a = float(i) * 6.2831853f / 16.f;
        float sx = std::cos(a) * rOuter * LX + std::sin(a) * rOuter * ZX;
        float sy = std::cos(a) * rOuter * LY + std::sin(a) * rOuter * ZY;
        minx = std::min(minx, sx);
        miny = std::min(miny, sy);
        maxx = std::max(maxx, sx);
        maxy = std::max(maxy, sy);
    }
    const int pad = 2;
    int W = std::max(6, int(std::ceil(maxx - minx)) + pad * 2);
    int H = std::max(6, int(std::ceil(maxy - miny)) + pad * 2);
    float ox = -minx + pad, oy = -miny + pad;
    Bitmap b(W, H);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            float lat, z;
            unproject(x + 0.5f - ox, y + 0.5f - oy, lat, z);
            float d = std::sqrt(lat * lat + z * z);
            if (d <= rOuter && d >= rInner) b.set(x, y, d > rOuter - 0.08f && rim ? rim : fill);
        }
    }
    return b;
}

Pt P(float side, float u, float v, float h, float cx, float cy) {
    return {cx + side * u * LX + v * ZX, cy + side * u * LY + v * ZY - h * HH};
}

Bitmap buildHouse(float side) {
    Bitmap b(150, 128);
    const float cx = 75, cy = 64;
    const float wu = 1.42f, wv = 1.22f, bh = 1.05f, rh = 0.78f;
    auto poly = [&](std::initializer_list<Pt> pts, int c) { b.poly(std::vector<Pt>(pts), c); };
    // Driveway toward the street, under the walls.
    poly({P(side, -wu, -0.15f, 0, cx, cy), P(side, -wu - 1.2f, -0.2f, 0, cx, cy),
          P(side, -wu - 1.2f, 0.8f, 0, cx, cy), P(side, -wu, 0.75f, 0, cx, cy)},
         10);
    auto wall = [&](float u0, float v0, float u1, float v1, int c) {
        poly({P(side, u0, v0, 0, cx, cy), P(side, u1, v1, 0, cx, cy), P(side, u1, v1, bh, cx, cy),
              P(side, u0, v0, bh, cx, cy)},
             c);
    };
    wall(-wu, wv, wu, wv, 2);
    wall(wu, -wv, wu, wv, 2);
    wall(-wu, -wv, -wu, wv, 1);
    wall(-wu, -wv, wu, -wv, 1);
    Pt apex = P(side, 0, 0.05f, bh + rh, cx, cy);
    auto roof = [&](float u0, float v0, float u1, float v1, int c) {
        poly({P(side, u0, v0, bh, cx, cy), P(side, u1, v1, bh, cx, cy), apex}, c);
    };
    roof(-wu - 0.18f, wv + 0.12f, wu + 0.18f, wv + 0.12f, 4);
    roof(wu + 0.18f, wv + 0.12f, wu + 0.18f, -wv - 0.12f, 3);
    roof(wu + 0.18f, -wv - 0.12f, -wu - 0.18f, -wv - 0.12f, 4);
    roof(-wu - 0.18f, -wv - 0.12f, -wu - 0.18f, wv + 0.12f, 3);
    // Door and window on the street face.
    poly({P(side, -wu - 0.02f, -0.08f, 0, cx, cy), P(side, -wu - 0.02f, 0.38f, 0, cx, cy),
          P(side, -wu - 0.02f, 0.38f, 0.62f, cx, cy), P(side, -wu - 0.02f, -0.08f, 0.62f, cx, cy)},
         5);
    poly({P(side, -wu - 0.02f, 0.55f, 0.42f, cx, cy), P(side, -wu - 0.02f, 1.02f, 0.42f, cx, cy),
          P(side, -wu - 0.02f, 1.02f, 0.88f, cx, cy), P(side, -wu - 0.02f, 0.55f, 0.88f, cx, cy)},
         6);
    // Chimney.
    float cu = 0.38f, cv = 0.28f, cs = 0.16f;
    poly({P(side, cu - cs, cv - cs, bh + rh * 0.35f, cx, cy), P(side, cu + cs, cv - cs, bh + rh * 0.35f, cx, cy),
          P(side, cu + cs, cv - cs, bh + rh * 0.85f, cx, cy), P(side, cu - cs, cv - cs, bh + rh * 0.85f, cx, cy)},
         7);
    poly({P(side, cu - cs, cv + cs, bh + rh * 0.35f, cx, cy), P(side, cu - cs, cv - cs, bh + rh * 0.35f, cx, cy),
          P(side, cu - cs, cv - cs, bh + rh * 0.85f, cx, cy), P(side, cu - cs, cv + cs, bh + rh * 0.85f, cx, cy)},
         7);
    // Bush at the street corner.
    Pt bush = P(side, -wu + 0.15f, -wv + 0.15f, 0.22f, cx, cy);
    b.ellipse(bush.first, bush.second, 7, 5, 8);
    b.outline(9, false);
    return b;
}

Bitmap buildRider() {
    Bitmap b(36, 40);
    b.ellipse(18, 28, 6, 4, 3);
    b.ellipse(18, 12, 5, 3, 3);
    b.ellipse(18, 28, 2, 2, 6);
    b.ellipse(18, 12, 2, 2, 6);
    b.line(18, 14, 18, 26, 6, 2.2f);
    b.line(10, 16, 26, 16, 6, 2.f);
    b.ellipse(18, 18, 6, 7, 1);
    b.ellipse(18, 15, 4, 4, 2);
    b.rect(13, 20, 7, 6, 4);
    b.rect(14, 21, 5, 2, 7);
    b.set(16, 14, 7);
    b.set(20, 14, 7);
    b.outline(8, false);
    return b;
}

Bitmap buildTree() {
    Bitmap b(40, 48);
    b.rect(17, 30, 6, 16, 3);
    b.ellipse(20, 22, 14, 12, 2);
    b.ellipse(18, 18, 10, 9, 1);
    b.outline(4, false);
    return b;
}

Bitmap buildLamp() {
    Bitmap b(28, 46);
    b.rect(12, 14, 3, 30, 4);
    b.line(13, 14, 4, 10, 4, 2.f);
    b.rect(1, 6, 8, 5, 6);
    b.ellipse(5, 16, 6, 3, 6);
    return b;
}

Bitmap buildCrate() {
    Bitmap b(28, 26);
    b.rect(4, 8, 20, 14, 1);
    b.rect(6, 10, 16, 10, 2);
    b.ellipse(14, 14, 6, 4, 4);
    b.ellipse(14, 13, 3, 2, 5);
    b.rect(8, 4, 12, 5, 1);
    b.outline(9, false);
    return b;
}

Bitmap buildCar() {
    Bitmap b(64, 56);
    const float cx = 32, cy = 28;
    auto poly = [&](std::initializer_list<Pt> pts, int c) { b.poly(std::vector<Pt>(pts), c); };
    auto Q = [&](float u, float v, float h) { return P(1.f, u, v, h, cx, cy); };
    poly({Q(-0.55f, -0.95f, 0), Q(0.55f, -0.95f, 0), Q(0.55f, 0.95f, 0), Q(-0.55f, 0.95f, 0)}, 4);
    poly({Q(-0.48f, -0.85f, 0.05f), Q(0.48f, -0.85f, 0.05f), Q(0.48f, 0.85f, 0.42f), Q(-0.48f, 0.85f, 0.42f)}, 1);
    poly({Q(-0.48f, -0.85f, 0.05f), Q(-0.48f, 0.85f, 0.05f), Q(-0.48f, 0.85f, 0.42f), Q(-0.48f, -0.85f, 0.42f)}, 2);
    poly({Q(-0.36f, -0.15f, 0.42f), Q(0.36f, -0.15f, 0.42f), Q(0.30f, 0.55f, 0.78f), Q(-0.30f, 0.55f, 0.78f)}, 1);
    poly({Q(-0.28f, -0.05f, 0.48f), Q(0.22f, -0.05f, 0.48f), Q(0.16f, 0.42f, 0.72f), Q(-0.22f, 0.42f, 0.72f)}, 3);
    b.ellipse(Q(-0.42f, -0.62f, 0.08f).first, Q(-0.42f, -0.62f, 0.08f).second, 3, 2, 5);
    b.ellipse(Q(0.42f, 0.7f, 0.12f).first, Q(0.42f, 0.7f, 0.12f).second, 2, 2, 6);
    b.outline(7, false);
    return b;
}

Bitmap buildMail() {
    Bitmap b(22, 28);
    b.rect(10, 12, 3, 14, 4);
    b.rect(4, 6, 14, 8, 1);
    b.rect(5, 7, 12, 5, 2);
    b.rect(16, 4, 3, 6, 3);
    b.outline(5, false);
    return b;
}

Bitmap buildCup() {
    Bitmap b(16, 18);
    b.poly({{3, 5}, {13, 5}, {11, 16}, {5, 16}}, 6);
    b.rect(4, 2, 8, 4, 7);
    b.line(8, 2, 10, 0, 8, 1.4f);
    return b;
}

Bitmap buildBlob() {
    Bitmap b(32, 20);
    b.ellipse(16, 10, 14, 7, 1);
    return b;
}

Bitmap buildButton() {
    Bitmap b(168, 26);
    for (int y = 0; y < 26; y++) {
        for (int x = 0; x < 168; x++) {
            float dx = 0, dy = 0;
            if (x < 10) dx = 10 - x;
            if (x > 157) dx = std::max(dx, float(x - 157));
            if (y < 8) dy = 8 - y;
            if (y > 17) dy = std::max(dy, float(y - 17));
            if (dx * dx + dy * dy > 64) continue;
            b.set(x, y, y < 4 ? 5 : 4);
        }
    }
    return b;
}

Bitmap buildShade() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 3);
    return b;
}

Bitmap buildA() {
    Bitmap b(36, 44);
    b.ellipse(20, 28, 13, 11, 15);
    b.rect(26, 16, 7, 22, 15);
    b.ellipse(18, 26, 13, 11, 2);
    b.rect(24, 14, 7, 22, 2);
    b.ellipse(18, 26, 12, 10, 1);
    b.rect(24, 14, 6, 21, 1);
    b.ellipse(17, 27, 6, 4, 0);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{4, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
    uint8_t dollar[64] = {};
    const char* rows[7] = {".###.", "#.#..", ".###.", "..#.#", ".###.", ".....", "....."};
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 5; x++) {
            if (rows[y][x] != '#') continue;
            dollar[y * 8 + x + 1] = 1;
            if (y + 1 < 8) dollar[(y + 1) * 8 + x + 2] = 15;
        }
    }
    a.tileDollar = tiles.alloc(1);
    vdp.loadTile(a.tileDollar, dollar);
    a.font[int('$') - 32] = a.tileDollar;

    auto solid = [&](int idx) {
        uint8_t p[64];
        for (int i = 0; i < 64; i++) p[i] = uint8_t(idx);
        int t = tiles.alloc(1);
        vdp.loadTile(t, p);
        return t;
    };
    a.tileBar = solid(1);
    a.tileBarDim = solid(2);
    uint8_t star[64] = {};
    const char* srows[7] = {"..#..", ".###.", "#####", ".###.", "..#..", ".....", "....."};
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (srows[y][x] == '#') star[y * 8 + x + 1] = 1;
    a.tileStar = tiles.alloc(1);
    vdp.loadTile(a.tileStar, star);
    for (int i = 0; i < 64; i++)
        if (star[i]) star[i] = 2;
    a.tileStarDim = tiles.alloc(1);
    vdp.loadTile(a.tileStarDim, star);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t sh = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), gs::rgb4(1, 1, 2), gs::rgb4(6, 6, 7),
                            gs::rgb4(10, 10, 11), gs::rgb4(15, 13, 4), gs::rgb4(15, 8, 2), gs::rgb4(15, 4, 3), 0, 0, 0, 0,
                            0, 0, sh});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), gs::rgb4(12, 9, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, sh});
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(4, 15, 3), gs::rgb4(2, 7, 2), gs::rgb4(14, 15, 14), gs::rgb4(3, 14, 3), gs::rgb4(8, 15, 7),
            gs::rgb4(4, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_CYAN, {0, gs::rgb4(2, 13, 15), gs::rgb4(1, 7, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_RIDER,
           {0, gs::rgb4(4, 15, 3), gs::rgb4(2, 9, 2), gs::rgb4(2, 2, 3), gs::rgb4(13, 3, 3), gs::rgb4(12, 8, 5),
            gs::rgb4(7, 7, 8), gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_GROUND,
           {0, gs::rgb4(3, 11, 4), gs::rgb4(2, 8, 3), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(15, 12, 2),
            gs::rgb4(14, 14, 13), gs::rgb4(11, 11, 12), gs::rgb4(8, 8, 9), gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6), 0, 0,
            0, 0, sh});
    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(10, 13, 14), gs::rgb4(6, 9, 11), gs::rgb4(8, 12, 14), gs::rgb4(2, 2, 3), gs::rgb4(14, 3, 3),
            gs::rgb4(15, 15, 12), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_NATURE,
           {0, gs::rgb4(3, 10, 4), gs::rgb4(2, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(1, 1, 2), gs::rgb4(6, 6, 7),
            gs::rgb4(15, 13, 5), 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_PICK,
           {0, gs::rgb4(10, 6, 3), gs::rgb4(7, 4, 2), gs::rgb4(15, 12, 3), gs::rgb4(13, 9, 4), gs::rgb4(4, 12, 4),
            gs::rgb4(14, 8, 3), gs::rgb4(15, 15, 14), gs::rgb4(8, 5, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_MAIL,
           {0, gs::rgb4(3, 8, 14), gs::rgb4(2, 5, 9), gs::rgb4(14, 3, 3), gs::rgb4(6, 4, 2), gs::rgb4(1, 1, 2), 0, 0, 0,
            0, 0, 0, 0, 0, 0, sh});

    auto house = [&](int pal, uint16_t wall, uint16_t wallD, uint16_t roof, uint16_t roofD) {
        setPal(vdp, pal,
               {0, wall, wallD, roof, roofD, gs::rgb4(5, 3, 2), gs::rgb4(8, 13, 15), gs::rgb4(9, 5, 4),
                gs::rgb4(2, 8, 3), gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 10), 0, 0, 0, 0, sh});
    };
    house(PAL_H0, gs::rgb4(13, 8, 9), gs::rgb4(10, 5, 7), gs::rgb4(7, 8, 4), gs::rgb4(5, 6, 3));
    house(PAL_H1, gs::rgb4(14, 12, 6), gs::rgb4(11, 9, 4), gs::rgb4(8, 6, 11), gs::rgb4(6, 4, 8));
    house(PAL_H2, gs::rgb4(13, 10, 7), gs::rgb4(10, 7, 5), gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 7));
    house(PAL_H3, gs::rgb4(14, 12, 10), gs::rgb4(11, 9, 7), gs::rgb4(11, 5, 4), gs::rgb4(8, 3, 3));
    setPal(vdp, PAL_ORANGE, {0, gs::rgb4(15, 9, 2), gs::rgb4(12, 6, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, sh});

    vdp.setFogColor(gs::rgb4(3, 5, 8));
    loadFont(vdp, art);
    art.ground = gs::uploadMipped(vdp, rasterGround());
    art.house[0] = gs::uploadMipped(vdp, buildHouse(1.f));
    art.house[1] = gs::uploadMipped(vdp, buildHouse(-1.f));
    art.rider = gs::uploadMipped(vdp, buildRider());
    art.ring = gs::uploadMipped(vdp, rasterDisc(0.60f, 0.38f, 1, 0));
    art.mat = gs::uploadMipped(vdp, rasterDisc(CATCH_R, 0.f, 1, 2));
    art.matOff = gs::uploadMipped(vdp, rasterDisc(CATCH_R * 0.92f, 0.f, 6, 2));
    art.mail = gs::uploadMipped(vdp, buildMail());
    art.tree = gs::uploadMipped(vdp, buildTree());
    art.lamp = gs::uploadMipped(vdp, buildLamp());
    art.crate = gs::uploadMipped(vdp, buildCrate());
    art.hole = gs::uploadMipped(vdp, rasterDisc(0.55f, 0.f, 4, 5));
    art.car = gs::uploadMipped(vdp, buildCar());
    art.cup = gs::uploadMipped(vdp, buildCup());
    art.blob = gs::uploadMipped(vdp, buildBlob());
    art.button = gs::uploadMipped(vdp, buildButton());
    art.shade = gs::uploadMipped(vdp, buildShade());
    art.letterA = gs::uploadMipped(vdp, buildA());
}

}  // namespace gig
