#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace railbox {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void wheel(gs::Bitmap& b, float cx, float cy, float phase) {
    b.ellipse(cx, cy, 7, 7, 8);
    b.ellipse(cx, cy, 2, 2, 1);
    for (int k = 0; k < 3; k++) {
        float a = phase + k * 1.0472f;
        b.line(cx, cy, cx + std::cos(a) * 6.f, cy + std::sin(a) * 6.f, 9, 1.f);
    }
}

gs::Bitmap cabArt(int phase) {
    gs::Bitmap b(132, 62);
    b.rect(6, 46, 118, 4, 2);
    b.rect(10, 24, 64, 24, 3);
    b.rect(10, 24, 64, 3, 4);
    b.rect(74, 12, 32, 36, 3);
    b.rect(74, 12, 32, 3, 4);
    b.rect(78, 18, 22, 12, 5);
    b.rect(80, 20, 8, 6, 10);
    b.rect(106, 28, 18, 20, 3);
    b.poly({{124, 30}, {130, 38}, {124, 46}}, 6);
    b.rect(108, 32, 6, 4, 11);
    b.rect(82, 34, 14, 6, 7);
    b.blit(gs::textBitmap("S3", {1, 1, 0, 0, 1}), 83, 35);
    b.ellipse(92, 10, 3, 3, 12);
    b.rect(14, 48, 36, 6, 8);
    b.rect(88, 48, 28, 6, 8);
    float p = phase * 1.2f;
    wheel(b, 24, 54, p);
    wheel(b, 42, 54, p);
    wheel(b, 96, 54, p);
    wheel(b, 110, 54, p);
    float rod = 54.f + std::sin(p) * 3.f;
    b.line(24, rod, 42, rod, 9, 2);
    b.line(96, rod, 110, rod, 9, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap boxArt() {
    gs::Bitmap b(BOX_W, 64);
    b.rect(0, 46, BOX_W, 16, 4);
    b.rect(0, 46, BOX_W, 2, 5);
    b.rect(0, 60, BOX_W, 3, 5);
    b.rect(0, 46, 3, 16, 5);
    b.rect(BOX_W - 3, 46, 3, 16, 5);
    for (int x = 12; x < BOX_W - 6; x += 18) b.rect(x, 52, 2, 6, 5);
    b.rect(8, 36, BOX_W - 16, 10, 3);
    b.rect(8, 36, BOX_W - 16, 2, 2);
    b.rect(28, 16, 4, 22, 6);
    b.rect(BOX_W - 32, 16, 4, 22, 6);
    b.poly({{20, 18}, {BOX_W / 2.f, 4}, {float(BOX_W - 20), 18}}, 7);
    b.rect(36, 22, BOX_W - 72, 12, 8);
    gs::Bitmap label = gs::textBitmap("BOX", {1, 1, 0, 0, 1});
    b.blit(label, (BOX_W - label.w) / 2, 24);
    b.rect(14, 24, 2, 12, 1);
    b.ellipse(15, 22, 3, 3, 9);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(32, 22);
    b.rect(0, 2, 32, 18, 4);
    b.rect(0, 5, 32, 12, 3);
    for (int x = 1; x < 32; x += 8) b.rect(x, 7, 5, 8, 2);
    b.rect(0, 6, 32, 2, 5);
    b.rect(0, 14, 32, 2, 5);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(32, 52);
    b.rect(14, 28, 4, 22, 6);
    b.ellipse(16, 18, 13, 14, 2);
    b.ellipse(11, 16, 7, 7, 3);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(110, 40);
    b.poly({{0, 39}, {22, 22}, {46, 30}, {72, 8}, {96, 24}, {109, 39}}, 7);
    b.poly({{40, 39}, {72, 16}, {100, 39}}, 8);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(52, 22);
    b.ellipse(14, 13, 12, 7, 2);
    b.ellipse(30, 11, 14, 8, 1);
    b.ellipse(44, 13, 8, 6, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(28, 48);
    b.rect(12, 22, 4, 24, 1);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 14, 9, 9, 3);
    b.line(14, 14, 14, 7, 4, 1);
    b.line(14, 14, 20, 16, 5, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 3);
    b.ellipse(10, 10, 4, 4, 4);
    return b;
}

gs::Image words(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 0, 15, 1}));
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_DIM, gs::rgb4(9, 11, 13));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(3, 15, 7));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4), gs::rgb4(2, 5, 10), gs::rgb4(4, 8, 13), gs::rgb4(11, 14, 15),
            gs::rgb4(1, 2, 4), gs::rgb4(12, 11, 8), gs::rgb4(2, 2, 3), gs::rgb4(10, 10, 11), gs::rgb4(15, 15, 15),
            gs::rgb4(15, 13, 4), gs::rgb4(14, 10, 2), 0, 0, 0});
    setPal(vdp, PAL_BOX,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 7), gs::rgb4(11, 9, 4), gs::rgb4(14, 12, 3),
            gs::rgb4(4, 3, 2), gs::rgb4(10, 3, 2), gs::rgb4(14, 13, 10), gs::rgb4(15, 12, 2), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LAND,
           {0, gs::rgb4(14, 14, 15), gs::rgb4(2, 6, 3), gs::rgb4(4, 9, 4), gs::rgb4(5, 4, 3), gs::rgb4(8, 8, 9),
            gs::rgb4(6, 4, 2), gs::rgb4(3, 6, 4), gs::rgb4(5, 8, 5), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 10), gs::rgb4(14, 3, 3), gs::rgb4(2, 2, 4), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 14, 15), gs::rgb4(9, 9, 11), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 11), 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, art);
    art.cab[0] = gs::uploadImage(vdp, cabArt(0));
    art.cab[1] = gs::uploadImage(vdp, cabArt(1));
    art.box = gs::uploadImage(vdp, boxArt());
    art.rail = gs::uploadImage(vdp, railArt());
    art.tree = gs::uploadImage(vdp, treeArt());
    art.hill = gs::uploadImage(vdp, hillArt());
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    art.puff = gs::uploadImage(vdp, puffArt());
    art.sun = gs::uploadImage(vdp, sunArt());
    art.logo = words(vdp, "S3 RAILBOX", 3);
    art.tag = words(vdp, "STOP INSIDE THE BOX", 2);
    art.banIn = words(vdp, "IN THE BOX", 3);
    art.banCrew = words(vdp, "OTHER CREW", 2);
    art.banShort = words(vdp, "STOPPED SHORT", 2);
    art.banRan = words(vdp, "RAN THE BOX", 2);
}

}  // namespace railbox
