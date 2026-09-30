#include "game/art.h"

#include <string>

namespace cmaga {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : c[0]);
}

void walker(gs::Bitmap& b, int frame, int coat, int head, bool lamp) {
    b.ellipse(11, 5, 3.1f, 3.0f, head);
    b.rect(8, 8, 6, 9, coat);
    b.rect(5, 9, 3, 6, coat);
    int step = frame & 1;
    b.rect(8, 17, 3, 8 - step, 5);
    b.rect(12, 17, 3, 7 + step, 5);
    b.set(10, 5, 3);
    b.set(12, 5, 3);
    if (lamp) {
        b.rect(15, 10, 5, 3, 4);
        b.ellipse(21, 11, 2.2f, 2.2f, 2);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 13, 11), gs::rgb4(4, 5, 5), gs::rgb4(15, 15, 13)};
    const uint16_t pipe[] = {gs::rgb4(0, 0, 0), gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(3, 3, 4),
                             gs::rgb4(12, 12, 11), gs::rgb4(2, 2, 3)};
    const uint16_t moss[] = {gs::rgb4(0, 0, 0), gs::rgb4(3, 6, 3), gs::rgb4(5, 8, 4), gs::rgb4(2, 3, 2),
                             gs::rgb4(8, 10, 5)};
    const uint16_t water[] = {gs::rgb4(0, 0, 0), gs::rgb4(2, 4, 6), gs::rgb4(4, 7, 9), gs::rgb4(8, 10, 11),
                              gs::rgb4(1, 2, 3)};
    const uint16_t lamp[] = {gs::rgb4(0, 0, 0), gs::rgb4(10, 8, 6), gs::rgb4(15, 13, 4), gs::rgb4(2, 2, 2),
                             gs::rgb4(5, 4, 3), gs::rgb4(3, 2, 2), gs::rgb4(7, 3, 2)};
    const uint16_t shade[] = {gs::rgb4(0, 0, 0), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3),
                              gs::rgb4(9, 9, 8), gs::rgb4(3, 3, 4)};
    const uint16_t brass[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 6), gs::rgb4(7, 5, 2)};
    const uint16_t alert[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {gs::rgb4(0, 0, 0), gs::rgb4(6, 13, 6), gs::rgb4(12, 15, 10)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_PIPE, pipe, 6);
    pal(vdp, PAL_MOSS, moss, 5);
    pal(vdp, PAL_WATER, water, 5);
    pal(vdp, PAL_LAMP, lamp, 7);
    pal(vdp, PAL_SHADE, shade, 6);
    pal(vdp, PAL_BRASS, brass, 4);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    vdp.setFogColor(gs::rgb4(1, 2, 3));

    gs::Bitmap ring(96, 64);
    ring.ellipse(48, 32, 44, 28, 1);
    ring.ellipse(48, 32, 34, 20, 0);
    for (int y = 8; y < 56; y++)
        for (int x = 10; x < 86; x++) {
            if (ring.get(x, y) != 1) continue;
            if (((x / 6) + (y / 8)) % 3 == 0) ring.set(x, y, 2);
            if (x < 18 || x > 78) ring.set(x, y, 3);
        }
    ring.ellipse(48, 18, 8, 3, 4);
    art.ring = gs::uploadMipped(vdp, ring);

    gs::Bitmap rib(10, 48);
    for (int y = 0; y < 48; y++)
        for (int x = 0; x < 10; x++) {
            int c = (x < 2 || x > 7) ? 3 : 1;
            if (y % 12 < 2) c = 5;
            rib.set(x, y, c);
        }
    art.rib = gs::uploadMipped(vdp, rib);

    gs::Bitmap drip(4, 16);
    drip.line(2, 0, 2, 12, 2, 1);
    drip.ellipse(2, 14, 1.4f, 1.6f, 3);
    art.drip = gs::uploadMipped(vdp, drip);

    gs::Bitmap waterB(80, 16);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 80; x++) {
            int c = 1;
            if ((x + y * 5) % 11 < 2) c = 2;
            if (y < 3) c = 4;
            if (y > 12) c = 3;
            waterB.set(x, y, c);
        }
    art.water = gs::uploadMipped(vdp, waterB);

    gs::Bitmap grate(40, 28);
    for (int i = 0; i < 5; i++) grate.line(4.f + i * 8, 2, 4.f + i * 8, 26, 1, 1.4f);
    grate.line(2, 6, 38, 6, 2, 1.2f);
    grate.line(2, 16, 38, 16, 2, 1.2f);
    grate.line(2, 24, 38, 24, 2, 1.2f);
    art.grate = gs::uploadMipped(vdp, grate);

    for (int f = 0; f < 2; f++) {
        gs::Bitmap a(24, 28);
        walker(a, f, 1, 1, true);
        art.lamp[f] = gs::uploadMipped(vdp, a);
        gs::Bitmap s(24, 28);
        walker(s, f, 2, 4, false);
        art.shade[f] = gs::uploadMipped(vdp, s);
    }

    gs::Bitmap down(26, 10);
    down.ellipse(8, 5, 5, 3, 1);
    down.rect(12, 3, 10, 3, 4);
    down.ellipse(22, 4, 2, 2, 2);
    art.down = gs::uploadMipped(vdp, down);

    gs::Bitmap round(6, 14);
    round.rect(1, 2, 4, 10, 1);
    round.rect(1, 1, 4, 2, 2);
    round.rect(2, 11, 2, 2, 3);
    art.brass = gs::uploadMipped(vdp, round);

    gs::Bitmap muzzle(18, 10);
    muzzle.ellipse(9, 5, 8, 4, 1);
    muzzle.ellipse(9, 5, 3, 2, 2);
    art.muzzle = gs::uploadMipped(vdp, muzzle);

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 0;
    for (int c = 32; c < 96; c++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(c)), st);
        art.gw[c] = g.w > 0 ? g.w : 4;
        art.gh = g.h > 0 ? g.h : 8;
        art.glyph[c] = gs::uploadImage(vdp, g);
    }
}

}  // namespace cmaga
