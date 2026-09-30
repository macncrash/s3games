#include "game/art.h"

namespace foundrydawn {
namespace {

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int tile = tiles.shared(px);
        vdp.loadTile(tile, px);
        a.font[c - 32] = tile;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

int solidTile(gs::VDP& vdp, gs::TileAlloc& tiles, const uint8_t px[64]) {
    int tile = tiles.shared(px);
    vdp.loadTile(tile, px);
    return tile;
}

gs::Bitmap manBmp(int step) {
    gs::Bitmap b(16, 30);
    b.ellipse(8, 5, 3.1f, 3.2f, 4);
    b.rect(6, 8, 4, 2, 5);
    b.rect(5, 10, 6, 8, 2);
    b.rect(6, 11, 4, 5, 3);
    b.rect(2, 11, 3, 6, 6);
    b.rect(11, 12, 4, 3, 7);
    int kick = step ? 2 : 0;
    b.rect(5, 18, 3, 9 - kick, 6);
    b.rect(9, 18 + kick, 3, 9 - kick, 6);
    b.rect(4, 26 - kick, 4, 2, 1);
    b.rect(8, 26, 4, 2, 1);
    b.set(7, 4, 8);
    b.set(10, 4, 8);
    b.rect(12, 14, 3, 2, 9);
    return b;
}

gs::Bitmap crucibleBmp() {
    gs::Bitmap b(22, 18);
    b.poly({{2, 4}, {20, 4}, {17, 15}, {5, 15}}, 2);
    b.rect(1, 3, 20, 3, 3);
    b.rect(6, 6, 10, 6, 4);
    b.rect(4, 15, 3, 3, 1);
    b.rect(15, 15, 3, 3, 1);
    b.rect(8, 7, 6, 2, 5);
    return b;
}

gs::Bitmap flameBmp(int flick) {
    gs::Bitmap b(12, 18);
    b.ellipse(6, 13, 4.f, 4.2f, 2);
    b.ellipse(6, 9, 2.6f, 4.4f, 3);
    b.ellipse(6 + flick, 5, 1.4f, 3.f, 4);
    b.set(6, 2, 5);
    return b;
}

gs::Bitmap slagBmp() {
    gs::Bitmap b(14, 12);
    b.ellipse(7, 6, 5.5f, 4.2f, 2);
    b.ellipse(6, 5, 2.4f, 1.8f, 3);
    b.set(9, 7, 4);
    b.set(4, 8, 5);
    return b;
}

gs::Bitmap quenchBmp() {
    gs::Bitmap b(10, 16);
    b.rect(3, 0, 4, 14, 2);
    b.ellipse(5, 13, 4.f, 2.4f, 3);
    b.rect(4, 2, 2, 8, 4);
    return b;
}

gs::Bitmap coalBmp() {
    gs::Bitmap b(8, 7);
    b.ellipse(4, 4, 3.2f, 2.4f, 2);
    b.set(3, 3, 3);
    b.set(5, 4, 1);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(4, 4);
    b.set(1, 0, 1);
    b.set(2, 1, 1);
    b.set(1, 1, 2);
    b.set(0, 1, 1);
    b.set(1, 2, 1);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 2);
    b.ellipse(9, 6, 4.2f, 4.2f, 0);
    b.set(4, 5, 3);
    b.set(5, 9, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {C(0, 0, 0), C(14, 12, 8), C(7, 6, 5), C(3, 2, 2), C(15, 10, 3),
                            C(15, 6, 2), C(2, 2, 2),  C(12, 4, 1), C(5, 7, 8),  C(3, 4, 6),
                            C(10, 9, 8), C(15, 14, 8), C(9, 6, 2), C(15, 15, 11), C(4, 3, 3), C(1, 1, 1)};
    const uint16_t soot[] = {C(0, 0, 0), C(3, 2, 2), C(5, 3, 2), C(2, 1, 1), C(7, 4, 3),
                             C(4, 3, 2),  C(6, 5, 4), C(1, 1, 1), C(8, 5, 3), C(9, 6, 4),
                             C(2, 2, 2),  C(10, 6, 3), C(6, 6, 5), C(12, 8, 4), C(3, 2, 3), C(0, 0, 0)};
    const uint16_t floor[] = {C(0, 0, 0), C(2, 2, 2), C(4, 3, 3), C(5, 4, 3), C(3, 2, 2),
                              C(6, 5, 4),  C(7, 4, 2), C(8, 6, 4), C(1, 1, 1), C(9, 7, 4),
                              C(3, 3, 3),  C(10, 8, 5), C(6, 3, 2), C(12, 9, 5), C(4, 4, 4), C(0, 0, 0)};
    const uint16_t man[] = {C(0, 0, 0), C(2, 2, 2), C(4, 3, 2), C(6, 4, 3), C(12, 8, 6),
                            C(8, 5, 4),  C(3, 3, 4), C(5, 5, 6), C(1, 1, 1), C(14, 10, 4),
                            C(9, 7, 5),  C(7, 6, 5), C(10, 8, 6), C(15, 12, 6), C(2, 1, 1), C(0, 0, 0)};
    const uint16_t fire[] = {C(0, 0, 0), C(8, 2, 0), C(14, 5, 0), C(15, 10, 1), C(15, 14, 4),
                             C(15, 15, 10), C(6, 1, 0), C(12, 3, 0), C(10, 6, 1), C(4, 1, 0),
                             C(15, 8, 2), C(9, 4, 1), C(7, 3, 1), C(14, 12, 6), C(3, 1, 0), C(0, 0, 0)};
    const uint16_t ember[] = {C(0, 0, 0), C(10, 3, 0), C(14, 6, 1), C(15, 10, 2), C(8, 2, 0),
                              C(4, 1, 0),  C(12, 4, 1), C(6, 2, 1), C(15, 12, 4), C(3, 1, 1),
                              C(9, 5, 2),  C(7, 3, 1), C(11, 7, 2), C(2, 1, 0), C(13, 8, 3), C(0, 0, 0)};
    const uint16_t iron[] = {C(0, 0, 0), C(2, 2, 3), C(5, 5, 6), C(7, 7, 8), C(9, 8, 6),
                             C(12, 6, 2), C(3, 3, 4), C(10, 10, 11), C(4, 4, 5), C(8, 6, 4),
                             C(1, 1, 2),  C(11, 9, 6), C(6, 6, 7), C(13, 11, 7), C(4, 5, 6), C(0, 0, 0)};
    const uint16_t slag[] = {C(0, 0, 0), C(6, 2, 0), C(12, 4, 1), C(15, 8, 2), C(15, 12, 4),
                             C(8, 3, 1),  C(4, 1, 0), C(10, 5, 1), C(14, 6, 2), C(9, 4, 1),
                             C(3, 1, 0),  C(13, 7, 2), C(7, 3, 1), C(15, 10, 3), C(5, 2, 1), C(0, 0, 0)};
    const uint16_t quench[] = {C(0, 0, 0), C(2, 4, 8), C(4, 8, 12), C(8, 12, 15), C(12, 14, 15),
                               C(1, 2, 4),  C(3, 6, 10), C(6, 10, 14), C(9, 13, 15), C(2, 5, 8),
                               C(5, 9, 12), C(7, 11, 14), C(10, 14, 15), C(3, 7, 11), C(1, 3, 6), C(0, 0, 0)};
    const uint16_t smoke[] = {C(0, 0, 0), C(3, 3, 3), C(5, 5, 5), C(7, 7, 7), C(2, 2, 2),
                              C(4, 4, 4),  C(6, 6, 6), C(8, 8, 8), C(9, 8, 7), C(1, 1, 1),
                              C(10, 9, 8), C(5, 4, 4), C(7, 6, 5), C(3, 3, 4), C(6, 5, 4), C(0, 0, 0)};
    const uint16_t moon[] = {C(0, 0, 0), C(8, 8, 10), C(13, 13, 15), C(6, 6, 8), C(4, 4, 6),
                             C(11, 11, 13), C(3, 3, 5), C(15, 15, 15), C(9, 9, 11), C(7, 7, 9),
                             C(2, 2, 4), C(10, 10, 12), C(12, 12, 14), C(5, 5, 7), C(1, 1, 3), C(0, 0, 0)};
    const uint16_t sun[] = {C(0, 0, 0), C(15, 9, 3), C(15, 12, 5), C(15, 15, 8), C(12, 6, 2),
                            C(14, 8, 2), C(10, 5, 1), C(15, 13, 6), C(8, 4, 1), C(13, 9, 3),
                            C(15, 14, 10), C(11, 6, 2), C(9, 5, 2), C(14, 11, 4), C(7, 3, 1), C(0, 0, 0)};
    const uint16_t gold[] = {C(0, 0, 0), C(12, 9, 2), C(15, 12, 3), C(15, 14, 6), C(8, 6, 1),
                             C(10, 7, 2), C(14, 11, 4), C(6, 4, 1), C(13, 10, 3), C(15, 13, 5),
                             C(9, 6, 1), C(11, 8, 2), C(7, 5, 1), C(14, 12, 6), C(5, 3, 1), C(1, 1, 0)};
    const uint16_t alert[] = {C(0, 0, 0), C(12, 2, 1), C(15, 4, 2), C(15, 8, 3), C(8, 1, 1),
                              C(6, 1, 0),  C(14, 6, 2), C(10, 2, 1), C(15, 12, 6), C(4, 0, 0),
                              C(13, 3, 1), C(9, 2, 1), C(15, 10, 4), C(7, 1, 1), C(11, 4, 2), C(2, 0, 0)};
    const uint16_t pip[] = {C(0, 0, 0), C(10, 9, 7), C(6, 5, 4), C(14, 12, 8), C(4, 3, 3),
                            C(12, 8, 3), C(8, 7, 5), C(15, 14, 10), C(3, 2, 2), C(9, 6, 3),
                            C(5, 4, 3),  C(11, 10, 8), C(7, 6, 4), C(13, 11, 6), C(2, 2, 2), C(1, 1, 1)};
    const uint16_t coal[] = {C(0, 0, 0), C(2, 2, 2), C(4, 4, 4), C(6, 5, 4), C(3, 3, 3),
                             C(5, 4, 3),  C(7, 6, 5), C(1, 1, 1), C(8, 7, 5), C(9, 6, 3),
                             C(3, 2, 2),  C(6, 6, 6), C(4, 3, 2), C(10, 8, 5), C(2, 2, 1), C(0, 0, 0)};

    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_SOOT, soot);
    setPal(vdp, PAL_FLOOR, floor);
    setPal(vdp, PAL_MAN, man);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_EMBER, ember);
    setPal(vdp, PAL_IRON, iron);
    setPal(vdp, PAL_SLAG, slag);
    setPal(vdp, PAL_QUENCH, quench);
    setPal(vdp, PAL_SMOKE, smoke);
    setPal(vdp, PAL_MOON, moon);
    setPal(vdp, PAL_SUN, sun);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_PIP, pip);
    setPal(vdp, PAL_COAL, coal);
    vdp.setFogColor(C(2, 1, 1));

    gs::TileAlloc tiles(vdp);
    uint8_t sootPx[64], beamPx[64], platePx[64], gratePx[64], darkPx[64];
    for (int i = 0; i < 64; i++) {
        int x = i % 8, y = i / 8;
        sootPx[i] = ((y == 0 || y == 4) ? 3 : ((x + (y < 4 ? 0 : 4)) % 8 == 0 ? 1 : 2));
        beamPx[i] = (x == 0 || x == 7) ? 1 : (y % 3 == 0 ? 5 : 2);
        platePx[i] = (y > 5) ? 4 : ((x + y) % 5 == 0 ? 1 : 2);
        gratePx[i] = (y % 2 == 0 || x % 3 == 0) ? 6 : 3;
        darkPx[i] = (x == 0 || y == 7) ? 1 : 8;
    }
    art.soot = solidTile(vdp, tiles, sootPx);
    art.beam = solidTile(vdp, tiles, beamPx);
    art.plate = solidTile(vdp, tiles, platePx);
    art.grate = solidTile(vdp, tiles, gratePx);
    art.dark = solidTile(vdp, tiles, darkPx);

    loadFont(vdp, tiles, art);
    art.man[0] = gs::uploadMipped(vdp, manBmp(0));
    art.man[1] = gs::uploadMipped(vdp, manBmp(1));
    art.crucible = gs::uploadMipped(vdp, crucibleBmp());
    art.flame[0] = gs::uploadMipped(vdp, flameBmp(0));
    art.flame[1] = gs::uploadMipped(vdp, flameBmp(1));
    art.slag = gs::uploadMipped(vdp, slagBmp());
    art.quench = gs::uploadMipped(vdp, quenchBmp());
    art.coal = gs::uploadMipped(vdp, coalBmp());
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.moon = gs::uploadMipped(vdp, moonBmp());
}

}  // namespace foundrydawn
