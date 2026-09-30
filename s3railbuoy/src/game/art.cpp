#include "game/art.h"

#include <cmath>
#include <cstring>
#include <initializer_list>

namespace railbuoy {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void stamp(gs::Bitmap& b, float lx, float ly, float ang, int c) {
    float x = 20.f + lx * std::cos(ang) - ly * std::sin(ang);
    float y = 20.f + lx * std::sin(ang) + ly * std::cos(ang);
    b.set(int(x), int(y), c);
}

gs::Bitmap carArt(int dir) {
    gs::Bitmap b(40, 40);
    float ang = dir * 0.78539816f;
    for (int ly = -6; ly <= 6; ly++) {
        for (int lx = -15; lx <= 14; lx++) {
            int c = 2;
            if (lx > 8) c = 4;
            if (lx > 6 && lx < 12 && ly > -4 && ly < 4) c = 5;
            if (ly == -6 || ly == 6 || lx == -15) c = 1;
            if (lx < -12 && (ly == -4 || ly == 4)) c = 6;
            if (lx > 12 && std::abs(ly) < 3) c = 7;
            stamp(b, float(lx), float(ly), ang, c);
        }
    }
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(26, 30);
    b.rect(12, 20, 2, 8, 4);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 11, 4, 2);
    b.ellipse(13, 12, 3, 3, 3);
    b.ellipse(9, 8, 2, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap dockArt(const char* name, bool home) {
    gs::Bitmap b(88, 46);
    b.rect(0, 18, 88, 22, home ? 3 : 4);
    b.rect(0, 18, 88, 3, home ? 2 : 5);
    b.rect(0, 36, 88, 4, 6);
    for (int x = 6; x < 84; x += 14) b.rect(x, 22, 3, 14, 6);
    b.rect(8, 6, 4, 16, 7);
    b.rect(76, 6, 4, 16, 7);
    b.poly({{6, 10}, {44, 0}, {82, 10}}, home ? 2 : 5);
    b.rect(22, 12, 44, 12, 8);
    gs::Bitmap label = gs::textBitmap(name, {1, 1, 1, 0, 0});
    b.blit(label, 22 + (44 - label.w) / 2, 15);
    if (home) b.ellipse(44, 40, 3, 2, 9);
    else b.rect(40, 40, 8, 3, 9);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(10, 10);
    b.rect(1, 4, 8, 3, 1);
    b.rect(2, 2, 2, 6, 2);
    b.rect(6, 2, 2, 6, 2);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(14, 8);
    b.ellipse(4, 4, 3, 2, 1);
    b.ellipse(10, 4, 3, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(10, 12, 13), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_BUOY,
           {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 14), gs::rgb4(15, 14, 4), gs::rgb4(6, 5, 3), gs::rgb4(15, 15, 15),
            gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(13, 12, 8), gs::rgb4(8, 7, 5), gs::rgb4(12, 3, 2), gs::rgb4(6, 10, 13),
            gs::rgb4(4, 4, 5), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_DOCK,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(12, 8, 4), gs::rgb4(9, 6, 3), gs::rgb4(7, 5, 2), gs::rgb4(14, 10, 5),
            gs::rgb4(5, 4, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 13, 10), gs::rgb4(4, 12, 6)});
    setPal(vdp, PAL_FAR,
           {0, gs::rgb4(4, 1, 1), gs::rgb4(10, 4, 3), gs::rgb4(8, 3, 2), gs::rgb4(12, 5, 4), gs::rgb4(14, 3, 2),
            gs::rgb4(6, 2, 2), gs::rgb4(5, 2, 2), gs::rgb4(12, 10, 9), gs::rgb4(15, 12, 2)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(7, 7, 8), gs::rgb4(12, 12, 13)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2)});

    for (int i = 0; i < 8; i++) art.car[i] = gs::uploadImage(vdp, carArt(i));
    art.buoy = gs::uploadImage(vdp, buoyArt());
    art.dock = gs::uploadImage(vdp, dockArt("HOME", true));
    art.far = gs::uploadImage(vdp, dockArt("FAR", false));
    art.sleeper = gs::uploadImage(vdp, sleeperArt());
    art.foam = gs::uploadImage(vdp, foamArt());
    for (int i = 0; i < 96; i++) {
        char ch[2] = {char(32 + i), 0};
        art.font[i] = gs::uploadImage(vdp, gs::textBitmap(ch, {1, 1, 1, 0, 0}));
    }
    vdp.setFogColor(gs::rgb4(2, 4, 8));
}

}  // namespace railbuoy
