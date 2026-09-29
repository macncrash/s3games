#include "game/art.h"

#include <initializer_list>

namespace rcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap sapperArt(int step) {
    gs::Bitmap b(32, 52);
    b.ellipse(16, 7, 8, 4, 3);
    b.rect(10, 6, 12, 3, 2);
    b.ellipse(16, 14, 6, 6, 4);
    b.set(13, 14, 8);
    b.set(19, 14, 8);
    b.rect(14, 18, 4, 2, 5);
    b.poly({{9, 22}, {23, 22}, {26, 36}, {6, 36}}, 1);
    b.rect(13, 23, 6, 10, 6);
    b.rect(6, 24, 5, 3, 2);
    if (step == 0) {
        b.rect(9, 36, 5, 12, 2);
        b.rect(18, 36, 5, 10, 2);
        b.rect(8, 47, 7, 3, 7);
        b.rect(17, 45, 7, 3, 7);
    } else {
        b.rect(9, 36, 5, 10, 2);
        b.rect(18, 36, 5, 12, 2);
        b.rect(8, 45, 7, 3, 7);
        b.rect(17, 47, 7, 3, 7);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap mattockArt() {
    gs::Bitmap b(22, 26);
    b.rect(10, 8, 2, 16, 2);
    b.poly({{2, 6}, {20, 4}, {18, 9}, {4, 11}}, 1);
    b.rect(4, 7, 12, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap spoilArt() {
    gs::Bitmap b(44, 22);
    b.poly({{4, 18}, {16, 4}, {28, 16}}, 1);
    b.poly({{18, 16}, {30, 6}, {40, 18}}, 2);
    b.ellipse(22, 18, 18, 4, 3);
    b.rect(14, 10, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap fascineArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(24, 8, 20, 6, 1);
    for (int x = 8; x < 42; x += 6) b.line(float(x), 3.f, float(x + 2), 13.f, 2, 1.2f);
    b.rect(6, 6, 36, 2, 3);
    b.rect(4, 7, 4, 3, 4);
    b.rect(40, 7, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap gabionArt() {
    gs::Bitmap b(28, 34);
    b.ellipse(14, 6, 10, 4, 3);
    b.rect(4, 6, 20, 20, 1);
    b.ellipse(14, 26, 10, 4, 2);
    for (int y = 8; y < 24; y += 4) b.rect(4, float(y), 20, 1, 4);
    for (int x = 8; x < 22; x += 5) b.rect(float(x), 6, 1, 20, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(36, 28);
    b.line(4, 24, 18, 4, 1, 2.4f);
    b.line(18, 24, 4, 6, 2, 2.2f);
    b.line(16, 22, 32, 6, 1, 2.2f);
    b.rect(2, 22, 32, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap kegArt() {
    gs::Bitmap b(24, 30);
    b.ellipse(12, 6, 9, 4, 3);
    b.rect(3, 6, 18, 16, 1);
    b.ellipse(12, 22, 9, 4, 2);
    b.rect(3, 12, 18, 3, 4);
    b.rect(10, 8, 4, 12, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(30, 30);
    b.ellipse(15, 15, 13, 13, 1);
    b.ellipse(15, 15, 8, 8, 0);
    b.ellipse(15, 15, 3, 3, 2);
    b.rect(14, 3, 2, 24, 3);
    b.rect(3, 14, 24, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(32, 24);
    b.rect(2, 6, 28, 16, 1);
    b.poly({{2, 6}, {8, 2}, {30, 2}, {24, 6}}, 2);
    b.rect(6, 10, 20, 2, 3);
    b.line(8, 6, 8, 22, 4, 1.2f);
    b.line(24, 6, 24, 22, 4, 1.2f);
    b.outline(5, false);
    return b;
}

gs::Bitmap rampartArt() {
    gs::Bitmap b(240, 70);
    b.poly({{0, 68}, {18, 22}, {222, 22}, {240, 68}}, 1);
    b.poly({{18, 22}, {120, 8}, {222, 22}, {120, 30}}, 2);
    b.rect(24, 28, 192, 28, 3);
    for (int x = 30; x < 210; x += 28) b.rect(float(x), 22.f, 10.f, 16.f, 4);
    b.rect(100, 36, 40, 18, 6);
    b.rect(20, 56, 200, 8, 5);
    b.outline(7, false);
    return b;
}

gs::Bitmap merlonArt() {
    gs::Bitmap b(18, 22);
    b.rect(2, 4, 14, 16, 1);
    b.rect(2, 2, 14, 4, 2);
    b.rect(4, 8, 4, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(28, 36);
    b.rect(4, 2, 2, 32, 2);
    b.poly({{6, 4}, {26, 8}, {24, 16}, {6, 14}}, 1);
    b.rect(10, 6, 8, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap embrasureArt() {
    gs::Bitmap b(36, 12);
    b.poly({{0, 2}, {36, 2}, {30, 10}, {6, 10}}, 1);
    b.rect(10, 4, 16, 4, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap revetArt() {
    gs::Bitmap b(16, 40);
    b.rect(4, 2, 8, 36, 1);
    for (int y = 6; y < 36; y += 6) b.rect(4, float(y), 8, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(22, 8);
    b.ellipse(11, 4, 10, 3, 1);
    return b;
}

gs::Bitmap scarArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 7, 3, 1);
    b.rect(6, 3, 6, 2, 2);
    return b;
}

void glyphs(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 96; i++) {
        gs::Bitmap g(8, 8);
        const uint8_t* rows = gs::glyph(char(32 + i));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (rows && rows[y * 5 + x]) g.set(x + 1, y, 1);
        art.glyph[i] = gs::uploadMipped(vdp, g);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 6)});
    setPal(vdp, PAL_EARTH,
           {0, gs::rgb4(7, 5, 2), gs::rgb4(9, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(11, 8, 4), gs::rgb4(4, 3, 2),
            gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_SAPPER,
           {0, gs::rgb4(5, 6, 3), gs::rgb4(3, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(13, 9, 6), gs::rgb4(9, 5, 3),
            gs::rgb4(12, 10, 4), gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(6, 3, 1), gs::rgb4(11, 8, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(7, 8, 8), gs::rgb4(4, 5, 5), gs::rgb4(11, 11, 9), gs::rgb4(13, 10, 4), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 3, 2), gs::rgb4(6, 4, 2), gs::rgb4(15, 12, 4), gs::rgb4(8, 1, 1)});
    setPal(vdp, PAL_WICKER,
           {0, gs::rgb4(9, 7, 3), gs::rgb4(6, 5, 2), gs::rgb4(12, 10, 5), gs::rgb4(4, 3, 1), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_POWDER,
           {0, gs::rgb4(6, 5, 4), gs::rgb4(4, 3, 3), gs::rgb4(9, 8, 6), gs::rgb4(12, 4, 2), gs::rgb4(14, 12, 6),
            gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 9, 5)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(8, 6, 3)});

    art.sapper[0] = gs::uploadMipped(vdp, sapperArt(0));
    art.sapper[1] = gs::uploadMipped(vdp, sapperArt(1));
    art.mattock = gs::uploadMipped(vdp, mattockArt());
    art.spoil = gs::uploadMipped(vdp, spoilArt());
    art.fascine = gs::uploadMipped(vdp, fascineArt());
    art.gabion = gs::uploadMipped(vdp, gabionArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.keg = gs::uploadMipped(vdp, kegArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.rampart = gs::uploadMipped(vdp, rampartArt());
    art.merlon = gs::uploadMipped(vdp, merlonArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.embrasure = gs::uploadMipped(vdp, embrasureArt());
    art.revet = gs::uploadMipped(vdp, revetArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.scar = gs::uploadMipped(vdp, scarArt());
    glyphs(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 3, 2));
}

}  // namespace rcler
