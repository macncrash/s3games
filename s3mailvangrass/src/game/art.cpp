#include "game/art.h"

#include <initializer_list>

namespace vangrass {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

gs::Bitmap paintVan() {
    gs::Bitmap b(96, 56);
    b.rect(8, 18, 78, 26, 1);
    b.rect(48, 8, 28, 16, 1);
    b.poly({{46.f, 18.f}, {52.f, 8.f}, {76.f, 8.f}, {78.f, 18.f}}, 1);
    b.rect(52, 10, 10, 7, 3);
    b.rect(64, 10, 10, 7, 3);
    b.rect(14, 22, 22, 12, 3);
    b.rect(10, 40, 74, 5, 4);
    b.rect(18, 24, 6, 8, 6);
    b.rect(26, 24, 6, 8, 6);
    b.line(8, 30, 86, 30, 5, 2.2f);
    b.rect(70, 22, 10, 8, 9);
    b.ellipse(22, 44, 8, 8, 8);
    b.ellipse(22, 44, 3.2f, 3.2f, 7);
    b.ellipse(70, 44, 8, 8, 8);
    b.ellipse(70, 44, 3.2f, 3.2f, 7);
    b.rect(4, 28, 6, 4, 2);
    b.rect(84, 26, 4, 3, 10);
    b.outline(11, false);
    return b;
}

gs::Bitmap paintWheel() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 3, 3, 2);
    b.line(9, 2, 9, 16, 3, 1.2f);
    b.line(2, 9, 16, 9, 3, 1.2f);
    return b;
}

gs::Bitmap paintSod() {
    gs::Bitmap b(28, 16);
    b.rect(0, 4, 28, 12, 1);
    for (int i = 0; i < 10; i++) {
        float x = float((i * 5 + 2) % 24);
        b.line(x, 12, x + 1.f, 3, (i & 1) ? 2 : 3, 1.2f);
    }
    b.line(0, 8, 28, 8, 4, 1.f);
    return b;
}

gs::Bitmap paintTuft() {
    gs::Bitmap b(12, 16);
    b.line(6, 15, 3, 3, 1, 1.6f);
    b.line(6, 15, 6, 2, 2, 1.6f);
    b.line(6, 15, 9, 4, 3, 1.6f);
    return b;
}

gs::Bitmap paintRoad() {
    gs::Bitmap b(32, 20);
    b.rect(0, 0, 32, 20, 1);
    b.line(0, 2, 32, 2, 2, 1.2f);
    b.rect(12, 8, 8, 3, 3);
    return b;
}

gs::Bitmap paintDash() {
    gs::Bitmap b(16, 8);
    b.rect(0, 2, 10, 3, 1);
    return b;
}

gs::Bitmap paintHouse() {
    gs::Bitmap b(48, 40);
    b.rect(6, 16, 36, 22, 1);
    b.poly({{4.f, 16.f}, {24.f, 4.f}, {44.f, 16.f}}, 2);
    b.rect(12, 22, 8, 10, 3);
    b.rect(26, 24, 10, 14, 4);
    b.rect(20, 8, 3, 6, 5);
    return b;
}

gs::Bitmap paintBox() {
    gs::Bitmap b(16, 22);
    b.rect(3, 6, 10, 14, 1);
    b.rect(5, 8, 6, 4, 2);
    b.rect(6, 2, 4, 5, 3);
    b.line(2, 20, 14, 20, 4, 1.4f);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(12, 36);
    b.rect(5, 8, 2, 26, 1);
    b.ellipse(6, 6, 4, 3, 2);
    return b;
}

gs::Bitmap paintEnd() {
    gs::Bitmap b(10, 28);
    for (int y = 0; y < 28; y += 6) b.rect(2, y, 6, 3, (y / 6) & 1 ? 1 : 2);
    return b;
}

gs::Bitmap paintDrop() {
    gs::Bitmap b(24, 18);
    b.rect(0, 0, 24, 18, 1);
    b.line(0, 2, 24, 6, 2, 1.4f);
    b.line(4, 8, 20, 14, 2, 1.2f);
    return b;
}

gs::Bitmap paintSmoke() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 2);
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
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(13, 13, 12), gs::rgb4(6, 10, 14), gs::rgb4(3, 3, 4), gs::rgb4(12, 2, 2),
            gs::rgb4(15, 15, 8), gs::rgb4(8, 8, 8), gs::rgb4(2, 2, 2), gs::rgb4(9, 7, 2), gs::rgb4(14, 14, 6), ink});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(3, 9, 2), gs::rgb4(6, 12, 3), gs::rgb4(2, 6, 1), gs::rgb4(4, 8, 2), ink});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(14, 12, 3), ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(12, 3, 3), gs::rgb4(15, 15, 12), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 2), ink});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(11, 8, 5), gs::rgb4(10, 3, 2), gs::rgb4(8, 12, 14), gs::rgb4(5, 3, 2), gs::rgb4(6, 6, 6), ink});
    setPal(vdp, PAL_END, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 2, 2), ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(10, 10, 9), ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(6, 9, 13), ink});

    uint8_t sky[64];
    for (int i = 0; i < 64; i++) sky[i] = 1;
    art.skyTile = 1;
    vdp.loadTile(1, sky);

    art.van = gs::uploadMipped(vdp, paintVan());
    art.wheel = gs::uploadMipped(vdp, paintWheel());
    art.sod = gs::uploadMipped(vdp, paintSod());
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.road = gs::uploadMipped(vdp, paintRoad());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.house = gs::uploadMipped(vdp, paintHouse());
    art.box = gs::uploadMipped(vdp, paintBox());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.endpost = gs::uploadMipped(vdp, paintEnd());
    art.drop = gs::uploadMipped(vdp, paintDrop());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    loadFont(vdp, art);
}

}  // namespace vangrass
