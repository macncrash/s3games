#include "game/art.h"

#include <initializer_list>

namespace mushbox {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap mushBlob(float squash) {
    gs::Bitmap b(48, 40);
    float rx = 18.f * (1.f + squash);
    float ry = 15.f * (1.f - squash * 0.65f);
    b.ellipse(24, 22, rx + 1.6f, ry + 1.4f, 1);
    b.ellipse(24, 22, rx, ry, 2);
    b.ellipse(22, 18, rx * 0.55f, ry * 0.42f, 3);
    b.ellipse(16, 22, 3.2f, 2.2f, 4);
    b.ellipse(32, 22, 3.2f, 2.2f, 4);
    b.ellipse(18, 19, 2.4f, 2.6f, 5);
    b.ellipse(30, 19, 2.4f, 2.6f, 5);
    b.ellipse(18.6f, 19.4f, 1.1f, 1.3f, 6);
    b.ellipse(30.6f, 19.4f, 1.1f, 1.3f, 6);
    b.ellipse(24, 25, 2.2f, 1.3f, 7);
    return b;
}

gs::Bitmap boxArt() {
    gs::Bitmap b(96, 72);
    b.rect(0, 0, 96, 72, 1);
    b.rect(6, 6, 84, 60, 2);
    b.rect(10, 10, 76, 52, 3);
    for (int i = 0; i < 4; i++) {
        b.rect(8 + i * 22, 4, 4, 64, 4);
        b.rect(4, 8 + i * 16, 88, 3, 5);
    }
    b.rect(2, 2, 8, 8, 6);
    b.rect(86, 2, 8, 8, 6);
    b.rect(2, 62, 8, 8, 6);
    b.rect(86, 62, 8, 8, 6);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(32, 10);
    b.rect(0, 0, 32, 10, 1);
    b.rect(1, 1, 30, 8, 4);
    b.line(2, 3, 29, 3, 5, 1);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(8, 8);
    b.line(4, 0, 4, 7, 1, 1);
    b.line(0, 4, 7, 4, 1, 1);
    b.set(4, 4, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(16, 7, 14, 5, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
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
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 4),
                          gs::rgb4(6, 14, 7), gs::rgb4(6, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MUSH,
           {0, gs::rgb4(6, 3, 2), gs::rgb4(14, 10, 6), gs::rgb4(15, 14, 10), gs::rgb4(15, 8, 7), gs::rgb4(15, 15, 14),
            gs::rgb4(2, 2, 3), gs::rgb4(10, 4, 4), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOX,
           {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(13, 11, 7), gs::rgb4(8, 5, 2), gs::rgb4(12, 9, 4),
            gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    art.mush[0] = gs::uploadMipped(vdp, mushBlob(0.f));
    art.mush[1] = gs::uploadMipped(vdp, mushBlob(0.28f));
    art.mush[2] = gs::uploadMipped(vdp, mushBlob(-0.18f));
    art.box = gs::uploadMipped(vdp, boxArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(8, 12, 14));
}

}  // namespace mushbox
