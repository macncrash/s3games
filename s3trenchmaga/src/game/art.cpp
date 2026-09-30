#include "game/art.h"

#include <string>

namespace tmaga {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : c[0]);
}

void soldier(gs::Bitmap& b, int frame, int coat, int helm) {
    b.ellipse(11, 5, 4.2f, 3.4f, helm);
    b.rect(8, 7, 6, 2, 3);
    b.rect(7, 9, 8, 11, coat);
    b.rect(5, 11, 3, 6, coat);
    b.rect(14, 12, 6, 2, 6);
    int step = frame & 1;
    b.rect(8, 20, 3, 8 - step, 5);
    b.rect(12, 20, 3, 7 + step, 5);
    b.set(10, 5, 3);
    b.set(13, 5, 3);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 9), gs::rgb4(4, 4, 3), gs::rgb4(15, 15, 12)};
    const uint16_t bag[] = {gs::rgb4(0, 0, 0), gs::rgb4(7, 6, 3), gs::rgb4(10, 8, 4), gs::rgb4(4, 4, 2),
                            gs::rgb4(12, 10, 6), gs::rgb4(3, 3, 2)};
    const uint16_t mud[] = {gs::rgb4(0, 0, 0), gs::rgb4(5, 4, 2), gs::rgb4(3, 4, 2), gs::rgb4(8, 7, 4),
                            gs::rgb4(2, 2, 1), gs::rgb4(6, 5, 3)};
    const uint16_t brass[] = {gs::rgb4(0, 0, 0), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7), gs::rgb4(8, 6, 2),
                              gs::rgb4(14, 5, 3)};
    const uint16_t foe[] = {gs::rgb4(0, 0, 0), gs::rgb4(11, 9, 7), gs::rgb4(4, 5, 3), gs::rgb4(2, 2, 2),
                            gs::rgb4(6, 6, 4), gs::rgb4(3, 2, 2), gs::rgb4(8, 3, 2)};
    const uint16_t peel[] = {gs::rgb4(0, 0, 0), gs::rgb4(9, 10, 8), gs::rgb4(5, 6, 5), gs::rgb4(2, 3, 3),
                             gs::rgb4(7, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(4, 5, 4)};
    const uint16_t fx[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 10), gs::rgb4(15, 9, 3), gs::rgb4(9, 9, 8),
                           gs::rgb4(12, 12, 12)};
    const uint16_t alert[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {gs::rgb4(0, 0, 0), gs::rgb4(7, 13, 6), gs::rgb4(13, 15, 10)};
    const uint16_t road[] = {
        gs::rgb4(2, 2, 1), gs::rgb4(4, 5, 2), gs::rgb4(3, 4, 2), gs::rgb4(6, 5, 3), gs::rgb4(5, 4, 2),
        gs::rgb4(7, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(3, 3, 2),
        gs::rgb4(7, 6, 3), gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5), gs::rgb4(4, 5, 6), gs::rgb4(9, 8, 5),
        gs::rgb4(7, 6, 4)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_BAG, bag, 6);
    pal(vdp, PAL_MUD, mud, 6);
    pal(vdp, PAL_BRASS, brass, 5);
    pal(vdp, PAL_FOE, foe, 7);
    pal(vdp, PAL_PEEL, peel, 7);
    pal(vdp, PAL_FX, fx, 5);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    pal(vdp, PAL_ROAD, road, 16);
    vdp.setFogColor(gs::rgb4(5, 5, 6));

    gs::Bitmap bagB(56, 28);
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 3; col++) {
            float cx = 10.f + col * 18.f + (row ? 8.f : 0.f);
            float cy = 8.f + row * 12.f;
            bagB.ellipse(cx, cy, 8.5f, 6.f, (col + row) & 1 ? 1 : 2);
            bagB.ellipse(cx, cy - 1.f, 5.f, 3.f, 4);
            bagB.line(cx - 6, cy, cx + 6, cy, 3, 1);
        }
    }
    art.bag = gs::uploadMipped(vdp, bagB);

    gs::Bitmap plank(72, 14);
    for (int y = 0; y < 14; y++)
        for (int x = 0; x < 72; x++) {
            int c = 1;
            if ((x / 10) % 2 == 0) c = 2;
            if (y < 2 || y > 11) c = 3;
            if (x % 10 == 0) c = 5;
            plank.set(x, y, c);
        }
    art.plank = gs::uploadMipped(vdp, plank);

    gs::Bitmap post(8, 36);
    post.rect(3, 0, 2, 36, 5);
    post.rect(1, 2, 6, 3, 3);
    art.post = gs::uploadMipped(vdp, post);

    gs::Bitmap wire(28, 16);
    wire.line(0, 3, 27, 5, 4, 1);
    wire.line(0, 8, 27, 7, 4, 1);
    wire.line(0, 13, 27, 11, 3, 1);
    art.wire = gs::uploadMipped(vdp, wire);

    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(22, 30);
        soldier(b, f, 2, 4);
        art.foe[f] = gs::uploadMipped(vdp, b);
        gs::Bitmap p(22, 30);
        soldier(p, f, 2, 1);
        p.rect(15, 13, 6, 2, 3);
        art.peel[f] = gs::uploadMipped(vdp, p);
    }

    gs::Bitmap down(24, 12);
    down.ellipse(8, 6, 5, 3.2f, 2);
    down.rect(12, 4, 10, 4, 4);
    down.rect(2, 7, 6, 2, 5);
    art.down = gs::uploadMipped(vdp, down);

    gs::Bitmap round(6, 16);
    round.rect(1, 3, 4, 11, 1);
    round.rect(1, 1, 4, 3, 2);
    round.rect(2, 13, 2, 2, 3);
    art.brass = gs::uploadMipped(vdp, round);

    gs::Bitmap notch(16, 10);
    notch.line(1, 1, 8, 8, 1, 1);
    notch.line(15, 1, 8, 8, 1, 1);
    notch.rect(7, 2, 2, 4, 3);
    art.notch = gs::uploadMipped(vdp, notch);

    gs::Bitmap flash(14, 10);
    flash.ellipse(7, 5, 6, 4, 1);
    flash.ellipse(7, 5, 3, 2, 2);
    art.flash = gs::uploadMipped(vdp, flash);

    gs::Bitmap stab(18, 6);
    stab.line(0, 3, 16, 3, 4, 2);
    stab.line(12, 1, 17, 3, 1, 1);
    stab.line(12, 5, 17, 3, 1, 1);
    art.stab = gs::uploadMipped(vdp, stab);

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

}  // namespace tmaga
