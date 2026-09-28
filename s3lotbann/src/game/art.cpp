#include "game/art.h"

#include <string>

namespace lotbann {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap runnerArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 26, 8, 4, 4);
    b.rect(10, 12, 12, 12, 1);
    b.rect(8, 16, 4, 8, 1);
    b.rect(20, 16, 4, 8, 1);
    b.ellipse(16, 9, 6, 6, 2);
    b.rect(11, 5, 10, 4, 3);
    b.rect(13, 18, 6, 5, 5);
    b.outline(6, false);
    return b;
}

Bitmap carArt() {
    Bitmap b(64, 36);
    b.ellipse(14, 28, 6, 4, 4);
    b.ellipse(50, 28, 6, 4, 4);
    b.rect(6, 10, 52, 16, 1);
    b.ellipse(32, 16, 28, 10, 1);
    b.rect(18, 8, 28, 12, 2);
    b.rect(20, 10, 11, 7, 3);
    b.rect(33, 10, 11, 7, 3);
    b.rect(8, 14, 6, 4, 5);
    b.rect(50, 14, 6, 4, 5);
    b.outline(6, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(40, 56);
    b.rect(18, 8, 4, 46, 4);
    b.rect(22, 10, 16, 22, 1);
    b.rect(22, 16, 16, 4, 3);
    b.rect(22, 24, 16, 4, 2);
    b.ellipse(20, 52, 5, 3, 5);
    b.outline(6, false);
    return b;
}

Bitmap boothArt() {
    Bitmap b(80, 56);
    b.rect(8, 18, 64, 32, 1);
    b.poly({{6, 20}, {40, 4}, {74, 20}}, 2);
    b.rect(34, 30, 14, 20, 4);
    b.rect(14, 26, 14, 12, 3);
    b.rect(52, 26, 14, 12, 3);
    b.rect(16, 28, 10, 8, 5);
    b.rect(54, 28, 10, 8, 5);
    b.outline(6, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(24, 48);
    b.rect(10, 16, 4, 30, 2);
    b.ellipse(12, 10, 8, 6, 1);
    b.ellipse(12, 9, 4, 3, 3);
    b.ellipse(12, 46, 6, 2, 4);
    return b;
}

Bitmap dashArt() {
    Bitmap b(8, 8);
    b.rect(0, 2, 8, 4, 1);
    return b;
}

Bitmap padArt() {
    Bitmap b(48, 32);
    b.rect(2, 4, 44, 24, 1);
    b.rect(8, 10, 32, 12, 2);
    return b;
}

Bitmap puffArt() {
    Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 10, 1);
    b.ellipse(14, 14, 6, 5, 2);
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
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(15, 12, 4), gs::rgb4(8, 14, 6), gs::rgb4(15, 5, 4), gs::rgb4(6, 8, 12),
                          gs::rgb4(10, 10, 11), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 3), gs::rgb4(14, 14, 13), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    auto carPal = [&](int pal, uint16_t body, uint16_t roof) {
        setPal(vdp, pal,
               {0, body, roof, gs::rgb4(8, 12, 14), gs::rgb4(1, 1, 1), gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0,
                shadow});
    };
    carPal(PAL_CAR, gs::rgb4(13, 3, 3), gs::rgb4(8, 2, 2));
    carPal(PAL_CARB, gs::rgb4(3, 6, 13), gs::rgb4(2, 3, 8));
    carPal(PAL_CARC, gs::rgb4(12, 11, 4), gs::rgb4(7, 6, 2));
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(4, 8, 14), gs::rgb4(14, 11, 8), gs::rgb4(2, 3, 6), gs::rgb4(2, 2, 3), gs::rgb4(15, 12, 3), gs::rgb4(1, 1, 2), 0,
            0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(14, 2, 3), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 15), gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 3), gs::rgb4(1, 1, 1), 0, 0,
            0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOOTH,
           {0, gs::rgb4(9, 8, 6), gs::rgb4(12, 4, 3), gs::rgb4(5, 8, 11), gs::rgb4(3, 2, 2), gs::rgb4(13, 14, 15), gs::rgb4(1, 1, 1), 0, 0,
            0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(5, 5, 6), gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    art.runner = gs::uploadMipped(vdp, runnerArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.booth = gs::uploadMipped(vdp, boothArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.dash = gs::uploadMipped(vdp, dashArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    vdp.setFogColor(gs::rgb4(2, 2, 3));
}

}  // namespace lotbann
