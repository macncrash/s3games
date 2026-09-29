#include "art.h"

#include <cstdint>
#include <initializer_list>

namespace trench {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void paintField(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, 2);
    b.rect(0, 0, 320, 36, 3);
    b.rect(0, 34, 320, 3, 4);
    for (int y = 40; y < 150; y += 3)
        for (int x = 4; x < 316; x += 5) {
            uint32_t h = hash2(x, y);
            if ((h % 17) == 0) b.set(x, y, (h & 1) ? 5 : 6);
            if ((h % 41) == 0) b.set(x + 1, y + 1, 7);
        }
    for (int i = 0; i < 9; i++) {
        int x = 18 + i * 34;
        int y = 52 + (i % 3) * 22;
        b.ellipse(float(x), float(y), 10, 5, 8);
        b.ellipse(float(x) - 2, float(y) - 1, 5, 2, 9);
    }
    for (int x = 8; x < 312; x += 8) {
        int sag = ((x / 8) % 5) - 2;
        b.set(x, 118 + sag, 10);
        b.set(x + 1, 119 + sag, 11);
        if ((x / 8) % 6 == 0) b.line(float(x), 112, float(x), 124, 11, 1.f);
    }
    b.rect(0, 148, 320, 10, 12);
    b.rect(0, 156, 320, 40, 1);
    b.rect(0, 156, 320, 4, 13);
    for (int x = 0; x < 320; x += 16) {
        b.rect(x, 168, 14, 6, 14);
        b.rect(x + 2, 174, 10, 3, 15);
    }
    b.rect(0, 196, 320, 28, 5);
    b.rect(0, 196, 320, 3, 8);
    b.rect(148, 132, 24, 28, 13);
    b.rect(146, 128, 28, 6, 14);
    b.ellipse(160, 146, 8, 4, 9);
    b.rect(158, 112, 4, 20, 11);
    b.rect(150, 108, 20, 4, 11);
    b.rect(152, 100, 3, 10, 10);
    b.rect(165, 100, 3, 10, 10);
    b.ellipse(160, 118, 4, 3, 15);
    gs::Bitmap tag = gs::textBitmap("TRENCH", {1, 4, 0, 0, 1});
    b.blit(tag, 8, 8);
}

gs::Bitmap soldier(int kind, int step) {
    gs::Bitmap b(28, 40);
    b.ellipse(12, 8, 6, 5, 4);
    b.rect(7, 6, 10, 3, 5);
    b.rect(6, 14, 12, 14, kind == 0 ? 1 : (kind == 1 ? 2 : 6));
    b.rect(6, 14, 4, 14, 3);
    b.rect(9, 16, 4, 3, 8);
    if (kind == 2) b.ellipse(18, 20, 5, 4, 7);
    b.rect(16, 16, 10, 2, 9);
    int lx = step ? 6 : 9;
    int rx = step ? 16 : 13;
    b.rect(lx, 28, 4, 8, 1);
    b.rect(rx, 28, 4, 8, 3);
    b.rect(lx - 1, 35, 6, 3, 10);
    b.rect(rx - 1, 35, 6, 3, 10);
    b.outline(11, false);
    return b;
}

gs::Bitmap rifleman(int step) {
    gs::Bitmap b(32, 40);
    b.ellipse(14, 8, 6, 5, 4);
    b.rect(9, 5, 11, 3, 5);
    b.rect(8, 14, 12, 14, 1);
    b.rect(8, 14, 4, 14, 2);
    b.rect(11, 16, 4, 3, 8);
    b.rect(18, 15, 12, 2, 6);
    b.rect(28, 13, 3, 4, 9);
    b.rect(10, 28, 4, 8, 1);
    b.rect(16 + step, 28, 4, 8, 2);
    b.rect(9, 35, 6, 3, 10);
    b.rect(16, 35, 6, 3, 10);
    b.outline(11, false);
    return b;
}

gs::Bitmap wellHead() {
    gs::Bitmap b(48, 64);
    b.rect(8, 28, 32, 28, 1);
    b.rect(6, 24, 36, 6, 2);
    b.rect(10, 32, 28, 18, 3);
    b.ellipse(24, 40, 8, 5, 4);
    b.ellipse(22, 38, 3, 2, 5);
    for (int i = 0; i < 5; i++) b.rect(10 + i * 6, 50, 4, 4, i & 1 ? 6 : 7);
    b.rect(22, 6, 4, 22, 8);
    b.rect(12, 4, 24, 4, 8);
    b.rect(14, 0, 3, 8, 9);
    b.rect(31, 0, 3, 8, 9);
    b.ellipse(24, 16, 4, 3, 10);
    b.line(24, 18, 24, 34, 9, 1.f);
    b.outline(11, false);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(24, 20);
    b.line(4, 2, 12, 10, 1, 1.5f);
    b.line(12, 10, 8, 18, 1, 1.5f);
    b.line(12, 10, 20, 6, 2, 1.f);
    return b;
}

gs::Bitmap shotArt() {
    gs::Bitmap b(6, 8);
    b.rect(2, 0, 2, 6, 1);
    b.set(2, 6, 2);
    b.set(3, 7, 3);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2), gs::rgb4(9, 8, 6),
                          gs::rgb4(15, 14, 8), gs::rgb4(8, 10, 6), gs::rgb4(4, 4, 3), gs::rgb4(12, 8, 4),
                          gs::rgb4(3, 3, 2), gs::rgb4(7, 6, 4), gs::rgb4(1, 1, 1), gs::rgb4(10, 9, 7),
                          gs::rgb4(5, 4, 3), gs::rgb4(13, 12, 8), gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_FIELD, {0, gs::rgb4(5, 4, 2), gs::rgb4(6, 7, 8), gs::rgb4(4, 5, 7), gs::rgb4(8, 8, 6),
                            gs::rgb4(4, 3, 2), gs::rgb4(7, 6, 3), gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 1),
                            gs::rgb4(1, 1, 1), gs::rgb4(9, 8, 6), gs::rgb4(3, 2, 1), gs::rgb4(6, 5, 3),
                            gs::rgb4(8, 7, 4), gs::rgb4(10, 8, 4), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(4, 6, 3), gs::rgb4(3, 4, 2), gs::rgb4(2, 3, 2), gs::rgb4(12, 9, 6),
                          gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(2, 2, 1), gs::rgb4(14, 12, 8),
                          gs::rgb4(15, 14, 6), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FOE, {0, gs::rgb4(6, 5, 4), gs::rgb4(8, 3, 2), gs::rgb4(4, 3, 3), gs::rgb4(11, 8, 6),
                          gs::rgb4(5, 4, 3), gs::rgb4(9, 4, 3), gs::rgb4(3, 2, 2), gs::rgb4(13, 11, 8),
                          gs::rgb4(10, 9, 7), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SAP, {0, gs::rgb4(5, 5, 4), gs::rgb4(3, 5, 6), gs::rgb4(3, 3, 3), gs::rgb4(10, 8, 6),
                          gs::rgb4(4, 4, 3), gs::rgb4(4, 7, 8), gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 7),
                          gs::rgb4(9, 8, 6), gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHELL, {0, gs::rgb4(7, 6, 3), gs::rgb4(9, 7, 2), gs::rgb4(4, 3, 2), gs::rgb4(12, 9, 5),
                            gs::rgb4(5, 4, 2), gs::rgb4(11, 8, 3), gs::rgb4(14, 10, 3), gs::rgb4(13, 11, 7),
                            gs::rgb4(8, 6, 2), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WELL, {0, gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5), gs::rgb4(4, 5, 6), gs::rgb4(2, 4, 6),
                           gs::rgb4(10, 12, 13), gs::rgb4(5, 5, 4), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 2),
                           gs::rgb4(9, 7, 4), gs::rgb4(2, 2, 1), gs::rgb4(11, 9, 5), gs::rgb4(1, 1, 1), 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 3), gs::rgb4(12, 6, 2), gs::rgb4(8, 8, 7), 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_OK, {0, gs::rgb4(8, 14, 6), gs::rgb4(3, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    gs::Bitmap field(320, 224);
    paintField(field);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, field, PAL_FIELD);
    vdp.A.enabled = false;
    vdp.B.enabled = true;

    art.rifle[0] = gs::uploadMipped(vdp, rifleman(0));
    art.rifle[1] = gs::uploadMipped(vdp, rifleman(1));
    art.raider[0] = gs::uploadMipped(vdp, soldier(0, 0));
    art.raider[1] = gs::uploadMipped(vdp, soldier(0, 1));
    art.sapper[0] = gs::uploadMipped(vdp, soldier(1, 0));
    art.sapper[1] = gs::uploadMipped(vdp, soldier(1, 1));
    art.shell[0] = gs::uploadMipped(vdp, soldier(2, 0));
    art.shell[1] = gs::uploadMipped(vdp, soldier(2, 1));
    art.well = gs::uploadMipped(vdp, wellHead());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.shot = gs::uploadMipped(vdp, shotArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
}

}  // namespace trench
