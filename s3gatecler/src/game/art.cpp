#include "game/art.h"

#include <initializer_list>
#include <string>

namespace cler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap worker(int step) {
    gs::Bitmap b(48, 76);
    b.ellipse(22, 12, 9, 4, 12);
    b.rect(14, 12, 16, 4, 5);
    b.ellipse(22, 18, 7, 7, 4);
    b.set(19, 18, 11);
    b.set(25, 18, 11);
    b.rect(18, 22, 8, 3, 10);
    b.poly({{12, 26}, {32, 26}, {36, 50}, {8, 50}}, 1);
    b.poly({{12, 26}, {22, 26}, {20, 50}, {8, 50}}, 2);
    b.rect(19, 28, 6, 8, 9);
    b.line(30, 32, 42, 46, 6, 3.f);
    b.rect(36, 44, 10, 3, 7);
    b.rect(37, 47, 2, 7, 7);
    b.rect(40, 47, 2, 7, 7);
    b.rect(43, 47, 2, 7, 7);
    if (step == 0) {
        b.rect(14, 50, 7, 16, 3);
        b.rect(25, 50, 7, 13, 3);
        b.rect(12, 64, 10, 5, 8);
        b.rect(24, 61, 10, 5, 8);
    } else {
        b.rect(14, 50, 7, 13, 3);
        b.rect(25, 50, 7, 16, 3);
        b.rect(13, 61, 10, 5, 8);
        b.rect(23, 64, 10, 5, 8);
    }
    b.outline(11, false);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(52, 28);
    b.poly({{6, 8}, {34, 8}, {38, 18}, {4, 18}}, 1);
    b.rect(8, 9, 22, 6, 7);
    b.ellipse(12, 20, 6, 6, 4);
    b.ellipse(12, 20, 2, 2, 3);
    b.line(32, 12, 48, 4, 3, 2.5f);
    b.rect(44, 2, 5, 4, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap loadArt() {
    gs::Bitmap b(28, 18);
    b.ellipse(14, 11, 12, 6, 4);
    b.ellipse(10, 9, 6, 4, 1);
    b.ellipse(18, 10, 5, 3, 2);
    b.rect(8, 8, 3, 6, 3);
    b.rect(16, 6, 2, 5, 3);
    return b;
}

gs::Bitmap brushArt() {
    gs::Bitmap b(44, 32);
    b.ellipse(22, 20, 16, 8, 5);
    b.ellipse(14, 16, 9, 6, 1);
    b.ellipse(28, 15, 8, 6, 2);
    b.line(8, 22, 18, 10, 3, 2.f);
    b.line(16, 24, 26, 8, 3, 2.f);
    b.line(24, 22, 34, 9, 3, 2.f);
    b.ellipse(12, 12, 4, 3, 1);
    b.ellipse(30, 11, 4, 3, 2);
    b.outline(8, false);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(40, 30);
    b.ellipse(14, 18, 10, 8, 1);
    b.ellipse(26, 16, 11, 8, 2);
    b.ellipse(20, 12, 7, 5, 3);
    b.rect(10, 14, 6, 3, 4);
    b.rect(24, 13, 5, 2, 4);
    b.outline(9, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 32);
    b.poly({{6, 10}, {30, 8}, {32, 26}, {4, 28}}, 1);
    b.poly({{6, 10}, {18, 8}, {17, 26}, {4, 28}}, 2);
    b.line(6, 16, 31, 14, 3, 2.f);
    b.line(6, 22, 31, 20, 3, 2.f);
    b.line(18, 9, 18, 27, 3, 2.f);
    b.rect(8, 6, 8, 4, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap dirtArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(20, 9, 18, 6, 5);
    b.ellipse(16, 8, 10, 4, 4);
    return b;
}

gs::Bitmap sweptArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(20, 9, 16, 5, 3);
    b.ellipse(15, 8, 8, 3, 4);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(44, 140);
    b.rect(6, 16, 32, 120, 2);
    b.rect(8, 18, 14, 116, 1);
    for (int y = 28; y < 128; y += 16) b.rect(6, y, 32, 3, 4);
    b.rect(2, 8, 40, 10, 1);
    b.rect(0, 4, 44, 6, 2);
    b.rect(14, 48, 8, 18, 6);
    b.rect(16, 50, 4, 14, 3);
    b.rect(10, 90, 8, 10, 5);
    b.rect(24, 108, 7, 8, 5);
    b.outline(9, false);
    return b;
}

gs::Bitmap lintelArt() {
    gs::Bitmap b(240, 30);
    b.rect(4, 8, 232, 16, 2);
    b.rect(4, 8, 232, 5, 1);
    for (int x = 16; x < 228; x += 28) b.rect(x, 8, 3, 16, 4);
    b.rect(0, 4, 240, 5, 1);
    b.rect(8, 22, 224, 4, 3);
    b.outline(9, false);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(32, 40);
    b.poly({{8, 6}, {24, 6}, {28, 36}, {4, 36}}, 2);
    b.poly({{10, 8}, {18, 8}, {16, 34}, {6, 34}}, 1);
    b.rect(14, 16, 5, 10, 6);
    b.outline(9, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 28);
    b.rect(7, 0, 2, 6, 3);
    b.poly({{3, 8}, {13, 8}, {14, 18}, {2, 18}}, 1);
    b.ellipse(8, 13, 3, 3, 2);
    b.rect(5, 18, 6, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap cribArt() {
    gs::Bitmap b(72, 52);
    b.rect(6, 16, 6, 28, 3);
    b.rect(60, 16, 6, 28, 3);
    b.rect(4, 12, 64, 8, 1);
    b.rect(6, 13, 60, 3, 2);
    for (int y = 24; y <= 40; y += 8) {
        b.rect(10, y, 52, 5, 1);
        b.rect(10, y, 52, 2, 2);
    }
    b.rect(8, 22, 56, 18, 7);
    b.rect(4, 42, 10, 6, 4);
    b.rect(58, 42, 10, 6, 4);
    gs::Bitmap label = gs::textBitmap("CRIB", {2, 6, 0, 0, 1});
    b.blit(label, 14, 0);
    b.outline(5, false);
    return b;
}

gs::Bitmap rubbleArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(11, 10, 9, 5, 4);
    b.ellipse(8, 8, 4, 3, 1);
    b.ellipse(15, 8, 4, 3, 6);
    b.rect(6, 6, 2, 5, 3);
    return b;
}

gs::Bitmap doorArt(bool open) {
    gs::Bitmap b(180, 100);
    b.rect(8, 28, 164, 70, open ? 7 : 1);
    b.ellipse(90, 32, 78, 26, open ? 7 : 1);
    b.rect(28, 40, 124, 52, open ? 8 : 2);
    b.ellipse(90, 44, 52, 18, open ? 8 : 2);
    if (!open) {
        b.rect(86, 48, 8, 40, 3);
        b.rect(70, 62, 10, 8, 4);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(48, 70);
    b.rect(21, 40, 7, 26, 3);
    b.rect(23, 42, 3, 20, 4);
    b.ellipse(24, 28, 18, 16, 1);
    b.ellipse(16, 24, 10, 8, 2);
    b.ellipse(32, 30, 9, 7, 2);
    b.outline(5, false);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(24, 24);
    b.ellipse(12, 12, 9, 9, 1);
    b.ellipse(12, 12, 5, 5, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(72, 26);
    b.ellipse(22, 16, 16, 7, 3);
    b.ellipse(40, 13, 18, 8, 4);
    b.ellipse(56, 16, 12, 6, 3);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(20, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(7, 6, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(16, 12);
    b.poly({{2, 2}, {8, 10}, {14, 2}, {11, 2}, {8, 6}, {5, 2}}, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 0, 1};
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 10, 12), gs::rgb4(4, 4, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 11), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 9), gs::rgb4(5, 1, 1), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(2, 4, 2), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(11, 10, 9), gs::rgb4(8, 7, 7), gs::rgb4(5, 5, 6), gs::rgb4(6, 6, 5), gs::rgb4(3, 6, 3),
            gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 9), 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_FIGURE,
           {0, gs::rgb4(13, 8, 3), gs::rgb4(9, 5, 2), gs::rgb4(4, 4, 6), gs::rgb4(14, 11, 8), gs::rgb4(3, 2, 1),
            gs::rgb4(10, 7, 3), gs::rgb4(12, 13, 13), gs::rgb4(2, 2, 2), gs::rgb4(6, 7, 5), gs::rgb4(8, 3, 3),
            gs::rgb4(1, 1, 1), gs::rgb4(5, 6, 3)});
    setPal(vdp, PAL_EARTH,
           {0, gs::rgb4(5, 9, 3), gs::rgb4(3, 6, 2), gs::rgb4(7, 5, 2), gs::rgb4(8, 6, 3), gs::rgb4(5, 4, 2),
            gs::rgb4(10, 9, 8), gs::rgb4(6, 6, 5), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_CRATE,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 5, 6), gs::rgb4(3, 2, 2), gs::rgb4(2, 1, 1),
            gs::rgb4(15, 13, 8), gs::rgb4(14, 11, 7)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 7, 2), gs::rgb4(5, 10, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 1), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(10, 9, 7), gs::rgb4(13, 12, 9), gs::rgb4(11, 10, 8), gs::rgb4(13, 12, 10)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(1, 1, 3), gs::rgb4(0, 0, 2), gs::rgb4(8, 8, 11), gs::rgb4(12, 12, 14), gs::rgb4(14, 13, 9),
            gs::rgb4(10, 9, 6), gs::rgb4(14, 10, 4), gs::rgb4(15, 13, 7), gs::rgb4(1, 1, 2)});

    const uint16_t yard[16] = {
        0,
        gs::rgb4(3, 6, 2), gs::rgb4(2, 4, 1), gs::rgb4(4, 6, 3),
        gs::rgb4(5, 5, 3), gs::rgb4(3, 4, 2),
        gs::rgb4(9, 8, 6), gs::rgb4(7, 6, 5),
        gs::rgb4(11, 10, 8), gs::rgb4(8, 7, 6), gs::rgb4(6, 5, 4),
        gs::rgb4(2, 3, 2), gs::rgb4(1, 2, 1), gs::rgb4(3, 3, 2),
        gs::rgb4(10, 9, 7), gs::rgb4(6, 5, 4),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    loadFont(vdp, art);
    art.worker[0] = gs::uploadMipped(vdp, worker(0));
    art.worker[1] = gs::uploadMipped(vdp, worker(1));
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.load = gs::uploadMipped(vdp, loadArt());
    art.brush = gs::uploadMipped(vdp, brushArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.dirt = gs::uploadMipped(vdp, dirtArt());
    art.swept = gs::uploadMipped(vdp, sweptArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.lintel = gs::uploadMipped(vdp, lintelArt());
    art.keystone = gs::uploadMipped(vdp, keyArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.crib = gs::uploadMipped(vdp, cribArt());
    art.rubble = gs::uploadMipped(vdp, rubbleArt());
    art.door = gs::uploadMipped(vdp, doorArt(false));
    art.dawn = gs::uploadMipped(vdp, doorArt(true));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(6, 4, 5));
}

}  // namespace cler
