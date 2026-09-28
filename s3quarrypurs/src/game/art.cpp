#include "game/art.h"

#include <initializer_list>
#include <string>

namespace quarrypurs {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void fill8(uint8_t* p, int c) {
    for (int i = 0; i < 64; i++) p[i] = uint8_t(c);
}

int makeTile(gs::TileAlloc& alloc, gs::VDP& vdp, const uint8_t* px) {
    int t = alloc.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void rockTile(uint8_t* p, int salt) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int n = (x * 3 + y * 5 + salt * 11) & 15;
            int c = 1;
            if (n < 2) c = 2;
            else if (n > 12) c = 3;
            else if (((x + salt) ^ y) & 4) c = 4;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void seamTile(uint8_t* p) {
    rockTile(p, 2);
    for (int x = 0; x < 8; x++) {
        p[6 * 8 + x] = 8;
        p[7 * 8 + x] = 9;
        if ((x & 3) == 0) p[5 * 8 + x] = 10;
    }
}

void haulTile(uint8_t* p) {
    fill8(p, 5);
    for (int x = 0; x < 8; x++) {
        p[x] = 6;
        p[7 * 8 + x] = 7;
        if ((x & 1) == 0) p[3 * 8 + x] = 11;
        else p[4 * 8 + x] = 11;
    }
    p[2 * 8 + 3] = 12;
    p[5 * 8 + 6] = 12;
}

void skyLip(uint8_t* p) {
    fill8(p, 0);
    for (int y = 4; y < 8; y++)
        for (int x = 0; x < 8; x++) p[y * 8 + x] = uint8_t(y == 4 ? 3 : 1);
    p[5 * 8 + 2] = 13;
    p[6 * 8 + 6] = 2;
}

void paintFace(gs::VDP& vdp, int lip, int rockA, int rockB, int seam, int haul) {
    auto put = [&](int x, int y, int tile) { vdp.B.set(x, y, gs::entry(tile, PAL_ROCK)); };
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            if (y < 4) continue;
            if (y == 4) put(x, y, lip);
            else if (y == kBenchRow[0] || y == kBenchRow[1] || y == kBenchRow[2]) put(x, y, haul);
            else if (y == kBenchRow[0] - 1 || y == kBenchRow[1] - 1 || y == kBenchRow[2] - 1) put(x, y, seam);
            else put(x, y, ((x + y) & 1) ? rockB : rockA);
        }
    }
}

void cab(Bitmap& b, int x, int y, int w, int h) {
    b.rect(float(x), float(y), float(w), float(h), 4);
    b.rect(float(x + 1), float(y + 1), float(w - 2), float(h / 2), 8);
    b.rect(float(x), float(y + h - 2), float(w), 2, 3);
}

Bitmap loaderArt(int frame) {
    Bitmap b(48, 28);
    b.rect(6, 14, 30, 8, 2);
    b.rect(8, 12, 26, 3, 1);
    cab(b, 22, 6, 12, 9);
    b.rect(4, 16, 8, 4, 10);
    b.poly({{2, 18}, {8, 14}, {8, 20}}, 9);
    b.rect(36, 17, 8, 3, 6);
    int wy = (frame & 1) ? 20 : 21;
    b.ellipse(14, float(wy), 4.2f, 4.2f, 6);
    b.ellipse(28, float(wy), 4.2f, 4.2f, 6);
    b.ellipse(14, float(wy), 1.6f, 1.6f, 5);
    b.ellipse(28, float(wy), 1.6f, 1.6f, 5);
    b.outline(5, false);
    return b;
}

Bitmap dumperArt(int frame) {
    Bitmap b(56, 30);
    b.poly({{8, 16}, {40, 12}, {42, 20}, {8, 20}}, 2);
    b.rect(8, 18, 32, 6, 3);
    cab(b, 40, 10, 12, 10);
    b.rect(6, 16, 4, 6, 10);
    int wy = (frame & 1) ? 22 : 23;
    b.ellipse(16, float(wy), 4.5f, 4.5f, 6);
    b.ellipse(30, float(wy), 4.5f, 4.5f, 6);
    b.ellipse(46, float(wy), 4.f, 4.f, 6);
    b.outline(5, false);
    return b;
}

Bitmap hoeArt(int frame) {
    Bitmap b(52, 32);
    b.rect(16, 16, 22, 8, 2);
    cab(b, 18, 8, 14, 9);
    float boom = (frame & 1) ? 6.f : 8.f;
    b.line(30, 12, 46, boom, 1, 2.f);
    b.line(46, boom, 48, boom + 10.f, 9, 2.f);
    b.rect(44, boom + 8.f, 6, 4, 6);
    b.ellipse(22, 24, 4.2f, 4.2f, 6);
    b.ellipse(34, 24, 4.2f, 4.2f, 6);
    b.rect(10, 20, 8, 4, 7);
    b.outline(5, false);
    return b;
}

Bitmap crushArt(int frame) {
    Bitmap b(50, 30);
    b.rect(8, 8, 28, 14, 2);
    b.rect(10, 10, 24, 4, 1);
    for (int i = 0; i < 4; i++) b.rect(float(12 + i * 5), (frame & 1) ? 15.f : 16.f, 3, 4, 9);
    cab(b, 34, 10, 10, 8);
    b.rect(4, 16, 6, 5, 10);
    b.ellipse(16, 24, 4.f, 4.f, 6);
    b.ellipse(32, 24, 4.f, 4.f, 6);
    b.outline(5, false);
    return b;
}

Bitmap rockArt() {
    Bitmap b(10, 6);
    b.poly({{1, 5}, {3, 1}, {8, 2}, {9, 5}}, 2);
    b.outline(3, false);
    return b;
}

Bitmap puffArt() {
    Bitmap b(12, 10);
    b.ellipse(6, 5, 5, 4, 1);
    b.ellipse(4, 4, 2, 1.5f, 11);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(6, 6);
    b.rect(2, 0, 2, 6, 2);
    b.rect(0, 2, 6, 2, 9);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(20, 6);
    b.ellipse(10, 3, 9, 2, 1);
    return b;
}

Bitmap coneArt() {
    Bitmap b(10, 16);
    b.poly({{5, 1}, {9, 14}, {1, 14}}, 9);
    b.rect(2, 6, 6, 2, 5);
    b.rect(3, 10, 4, 2, 5);
    return b;
}

Bitmap boulderArt() {
    Bitmap b(22, 16);
    b.ellipse(11, 9, 10, 6, 2);
    b.ellipse(8, 8, 3, 2, 1);
    b.outline(5, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(8, 18);
    b.rect(3, 4, 2, 14, 5);
    b.ellipse(4, 4, 3, 3, 9);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 15, 0, 1};
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
    const uint16_t ink = gs::rgb4(15, 14, 10);
    const uint16_t shade = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 5, 4), gs::rgb4(10, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(9, 2, 1), 0, 0, gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 3), 0, 0, gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(12, 8, 2), 0, 0, gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});

    auto machine = [&](int pal, uint16_t hi, uint16_t body, uint16_t lo, uint16_t stripe) {
        setPal(vdp, pal,
               {0, hi, body, lo, gs::rgb4(6, 10, 12), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 4),
                gs::rgb4(12, 14, 15), stripe, gs::rgb4(14, 8, 2), gs::rgb4(13, 12, 10), gs::rgb4(8, 7, 6),
                gs::rgb4(15, 14, 6), gs::rgb4(9, 8, 6), shade});
    };
    machine(PAL_YOU, gs::rgb4(15, 14, 5), gs::rgb4(13, 10, 2), gs::rgb4(8, 6, 1), gs::rgb4(15, 12, 2));
    machine(PAL_DUMP, gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(12, 3, 2));
    machine(PAL_HOE, gs::rgb4(14, 10, 5), gs::rgb4(10, 6, 2), gs::rgb4(6, 3, 1), gs::rgb4(15, 12, 3));
    machine(PAL_CRUSH, gs::rgb4(11, 12, 8), gs::rgb4(6, 7, 4), gs::rgb4(3, 4, 2), gs::rgb4(13, 4, 2));

    setPal(vdp, PAL_FX,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(15, 12, 4), gs::rgb4(12, 6, 2), gs::rgb4(8, 8, 8), gs::rgb4(1, 1, 1),
            gs::rgb4(4, 3, 2), gs::rgb4(7, 6, 5), gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 2), gs::rgb4(10, 9, 7),
            gs::rgb4(12, 12, 11), 0, 0, 0, shade});
    setPal(vdp, PAL_ROCK,
           {0, gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(11, 8, 6), gs::rgb4(6, 5, 4), gs::rgb4(7, 6, 4),
            gs::rgb4(10, 8, 5), gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 4), gs::rgb4(6, 5, 3), gs::rgb4(12, 9, 5),
            gs::rgb4(14, 11, 2), gs::rgb4(5, 4, 3), gs::rgb4(13, 12, 8), gs::rgb4(3, 2, 2), shade});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 10, 7), gs::rgb4(8, 6, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(10, 8, 6), gs::rgb4(7, 5, 4), gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 8), gs::rgb4(1, 1, 1),
            gs::rgb4(3, 3, 3), 0, 0, gs::rgb4(15, 12, 2), gs::rgb4(12, 6, 2), 0, 0, 0, 0, shade});

    uint8_t tile[64];
    gs::TileAlloc alloc(vdp);
    skyLip(tile);
    int lip = makeTile(alloc, vdp, tile);
    rockTile(tile, 0);
    int rockA = makeTile(alloc, vdp, tile);
    rockTile(tile, 5);
    int rockB = makeTile(alloc, vdp, tile);
    seamTile(tile);
    int seam = makeTile(alloc, vdp, tile);
    haulTile(tile);
    int haul = makeTile(alloc, vdp, tile);
    paintFace(vdp, lip, rockA, rockB, seam, haul);

    art.loader[0] = gs::uploadMipped(vdp, loaderArt(0));
    art.loader[1] = gs::uploadMipped(vdp, loaderArt(1));
    art.dumper[0] = gs::uploadMipped(vdp, dumperArt(0));
    art.dumper[1] = gs::uploadMipped(vdp, dumperArt(1));
    art.hoe[0] = gs::uploadMipped(vdp, hoeArt(0));
    art.hoe[1] = gs::uploadMipped(vdp, hoeArt(1));
    art.crush[0] = gs::uploadMipped(vdp, crushArt(0));
    art.crush[1] = gs::uploadMipped(vdp, crushArt(1));
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.boulder = gs::uploadMipped(vdp, boulderArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    loadFont(vdp, alloc, art);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.B.scroll(0, 0);
    vdp.hudEnabled = true;
}

}  // namespace quarrypurs
