#include "game/art.h"

namespace trench {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap truckArt() {
    gs::Bitmap b(48, 36);
    b.rect(2, 14, 36, 14, 3);
    b.rect(28, 6, 16, 16, 4);
    b.rect(32, 9, 8, 6, 8);
    b.rect(6, 16, 10, 6, 2);
    b.ellipse(12, 28, 6, 6, 1);
    b.ellipse(34, 28, 6, 6, 1);
    b.ellipse(12, 28, 2, 2, 6);
    b.ellipse(34, 28, 2, 2, 6);
    b.rect(0, 18, 4, 3, 5);
    b.outline(7, false);
    return b;
}

gs::Bitmap youArt() {
    gs::Bitmap b(24, 32);
    b.ellipse(12, 6, 5, 5, 3);
    b.rect(7, 12, 10, 12, 4);
    b.rect(4, 13, 4, 8, 4);
    b.rect(16, 14, 6, 3, 2);
    b.rect(8, 24, 3, 7, 5);
    b.rect(13, 24, 3, 7, 5);
    b.rect(18, 13, 4, 2, 1);
    b.outline(7, false);
    return b;
}

gs::Bitmap bagArt() {
    gs::Bitmap b(40, 18);
    b.ellipse(20, 10, 18, 7, 3);
    b.ellipse(10, 9, 8, 6, 4);
    b.ellipse(28, 11, 9, 6, 2);
    b.rect(16, 6, 8, 4, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 2, 3, 24, 3);
    b.poly({{2, 4}, {5, 0}, {8, 4}}, 4);
    b.rect(3, 24, 5, 3, 2);
    return b;
}

gs::Bitmap cableArt() {
    gs::Bitmap b(32, 8);
    b.rect(0, 2, 32, 4, 3);
    for (int x = 0; x < 32; x += 8) b.rect(x, 2, 4, 4, 4);
    return b;
}

gs::Bitmap flareArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 3);
    b.ellipse(8, 8, 4, 4, 4);
    b.ellipse(8, 8, 2, 2, 6);
    return b;
}

gs::Bitmap wireArt() {
    gs::Bitmap b(48, 10);
    b.line(0, 6, 16, 2, 2, 1);
    b.line(16, 2, 32, 7, 2, 1);
    b.line(32, 7, 47, 3, 2, 1);
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
    const uint16_t ink = gs::rgb4(15, 14, 11);
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 8, 7), gs::rgb4(15, 14, 8), gs::rgb4(14, 4, 3), gs::rgb4(4, 12, 5),
                          gs::rgb4(14, 11, 4), gs::rgb4(6, 7, 8), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BAG, {0, gs::rgb4(4, 3, 2), gs::rgb4(8, 7, 4), gs::rgb4(11, 9, 5), gs::rgb4(13, 11, 7),
                          gs::rgb4(6, 5, 3), gs::rgb4(9, 8, 6), shadow, ink});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 4), gs::rgb4(12, 9, 6), gs::rgb4(6, 7, 4),
                          gs::rgb4(4, 4, 3), gs::rgb4(14, 12, 8), shadow, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(1, 1, 1), gs::rgb4(3, 4, 2), gs::rgb4(5, 6, 3), gs::rgb4(7, 8, 4),
                            gs::rgb4(10, 8, 3), gs::rgb4(8, 8, 7), shadow, gs::rgb4(12, 13, 14)});
    setPal(vdp, PAL_CABLE, {0, shadow, gs::rgb4(8, 7, 2), gs::rgb4(14, 12, 2), gs::rgb4(2, 2, 2), gs::rgb4(12, 4, 2),
                            0, shadow, ink});
    setPal(vdp, PAL_FLARE, {0, gs::rgb4(6, 1, 0), gs::rgb4(12, 4, 1), gs::rgb4(15, 8, 1), gs::rgb4(15, 13, 3),
                            gs::rgb4(15, 15, 10), 0, shadow, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 6), gs::rgb4(4, 10, 4), shadow, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(10, 2, 2), shadow, ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STAKE, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 5, 3), gs::rgb4(8, 6, 3), gs::rgb4(10, 8, 4),
                            gs::rgb4(5, 4, 2), 0, shadow, ink});
    // Road bank 12: ground, verge, tarmac, paint.
    uint16_t road[16] = {};
    road[1] = gs::rgb4(4, 6, 3);
    road[2] = gs::rgb4(3, 5, 2);
    road[3] = gs::rgb4(6, 7, 3);
    road[4] = gs::rgb4(5, 5, 3);
    road[5] = gs::rgb4(4, 4, 2);
    road[6] = gs::rgb4(4, 4, 4);
    road[7] = gs::rgb4(3, 3, 3);
    road[8] = gs::rgb4(6, 6, 5);
    road[9] = gs::rgb4(2, 2, 2);
    road[10] = gs::rgb4(5, 5, 4);
    road[14] = gs::rgb4(12, 11, 6);
    road[15] = gs::rgb4(5, 5, 5);
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.truck = gs::uploadMipped(vdp, truckArt());
    art.you = gs::uploadMipped(vdp, youArt());
    art.bag = gs::uploadMipped(vdp, bagArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.wire = gs::uploadMipped(vdp, wireArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(7, 8, 8));
}

}  // namespace trench
