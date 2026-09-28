#include "game/art.h"

#include <string>

namespace bmaga {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : c[0]);
}

void man(gs::Bitmap& b, int frame, int coat, int helm) {
    b.ellipse(10, 5, 3.2f, 3.2f, 1);
    b.rect(7, 7, 6, 3, helm);
    b.rect(6, 10, 8, 10, coat);
    b.rect(4, 11, 3, 7, coat);
    b.rect(13, 11, 3, 7, 3);
    int step = frame & 1;
    b.rect(7, 20, 3, 7 - step, 5);
    b.rect(11, 20, 3, 6 + step, 5);
    b.set(9, 5, 3);
    b.set(11, 5, 3);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 12, 6), gs::rgb4(6, 5, 4), gs::rgb4(15, 15, 13),
                            gs::rgb4(2, 2, 2)};
    const uint16_t stone[] = {gs::rgb4(0, 0, 0), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4),
                              gs::rgb4(11, 10, 8), gs::rgb4(2, 2, 3)};
    const uint16_t field[] = {gs::rgb4(0, 0, 0), gs::rgb4(6, 5, 3), gs::rgb4(4, 6, 3), gs::rgb4(8, 7, 4),
                              gs::rgb4(3, 3, 2), gs::rgb4(10, 9, 6), gs::rgb4(2, 3, 2)};
    const uint16_t brass[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 6), gs::rgb4(7, 5, 2),
                              gs::rgb4(14, 4, 3)};
    const uint16_t foe[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 8, 6), gs::rgb4(4, 5, 3), gs::rgb4(2, 2, 2),
                            gs::rgb4(6, 7, 4), gs::rgb4(3, 2, 2), gs::rgb4(9, 3, 2)};
    const uint16_t peel[] = {gs::rgb4(0, 0, 0), gs::rgb4(11, 9, 7), gs::rgb4(5, 6, 6), gs::rgb4(2, 3, 3),
                             gs::rgb4(7, 8, 8), gs::rgb4(3, 3, 3)};
    const uint16_t fx[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(8, 8, 7)};
    const uint16_t alert[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {gs::rgb4(0, 0, 0), gs::rgb4(6, 13, 6), gs::rgb4(12, 15, 10)};
    pal(vdp, PAL_HUD, hud, 5);
    pal(vdp, PAL_STONE, stone, 6);
    pal(vdp, PAL_FIELD, field, 7);
    pal(vdp, PAL_BRASS, brass, 5);
    pal(vdp, PAL_FOE, foe, 7);
    pal(vdp, PAL_PEEL, peel, 6);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    vdp.setFogColor(gs::rgb4(3, 3, 4));

    gs::Bitmap stoneB(64, 40);
    for (int y = 0; y < 40; y++)
        for (int x = 0; x < 64; x++) {
            int c = ((x / 8 + y / 8) & 1) ? 1 : 2;
            if ((x % 8) == 0 || (y % 8) == 0) c = 3;
            if (y > 34) c = 5;
            stoneB.set(x, y, c);
        }
    art.stone = gs::uploadMipped(vdp, stoneB);

    gs::Bitmap pillar(14, 40);
    for (int y = 0; y < 40; y++)
        for (int x = 0; x < 14; x++) {
            int c = (x < 2 || x > 11) ? 5 : ((y / 8) & 1) ? 1 : 2;
            if (y % 8 == 0) c = 3;
            pillar.set(x, y, c);
        }
    art.pillar = gs::uploadMipped(vdp, pillar);

    gs::Bitmap fieldB(80, 48);
    for (int y = 0; y < 48; y++)
        for (int x = 0; x < 80; x++) {
            int c = 1;
            if (((x + y * 3) / 7) % 5 == 0) c = 2;
            if (y > 36 && (x % 11) < 2) c = 4;
            if (y < 8) c = 3;
            fieldB.set(x, y, c);
        }
    art.field = gs::uploadMipped(vdp, fieldB);

    gs::Bitmap wire(16, 28);
    wire.rect(7, 6, 2, 22, 5);
    wire.line(1, 8, 15, 10, 4, 1);
    wire.line(1, 14, 15, 16, 4, 1);
    wire.line(1, 20, 15, 22, 4, 1);
    art.wire = gs::uploadMipped(vdp, wire);

    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(20, 28);
        man(b, f, 2, 4);
        art.foe[f] = gs::uploadMipped(vdp, b);
        gs::Bitmap p(20, 28);
        man(p, f, 2, 4);
        p.rect(14, 12, 5, 2, 3);
        art.peel[f] = gs::uploadMipped(vdp, p);
    }
    gs::Bitmap down(22, 10);
    down.ellipse(8, 5, 4, 3, 2);
    down.rect(11, 3, 9, 4, 4);
    down.rect(2, 6, 6, 2, 5);
    art.down = gs::uploadMipped(vdp, down);

    gs::Bitmap round(6, 14);
    round.rect(1, 2, 4, 10, 1);
    round.rect(1, 1, 4, 2, 2);
    round.rect(2, 11, 2, 2, 3);
    art.brass = gs::uploadMipped(vdp, round);

    gs::Bitmap chev(14, 8);
    chev.line(1, 1, 7, 6, 1, 1);
    chev.line(13, 1, 7, 6, 1, 1);
    art.chev = gs::uploadMipped(vdp, chev);

    gs::Bitmap flash(12, 8);
    flash.ellipse(6, 4, 5, 3, 1);
    flash.ellipse(6, 4, 2, 1.4f, 2);
    art.flash = gs::uploadMipped(vdp, flash);

    gs::Bitmap puff(10, 10);
    puff.ellipse(5, 5, 4, 3, 3);
    puff.ellipse(4, 4, 2, 1.5f, 1);
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

}  // namespace bmaga
