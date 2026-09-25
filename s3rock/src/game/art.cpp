#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace rock {
namespace {

struct Rng {
    uint32_t s;
    float next() {
        s = s * 1664525u + 1013904223u;
        return (s >> 8) * (1.0f / 16777216.0f);
    }
};

void setPal(gs::VDP& vdp, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? c[i] : 0);
}

gs::Bitmap shearBow(const gs::Bitmap& src, int lean) {
    if (!lean) return src;
    gs::Bitmap s(src.w, src.h);
    for (int y = 0; y < src.h; y++) {
        int off = 0;
        if (y < src.h / 2) off = lean * ((src.h / 2 - y) / 5);
        for (int x = 0; x < src.w; x++) {
            int v = src.get(x, y);
            if (v) s.set(x + off, y, v);
        }
    }
    return s;
}

gs::Bitmap makeHull() {
    gs::Bitmap b(42, 54);
    // A skiff from above: pointed bow, planked deck, cabin, red sheer stripe.
    b.poly({{21, 2}, {30, 12}, {34, 24}, {33, 40}, {27, 51}, {15, 51}, {9, 40}, {8, 24}, {12, 12}}, 2);
    b.poly({{21, 6}, {27, 14}, {30, 24}, {29, 39}, {25, 47}, {17, 47}, {13, 39}, {12, 24}, {15, 14}}, 4);
    for (int y = 18; y <= 44; y += 4) b.line(13, float(y), 29, float(y), 3, 1.0f);
    b.line(21, 8, 21, 47, 2, 1.1f);
    b.line(15, 18, 21, 7, 8, 1.4f);
    b.line(27, 18, 21, 7, 8, 1.4f);
    b.line(12, 26, 16, 16, 5, 1.3f);
    b.line(30, 26, 26, 16, 5, 1.3f);
    b.rect(17, 27, 8, 11, 6);
    b.rect(18, 29, 6, 5, 7);
    b.rect(18, 35, 6, 2, 5);
    b.line(21, 44, 26, 50, 6, 1.2f);  // tiller
    b.ellipse(21, 3.5f, 2.6f, 1.6f, 9);
    b.outline(1, true);
    return b;
}

gs::Bitmap makeRock(int kind, int variant) {
    int s = kind == 0 ? 28 : kind == 1 ? 40 : 52;
    gs::Bitmap b(s, s);
    Rng rng{1009u + uint32_t(kind) * 97u + uint32_t(variant) * 131u};
    const int n = 8 + variant;
    std::vector<gs::Pt> pts;
    pts.reserve(size_t(n));
    const float cx = s * 0.50f, cy = s * 0.52f;
    for (int i = 0; i < n; i++) {
        float a = (float(i) + 0.25f * rng.next()) / float(n) * 6.2831853f;
        float rad = float(s) * ROCK_FILL * (0.72f + 0.24f * rng.next());
        float ox = cx + std::cos(a) * rad;
        float oy = cy + std::sin(a) * rad * (0.86f + 0.08f * rng.next());
        pts.push_back({ox, oy});
    }
    b.poly(pts, 3);
    b.ellipse(cx + s * 0.08f, cy + s * 0.10f, s * 0.16f, s * 0.12f, 2);
    b.ellipse(cx - s * 0.10f, cy - s * 0.12f, s * 0.11f, s * 0.07f, 4);
    b.ellipse(cx - s * 0.06f, cy - s * 0.16f, s * 0.035f, s * 0.025f, 5);
    b.line(cx - s * 0.12f, cy - s * 0.02f, cx + s * 0.16f, cy + s * 0.14f, 1, 1.1f);
    if (variant != 1) b.line(cx + s * 0.02f, cy, cx + s * 0.14f, cy - s * 0.16f, 2, 1.0f);
    if (variant == 2) b.ellipse(cx + s * 0.12f, cy + s * 0.02f, s * 0.07f, s * 0.045f, 6);
    // A chip missing, so the three shapes don't read as the same pebble.
    if (variant == 0) b.ellipse(cx + s * 0.22f, cy - s * 0.08f, s * 0.06f, s * 0.05f, 2);
    b.outline(1, true);
    return b;
}

gs::Bitmap makeSplash(int frame) {
    gs::Bitmap b(22, 22);
    float s = frame ? 1.25f : 1.0f;
    b.ellipse(11, 12, 6.5f * s, 3.2f * s, 2);
    b.ellipse(11, 11, 2.4f, 1.6f, 1);
    b.line(11, 11, 3, 6, 1, 1.1f);
    b.line(11, 11, 19, 5, 1, 1.1f);
    b.line(11, 11, 5, 16, 2, 1.0f);
    b.line(11, 11, 18, 15, 2, 1.0f);
    if (frame) {
        b.line(11, 8, 11, 2, 1, 1.0f);
        b.ellipse(4, 8, 1.3f, 1.0f, 1);
        b.ellipse(18, 9, 1.4f, 1.0f, 1);
    }
    return b;
}

void paintWater(uint8_t* px, int v) {
    std::memset(px, 1, 64);
    int ripple = 1 + (v % 5);
    for (int x = 0; x < 8; x++)
        if ((x + v) % 3 != 0) px[ripple * 8 + x] = 2;
    for (int k = 0; k < 4; k++) {
        int x = (k * 3 + v * 2) & 7;
        int y = (k + v * 3) & 7;
        px[y * 8 + x] = k == 0 ? 3 : 2;
    }
    px[((v * 5 + 2) & 7) * 8 + ((v * 3 + 4) & 7)] = 4;
}

void paintBank(uint8_t* px, int v) {
    std::memset(px, 2, 64);
    for (int i = 0; i < 10; i++) {
        int x = (i * 3 + v) & 7;
        int y = (i * 5 + v * 2) & 7;
        px[y * 8 + x] = (i % 4 == 0) ? 1 : 3;
    }
    if (v & 1) {
        px[2 * 8 + 3] = 4;
        px[5 * 8 + 6] = 4;
    }
    for (int y = 0; y < 8; y++) px[y * 8 + (v % 2)] = 5;
}

void buildField(gs::VDP& vdp) {
    gs::TileAlloc tiles(vdp, 1);
    int water[6];
    uint8_t px[64];
    for (int v = 0; v < 6; v++) {
        paintWater(px, v);
        water[v] = tiles.shared(px);
    }
    int bank[4];
    for (int v = 0; v < 4; v++) {
        paintBank(px, v);
        bank[v] = tiles.shared(px);
    }
    vdp.B.clear();
    vdp.A.clear();
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) {
            int t = water[(cx * 3 + cy * 5 + (cx ^ cy)) % 6];
            vdp.B.set(cx, cy, gs::entry(t, PAL_WATER));
        }
    }
    // Three columns of cliff on each side. The inner column is the lit lip.
    for (int cy = 0; cy < vdp.A.h; cy++) {
        for (int i = 0; i < 3; i++) {
            int t = bank[(cy + i) & 3];
            vdp.A.set(i, cy, gs::entry(t, PAL_BANK, i == 2 ? 1 : 0, 0));
            vdp.A.set(37 + i, cy, gs::entry(t, PAL_BANK, i == 0 ? 1 : 0, 0));
        }
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hull[] = {0,
                             gs::rgb4(1, 1, 2),
                             gs::rgb4(4, 2, 1),
                             gs::rgb4(8, 5, 2),
                             gs::rgb4(12, 8, 3),
                             gs::rgb4(15, 13, 9),
                             gs::rgb4(3, 3, 5),
                             gs::rgb4(7, 13, 15),
                             gs::rgb4(13, 2, 2),
                             gs::rgb4(14, 15, 15)};
    const uint16_t stone[] = {0,
                              gs::rgb4(2, 2, 3),
                              gs::rgb4(4, 4, 5),
                              gs::rgb4(8, 8, 9),
                              gs::rgb4(12, 12, 13),
                              gs::rgb4(15, 15, 15),
                              gs::rgb4(4, 7, 4)};
    const uint16_t warm[] = {0,
                             gs::rgb4(3, 2, 1),
                             gs::rgb4(6, 3, 2),
                             gs::rgb4(10, 6, 3),
                             gs::rgb4(14, 10, 6),
                             gs::rgb4(15, 14, 12),
                             gs::rgb4(5, 6, 3)};
    const uint16_t moss[] = {0,
                             gs::rgb4(1, 2, 1),
                             gs::rgb4(3, 4, 3),
                             gs::rgb4(6, 7, 4),
                             gs::rgb4(10, 11, 7),
                             gs::rgb4(14, 15, 12),
                             gs::rgb4(3, 6, 2)};
    const uint16_t hud[] = {0,
                            gs::rgb4(15, 14, 11),
                            gs::rgb4(1, 2, 5),
                            gs::rgb4(8, 14, 8),
                            gs::rgb4(15, 11, 3),
                            gs::rgb4(14, 3, 2),
                            gs::rgb4(6, 8, 11)};
    const uint16_t gold[] = {0, gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 0)};
    const uint16_t bad[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(4, 0, 0)};
    const uint16_t fx[] = {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 15, 15), gs::rgb4(6, 11, 14), gs::rgb4(15, 12, 8)};
    const uint16_t water[] = {0,
                              gs::rgb4(1, 6, 11),
                              gs::rgb4(2, 9, 14),
                              gs::rgb4(10, 15, 15),
                              gs::rgb4(0, 4, 8)};
    const uint16_t bank[] = {0,
                             gs::rgb4(2, 2, 2),
                             gs::rgb4(5, 5, 4),
                             gs::rgb4(8, 8, 6),
                             gs::rgb4(3, 6, 3),
                             gs::rgb4(11, 10, 8)};

    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_BOAT, hull, 10);
    setPal(vdp, PAL_ROCK, stone, 7);
    setPal(vdp, PAL_WARM, warm, 7);
    setPal(vdp, PAL_MOSS, moss, 7);
    setPal(vdp, PAL_FX, fx, 5);
    setPal(vdp, PAL_GOLD, gold, 3);
    setPal(vdp, PAL_BAD, bad, 3);
    setPal(vdp, PAL_WATER, water, 5);
    setPal(vdp, PAL_BANK, bank, 6);
    vdp.setFogColor(gs::rgb4(8, 12, 14));

    gs::Bitmap base = makeHull();
    art.boat[0] = gs::uploadMipped(vdp, shearBow(base, -1));
    art.boat[1] = gs::uploadMipped(vdp, base);
    art.boat[2] = gs::uploadMipped(vdp, shearBow(base, 1));
    for (int k = 0; k < 3; k++)
        for (int v = 0; v < 3; v++) art.rock[k][v] = gs::uploadMipped(vdp, makeRock(k, v));
    art.splash[0] = gs::uploadMipped(vdp, makeSplash(0));
    art.splash[1] = gs::uploadMipped(vdp, makeSplash(1));

    gs::Bitmap foam(8, 18);
    foam.ellipse(4, 9, 1.5f, 7.0f, 1);
    foam.ellipse(4, 6, 1.1f, 2.2f, 2);
    art.foam = gs::uploadMipped(vdp, foam);

    gs::Bitmap shade(32, 14);
    shade.ellipse(16, 7, 14, 5, 1);
    art.shade = gs::uploadMipped(vdp, shade);

    gs::Bitmap crack(26, 32);
    crack.line(7, 4, 15, 16, 1, 1.3f);
    crack.line(15, 16, 11, 28, 1, 1.3f);
    crack.line(15, 14, 22, 7, 8, 1.1f);
    crack.line(10, 20, 4, 24, 8, 1.0f);
    art.crack = gs::uploadMipped(vdp, crack);

    gs::Bitmap rib(8, 14);
    rib.rect(1, 1, 6, 12, 4);
    rib.rect(1, 1, 6, 3, 5);
    rib.line(2, 5, 6, 12, 3, 1.0f);
    rib.outline(2, false);
    art.rib = gs::uploadMipped(vdp, rib);

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 2;
    st.shadow = 0;
    st.spacing = 1;
    for (int c = 32; c < 127; c++)
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), st));

    buildField(vdp);
}

}  // namespace rock
