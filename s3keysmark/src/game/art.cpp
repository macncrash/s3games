#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace keysmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap noteArt() {
    gs::Bitmap b(12, 10);
    b.ellipse(6, 5, 5.2f, 3.6f, 2);
    b.ellipse(6, 5, 3.4f, 2.2f, 1);
    b.rect(10, 1, 1, 6, 2);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(28, 40);
    b.rect(1, 1, 26, 38, 1);
    b.rect(2, 2, 22, 4, 2);
    b.rect(3, 30, 22, 7, 3);
    b.rect(12, 16, 4, 6, 4);
    return b;
}

gs::Bitmap staffArt() {
    gs::Bitmap b(8, 2);
    b.rect(0, 0, 8, 2, 1);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(4, 16);
    b.rect(1, 0, 2, 16, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.2f, 4.2f, 2);
    b.ellipse(5, 5, 2.4f, 2.4f, 1);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(12, 10);
    b.poly({{6, 1}, {11, 9}, {1, 9}}, 1);
    b.poly({{6, 4}, {9, 8}, {3, 8}}, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Image phrase(gs::VDP& vdp, const char* s) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {2, 1, 2, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(8, 9, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_IVORY, {0, gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 14), gs::rgb4(7, 6, 5), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 3, 2), gs::rgb4(10, 6, 3)});
    setPal(vdp, PAL_NOTE, {0, gs::rgb4(14, 12, 10), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_PLAYED, {0, gs::rgb4(15, 13, 5), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 12, 4), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(15, 14, 6), gs::rgb4(10, 8, 2)});

    loadFont(vdp, art);
    art.note = gs::uploadMipped(vdp, noteArt());
    art.key = gs::uploadMipped(vdp, keyArt());
    art.staff = gs::uploadMipped(vdp, staffArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.hand = gs::uploadMipped(vdp, handArt());
    art.logo = phrase(vdp, "S3 KEYSMARK");
    art.finished = phrase(vdp, "FINISHED MARK");
    art.open = phrase(vdp, "STILL OPEN");
}

}  // namespace keysmark
