#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace fmark {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float PPM = 5.5f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

void quad(Bitmap& b, float cgx, float cgy, float hdg, float n0, float r0, float n1, float r1, float n2, float r2,
          float n3, float r3, int c) {
    const float ca = std::cos(hdg), sa = std::sin(hdg);
    auto p = [&](float n, float r) -> Pt {
        float wx = r * ca + n * sa;
        float wy = -r * sa + n * ca;
        return {cgx + wx * PPM, cgy - wy * PPM};
    };
    b.poly({p(n0, r0), p(n1, r1), p(n2, r2), p(n3, r3)}, c);
}

void disc(Bitmap& b, float cgx, float cgy, float hdg, float n, float r, float rad, int c) {
    const float ca = std::cos(hdg), sa = std::sin(hdg);
    float wx = r * ca + n * sa;
    float wy = -r * sa + n * ca;
    b.ellipse(cgx + wx * PPM, cgy - wy * PPM, rad * PPM, rad * PPM, c);
}

Bitmap drawHull(float hdg) {
    const float cgx = 78.f, cgy = 78.f;
    Bitmap b(156, 156);
    quad(b, cgx, cgy, hdg, -6.4f, -2.15f, 5.1f, -2.15f, 5.1f, 2.15f, -6.4f, 2.15f, 1);
    quad(b, cgx, cgy, hdg, 5.1f, -2.15f, 7.15f, 0.f, 5.1f, 2.15f, 5.1f, -2.15f, 1);
    quad(b, cgx, cgy, hdg, -7.15f, -1.45f, -6.4f, -2.15f, -6.4f, 2.15f, -7.15f, 1.45f, 2);
    quad(b, cgx, cgy, hdg, -5.6f, -2.2f, 4.4f, -2.2f, 4.4f, -1.55f, -5.6f, -1.55f, 3);
    quad(b, cgx, cgy, hdg, -1.1f, -1.55f, 3.3f, -1.55f, 3.3f, 1.55f, -1.1f, 1.55f, 4);
    quad(b, cgx, cgy, hdg, -0.4f, -1.15f, 2.5f, -1.15f, 2.5f, 1.15f, -0.4f, 1.15f, 5);
    quad(b, cgx, cgy, hdg, 0.1f, -0.85f, 0.7f, -0.85f, 0.7f, 0.85f, 0.1f, 0.85f, 6);
    quad(b, cgx, cgy, hdg, 1.1f, -0.85f, 1.7f, -0.85f, 1.7f, 0.85f, 1.1f, 0.85f, 6);
    disc(b, cgx, cgy, hdg, 3.7f, 0.f, 0.55f, 7);
    disc(b, cgx, cgy, hdg, 3.7f, 0.f, 0.28f, 8);
    quad(b, cgx, cgy, hdg, -4.8f, -1.15f, -3.5f, -1.15f, -3.5f, -0.35f, -4.8f, -0.35f, 9);
    quad(b, cgx, cgy, hdg, -4.6f, 0.35f, -3.3f, 0.35f, -3.3f, 1.15f, -4.6f, 1.15f, 10);
    quad(b, cgx, cgy, hdg, -3.0f, -1.2f, -1.7f, -1.2f, -1.7f, -0.4f, -3.0f, -0.4f, 9);
    b.outline(2, false);
    return b.cropToContent(1);
}

Bitmap pierArt() {
    Bitmap b(48, 48);
    for (int y = 0; y < 48; ++y)
        for (int x = 0; x < 48; ++x) {
            int c = ((x / 8) + (y / 8)) & 1 ? 1 : 2;
            if (x < 3) c = 3;
            if ((x + y) % 17 == 0) c = 4;
            b.set(x, y, c);
        }
    return b;
}

Bitmap shedArt() {
    Bitmap b(40, 28);
    b.rect(2, 8, 36, 18, 1);
    b.poly({{2, 8}, {20, 1}, {38, 8}}, 2);
    b.rect(16, 16, 8, 10, 3);
    b.rect(6, 12, 6, 5, 4);
    b.rect(28, 12, 6, 5, 4);
    return b;
}

Bitmap markArt() {
    Bitmap b(96, 64);
    b.rect(2, 2, 92, 60, 1);
    b.rect(8, 8, 80, 48, 0);
    b.rect(2, 2, 92, 6, 2);
    b.rect(2, 56, 92, 6, 2);
    b.rect(2, 2, 6, 60, 2);
    b.rect(88, 2, 6, 60, 2);
    return b;
}

Bitmap crossArt() {
    Bitmap b(48, 48);
    b.rect(20, 4, 8, 40, 1);
    b.rect(4, 20, 40, 8, 1);
    b.ellipse(24, 24, 6, 6, 2);
    b.ellipse(24, 24, 2, 2, 3);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(18, 22);
    b.ellipse(9, 12, 7, 7, 1);
    b.rect(7, 2, 4, 8, 2);
    b.ellipse(9, 12, 3, 3, 3);
    return b;
}

Bitmap foamArt() {
    Bitmap b(28, 12);
    b.ellipse(8, 6, 6, 3, 1);
    b.ellipse(18, 6, 7, 4, 2);
    b.ellipse(13, 7, 4, 2, 1);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(20, 36);
    b.poly({{10, 2}, {18, 34}, {10, 26}, {2, 34}}, 1);
    b.poly({{10, 8}, {14, 32}, {10, 24}, {6, 32}}, 2);
    return b;
}

Bitmap postArt() {
    Bitmap b(10, 28);
    b.rect(3, 6, 4, 22, 1);
    b.ellipse(5, 5, 4, 4, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4));
    textPal(vdp, PAL_GOOD, gs::rgb4(8, 15, 8));

    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(4, 5, 7), gs::rgb4(2, 5, 12), gs::rgb4(11, 12, 13), gs::rgb4(6, 9, 12),
            gs::rgb4(13, 14, 15), gs::rgb4(12, 4, 3), gs::rgb4(15, 12, 4), gs::rgb4(3, 4, 6), gs::rgb4(8, 3, 3)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(8, 7, 5), gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 3), gs::rgb4(10, 9, 6)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12), gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 10), gs::rgb4(3, 8, 12), gs::rgb4(5, 11, 13)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 14, 14), gs::rgb4(9, 12, 13)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(9, 5, 4), gs::rgb4(6, 3, 3), gs::rgb4(3, 3, 4), gs::rgb4(12, 12, 10)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 6, 3), gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(10, 13, 14), gs::rgb4(14, 15, 15)});

    const float step = 6.2831853f / 16.f;
    for (int i = 0; i < 16; ++i) art.hull[i] = gs::uploadMipped(vdp, drawHull(step * float(i)));
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.cross = gs::uploadMipped(vdp, crossArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.post = gs::uploadMipped(vdp, postArt());
    loadFont(vdp, art);
}

}  // namespace fmark
