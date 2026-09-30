#include "game/art.h"

#include <string>

namespace foundrymaga {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : 0);
}

void pourer(gs::Bitmap& b, int frame, int apron, int helm) {
    b.ellipse(12, 5, 3.4f, 3.2f, 1);
    b.rect(9, 7, 6, 3, helm);
    b.poly({{6, 11}, {18, 11}, {17, 22}, {7, 22}}, apron);
    b.rect(8, 14, 8, 6, 4);
    b.rect(4, 12, 3, 8, apron);
    b.rect(17, 12, 3, 7, 3);
    int step = frame & 1;
    b.rect(8, 22, 3, 8 - step, 5);
    b.rect(13, 22, 3, 7 + step, 5);
    b.rect(7, 29, 4, 2, 6);
    b.rect(12, 28 + step, 4, 2, 6);
    b.set(11, 5, 3);
    b.set(13, 5, 3);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 11, 6), gs::rgb4(5, 4, 3), gs::rgb4(15, 14, 10)};
    const uint16_t brick[] = {0, gs::rgb4(10, 4, 2), gs::rgb4(6, 2, 1), gs::rgb4(13, 7, 3), gs::rgb4(3, 1, 1),
                              gs::rgb4(15, 9, 2), gs::rgb4(2, 1, 1)};
    const uint16_t iron[] = {0, gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(10, 9, 8), gs::rgb4(2, 2, 2),
                             gs::rgb4(13, 8, 3)};
    const uint16_t ember[] = {0, gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 3), gs::rgb4(12, 4, 1), gs::rgb4(8, 2, 0),
                              gs::rgb4(15, 15, 8)};
    const uint16_t brass[] = {0, gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 5), gs::rgb4(7, 5, 2), gs::rgb4(14, 5, 2)};
    const uint16_t foe[] = {0, gs::rgb4(13, 8, 5), gs::rgb4(5, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(9, 4, 2),
                            gs::rgb4(4, 3, 3), gs::rgb4(7, 5, 3), gs::rgb4(15, 6, 1)};
    const uint16_t peel[] = {0, gs::rgb4(11, 10, 8), gs::rgb4(5, 6, 6), gs::rgb4(2, 3, 3), gs::rgb4(8, 8, 7),
                             gs::rgb4(3, 3, 3), gs::rgb4(6, 5, 4)};
    const uint16_t fx[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 7, 1), gs::rgb4(7, 6, 5)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 2), gs::rgb4(8, 2, 1)};
    const uint16_t ok[] = {0, gs::rgb4(6, 13, 5), gs::rgb4(12, 15, 8)};
    const uint16_t soot[] = {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 4, 3), gs::rgb4(1, 1, 1)};
    const uint16_t slag[] = {0, gs::rgb4(5, 4, 3), gs::rgb4(9, 5, 2), gs::rgb4(3, 2, 2), gs::rgb4(13, 7, 2)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_BRICK, brick, 7);
    pal(vdp, PAL_IRON, iron, 6);
    pal(vdp, PAL_EMBER, ember, 6);
    pal(vdp, PAL_BRASS, brass, 5);
    pal(vdp, PAL_FOE, foe, 8);
    pal(vdp, PAL_PEEL, peel, 7);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    pal(vdp, PAL_SOOT, soot, 4);
    pal(vdp, PAL_SLAG, slag, 5);
    const uint16_t road[16] = {
        0,
        gs::rgb4(6, 3, 1),
        gs::rgb4(3, 2, 1),
        gs::rgb4(9, 4, 1),
        gs::rgb4(12, 6, 1),
        gs::rgb4(4, 3, 2),
        gs::rgb4(14, 8, 2),
        gs::rgb4(5, 4, 3),
        gs::rgb4(15, 11, 3),
        gs::rgb4(2, 2, 2),
        gs::rgb4(8, 5, 2),
        gs::rgb4(1, 1, 1),
        gs::rgb4(10, 4, 1),
        gs::rgb4(7, 3, 1),
        gs::rgb4(13, 9, 3),
        gs::rgb4(4, 2, 1),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);
    vdp.setFogColor(gs::rgb4(8, 3, 1));

    gs::Bitmap furnace(40, 56);
    furnace.rect(4, 16, 32, 36, 1);
    furnace.rect(8, 20, 24, 28, 2);
    furnace.poly({{2, 16}, {38, 16}, {34, 6}, {6, 6}}, 3);
    furnace.rect(15, 0, 10, 8, 4);
    furnace.ellipse(20, 34, 7, 9, 5);
    furnace.ellipse(20, 36, 3, 5, 6);
    furnace.rect(6, 50, 28, 6, 3);
    furnace.outline(4, false);
    art.furnace = gs::uploadMipped(vdp, furnace);

    gs::Bitmap chimney(14, 40);
    chimney.rect(3, 6, 8, 34, 1);
    chimney.rect(5, 8, 3, 30, 2);
    chimney.rect(1, 2, 12, 6, 3);
    chimney.outline(4, false);
    art.chimney = gs::uploadMipped(vdp, chimney);

    gs::Bitmap crucible(22, 26);
    crucible.poly({{3, 8}, {19, 8}, {17, 22}, {5, 22}}, 1);
    crucible.ellipse(11, 8, 8, 3, 2);
    crucible.ellipse(11, 8, 5, 2, 3);
    crucible.rect(9, 1, 4, 7, 4);
    crucible.outline(5, false);
    art.crucible = gs::uploadMipped(vdp, crucible);

    for (int fr = 0; fr < 2; fr++) {
        gs::Bitmap flame(16, 22);
        flame.poly({{8, 1}, {14, 10}, {10, 20}, {6, 20}, {2, 10}}, fr ? 2 : 1);
        flame.poly({{8, 7}, {11, 13}, {8, 18}, {5, 13}}, 5);
        art.flame[fr] = gs::uploadMipped(vdp, flame);
    }

    for (int f = 0; f < 2; f++) {
        gs::Bitmap b(24, 32);
        pourer(b, f, 2, 4);
        art.apron[f] = gs::uploadMipped(vdp, b);
        gs::Bitmap p(24, 32);
        pourer(p, f, 2, 4);
        p.rect(18, 13, 5, 2, 3);
        art.peel[f] = gs::uploadMipped(vdp, p);
    }

    gs::Bitmap down(24, 10);
    down.ellipse(7, 5, 4, 3, 2);
    down.rect(10, 3, 11, 4, 4);
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

    gs::Bitmap lip(48, 16);
    lip.poly({{2, 4}, {46, 4}, {40, 14}, {8, 14}}, 1);
    lip.rect(10, 6, 28, 4, 2);
    lip.ellipse(24, 8, 6, 2, 3);
    lip.outline(4, false);
    art.lip = gs::uploadMipped(vdp, lip);

    gs::Bitmap ingot(22, 10);
    ingot.poly({{2, 8}, {6, 2}, {20, 2}, {16, 8}}, 1);
    ingot.rect(4, 8, 12, 2, 2);
    art.ingot = gs::uploadMipped(vdp, ingot);

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

}  // namespace foundrymaga
