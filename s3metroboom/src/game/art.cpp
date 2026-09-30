#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace metro {
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

void solid(uint8_t* px, int c) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

void paintTiles(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    uint8_t arch[64], seam[64], cable[64], dark[64], rail[64], sleep[64], deck[64], lip[64];
    solid(arch, 2);
    for (int y = 0; y < 8; y++) {
        arch[y * 8 + 0] = 1;
        arch[y * 8 + 7] = 3;
    }
    arch[1 * 8 + 3] = 4;
    arch[4 * 8 + 5] = 4;
    solid(seam, 3);
    for (int x = 0; x < 8; x++) seam[7 * 8 + x] = 1;
    solid(cable, 1);
    for (int x = 0; x < 8; x++) {
        cable[2 * 8 + x] = 5;
        cable[3 * 8 + x] = 6;
    }
    cable[2 * 8 + 1] = 4;
    cable[3 * 8 + 6] = 4;
    solid(dark, 1);
    solid(rail, 7);
    for (int x = 0; x < 8; x++) {
        rail[2 * 8 + x] = 8;
        rail[3 * 8 + x] = 9;
        rail[6 * 8 + x] = 8;
    }
    rail[3 * 8 + 4] = 10;
    solid(sleep, 7);
    for (int y = 0; y < 8; y++) sleep[y * 8 + 1] = 11;
    for (int x = 0; x < 8; x += 2) sleep[5 * 8 + x] = 11;
    solid(deck, 12);
    for (int x = 1; x < 8; x += 3) deck[3 * 8 + x] = 13;
    solid(lip, 12);
    for (int x = 0; x < 8; x++) lip[0 * 8 + x] = 14;

    art.arch = tiles.alloc(1);
    vdp.loadTile(art.arch, arch);
    art.seam = tiles.alloc(1);
    vdp.loadTile(art.seam, seam);
    art.cable = tiles.alloc(1);
    vdp.loadTile(art.cable, cable);
    art.voidTile = tiles.alloc(1);
    vdp.loadTile(art.voidTile, dark);
    art.rail = tiles.alloc(1);
    vdp.loadTile(art.rail, rail);
    art.sleeper = tiles.alloc(1);
    vdp.loadTile(art.sleeper, sleep);
    art.deck = tiles.alloc(1);
    vdp.loadTile(art.deck, deck);
    art.lip = tiles.alloc(1);
    vdp.loadTile(art.lip, lip);
}

void paintCar(gs::Bitmap& b, bool crew) {
    b.rect(8, 16, 104, 22, 2);
    b.poly({{18, 16}, {28, 6}, {92, 6}, {104, 16}}, 3);
    b.rect(30, 8, 22, 7, 6);
    b.rect(58, 8, 22, 7, 6);
    b.rect(2, 20, 10, 12, 4);
    b.rect(108, 18, 8, 14, 4);
    b.rect(12, 20, 6, 10, 5);
    for (int i = 0; i < 4; i++) b.rect(24 + i * 20, 20, 12, 8, 6);
    b.rect(10, 34, 100, 4, crew ? 5 : 7);
    b.ellipse(30, 40, 7, 7, 8);
    b.ellipse(30, 40, 3, 3, 9);
    b.ellipse(90, 40, 7, 7, 8);
    b.ellipse(90, 40, 3, 3, 9);
    b.rect(70, 22, 10, 8, 10);
}

gs::Bitmap makeDrive() {
    gs::Bitmap b(22, 16);
    b.rect(1, 3, 20, 11, 2);
    b.rect(3, 5, 16, 7, 3);
    b.ellipse(11, 8, 4, 3, 4);
    b.rect(10, 6, 2, 5, 5);
    b.rect(0, 6, 3, 5, 6);
    return b;
}

gs::Bitmap makeBoom() {
    gs::Bitmap b(78, 56);
    b.rect(6, 48, 14, 8, 2);
    b.rect(10, 18, 6, 32, 3);
    b.poly({{12, 20}, {70, 8}, {70, 16}, {16, 28}}, 4);
    b.rect(64, 6, 10, 14, 5);
    b.rect(68, 18, 2, 16, 6);
    b.rect(62, 32, 14, 4, 5);
    b.line(16, 22, 66, 12, 7, 1);
    return b;
}

gs::Bitmap makeHook() {
    gs::Bitmap b(12, 18);
    b.rect(5, 0, 2, 8, 2);
    b.poly({{3, 8}, {9, 8}, {8, 16}, {4, 16}}, 3);
    b.rect(4, 14, 4, 2, 4);
    return b;
}

gs::Bitmap makeLamp() {
    gs::Bitmap b(14, 30);
    b.rect(6, 10, 2, 20, 2);
    b.ellipse(7, 8, 5, 5, 3);
    b.ellipse(7, 8, 2, 2, 4);
    b.rect(3, 12, 8, 2, 2);
    return b;
}

gs::Bitmap makeGate() {
    gs::Bitmap b(16, 36);
    b.rect(2, 0, 12, 28, 2);
    b.rect(4, 4, 8, 6, 3);
    b.rect(4, 14, 8, 6, 4);
    b.rect(6, 26, 4, 10, 5);
    b.rect(1, 32, 14, 3, 6);
    return b;
}

void layTunnel(gs::VDP& vdp, const Art& art) {
    auto row = [&](gs::Plane& p, int y, int tile, int pal) {
        for (int x = 0; x < p.w; x++) p.set(x, y, gs::entry(tile, pal));
    };
    gs::Plane& B = vdp.B;
    gs::Plane& A = vdp.A;
    for (int y = 0; y < 4; y++) row(B, y, y < 1 ? art.cable : art.arch, PAL_TUNNEL);
    for (int y = 4; y < 9; y++) row(B, y, (y & 1) ? art.seam : art.arch, PAL_TUNNEL);
    row(B, 9, art.voidTile, PAL_TUNNEL);
    row(B, 10, art.rail, PAL_TUNNEL);
    for (int y = 14; y < 18; y++) row(A, y, art.voidTile, PAL_TUNNEL);
    row(A, 18, art.sleeper, PAL_TUNNEL);
    row(A, 19, art.rail, PAL_TUNNEL);
    row(A, 20, art.lip, PAL_TUNNEL);
    for (int y = 21; y < 26; y++) row(A, y, art.deck, PAL_TUNNEL);
    for (int y = 26; y < 28; y++) row(B, y, art.voidTile, PAL_TUNNEL);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 13), gs::rgb4(3, 4, 5), gs::rgb4(15, 13, 5), gs::rgb4(14, 4, 3),
                          gs::rgb4(6, 13, 9)});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(1, 3, 4), gs::rgb4(12, 14, 13), gs::rgb4(8, 12, 11), gs::rgb4(3, 6, 7),
                          gs::rgb4(15, 8, 2), gs::rgb4(5, 12, 14), gs::rgb4(15, 11, 3), gs::rgb4(2, 3, 3),
                          gs::rgb4(7, 8, 8), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(3, 1, 2), gs::rgb4(13, 4, 5), gs::rgb4(9, 2, 3), gs::rgb4(4, 3, 4),
                           gs::rgb4(15, 9, 3), gs::rgb4(6, 3, 4), gs::rgb4(10, 3, 4), gs::rgb4(2, 1, 2),
                           gs::rgb4(6, 5, 5), gs::rgb4(14, 12, 10)});
    setPal(vdp, PAL_DRIVE, {0, gs::rgb4(4, 3, 1), gs::rgb4(12, 9, 3), gs::rgb4(2, 6, 8), gs::rgb4(8, 14, 15),
                            gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 3), gs::rgb4(14, 6, 2)});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 4), gs::rgb4(14, 11, 3),
                           gs::rgb4(15, 13, 5), gs::rgb4(4, 4, 4), gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 4)});
    setPal(vdp, PAL_TUNNEL, {0, gs::rgb4(1, 2, 3), gs::rgb4(4, 5, 6), gs::rgb4(6, 7, 8), gs::rgb4(8, 9, 8),
                             gs::rgb4(10, 8, 3), gs::rgb4(12, 10, 4), gs::rgb4(2, 3, 4), gs::rgb4(9, 9, 7),
                             gs::rgb4(12, 12, 9), gs::rgb4(5, 6, 5), gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 6),
                             gs::rgb4(7, 6, 5), gs::rgb4(11, 10, 6), gs::rgb4(13, 12, 7)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(2, 3, 3), gs::rgb4(5, 6, 6), gs::rgb4(15, 14, 7), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(2, 2, 2), gs::rgb4(12, 3, 2), gs::rgb4(14, 12, 3), gs::rgb4(4, 4, 5),
                           gs::rgb4(8, 8, 7), gs::rgb4(6, 5, 3), gs::rgb4(15, 15, 8)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    paintTiles(vdp, tiles, art);
    layTunnel(vdp, art);

    gs::Bitmap car(120, 48);
    paintCar(car, false);
    art.car = gs::uploadMipped(vdp, car);
    gs::Bitmap crew(120, 48);
    paintCar(crew, true);
    art.crew = gs::uploadMipped(vdp, crew);
    art.drive = gs::uploadMipped(vdp, makeDrive());
    art.boom = gs::uploadMipped(vdp, makeBoom());
    art.hook = gs::uploadMipped(vdp, makeHook());
    art.lamp = gs::uploadMipped(vdp, makeLamp());
    art.gate = gs::uploadMipped(vdp, makeGate());
    vdp.setFogColor(gs::rgb4(1, 2, 3));
}

}  // namespace metro
