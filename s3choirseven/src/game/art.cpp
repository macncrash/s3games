#include "art.h"

#include <initializer_list>

namespace choirseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void paintSinger(gs::Bitmap& b, int voice) {
    int robe = voice == 0 ? 3 : voice == 1 ? 4 : 5;
    b.ellipse(16, 10, 7.f, 7.f, 2);
    b.rect(13, 16, 6, 4, 2);
    b.poly({{6, 22}, {26, 22}, {22, 52}, {10, 52}}, robe);
    b.rect(8, 50, 6, 12, 6);
    b.rect(18, 50, 6, 12, 6);
    b.rect(7, 60, 8, 3, 7);
    b.rect(17, 60, 8, 3, 7);
    b.rect(14, 8, 2, 2, 1);
    b.rect(18, 8, 2, 2, 1);
    b.rect(15, 12, 4, 1, 8);
    if (voice == 2) b.rect(12, 14, 8, 2, 6);
}

void paintNote(gs::Bitmap& b) {
    b.ellipse(6, 12, 5.f, 4.f, 1);
    b.rect(10, 2, 2, 12, 1);
    b.line(12, 2, 18, 4, 1, 1.4f);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_NAVE, {0, gs::rgb4(6, 5, 8), gs::rgb4(3, 2, 5)});
    setPal(vdp, PAL_TREBLE, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 8, 5), gs::rgb4(15, 12, 9), gs::rgb4(8, 10, 15),
                             gs::rgb4(5, 6, 12), gs::rgb4(3, 3, 8), gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1),
                             gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_ALTO, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 8, 5), gs::rgb4(9, 14, 8), gs::rgb4(4, 10, 5),
                           gs::rgb4(2, 6, 4), gs::rgb4(1, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1),
                           gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_BASS, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 8, 5), gs::rgb4(12, 8, 14), gs::rgb4(7, 3, 10),
                           gs::rgb4(4, 2, 6), gs::rgb4(2, 1, 3), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1),
                           gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(12, 6, 8), gs::rgb4(5, 2, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1)});
    vdp.setFogColor(gs::rgb4(2, 1, 4));

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) {
        gs::Bitmap body(32, 64);
        paintSinger(body, i);
        art.singer[i] = gs::uploadMipped(vdp, body);
    }
    gs::Bitmap note(20, 16);
    paintNote(note);
    art.note = gs::uploadMipped(vdp, note);

    gs::Bitmap bar(4, 4);
    bar.rect(0, 0, 4, 4, 1);
    art.bar = gs::uploadMipped(vdp, bar);

    gs::Bitmap pip(8, 8);
    pip.ellipse(4, 4, 3.2f, 3.2f, 1);
    art.pip = gs::uploadMipped(vdp, pip);

    art.wordChoir = gs::uploadMipped(vdp, gs::textBitmap("CHOIR", {3, 1, 2, 0, 1}));
    art.wordSeven = gs::uploadMipped(vdp, gs::textBitmap("FIRST TO SEVEN", {2, 1, 2, 0, 1}));
}

}  // namespace choirseven
