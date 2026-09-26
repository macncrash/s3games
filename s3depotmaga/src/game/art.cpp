#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace depotmaga {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap figure(int step, bool lamp) {
    gs::Bitmap b(36, 68);
    b.rect(11, 3, 14, 5, 4);
    if (lamp) {
        b.rect(21, 2, 7, 5, 5);
        b.rect(23, 3, 3, 3, 6);
    }
    b.ellipse(18, 14, 7, 7, 3);
    b.rect(12, 8, 13, 4, 4);
    b.set(15, 14, 7);
    b.set(21, 14, 7);
    b.rect(10, 22, 16, 20, 1);
    b.rect(10, 22, 6, 20, 2);
    b.rect(10, 38, 16, 3, 8);
    b.rect(5, 24, 5, 15, 1);
    b.rect(26, 24, 5, 15, 2);
    b.rect(5, 37, 5, 4, 3);
    b.rect(26, 37, 5, 4, 3);
    int la = step ? 18 : 22;
    int lb = step ? 22 : 18;
    b.rect(11, 42, 6, la, 2);
    b.rect(19, 42, 6, lb, 1);
    b.rect(10, 42 + la - 2, 8, 4, 7);
    b.rect(18, 42 + lb - 2, 8, 4, 7);
    b.outline(7, false);
    return b;
}

gs::Bitmap fallenMan() {
    gs::Bitmap b(68, 28);
    b.ellipse(12, 14, 7, 6, 3);
    b.rect(7, 8, 12, 4, 4);
    b.rect(18, 11, 30, 10, 1);
    b.rect(18, 11, 30, 4, 2);
    b.rect(46, 13, 14, 6, 2);
    b.rect(44, 18, 10, 4, 7);
    b.rect(20, 20, 18, 3, 8);
    b.outline(7, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(22, 18);
    b.rect(1, 2, 20, 14, 1);
    b.rect(1, 2, 20, 4, 2);
    b.rect(1, 12, 20, 4, 3);
    b.rect(4, 7, 6, 5, 4);
    b.rect(12, 7, 5, 5, 3);
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(44, 36);
    b.rect(1, 18, 20, 16, 1);
    b.rect(1, 18, 20, 4, 2);
    b.rect(18, 12, 22, 20, 3);
    b.rect(18, 12, 22, 4, 1);
    b.rect(8, 2, 18, 16, 2);
    b.rect(8, 2, 18, 4, 1);
    b.rect(12, 8, 8, 6, 4);
    b.rect(24, 20, 8, 6, 4);
    return b;
}

gs::Bitmap boxcarArt() {
    gs::Bitmap b(72, 52);
    b.rect(2, 4, 68, 6, 7);
    b.rect(0, 8, 72, 4, 8);
    b.rect(2, 12, 68, 26, 1);
    b.rect(2, 12, 10, 26, 2);
    b.rect(4, 16, 18, 20, 5);
    b.rect(8, 18, 10, 14, 6);
    b.rect(28, 16, 14, 10, 6);
    b.rect(48, 16, 14, 10, 6);
    b.rect(2, 32, 68, 4, 10);
    b.rect(8, 40, 12, 10, 9);
    b.rect(52, 40, 12, 10, 9);
    b.ellipse(14, 44, 5, 5, 9);
    b.ellipse(58, 44, 5, 5, 9);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(64, 72);
    b.rect(10, 28, 8, 38, 2);
    b.rect(8, 26, 12, 4, 1);
    b.rect(4, 10, 54, 6, 1);
    b.rect(4, 16, 54, 3, 3);
    b.rect(46, 18, 2, 22, 3);
    b.rect(40, 38, 14, 8, 4);
    b.rect(44, 46, 4, 6, 7);
    b.rect(4, 64, 22, 6, 7);
    b.rect(14, 18, 2, 10, 8);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(40, 76);
    b.ellipse(20, 18, 16, 12, 1);
    b.ellipse(20, 16, 11, 7, 2);
    b.rect(6, 22, 28, 5, 8);
    b.rect(16, 26, 8, 42, 2);
    b.rect(14, 26, 4, 42, 3);
    b.rect(10, 66, 20, 6, 7);
    b.rect(18, 48, 4, 4, 4);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(28, 34);
    b.ellipse(14, 8, 12, 6, 1);
    b.rect(2, 8, 24, 16, 2);
    b.rect(2, 14, 24, 4, 8);
    b.ellipse(14, 24, 12, 6, 3);
    b.rect(2, 22, 24, 3, 7);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(36, 30);
    b.ellipse(12, 16, 10, 9, 5);
    b.ellipse(22, 15, 11, 10, 6);
    b.ellipse(17, 18, 8, 7, 1);
    b.line(14, 10, 20, 12, 7, 1.2f);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 64);
    b.rect(6, 16, 4, 44, 2);
    b.rect(5, 16, 2, 44, 1);
    b.rect(3, 8, 10, 10, 3);
    b.rect(5, 10, 6, 6, 8);
    b.rect(2, 56, 12, 6, 7);
    return b;
}

gs::Bitmap jambArt() {
    gs::Bitmap b(40, 150);
    b.rect(4, 0, 32, 150, 2);
    b.rect(6, 0, 12, 150, 1);
    for (int y = 6; y < 146; y += 12) b.rect(6, y, 28, 3, 4);
    b.rect(12, 18, 16, 22, 5);
    b.rect(14, 20, 12, 8, 6);
    b.rect(14, 8, 10, 6, 8);
    b.rect(16, 9, 6, 4, 11);
    b.rect(4, 142, 32, 8, 3);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(74, 28);
    b.rect(0, 0, 74, 28, 3);
    b.rect(3, 3, 68, 22, 1);
    b.rect(3, 3, 68, 3, 2);
    gs::Bitmap word = gs::textBitmap("DEPOT", gs::TextStyle{2, 4, 0, 0, 1});
    b.blit(word, 8, 7);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 13, 13, 1);
    b.ellipse(14, 14, 10, 10, 2);
    b.line(14, 14, 14, 6, 7, 1.6f);
    b.line(14, 14, 21, 16, 7, 1.6f);
    b.ellipse(14, 14, 2, 2, 4);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(28, 78);
    b.rect(11, 4, 6, 46, 2);
    b.rect(12, 2, 4, 10, 1);
    b.rect(13, 2, 2, 8, 6);
    b.rect(7, 28, 16, 12, 5);
    b.rect(9, 30, 12, 4, 3);
    b.rect(8, 44, 6, 18, 4);
    b.poly({{10, 64}, {18, 64}, {22, 76}, {8, 76}}, 7);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(36, 36);
    const float cx = 18, cy = 18;
    for (int i = 0; i < 72; i++) {
        if (i % 18 < 3) continue;
        float a = i * 6.2831853f / 72.f;
        b.set(int(std::lround(cx + std::cos(a) * 13)), int(std::lround(cy + std::sin(a) * 13)), 1);
        b.set(int(std::lround(cx + std::cos(a) * 12)), int(std::lround(cy + std::sin(a) * 12)), 1);
    }
    b.rect(17, 2, 2, 5, 1);
    b.rect(17, 29, 2, 5, 1);
    b.rect(2, 17, 5, 2, 1);
    b.rect(29, 17, 5, 2, 1);
    return b;
}

gs::Bitmap roundArt(bool live) {
    gs::Bitmap b(10, 24);
    if (live) {
        b.rect(2, 1, 6, 16, 1);
        b.rect(3, 2, 4, 6, 2);
        b.rect(2, 14, 6, 4, 3);
        b.rect(3, 17, 4, 4, 6);
    } else {
        b.rect(3, 7, 4, 10, 7);
        b.rect(3, 15, 4, 4, 8);
    }
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 6, 5);
    b.ellipse(11, 11, 4, 3, 4);
    b.rect(10, 1, 2, 20, 4);
    b.rect(1, 10, 20, 2, 4);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 6);
    b.ellipse(8, 8, 3, 2, 5);
    b.set(3, 3, 4);
    b.set(12, 5, 5);
    b.set(5, 12, 6);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap chevronArt() {
    gs::Bitmap b(32, 10);
    for (int x = 0; x < 32; x += 8) {
        b.rect(x, 1, 4, 8, 1);
        b.rect(x + 4, 1, 4, 8, 3);
    }
    return b;
}

gs::Bitmap sillArt() {
    gs::Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    b.rect(0, 0, 32, 3, 2);
    b.rect(0, 12, 32, 4, 3);
    for (int x = 2; x < 30; x += 8) b.rect(x, 5, 2, 6, 3);
    return b;
}

gs::Bitmap steamArt() {
    gs::Bitmap b(40, 24);
    b.ellipse(14, 14, 10, 7, 1);
    b.ellipse(24, 12, 12, 8, 2);
    b.ellipse(30, 16, 7, 5, 3);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 10, 10, 5);
    b.ellipse(14, 14, 6, 6, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12), gs::rgb4(8, 7, 6), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 8), gs::rgb4(8, 6, 1), gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), gs::rgb4(15, 12, 10)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 8, 3), gs::rgb4(14, 15, 12)});
    vdp.setColor(PAL_HUD * 16 + 15, shadow);
    vdp.setColor(PAL_AMBER * 16 + 15, shadow);
    vdp.setColor(PAL_RED * 16 + 15, shadow);
    vdp.setColor(PAL_GREEN * 16 + 15, shadow);

    setPal(vdp, PAL_LIFT,
           {0, gs::rgb4(8, 9, 4), gs::rgb4(4, 5, 2), gs::rgb4(13, 9, 6), gs::rgb4(3, 3, 2), gs::rgb4(15, 12, 3),
            gs::rgb4(11, 8, 2), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_SHUNT,
           {0, gs::rgb4(7, 8, 11), gs::rgb4(4, 5, 7), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 3), gs::rgb4(15, 13, 4),
            gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(13, 6, 4), gs::rgb4(9, 3, 2), gs::rgb4(5, 2, 2), gs::rgb4(10, 8, 7), gs::rgb4(1, 1, 2),
            gs::rgb4(12, 8, 4), gs::rgb4(3, 2, 2), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 2), gs::rgb4(4, 3, 3),
            gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1), gs::rgb4(13, 11, 7),
            gs::rgb4(9, 7, 4), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_METAL,
           {0, gs::rgb4(13, 13, 14), gs::rgb4(8, 8, 10), gs::rgb4(4, 4, 6), gs::rgb4(10, 5, 2), gs::rgb4(9, 6, 3),
            gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10), gs::rgb4(10, 7, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 14, 6),
            gs::rgb4(15, 8, 2), gs::rgb4(15, 11, 4), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_SOOT, {0, gs::rgb4(10, 9, 8), gs::rgb4(13, 12, 11), gs::rgb4(6, 5, 5)});

    const uint16_t yard[16] = {
        0,
        gs::rgb4(8, 6, 4), gs::rgb4(6, 5, 3), gs::rgb4(9, 7, 4),
        gs::rgb4(4, 3, 3), gs::rgb4(3, 3, 3),
        gs::rgb4(6, 5, 4), gs::rgb4(5, 4, 4),
        gs::rgb4(7, 6, 5), gs::rgb4(11, 11, 12),
        gs::rgb4(3, 2, 2), gs::rgb4(3, 3, 4),
        gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 5),
        gs::rgb4(12, 9, 3), gs::rgb4(8, 7, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    loadFont(vdp, art);
    art.lift[0] = gs::uploadMipped(vdp, figure(0, false));
    art.lift[1] = gs::uploadMipped(vdp, figure(1, false));
    art.shunt[0] = gs::uploadMipped(vdp, figure(0, true));
    art.shunt[1] = gs::uploadMipped(vdp, figure(1, true));
    art.fallen = gs::uploadMipped(vdp, fallenMan());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.boxcar = gs::uploadMipped(vdp, boxcarArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.jamb = gs::uploadMipped(vdp, jambArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt(true));
    art.spent = gs::uploadMipped(vdp, roundArt(false));
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.sill = gs::uploadMipped(vdp, sillArt());
    art.steam = gs::uploadMipped(vdp, steamArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(7, 4, 3));
}

}  // namespace depotmaga
