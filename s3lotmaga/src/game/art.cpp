#include "game/art.h"

#include <string>

namespace lot {
namespace {

void pal(gs::VDP& vdp, int bank, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(bank * 16 + i, i < n ? c[i] : c[0]);
}

void runner(gs::Bitmap& b, int frame, int coat, int tool) {
    b.ellipse(10, 4, 3.0f, 2.8f, 1);
    b.rect(7, 6, 6, 3, 4);
    b.rect(6, 9, 8, 9, coat);
    b.rect(3, 10, 4, 6, coat);
    b.rect(13, 10, 4, 3, tool);
    int step = frame & 1;
    b.rect(7, 18, 3, 8 - step, 5);
    b.rect(11, 18, 3, 7 + step, 5);
    b.set(9, 4, 3);
    b.set(11, 4, 3);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 9), gs::rgb4(5, 5, 6), gs::rgb4(15, 15, 12)};
    const uint16_t lotc[] = {gs::rgb4(0, 0, 0), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 4), gs::rgb4(2, 2, 3),
                             gs::rgb4(6, 6, 7)};
    const uint16_t paint[] = {gs::rgb4(0, 0, 0), gs::rgb4(2, 4, 7), gs::rgb4(8, 10, 12), gs::rgb4(1, 2, 3),
                              gs::rgb4(12, 8, 3), gs::rgb4(4, 4, 5)};
    const uint16_t store[] = {gs::rgb4(0, 0, 0), gs::rgb4(7, 5, 4), gs::rgb4(12, 10, 7), gs::rgb4(3, 6, 8),
                              gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 2)};
    const uint16_t brass[] = {gs::rgb4(0, 0, 0), gs::rgb4(12, 9, 3), gs::rgb4(15, 14, 7), gs::rgb4(6, 4, 2)};
    const uint16_t commit[] = {gs::rgb4(0, 0, 0), gs::rgb4(11, 8, 6), gs::rgb4(4, 2, 2), gs::rgb4(1, 1, 1),
                               gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(13, 11, 5)};
    const uint16_t peel[] = {gs::rgb4(0, 0, 0), gs::rgb4(9, 9, 8), gs::rgb4(4, 5, 5), gs::rgb4(2, 2, 2),
                             gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 4), gs::rgb4(8, 7, 3)};
    const uint16_t fx[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(7, 7, 6)};
    const uint16_t alert[] = {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2)};
    const uint16_t ok[] = {gs::rgb4(0, 0, 0), gs::rgb4(6, 13, 6), gs::rgb4(12, 15, 10)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_LOT, lotc, 5);
    pal(vdp, PAL_CAR, paint, 6);
    pal(vdp, PAL_STORE, store, 6);
    pal(vdp, PAL_BRASS, brass, 4);
    pal(vdp, PAL_COMMIT, commit, 7);
    pal(vdp, PAL_PEEL, peel, 7);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_ALERT, alert, 3);
    pal(vdp, PAL_OK, ok, 3);
    vdp.setFogColor(gs::rgb4(2, 2, 4));

    gs::Bitmap stall(8, 48);
    stall.rect(3, 0, 2, 48, 2);
    art.stall = gs::uploadMipped(vdp, stall);

    gs::Bitmap car(36, 18);
    car.rect(2, 4, 32, 12, 1);
    car.rect(8, 1, 16, 6, 2);
    car.rect(10, 2, 5, 3, 3);
    car.rect(18, 2, 5, 3, 3);
    car.rect(1, 8, 4, 4, 5);
    car.rect(31, 8, 4, 4, 5);
    car.rect(4, 14, 6, 3, 4);
    car.rect(26, 14, 6, 3, 4);
    art.car = gs::uploadMipped(vdp, car);

    gs::Bitmap lamp(8, 36);
    lamp.rect(3, 8, 2, 28, 4);
    lamp.ellipse(4, 5, 3.2f, 3.2f, 2);
    lamp.rect(2, 4, 4, 2, 1);
    art.lamp = gs::uploadMipped(vdp, lamp);

    gs::Bitmap door(48, 28);
    door.rect(0, 0, 48, 28, 1);
    door.rect(4, 4, 16, 16, 3);
    door.rect(28, 4, 16, 16, 3);
    door.rect(22, 10, 4, 14, 5);
    door.rect(0, 24, 48, 4, 2);
    door.set(24, 16, 4);
    art.door = gs::uploadMipped(vdp, door);

    gs::Bitmap clerk(16, 22);
    clerk.ellipse(8, 3, 3, 3, 2);
    clerk.rect(5, 6, 6, 9, 1);
    clerk.rect(2, 8, 3, 6, 5);
    clerk.rect(11, 8, 4, 3, 4);
    clerk.rect(5, 15, 3, 6, 5);
    clerk.rect(9, 15, 3, 6, 5);
    art.clerk = gs::uploadMipped(vdp, clerk);

    for (int f = 0; f < 2; f++) {
        gs::Bitmap c(20, 28);
        runner(c, f, 2, 6);
        art.commit[f] = gs::uploadMipped(vdp, c);
        gs::Bitmap p(20, 28);
        runner(p, f, 2, 6);
        p.rect(14, 14, 5, 5, 6);
        art.peel[f] = gs::uploadMipped(vdp, p);
    }

    gs::Bitmap down(22, 10);
    down.ellipse(7, 5, 4, 3, 2);
    down.rect(10, 3, 10, 4, 4);
    down.rect(2, 6, 6, 2, 5);
    art.down = gs::uploadMipped(vdp, down);

    gs::Bitmap round(6, 14);
    round.rect(1, 2, 4, 10, 1);
    round.rect(1, 1, 4, 2, 2);
    round.rect(2, 11, 2, 2, 3);
    art.brass = gs::uploadMipped(vdp, round);

    gs::Bitmap chev(12, 8);
    chev.line(1, 1, 6, 6, 1, 1);
    chev.line(11, 1, 6, 6, 1, 1);
    art.chev = gs::uploadMipped(vdp, chev);

    gs::Bitmap flash(12, 8);
    flash.ellipse(6, 4, 5, 3, 1);
    flash.ellipse(6, 4, 2, 1.3f, 2);
    art.flash = gs::uploadMipped(vdp, flash);

    gs::Bitmap bag(10, 8);
    bag.rect(1, 2, 8, 5, 1);
    bag.rect(3, 1, 4, 2, 3);
    art.bag = gs::uploadMipped(vdp, bag);

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

}  // namespace lot
