#include "art.h"

#include <cstring>

namespace subgrass {
namespace {

gs::Bitmap shear(const gs::Bitmap& src, float k) {
    gs::Bitmap o(src.w, src.h);
    float cx = src.w * 0.5f;
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            int c = src.get(x, y);
            if (!c) continue;
            int ny = y + int(k * (float(x) - cx));
            o.set(x, ny, c);
        }
    }
    return o;
}

void paintSub(gs::Bitmap& b) {
    b.ellipse(46, 20, 30, 10, 2);
    b.ellipse(46, 20, 24, 6, 1);
    b.ellipse(58, 12, 8, 7, 2);
    b.rect(54, 6, 3, 8, 3);
    b.ellipse(62, 18, 5, 3, 4);
    b.rect(20, 18, 10, 3, 5);
    b.rect(16, 16, 4, 7, 6);
    b.ellipse(18, 14, 2, 2, 6);
    b.ellipse(18, 24, 2, 2, 6);
    b.rect(70, 17, 6, 5, 3);
    b.line(8, 20, 16, 20, 6, 1.4f);
}

void paintTuft(gs::Bitmap& b) {
    b.line(8, 16, 4, 2, 1, 1.6f);
    b.line(8, 16, 8, 1, 2, 1.6f);
    b.line(8, 16, 13, 3, 1, 1.6f);
    b.ellipse(8, 16, 4, 2, 3);
}

void paintGround(gs::Bitmap& b) {
    b.rect(0, 0, 20, 6, 1);
    b.rect(0, 6, 20, 26, 2);
    b.set(3, 2, 3);
    b.set(11, 3, 3);
    b.set(16, 1, 3);
    b.rect(2, 12, 4, 3, 4);
    b.rect(12, 18, 5, 3, 4);
}

void paintBubble(gs::Bitmap& b) {
    b.ellipse(4, 4, 3, 3, 1);
    b.set(3, 3, 2);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto pal = [&](int p, int i, int r, int g, int b) { vdp.setColor(p * 16 + i, gs::rgb4(r, g, b)); };
    for (int p = 0; p < 16; p++) pal(p, 0, 0, 0, 0);

    pal(PAL_HUD, 1, 14, 15, 13);
    pal(PAL_TITLE, 1, 8, 15, 12);
    pal(PAL_WIN, 1, 6, 15, 7);
    pal(PAL_FAIL, 1, 15, 6, 4);
    pal(PAL_SUB, 1, 3, 6, 5);
    pal(PAL_SUB, 2, 6, 10, 8);
    pal(PAL_SUB, 3, 12, 14, 11);
    pal(PAL_SUB, 4, 6, 14, 15);
    pal(PAL_SUB, 5, 13, 4, 3);
    pal(PAL_SUB, 6, 2, 3, 3);
    pal(PAL_BUB, 1, 8, 13, 15);
    pal(PAL_BUB, 2, 14, 15, 15);
    pal(PAL_GRASS, 1, 4, 12, 3);
    pal(PAL_GRASS, 2, 2, 7, 2);
    pal(PAL_GRASS, 3, 8, 15, 5);
    pal(PAL_GRASS, 4, 3, 8, 3);
    pal(PAL_MUD, 1, 10, 8, 3);
    pal(PAL_MUD, 2, 6, 5, 2);
    pal(PAL_MUD, 3, 13, 11, 5);
    pal(PAL_MUD, 4, 5, 4, 2);
    pal(PAL_ROCK, 1, 5, 6, 7);
    pal(PAL_ROCK, 2, 3, 4, 5);
    pal(PAL_ROCK, 3, 8, 9, 10);
    pal(PAL_ROCK, 4, 2, 3, 4);
    pal(PAL_KELP, 1, 2, 9, 3);
    pal(PAL_KELP, 2, 5, 12, 4);

    vdp.setFogColor(gs::rgb4(1, 3, 6));

    gs::Bitmap hull(88, 40);
    paintSub(hull);
    art.sub[0] = gs::uploadMipped(vdp, hull);
    art.sub[1] = gs::uploadMipped(vdp, shear(hull, -0.18f));
    art.sub[2] = gs::uploadMipped(vdp, shear(hull, 0.18f));

    gs::Bitmap tuft(16, 18);
    paintTuft(tuft);
    art.tuft = gs::uploadMipped(vdp, tuft);

    gs::Bitmap ground(20, 32);
    paintGround(ground);
    art.ground = gs::uploadMipped(vdp, ground);

    gs::Bitmap bub(8, 8);
    paintBubble(bub);
    art.bubble = gs::uploadMipped(vdp, bub);

    gs::Bitmap flag(10, 8);
    flag.rect(0, 0, 2, 8, 1);
    flag.poly({{2, 1}, {9, 3}, {2, 5}}, 2);
    art.sailflag = gs::uploadMipped(vdp, flag);

    for (int i = 0; i < 96; i++) {
        gs::Bitmap g(5, 7);
        const uint8_t* src = gs::glyph(char(32 + i));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (src[y * 5 + x]) g.set(x, y, 1);
        art.glyph[i] = gs::uploadMipped(vdp, g);
    }

    gs::TextStyle st;
    st.scale = 3;
    st.color = 1;
    st.spacing = 1;
    art.title = gs::uploadMipped(vdp, gs::textBitmap("SUB GRASS", st));
    st.scale = 2;
    art.wordStop = gs::uploadMipped(vdp, gs::textBitmap("FULL STOP", st));
    art.wordGrass = gs::uploadMipped(vdp, gs::textBitmap("ON THE GRASS", st));
}

}  // namespace subgrass
