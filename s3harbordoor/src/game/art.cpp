#include "game/art.h"

#include <cstdint>

namespace harbordoor {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? c[i] : 0);
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
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap doorArt() {
    gs::Bitmap b(56, 200);
    b.rect(4, 0, 48, 200, 2);
    for (int y = 0; y < 200; y += 20) {
        b.rect(6, float(y), 44, 18, (y / 20) & 1 ? 3 : 4);
        b.rect(8, float(y + 2), 40, 2, 5);
        for (int x = 12; x < 48; x += 12) b.ellipse(float(x), float(y + 10), 1.6f, 1.6f, 6);
    }
    b.rect(0, 0, 6, 200, 1);
    b.rect(50, 0, 6, 200, 7);
    for (int y = 16; y < 190; y += 36) {
        b.rect(0, float(y), 10, 8, 8);
        b.ellipse(4, float(y + 4), 2.2f, 2.2f, 9);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap gunArt() {
    gs::Bitmap b(48, 28);
    b.rect(2, 8, 16, 14, 2);
    b.rect(4, 10, 12, 10, 3);
    b.rect(16, 11, 28, 6, 4);
    b.rect(16, 12, 28, 3, 5);
    b.ellipse(10, 8, 6, 5, 6);
    b.rect(0, 20, 18, 6, 1);
    b.ellipse(42, 14, 3, 3, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap skiffArt() {
    gs::Bitmap b(52, 26);
    b.poly({{4, 16}, {14, 8}, {46, 8}, {50, 14}, {46, 22}, {10, 22}}, 2);
    b.poly({{16, 10}, {40, 10}, {38, 18}, {18, 18}}, 3);
    b.rect(22, 4, 10, 8, 4);
    b.rect(24, 6, 6, 4, 5);
    b.ellipse(12, 14, 2, 2, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap ramArt() {
    gs::Bitmap b(64, 32);
    b.poly({{2, 18}, {16, 8}, {58, 8}, {62, 16}, {54, 26}, {12, 26}}, 2);
    b.rect(20, 10, 28, 10, 3);
    b.poly({{2, 16}, {16, 12}, {16, 22}, {6, 22}}, 4);
    b.rect(30, 2, 8, 10, 5);
    b.ellipse(48, 16, 3, 3, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(12, 6);
    b.rect(0, 1, 10, 4, 1);
    b.rect(6, 2, 6, 2, 2);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(20, 16);
    b.ellipse(10, 10, 8, 4, 1);
    b.ellipse(6, 6, 3, 4, 2);
    b.ellipse(14, 5, 2.5f, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)};
    const uint16_t iron[] = {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 6, 7), gs::rgb4(7, 8, 9), gs::rgb4(9, 10, 11),
                             gs::rgb4(4, 5, 6), gs::rgb4(12, 12, 10), gs::rgb4(3, 3, 4), gs::rgb4(8, 7, 4), gs::rgb4(3, 3, 2)};
    const uint16_t gun[] = {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 5, 3), gs::rgb4(9, 7, 4), gs::rgb4(4, 5, 6),
                            gs::rgb4(10, 11, 12), gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 6)};
    const uint16_t skiff[] = {0, gs::rgb4(1, 2, 3), gs::rgb4(4, 6, 5), gs::rgb4(8, 10, 7), gs::rgb4(10, 8, 4),
                              gs::rgb4(14, 13, 8), gs::rgb4(15, 12, 4)};
    const uint16_t ram[] = {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 2, 2), gs::rgb4(9, 4, 3), gs::rgb4(12, 6, 3),
                            gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 6)};
    const uint16_t shot[] = {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)};
    const uint16_t foam[] = {0, gs::rgb4(10, 13, 14), gs::rgb4(14, 15, 15), gs::rgb4(7, 11, 13)};
    const uint16_t gold[] = {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)};
    const uint16_t water[] = {gs::rgb4(1, 3, 6), gs::rgb4(2, 5, 8), gs::rgb4(3, 7, 10), gs::rgb4(5, 9, 12),
                              gs::rgb4(8, 12, 13), gs::rgb4(2, 4, 6), gs::rgb4(1, 2, 4), gs::rgb4(4, 6, 8),
                              gs::rgb4(6, 8, 9), gs::rgb4(9, 11, 10), gs::rgb4(3, 5, 7), gs::rgb4(7, 10, 11),
                              gs::rgb4(12, 14, 14), gs::rgb4(1, 4, 7), gs::rgb4(4, 8, 11), gs::rgb4(10, 13, 14)};
    setPal(vdp, PAL_HUD, hud, 16);
    setPal(vdp, PAL_IRON, iron, 10);
    setPal(vdp, PAL_GUN, gun, 8);
    setPal(vdp, PAL_SKIFF, skiff, 7);
    setPal(vdp, PAL_RAM, ram, 7);
    setPal(vdp, PAL_SHOT, shot, 3);
    setPal(vdp, PAL_FOAM, foam, 4);
    setPal(vdp, PAL_GOLD, gold, 16);
    setPal(vdp, PAL_ALERT, alert, 16);
    setPal(vdp, PAL_WATER, water, 16);
    vdp.setFogColor(gs::rgb4(2, 4, 7));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.door = gs::uploadMipped(vdp, doorArt());
    art.gun = gs::uploadMipped(vdp, gunArt());
    art.skiff = gs::uploadMipped(vdp, skiffArt());
    art.ram = gs::uploadMipped(vdp, ramArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
}

}  // namespace harbordoor
