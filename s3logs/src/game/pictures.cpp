#include "pictures.h"

#include <cstring>

namespace logs {
namespace {

using gs::Bitmap;
using gs::Pt;
using gs::rgb4;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap label(const char* s, int scale, int color, int outline) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = color;
    st.outline = outline;
    st.spacing = 1;
    return gs::textBitmap(s, st);
}

// Chase view: bow up the screen, stern toward the camera. Pike on the right.
void bateau(Bitmap& b, int swing) {
    const float cx = 26.f;
    b.poly({{cx, 5}, {cx + 16, 28}, {cx + 13, 66}, {cx - 13, 66}, {cx - 16, 28}}, 2);
    b.poly({{cx, 9}, {cx + 12, 28}, {cx + 9, 62}, {cx - 9, 62}, {cx - 12, 28}}, 1);
    b.line(cx, 14, cx, 60, 2, 1.15f);
    b.line(cx - 10, 30, cx + 10, 30, 8, 1.7f);
    b.line(cx - 8, 44, cx + 8, 44, 8, 1.7f);
    b.line(cx - 8, 56, cx + 8, 56, 8, 1.7f);
    b.ellipse(cx - 6, 58, 2.2f, 1.4f, 8);  // rope coil
    b.rect(cx - 4.2f, 46, 8.4f, 10, 3);
    b.ellipse(cx, 44, 3.6f, 3.8f, 4);
    b.ellipse(cx, 42.4f, 3.8f, 2.3f, 5);
    b.rect(cx - 4.2f, 42.6f, 8.4f, 1.4f, 5);
    float px = swing ? 22.f : 13.f;
    float py = swing ? 14.f : 22.f;
    b.line(cx + 3.f, 50.f, cx + px, py, 6, 1.8f);
    b.line(cx + px, py, cx + px + 2.5f, py - 6.f, 9, 2.1f);
    b.ellipse(cx + 4.f, 50.f, 1.6f, 1.6f, 4);
}

void stickBmp(Bitmap& b, int kind) {
    b.ellipse(12, 8, 8.2f, 5.2f, 3);
    b.ellipse(12, 8, 4.6f, 2.7f, 4);
    b.ellipse(12, 8, 1.5f, 1.0f, 2);
    b.poly({{5, 11}, {19, 11}, {17, 38}, {7, 38}}, 1);
    b.poly({{5, 11}, {9, 13}, {8, 38}, {7, 38}}, 2);
    b.line(9, 14, 8, 36, 5, 1.05f);
    b.line(15, 16, 14, 34, 2, 1.05f);
    if (kind == 1) b.ellipse(15, 24, 2.3f, 1.7f, 2);
    if (kind == 2) {
        b.line(17, 20, 23, 13, 2, 2.3f);
        b.ellipse(14, 28, 1.8f, 1.3f, 6);
    }
    b.ellipse(12, 39, 7, 3.2f, 5);
}

void rockBmp(Bitmap& b) {
    b.ellipse(15, 14, 13, 8.5f, 2);
    b.ellipse(14, 12, 9.5f, 6.2f, 1);
    b.ellipse(10, 10, 3.2f, 2.1f, 4);
    b.ellipse(18, 14, 3.4f, 2.2f, 3);
    b.ellipse(15, 17, 10, 2.4f, 5);
}

void sweeperBmp(Bitmap& b) {
    b.line(3, 18, 46, 7, 2, 5.2f);
    b.line(6, 17, 44, 8, 1, 2.4f);
    b.line(18, 13, 12, 4, 2, 2.2f);
    b.line(28, 11, 33, 3, 2, 2.1f);
    b.line(38, 9, 44, 3, 6, 1.8f);
    b.ellipse(5, 18, 4.2f, 3.1f, 3);
    b.ellipse(5, 18, 2.1f, 1.4f, 4);
}

void pine(Bitmap& b, int kind) {
    b.rect(12, 34, 5, 13, 3);
    b.rect(13, 36, 2, 11, 4);
    b.poly({{14, 2}, {25, 16}, {3, 16}}, kind ? 2 : 1);
    b.poly({{14, 11}, {28, 26}, {0, 26}}, 1);
    b.poly({{14, 20}, {30, 38}, {-1, 38}}, kind ? 1 : 2);
    b.poly({{14, 15}, {22, 28}, {6, 28}}, 2);
}

void pilingBmp(Bitmap& b) {
    b.rect(4, 6, 8, 34, 1);
    b.rect(4, 6, 2, 34, 2);
    b.rect(2, 3, 12, 5, 2);
    b.rect(3, 4, 10, 2, 1);
    b.line(3, 16, 13, 16, 3, 1.5f);
    b.line(3, 24, 13, 24, 3, 1.5f);
    b.line(3, 32, 13, 32, 4, 1.4f);
    b.ellipse(8, 40, 6, 2.3f, 4);
}

void wingBmp(Bitmap& b) {
    b.ellipse(7, 8, 6, 6, 3);
    b.ellipse(37, 8, 6, 6, 3);
    b.ellipse(7, 8, 3.2f, 3, 4);
    b.ellipse(37, 8, 3.2f, 3, 4);
    b.rect(7, 3, 30, 10, 1);
    b.rect(7, 3, 30, 3, 2);
    b.ellipse(16, 8, 1.7f, 1.7f, 4);
    b.ellipse(22, 8, 2.1f, 2.1f, 3);
    b.ellipse(28, 8, 1.7f, 1.7f, 4);
}

void millBmp(Bitmap& b, int frame) {
    b.poly({{6, 16}, {28, 3}, {50, 16}}, 6);
    b.poly({{10, 16}, {28, 7}, {46, 16}}, 2);
    b.rect(10, 16, 36, 22, 5);
    b.rect(10, 16, 36, 3, 1);
    b.rect(10, 16, 3, 22, 1);
    b.rect(43, 16, 3, 22, 1);
    b.rect(24, 16, 3, 22, 1);
    b.rect(15, 26, 7, 12, 2);
    b.rect(33, 22, 7, 6, 3);
    b.rect(34, 23, 5, 4, 8);
    b.rect(40, 6, 5, 10, 4);
    b.ellipse(6, 32, 8, 8, 2);
    b.ellipse(6, 32, 3.2f, 3.2f, 3);
    if (frame == 0) {
        b.line(6, 24, 6, 40, 3, 1.3f);
        b.line(-1, 32, 13, 32, 3, 1.3f);
    } else {
        b.line(0, 26, 12, 38, 3, 1.3f);
        b.line(0, 38, 12, 26, 3, 1.3f);
    }
}

void cabinBmp(Bitmap& b) {
    b.poly({{2, 14}, {20, 2}, {38, 14}}, 6);
    b.poly({{6, 14}, {20, 6}, {34, 14}}, 2);
    b.rect(6, 14, 28, 16, 5);
    b.rect(6, 14, 28, 3, 1);
    b.rect(6, 14, 3, 16, 1);
    b.rect(31, 14, 3, 16, 1);
    b.rect(15, 20, 7, 10, 2);
    b.rect(9, 18, 5, 4, 3);
    b.rect(26, 18, 5, 4, 8);
    b.rect(30, 4, 4, 10, 4);
}

void splashBmp(Bitmap& b) {
    b.ellipse(10, 9, 8, 3.6f, 1);
    b.ellipse(5, 7, 3, 2.1f, 2);
    b.ellipse(15, 6, 2.6f, 2.2f, 2);
    b.line(3, 8, 1, 2, 1, 1.2f);
    b.line(17, 8, 19, 2, 1, 1.2f);
    b.line(10, 7, 10, 1, 2, 1.1f);
}

void shadowBmp(Bitmap& b) { b.ellipse(18, 6, 15, 3.6f, 1); }

void sunBmp(Bitmap& b) {
    b.line(9, 1, 9, 4, 4, 1.3f);
    b.line(9, 14, 9, 17, 4, 1.3f);
    b.line(1, 9, 4, 9, 4, 1.3f);
    b.line(14, 9, 17, 9, 4, 1.3f);
    b.line(3, 3, 5, 5, 4, 1.2f);
    b.line(13, 13, 15, 15, 4, 1.2f);
    b.line(15, 3, 13, 5, 4, 1.2f);
    b.line(5, 13, 3, 15, 4, 1.2f);
    b.ellipse(9, 9, 5.5f, 5.5f, 4);
    b.ellipse(9, 9, 3.4f, 3.4f, 3);
}

void cloudBmp(Bitmap& b) {
    b.ellipse(16, 10, 11, 5.5f, 2);
    b.ellipse(24, 9, 8, 4.8f, 1);
    b.ellipse(10, 9, 7, 4.6f, 1);
    b.ellipse(18, 7, 6, 3.5f, 1);
}

void birdBmp(Bitmap& b) {
    b.line(0, 5, 5, 1, 5, 1.35f);
    b.line(5, 1, 11, 5, 5, 1.35f);
}

void font(gs::VDP& vdp, int* out) {
    uint8_t px[64];
    int tile = 1;
    for (int c = 32; c < 128; c++) {
        std::memset(px, 0, sizeof px);
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        vdp.loadTile(tile, px);
        out[c - 32] = tile++;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, rgb4(15, 15, 15), rgb4(2, 2, 4)});
    setPal(vdp, PAL_BOAT,
           {0, rgb4(12, 8, 4), rgb4(7, 4, 2), rgb4(13, 2, 2), rgb4(14, 10, 7), rgb4(2, 2, 4), rgb4(9, 10, 11),
            rgb4(14, 15, 15), rgb4(8, 6, 3), rgb4(13, 13, 14)});
    setPal(vdp, PAL_LOG,
           {0, rgb4(11, 7, 3), rgb4(6, 4, 2), rgb4(14, 12, 8), rgb4(9, 7, 4), rgb4(4, 3, 2), rgb4(4, 7, 3)});
    setPal(vdp, PAL_ROCK, {0, rgb4(9, 9, 10), rgb4(5, 5, 6), rgb4(3, 5, 3), rgb4(13, 13, 14), rgb4(4, 4, 5)});
    setPal(vdp, PAL_TREE, {0, rgb4(1, 5, 2), rgb4(3, 8, 3), rgb4(6, 4, 2), rgb4(3, 2, 1)});
    setPal(vdp, PAL_BOOM,
           {0, rgb4(11, 8, 4), rgb4(6, 4, 2), rgb4(8, 9, 10), rgb4(3, 4, 5), rgb4(14, 13, 9), rgb4(12, 2, 2),
            rgb4(3, 5, 2), rgb4(15, 15, 14)});
    setPal(vdp, PAL_FX, {0, rgb4(15, 15, 15), rgb4(8, 13, 15), rgb4(4, 8, 12)});
    setPal(vdp, PAL_SKY, {0, rgb4(15, 15, 15), rgb4(11, 12, 14), rgb4(15, 15, 12), rgb4(15, 13, 5), rgb4(2, 2, 4)});
    setPal(vdp, PAL_TITLE, {0, rgb4(15, 14, 10), rgb4(2, 3, 6)});
    setPal(vdp, PAL_AMBER, {0, rgb4(15, 12, 3), rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 4, 3), rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, rgb4(8, 15, 8), rgb4(1, 3, 1)});
    setPal(vdp, PAL_ROAD,
           {0, rgb4(2, 6, 2), rgb4(1, 4, 1), rgb4(4, 7, 3), rgb4(7, 6, 3), rgb4(4, 3, 1), rgb4(2, 5, 8),
            rgb4(1, 4, 7), rgb4(5, 5, 4), rgb4(3, 6, 8), rgb4(6, 5, 3), rgb4(1, 4, 8), rgb4(2, 7, 11),
            rgb4(7, 12, 15), rgb4(12, 13, 14), rgb4(5, 9, 12)});
    setPal(vdp, PAL_DIM, {0, rgb4(8, 10, 12), rgb4(2, 2, 3)});
    vdp.setFogColor(rgb4(9, 12, 14));

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    font(vdp, art.font);

    for (int i = 0; i < 2; i++) {
        Bitmap b(52, 72);
        bateau(b, i);
        art.boat[i] = gs::uploadMipped(vdp, b);
    }
    for (int i = 0; i < 3; i++) {
        Bitmap b(26, 46);
        stickBmp(b, i);
        art.stick[i] = gs::uploadMipped(vdp, b);
    }
    Bitmap rk(30, 24);
    rockBmp(rk);
    art.rock = gs::uploadMipped(vdp, rk);
    Bitmap sw(50, 24);
    sweeperBmp(sw);
    art.sweeper = gs::uploadMipped(vdp, sw);
    for (int i = 0; i < 2; i++) {
        Bitmap tr(32, 48);
        pine(tr, i);
        art.tree[i] = gs::uploadMipped(vdp, tr);
    }
    Bitmap pil(16, 44);
    pilingBmp(pil);
    art.piling = gs::uploadMipped(vdp, pil);
    Bitmap wn(44, 16);
    wingBmp(wn);
    art.wing = gs::uploadMipped(vdp, wn);
    for (int i = 0; i < 2; i++) {
        Bitmap m(56, 44);
        millBmp(m, i);
        art.mill[i] = gs::uploadMipped(vdp, m);
    }
    Bitmap cb(40, 32);
    cabinBmp(cb);
    art.cabin = gs::uploadMipped(vdp, cb);
    Bitmap sp(20, 14);
    splashBmp(sp);
    art.splash = gs::uploadMipped(vdp, sp);
    Bitmap sh(36, 12);
    shadowBmp(sh);
    art.shadow = gs::uploadMipped(vdp, sh);
    Bitmap su(18, 18);
    sunBmp(su);
    art.sun = gs::uploadMipped(vdp, su);
    Bitmap cl(40, 16);
    cloudBmp(cl);
    art.cloud = gs::uploadMipped(vdp, cl);
    Bitmap bi(12, 8);
    birdBmp(bi);
    art.bird = gs::uploadMipped(vdp, bi);

    art.title = gs::uploadMipped(vdp, label("S3 LOGS", 4, 1, 2));
    art.line1 = gs::uploadMipped(vdp, label("THE DRIVE HAS TO", 2, 1, 2));
    art.line2 = gs::uploadMipped(vdp, label("REACH THE BOOM", 2, 1, 2));
    art.win = gs::uploadMipped(vdp, label("REACHED THE BOOM", 2, 1, 2));
    art.fail = gs::uploadMipped(vdp, label("THE DRIVE BROKE", 2, 1, 2));
    art.sign = gs::uploadMipped(vdp, label("BOOM", 2, 1, 2));
}

}  // namespace logs
