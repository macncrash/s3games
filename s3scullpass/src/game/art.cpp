#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace scullpass {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + lx * c - ly * s, cy + lx * s + ly * c};
}

Bitmap paintHull(float heading) {
    Bitmap b(64, 64);
    const float cx = 32.f, cy = 32.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    // Nose is -local Y. Heading 0 points up the bitmap.
    poly({{0.f, -26.f}, {3.2f, -16.f}, {3.6f, 14.f}, {2.2f, 22.f}, {-2.2f, 22.f}, {-3.6f, 14.f}, {-3.2f, -16.f}}, 1);
    poly({{0.f, -18.f}, {1.6f, -8.f}, {1.6f, 12.f}, {-1.6f, 12.f}, {-1.6f, -8.f}}, 2);
    b.ellipse(cx, cy + 4.f, 1.6f, 2.4f, 3);
    poly({{-2.4f, 2.f}, {2.4f, 2.f}, {1.4f, 8.f}, {-1.4f, 8.f}}, 4);
    b.ellipse(cx, cy - 10.f, 1.1f, 1.1f, 5);
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

Bitmap word(const char* s, int scale, int col) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = col;
    st.outline = 15;
    st.spacing = 1;
    return gs::textBitmap(s, st);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    loadFont(vdp, art);
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 3), shadow});
    setPal(vdp, PAL_HULL, {0, gs::rgb4(14, 14, 12), gs::rgb4(12, 3, 3), gs::rgb4(6, 8, 11), gs::rgb4(13, 9, 5),
                           gs::rgb4(15, 13, 6), shadow});
    setPal(vdp, PAL_OAR, {0, gs::rgb4(10, 7, 3), gs::rgb4(14, 13, 10), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_CLIFF, {0, gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 5), gs::rgb4(10, 9, 7), gs::rgb4(3, 4, 3),
                            gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 6, 2), gs::rgb4(1, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(6, 8, 3)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(15, 14, 3), gs::rgb4(12, 3, 3), ink});
    setPal(vdp, PAL_STORM, {0, gs::rgb4(4, 4, 7), gs::rgb4(8, 8, 11), gs::rgb4(12, 12, 14)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(12, 15, 8), ink, gs::rgb4(4, 8, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), ink, gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 5, 2), ink});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(9, 10, 11), gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 15, 15), gs::rgb4(7, 12, 13)});

    const float PI = 3.14159265f;
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(-i * (PI / 4.f)));

    {
        Bitmap b(40, 10);
        b.line(2, 5, 34, 5, 1, 1.6f);
        b.ellipse(36, 5, 3.2f, 2.2f, 2);
        b.rect(0, 3, 4, 4, 3);
        art.oar = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(28, 36);
        b.poly({{4.f, 34.f}, {14.f, 2.f}, {24.f, 34.f}}, 1);
        b.poly({{8.f, 28.f}, {14.f, 8.f}, {20.f, 28.f}}, 2);
        b.rect(12, 30, 4, 6, 4);
        b.line(6, 18, 22, 14, 3, 1.2f);
        art.cliff = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(18, 28);
        b.poly({{9.f, 1.f}, {16.f, 16.f}, {2.f, 16.f}}, 1);
        b.poly({{9.f, 8.f}, {15.f, 24.f}, {3.f, 24.f}}, 2);
        b.rect(8, 22, 2, 6, 3);
        art.pine = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(6, 22);
        b.rect(2, 0, 2, 20, 1);
        b.rect(1, 18, 4, 4, 2);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        Bitmap b(48, 8);
        for (int x = 0; x < 48; x += 8) b.rect(float(x), 0, 4, 8, (x / 8) % 2 ? 1 : 2);
        art.tape = gs::uploadMipped(vdp, b);
    }
    art.title = gs::uploadMipped(vdp, word("SCULL PASS", 2, 1));
    art.clearWord = gs::uploadMipped(vdp, word("CLEAR", 3, 1));
    art.missed = gs::uploadMipped(vdp, word("MISSED", 2, 1));
    art.paused = gs::uploadMipped(vdp, word("HOLD", 2, 1));
}

}  // namespace scullpass
