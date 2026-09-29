#include "game/art.h"

#include <cstring>

#include "console/gfx.h"

namespace striker {

static void pal(gs::VDP& vdp, int p, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, 0);
    for (int i = 0; i < n; i++) vdp.setColor(p * 16 + i, c[i]);
}

static gs::Image up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadImage(vdp, b); }

static gs::Image words(gs::VDP& vdp, const char* s, int scale, int color, int outline) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = outline;
    st.spacing = 1;
    return up(vdp, gs::textBitmap(s, st));
}

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t fair[] = {
        0,
        gs::rgb4(2, 1, 4),
        gs::rgb4(6, 2, 3),
        gs::rgb4(12, 8, 3),
        gs::rgb4(3, 6, 3),
        gs::rgb4(9, 4, 2),
        gs::rgb4(14, 12, 8),
        gs::rgb4(4, 3, 6),
    };
    const uint16_t tower[] = {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(4, 2, 1), gs::rgb4(14, 13, 10),
                              gs::rgb4(2, 2, 3)};
    const uint16_t bell[] = {0, gs::rgb4(14, 11, 3), gs::rgb4(10, 7, 1), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1)};
    const uint16_t puck[] = {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 8, 4), gs::rgb4(6, 1, 1)};
    const uint16_t mallet[] = {0, gs::rgb4(10, 6, 2), gs::rgb4(5, 3, 1), gs::rgb4(14, 4, 3), gs::rgb4(3, 2, 2),
                               gs::rgb4(12, 9, 6)};
    const uint16_t ink[] = {0, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 3)};
    const uint16_t gold[] = {0, gs::rgb4(15, 13, 3), gs::rgb4(6, 3, 1)};
    const uint16_t meter[] = {0, gs::rgb4(3, 10, 4), gs::rgb4(14, 12, 3), gs::rgb4(14, 4, 2), gs::rgb4(2, 2, 3)};
    const uint16_t win[] = {0, gs::rgb4(15, 14, 6), gs::rgb4(8, 2, 2)};

    pal(vdp, PAL_FAIR, fair, 8);
    pal(vdp, PAL_TOWER, tower, 6);
    pal(vdp, PAL_BELL, bell, 5);
    pal(vdp, PAL_PUCK, puck, 4);
    pal(vdp, PAL_MALLET, mallet, 6);
    pal(vdp, PAL_INK, ink, 3);
    pal(vdp, PAL_GOLD, gold, 3);
    pal(vdp, PAL_METER, meter, 5);
    pal(vdp, PAL_WIN, win, 3);
    vdp.setFogColor(gs::rgb4(2, 1, 4));

    gs::Bitmap booth(320, 80);
    for (int y = 0; y < 80; y++) {
        for (int x = 0; x < 320; x++) {
            int stripe = ((x + y) / 10) & 1;
            int c = y > 64 ? 5 : (stripe ? 2 : 3);
            if (y < 8 && (x / 16) % 2 == 0) c = 6;
            booth.set(x, y, c);
        }
    }
    gs::TileAlloc tiles(vdp, 1);
    vdp.B.resize(64, 32);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 18, booth, PAL_FAIR);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.HUD.clear();
    vdp.hudEnabled = false;

    gs::Bitmap tw(36, 168);
    for (int y = 0; y < 168; y++) {
        for (int x = 0; x < 36; x++) {
            int c = 0;
            if (x < 4 || x >= 32) c = 3;
            else if (x >= 14 && x < 22) c = 5;
            else c = ((y / 12) & 1) ? 1 : 2;
            if (y % 28 == 0 && x >= 4 && x < 32) c = 4;
            tw.set(x, y, c);
        }
    }
    art.tower = up(vdp, tw);

    gs::Bitmap be(44, 36);
    be.ellipse(22, 20, 16, 12, 1);
    be.ellipse(22, 16, 10, 7, 3);
    be.rect(20, 4, 4, 10, 2);
    be.ellipse(22, 4, 6, 3, 2);
    be.rect(10, 28, 24, 3, 4);
    art.bell = up(vdp, be);

    gs::Bitmap pk(18, 12);
    pk.ellipse(9, 6, 8, 5, 1);
    pk.ellipse(7, 4, 3, 2, 2);
    art.puck = up(vdp, pk);

    auto arm = [](gs::Bitmap& b, bool upSwing) {
        b.ellipse(18, 28, 7, 8, 3);
        b.rect(15, 18, 6, 12, 5);
        b.rect(8, 16, 8, 4, 4);
        if (!upSwing) {
            b.rect(4, 10, 22, 6, 1);
            b.rect(4, 8, 6, 10, 2);
        } else {
            b.rect(16, 2, 6, 20, 1);
            b.rect(14, 2, 10, 6, 2);
        }
    };
    gs::Bitmap m0(40, 40);
    arm(m0, false);
    art.mallet = up(vdp, m0);
    gs::Bitmap m1(40, 40);
    arm(m1, true);
    art.malletUp = up(vdp, m1);

    gs::Bitmap lamp(10, 10);
    lamp.ellipse(5, 5, 4, 4, 1);
    art.lamp = up(vdp, lamp);

    gs::Bitmap base(80, 16);
    base.rect(0, 4, 80, 12, 1);
    base.rect(8, 0, 64, 6, 2);
    art.base = up(vdp, base);

    art.title = words(vdp, "STRIKER", 3, 1, 2);
    art.sub = words(vdp, "RING THE BELL", 1, 1, 2);
    art.hint = words(vdp, "BEFORE TRY 3 DIES", 1, 1, 2);
    art.swing = words(vdp, "SWING", 2, 1, 2);
    art.miss = words(vdp, "MISS", 2, 1, 2);
    art.ding = words(vdp, "BELL", 3, 1, 2);
    art.dead = words(vdp, "TRY 3 DEAD", 2, 1, 2);
    for (int d = 0; d < 10; d++) {
        char s[2] = {char('0' + d), 0};
        art.digit[d] = words(vdp, s, 2, 1, 2);
    }
}

}  // namespace striker
