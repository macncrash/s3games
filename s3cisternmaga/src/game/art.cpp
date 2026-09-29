#include "game/art.h"

#include <string>

namespace cmaga {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : c[0]);
}

void figure(gs::Bitmap& b, int frame, int cloth, int cap) {
    b.ellipse(9, 5, 3.1f, 3.0f, 1);
    b.rect(6, 7, 6, 2, cap);
    b.rect(5, 9, 8, 9, cloth);
    b.rect(3, 10, 3, 6, cloth);
    b.rect(12, 10, 3, 6, 3);
    int step = frame & 1;
    b.rect(6, 18, 3, 7 - step, 5);
    b.rect(10, 18, 3, 6 + step, 5);
    b.set(8, 5, 3);
    b.set(10, 5, 3);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(13, 14, 12), gs::rgb4(4, 5, 5), gs::rgb4(15, 15, 14)};
    const uint16_t lime[] = {gs::rgb4(0, 0, 0), gs::rgb4(8, 8, 7), gs::rgb4(11, 11, 9), gs::rgb4(5, 5, 4),
                             gs::rgb4(3, 4, 3), gs::rgb4(13, 12, 8), gs::rgb4(6, 7, 5)};
    const uint16_t water[] = {gs::rgb4(0, 0, 0), gs::rgb4(2, 5, 6), gs::rgb4(3, 8, 8), gs::rgb4(1, 3, 4),
                              gs::rgb4(6, 11, 10), gs::rgb4(4, 6, 6)};
    const uint16_t vault[] = {gs::rgb4(0, 0, 0), gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 7), gs::rgb4(2, 2, 3),
                              gs::rgb4(9, 8, 6)};
    const uint16_t raider[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 9, 6), gs::rgb4(6, 3, 2), gs::rgb4(2, 2, 2),
                               gs::rgb4(8, 5, 3), gs::rgb4(3, 2, 2), gs::rgb4(10, 3, 2)};
    const uint16_t wader[] = {gs::rgb4(0, 0, 0), gs::rgb4(9, 10, 9), gs::rgb4(3, 5, 5), gs::rgb4(1, 2, 2),
                              gs::rgb4(5, 7, 6), gs::rgb4(2, 3, 3)};
    const uint16_t brass[] = {gs::rgb4(0, 0, 0), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7), gs::rgb4(7, 5, 2)};
    const uint16_t fx[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 8), gs::rgb4(14, 7, 2), gs::rgb4(8, 12, 12)};
    const uint16_t alert[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {gs::rgb4(0, 0, 0), gs::rgb4(6, 13, 7), gs::rgb4(12, 15, 11)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_LIME, lime, 7);
    pal(vdp, PAL_WATER, water, 6);
    pal(vdp, PAL_VAULT, vault, 5);
    pal(vdp, PAL_RAIDER, raider, 7);
    pal(vdp, PAL_WADER, wader, 6);
    pal(vdp, PAL_BRASS, brass, 4);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    vdp.setFogColor(gs::rgb4(1, 2, 3));

    gs::Bitmap block(20, 12);
    for (int y = 0; y < 12; y++)
        for (int x = 0; x < 20; x++) {
            int c = ((x / 5 + y / 4) & 1) ? 1 : 2;
            if (x == 0 || y == 0) c = 3;
            if (y > 9) c = 4;
            block.set(x, y, c);
        }
    art.block = gs::uploadMipped(vdp, block);

    gs::Bitmap lip(28, 10);
    lip.rect(0, 2, 28, 6, 1);
    lip.rect(0, 7, 28, 2, 3);
    lip.rect(2, 3, 24, 2, 6);
    for (int x = 4; x < 28; x += 7) lip.line(float(x), 2, float(x), 8, 3, 1);
    art.lip = gs::uploadMipped(vdp, lip);

    gs::Bitmap waterB(48, 28);
    waterB.ellipse(24, 14, 22, 12, 1);
    waterB.ellipse(24, 13, 14, 7, 2);
    waterB.ellipse(18, 11, 4, 2, 4);
    waterB.ellipse(30, 16, 6, 2, 5);
    art.water = gs::uploadMipped(vdp, waterB);

    gs::Bitmap stair(16, 28);
    for (int i = 0; i < 6; i++) {
        int y = 2 + i * 4;
        int inset = i;
        stair.rect(inset, y, 16 - inset * 2, 3, (i & 1) ? 1 : 2);
        stair.line(float(inset), float(y), float(16 - inset), float(y), 3, 1);
    }
    art.stair = gs::uploadMipped(vdp, stair);

    gs::Bitmap lamp(10, 16);
    lamp.rect(4, 2, 2, 8, 4);
    lamp.ellipse(5, 12, 3.2f, 3.0f, 1);
    lamp.ellipse(5, 12, 1.4f, 1.2f, 2);
    art.lamp = gs::uploadMipped(vdp, lamp);

    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(18, 26);
        figure(b, f, 2, 4);
        art.raider[f] = gs::uploadMipped(vdp, b);
        gs::Bitmap w(18, 26);
        figure(w, f, 2, 4);
        w.rect(4, 16, 10, 4, 3);
        art.wader[f] = gs::uploadMipped(vdp, w);
    }

    gs::Bitmap sunk(20, 8);
    sunk.ellipse(8, 4, 5, 2.4f, 2);
    sunk.rect(11, 2, 7, 3, 4);
    sunk.ellipse(6, 5, 3, 1.2f, 5);
    art.sunk = gs::uploadMipped(vdp, sunk);

    gs::Bitmap round(5, 12);
    round.rect(1, 2, 3, 8, 1);
    round.rect(1, 1, 3, 2, 2);
    round.rect(2, 9, 1, 2, 3);
    art.brass = gs::uploadMipped(vdp, round);

    gs::Bitmap notch(12, 8);
    notch.line(1, 1, 6, 6, 1, 1);
    notch.line(11, 1, 6, 6, 1, 1);
    art.notch = gs::uploadMipped(vdp, notch);

    gs::Bitmap splash(14, 10);
    splash.ellipse(7, 6, 6, 3, 3);
    splash.ellipse(7, 5, 3, 1.6f, 1);
    splash.line(7, 1, 7, 4, 1, 1);
    art.splash = gs::uploadMipped(vdp, splash);

    gs::Bitmap drip(8, 10);
    drip.ellipse(4, 6, 2.2f, 3.0f, 2);
    drip.ellipse(4, 5, 1.0f, 1.2f, 4);
    art.drip = gs::uploadMipped(vdp, drip);

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
