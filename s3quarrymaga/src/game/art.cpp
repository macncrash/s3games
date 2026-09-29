#include "game/art.h"

#include <string>

namespace qmaga {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : c[0]);
}

void figure(gs::Bitmap& b, int frame, int coat, int hat) {
    b.ellipse(10, 4, 3.0f, 2.6f, 1);
    b.rect(7, 1, 6, 3, hat);
    b.rect(6, 7, 8, 10, coat);
    b.rect(3, 8, 3, 6, coat);
    b.rect(14, 8, 3, 6, 3);
    int step = frame & 1;
    b.rect(7, 17, 3, 8 - step, 5);
    b.rect(11, 17, 3, 7 + step, 5);
    b.set(9, 4, 3);
    b.set(11, 4, 3);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 8), gs::rgb4(5, 5, 4), gs::rgb4(15, 15, 12)};
    const uint16_t lime[] = {gs::rgb4(0, 0, 0), gs::rgb4(10, 9, 6), gs::rgb4(7, 7, 5), gs::rgb4(4, 4, 3),
                             gs::rgb4(13, 12, 8), gs::rgb4(3, 3, 2), gs::rgb4(8, 9, 7)};
    const uint16_t cranePal[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 8, 2), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4),
                              gs::rgb4(14, 12, 4)};
    const uint16_t brass[] = {gs::rgb4(0, 0, 0), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7), gs::rgb4(8, 6, 2)};
    const uint16_t cut[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2),
                            gs::rgb4(11, 8, 2), gs::rgb4(3, 2, 2), gs::rgb4(9, 3, 2)};
    const uint16_t dust[] = {gs::rgb4(0, 0, 0), gs::rgb4(11, 10, 8), gs::rgb4(6, 7, 6), gs::rgb4(3, 3, 3),
                             gs::rgb4(8, 8, 6), gs::rgb4(4, 4, 3)};
    const uint16_t fx[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 8), gs::rgb4(15, 7, 2), gs::rgb4(9, 9, 8)};
    const uint16_t alert[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {gs::rgb4(0, 0, 0), gs::rgb4(5, 13, 6), gs::rgb4(12, 15, 10)};
    const uint16_t watch[] = {gs::rgb4(0, 0, 0), gs::rgb4(9, 8, 6), gs::rgb4(4, 5, 7), gs::rgb4(14, 10, 2),
                              gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 4)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_LIME, lime, 7);
    pal(vdp, PAL_CRANE, cranePal, 5);
    pal(vdp, PAL_BRASS, brass, 4);
    pal(vdp, PAL_CUT, cut, 7);
    pal(vdp, PAL_DUST, dust, 6);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    pal(vdp, PAL_WATCH, watch, 6);
    vdp.setFogColor(gs::rgb4(6, 6, 5));

    gs::Bitmap bench(72, 16);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 72; x++) {
            int c = ((x / 6 + y) & 1) ? 1 : 2;
            if (y < 2 || y > 13) c = 3;
            if ((x % 18) == 0) c = 5;
            bench.set(x, y, c);
        }
    bench.rect(4, 6, 10, 3, 4);
    art.bench = gs::uploadMipped(vdp, bench);

    gs::Bitmap crane(96, 28);
    crane.rect(8, 22, 10, 6, 2);
    crane.rect(11, 6, 4, 18, 2);
    crane.line(13, 8, 88, 4, 1, 2);
    crane.line(13, 10, 70, 16, 3, 1);
    crane.rect(84, 4, 6, 8, 4);
    crane.line(87, 12, 87, 24, 2, 1);
    art.crane = gs::uploadMipped(vdp, crane);

    gs::Bitmap crusher(40, 22);
    crusher.rect(2, 8, 36, 12, 2);
    crusher.rect(6, 2, 28, 8, 3);
    crusher.rect(10, 12, 8, 6, 1);
    crusher.rect(22, 12, 8, 6, 4);
    art.crusher = gs::uploadMipped(vdp, crusher);

    gs::Bitmap hat(16, 22);
    figure(hat, 0, 2, 3);
    hat.rect(8, 9, 2, 6, 4);
    art.hardhat = gs::uploadMipped(vdp, hat);

    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(20, 26);
        figure(b, f, 2, 4);
        art.cut[f] = gs::uploadMipped(vdp, b);
        gs::Bitmap d(20, 26);
        figure(d, f, 2, 4);
        d.rect(15, 10, 4, 3, 3);
        art.dust[f] = gs::uploadMipped(vdp, d);
    }

    gs::Bitmap down(22, 10);
    down.ellipse(7, 5, 4, 3, 2);
    down.rect(10, 3, 10, 4, 4);
    down.rect(2, 6, 5, 2, 5);
    art.down = gs::uploadMipped(vdp, down);

    gs::Bitmap round(6, 14);
    round.rect(1, 2, 4, 10, 1);
    round.rect(1, 1, 4, 2, 2);
    round.rect(2, 11, 2, 2, 3);
    art.brass = gs::uploadMipped(vdp, round);

    gs::Bitmap chev(16, 8);
    chev.line(1, 1, 8, 6, 1, 1);
    chev.line(15, 1, 8, 6, 1, 1);
    art.chev = gs::uploadMipped(vdp, chev);

    gs::Bitmap flash(14, 10);
    flash.ellipse(7, 5, 6, 4, 1);
    flash.ellipse(7, 5, 2.4f, 1.6f, 2);
    art.flash = gs::uploadMipped(vdp, flash);

    gs::Bitmap puff(12, 10);
    puff.ellipse(6, 5, 5, 3.2f, 3);
    puff.ellipse(5, 4, 2, 1.4f, 1);
    art.puff = gs::uploadMipped(vdp, puff);

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

}  // namespace qmaga
