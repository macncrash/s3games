#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace tablechime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

bool onCloth(float x, float y) { return x >= kL && x <= kR && y >= kT && y <= kB; }

gs::Bitmap clothArt() {
    const int w = int(kR - kL);
    const int h = int(kB - kT);
    gs::Bitmap b(w, h);
    b.rect(0, 0, float(w), float(h), 5);
    b.rect(5, 5, float(w - 10), float(h - 10), 3);
    b.rect(8, 8, float(w - 16), float(h - 16), 2);
    for (int y = 12; y < h - 12; y += 8) b.rect(12, float(y), float(w - 24), 1, 4);
    b.rect(10, 10, float(w - 20), 2, 1);
    b.rect(10, float(h - 12), float(w - 20), 2, 1);
    b.rect(10, 10, 2, float(h - 20), 1);
    b.rect(float(w - 12), 10, 2, float(h - 20), 1);
    float mid = kNetY - kT;
    b.rect(10, mid - 1.f, float(w - 20), 2, 1);
    return b;
}

gs::Bitmap netArt() {
    gs::Bitmap b(int(kR - kL - 20), 12);
    for (int x = 0; x < b.w; x += 3) b.rect(float(x), 2, 1, 8, 1);
    b.rect(0, 0, float(b.w), 2, 2);
    b.rect(0, 10, float(b.w), 2, 2);
    return b;
}

gs::Bitmap cupArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 16, 12, 10, 2);
    b.ellipse(14, 15, 8, 6.5f, 1);
    b.ellipse(14, 15, 4.2f, 3.4f, 4);
    b.rect(13, 3, 2, 7, 3);
    b.ellipse(14, 3, 3.4f, 2.2f, 3);
    b.ellipse(10, 12, 2.2f, 1.4f, 5);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.2f, 4.2f, 1);
    b.ellipse(3.6f, 3.6f, 1.3f, 1.1f, 2);
    return b;
}

gs::Bitmap batArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(10, 8, 9, 6.5f, 1);
    b.rect(16, 6, 10, 4, 2);
    b.ellipse(8, 7, 3.2f, 2.2f, 3);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(13, 13);
    b.rect(6, 1, 1, 11, 1);
    b.rect(1, 6, 11, 1, 1);
    b.ellipse(6.5f, 6.5f, 3.6f, 3.6f, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 2);
    b.ellipse(18, 18, 13, 13, 1);
    b.ellipse(18, 18, 11.5f, 11.5f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 3.14159265f / 6.f;
        float x = 18.f + std::sin(a) * 9.2f;
        float y = 18.f - std::cos(a) * 9.2f;
        b.rect(x - 0.6f, y - 0.6f, 1.4f, 1.4f, i % 3 == 0 ? 4 : 5);
    }
    b.rect(17, 4, 2, 6, 4);
    b.ellipse(18, 4, 2.4f, 1.6f, 4);
    return b;
}

}  // namespace

Spot spotAt(float x, float y) {
    float dx = x - kCupX;
    float dy = y - kCupY;
    if (dx * dx + dy * dy <= kCupR * kCupR && y < kNetY - kNetH && onCloth(x, y)) return Spot::Cup;
    if (onCloth(x, y) && std::fabs(y - kNetY) <= kNetH) return Spot::Net;
    if (onCloth(x, y) && y > kNetY) return Spot::Short;
    if (onCloth(x, y)) return Spot::Wide;
    return Spot::Hot;
}

void Art::load(gs::VDP& vdp) {
    setPal(vdp, PAL_CLOTH,
           {0, gs::rgb4(13, 12, 9), gs::rgb4(1, 7, 4), gs::rgb4(2, 10, 5), gs::rgb4(8, 12, 8), gs::rgb4(7, 4, 2)});
    setPal(vdp, PAL_NET, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_CUP,
           {0, gs::rgb4(7, 4, 1), gs::rgb4(11, 7, 2), gs::rgb4(15, 12, 5), gs::rgb4(2, 1, 1), gs::rgb4(15, 14, 8),
            gs::rgb4(14, 10, 4)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 13), gs::rgb4(11, 12, 10)});
    setPal(vdp, PAL_BAT, {0, gs::rgb4(11, 2, 3), gs::rgb4(5, 2, 2), gs::rgb4(15, 9, 8)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 5, 2)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(12, 10, 6), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 8), gs::rgb4(8, 6, 3)});
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_INK, gs::rgb4(13, 12, 10));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 8));
    vdp.setFogColor(gs::rgb4(2, 2, 3));
    loadFont(vdp, *this);
    cloth = gs::uploadImage(vdp, clothArt());
    net = gs::uploadImage(vdp, netArt());
    cup = gs::uploadImage(vdp, cupArt());
    ball = gs::uploadImage(vdp, ballArt());
    bat = gs::uploadImage(vdp, batArt());
    cross = gs::uploadImage(vdp, crossArt());
    clock = gs::uploadImage(vdp, clockArt());
}

}  // namespace tablechime
