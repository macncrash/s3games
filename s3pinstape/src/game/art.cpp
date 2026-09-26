#include "game/art.h"

#include <initializer_list>

namespace pinstape {
namespace {

constexpr int WALL = 1, WALLD = 2, MAPLE = 3, MAPLED = 4, MAPLEL = 5, GUTTER = 6;
constexpr int FELT = 7, FELTD = 8, PAPER = 9, PAPERS = 10, RED = 11, INK = 12;
constexpr int BRASS = 13, LAMP = 14, CREAM = 15;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void shadow(gs::VDP& vdp, int pal, uint16_t c) { vdp.setColor(pal * 16 + 15, c); }

void stamp(gs::Bitmap& b, int x, int y, const char* s, int c) {
    for (int i = 0; s[i]; i++) {
        const uint8_t* g = gs::glyph(s[i]);
        for (int gy = 0; gy < 7; gy++)
            for (int gx = 0; gx < 5; gx++)
                if (g[gy * 5 + gx]) b.set(x + i * 6 + gx, y + gy, c);
    }
}

void miniPin(gs::Bitmap& b, int x, int y, bool on) {
    int body = on ? RED : PAPERS;
    int band = on ? INK : PAPER;
    b.ellipse(x + 4, y + 2, 3, 2, body);
    b.rect(x + 3, y + 3, 3, 5, body);
    b.rect(x + 3, y + 5, 3, 2, band);
    b.ellipse(x + 4, y + 9, 3, 2, body);
}

void paintHouse(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 0, 320, 4, WALLD);

    // Receipt. The three rows are the tape. Filled pins are the leave.
    b.rect(4, 2, 156, 68, PAPERS);
    b.rect(6, 4, 152, 64, PAPER);
    b.rect(6, 4, 152, 3, RED);
    for (int x = 0; x < 152; x += 8) b.rect(6 + x, 64, 4, 4, PAPER);
    stamp(b, 12, 8, "TAPE", INK);
    const char* rowName[3] = {"HEAD", "THREES", "TWOS"};
    const int rowMask[3] = {1 << 0, (1 << 1) | (1 << 2), (1 << 3) | (1 << 4)};
    static const int order[5] = {3, 1, 0, 2, 4};
    for (int r = 0; r < 3; r++) {
        int y = kRowY[r] - 3;
        stamp(b, 12, y, rowName[r], INK);
        for (int s = 0; s < 5; s++) {
            int pin = order[s];
            bool on = (rowMask[r] & (1 << pin)) != 0;
            miniPin(b, kMarkX0 + s * kMarkPitch, kRowY[r] - 5, on);
        }
    }

    // Dark window for the live count.
    b.rect(166, 4, 148, 64, BRASS);
    b.rect(169, 7, 142, 58, WALLD);
    b.ellipse(184, 16, 3, 3, LAMP);
    b.ellipse(298, 16, 3, 3, LAMP);

    b.rect(0, 70, 320, 4, BRASS);

    // Short maple lane. The rack sits close to the foul line.
    b.rect(18, 74, 284, 72, GUTTER);
    for (int x = 36; x < 284; x++) {
        int stripe = ((x - 36) / 10) & 1;
        b.rect(x, 76, 1, 66, stripe ? MAPLE : MAPLED);
        if ((x - 36) % 10 == 0) b.rect(x, 76, 1, 66, MAPLEL);
    }
    b.rect(36, 140, 248, 2, RED);
    for (int i = 0; i < 5; i++) {
        float x = laneX(kPinX[i], kPinZ[i]);
        float y = laneY(kPinZ[i]);
        b.ellipse(x, y + 1, 5, 2, WALLD);
        float dx = laneX(kPinX[i], 0.55f);
        b.ellipse(dx, 132, 2, 2, CREAM);
    }

    // Approach, then the open pin drawer.
    b.rect(0, 146, 320, 16, MAPLED);
    b.rect(0, 146, 320, 2, MAPLEL);
    stamp(b, 8, 151, "SHORT", INK);

    b.rect(0, 162, 320, 46, MAPLE);
    b.rect(0, 162, 320, 3, MAPLEL);
    b.rect(8, 170, 304, 32, FELTD);
    b.rect(12, 174, 296, 24, FELT);
    stamp(b, 14, 164, "DRAWER", INK);
    static const char* worth = "23532";
    for (int s = 0; s < 5; s++) {
        float cx = 48.f + s * 56.f;
        b.ellipse(cx, kDrawerY, 16, 10, FELTD);
        b.ellipse(cx, kDrawerY, 13, 8, FELT);
        char lab[2] = {worth[s], 0};
        stamp(b, int(cx) - 2, 196, lab, CREAM);
    }

    b.rect(0, 208, 320, 16, INK);
    b.rect(0, 208, 320, 2, BRASS);
}

gs::Bitmap pinArt() {
    gs::Bitmap b(16, 34);
    b.ellipse(8, 28, 6, 4, 1);
    b.ellipse(8, 27, 5, 3, 2);
    b.rect(5, 16, 6, 10, 1);
    b.rect(5, 18, 6, 3, 3);
    b.rect(5, 19, 6, 1, 4);
    b.ellipse(8, 14, 5, 4, 1);
    b.ellipse(6, 12, 2, 2, 6);
    b.ellipse(8, 8, 3, 3, 1);
    b.rect(7, 5, 2, 4, 1);
    b.ellipse(8, 4, 3, 2, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(6, 6, 2, 1, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap bowlerArt(int pose) {
    gs::Bitmap b(32, 46);
    b.ellipse(16, 7, 6, 5, 4);
    b.rect(10, 3, 12, 4, 4);
    b.ellipse(16, 12, 5, 5, 3);
    b.rect(12, 16, 8, 3, 3);
    b.poly({{10, 19}, {22, 19}, {24, 32}, {8, 32}}, 1);
    b.poly({{13, 21}, {19, 21}, {18, 30}, {14, 30}}, 7);
    b.rect(10, 31, 12, 2, 2);
    int step = pose == 1 ? 2 : 0;
    b.rect(11 - step, 33, 4, 8, 5);
    b.rect(17 + step, 33, 4, 8, 5);
    b.ellipse(12 - step, 42, 4, 2, 6);
    b.ellipse(20 + step, 42, 4, 2, 6);
    if (pose == 0) {
        b.rect(5, 20, 4, 10, 3);
        b.ellipse(7, 30, 3, 3, 3);
        b.rect(22, 20, 4, 10, 1);
        b.ellipse(24, 30, 3, 3, 3);
    } else if (pose == 1) {
        b.poly({{8, 20}, {12, 20}, {6, 32}, {2, 30}}, 3);
        b.ellipse(4, 32, 3, 3, 3);
        b.poly({{20, 20}, {24, 18}, {28, 10}, {24, 8}}, 1);
        b.ellipse(27, 10, 3, 3, 3);
    } else {
        b.rect(6, 20, 4, 9, 3);
        b.ellipse(8, 29, 3, 3, 3);
        b.poly({{20, 20}, {24, 20}, {30, 26}, {26, 28}}, 1);
        b.ellipse(29, 27, 3, 3, 3);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(12, 16);
    b.poly({{6, 1}, {11, 14}, {1, 14}}, 1);
    b.poly({{6, 6}, {9, 13}, {3, 13}}, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap tickArt() {
    gs::Bitmap b(12, 10);
    b.line(1, 5, 4, 8, 1, 2);
    b.line(4, 8, 11, 1, 1, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 8, 3, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12)});
    shadow(vdp, PAL_TEXT, ink);
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10)});
    shadow(vdp, PAL_GOLD, ink);
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 10, 3), gs::rgb4(15, 14, 8)});
    shadow(vdp, PAL_AMBER, ink);
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 8), gs::rgb4(12, 15, 12)});
    shadow(vdp, PAL_GREEN, ink);
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 8)});
    shadow(vdp, PAL_RED, ink);
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(4, 2, 3), gs::rgb4(2, 1, 1), gs::rgb4(11, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(14, 10, 5),
            gs::rgb4(1, 1, 3), gs::rgb4(1, 8, 5), gs::rgb4(0, 4, 2), gs::rgb4(15, 14, 12), gs::rgb4(12, 10, 8),
            gs::rgb4(12, 2, 2), gs::rgb4(2, 1, 1), gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_PIN, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 11, 9), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
                          gs::rgb4(2, 1, 2), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(10, 2, 2), gs::rgb4(5, 1, 1), gs::rgb4(15, 8, 6), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BOWLER,
           {0, gs::rgb4(8, 4, 6), gs::rgb4(4, 2, 3), gs::rgb4(15, 12, 9), gs::rgb4(3, 2, 2), gs::rgb4(2, 3, 6),
            gs::rgb4(2, 1, 1), gs::rgb4(14, 13, 10), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 1, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap house(gs::SCREEN_W, gs::SCREEN_H);
    paintHouse(house);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, house, PAL_HOUSE);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(2, 1, 1);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(2, 1, 1));

    art.pin = gs::uploadMipped(vdp, pinArt());
    art.ball = gs::uploadMipped(vdp, ballArt());
    for (int i = 0; i < 3; i++) art.bowler[i] = gs::uploadMipped(vdp, bowlerArt(i));
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.tick = gs::uploadMipped(vdp, tickArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace pinstape
