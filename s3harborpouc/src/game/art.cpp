#include "game/art.h"

namespace harborpouc {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? c[i] : 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 0, 1};
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

gs::Bitmap sailor(bool duck) {
    gs::Bitmap b(24, duck ? 22 : 42);
    int top = duck ? 0 : 2;
    b.ellipse(12, float(top + 6), 6, 6, 2);
    b.rect(7, float(top + 1), 10, 4, 6);
    b.rect(6, float(top + 3), 12, 2, 7);
    b.rect(8, float(top + 12), 8, duck ? 8 : 14, 3);
    b.rect(9, float(top + 12), 6, 3, 8);
    if (!duck) {
        b.rect(6, 26, 5, 12, 4);
        b.rect(13, 26, 5, 12, 4);
        b.rect(6, 36, 5, 4, 5);
        b.rect(13, 36, 5, 4, 5);
        b.rect(4, 16, 4, 8, 3);
        b.rect(16, 16, 4, 8, 3);
    } else {
        b.rect(4, 14, 6, 6, 4);
        b.rect(14, 14, 6, 6, 4);
        b.rect(4, 18, 6, 3, 5);
        b.rect(14, 18, 6, 3, 5);
    }
    b.rect(10, float(top + 7), 2, 2, 1);
    b.rect(14, float(top + 7), 2, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap pouchArt() {
    gs::Bitmap b(20, 16);
    b.poly({{3, 6}, {10, 2}, {17, 6}, {16, 14}, {4, 14}}, 2);
    b.rect(6, 7, 8, 6, 3);
    b.line(4, 6, 16, 6, 4, 1.4f);
    b.ellipse(10, 5, 3, 2, 5);
    b.rect(9, 1, 2, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(44, 14);
    b.rect(0, 2, 44, 10, 2);
    for (int x = 0; x < 44; x += 11) b.rect(float(x), 2, 10, 10, (x / 11) & 1 ? 3 : 2);
    b.rect(0, 2, 44, 2, 4);
    for (int x = 4; x < 42; x += 11) b.ellipse(float(x), 8, 1.1f, 1.1f, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap pontoonArt() {
    gs::Bitmap b(72, 18);
    b.poly({{4, 6}, {68, 6}, {70, 12}, {64, 16}, {8, 16}, {2, 12}}, 2);
    b.rect(8, 8, 56, 5, 3);
    for (int x = 12; x < 62; x += 10) b.rect(float(x), 4, 2, 6, 4);
    b.ellipse(14, 12, 2, 1.4f, 5);
    b.ellipse(58, 12, 2, 1.4f, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap bollardArt() {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 6, 6, 4, 2);
    b.rect(5, 8, 6, 16, 3);
    b.rect(3, 22, 10, 4, 4);
    b.rect(6, 12, 4, 2, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(52, 56);
    b.poly({{2, 18}, {26, 2}, {50, 18}}, 2);
    b.rect(6, 18, 40, 34, 3);
    b.rect(10, 24, 12, 14, 4);
    b.rect(30, 28, 10, 24, 5);
    b.rect(22, 6, 6, 12, 6);
    b.ellipse(25, 8, 2, 2, 7);
    b.rect(4, 50, 44, 4, 8);
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 80);
    b.rect(3, 0, 8, 80, 2);
    b.rect(5, 0, 3, 80, 3);
    for (int y = 8; y < 76; y += 16) b.rect(2, float(y), 10, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(88, 12);
    b.rect(0, 2, 88, 8, 2);
    b.rect(0, 2, 88, 3, 3);
    for (int x = 6; x < 84; x += 12) b.rect(float(x), 4, 3, 5, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(18, 20);
    b.rect(7, 0, 4, 8, 2);
    b.poly({{4, 8}, {14, 8}, {15, 14}, {10, 18}, {5, 14}}, 3);
    b.rect(8, 10, 3, 5, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap cableArt() {
    gs::Bitmap b(4, 32);
    b.rect(1, 0, 2, 32, 1);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(16, 22);
    b.ellipse(8, 8, 6, 6, 2);
    b.rect(6, 8, 4, 6, 3);
    b.rect(7, 14, 2, 6, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(20, 10);
    b.line(0, 6, 8, 2, 1, 1.4f);
    b.line(8, 2, 18, 7, 1, 1.4f);
    b.line(8, 3, 12, 6, 2, 1.2f);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap waveArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(8, 5, 6, 2.5f, 1);
    b.ellipse(18, 4, 5, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 10, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {gs::rgb4(0, 0, 0), gs::rgb4(14, 13, 9), gs::rgb4(8, 10, 12), gs::rgb4(4, 6, 8),
                            gs::rgb4(15, 15, 15), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)};
    const uint16_t player[] = {0,
                               gs::rgb4(2, 1, 1),
                               gs::rgb4(13, 9, 6),
                               gs::rgb4(2, 4, 9),
                               gs::rgb4(1, 2, 5),
                               gs::rgb4(3, 2, 2),
                               gs::rgb4(15, 15, 14),
                               gs::rgb4(1, 2, 6),
                               gs::rgb4(12, 12, 10)};
    const uint16_t pouch[] = {0,
                              gs::rgb4(2, 1, 1),
                              gs::rgb4(10, 6, 2),
                              gs::rgb4(7, 4, 1),
                              gs::rgb4(14, 11, 5),
                              gs::rgb4(12, 9, 4),
                              gs::rgb4(4, 3, 2)};
    const uint16_t wood[] = {0,
                             gs::rgb4(2, 1, 1),
                             gs::rgb4(11, 8, 4),
                             gs::rgb4(8, 6, 3),
                             gs::rgb4(13, 11, 7),
                             gs::rgb4(4, 3, 2)};
    const uint16_t steel[] = {0,
                              gs::rgb4(1, 1, 1),
                              gs::rgb4(11, 3, 2),
                              gs::rgb4(7, 2, 2),
                              gs::rgb4(5, 5, 6),
                              gs::rgb4(14, 12, 4)};
    const uint16_t water[] = {0, gs::rgb4(6, 10, 12), gs::rgb4(10, 13, 14), gs::rgb4(3, 6, 9), gs::rgb4(2, 4, 6)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), gs::rgb4(15, 12, 4)};
    const uint16_t go[] = {0, gs::rgb4(6, 14, 7), gs::rgb4(2, 6, 3), gs::rgb4(12, 15, 10)};
    const uint16_t house[] = {0,
                              gs::rgb4(2, 1, 1),
                              gs::rgb4(12, 4, 3),
                              gs::rgb4(13, 12, 10),
                              gs::rgb4(6, 9, 12),
                              gs::rgb4(4, 3, 2),
                              gs::rgb4(8, 8, 8),
                              gs::rgb4(15, 14, 6),
                              gs::rgb4(5, 4, 3)};
    const uint16_t foam[] = {0, gs::rgb4(3, 3, 4), gs::rgb4(14, 14, 13)};
    setPal(vdp, PAL_HUD, hud, 16);
    setPal(vdp, PAL_PLAYER, player, 9);
    setPal(vdp, PAL_POUCH, pouch, 7);
    setPal(vdp, PAL_WOOD, wood, 6);
    setPal(vdp, PAL_STEEL, steel, 6);
    setPal(vdp, PAL_WATER, water, 5);
    setPal(vdp, PAL_ALERT, alert, 4);
    setPal(vdp, PAL_GO, go, 4);
    setPal(vdp, PAL_HOUSE, house, 9);
    setPal(vdp, PAL_FOAM, foam, 3);
    vdp.setFogColor(gs::rgb4(4, 7, 10));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.stand = gs::uploadMipped(vdp, sailor(false));
    art.duck = gs::uploadMipped(vdp, sailor(true));
    art.pouch = gs::uploadMipped(vdp, pouchArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.pontoon = gs::uploadMipped(vdp, pontoonArt());
    art.bollard = gs::uploadMipped(vdp, bollardArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.wave = gs::uploadMipped(vdp, waveArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace harborpouc
