#include "game/art.h"

#include <initializer_list>
#include <string>

namespace sally {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 2);
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

void stoneTile(gs::VDP& vdp) {
    uint8_t px[64];
    for (int i = 0; i < 64; i++) px[i] = ((i / 8) & 1) ? 2 : 3;
    px[0] = 1;
    px[11] = 4;
    px[22] = 1;
    px[37] = 4;
    px[50] = 5;
    px[63] = 1;
    vdp.loadTile(1, px);
}

gs::Bitmap leafArt() {
    gs::Bitmap b(40, 100);
    b.rect(3, 4, 34, 92, 2);
    b.rect(3, 4, 6, 92, 1);
    b.rect(31, 4, 6, 92, 3);
    for (int y = 12; y < 90; y += 16) b.rect(8, float(y), 24, 3, 4);
    for (int i = 0; i < 5; i++) b.rect(float(10 + i * 4), 18, 2, 64, 5);
    b.rect(16, 40, 8, 14, 6);
    b.rect(18, 44, 4, 6, 7);
    b.rect(8, 84, 24, 6, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(18, 118);
    b.rect(2, 2, 14, 114, 2);
    b.rect(2, 2, 4, 114, 1);
    b.rect(12, 2, 4, 114, 3);
    for (int y = 8; y < 110; y += 18) b.rect(3, float(y), 12, 4, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(128, 22);
    b.rect(2, 8, 124, 12, 2);
    b.rect(2, 6, 124, 4, 1);
    b.rect(2, 16, 124, 4, 3);
    b.ellipse(64, 20, 40, 10, 0);
    for (int i = 0; i < 9; i++) b.rect(float(10 + i * 12), 10, 4, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(8, 48);
    b.rect(2, 1, 4, 46, 1);
    b.rect(3, 1, 2, 46, 2);
    for (int y = 4; y < 44; y += 8) b.rect(1, float(y), 6, 2, 3);
    return b;
}

gs::Bitmap chainArt() {
    gs::Bitmap b(12, 36);
    for (int y = 2; y < 30; y += 7) {
        b.ellipse(6, float(y + 2), 4, 3, 1);
        b.ellipse(6, float(y + 2), 2, 1, 0);
    }
    b.rect(2, 30, 8, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap grateArt() {
    gs::Bitmap b(36, 10);
    b.rect(1, 2, 34, 6, 2);
    for (int x = 4; x < 32; x += 6) b.rect(float(x), 1, 2, 8, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap helmArt(int frame) {
    gs::Bitmap b(28, 50);
    b.ellipse(14, 8, 7, 5, 4);
    b.rect(8, 8, 12, 6, 3);
    b.rect(10, 10, 8, 3, 1);
    b.set(11, 11, 8);
    b.set(16, 11, 8);
    b.rect(10, 14, 8, 12, 2);
    b.rect(9, 24, 10, 3, 6);
    if (frame == 0) {
        b.line(10, 18, 3, 32, 2, 3.0f);
        b.rect(7, 32, 6, 12, 5);
        b.rect(16, 32, 6, 12, 3);
    } else {
        b.line(9, 18, 2, 22, 2, 3.0f);
        b.line(19, 18, 26, 26, 2, 3.0f);
        b.rect(8, 32, 5, 12, 5);
        b.rect(16, 32, 5, 12, 3);
    }
    b.rect(6, 42, 7, 5, 7);
    b.rect(16, 42, 7, 5, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap ramArt(int frame) {
    gs::Bitmap b(30, 48);
    b.ellipse(15, 7, 6, 4, 4);
    b.rect(10, 10, 10, 12, 1);
    b.rect(9, 20, 12, 3, 6);
    float arm = frame ? 28.f : 18.f;
    b.line(10, 16, 2, arm, 1, 3.0f);
    b.line(20, 16, 28, arm, 1, 3.0f);
    b.rect(8, 28, 6, 14, 3);
    b.rect(16, 28, 6, 14, 2);
    b.rect(7, 40, 7, 5, 5);
    b.rect(16, 40, 7, 5, 5);
    b.rect(2, float(arm - 2), 26, 4, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap torchArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 12, 3, 14, 4);
    b.ellipse(6, 8, 4, 6, 1);
    b.ellipse(6, 7, 2, 3, 2);
    b.outline(15, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(12, 8, 5, 5, 0);
    b.set(6, 7, 2);
    return b;
}

gs::Bitmap wedgeArt() {
    gs::Bitmap b(22, 8);
    b.rect(1, 2, 20, 4, 3);
    b.rect(1, 2, 4, 4, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(6, 6);
    b.rect(2, 0, 2, 6, 1);
    b.rect(0, 2, 6, 2, 2);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(16, 28);
    b.rect(2, 2, 12, 18, 1);
    b.rect(4, 6, 8, 3, 2);
    b.rect(6, 10, 4, 6, 3);
    b.poly({{2, 20}, {8, 26}, {14, 20}}, 1);
    b.outline(15, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_TORCH, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 7, 1), gs::rgb4(8, 3, 1), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 8), gs::rgb4(2, 8, 4), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 9, 8), gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(12, 11, 9), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_MAIL, {0, gs::rgb4(9, 10, 12), gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(13, 12, 8),
                           gs::rgb4(6, 5, 3), gs::rgb4(12, 8, 4), gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(11, 12, 13), gs::rgb4(6, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 6),
                           gs::rgb4(2, 2, 3), gs::rgb4(14, 10, 3), gs::rgb4(4, 3, 1)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(12, 3, 3), gs::rgb4(15, 13, 6), gs::rgb4(6, 1, 1), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(12, 8, 3), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(3, 4, 8), gs::rgb4(1, 1, 3), gs::rgb4(6, 7, 10)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(2, 2, 6), gs::rgb4(4, 4, 8)});
    setPal(vdp, PAL_MIST, {0, gs::rgb4(5, 5, 7)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 4), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 3),
                           gs::rgb4(4, 4, 3), gs::rgb4(1, 1, 1), gs::rgb4(7, 6, 4)});
    stoneTile(vdp);
    loadFont(vdp, art);
    art.leaf = gs::uploadMipped(vdp, leafArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.grate = gs::uploadMipped(vdp, grateArt());
    art.helm[0] = gs::uploadMipped(vdp, helmArt(0));
    art.helm[1] = gs::uploadMipped(vdp, helmArt(1));
    art.ram[0] = gs::uploadMipped(vdp, ramArt(0));
    art.ram[1] = gs::uploadMipped(vdp, ramArt(1));
    art.torch = gs::uploadMipped(vdp, torchArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.wedge = gs::uploadMipped(vdp, wedgeArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
}

}  // namespace sally
