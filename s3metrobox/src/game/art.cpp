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
    uint8_t brick[64], hi[64], pipe[64], dark[64], far[64], near[64], plat[64], edge[64];
    solid(brick, 2);
    for (int y = 0; y < 8; y++) brick[y * 8 + 0] = 1;
    for (int x = 0; x < 8; x++) brick[0 * 8 + x] = 1;
    brick[2 * 8 + 3] = 3;
    brick[5 * 8 + 6] = 3;
    solid(hi, 3);
    for (int y = 0; y < 8; y++) hi[y * 8 + 7] = 1;
    solid(pipe, 4);
    for (int x = 0; x < 8; x++) {
        pipe[2 * 8 + x] = 5;
        pipe[3 * 8 + x] = 6;
        pipe[5 * 8 + x] = 5;
    }
    solid(dark, 1);
    solid(far, 1);
    for (int x = 0; x < 8; x++) far[6 * 8 + x] = 7;
    far[6 * 8 + 1] = 8;
    far[6 * 8 + 6] = 8;
    solid(near, 9);
    for (int x = 0; x < 8; x++) {
        near[3 * 8 + x] = 8;
        near[4 * 8 + x] = 7;
        near[7 * 8 + x] = 10;
    }
    near[4 * 8 + 2] = 11;
    near[4 * 8 + 5] = 11;
    solid(plat, 12);
    for (int x = 0; x < 8; x += 2) plat[2 * 8 + x] = 13;
    solid(edge, 12);
    for (int x = 0; x < 8; x++) edge[0 * 8 + x] = 14;

    art.brick = tiles.alloc(1);
    vdp.loadTile(art.brick, brick);
    art.brickHi = tiles.alloc(1);
    vdp.loadTile(art.brickHi, hi);
    art.pipe = tiles.alloc(1);
    vdp.loadTile(art.pipe, pipe);
    art.dark = tiles.alloc(1);
    vdp.loadTile(art.dark, dark);
    art.farRail = tiles.alloc(1);
    vdp.loadTile(art.farRail, far);
    art.nearRail = tiles.alloc(1);
    vdp.loadTile(art.nearRail, near);
    art.plat = tiles.alloc(1);
    vdp.loadTile(art.plat, plat);
    art.platEdge = tiles.alloc(1);
    vdp.loadTile(art.platEdge, edge);
}

void paintTrain(gs::Bitmap& b, bool rival) {
    b.rect(6, 14, 108, 22, 2);
    b.rect(10, 8, 96, 8, 3);
    b.rect(4, 16, 8, 16, 4);
    b.rect(108, 16, 8, 16, 4);
    for (int i = 0; i < 5; i++) b.rect(16 + i * 18, 16, 12, 8, 6);
    b.rect(8, 28, 104, 3, rival ? 5 : 7);
    b.rect(40, 18, 3, 16, 1);
    b.rect(78, 18, 3, 16, 1);
    b.ellipse(28, 38, 7, 7, 8);
    b.ellipse(28, 38, 3, 3, 9);
    b.ellipse(92, 38, 7, 7, 8);
    b.ellipse(92, 38, 3, 3, 9);
    b.ellipse(58, 38, 6, 6, 8);
    b.rect(8, 18, 4, 4, 10);
    b.rect(110, 18, 3, 3, 10);
}

gs::Bitmap makePost() {
    gs::Bitmap b(10, 44);
    b.rect(3, 0, 4, 44, 2);
    b.rect(1, 0, 8, 6, 3);
    b.rect(0, 40, 10, 4, 4);
    for (int y = 8; y < 40; y += 6) b.rect(3, y, 4, 3, 5);
    return b;
}

gs::Bitmap makeLamp() {
    gs::Bitmap b(16, 36);
    b.rect(7, 8, 2, 28, 2);
    b.ellipse(8, 8, 6, 6, 3);
    b.ellipse(8, 8, 3, 3, 4);
    b.rect(4, 14, 8, 2, 2);
    return b;
}

gs::Bitmap makePerson(int coat) {
    gs::Bitmap b(12, 22);
    b.ellipse(6, 4, 3, 3, 2);
    b.rect(3, 8, 6, 8, coat);
    b.rect(2, 16, 2, 6, 4);
    b.rect(8, 16, 2, 6, 4);
    b.rect(1, 9, 2, 5, coat);
    b.rect(9, 9, 2, 5, coat);
    return b;
}

gs::Bitmap makeClock() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 14, 10, 10, 3);
    b.rect(13, 6, 2, 9, 4);
    b.rect(13, 13, 7, 2, 5);
    b.rect(12, 4, 4, 2, 4);
    return b;
}

gs::Bitmap makeSignal() {
    gs::Bitmap b(14, 22);
    b.rect(6, 8, 2, 14, 2);
    b.rect(3, 1, 8, 12, 3);
    b.ellipse(7, 5, 2, 2, 4);
    b.ellipse(7, 9, 2, 2, 5);
    return b;
}

void layTunnel(gs::VDP& vdp, const Art& art) {
    auto row = [&](gs::Plane& p, int y, int tile, int pal) {
        for (int x = 0; x < p.w; x++) p.set(x, y, gs::entry(tile, pal));
    };
    gs::Plane& B = vdp.B;
    gs::Plane& A = vdp.A;
    for (int y = 0; y < 6; y++) row(B, y, y < 2 ? art.pipe : art.brick, PAL_TUNNEL);
    for (int y = 6; y < 10; y++) row(B, y, (y & 1) ? art.brickHi : art.brick, PAL_TUNNEL);
    row(B, 10, art.farRail, PAL_TUNNEL);
    row(B, 11, art.dark, PAL_TUNNEL);
    for (int y = 12; y < 16; y++) row(A, y, art.brick, PAL_TUNNEL);
    row(A, 16, art.dark, PAL_TUNNEL);
    row(A, 17, art.nearRail, PAL_TUNNEL);
    row(A, 18, art.nearRail, PAL_TUNNEL);
    row(A, 19, art.platEdge, PAL_TUNNEL);
    for (int y = 20; y < 25; y++) row(A, y, art.plat, PAL_TUNNEL);
    for (int y = 25; y < 28; y++) row(B, y, art.dark, PAL_TUNNEL);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 10), gs::rgb4(4, 3, 2), gs::rgb4(15, 12, 4), gs::rgb4(12, 3, 3),
                          gs::rgb4(8, 14, 8)});
    setPal(vdp, PAL_TRAIN, {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 6), gs::rgb4(11, 9, 4), gs::rgb4(6, 6, 7),
                            gs::rgb4(13, 4, 3), gs::rgb4(6, 10, 13), gs::rgb4(15, 10, 2), gs::rgb4(2, 2, 2),
                            gs::rgb4(8, 8, 8), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_RIVAL, {0, gs::rgb4(3, 1, 1), gs::rgb4(12, 3, 3), gs::rgb4(8, 2, 2), gs::rgb4(5, 4, 5),
                            gs::rgb4(15, 8, 3), gs::rgb4(4, 6, 8), gs::rgb4(9, 2, 2), gs::rgb4(2, 1, 1),
                            gs::rgb4(7, 6, 6), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(6, 4, 1), gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 8), gs::rgb4(10, 8, 2),
                          gs::rgb4(15, 9, 1), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_TUNNEL, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 4, 5), gs::rgb4(7, 6, 6), gs::rgb4(3, 3, 4),
                             gs::rgb4(8, 8, 9), gs::rgb4(5, 5, 6), gs::rgb4(4, 4, 5), gs::rgb4(9, 8, 4),
                             gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 3), gs::rgb4(12, 10, 4), gs::rgb4(7, 6, 5),
                             gs::rgb4(8, 7, 6), gs::rgb4(13, 11, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 3), gs::rgb4(6, 6, 7), gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 12),
                           gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 9, 6), gs::rgb4(4, 6, 10), gs::rgb4(3, 3, 4),
                           gs::rgb4(10, 4, 4), gs::rgb4(8, 10, 6)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(2, 2, 2), gs::rgb4(10, 9, 7), gs::rgb4(2, 3, 4), gs::rgb4(15, 4, 3),
                            gs::rgb4(15, 12, 4), gs::rgb4(14, 14, 12)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    paintTiles(vdp, tiles, art);
    layTunnel(vdp, art);

    gs::Bitmap train(120, 48);
    paintTrain(train, false);
    art.train = gs::uploadMipped(vdp, train);
    gs::Bitmap rival(120, 48);
    paintTrain(rival, true);
    art.rival = gs::uploadMipped(vdp, rival);
    art.post = gs::uploadMipped(vdp, makePost());
    art.lamp = gs::uploadMipped(vdp, makeLamp());
    art.person = gs::uploadMipped(vdp, makePerson(3));
    art.clock = gs::uploadMipped(vdp, makeClock());
    art.signal = gs::uploadMipped(vdp, makeSignal());
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace metro
