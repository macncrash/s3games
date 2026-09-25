#include "art.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cable {
namespace {

constexpr int CAR_W = 128;
constexpr int CAR_H = 80;
constexpr float CAR_AX = 64.f;
constexpr float CAR_AY = 58.f;
constexpr float PLAT_AX = 36.f;
constexpr float PLAT_AY = 46.f;

void color(gs::VDP& v, int pal, int i, int r, int g, int b) { v.setColor(pal * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    color(v, PAL_HUD, 1, 15, 15, 15);
    color(v, PAL_HUD, 2, 9, 10, 12);
    color(v, PAL_HUD, 3, 15, 12, 4);
    color(v, PAL_HUD, 4, 15, 4, 3);
    color(v, PAL_HUD, 5, 5, 14, 8);
    color(v, PAL_HUD, 6, 1, 1, 2);
    color(v, PAL_HUD, 7, 15, 14, 11);
    color(v, PAL_HUD, 8, 8, 10, 12);

    color(v, PAL_CAR, 1, 5, 1, 2);
    color(v, PAL_CAR, 2, 11, 2, 3);
    color(v, PAL_CAR, 3, 14, 12, 8);
    color(v, PAL_CAR, 4, 5, 7, 10);
    color(v, PAL_CAR, 5, 9, 12, 14);
    color(v, PAL_CAR, 6, 7, 1, 2);
    color(v, PAL_CAR, 7, 15, 15, 15);
    color(v, PAL_CAR, 8, 12, 9, 3);
    color(v, PAL_CAR, 9, 1, 1, 1);
    color(v, PAL_CAR, 10, 12, 8, 5);
    color(v, PAL_CAR, 11, 2, 2, 4);
    color(v, PAL_CAR, 12, 15, 14, 5);
    color(v, PAL_CAR, 13, 7, 9, 11);
    color(v, PAL_CAR, 14, 2, 0, 1);
    color(v, PAL_CAR, 15, 15, 14, 12);

    color(v, PAL_LAND, 1, 12, 11, 9);
    color(v, PAL_LAND, 2, 7, 6, 5);
    color(v, PAL_LAND, 3, 10, 11, 12);
    color(v, PAL_LAND, 4, 9, 6, 3);
    color(v, PAL_LAND, 5, 12, 4, 2);
    color(v, PAL_LAND, 6, 15, 15, 15);
    color(v, PAL_LAND, 7, 15, 12, 3);
    color(v, PAL_LAND, 8, 2, 2, 2);
    color(v, PAL_LAND, 9, 5, 4, 3);
    color(v, PAL_LAND, 10, 3, 2, 2);
    color(v, PAL_LAND, 11, 10, 7, 3);
    color(v, PAL_LAND, 12, 4, 5, 8);
    color(v, PAL_LAND, 13, 6, 8, 10);
    color(v, PAL_LAND, 14, 14, 13, 11);

    color(v, PAL_HOUSE, 1, 12, 8, 6);
    color(v, PAL_HOUSE, 2, 14, 11, 8);
    color(v, PAL_HOUSE, 3, 10, 3, 2);
    color(v, PAL_HOUSE, 4, 4, 3, 3);
    color(v, PAL_HOUSE, 5, 5, 7, 9);
    color(v, PAL_HOUSE, 6, 12, 13, 10);
    color(v, PAL_HOUSE, 7, 8, 2, 2);
    color(v, PAL_HOUSE, 8, 2, 1, 1);
    color(v, PAL_HOUSE, 9, 6, 2, 2);
    color(v, PAL_HOUSE, 10, 9, 9, 8);
    color(v, PAL_HOUSE, 11, 12, 4, 5);

    color(v, PAL_TREE, 1, 2, 5, 2);
    color(v, PAL_TREE, 2, 3, 8, 3);
    color(v, PAL_TREE, 3, 5, 4, 2);
    color(v, PAL_TREE, 4, 6, 10, 4);
    color(v, PAL_TREE, 5, 1, 2, 1);

    color(v, PAL_MARK, 1, 15, 15, 15);
    color(v, PAL_MARK, 2, 15, 13, 4);
    color(v, PAL_MARK, 3, 14, 3, 2);
    color(v, PAL_MARK, 4, 1, 1, 1);

    color(v, PAL_STEEL, 1, 6, 5, 4);
    color(v, PAL_STEEL, 2, 11, 12, 13);
    color(v, PAL_STEEL, 3, 1, 1, 1);
    color(v, PAL_STEEL, 4, 8, 5, 3);
    color(v, PAL_STEEL, 5, 14, 14, 15);
    color(v, PAL_STEEL, 6, 2, 2, 2);
    color(v, PAL_STEEL, 7, 8, 5, 2);

    color(v, PAL_PEOPLE, 1, 13, 8, 6);
    color(v, PAL_PEOPLE, 2, 10, 2, 3);
    color(v, PAL_PEOPLE, 3, 3, 5, 10);
    color(v, PAL_PEOPLE, 4, 2, 2, 4);
    color(v, PAL_PEOPLE, 5, 2, 2, 2);
    color(v, PAL_PEOPLE, 6, 14, 12, 8);
    color(v, PAL_PEOPLE, 7, 1, 1, 1);
    color(v, PAL_PEOPLE, 8, 1, 1, 1);

    color(v, PAL_ROCK, 1, 6, 6, 5);
    color(v, PAL_ROCK, 2, 9, 9, 8);
    color(v, PAL_ROCK, 3, 4, 4, 3);
    color(v, PAL_ROCK, 4, 4, 6, 3);
    color(v, PAL_ROCK, 5, 2, 2, 2);

    color(v, PAL_LAMP, 1, 4, 4, 5);
    color(v, PAL_LAMP, 2, 15, 13, 5);
    color(v, PAL_LAMP, 3, 15, 15, 12);
    color(v, PAL_LAMP, 4, 3, 3, 3);
    color(v, PAL_LAMP, 5, 1, 1, 1);

    color(v, PAL_WATER, 1, 2, 5, 8);
    color(v, PAL_WATER, 2, 3, 8, 10);
    color(v, PAL_WATER, 3, 10, 13, 13);
    color(v, PAL_WATER, 4, 1, 3, 4);

    color(v, PAL_BG, 1, 5, 4, 3);
    color(v, PAL_BG, 2, 6, 5, 4);
    color(v, PAL_BG, 3, 8, 7, 5);
    color(v, PAL_BG, 4, 8, 7, 6);
    color(v, PAL_BG, 5, 2, 2, 2);
    color(v, PAL_BG, 6, 4, 5, 8);
    color(v, PAL_BG, 7, 5, 6, 9);
    color(v, PAL_BG, 8, 7, 8, 11);
    color(v, PAL_BG, 9, 8, 9, 12);
    color(v, PAL_BG, 10, 14, 11, 5);
    color(v, PAL_BG, 11, 15, 10, 4);
    color(v, PAL_BG, 12, 15, 14, 8);
    color(v, PAL_BG, 13, 12, 12, 13);
    color(v, PAL_BG, 14, 14, 14, 15);
    color(v, PAL_BG, 15, 9, 10, 11);
}

void fillHoles(gs::Bitmap& b) {
    gs::Bitmap src = b;
    for (int y = 1; y < b.h - 1; y++) {
        for (int x = 1; x < b.w - 1; x++) {
            if (src.get(x, y)) continue;
            int votes[16] = {};
            int cnt = 0, best = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int c = src.get(x + dx, y + dy);
                    if (!c) continue;
                    votes[c]++;
                    cnt++;
                    if (votes[c] > votes[best]) best = c;
                }
            }
            if (cnt >= 5 && best) b.set(x, y, best);
        }
    }
}

// Rotate around (ax, ay) by theta (screen space, y down). The anchor lands on (outAx, outAy).
gs::Bitmap spin(const gs::Bitmap& src, float ax, float ay, float theta, float& outAx, float& outAy) {
    const float c = std::cos(theta), s = std::sin(theta);
    float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;
    for (float x : {0.f, float(src.w - 1)}) {
        for (float y : {0.f, float(src.h - 1)}) {
            float dx = x - ax, dy = y - ay;
            float ox = dx * c - dy * s, oy = dx * s + dy * c;
            minx = std::min(minx, ox);
            maxx = std::max(maxx, ox);
            miny = std::min(miny, oy);
            maxy = std::max(maxy, oy);
        }
    }
    const int pad = 2;
    int w = int(std::ceil(maxx - minx)) + pad * 2 + 2;
    int h = int(std::ceil(maxy - miny)) + pad * 2 + 2;
    gs::Bitmap out(w, h);
    outAx = pad - minx;
    outAy = pad - miny;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float rx = (x + 0.5f) - outAx;
            float ry = (y + 0.5f) - outAy;
            int ix = int(std::lround(ax + rx * c + ry * s));
            int iy = int(std::lround(ay - rx * s + ry * c));
            out.set(x, y, src.get(ix, iy));
        }
    }
    fillHoles(out);
    return out;
}

void font(gs::TileAlloc& tiles, int* out) {
    uint8_t px[64];
    for (int ch = 32; ch < 128; ch++) {
        const uint8_t* g = gs::glyph(char(ch));
        for (int i = 0; i < 64; i++) px[i] = 0;
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x] = 1;
                if (x + 1 < 8 && y + 1 < 8) px[(y + 1) * 8 + (x + 1)] = 6;
            }
        }
        out[ch - 32] = tiles.shared(px);
    }
}

gs::Image words(gs::VDP& v, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 6;
    st.shadow = 2;
    st.spacing = 1;
    return gs::uploadImage(v, gs::textBitmap(s, st));
}

gs::Bitmap carFlat() {
    gs::Bitmap b(CAR_W, CAR_H);
    b.rect(16, 16, 96, 8, 1);
    b.rect(34, 8, 56, 9, 13);
    b.rect(36, 10, 52, 5, 5);
    gs::TextStyle st;
    st.scale = 1;
    st.color = 7;
    st.spacing = 0;
    b.blit(gs::textBitmap("HILL", st), 46, 18);
    b.rect(18, 24, 92, 30, 2);
    b.rect(20, 28, 88, 14, 3);
    for (int i = 0; i < 4; i++) {
        if (i == 2) continue;
        int x = 26 + i * 22;
        b.rect(x, 30, 16, 11, 4);
        b.rect(x + 1, 31, 14, 4, 5);
    }
    b.rect(66, 27, 16, 27, 6);
    b.rect(68, 30, 12, 10, 4);
    b.ellipse(74, 36, 3, 3, 10);
    b.rect(72, 35, 4, 2, 11);
    b.rect(20, 54, 88, 7, 1);
    b.ellipse(34, 64, 8, 8, 9);
    b.ellipse(96, 64, 8, 8, 9);
    b.ellipse(34, 64, 3, 3, 8);
    b.ellipse(96, 64, 3, 3, 8);
    b.ellipse(112, 40, 4, 4, 12);
    b.ellipse(16, 40, 3, 3, 8);
    b.rect(60, 56, 8, 5, 8);
    b.outline(14, false);
    b.rect(52, 55, 24, 6, 7);
    return b;
}

gs::Bitmap sleeperFlat() {
    gs::Bitmap b(30, 16);
    b.rect(0, 3, 30, 11, 1);
    b.rect(0, 4, 30, 3, 2);
    b.rect(0, 10, 30, 2, 2);
    b.rect(0, 5, 30, 1, 5);
    b.rect(13, 6, 4, 5, 3);
    b.outline(6, false);
    return b;
}

gs::Bitmap platform(const char* name, int style) {
    gs::Bitmap b(150, 90);
    b.rect(44, 56, 6, 28, 9);
    b.rect(86, 56, 6, 30, 9);
    b.rect(124, 56, 6, 26, 9);
    b.rect(28, 50, 114, 16, 2);
    b.rect(26, 42, 118, 8, 1);
    b.rect(26, 42, 118, 2, 14);
    for (int x = 34; x <= 136; x += 16) b.rect(x, 28, 2, 15, 3);
    b.rect(32, 26, 108, 3, 3);
    b.rect(88, 16, 52, 26, 4);
    b.rect(92, 30, 16, 10, 13);
    b.rect(116, 30, 16, 10, 13);
    if (style == 0) b.poly({{84, 18}, {114, 4}, {146, 18}}, 5);
    else if (style == 1) b.poly({{84, 18}, {114, 6}, {146, 18}}, 11);
    else {
        b.rect(84, 6, 62, 12, 12);
        b.rect(84, 6, 62, 3, 11);
    }
    b.rect(90, 18, 46, 10, 10);
    gs::TextStyle st;
    st.scale = 1;
    st.color = 7;
    st.spacing = 0;
    gs::Bitmap word = gs::textBitmap(name, st);
    b.blit(word, 92 + (42 - word.w) / 2, 20);
    b.outline(8, false);
    b.rect(30, 20, 12, 30, 6);
    b.rect(32, 43, 8, 6, 7);
    b.poly({{36, 24}, {42, 33}, {30, 33}}, 7);
    return b;
}

gs::Bitmap house(int kind) {
    gs::Bitmap b(46, 62);
    b.rect(6, 24, 34, 36, 1);
    b.rect(8, 26, 30, 32, 2);
    if (kind == 0) b.poly({{4, 26}, {23, 6}, {42, 26}}, 3);
    else if (kind == 1) {
        b.rect(4, 16, 38, 10, 9);
        b.rect(4, 16, 38, 3, 3);
    } else {
        b.poly({{2, 28}, {16, 8}, {44, 22}}, 3);
    }
    b.rect(18, 42, 10, 16, 4);
    b.rect(10, 30, 8, 8, 5);
    b.rect(28, 30, 8, 8, 5);
    b.rect(11, 31, 6, 3, 6);
    b.rect(29, 31, 6, 3, 6);
    if (kind != 1) b.rect(14, 40, 18, 3, 7);
    b.rect(12, 22, 3, 3, 11);
    b.outline(8, false);
    return b;
}

gs::Bitmap tree() {
    gs::Bitmap b(30, 66);
    b.poly({{15, 2}, {27, 56}, {3, 56}}, 1);
    b.poly({{15, 12}, {23, 54}, {7, 54}}, 2);
    b.poly({{15, 20}, {20, 48}, {10, 48}}, 4);
    b.rect(13, 54, 4, 10, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap rock() {
    gs::Bitmap b(54, 36);
    b.ellipse(26, 22, 24, 12, 1);
    b.ellipse(18, 20, 12, 8, 2);
    b.ellipse(34, 18, 8, 6, 3);
    b.ellipse(22, 16, 6, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap lamp() {
    gs::Bitmap b(16, 52);
    b.rect(7, 12, 2, 34, 1);
    b.rect(4, 46, 8, 4, 4);
    b.ellipse(8, 9, 6, 6, 2);
    b.ellipse(8, 9, 3, 3, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap person(int kind) {
    gs::Bitmap b(18, 32);
    b.ellipse(9, 8, 4.2f, 4.2f, 1);
    b.rect(6, 5, 6, 3, 5);
    b.rect(6, 12, 6, 9, kind ? 3 : 2);
    b.rect(3, 13, 3, 8, kind ? 3 : 2);
    b.rect(12, 13, 3, 8, 6);
    b.rect(6, 21, 3, 8, 4);
    b.rect(10, 21, 3, 8, 4);
    b.rect(5, 28, 4, 2, 7);
    b.rect(10, 28, 4, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap sheave() {
    gs::Bitmap b(44, 64);
    b.rect(20, 20, 4, 36, 2);
    b.rect(6, 54, 32, 6, 1);
    b.ellipse(22, 18, 15, 15, 2);
    b.ellipse(22, 18, 5, 5, 3);
    b.line(8, 18, 36, 18, 5, 2);
    b.line(22, 4, 22, 32, 5, 2);
    b.outline(6, false);
    return b;
}

gs::Bitmap bumper() {
    gs::Bitmap b(28, 32);
    b.rect(4, 10, 20, 16, 7);
    b.rect(2, 14, 24, 8, 4);
    b.rect(8, 4, 12, 8, 2);
    b.rect(12, 2, 4, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap water() {
    gs::Bitmap b(96, 28);
    b.ellipse(48, 18, 44, 9, 1);
    b.ellipse(34, 16, 22, 6, 2);
    b.line(12, 16, 84, 16, 3, 1.4f);
    b.line(18, 21, 78, 21, 3, 1.2f);
    return b;
}

gs::Bitmap bird() {
    gs::Bitmap b(14, 8);
    b.line(0, 4, 6, 1, 1, 1.2f);
    b.line(6, 1, 13, 5, 1, 1.2f);
    return b;
}

gs::Bitmap bar() {
    gs::Bitmap b(52, 8);
    b.rect(0, 2, 52, 4, 1);
    b.rect(0, 3, 52, 2, 2);
    return b;
}

gs::Bitmap tick() {
    gs::Bitmap b(4, 18);
    b.rect(1, 0, 2, 18, 2);
    return b;
}

void paintHill(gs::VDP& v, gs::TileAlloc& tiles) {
    gs::Bitmap b(320, 224);
    const float len = std::hypot(kDx, kDy);
    const float tx = kDx / len, ty = kDy / len;
    const float nx = -ty, ny = tx;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float u = (x - kX0) / kDx;
            float yL = kY0 + u * kDy;
            float d = y - yL;
            if (d > 7.f) {
                b.set(x, y, 1);
                if ((((x * 13) ^ (y * 7)) & 31) == 0) b.set(x, y, 3);
                else if ((((x * 3) ^ (y * 11)) & 47) == 0) b.set(x, y, 2);
            }
        }
    }
    auto plot = [&](float x, float y, int c) { b.set(int(std::lround(x)), int(std::lround(y)), c); };
    for (float u = 0.f; u <= 1.001f; u += 0.45f / len) {
        float x = kX0 + u * kDx;
        float y = kY0 + u * kDy;
        for (float o : {-4.5f, -3.2f, 3.2f, 4.5f}) plot(x + nx * o, y + ny * o, 15);
        plot(x, y, 5);
        plot(x + nx, y + ny, 5);
    }
    b.ellipse(128, 30, 14, 14, 11);
    b.ellipse(123, 26, 10, 10, 12);
    b.ellipse(70, 18, 22, 8, 13);
    b.ellipse(96, 16, 16, 7, 14);
    b.ellipse(150, 28, 18, 7, 13);
    uint32_t rng = 0x51u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return rng;
    };
    for (int x = 168; x < 308;) {
        int bw = 14 + int(rnd() % 18);
        int bh = 18 + int(rnd() % 28);
        int mid = x + bw / 2;
        float yL = kY0 + (mid - kX0) / kDx * kDy;
        int base = int(yL) - 18;
        if (base < 28) {
            x += bw;
            continue;
        }
        int top = std::max(4, base - bh);
        int col = 6 + int(rnd() % 3);
        b.rect(float(x), float(top), float(bw - 2), float(base - top), col);
        for (int wy = top + 3; wy < base - 5; wy += 6) {
            for (int wx = x + 2; wx < x + bw - 6; wx += 5) b.rect(float(wx), float(wy), 2, 3, (rnd() % 5 == 0) ? 10 : 9);
        }
        x += bw;
    }
    gs::bitmapToPlane(tiles, v.B, 0, 0, b, PAL_BG);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    vdp.setFogColor(gs::rgb4(12, 11, 10));
    vdp.A.enabled = false;
    gs::TileAlloc tiles(vdp, 1);
    font(tiles, art.font);
    paintHill(vdp, tiles);

    const float theta = std::atan2(kDy, kDx);
    art.car = gs::uploadMipped(vdp, spin(carFlat(), CAR_AX, CAR_AY, theta, art.sillX, art.sillY));
    float sax = 15.f, say = 8.f;
    art.sleeper = gs::uploadMipped(vdp, spin(sleeperFlat(), sax, say, theta, art.sleepX, art.sleepY));
    const char* names[3] = {"WHARF", "TERRACE", "CROWN"};
    for (int i = 0; i < 3; i++) art.plat[i] = gs::uploadMipped(vdp, platform(names[i], i));
    art.platX = PLAT_AX;
    art.platY = PLAT_AY;
    for (int i = 0; i < 3; i++) art.house[i] = gs::uploadMipped(vdp, house(i));
    art.tree = gs::uploadMipped(vdp, tree());
    art.rock = gs::uploadMipped(vdp, rock());
    art.lamp = gs::uploadMipped(vdp, lamp());
    art.person[0] = gs::uploadMipped(vdp, person(0));
    art.person[1] = gs::uploadMipped(vdp, person(1));
    art.sheave = gs::uploadMipped(vdp, sheave());
    art.bumper = gs::uploadMipped(vdp, bumper());
    art.water = gs::uploadMipped(vdp, water());
    art.bird = gs::uploadMipped(vdp, bird());
    art.bar = gs::uploadMipped(vdp, bar());
    art.tick = gs::uploadMipped(vdp, tick());
    art.title = words(vdp, "S3 CABLE", 3);
    art.line1 = words(vdp, "STOP LEVEL", 2);
    art.line2 = words(vdp, "WITH THE MARK", 2);
    art.level = words(vdp, "LEVEL", 4);
    art.ran = words(vdp, "RAN PAST", 3);
    art.enter = words(vdp, "ENTER", 2);
}

}  // namespace cable
