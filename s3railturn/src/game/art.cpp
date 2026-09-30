#include "game/art.h"

#include <cmath>
#include <string>

namespace railturn {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Pt rot(float cx, float cy, float x, float y, float bank) {
    float dx = x - cx, dy = y - cy;
    float c = std::cos(bank), s = std::sin(bank);
    return {cx + dx * c - dy * s, cy + dx * s + dy * c};
}

void quad(Bitmap& b, float x, float y, float w, float h, int c, float bank, float cx, float cy) {
    b.poly({rot(cx, cy, x, y, bank), rot(cx, cy, x + w, y, bank), rot(cx, cy, x + w, y + h, bank),
            rot(cx, cy, x, y + h, bank)},
           c);
}

Bitmap carArt(float bank) {
    Bitmap b(112, 80);
    const float cx = 56, cy = 46;
    quad(b, 22, 28, 68, 30, 3, bank, cx, cy);
    quad(b, 26, 22, 60, 10, 4, bank, cx, cy);
    quad(b, 30, 32, 16, 12, 8, bank, cx, cy);
    quad(b, 50, 32, 16, 12, 8, bank, cx, cy);
    quad(b, 70, 32, 14, 12, 8, bank, cx, cy);
    quad(b, 18, 54, 12, 10, 2, bank, cx, cy);
    quad(b, 82, 54, 12, 10, 2, bank, cx, cy);
    quad(b, 24, 58, 64, 6, 5, bank, cx, cy);
    quad(b, 48, 10, 4, 16, 6, bank, cx, cy);
    quad(b, 40, 8, 20, 4, 6, bank, cx, cy);
    quad(b, 34, 18, 44, 6, 7, bank, cx, cy);
    b.outline(1, false);
    return b;
}

Bitmap pylonArt() {
    Bitmap b(28, 72);
    b.rect(4, 4, 4, 64, 2);
    b.rect(20, 4, 4, 64, 2);
    b.rect(4, 16, 20, 3, 3);
    b.rect(4, 36, 20, 3, 3);
    b.rect(4, 56, 20, 3, 3);
    b.line(6, 8, 22, 36, 4, 1.5f);
    b.line(22, 8, 6, 36, 4, 1.5f);
    b.line(6, 40, 22, 56, 4, 1.5f);
    b.outline(1, false);
    return b;
}

Bitmap blockArt(int kind) {
    Bitmap b(40, 56);
    int body = kind == 0 ? 2 : kind == 1 ? 3 : 4;
    b.rect(4, 10, 32, 42, body);
    b.rect(4, 6, 32, 6, 5);
    for (int y = 16; y < 48; y += 10)
        for (int x = 8; x < 32; x += 8) b.rect(float(x), float(y), 4, 5, 6);
    b.outline(1, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(80, 18);
    b.ellipse(40, 9, 36, 7, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_AMBER, {gs::rgb4(0, 0, 0), gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {gs::rgb4(0, 0, 0), gs::rgb4(15, 3, 2)});
    setPal(vdp, PAL_GREEN, {gs::rgb4(0, 0, 0), gs::rgb4(6, 15, 8)});
    setPal(vdp, PAL_CAR,
           {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(4, 4, 5), gs::rgb4(13, 3, 2), gs::rgb4(15, 6, 4),
            gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(15, 12, 4), gs::rgb4(10, 14, 15)});
    setPal(vdp, PAL_STEEL,
           {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(6, 7, 8), gs::rgb4(10, 11, 12), gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_CITY,
           {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(5, 6, 8), gs::rgb4(7, 8, 10), gs::rgb4(9, 7, 6),
            gs::rgb4(4, 4, 5), gs::rgb4(14, 13, 8)});
    // Road bank: steel rail, amber edges, the hillside falls away.
    setPal(vdp, PAL_ROAD,
           {gs::rgb4(0, 0, 0), gs::rgb4(3, 5, 3), gs::rgb4(2, 4, 2), gs::rgb4(5, 6, 4), gs::rgb4(3, 3, 4),
            gs::rgb4(5, 5, 6), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(7, 5, 3), gs::rgb4(5, 5, 6),
            gs::rgb4(8, 8, 9), gs::rgb4(2, 4, 8), gs::rgb4(3, 5, 9), gs::rgb4(6, 8, 12), gs::rgb4(15, 12, 3),
            gs::rgb4(12, 12, 13)});

    const float banks[5] = {-0.55f, -0.28f, 0.f, 0.28f, 0.55f};
    for (int i = 0; i < 5; i++) art.car[i] = gs::uploadImage(vdp, carArt(banks[i]));
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.pylon = gs::uploadImage(vdp, pylonArt());
    for (int i = 0; i < 3; i++) art.block[i] = gs::uploadImage(vdp, blockArt(i));

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    for (int c = 0; c < 96; c++) {
        std::string s(1, char(32 + c));
        art.glyph[c] = gs::uploadImage(vdp, gs::textBitmap(s, st));
    }
    vdp.setFogColor(gs::rgb4(6, 7, 10));
}

}  // namespace railturn
