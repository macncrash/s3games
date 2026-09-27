#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace tablebell {
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
    b.rect(6, 6, float(w - 12), float(h - 12), 3);
    b.rect(8, 8, float(w - 16), float(h - 16), 2);
    for (int y = 10; y < h - 10; y += 6) b.rect(10, float(y), float(w - 20), 2, (y / 6) & 1 ? 3 : 2);
    b.rect(10, 10, float(w - 20), 2, 1);
    b.rect(10, float(h - 12), float(w - 20), 2, 1);
    b.rect(10, 10, 2, float(h - 20), 1);
    b.rect(float(w - 12), 10, 2, float(h - 20), 1);
    float mid = (kNetY - kT);
    b.rect(10, mid - 1.f, float(w - 20), 2, 1);
    b.line(float(w) * 0.5f, 12, float(w) * 0.5f, float(h - 12), 4, 1.2f);
    return b;
}

gs::Bitmap netArt() {
    gs::Bitmap b(int(kR - kL - 16), 10);
    for (int x = 0; x < b.w; x += 4) b.rect(float(x), 1, 2, 8, 1);
    b.rect(0, 0, float(b.w), 2, 2);
    b.rect(0, 8, float(b.w), 2, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 18, 14, 11, 2);
    b.ellipse(16, 17, 10, 8, 1);
    b.ellipse(16, 16, 6.2f, 5.2f, 4);
    b.ellipse(12, 13, 3.2f, 2.2f, 3);
    b.rect(15, 4, 2, 8, 2);
    b.ellipse(16, 4, 3.2f, 2.4f, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(4.2f, 4.2f, 1.6f, 1.4f, 2);
    return b;
}

gs::Bitmap batArt() {
    gs::Bitmap b(26, 14);
    b.ellipse(9, 7, 8, 6, 1);
    b.rect(14, 5, 10, 4, 2);
    b.ellipse(8, 6, 3, 2, 3);
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(14, 14);
    b.rect(6, 1, 2, 12, 1);
    b.rect(1, 6, 12, 2, 1);
    b.ellipse(7, 7, 4.2f, 4.2f, 2);
    b.ellipse(7, 7, 2.2f, 2.2f, 0);
    return b;
}

}  // namespace

Spot spotAt(float x, float y) {
    float dx = x - kBellX;
    float dy = y - kBellY;
    if (dx * dx + dy * dy <= kBellR * kBellR && y < kNetY - kNetH && onCloth(x, y)) return Spot::Bell;
    if (onCloth(x, y) && std::fabs(y - kNetY) <= kNetH) return Spot::Net;
    if (onCloth(x, y) && y > kNetY) return Spot::Short;
    if (onCloth(x, y)) return Spot::Wide;
    return Spot::Hot;
}

void Art::load(gs::VDP& vdp) {
    setPal(vdp, PAL_CLOTH,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(1, 8, 3), gs::rgb4(2, 11, 4), gs::rgb4(12, 13, 10), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_NET, {0, gs::rgb4(14, 14, 14), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(8, 5, 1), gs::rgb4(12, 8, 2), gs::rgb4(15, 13, 6), gs::rgb4(1, 1, 2), gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 10)});
    setPal(vdp, PAL_BAT, {0, gs::rgb4(12, 2, 2), gs::rgb4(6, 2, 1), gs::rgb4(15, 8, 7)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 6, 2)});
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 11));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7));
    vdp.setFogColor(gs::rgb4(2, 3, 2));
    loadFont(vdp, *this);
    cloth = gs::uploadImage(vdp, clothArt());
    net = gs::uploadImage(vdp, netArt());
    bell = gs::uploadImage(vdp, bellArt());
    ball = gs::uploadImage(vdp, ballArt());
    bat = gs::uploadImage(vdp, batArt());
    cross = gs::uploadImage(vdp, crossArt());
}

}  // namespace tablebell
