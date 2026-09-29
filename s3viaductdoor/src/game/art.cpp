#include "game/art.h"

#include <initializer_list>
#include <string>

namespace viaduct {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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
    uint8_t course[64] = {};
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 2;
            if (y == 0 || y == 7) c = 4;
            else if ((x + (y > 3 ? 4 : 0)) % 8 == 0) c = 3;
            else if (y < 3) c = 1;
            course[y * 8 + x] = uint8_t(c);
        }
    a.course = tiles.alloc(1);
    vdp.loadTile(a.course, course);
}

gs::Bitmap slabArt() {
    gs::Bitmap b(48, 96);
    b.rect(2, 2, 44, 92, 2);
    b.rect(2, 2, 8, 92, 1);
    b.rect(38, 2, 8, 92, 3);
    for (int y = 8; y < 90; y += 12) b.rect(8, float(y), 32, 2, 4);
    for (int y = 16; y < 80; y += 22) {
        b.rect(18, float(y), 12, 10, 5);
        b.rect(21, float(y + 3), 6, 4, 6);
    }
    b.rect(6, 86, 36, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(28, 140);
    b.poly({{4, 8}, {24, 8}, {26, 136}, {2, 136}}, 2);
    b.rect(6, 10, 6, 122, 1);
    b.rect(18, 10, 6, 122, 3);
    for (int y = 18; y < 128; y += 16) b.rect(6, float(y), 16, 3, 4);
    b.rect(8, 4, 12, 8, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap crownArt() {
    gs::Bitmap b(160, 22);
    b.poly({{4, 18}, {20, 4}, {140, 4}, {156, 18}}, 2);
    b.rect(8, 12, 144, 6, 1);
    for (int i = 0; i < 12; i++) b.rect(float(14 + i * 11), 8, 4, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap wardenArt(int frame) {
    gs::Bitmap b(26, 46);
    b.ellipse(13, 6, 5, 3, 6);
    b.ellipse(13, 12, 5, 5, 4);
    b.set(11, 12, 8);
    b.set(15, 12, 8);
    b.rect(9, 16, 8, 12, 1);
    b.rect(8, 26, 10, 3, 5);
    if (frame == 0) {
        b.line(9, 18, 3, 30, 1, 2.6f);
        b.rect(6, 30, 5, 10, 2);
        b.rect(15, 30, 5, 10, 3);
    } else {
        b.line(9, 18, 2, 22, 1, 2.6f);
        b.line(17, 18, 24, 26, 1, 2.6f);
        b.rect(7, 30, 5, 10, 2);
        b.rect(15, 30, 5, 10, 3);
    }
    b.rect(5, 40, 7, 4, 7);
    b.rect(15, 40, 7, 4, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap gustArt(int frame) {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 6, 5, 3, 3);
    b.ellipse(12, 12, 5, 5, 2);
    b.rect(8, 16, 8, 10, 1);
    float arm = frame ? 28.f : 20.f;
    b.line(8, 18, 2, arm, 1, 2.4f);
    b.line(16, 18, 22, arm, 1, 2.4f);
    b.rect(7, 26, 4, 10, 4);
    b.rect(13, 26, 4, 10, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap hitchArt() {
    gs::Bitmap b(12, 36);
    for (int y = 2; y < 28; y += 6) {
        b.rect(3, float(y), 6, 4, 1);
        b.rect(4, float(y + 1), 4, 2, 2);
    }
    b.ellipse(6, 30, 5, 4, 3);
    b.ellipse(6, 30, 2, 2, 0);
    b.outline(15, false);
    return b;
}

gs::Bitmap chockArt() {
    gs::Bitmap b(22, 12);
    b.poly({{2, 10}, {20, 10}, {16, 2}, {6, 2}}, 1);
    b.rect(7, 5, 8, 3, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 28);
    b.rect(3, 2, 2, 24, 3);
    b.poly({{5, 4}, {20, 8}, {5, 14}}, 1);
    b.poly({{6, 6}, {16, 8}, {6, 12}}, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 24);
    b.rect(6, 1, 2, 6, 3);
    b.poly({{2, 8}, {12, 8}, {10, 16}, {4, 16}}, 1);
    b.rect(4, 10, 6, 4, 2);
    b.rect(5, 16, 4, 6, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap rivetArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap moteArt() {
    gs::Bitmap b(4, 6);
    b.rect(1, 0, 2, 6, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 6, 8), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_DUSK, {0, gs::rgb4(12, 8, 5), gs::rgb4(6, 4, 6), gs::rgb4(3, 2, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(10, 2, 2), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 8), gs::rgb4(3, 8, 5), gs::rgb4(14, 14, 8)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(11, 10, 8), gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(13, 12, 9),
                           gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(4, 6, 9), gs::rgb4(8, 5, 3), gs::rgb4(3, 3, 4), gs::rgb4(12, 9, 7),
                           gs::rgb4(6, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(9, 7, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(9, 10, 11), gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(12, 12, 10),
                           gs::rgb4(4, 5, 6), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 8, 2), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(12, 3, 3), gs::rgb4(15, 10, 4), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_GORGE, {0, gs::rgb4(2, 4, 7), gs::rgb4(1, 2, 4), gs::rgb4(4, 6, 8)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(3, 3, 6), gs::rgb4(6, 4, 5), gs::rgb4(10, 6, 4)});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(14, 13, 8), gs::rgb4(8, 8, 6)});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(5, 5, 5), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(12, 10, 4),
                           gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 2), gs::rgb4(9, 9, 8), gs::rgb4(4, 4, 4),
                           gs::rgb4(7, 6, 5), gs::rgb4(10, 9, 7)});
    loadFont(vdp, art);
    art.slab = gs::uploadMipped(vdp, slabArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.crown = gs::uploadMipped(vdp, crownArt());
    art.warden[0] = gs::uploadMipped(vdp, wardenArt(0));
    art.warden[1] = gs::uploadMipped(vdp, wardenArt(1));
    art.gust[0] = gs::uploadMipped(vdp, gustArt(0));
    art.gust[1] = gs::uploadMipped(vdp, gustArt(1));
    art.hitch = gs::uploadMipped(vdp, hitchArt());
    art.chock = gs::uploadMipped(vdp, chockArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.rivet = gs::uploadMipped(vdp, rivetArt());
    art.mote = gs::uploadMipped(vdp, moteArt());
}

}  // namespace viaduct
