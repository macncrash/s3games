#include "game/art.h"

#include <cmath>
#include <string>

namespace board {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void inkPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t alt) {
    uint16_t c[16] = {};
    c[1] = ink;
    c[2] = alt;
    c[15] = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, c);
}

void callPalette(gs::VDP& vdp, int pal, uint16_t hi, uint16_t mid, uint16_t deep) {
    uint16_t c[16] = {};
    c[1] = gs::rgb4(15, 15, 15);
    c[2] = hi;
    c[3] = mid;
    c[4] = deep;
    c[5] = gs::rgb4(13, 10, 3);
    c[6] = gs::rgb4(2, 2, 3);
    c[15] = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, c);
}

gs::Bitmap ringArt() {
    gs::Bitmap b(28, 28);
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 28; x++) {
            float d = std::hypot(x - 13.5f, y - 13.5f);
            if (d > 9.2f && d < 12.2f) b.set(x, y, 1);
        }
    }
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 5);
    b.ellipse(11, 11, 7.5f, 7.5f, 4);
    b.ellipse(11, 11, 5.5f, 5.5f, 3);
    b.ellipse(11, 11, 3.2f, 3.2f, 2);
    b.ellipse(8.5f, 8.2f, 1.6f, 1.4f, 1);
    return b;
}

gs::Bitmap badgePier() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 6, 4, 4, 4);
    b.ellipse(10, 6, 2, 2, 2);
    b.rect(9, 6, 2, 8, 2);
    b.line(4, 12, 16, 12, 2, 2);
    b.line(5, 14, 10, 18, 2, 2);
    b.line(15, 14, 10, 18, 2, 2);
    b.ellipse(10, 5, 1, 1, 1);
    return b;
}

gs::Bitmap badgeInn() {
    gs::Bitmap b(20, 20);
    b.poly({{2, 9}, {10, 2}, {18, 9}}, 4);
    b.poly({{4, 9}, {10, 4}, {16, 9}}, 2);
    b.rect(4, 9, 12, 8, 3);
    b.rect(5, 10, 10, 7, 2);
    b.rect(8, 12, 4, 5, 4);
    b.rect(5, 11, 3, 3, 1);
    b.rect(12, 11, 3, 3, 1);
    return b;
}

gs::Bitmap badgeFire() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 13, 6, 5, 4);
    b.ellipse(10, 12, 4.5f, 6, 3);
    b.ellipse(10, 10, 3, 5, 2);
    b.ellipse(10, 8, 1.6f, 3, 1);
    b.ellipse(8, 13, 1.4f, 2, 1);
    return b;
}

gs::Bitmap badgeCab() {
    gs::Bitmap b(20, 20);
    b.rect(2, 10, 16, 5, 4);
    b.rect(3, 11, 14, 3, 2);
    b.poly({{5, 10}, {7, 6}, {13, 6}, {15, 10}}, 3);
    b.rect(7, 7, 6, 3, 1);
    b.ellipse(6, 16, 2.4f, 2.4f, 4);
    b.ellipse(14, 16, 2.4f, 2.4f, 4);
    b.ellipse(6, 16, 1, 1, 1);
    b.ellipse(14, 16, 1, 1, 1);
    return b;
}

gs::Bitmap badgeHall() {
    gs::Bitmap b(20, 20);
    b.ellipse(7, 14, 3.5f, 2.4f, 4);
    b.ellipse(7, 14, 2, 1.3f, 2);
    b.rect(9, 4, 2, 10, 2);
    b.rect(9, 4, 7, 2, 2);
    b.line(16, 5, 16, 10, 3, 1.5f);
    b.ellipse(10, 5, 1, 1, 1);
    return b;
}

gs::Bitmap badgeWire() {
    gs::Bitmap b(20, 20);
    b.poly({{11, 1}, {4, 11}, {9, 11}, {7, 19}, {16, 8}, {11, 8}}, 4);
    b.poly({{11, 2}, {6, 10}, {10, 10}, {8, 16}, {14, 9}, {10, 9}}, 2);
    b.rect(10, 4, 2, 3, 1);
    return b;
}

gs::Bitmap plugArt() {
    gs::Bitmap b(16, 28);
    b.rect(6, 0, 4, 10, 2);
    b.rect(7, 0, 2, 10, 1);
    b.ellipse(8, 15, 7, 6, 3);
    b.ellipse(6, 13, 2, 1.6f, 4);
    b.rect(7, 20, 2, 6, 5);
    b.rect(6, 24, 4, 2, 5);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 3.2f, 2);
    b.ellipse(4, 4, 1.8f, 1.8f, 1);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(16, 4);
    b.rect(0, 0, 16, 4, 2);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(16, 16);
    b.line(8, 1, 8, 15, 1, 2);
    b.line(1, 8, 15, 8, 1, 2);
    b.line(3, 3, 13, 13, 1, 1.5f);
    b.line(13, 3, 3, 13, 1, 1.5f);
    return b;
}

gs::Bitmap operatorArt() {
    gs::Bitmap b(96, 112);
    b.rect(16, 78, 64, 10, 5);
    b.ellipse(48, 98, 40, 20, 4);
    b.ellipse(48, 94, 32, 14, 5);
    b.poly({{48, 68}, {32, 104}, {64, 104}}, 7);
    b.rect(41, 60, 14, 16, 1);
    b.rect(43, 62, 5, 14, 2);
    b.ellipse(48, 40, 20, 22, 3);
    b.ellipse(48, 46, 16, 16, 1);
    b.ellipse(30, 44, 7, 12, 3);
    b.ellipse(66, 42, 7, 12, 3);
    b.ellipse(70, 26, 8, 8, 3);
    b.ellipse(41, 46, 3.2f, 3.2f, 10);
    b.ellipse(55, 46, 3.2f, 3.2f, 10);
    b.ellipse(42, 46, 1.4f, 1.6f, 9);
    b.ellipse(56, 46, 1.4f, 1.6f, 9);
    b.line(36, 41, 46, 40, 3, 1.6f);
    b.line(50, 40, 60, 41, 3, 1.6f);
    b.line(48, 48, 47, 54, 2, 1.2f);
    b.ellipse(48, 58, 4, 2.2f, 8);
    b.line(44, 57, 52, 57, 1, 1);
    b.ellipse(36, 52, 3, 2, 11);
    b.ellipse(60, 52, 3, 2, 11);
    for (int i = 0; i <= 32; i++) {
        float a = 3.14159f + i * (3.14159f / 32.0f);
        float x = 48 + std::cos(a) * 22;
        float y = 42 + std::sin(a) * 24;
        b.rect(x, y, 2, 2, 6);
    }
    b.ellipse(27, 48, 6, 8, 6);
    b.ellipse(27, 48, 3, 5, 5);
    b.line(28, 54, 42, 60, 6, 2);
    b.ellipse(43, 60, 3, 2, 6);
    b.poly({{48, 66}, {39, 76}, {48, 72}}, 8);
    b.poly({{48, 66}, {57, 76}, {48, 72}}, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap cardArt() {
    gs::Bitmap b(216, 150);
    b.rect(0, 0, 216, 150, 8);
    b.rect(3, 3, 210, 144, 6);
    b.rect(7, 7, 202, 136, 3);
    b.rect(10, 10, 196, 130, 6);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
        art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void paintBoard(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, 3);
    b.rect(0, 0, 320, 44, 12);
    b.ellipse(176, 12, 8, 8, 14);
    b.ellipse(180, 11, 7, 7, 12);
    const int skyX[] = {0, 26, 48, 86, 104, 142, 164, 210, 228, 258, 276, 298};
    const int skyW[] = {26, 22, 38, 18, 38, 22, 46, 18, 30, 18, 22, 22};
    const int skyH[] = {20, 34, 16, 30, 18, 28, 14, 36, 20, 26, 16, 24};
    for (int i = 0; i < 12; i++) {
        int top = 44 - skyH[i];
        b.rect(float(skyX[i]), float(top), float(skyW[i]), float(skyH[i]), 13);
        for (int y = top + 3; y < 42; y += 5) {
            for (int x = skyX[i] + 2; x < skyX[i] + skyW[i] - 2; x += 4) {
                if (((x * 3 + y * 5) % 7) == 0) b.rect(float(x), float(y), 2, 2, 14);
            }
        }
    }
    for (int x = 0; x < 320; x += 40) b.rect(float(x), 0, 2, 44, 6);
    b.rect(0, 20, 320, 2, 6);

    b.rect(8, 40, 304, 172, 3);
    b.rect(14, 46, 292, 160, 4);
    b.rect(18, 50, 284, 8, 5);
    for (int y = 48; y < 206; y += 7) b.rect(10, float(y), 4, 1, 5);

    b.rect(float(TRUNK_X - 44), 72, 88, 136, 6);
    b.rect(float(LINE_X - 44), 72, 88, 136, 6);
    b.rect(float(TRUNK_X + 44), 72, float(LINE_X - TRUNK_X - 88), 136, 11);
    for (int y = 80; y < 200; y += 8) b.rect(float(TRUNK_X + 48), float(y), float(LINE_X - TRUNK_X - 96), 1, 6);

    for (int i = 0; i < JACKS; i++) {
        int y = jackY(i);
        b.ellipse(float(TRUNK_X), float(y), 11, 11, 8);
        b.ellipse(float(TRUNK_X), float(y), 7, 7, 7);
        b.ellipse(float(LINE_X), float(y), 11, 11, 8);
        b.ellipse(float(LINE_X), float(y), 7, 7, 7);
        char num[2] = {char('1' + i), 0};
        gs::Bitmap tn = gs::textBitmap(num, {1, 8, 0, 0, 1});
        b.blit(tn, TRUNK_X - 26, y - 3);
        gs::Bitmap ln = gs::textBitmap(lineName(i), {1, 8, 0, 0, 1});
        b.blit(ln, LINE_X + 16, y - 3);
    }

    gs::Bitmap plate = gs::textBitmap("CITY EXCHANGE", {1, 10, 0, 0, 1});
    b.rect(108, 52, float(plate.w + 10), 12, 8);
    b.rect(110, 54, float(plate.w + 6), 8, 9);
    b.blit(plate, 113, 55);

    b.rect(292, 46, 8, 5, 8);
    b.ellipse(296, 54, 7, 5, 8);
    b.ellipse(296, 54, 3, 2, 14);

    b.rect(0, 208, 320, 16, 3);
    b.rect(0, 214, 320, 10, 5);
    b.rect(16, 216, 40, 7, 9);
    b.line(20, 218, 52, 218, 10, 1);
    b.line(20, 220, 48, 220, 10, 1);
    b.rect(268, 215, 3, 8, 4);
    b.rect(266, 214, 7, 2, 8);
    b.ellipse(160, 218, 6, 5, 9);
    b.ellipse(160, 218, 4, 3.5f, 6);
    b.line(160, 218, 160, 215, 8, 1);
    b.line(160, 218, 163, 219, 8, 1);

    b.rect(float(TRUNK_X - 42), 74, 3, 3, 8);
    b.rect(float(TRUNK_X + 38), 74, 3, 3, 8);
    b.rect(float(LINE_X - 42), 74, 3, 3, 8);
    b.rect(float(LINE_X + 38), 74, 3, 3, 8);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    uint16_t bg[16] = {};
    bg[1] = gs::rgb4(3, 4, 7);
    bg[2] = gs::rgb4(5, 6, 9);
    bg[3] = gs::rgb4(4, 2, 1);
    bg[4] = gs::rgb4(8, 5, 2);
    bg[5] = gs::rgb4(12, 8, 4);
    bg[6] = gs::rgb4(1, 1, 2);
    bg[7] = gs::rgb4(0, 0, 1);
    bg[8] = gs::rgb4(13, 10, 3);
    bg[9] = gs::rgb4(14, 13, 10);
    bg[10] = gs::rgb4(3, 2, 4);
    bg[11] = gs::rgb4(2, 5, 4);
    bg[12] = gs::rgb4(1, 2, 6);
    bg[13] = gs::rgb4(2, 3, 7);
    bg[14] = gs::rgb4(15, 13, 6);
    bg[15] = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_BG, bg);

    inkPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12));
    inkPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2));
    inkPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(15, 7, 4));
    inkPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 6), gs::rgb4(3, 10, 3));

    callPalette(vdp, PAL_PIER, gs::rgb4(5, 15, 15), gs::rgb4(2, 11, 13), gs::rgb4(1, 6, 8));
    callPalette(vdp, PAL_INN, gs::rgb4(15, 12, 4), gs::rgb4(12, 8, 1), gs::rgb4(7, 4, 0));
    callPalette(vdp, PAL_FIRE, gs::rgb4(15, 5, 3), gs::rgb4(13, 2, 1), gs::rgb4(7, 0, 0));
    callPalette(vdp, PAL_CAB, gs::rgb4(15, 14, 4), gs::rgb4(13, 10, 1), gs::rgb4(8, 6, 0));
    callPalette(vdp, PAL_HALL, gs::rgb4(13, 7, 15), gs::rgb4(9, 3, 12), gs::rgb4(5, 1, 7));
    callPalette(vdp, PAL_WIRE, gs::rgb4(8, 15, 6), gs::rgb4(3, 11, 3), gs::rgb4(1, 6, 1));

    uint16_t cord[16] = {};
    cord[1] = gs::rgb4(15, 3, 3);
    cord[2] = gs::rgb4(9, 1, 1);
    cord[3] = gs::rgb4(2, 2, 3);
    cord[4] = gs::rgb4(8, 8, 9);
    cord[5] = gs::rgb4(13, 10, 3);
    cord[15] = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_CORD, cord);

    uint16_t op[16] = {};
    op[1] = gs::rgb4(15, 11, 8);
    op[2] = gs::rgb4(11, 7, 5);
    op[3] = gs::rgb4(3, 2, 2);
    op[4] = gs::rgb4(2, 3, 7);
    op[5] = gs::rgb4(1, 2, 4);
    op[6] = gs::rgb4(14, 11, 4);
    op[7] = gs::rgb4(15, 15, 14);
    op[8] = gs::rgb4(13, 5, 6);
    op[9] = gs::rgb4(1, 1, 2);
    op[10] = gs::rgb4(15, 15, 15);
    op[11] = gs::rgb4(13, 8, 7);
    op[15] = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_OP, op);

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    gs::Bitmap board(gs::SCREEN_W, gs::SCREEN_H);
    paintBoard(board);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, board, PAL_BG);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.setFogColor(gs::rgb4(1, 1, 2));
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.lineBackdrop[y] = gs::rgb4(1, 2, 6);

    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.badge[0] = gs::uploadMipped(vdp, badgePier());
    art.badge[1] = gs::uploadMipped(vdp, badgeInn());
    art.badge[2] = gs::uploadMipped(vdp, badgeFire());
    art.badge[3] = gs::uploadMipped(vdp, badgeCab());
    art.badge[4] = gs::uploadMipped(vdp, badgeHall());
    art.badge[5] = gs::uploadMipped(vdp, badgeWire());
    art.plug = gs::uploadMipped(vdp, plugArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.op = gs::uploadMipped(vdp, operatorArt());
    art.card = gs::uploadMipped(vdp, cardArt());
}

}  // namespace board
