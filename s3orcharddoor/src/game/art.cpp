#include "game/art.h"

namespace orcharddoor {
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
        if (g) {
            for (int y = 0; y < 7; y++)
                for (int x = 0; x < 5; x++)
                    if (g[y * 5 + x]) {
                        px[y * 8 + x + 1] = 1;
                        if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                    }
        }
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap treeBmp() {
    gs::Bitmap b(36, 64);
    b.rect(15, 34, 6, 30, 4);
    b.rect(16, 34, 2, 28, 5);
    b.ellipse(18, 26, 14, 16, 2);
    b.ellipse(12, 20, 7, 6, 3);
    b.ellipse(24, 18, 6, 5, 1);
    b.ellipse(18, 14, 5, 4, 6);
    b.set(10, 22, 7);
    b.set(26, 24, 7);
    return b;
}

gs::Bitmap doorBmp() {
    gs::Bitmap b(40, 72);
    b.rect(2, 2, 36, 68, 2);
    b.rect(4, 4, 32, 64, 3);
    for (int i = 0; i < 4; i++) b.rect(6, 6 + i * 16, 28, 2, 4);
    b.rect(6, 8, 2, 58, 4);
    b.rect(32, 8, 2, 58, 4);
    b.rect(18, 8, 2, 58, 5);
    b.ellipse(28, 38, 2, 2, 6);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(10, 80);
    b.rect(1, 0, 8, 80, 2);
    b.rect(2, 0, 3, 80, 3);
    b.rect(1, 8, 8, 3, 4);
    b.rect(1, 70, 8, 3, 4);
    return b;
}

gs::Bitmap barBmp() {
    gs::Bitmap b(48, 8);
    b.rect(0, 2, 48, 4, 2);
    b.rect(0, 3, 48, 2, 3);
    b.rect(4, 1, 4, 6, 4);
    b.rect(40, 1, 4, 6, 4);
    return b;
}

gs::Bitmap manBmp(int step) {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 6, 4, 4, 3);
    b.rect(8, 11, 8, 12, 4);
    b.rect(9, 12, 6, 4, 5);
    b.rect(5, 13, 4, 9, 4);
    b.rect(15, 12, 5, 8, 4);
    int ly = step ? 24 : 26;
    int ry = step ? 26 : 24;
    b.rect(8, ly, 3, 13, 2);
    b.rect(13, ry, 3, 13, 2);
    b.rect(7, 36, 5, 3, 6);
    b.rect(13, 36, 5, 3, 6);
    return b;
}

gs::Bitmap appleBmp() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 7, 5, 4, 2);
    b.ellipse(5, 6, 3, 2, 3);
    b.line(6, 1, 7, 4, 4, 1);
    b.set(8, 2, 1);
    return b;
}

gs::Bitmap leafBmp() {
    gs::Bitmap b(10, 8);
    b.ellipse(5, 4, 4, 2, 2);
    b.line(1, 4, 9, 4, 3, 1);
    return b;
}

gs::Bitmap handBmp() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 6, 6, 4, 2);
    b.rect(2, 4, 3, 5, 3);
    b.rect(6, 3, 3, 5, 3);
    b.rect(10, 4, 3, 5, 3);
    return b;
}

gs::Bitmap moteBmp() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1, 1.2f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {C(0, 0, 0), C(15, 14, 10), C(8, 7, 4), C(15, 15, 15), C(4, 3, 2),
                            C(12, 10, 6), C(15, 12, 4), C(2, 2, 1), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(6, 5, 3)};
    const uint16_t grass[] = {C(0, 0, 0), C(2, 6, 1), C(3, 8, 2), C(5, 11, 3), C(1, 4, 1),
                              C(8, 12, 4), C(4, 7, 2), C(10, 14, 6), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(1, 2, 1)};
    const uint16_t wood[] = {C(0, 0, 0), C(6, 3, 1), C(9, 5, 2), C(12, 7, 3), C(5, 3, 1),
                             C(14, 10, 5), C(15, 13, 6), C(3, 2, 1), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(2, 1, 0)};
    const uint16_t leaf[] = {C(0, 0, 0), C(1, 5, 1), C(2, 8, 2), C(4, 11, 3), C(6, 4, 1),
                             C(8, 13, 4), C(10, 14, 5), C(15, 14, 8), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(1, 3, 1)};
    const uint16_t man[] = {C(0, 0, 0), C(8, 5, 3), C(4, 3, 2), C(13, 9, 6), C(2, 4, 8),
                            C(5, 8, 12), C(3, 2, 1), C(15, 12, 8), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(1, 1, 2)};
    const uint16_t iron[] = {C(0, 0, 0), C(4, 4, 5), C(7, 7, 8), C(11, 11, 12), C(14, 12, 8),
                             C(3, 3, 4), C(15, 15, 14), C(2, 2, 3), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(6, 6, 7)};
    const uint16_t apple[] = {C(0, 0, 0), C(2, 6, 1), C(12, 2, 2), C(15, 5, 3), C(4, 2, 1),
                              C(15, 10, 4), C(8, 1, 1), C(15, 14, 10), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(6, 1, 1)};
    const uint16_t alert[] = {C(0, 0, 0), C(15, 4, 2), C(15, 10, 3), C(15, 14, 6), C(8, 1, 1),
                              C(15, 15, 12), C(6, 2, 1), C(2, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(10, 3, 2)};
    const uint16_t dust[] = {C(0, 0, 0), C(8, 7, 5), C(11, 10, 7), C(14, 13, 9), C(6, 5, 3),
                             C(4, 4, 3), C(15, 14, 11), C(9, 8, 6), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(5, 4, 3)};
    const uint16_t bird[] = {C(0, 0, 0), C(2, 2, 3), C(5, 5, 7), C(9, 9, 11), C(14, 12, 6),
                             C(1, 1, 2), C(15, 15, 15), C(4, 3, 2), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(3, 3, 4)};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_GRASS, grass);
    setPal(vdp, PAL_WOOD, wood);
    setPal(vdp, PAL_LEAF, leaf);
    setPal(vdp, PAL_MAN, man);
    setPal(vdp, PAL_IRON, iron);
    setPal(vdp, PAL_APPLE, apple);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_DUST, dust);
    setPal(vdp, PAL_BIRD, bird);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.tree = gs::uploadMipped(vdp, treeBmp());
    art.door = gs::uploadMipped(vdp, doorBmp());
    art.post = gs::uploadMipped(vdp, postBmp());
    art.bar = gs::uploadMipped(vdp, barBmp());
    art.man[0] = gs::uploadMipped(vdp, manBmp(0));
    art.man[1] = gs::uploadMipped(vdp, manBmp(1));
    art.apple = gs::uploadMipped(vdp, appleBmp());
    art.leaf = gs::uploadMipped(vdp, leafBmp());
    art.hand = gs::uploadMipped(vdp, handBmp());
    art.mote = gs::uploadMipped(vdp, moteBmp());
    vdp.setFogColor(C(6, 4, 3));
}

}  // namespace orcharddoor
