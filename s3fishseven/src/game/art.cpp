#include "game/art.h"

#include <cmath>
#include <string>

namespace fishseven {

static void pal(gs::VDP& vdp, int p, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, c[i]);
}

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t z = gs::rgb4(0, 0, 0);
    const uint16_t fish[] = {z, gs::rgb4(12, 8, 2), gs::rgb4(3, 2, 1), gs::rgb4(15, 14, 6), z, z, z, z,
                             z, z, z, z, z, z, z, z};
    const uint16_t rival[] = {z, gs::rgb4(6, 10, 12), gs::rgb4(2, 3, 4), gs::rgb4(14, 15, 15), z, z, z, z,
                              z, z, z, z, z, z, z, z};
    const uint16_t boat[] = {z, gs::rgb4(9, 4, 1), gs::rgb4(3, 2, 1), gs::rgb4(13, 8, 3), gs::rgb4(14, 12, 8),
                             z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t lure[] = {z, gs::rgb4(15, 4, 2), gs::rgb4(15, 14, 4), gs::rgb4(2, 2, 2), z, z, z, z,
                             z, z, z, z, z, z, z, z};
    const uint16_t reed[] = {z, gs::rgb4(2, 8, 3), gs::rgb4(5, 12, 4), gs::rgb4(1, 4, 1), z, z, z, z,
                             z, z, z, z, z, z, z, z};
    const uint16_t sun[] = {z, gs::rgb4(15, 14, 4), gs::rgb4(15, 9, 2), z, z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t hud[] = {z, gs::rgb4(15, 15, 13), z, z, z, z, z, z, z, z, z, z, z, z, z, z};
    const uint16_t ink[] = {z, gs::rgb4(15, 13, 4), z, z, z, z, z, z, z, z, z, z, z, z, z, z};
    pal(vdp, PAL_FISH, fish);
    pal(vdp, PAL_RIVAL, rival);
    pal(vdp, PAL_BOAT, boat);
    pal(vdp, PAL_LURE, lure);
    pal(vdp, PAL_REED, reed);
    pal(vdp, PAL_SUN, sun);
    pal(vdp, PAL_HUD, hud);
    pal(vdp, PAL_INK, ink);
    vdp.setFogColor(gs::rgb4(3, 7, 11));

    gs::Bitmap fb(36, 16);
    fb.ellipse(14, 8, 11, 5.5f, 1);
    fb.poly({{22, 8}, {34, 2}, {34, 14}}, 1);
    fb.ellipse(8, 6, 1.5f, 1.5f, 2);
    fb.set(7, 5, 3);
    fb.line(10, 8, 20, 8, 3, 1);
    art.fish = gs::uploadImage(vdp, fb);

    gs::Bitmap boatB(52, 14);
    boatB.poly({{2, 6}, {8, 12}, {44, 12}, {50, 6}, {42, 4}, {10, 4}}, 1);
    boatB.rect(10, 5, 32, 3, 3);
    boatB.rect(22, 1, 8, 4, 4);
    art.boat = gs::uploadImage(vdp, boatB);

    gs::Bitmap ang(12, 18);
    ang.rect(4, 1, 4, 4, 4);
    ang.rect(3, 6, 6, 7, 1);
    ang.line(8, 7, 11, 2, 3, 1);
    ang.rect(3, 13, 2, 4, 2);
    ang.rect(7, 13, 2, 4, 2);
    art.angler = gs::uploadImage(vdp, ang);

    gs::Bitmap lureB(6, 9);
    lureB.ellipse(3, 3, 2.2f, 2.6f, 1);
    lureB.ellipse(3, 7, 1.1f, 1.4f, 2);
    lureB.set(3, 1, 3);
    art.lure = gs::uploadImage(vdp, lureB);

    gs::Bitmap reedB(8, 32);
    reedB.line(2, 31, 1, 6, 1, 1);
    reedB.line(5, 31, 7, 3, 2, 1);
    reedB.ellipse(1, 6, 2, 3.5f, 3);
    art.reed = gs::uploadImage(vdp, reedB);

    gs::Bitmap sunB(18, 18);
    sunB.ellipse(9, 9, 5, 5, 1);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        float c = std::cos(a), s = std::sin(a);
        sunB.line(9 + c * 6.5f, 9 + s * 6.5f, 9 + c * 8.4f, 9 + s * 8.4f, 2, 1);
    }
    art.sun = gs::uploadImage(vdp, sunB);

    for (int i = 0; i < 96; i++) {
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        st.spacing = 0;
        art.glyph[i] = gs::uploadImage(vdp, gs::textBitmap(std::string(1, char(32 + i)), st));
    }
}

}  // namespace fishseven
