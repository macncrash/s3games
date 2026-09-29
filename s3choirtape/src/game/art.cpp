#include "art.h"

#include <initializer_list>

namespace choirtape {
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
    int robe = voice == 3 ? 6 : 3;
    b.ellipse(12, 8, 6.f, 6.f, 2);
    b.rect(10, 13, 5, 3, 2);
    b.poly({{4, 18}, {20, 18}, {18, 40}, {6, 40}}, robe);
    b.rect(6, 39, 4, 8, 5);
    b.rect(14, 39, 4, 8, 5);
    b.rect(5, 46, 6, 2, 7);
    b.rect(13, 46, 6, 2, 7);
    b.rect(10, 6, 1, 2, 1);
    b.rect(14, 6, 1, 2, 1);
    b.rect(11, 10, 3, 1, 8);
    if (voice == 2) b.rect(9, 12, 6, 2, 5);
    if (voice == 3) b.ellipse(12, 9, 2.2f, 1.4f, 4);
}

void paintReel(gs::Bitmap& b) {
    b.ellipse(16, 16, 14.f, 14.f, 3);
    b.ellipse(16, 16, 8.f, 8.f, 1);
    b.ellipse(16, 16, 3.f, 3.f, 4);
    b.rect(15, 2, 2, 6, 2);
    b.rect(15, 24, 2, 6, 2);
}

void paintSlip(gs::Bitmap& b) {
    b.rect(0, 0, 28, 14, 1);
    b.rect(2, 3, 18, 2, 2);
    b.rect(2, 7, 12, 2, 3);
}

void paintNote(gs::Bitmap& b) {
    b.ellipse(5, 11, 4.f, 3.f, 1);
    b.rect(8, 2, 2, 10, 1);
    b.line(10, 2, 16, 4, 1, 1.2f);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(4, 3, 3)});
    setPal(vdp, PAL_NAVE, {0, gs::rgb4(5, 4, 8), gs::rgb4(2, 1, 4)});
    setPal(vdp, PAL_TREBLE, {0, gs::rgb4(15, 14, 9), gs::rgb4(12, 8, 5), gs::rgb4(10, 12, 15), gs::rgb4(4, 6, 12),
                             gs::rgb4(3, 3, 7), gs::rgb4(2, 2, 4), gs::rgb4(6, 5, 8), gs::rgb4(1, 1, 1),
                             gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_ALTO, {0, gs::rgb4(15, 14, 9), gs::rgb4(12, 8, 5), gs::rgb4(8, 14, 8), gs::rgb4(3, 8, 4),
                           gs::rgb4(2, 4, 3), gs::rgb4(1, 2, 2), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 1),
                           gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_BASS, {0, gs::rgb4(15, 14, 9), gs::rgb4(12, 8, 5), gs::rgb4(12, 7, 14), gs::rgb4(6, 2, 8),
                           gs::rgb4(3, 1, 4), gs::rgb4(2, 1, 2), gs::rgb4(5, 4, 6), gs::rgb4(1, 1, 1),
                           gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_HUM, {0, gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5),
                          gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(8, 7, 6), gs::rgb4(1, 1, 1),
                          gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(14, 12, 8), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1)});
    vdp.setFogColor(gs::rgb4(2, 1, 4));

    loadFont(vdp, art);
    for (int i = 0; i < kVoices; i++) {
        gs::Bitmap body(24, 50);
        paintSinger(body, i);
        art.singer[i] = gs::uploadMipped(vdp, body);
    }
    gs::Bitmap reel(32, 32);
    paintReel(reel);
    art.reel = gs::uploadMipped(vdp, reel);
    gs::Bitmap slip(28, 14);
    paintSlip(slip);
    art.slip = gs::uploadMipped(vdp, slip);
    gs::Bitmap note(18, 14);
    paintNote(note);
    art.note = gs::uploadMipped(vdp, note);
    gs::Bitmap solid(4, 4);
    solid.rect(0, 0, 4, 4, 1);
    art.solid = gs::uploadMipped(vdp, solid);
    art.wordChoir = gs::uploadMipped(vdp, gs::textBitmap("CHOIR", {3, 1, 2, 0, 1}));
}

}  // namespace choirtape
