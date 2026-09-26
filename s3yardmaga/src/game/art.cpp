#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace yardmaga {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap cutterArt(int step) {
    gs::Bitmap b(32, 64);
    b.rect(10, 2, 12, 5, 4);
    b.rect(11, 6, 10, 3, 4);
    b.ellipse(16, 13, 6, 6, 3);
    b.set(14, 12, 8);
    b.set(19, 12, 8);
    b.rect(8, 19, 16, 22, 1);
    b.rect(8, 19, 5, 22, 2);
    b.rect(8, 26, 16, 3, 5);
    b.rect(3, 21, 5, 14, 1);
    b.rect(24, 21, 5, 14, 2);
    b.rect(2, 30, 28, 3, 9);
    int la = step ? 15 : 20;
    int lb = step ? 20 : 15;
    b.rect(9, 41, 6, la, 1);
    b.rect(17, 41, 6, lb, 2);
    b.rect(8, 41 + la - 3, 8, 4, 6);
    b.rect(16, 41 + lb - 3, 8, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap handArt(int step) {
    gs::Bitmap b(32, 64);
    b.rect(11, 1, 10, 4, 5);
    b.rect(13, 2, 6, 3, 6);
    b.rect(10, 5, 12, 4, 4);
    b.ellipse(16, 14, 6, 6, 3);
    b.set(14, 13, 7);
    b.set(19, 13, 7);
    b.rect(8, 20, 16, 18, 1);
    b.rect(8, 20, 6, 18, 2);
    b.rect(9, 38, 14, 4, 8);
    b.rect(4, 22, 4, 13, 8);
    b.rect(24, 22, 4, 13, 8);
    b.rect(3, 33, 6, 3, 10);
    int la = step ? 14 : 19;
    int lb = step ? 19 : 14;
    b.rect(9, 42, 6, la, 8);
    b.rect(17, 42, 6, lb, 2);
    b.rect(8, 42 + la - 3, 8, 4, 9);
    b.rect(16, 42 + lb - 3, 8, 4, 9);
    b.outline(7, false);
    return b;
}

gs::Bitmap fallenArt() {
    gs::Bitmap b(68, 26);
    b.ellipse(12, 13, 7, 6, 3);
    b.rect(6, 7, 12, 4, 4);
    b.rect(18, 10, 28, 9, 1);
    b.rect(18, 10, 28, 3, 2);
    b.rect(22, 14, 12, 3, 5);
    b.rect(44, 12, 16, 6, 2);
    b.rect(42, 17, 10, 4, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(88, 74);
    b.poly({{2, 22}, {44, 2}, {86, 22}}, 7);
    b.rect(6, 20, 76, 46, 1);
    b.rect(6, 20, 76, 7, 2);
    b.rect(10, 32, 22, 30, 5);
    b.rect(14, 36, 14, 16, 6);
    b.rect(40, 30, 14, 12, 6);
    b.rect(60, 30, 14, 12, 6);
    b.rect(6, 60, 76, 8, 4);
    gs::Bitmap word = gs::textBitmap("YARD", gs::TextStyle{2, 8, 0, 0, 1});
    b.blit(word, 40, 46);
    return b;
}

gs::Bitmap boxcarArt() {
    gs::Bitmap b(96, 62);
    b.rect(2, 6, 92, 5, 7);
    b.rect(6, 10, 84, 32, 1);
    b.rect(6, 10, 84, 6, 2);
    b.rect(12, 18, 22, 20, 5);
    b.rect(16, 20, 14, 12, 6);
    b.rect(42, 18, 16, 10, 6);
    b.rect(66, 18, 16, 10, 6);
    b.rect(6, 38, 84, 5, 4);
    b.ellipse(24, 50, 8, 8, 9);
    b.ellipse(72, 50, 8, 8, 9);
    b.ellipse(24, 50, 3, 3, 3);
    b.ellipse(72, 50, 3, 3, 3);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(80, 96);
    b.rect(10, 36, 10, 52, 2);
    b.rect(8, 32, 14, 6, 1);
    b.rect(6, 8, 68, 8, 1);
    b.rect(6, 16, 68, 3, 3);
    b.rect(62, 16, 3, 30, 3);
    b.rect(54, 44, 16, 8, 4);
    b.rect(60, 52, 3, 12, 7);
    b.rect(56, 62, 11, 3, 4);
    b.rect(4, 84, 26, 8, 5);
    b.rect(20, 16, 2, 18, 7);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(48, 90);
    b.ellipse(24, 16, 18, 12, 1);
    b.ellipse(24, 14, 11, 7, 6);
    b.rect(8, 24, 32, 5, 4);
    b.rect(20, 28, 8, 50, 2);
    b.rect(16, 28, 5, 50, 3);
    b.rect(8, 76, 32, 8, 5);
    b.rect(22, 46, 4, 6, 8);
    return b;
}

gs::Bitmap palletArt() {
    gs::Bitmap b(44, 40);
    for (int i = 0; i < 3; ++i) {
        int y = 4 + i * 11;
        b.rect(2, y, 40, 9, 1 + (i & 1));
        b.rect(2, y, 40, 2, 5);
    }
    b.rect(6, 2, 3, 36, 3);
    b.rect(35, 2, 3, 36, 3);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(28, 36);
    b.ellipse(14, 8, 12, 5, 1);
    b.rect(2, 8, 24, 18, 4);
    b.rect(2, 14, 24, 5, 8);
    b.ellipse(14, 26, 12, 5, 2);
    b.rect(2, 24, 24, 3, 7);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(22, 76);
    b.rect(9, 18, 4, 50, 2);
    b.rect(8, 18, 2, 50, 1);
    b.rect(3, 6, 16, 14, 1);
    b.rect(6, 9, 10, 8, 6);
    b.rect(8, 11, 6, 4, 8);
    b.rect(2, 66, 18, 7, 5);
    return b;
}

gs::Bitmap poolArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(24, 8, 22, 6, 6);
    b.ellipse(24, 8, 10, 3, 4);
    return b;
}

gs::Bitmap chevronArt() {
    gs::Bitmap b(32, 8);
    for (int x = 0; x < 32; x += 8) {
        b.rect(x, 0, 4, 8, 1);
        b.rect(x + 4, 0, 4, 8, 7);
    }
    return b;
}

gs::Bitmap jambArt() {
    gs::Bitmap b(22, 120);
    b.rect(2, 0, 18, 120, 2);
    b.rect(2, 0, 6, 120, 1);
    for (int y = 6; y < 114; y += 14) b.rect(6, y, 10, 3, 9);
    b.rect(4, 112, 14, 8, 5);
    return b;
}

gs::Bitmap sillArt() {
    gs::Bitmap b(80, 20);
    b.rect(0, 0, 80, 20, 2);
    b.rect(0, 0, 80, 4, 1);
    b.rect(0, 15, 80, 5, 3);
    for (int x = 4; x < 78; x += 12) b.rect(x, 6, 2, 8, 4);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(26, 78);
    b.rect(10, 2, 6, 48, 1);
    b.rect(11, 0, 4, 8, 2);
    b.rect(12, 1, 2, 6, 8);
    b.rect(6, 28, 16, 12, 5);
    b.rect(8, 30, 12, 4, 3);
    b.rect(7, 44, 7, 16, 4);
    b.poly({{9, 62}, {18, 62}, {22, 76}, {7, 76}}, 5);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(36, 36);
    const float cx = 18, cy = 18;
    for (int i = 0; i < 72; ++i) {
        if (i % 18 < 3) continue;
        float a = i * 6.2831853f / 72.f;
        b.set(int(std::lround(cx + std::cos(a) * 13)), int(std::lround(cy + std::sin(a) * 13)), 1);
        b.set(int(std::lround(cx + std::cos(a) * 12)), int(std::lround(cy + std::sin(a) * 12)), 1);
    }
    b.rect(17, 2, 2, 5, 1);
    b.rect(17, 29, 2, 5, 1);
    b.rect(2, 17, 5, 2, 1);
    b.rect(29, 17, 5, 2, 1);
    b.rect(17, 17, 2, 2, 4);
    return b;
}

gs::Bitmap roundArt(bool live) {
    gs::Bitmap b(10, 22);
    if (live) {
        b.rect(2, 1, 6, 14, 1);
        b.rect(3, 2, 4, 5, 2);
        b.rect(2, 13, 6, 3, 3);
        b.rect(3, 16, 4, 4, 6);
    } else {
        b.rect(3, 6, 4, 9, 7);
        b.rect(3, 14, 4, 4, 8);
    }
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 5, 6);
    b.ellipse(11, 11, 4, 3, 4);
    b.rect(10, 1, 2, 20, 4);
    b.rect(1, 10, 20, 2, 4);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 6);
    b.ellipse(8, 8, 3, 2, 4);
    b.set(3, 3, 2);
    b.set(12, 4, 1);
    b.set(5, 12, 6);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 1);
    b.ellipse(8, 8, 3, 3, 4);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 6);
    b.ellipse(14, 14, 6, 6, 4);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 2);
    b.set(0, 1, 1);
    b.set(1, 1, 2);
    b.set(2, 1, 1);
    b.set(1, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 10);
    b.ellipse(16, 5, 14, 4, 1);
    return b;
}

gs::Bitmap steamArt() {
    gs::Bitmap b(36, 20);
    b.ellipse(12, 12, 9, 6, 1);
    b.ellipse(22, 10, 10, 7, 2);
    b.ellipse(28, 14, 6, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 0, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 11), gs::rgb4(7, 6, 5), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_AMBER,
           {0, gs::rgb4(15, 11, 2), gs::rgb4(15, 14, 8), gs::rgb4(8, 5, 1), gs::rgb4(3, 2, 1), 0, 0, gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(9, 2, 2), gs::rgb4(15, 12, 10)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(7, 15, 6), gs::rgb4(2, 8, 3), gs::rgb4(13, 15, 11)});
    vdp.setColor(PAL_HUD * 16 + 15, shadow);
    vdp.setColor(PAL_AMBER * 16 + 15, shadow);
    vdp.setColor(PAL_RED * 16 + 15, shadow);
    vdp.setColor(PAL_GREEN * 16 + 15, shadow);

    setPal(vdp, PAL_CUT,
           {0, gs::rgb4(2, 2, 4), gs::rgb4(5, 5, 8), gs::rgb4(13, 9, 6), gs::rgb4(1, 1, 2), gs::rgb4(13, 2, 2),
            gs::rgb4(1, 1, 1), gs::rgb4(10, 10, 12), gs::rgb4(15, 14, 8), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_HAND,
           {0, gs::rgb4(15, 12, 1), gs::rgb4(10, 8, 1), gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 5), gs::rgb4(15, 15, 8),
            gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), gs::rgb4(3, 4, 8), gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 9, 4), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 1), gs::rgb4(14, 11, 6),
            gs::rgb4(9, 7, 3)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(12, 12, 14), gs::rgb4(7, 7, 9), gs::rgb4(3, 3, 5), gs::rgb4(10, 5, 2), gs::rgb4(9, 6, 3),
            gs::rgb4(15, 13, 4), gs::rgb4(1, 1, 2), gs::rgb4(15, 15, 12), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(11, 5, 3), gs::rgb4(7, 3, 2), gs::rgb4(8, 7, 6), gs::rgb4(3, 2, 2), gs::rgb4(2, 2, 3),
            gs::rgb4(12, 10, 6), gs::rgb4(4, 4, 5), gs::rgb4(15, 12, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10), gs::rgb4(10, 7, 2), gs::rgb4(15, 15, 14), gs::rgb4(15, 14, 6),
            gs::rgb4(15, 8, 2), gs::rgb4(12, 9, 3), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(8, 8, 10), gs::rgb4(13, 13, 14), gs::rgb4(5, 5, 6)});

    const uint16_t yard[16] = {
        0,
        gs::rgb4(7, 6, 4), gs::rgb4(4, 4, 3), gs::rgb4(9, 7, 4),
        gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 2),
        gs::rgb4(8, 7, 6), gs::rgb4(5, 5, 4),
        gs::rgb4(10, 9, 7), gs::rgb4(4, 4, 5),
        gs::rgb4(3, 3, 3), gs::rgb4(3, 3, 4),
        gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 7),
        gs::rgb4(12, 9, 3), gs::rgb4(11, 10, 8),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    loadFont(vdp, art);
    art.cutter[0] = gs::uploadMipped(vdp, cutterArt(0));
    art.cutter[1] = gs::uploadMipped(vdp, cutterArt(1));
    art.hand[0] = gs::uploadMipped(vdp, handArt(0));
    art.hand[1] = gs::uploadMipped(vdp, handArt(1));
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.boxcar = gs::uploadMipped(vdp, boxcarArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.pallet = gs::uploadMipped(vdp, palletArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pool = gs::uploadMipped(vdp, poolArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    art.jamb = gs::uploadMipped(vdp, jambArt());
    art.sill = gs::uploadMipped(vdp, sillArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.sight = gs::uploadMipped(vdp, sightArt());
    art.round = gs::uploadMipped(vdp, roundArt(true));
    art.spent = gs::uploadMipped(vdp, roundArt(false));
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.steam = gs::uploadMipped(vdp, steamArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(8, 4, 2));
}

}  // namespace yardmaga
