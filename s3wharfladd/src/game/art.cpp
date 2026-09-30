#include "game/art.h"

#include <initializer_list>
#include <string>

namespace wharfladd {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void tile8(gs::Bitmap& b, int c) { b.rect(0, 0, 8, 8, c); }

// pose: 0 stand, 1 walk A, 2 walk B, 3 jump, 4 climb
gs::Bitmap docker(int pose) {
    gs::Bitmap b(32, 48);
    auto R = [&](int x, int y, int w, int h, int c) { b.rect(float(x), float(y), float(w), float(h), c); };
    R(10, 1, 12, 4, 8);    // sou'wester brim
    R(12, 4, 8, 6, 4);     // face
    R(13, 6, 2, 2, 1);
    R(17, 6, 2, 2, 1);
    R(14, 9, 4, 1, 9);
    if (pose == 4) {
        R(8, 11, 4, 12, 5);
        R(20, 11, 4, 12, 5);
        R(11, 12, 10, 16, 2);
        R(12, 26, 8, 3, 6);
        R(13, 29, 3, 12, 3);
        R(17, 29, 3, 12, 3);
        R(12, 40, 5, 6, 7);
        R(17, 40, 5, 6, 7);
        b.outline(1, false);
        return b;
    }
    int ls = 0, rs = 0, la = 0, ra = 0;
    if (pose == 1) {
        ls = 2;
        rs = -2;
        la = -2;
        ra = 1;
    } else if (pose == 2) {
        ls = -2;
        rs = 2;
        la = 1;
        ra = -2;
    }
    R(6, 14 + la, 4, 11, 5);
    R(22, 14 + ra, 4, 11, 5);
    R(10, 11, 12, 16, 2);
    R(11, 24, 10, 3, 6);
    if (pose == 3) {
        R(8, 30, 5, 8, 3);
        R(18, 28, 5, 8, 3);
        R(7, 37, 6, 5, 7);
        R(18, 35, 6, 5, 7);
    } else {
        R(11 + ls, 28, 4, 12, 3);
        R(17 + rs, 28, 4, 12, 3);
        R(10 + ls, 39, 6, 7, 7);
        R(16 + rs, 39, 6, 7, 7);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(14, 32);
    b.rect(1, 0, 3, 32, 3);
    b.rect(10, 0, 3, 32, 3);
    b.rect(1, 0, 1, 32, 5);
    for (int y = 2; y < 32; y += 8) b.rect(2, y, 10, 2, 4);
    return b;
}

gs::Bitmap dollyArt() {
    gs::Bitmap b(36, 28);
    b.rect(4, 2, 28, 16, 2);
    b.rect(6, 4, 24, 12, 3);
    b.rect(8, 6, 8, 8, 4);
    b.rect(18, 6, 8, 8, 5);
    b.rect(2, 16, 32, 4, 6);
    b.ellipse(8, 23, 4, 4, 1);
    b.ellipse(28, 23, 4, 4, 1);
    b.ellipse(8, 23, 2, 2, 7);
    b.ellipse(28, 23, 2, 2, 7);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(24, 12);
    b.poly({{2, 8}, {10, 3}, {12, 6}, {4, 10}}, 2);
    b.poly({{22, 8}, {14, 3}, {12, 6}, {20, 10}}, 3);
    b.rect(11, 5, 3, 3, 4);
    b.set(16, 6, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 8, 2, 14, 1);
    b.rect(2, 2, 6, 7, 3);
    b.rect(3, 3, 4, 5, 4);
    b.rect(1, 8, 8, 2, 2);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(18, 12);
    b.ellipse(9, 6, 8, 5, 2);
    b.ellipse(9, 6, 5, 3, 3);
    b.ellipse(9, 6, 2, 1, 4);
    return b;
}

gs::Bitmap pilingArt() {
    gs::Bitmap b(16, 40);
    b.rect(3, 0, 10, 36, 2);
    b.rect(4, 0, 3, 36, 3);
    b.rect(1, 32, 14, 6, 4);
    b.rect(0, 36, 16, 4, 5);
    for (int y = 4; y < 32; y += 8) b.rect(3, y, 10, 1, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 3, 6), gs::rgb4(2, 6, 9), gs::rgb4(4, 9, 12), gs::rgb4(8, 12, 14),
                            gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(13, 10, 5),
                             gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_COAT,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 4, 7), gs::rgb4(4, 3, 2), gs::rgb4(12, 8, 6), gs::rgb4(9, 9, 8),
            gs::rgb4(6, 5, 2), gs::rgb4(2, 2, 1), gs::rgb4(14, 12, 6), gs::rgb4(8, 3, 3)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(12, 11, 8),
                           gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(2, 2, 2), gs::rgb4(9, 6, 2), gs::rgb4(12, 8, 3), gs::rgb4(7, 4, 2),
                            gs::rgb4(14, 11, 6), gs::rgb4(4, 3, 2), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 14, 13), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5),
                           gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 14, 8), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 1)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(3, 2, 1), gs::rgb4(10, 8, 4), gs::rgb4(13, 11, 6), gs::rgb4(7, 5, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    auto solid = [&](int c) {
        gs::Bitmap b(8, 8);
        tile8(b, c);
        return tiles.shared(b.px.data());
    };
    art.sky = solid(1);
    {
        gs::Bitmap c(8, 8);
        tile8(c, 1);
        c.ellipse(3, 3, 3, 2, 2);
        art.cloud = tiles.shared(c.px.data());
        gs::Bitmap p(8, 8);
        tile8(p, 2);
        p.rect(0, 0, 8, 2, 4);
        p.rect(0, 6, 8, 2, 1);
        art.plank = tiles.shared(p.px.data());
        gs::Bitmap q(8, 8);
        tile8(q, 3);
        q.rect(0, 0, 8, 2, 5);
        q.rect(3, 2, 2, 4, 1);
        art.plankB = tiles.shared(q.px.data());
        gs::Bitmap e(8, 8);
        tile8(e, 2);
        e.rect(0, 0, 2, 8, 5);
        e.rect(6, 0, 2, 8, 1);
        art.beam = tiles.shared(e.px.data());
        gs::Bitmap n(8, 8);
        tile8(n, 4);
        n.rect(3, 3, 2, 2, 1);
        art.nail = tiles.shared(n.px.data());
        gs::Bitmap w(8, 8);
        tile8(w, 2);
        w.rect(0, 5, 8, 2, 3);
        w.rect(2, 2, 3, 1, 4);
        art.water = tiles.shared(w.px.data());
        gs::Bitmap z(8, 8);
        tile8(z, 1);
        z.rect(0, 4, 8, 2, 3);
        z.rect(5, 1, 2, 1, 4);
        art.waterB = tiles.shared(z.px.data());
    }

    art.stand = gs::uploadMipped(vdp, docker(0));
    art.walkA = gs::uploadMipped(vdp, docker(1));
    art.walkB = gs::uploadMipped(vdp, docker(2));
    art.jump = gs::uploadMipped(vdp, docker(3));
    art.climbA = gs::uploadMipped(vdp, docker(4));
    {
        gs::Bitmap alt = docker(4);
        alt.rect(9, 12, 3, 8, 6);
        alt.rect(20, 14, 3, 8, 6);
        art.climbB = gs::uploadMipped(vdp, alt);
    }
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.dolly = gs::uploadMipped(vdp, dollyArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.piling = gs::uploadMipped(vdp, pilingArt());
}

}  // namespace wharfladd
