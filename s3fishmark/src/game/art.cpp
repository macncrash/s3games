#include "art.h"

#include <initializer_list>

namespace fishmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

gs::Bitmap fishArt() {
    gs::Bitmap b(56, 22);
    b.ellipse(28, 12, 16, 7, 1);
    b.ellipse(26, 13, 12, 4, 2);
    b.poly({{44.f, 12.f}, {54.f, 4.f}, {50.f, 12.f}, {54.f, 20.f}}, 3);
    b.poly({{22.f, 6.f}, {30.f, 6.f}, {26.f, 2.f}}, 3);
    b.ellipse(16, 10, 2.2f, 2.2f, 4);
    b.ellipse(16, 10, 1.f, 1.f, 5);
    b.line(20, 13, 36, 13, 6, 1.2f);
    b.outline(15, false);
    return b;
}

gs::Bitmap anglerArt() {
    gs::Bitmap b(28, 44);
    b.ellipse(14, 7, 5, 5, 1);
    b.rect(11, 4, 6, 3, 2);
    b.rect(12, 13, 8, 12, 3);
    b.rect(10, 14, 3, 10, 4);
    b.rect(8, 24, 12, 3, 4);
    b.line(13, 25, 10, 40, 5, 2.f);
    b.line(17, 25, 20, 40, 5, 2.f);
    b.ellipse(12, 7, 1.f, 1.f, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap rodArt() {
    gs::Bitmap b(6, 52);
    b.line(3, 50, 3, 2, 1, 2.f);
    b.rect(2, 46, 2, 5, 2);
    b.ellipse(3, 2, 1.4f, 1.4f, 3);
    return b;
}

gs::Bitmap bobberArt() {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 6, 4, 5, 1);
    b.ellipse(5, 9, 4, 3, 2);
    b.line(5, 1, 5, 15, 3, 1.f);
    b.outline(15, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(150, 28);
    b.rect(0, 6, 150, 16, 1);
    for (int x = 4; x < 148; x += 12) b.rect(x, 6, 2, 16, 2);
    b.rect(0, 20, 150, 6, 3);
    for (int x = 10; x < 140; x += 28) b.rect(x, 22, 6, 6, 4);
    b.rect(0, 4, 150, 3, 5);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(96, 18);
    b.rect(0, 2, 96, 14, 1);
    for (int x = 0; x < 96; x += 16) b.line(float(x), 2, float(x), 16, 2, 1.f);
    b.rect(68, 1, 3, 16, 3);
    b.rect(70, 1, 1, 16, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(14, 28);
    b.line(4, 26, 3, 4, 1, 1.5f);
    b.line(8, 26, 10, 6, 2, 1.5f);
    b.ellipse(3, 4, 2.f, 3.f, 1);
    b.ellipse(11, 6, 2.f, 3.f, 2);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(18, 8);
    b.line(1, 5, 9, 3, 1, 1.4f);
    b.line(9, 3, 17, 5, 1, 1.4f);
    b.ellipse(9, 4, 1.4f, 1.2f, 2);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(9, 6, 8, 3, 1);
    b.ellipse(5, 4, 2, 2, 2);
    b.ellipse(13, 4, 2, 2, 2);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.4f, 1.4f, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 2);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

void waterTile(gs::VDP& vdp, Art& art) {
    uint8_t px[64];
    for (int i = 0; i < 64; i++) px[i] = ((i / 8) & 1) ? 1 : 2;
    px[3] = 3;
    px[20] = 3;
    px[44] = 3;
    art.waterTile = 1;
    vdp.loadTile(1, px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_FISH, {0, gs::rgb4(12, 13, 14), gs::rgb4(15, 15, 12), gs::rgb4(3, 8, 10), gs::rgb4(15, 15, 15),
                           gs::rgb4(1, 1, 2), gs::rgb4(8, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_ANGLER, {0, gs::rgb4(12, 8, 5), gs::rgb4(6, 8, 4), gs::rgb4(4, 7, 10), gs::rgb4(10, 6, 4),
                             gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(10, 8, 5), gs::rgb4(6, 5, 3), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 3),
                           gs::rgb4(12, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BOARD, {0, gs::rgb4(11, 8, 4), gs::rgb4(7, 5, 3), gs::rgb4(14, 12, 3), gs::rgb4(15, 15, 12), 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 9), gs::rgb4(3, 8, 11), gs::rgb4(8, 12, 13)});
    setPal(vdp, PAL_REED, {0, gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4)});
    setPal(vdp, PAL_BOBBER, {0, gs::rgb4(14, 3, 3), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(3, 4, 6), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(15, 15, 13), gs::rgb4(2, 5, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(15, 14, 8), gs::rgb4(5, 2, 2)});
    setPal(vdp, PAL_LINE, {0, gs::rgb4(14, 14, 12)});

    waterTile(vdp, art);
    art.fish = gs::uploadMipped(vdp, fishArt());
    art.angler = gs::uploadMipped(vdp, anglerArt());
    art.rod = gs::uploadMipped(vdp, rodArt());
    art.bobber = gs::uploadMipped(vdp, bobberArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.title = words(vdp, "FISH", 3, 1, 2);
    art.wordMark = words(vdp, "MARK", 3, 1, 2);
    art.finished = words(vdp, "FINISHED", 2, 1, 2);
    loadFont(vdp, art);
}

}  // namespace fishmark
