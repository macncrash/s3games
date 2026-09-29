#include "game/art.h"

#include <algorithm>
#include <string>

namespace rmaga {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : 0);
}

void coat(gs::Bitmap& b, int frame, int body, int cap, int trim) {
    b.ellipse(11, 6, 4.2f, 4.f, 1);
    b.rect(8, 2, 7, 4, cap);
    b.rect(7, 10, 9, 12, body);
    b.rect(7, 10, 3, 12, trim);
    b.rect(4, 12, 3, 8, body);
    b.rect(16, 12, 3, 8, trim);
    int step = frame & 1;
    b.rect(7, 22, 3, 8 - step, 5);
    b.rect(12, 22, 3, 7 + step, 5);
    b.rect(6, 29, 5, 2, 6);
    b.rect(11, 29, 5, 2, 6);
    b.set(10, 6, 3);
    b.set(13, 6, 3);
    b.rect(15, 16, 8, 2, 7);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 9), gs::rgb4(5, 4, 3), gs::rgb4(15, 15, 12)};
    const uint16_t earth[] = {0, gs::rgb4(5, 4, 2), gs::rgb4(7, 6, 3), gs::rgb4(3, 5, 2), gs::rgb4(9, 7, 4),
                              gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 5)};
    const uint16_t gab[] = {0, gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(11, 9, 5), gs::rgb4(3, 3, 2),
                            gs::rgb4(6, 7, 4)};
    const uint16_t flag[] = {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), gs::rgb4(9, 2, 2)};
    const uint16_t storm[] = {0, gs::rgb4(12, 9, 7), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), gs::rgb4(6, 5, 6),
                              gs::rgb4(2, 2, 3), gs::rgb4(4, 3, 2), gs::rgb4(10, 8, 3)};
    const uint16_t scout[] = {0, gs::rgb4(13, 11, 8), gs::rgb4(6, 7, 5), gs::rgb4(3, 4, 3), gs::rgb4(9, 10, 7),
                              gs::rgb4(4, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(2, 3, 2)};
    const uint16_t you[] = {0, gs::rgb4(13, 10, 7), gs::rgb4(4, 5, 7), gs::rgb4(2, 2, 4), gs::rgb4(7, 8, 11),
                            gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(12, 10, 4)};
    const uint16_t brass[] = {0, gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7), gs::rgb4(8, 6, 2), gs::rgb4(6, 2, 2)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 3), gs::rgb4(9, 9, 8)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {0, gs::rgb4(7, 14, 7), gs::rgb4(13, 15, 10)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_EARTH, earth, 7);
    pal(vdp, PAL_GAB, gab, 6);
    pal(vdp, PAL_FLAG, flag, 5);
    pal(vdp, PAL_STORM, storm, 8);
    pal(vdp, PAL_SCOUT, scout, 8);
    pal(vdp, PAL_YOU, you, 8);
    pal(vdp, PAL_BRASS, brass, 5);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    vdp.setFogColor(gs::rgb4(4, 3, 5));

    gs::Bitmap gabion(28, 36);
    for (int y = 4; y < 34; y++) {
        for (int x = 2; x < 26; x++) {
            int band = (y / 5) & 1;
            int c = band ? 1 : 2;
            if ((x + y) % 7 == 0) c = 5;
            if (x < 4 || x > 23) c = 4;
            gabion.set(x, y, c);
        }
    }
    gabion.rect(4, 2, 20, 3, 3);
    gabion.rect(6, 32, 16, 3, 4);
    art.gabion = gs::uploadMipped(vdp, gabion);

    gs::Bitmap flagB(22, 40);
    flagB.rect(3, 2, 2, 36, 3);
    flagB.rect(5, 4, 14, 10, 1);
    flagB.rect(5, 8, 14, 2, 2);
    flagB.rect(8, 4, 2, 10, 2);
    art.flag = gs::uploadMipped(vdp, flagB);

    gs::Bitmap chest(36, 22);
    chest.rect(2, 4, 32, 16, 1);
    chest.rect(2, 4, 32, 4, 3);
    chest.rect(15, 10, 6, 5, 4);
    chest.rect(4, 18, 28, 2, 2);
    art.chest = gs::uploadMipped(vdp, chest);

    gs::Bitmap brassB(6, 14);
    brassB.rect(1, 1, 4, 10, 1);
    brassB.rect(1, 1, 4, 3, 2);
    brassB.rect(1, 10, 4, 3, 4);
    art.brass = gs::uploadMipped(vdp, brassB);

    gs::Bitmap puff(16, 16);
    puff.ellipse(8, 8, 6, 5, 1);
    puff.ellipse(6, 7, 3, 2, 3);
    puff.ellipse(10, 9, 2, 2, 2);
    art.puff = gs::uploadMipped(vdp, puff);

    gs::Bitmap chev(12, 8);
    chev.line(1, 1, 6, 6, 1, 1.4f);
    chev.line(11, 1, 6, 6, 1, 1.4f);
    art.chev = gs::uploadMipped(vdp, chev);

    gs::Bitmap down(26, 12);
    down.ellipse(8, 5, 5, 4, 1);
    down.rect(12, 4, 12, 4, 2);
    down.rect(14, 7, 8, 2, 5);
    art.down = gs::uploadMipped(vdp, down);

    for (int f = 0; f < 2; f++) {
        gs::Bitmap yb(24, 34);
        coat(yb, f, 2, 4, 3);
        yb.rect(18, 14, 3, 8, 7);
        art.you[f] = gs::uploadMipped(vdp, yb);
        gs::Bitmap sb(24, 34);
        coat(sb, f, 2, 4, 3);
        art.storm[f] = gs::uploadMipped(vdp, sb);
        gs::Bitmap cb(24, 34);
        coat(cb, f, 2, 6, 4);
        art.scout[f] = gs::uploadMipped(vdp, cb);
    }

    gs::TextStyle big{1, 1, 0, 0, 1};
    for (int c = 32; c < 128; c++) {
        gs::Bitmap g = gs::textBitmap(std::string(1, char(c)), big);
        art.gw[c - 32] = std::max(1, g.w);
        art.gh = std::max(art.gh, g.h);
        art.glyph[c - 32] = gs::uploadImage(vdp, g);
    }
}

}  // namespace rmaga
