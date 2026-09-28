#include "game/art.h"

#include <initializer_list>
#include <string>

namespace qbann {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap crewArt() {
    Bitmap b(34, 56);
    b.ellipse(17, 8, 6, 6, 2);
    b.rect(11, 3, 12, 4, 3);
    b.poly({{17, 14}, {8, 20}, {6, 34}, {28, 34}, {26, 20}}, 4);
    b.rect(13, 18, 8, 10, 5);
    b.rect(6, 20, 5, 12, 6);
    b.rect(23, 20, 5, 12, 6);
    b.rect(10, 34, 5, 16, 7);
    b.rect(19, 34, 5, 16, 7);
    b.rect(8, 48, 8, 4, 1);
    b.rect(18, 48, 8, 4, 1);
    b.outline(15, false);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(40, 52);
    b.rect(6, 2, 3, 48, 3);
    b.poly({{9, 6}, {36, 12}, {34, 28}, {9, 24}}, 2);
    b.poly({{9, 24}, {34, 28}, {32, 40}, {9, 36}}, 4);
    b.rect(14, 14, 14, 3, 5);
    b.outline(15, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(36, 28);
    b.poly({{4, 22}, {10, 8}, {18, 4}, {28, 10}, {34, 20}, {22, 26}, {8, 26}}, 2);
    b.poly({{12, 16}, {18, 10}, {24, 16}, {16, 20}}, 3);
    b.outline(15, false);
    return b;
}

Bitmap cliffArt() {
    Bitmap b(44, 88);
    b.poly({{6, 84}, {4, 30}, {14, 10}, {24, 18}, {34, 6}, {42, 84}}, 2);
    b.poly({{14, 70}, {16, 36}, {26, 30}, {32, 72}}, 3);
    b.rect(18, 44, 8, 5, 4);
    b.rect(22, 58, 7, 4, 1);
    b.outline(15, false);
    return b;
}

Bitmap millArt() {
    Bitmap b(64, 58);
    b.rect(6, 24, 52, 26, 2);
    b.poly({{10, 24}, {32, 6}, {54, 24}}, 3);
    b.rect(26, 12, 12, 12, 1);
    b.rect(12, 32, 14, 8, 5);
    b.rect(36, 32, 14, 8, 6);
    b.rect(2, 48, 60, 6, 4);
    b.outline(15, false);
    return b;
}

Bitmap hopperArt() {
    Bitmap b(36, 44);
    b.poly({{4, 6}, {32, 6}, {26, 34}, {10, 34}}, 3);
    b.rect(12, 34, 12, 6, 5);
    b.rect(8, 2, 20, 5, 1);
    b.outline(15, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 40);
    b.rect(6, 4, 4, 34, 2);
    b.rect(2, 36, 12, 3, 3);
    b.rect(4, 6, 8, 4, 4);
    b.outline(15, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(18, 14);
    b.ellipse(9, 7, 7, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(26, 8);
    b.ellipse(13, 4, 11, 3, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    gs::TextStyle big{2, 1, 0, 0, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (g[row] & (1 << (4 - col))) px[row * 8 + col] = 1;
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(8, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), gs::rgb4(10, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(15, 12, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(5, 14, 6), gs::rgb4(13, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(3, 2, 2), gs::rgb4(13, 2, 2), gs::rgb4(6, 4, 3), gs::rgb4(9, 1, 1),
                             gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(8, 7, 6), gs::rgb4(5, 5, 4), gs::rgb4(10, 9, 7), gs::rgb4(3, 3, 2),
                           gs::rgb4(12, 11, 9), gs::rgb4(7, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MILL, {0, gs::rgb4(12, 12, 13), gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 3),
                           gs::rgb4(12, 8, 3), gs::rgb4(14, 11, 5), gs::rgb4(8, 5, 3), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(2, 2, 2), gs::rgb4(12, 9, 6), gs::rgb4(4, 3, 2), gs::rgb4(14, 11, 2),
                           gs::rgb4(8, 10, 12), gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 10, 7), gs::rgb4(8, 7, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(6, 5, 3),
        gs::rgb4(4, 3, 2),
        gs::rgb4(7, 6, 4),
        gs::rgb4(5, 4, 3),
        gs::rgb4(2, 2, 1),
        gs::rgb4(3, 3, 2),
        gs::rgb4(3, 2, 2),
        gs::rgb4(6, 5, 4),
        gs::rgb4(5, 4, 3),
        gs::rgb4(8, 6, 4),
        gs::rgb4(3, 4, 3),
        gs::rgb4(4, 5, 3),
        gs::rgb4(5, 6, 4),
        gs::rgb4(14, 10, 3),
        gs::rgb4(7, 6, 4),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.crew = gs::uploadMipped(vdp, crewArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.mill = gs::uploadMipped(vdp, millArt());
    art.hopper = gs::uploadMipped(vdp, hopperArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);
}

}  // namespace qbann
