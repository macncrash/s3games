#include "art.h"

#include <initializer_list>
#include <string>

namespace beacon {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i, c);
        i++;
    }
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    big.spacing = 1;
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

gs::Bitmap walker(int step) {
    gs::Bitmap b(22, 40);
    b.ellipse(11, 6, 4, 4, 4);
    b.rect(7, 2, 8, 3, 5);
    b.rect(8, 10, 6, 3, 6);
    b.rect(6, 13, 10, 12, 1);
    b.rect(8, 15, 6, 6, 2);
    float arm = step ? 2.f : 14.f;
    b.rect(arm, 14, 5, 3, 3);
    b.rect(step ? 7.f : 11.f, 25, 3, 10, 1);
    b.rect(step ? 12.f : 8.f, 25, 3, 10, 2);
    b.rect(6, 34, 5, 3, 7);
    b.rect(12, 34, 5, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap downed() {
    gs::Bitmap b(40, 16);
    b.ellipse(8, 8, 4, 4, 4);
    b.rect(11, 6, 20, 6, 1);
    b.rect(13, 7, 8, 3, 2);
    b.rect(28, 7, 8, 3, 3);
    b.rect(18, 11, 6, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(36, 96);
    b.poly({{18, 2}, {6, 22}, {30, 22}}, 2);
    b.rect(10, 20, 16, 68, 1);
    b.rect(12, 24, 12, 14, 3);
    b.rect(13, 26, 10, 10, 4);
    for (int y = 44; y < 80; y += 10) b.rect(14, float(y), 8, 3, 5);
    b.rect(15, 78, 6, 12, 6);
    b.rect(4, 88, 28, 6, 2);
    b.rect(16, 6, 4, 8, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(18, 16);
    b.rect(3, 3, 12, 10, 1);
    b.rect(5, 5, 8, 6, 2);
    b.line(3, 3, 15, 3, 3, 1.2f);
    b.line(3, 13, 15, 13, 3, 1.2f);
    b.rect(7, 0, 4, 3, 4);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(10, 14);
    b.ellipse(5, 9, 3, 4, 1);
    b.ellipse(5, 6, 2, 4, 2);
    b.ellipse(5, 4, 1, 2, 3);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 0, 2, 28, 1);
    b.rect(1, 4, 6, 2, 2);
    b.rect(1, 24, 6, 3, 3);
    return b;
}

gs::Bitmap spurArt() {
    gs::Bitmap b(28, 16);
    b.poly({{1, 14}, {8, 6}, {16, 10}, {26, 3}, {27, 14}}, 1);
    b.poly({{6, 13}, {12, 8}, {18, 12}}, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 0);
    b.line(6, 0, 6, 3, 2, 1.f);
    b.line(6, 9, 6, 12, 2, 1.f);
    b.line(0, 6, 3, 6, 2, 1.f);
    b.line(9, 6, 12, 6, 2, 1.f);
    return b;
}

gs::Bitmap carbineArt() {
    gs::Bitmap b(16, 36);
    b.rect(6, 0, 4, 22, 1);
    b.rect(5, 18, 6, 8, 2);
    b.rect(7, 26, 3, 8, 3);
    b.rect(4, 8, 3, 4, 4);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap flareArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(12, 8);
    b.ellipse(6, 4, 5, 3, 1);
    b.ellipse(4, 3, 2, 1, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(9, 8, 8), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 10), gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 10, 8), gs::rgb4(5, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 7), gs::rgb4(13, 15, 11), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 6), gs::rgb4(12, 14, 15),
                            gs::rgb4(3, 3, 4), gs::rgb4(8, 7, 5), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(3, 4, 7), gs::rgb4(5, 6, 9), gs::rgb4(2, 2, 4), gs::rgb4(12, 9, 6), gs::rgb4(1, 1, 2),
                           gs::rgb4(8, 7, 5), gs::rgb4(4, 3, 3), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 9), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(12, 15, 9), gs::rgb4(4, 11, 6), gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 4, 3), gs::rgb4(15, 13, 5), gs::rgb4(8, 6, 2), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SEA, {0, gs::rgb4(2, 5, 8), gs::rgb4(4, 8, 11), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(11, 9, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 4, 4), gs::rgb4(2, 3, 3), gs::rgb4(4, 5, 5),
        gs::rgb4(5, 5, 4), gs::rgb4(3, 4, 3),
        gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 4),
        gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(7, 7, 6),
        gs::rgb4(1, 3, 6), gs::rgb4(2, 5, 8), gs::rgb4(3, 6, 9),
        gs::rgb4(12, 11, 6), gs::rgb4(9, 8, 7),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.step[0] = gs::uploadMipped(vdp, walker(0));
    art.step[1] = gs::uploadMipped(vdp, walker(1));
    art.down = gs::uploadMipped(vdp, downed());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.spur = gs::uploadMipped(vdp, spurArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.carbine = gs::uploadMipped(vdp, carbineArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace beacon
