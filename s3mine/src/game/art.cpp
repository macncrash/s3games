#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>

namespace mine {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void ceiling(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int v = 0; v < 4; v++) {
        uint8_t px[64];
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                uint32_t h = uint32_t(x * 17 + y * 53 + v * 131 + x * y * 3);
                int c = 2;
                if (h % 5 == 0) c = 1;
                if (h % 9 == 0) c = 3;
                if (y == 7 && (v & 1)) c = 3;
                px[y * 8 + x] = uint8_t(c);
            }
        a.rock[v] = tiles.alloc(1);
        vdp.loadTile(a.rock[v], px);
    }
    uint8_t beam[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = y < 2 ? 1 : y > 5 ? 5 : 4;
            if (x == 3 || x == 4) c = 2;
            beam[y * 8 + x] = uint8_t(c);
        }
    a.beam = tiles.alloc(1);
    vdp.loadTile(a.beam, beam);
    for (int cy = 0; cy < 32; cy++)
        for (int cx = 0; cx < 64; cx++) {
            bool timber = (cy % 5 == 0);
            int id = timber ? a.beam : a.rock[(cx + cy * 3) & 3];
            vdp.A.set(cx, cy, gs::entry(id, PAL_CEIL));
        }
}

gs::Bitmap cartArt(int frame) {
    gs::Bitmap b(76, 72);
    b.rect(8, 50, 60, 5, 3);
    b.rect(10, 51, 56, 2, 12);
    b.ellipse(18, 56, 13, 13, 4);
    b.ellipse(58, 56, 13, 13, 4);
    b.ellipse(18, 56, 8, 8, 3);
    b.ellipse(58, 56, 8, 8, 3);
    b.ellipse(18, 56, 3, 3, 11);
    b.ellipse(58, 56, 3, 3, 11);
    float a0 = frame ? 0.55f : 0.1f;
    for (int i = 0; i < 4; i++) {
        float a = a0 + i * 1.5708f;
        b.line(18 + std::cos(a) * 3, 56 + std::sin(a) * 3, 18 + std::cos(a) * 11, 56 + std::sin(a) * 11, 11, 1.4f);
        b.line(58 + std::cos(a) * 3, 56 + std::sin(a) * 3, 58 + std::cos(a) * 11, 56 + std::sin(a) * 11, 11, 1.4f);
    }
    b.poly({{14, 28}, {62, 28}, {66, 50}, {10, 50}}, 2);
    b.poly({{22, 30}, {54, 30}, {56, 46}, {20, 46}}, 1);
    b.rect(14, 36, 52, 3, 3);
    b.rect(16, 44, 44, 3, 12);
    for (int i = 0; i < 4; i++) b.rect(22 + i * 10, 37, 2, 2, 4);
    b.ellipse(38, 16, 9, 9, 9);
    b.ellipse(38, 18, 7, 6, 6);
    b.rect(30, 22, 16, 14, 5);
    b.rect(33, 24, 10, 8, 2);
    b.rect(48, 18, 5, 16, 4);
    b.rect(50, 14, 3, 6, 3);
    b.ellipse(14, 34, 5, 5, 13);
    b.ellipse(14, 34, 2, 2, 14);
    b.ellipse(62, 40, 3, 3, 7);
    b.ellipse(62, 40, 1, 1.0f, 8);
    b.outline(10, false);
    return b;
}

gs::Bitmap propArt() {
    gs::Bitmap b(40, 88);
    b.rect(5, 18, 8, 62, 2);
    b.rect(6, 18, 3, 62, 1);
    b.rect(27, 18, 8, 62, 2);
    b.rect(28, 18, 3, 62, 1);
    b.rect(3, 10, 34, 10, 2);
    b.rect(3, 10, 34, 3, 1);
    b.line(12, 72, 28, 22, 3, 2.2f);
    b.rect(3, 74, 12, 6, 4);
    b.rect(25, 74, 12, 6, 4);
    b.rect(6, 40, 6, 8, 5);
    b.rect(18, 20, 4, 7, 4);
    b.ellipse(20, 32, 5, 6, 6);
    b.ellipse(19, 31, 2, 2.0f, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap oreArt() {
    gs::Bitmap b(48, 58);
    b.poly({{6, 28}, {42, 28}, {46, 50}, {2, 50}}, 1);
    b.rect(8, 30, 32, 8, 2);
    b.rect(6, 48, 36, 4, 4);
    b.poly({{14, 28}, {20, 8}, {26, 28}}, 5);
    b.poly({{16, 28}, {20, 12}, {23, 28}}, 7);
    b.poly({{24, 28}, {32, 4}, {40, 28}}, 3);
    b.poly({{28, 28}, {32, 10}, {36, 28}}, 6);
    b.poly({{10, 30}, {14, 16}, {18, 30}}, 5);
    b.rect(20, 36, 8, 6, 4);
    b.outline(8, false);
    return b;
}

gs::Bitmap ribArt() {
    gs::Bitmap b(18, 80);
    b.rect(4, 6, 10, 68, 2);
    b.rect(5, 6, 4, 68, 1);
    b.rect(2, 4, 14, 6, 2);
    b.rect(2, 68, 14, 6, 3);
    b.rect(7, 20, 4, 3, 4);
    b.rect(7, 40, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(36, 12);
    b.rect(1, 2, 34, 8, 2);
    b.rect(1, 2, 34, 3, 1);
    b.rect(16, 4, 4, 5, 4);
    b.outline(3, false);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(16, 16);
    b.poly({{8, 1}, {15, 8}, {8, 15}, {1, 8}}, 2);
    b.poly({{8, 4}, {12, 8}, {8, 12}, {4, 8}}, 1);
    b.ellipse(8, 8, 2, 2.0f, 3);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3.0f, 1);
    return b;
}

gs::Bitmap reticleArt() {
    gs::Bitmap b(16, 16);
    b.rect(7, 1, 2, 4, 1);
    b.rect(7, 11, 2, 4, 1);
    b.rect(1, 7, 4, 2, 1);
    b.rect(11, 7, 4, 2, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 4, 1);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 4, 1);
    b.ellipse(5, 4, 3, 2, 2);
    return b;
}

gs::Bitmap exitArt() {
    gs::Bitmap b(40, 56);
    b.ellipse(20, 30, 18, 24, 1);
    b.ellipse(20, 32, 14, 18, 2);
    b.ellipse(20, 34, 9, 12, 3);
    b.ellipse(20, 36, 6, 8, 0);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3)});
    vdp.setColor(PAL_HUD * 16 + 15, shadow);
    vdp.setColor(PAL_AMBER * 16 + 15, gs::rgb4(3, 2, 0));
    vdp.setColor(PAL_GOOD * 16 + 15, gs::rgb4(0, 2, 1));
    vdp.setColor(PAL_BAD * 16 + 15, gs::rgb4(2, 0, 0));

    setPal(vdp, PAL_WOOD, {0, gs::rgb4(13, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 1), gs::rgb4(6, 6, 7),
                           gs::rgb4(13, 2, 1), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 12), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_ORE, {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 14, 7), gs::rgb4(5, 5, 6),
                          gs::rgb4(15, 12, 3), gs::rgb4(12, 15, 11), gs::rgb4(15, 15, 13), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CART,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(7, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(2, 4, 9),
            gs::rgb4(12, 8, 6), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), gs::rgb4(13, 11, 4), gs::rgb4(1, 1, 1),
            gs::rgb4(8, 8, 9), gs::rgb4(5, 4, 3), gs::rgb4(15, 3, 2), gs::rgb4(8, 1, 1)});
    setPal(vdp, PAL_BOLT, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_RIB, {0, gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(5, 4, 3),
                          gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_CEIL, {0, gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(8, 6, 3),
                           gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_EXIT, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 14, 15), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(7, 6, 5), gs::rgb4(10, 9, 7)});

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6),
        gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2),
        gs::rgb4(7, 5, 3), gs::rgb4(10, 8, 5),
        gs::rgb4(8, 7, 6), gs::rgb4(11, 11, 12), gs::rgb4(8, 6, 3),
        gs::rgb4(1, 2, 3), gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5),
        gs::rgb4(12, 10, 5), gs::rgb4(9, 8, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    ceiling(vdp, tiles, art);
    art.cart[0] = gs::uploadMipped(vdp, cartArt(0));
    art.cart[1] = gs::uploadMipped(vdp, cartArt(1));
    art.prop = gs::uploadMipped(vdp, propArt());
    art.ore = gs::uploadMipped(vdp, oreArt());
    art.rib = gs::uploadMipped(vdp, ribArt());
    art.cap = gs::uploadMipped(vdp, capArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.reticle = gs::uploadMipped(vdp, reticleArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.exit = gs::uploadMipped(vdp, exitArt());
    vdp.B.enabled = false;
}

}  // namespace mine
