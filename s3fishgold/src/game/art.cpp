#include "game/art.h"

#include <cmath>
#include <string>

namespace fishgold {

static void pal(gs::VDP& vdp, int p, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, c[i]);
}

static gs::Bitmap fishShape(int kind) {
    int w = kind == KIND_MINNOW ? 22 : 40;
    int h = kind == KIND_MINNOW ? 12 : 18;
    gs::Bitmap b(w, h);
    float cx = w * 0.42f;
    float cy = h * 0.5f;
    b.ellipse(cx, cy, w * 0.32f, h * 0.36f, 1);
    b.poly({{cx + w * 0.22f, cy}, {float(w - 1), 1.f}, {float(w - 1), float(h - 2)}}, 1);
    b.ellipse(cx - w * 0.16f, cy - h * 0.12f, 1.6f, 1.6f, 2);
    b.set(int(cx - w * 0.18f), int(cy - h * 0.16f), 3);
    if (kind == KIND_GOLD) b.rect(cx - 4, cy - 1, 10, 2, 3);
    if (kind == KIND_CREAM) b.ellipse(cx + 2, cy + 1, 3, 2, 3);
    if (kind == KIND_SILVER) b.line(cx - 6, cy, cx + 8, cy, 3, 1);
    return b;
}

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t z = gs::rgb4(0, 0, 0);
    const uint16_t gold[] = {z, gs::rgb4(14, 11, 2), gs::rgb4(4, 2, 0), gs::rgb4(15, 15, 8), z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t cream[] = {z, gs::rgb4(14, 13, 9), gs::rgb4(5, 3, 1), gs::rgb4(15, 15, 12), z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t silver[] = {z, gs::rgb4(10, 12, 13), gs::rgb4(2, 3, 4), gs::rgb4(15, 15, 15), z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t minnow[] = {z, gs::rgb4(6, 10, 6), gs::rgb4(1, 2, 1), gs::rgb4(12, 14, 8), z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t boat[] = {z, gs::rgb4(8, 4, 1), gs::rgb4(3, 2, 1), gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 8), z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t lure[] = {z, gs::rgb4(15, 4, 2), gs::rgb4(15, 14, 4), gs::rgb4(2, 2, 2), z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t reed[] = {z, gs::rgb4(2, 8, 3), gs::rgb4(4, 11, 4), gs::rgb4(1, 4, 1), z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t sun[] = {z, gs::rgb4(15, 14, 4), gs::rgb4(15, 10, 3), z, z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t hud[] = {z, gs::rgb4(15, 15, 13), gs::rgb4(2, 3, 4), z, z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t alert[] = {z, gs::rgb4(15, 8, 3), z, z, z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t ink[] = {z, gs::rgb4(15, 13, 4), z, z, z, z, z, z, z, z, z, z, z, z, z, z};
    pal(vdp, PAL_GOLD, gold);
    pal(vdp, PAL_CREAM, cream);
    pal(vdp, PAL_SILVER, silver);
    pal(vdp, PAL_MINNOW, minnow);
    pal(vdp, PAL_BOAT, boat);
    pal(vdp, PAL_LURE, lure);
    pal(vdp, PAL_REED, reed);
    pal(vdp, PAL_SUN, sun);
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_ALERT, alert);
    pal(vdp, PAL_INK, ink);
    vdp.setFogColor(gs::rgb4(4, 8, 12));

    for (int k = 0; k < 4; k++) art.fish[k] = gs::uploadImage(vdp, fishShape(k));

    gs::Bitmap boatB(56, 16);
    boatB.poly({{2, 8}, {8, 14}, {48, 14}, {54, 8}, {46, 6}, {10, 6}}, 1);
    boatB.rect(8, 7, 40, 3, 3);
    boatB.rect(24, 3, 8, 4, 4);
    art.boat = gs::uploadImage(vdp, boatB);

    gs::Bitmap ang(14, 22);
    ang.rect(6, 2, 4, 4, 4);
    ang.rect(5, 7, 6, 8, 1);
    ang.rect(2, 8, 4, 2, 3);
    ang.line(10, 9, 13, 4, 3, 1);
    ang.rect(5, 15, 2, 6, 2);
    ang.rect(9, 15, 2, 6, 2);
    art.angler = gs::uploadImage(vdp, ang);

    gs::Bitmap lureB(7, 10);
    lureB.ellipse(3, 3, 2.4f, 3.2f, 1);
    lureB.ellipse(3, 7, 1.2f, 1.6f, 2);
    lureB.set(3, 1, 3);
    art.lure = gs::uploadImage(vdp, lureB);

    gs::Bitmap reedB(10, 36);
    reedB.line(3, 35, 2, 4, 1, 1);
    reedB.line(6, 35, 8, 2, 2, 1);
    reedB.ellipse(2, 5, 2.2f, 4, 3);
    reedB.ellipse(8, 3, 2.2f, 4, 2);
    art.reed = gs::uploadImage(vdp, reedB);

    gs::Bitmap sunB(22, 22);
    sunB.ellipse(11, 11, 6, 6, 1);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        float c = std::cos(a), s = std::sin(a);
        sunB.line(11 + c * 8, 11 + s * 8, 11 + c * 10, 11 + s * 10, 2, 1);
    }
    art.sun = gs::uploadImage(vdp, sunB);

    gs::Bitmap spl(16, 10);
    spl.ellipse(8, 6, 6, 2, 1);
    spl.line(8, 6, 4, 1, 2, 1);
    spl.line(8, 6, 12, 0, 2, 1);
    art.splash = gs::uploadImage(vdp, spl);

    for (int i = 0; i < 96; i++) {
        char ch = char(32 + i);
        std::string s(1, ch);
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        st.spacing = 0;
        art.glyph[i] = gs::uploadImage(vdp, gs::textBitmap(s, st));
    }
}

}  // namespace fishgold
