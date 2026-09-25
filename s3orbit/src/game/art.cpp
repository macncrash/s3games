#include "game/art.h"

#include "game/world.h"

#include <cstdint>
#include <string>

namespace orbit {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void armPal(gs::VDP& v, int pal, uint16_t seat, uint16_t lamp) {
    v.setColor(pal * 16 + 1, gs::rgb4(13, 14, 15));
    v.setColor(pal * 16 + 2, gs::rgb4(7, 8, 10));
    v.setColor(pal * 16 + 3, gs::rgb4(3, 4, 6));
    v.setColor(pal * 16 + 4, seat);
    v.setColor(pal * 16 + 5, gs::rgb4(15, 12, 3));
    v.setColor(pal * 16 + 6, gs::rgb4(1, 1, 2));
    v.setColor(pal * 16 + 7, lamp);
}

void paintPals(gs::VDP& v) {
    textPal(v, PAL_TEXT, gs::rgb4(15, 15, 15));
    textPal(v, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(v, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(v, PAL_GREEN, gs::rgb4(6, 15, 8));

    const uint16_t scene[16] = {
        0,
        gs::rgb4(5, 3, 8),    // nebula
        gs::rgb4(8, 5, 12),   // nebula bright
        gs::rgb4(2, 5, 11),   // ocean
        gs::rgb4(2, 8, 4),    // land
        gs::rgb4(5, 11, 5),   // land light
        gs::rgb4(13, 14, 15), // cloud
        gs::rgb4(6, 11, 15),  // atmosphere
        gs::rgb4(3, 4, 6),    // hull dark
        gs::rgb4(7, 8, 10),   // hull
        gs::rgb4(12, 13, 15), // hull light
        gs::rgb4(10, 15, 15), // window
        gs::rgb4(2, 3, 8),    // solar
        gs::rgb4(5, 8, 14),   // solar grid
        gs::rgb4(12, 12, 14), // dim star
        gs::rgb4(15, 15, 15), // star
    };
    for (int i = 0; i < 16; i++) v.setColor(PAL_SCENE * 16 + i, scene[i]);

    v.setColor(PAL_SHIP * 16 + 1, gs::rgb4(14, 15, 15));
    v.setColor(PAL_SHIP * 16 + 2, gs::rgb4(9, 10, 12));
    v.setColor(PAL_SHIP * 16 + 3, gs::rgb4(4, 5, 7));
    v.setColor(PAL_SHIP * 16 + 4, gs::rgb4(8, 14, 15));
    v.setColor(PAL_SHIP * 16 + 5, gs::rgb4(15, 8, 2));
    v.setColor(PAL_SHIP * 16 + 6, gs::rgb4(15, 15, 12));
    v.setColor(PAL_SHIP * 16 + 7, gs::rgb4(2, 2, 3));
    v.setColor(PAL_SHIP * 16 + 8, gs::rgb4(15, 13, 5));

    armPal(v, PAL_ARM, gs::rgb4(15, 11, 2), gs::rgb4(15, 14, 6));
    armPal(v, PAL_HOT, gs::rgb4(15, 3, 2), gs::rgb4(15, 5, 3));
    armPal(v, PAL_OK, gs::rgb4(4, 15, 6), gs::rgb4(12, 15, 13));

    v.setColor(PAL_FIRE * 16 + 1, gs::rgb4(15, 15, 11));
    v.setColor(PAL_FIRE * 16 + 2, gs::rgb4(15, 9, 2));
    v.setColor(PAL_FIRE * 16 + 3, gs::rgb4(12, 3, 1));
    v.setColor(PAL_FIRE * 16 + 4, gs::rgb4(5, 2, 1));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
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
        art.font[c - 32] = t;
    }
}

gs::Bitmap shipArt() {
    gs::Bitmap b(44, 14);
    b.rect(4, 3, 26, 8, 2);
    b.rect(4, 3, 26, 2, 1);
    b.rect(6, 8, 22, 3, 3);
    b.poly({{30, 3}, {41, 6}, {41, 8}, {30, 11}}, 2);
    b.poly({{30, 3}, {38, 6}, {30, 6}}, 1);
    b.rect(41, 6, 3, 2, 6);
    b.rect(22, 4, 6, 4, 4);
    b.rect(23, 5, 2, 2, 1);
    b.rect(8, 6, 14, 2, 5);
    b.rect(6, 1, 4, 2, 8);
    b.rect(6, 11, 4, 2, 8);
    b.rect(16, 1, 3, 2, 8);
    b.rect(16, 11, 3, 2, 8);
    b.rect(1, 5, 3, 4, 7);
    return b;
}

gs::Bitmap plumeArt() {
    gs::Bitmap b(12, 8);
    b.poly({{11, 1}, {11, 6}, {1, 4}}, 2);
    b.poly({{10, 2}, {10, 5}, {4, 4}}, 1);
    b.set(7, 3, 3);
    b.set(7, 4, 3);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 2);
    b.ellipse(4, 4, 1.4f, 1.4f, 1);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.2f, 2.2f, 2);
    b.set(2, 2, 1);
    b.set(3, 2, 1);
    return b;
}

gs::Bitmap jointArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 2);
    b.ellipse(6, 6, 3, 3, 5);
    b.ellipse(6, 6, 1.2f, 1.2f, 1);
    return b;
}

// Clamp opens to the left. Sprite centre is the capture seat.
gs::Bitmap collarArt() {
    gs::Bitmap b(32, 42);
    b.rect(8, 0, 24, 5, 2);
    b.rect(8, 0, 24, 1, 1);
    b.rect(8, 4, 24, 1, 3);
    b.rect(8, 37, 24, 5, 2);
    b.rect(8, 41, 24, 1, 1);
    b.rect(8, 37, 24, 1, 3);
    for (int x = 8; x < 30; x += 4) {
        b.rect(float(x), 1, 2, 3, 5);
        b.rect(float(x), 38, 2, 3, 5);
    }
    b.rect(20, 0, 12, 42, 2);
    b.rect(20, 0, 2, 42, 3);
    b.rect(14, 19, 7, 5, 4);
    b.rect(23, 20, 2, 2, 7);
    return b;
}

gs::Bitmap chevronArt() {
    gs::Bitmap b(12, 10);
    b.poly({{1, 1}, {1, 8}, {11, 5}}, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 2, 2, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 7);
    b.ellipse(4, 4, 1.2f, 1.2f, 1);
    return b;
}

void paintScene(gs::Bitmap& scene) {
    scene.ellipse(150, 36, 78, 30, 1);
    scene.ellipse(176, 28, 36, 18, 2);
    scene.ellipse(48, 70, 40, 16, 1);

    uint32_t n = 0x0B17u;
    for (int i = 0; i < 180; i++) {
        n = n * 1664525u + 1013904223u;
        int x = int(n % 230u);
        n = n * 1664525u + 1013904223u;
        int y = int((n >> 8) % 224u);
        scene.set(x, y, (i % 5 == 0) ? 15 : 14);
    }

    scene.ellipse(-40, 310, 200, 150, 7);
    scene.ellipse(-34, 318, 180, 132, 3);
    scene.ellipse(-10, 300, 54, 28, 4);
    scene.ellipse(18, 268, 28, 16, 5);
    scene.ellipse(-48, 340, 46, 18, 5);
    scene.ellipse(6, 250, 34, 12, 6);
    scene.ellipse(-28, 292, 40, 14, 6);
    scene.ellipse(24, 330, 26, 10, 6);

    scene.rect(302, 26, 16, 172, 12);
    for (int y = 26; y < 198; y += 8) scene.rect(302, float(y), 16, 2, 13);
    scene.rect(309, 26, 2, 172, 13);
    scene.rect(286, 110, 18, 4, 10);

    scene.ellipse(264, kShoulderY, 24, 56, 8);
    scene.ellipse(264, kShoulderY, 20, 50, 9);
    scene.ellipse(270, 100, 8, 26, 10);
    scene.rect(kStationL, 96, 24, 36, 8);
    scene.rect(kStationL + 2, 100, 16, 28, 9);
    scene.rect(kStationL, 110, 8, 8, 11);
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 2; j++) scene.rect(252 + j * 8, 76 + i * 12, 3, 2, 11);
    scene.rect(250, 156, 28, 16, 8);
    scene.rect(252, 158, 22, 5, 10);
    scene.line(272, 58, 272, 30, 10, 1);
    scene.rect(268, 28, 8, 3, 10);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    gs::Bitmap scene(gs::SCREEN_W, gs::SCREEN_H);
    paintScene(scene);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, scene, PAL_SCENE);
    art.ship = gs::uploadMipped(vdp, shipArt());
    art.plume = gs::uploadMipped(vdp, plumeArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.joint = gs::uploadMipped(vdp, jointArt());
    art.collar = gs::uploadMipped(vdp, collarArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace orbit
