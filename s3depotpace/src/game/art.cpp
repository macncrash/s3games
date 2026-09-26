#include "game/art.h"

#include <cmath>
#include <string>

namespace depotpace {
namespace {

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 15) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintDigit(gs::Bitmap& b, int digit, int x, int y) {
    static const char* rows[3][5] = {
        {"010", "010", "010", "010", "010"},
        {"111", "001", "111", "100", "111"},
        {"111", "001", "111", "001", "111"},
    };
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < 3; i++)
            if (rows[digit][j][i] == '1') b.rect(float(x + i * 3), float(y + j * 3), 2.f, 2.f, 6);
}

gs::Bitmap yardman(int step) {
    gs::Bitmap b(40, 76);
    b.ellipse(20, 9, 9, 4, 5);
    b.rect(11, 9, 18, 3, 5);
    b.rect(11, 12, 10, 3, 5);
    b.ellipse(20, 18, 6, 6, 4);
    b.set(17, 17, 8);
    b.set(23, 17, 8);
    b.rect(16, 23, 8, 2, 3);
    b.poly({{8, 28}, {32, 28}, {35, 54}, {5, 54}}, 1);
    b.poly({{8, 28}, {20, 28}, {18, 54}, {5, 54}}, 2);
    b.rect(18, 32, 2, 16, 9);
    b.rect(28, 34, 9, 12, 6);
    b.rect(29, 35, 4, 5, 7);
    b.line(30, 30, 34, 35, 10, 2.f);
    b.line(12, 32, 6, 46, 2, 3.f);
    b.ellipse(6, 48, 3, 3, 4);
    if (step == 0) {
        b.rect(11, 52, 7, 16, 11);
        b.rect(22, 52, 7, 12, 3);
        b.rect(9, 66, 11, 5, 8);
        b.rect(21, 62, 10, 5, 8);
    } else {
        b.rect(11, 52, 7, 12, 3);
        b.rect(22, 52, 7, 16, 11);
        b.rect(10, 62, 10, 5, 8);
        b.rect(20, 66, 11, 5, 8);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap fallenMan() {
    gs::Bitmap b(86, 36);
    b.ellipse(14, 20, 7, 6, 4);
    b.ellipse(14, 13, 9, 4, 5);
    b.poly({{22, 12}, {70, 16}, {74, 28}, {20, 26}}, 1);
    b.poly({{22, 12}, {46, 14}, {48, 26}, {20, 24}}, 2);
    b.rect(58, 18, 12, 8, 6);
    b.rect(68, 22, 12, 5, 8);
    b.ellipse(44, 12, 4, 3, 4);
    b.outline(8, false);
    return b;
}

gs::Bitmap shedWall() {
    gs::Bitmap b(52, 144);
    b.rect(2, 12, 48, 132, 2);
    for (int y = 14; y < 140; y += 7) {
        b.rect(2, y, 48, 1, 1);
        int shift = ((y / 7) & 1) ? 12 : 0;
        for (int x = 2 + shift; x < 50; x += 16) b.rect(x, y, 1, 7, 1);
    }
    for (int y = 16; y < 138; y += 14)
        for (int x = 8; x < 46; x += 16) b.rect(x, y, 6, 4, 9);
    b.rect(0, 0, 52, 6, 3);
    b.rect(0, 6, 52, 4, 6);
    b.rect(0, 10, 52, 2, 1);
    b.rect(14, 26, 24, 16, 6);
    b.rect(16, 28, 9, 12, 7);
    b.rect(27, 28, 9, 12, 5);
    b.rect(25, 28, 2, 12, 6);
    b.rect(12, 80, 28, 58, 5);
    b.rect(14, 82, 24, 52, 4);
    b.rect(16, 88, 8, 10, 7);
    b.line(20, 108, 20, 128, 6, 1.4f);
    b.outline(8, false);
    return b;
}

gs::Bitmap girderBar() {
    gs::Bitmap b(36, 16);
    b.rect(0, 1, 36, 14, 2);
    b.rect(0, 1, 36, 3, 1);
    b.rect(0, 12, 36, 3, 3);
    for (int x = 2; x < 34; x += 8) b.line(float(x), 4.f, float(x + 6), 12.f, 3, 1.2f);
    return b;
}

gs::Bitmap yardSign() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap label = gs::textBitmap("GOODS", st);
    gs::Bitmap b(label.w + 10, label.h + 8);
    b.rect(0, 0, float(b.w), float(b.h), 5);
    b.rect(2, 2, float(b.w - 4), float(b.h - 4), 4);
    b.blit(label, 5, 4);
    return b;
}

gs::Bitmap clockFace(int tick) {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 12, 12, 3);
    b.ellipse(13, 13, 10, 10, 2);
    for (int i = 0; i < 12; i++) {
        float a = -1.5708f + i * kTau / 12.f;
        int x = int(std::lround(13 + std::cos(a) * 8));
        int y = int(std::lround(13 + std::sin(a) * 8));
        b.set(x, y, 1);
    }
    float hour = -1.5708f + 1.15f;
    float minute = -1.5708f + (tick ? 2.4f : 0.35f);
    b.line(13, 13, 13 + std::cos(hour) * 5.f, 13 + std::sin(hour) * 5.f, 1, 1.6f);
    b.line(13, 13, 13 + std::cos(minute) * 8.f, 13 + std::sin(minute) * 8.f, 1, 1.2f);
    b.set(13, 13, 1);
    return b;
}

gs::Bitmap lampHead() {
    gs::Bitmap b(14, 18);
    b.rect(6, 0, 2, 4, 3);
    b.poly({{2, 6}, {12, 6}, {10, 14}, {4, 14}}, 1);
    b.ellipse(7, 10, 3, 3, 2);
    b.rect(5, 14, 4, 3, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap crateBox(int stacks, int digit) {
    gs::Bitmap b(36, stacks == 2 ? 58 : 40);
    auto box = [&](int y, int bh) {
        b.rect(2, y, 32, bh, 2);
        b.rect(2, y, 32, 4, 1);
        b.rect(4, y + 4, 8, bh - 8, 1);
        b.rect(2, y + bh / 2, 32, 3, 4);
        b.rect(8, y + 2, 3, bh - 4, 4);
        b.rect(24, y + 2, 3, bh - 4, 5);
        b.set(6, y + 6, 7);
        b.set(29, y + 6, 7);
        b.set(6, y + bh - 6, 7);
        b.set(29, y + bh - 6, 7);
    };
    if (stacks == 2) {
        box(2, 26);
        box(30, 26);
        paintDigit(b, digit, 13, 8);
    } else {
        box(2, 36);
        paintDigit(b, digit, 13, 12);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap boxcarArt() {
    gs::Bitmap b(84, 52);
    b.rect(2, 2, 80, 6, 3);
    b.rect(4, 8, 76, 30, 2);
    b.rect(6, 10, 22, 24, 9);
    b.rect(30, 10, 18, 26, 5);
    b.rect(32, 12, 14, 8, 4);
    b.rect(50, 10, 26, 24, 3);
    b.rect(4, 8, 76, 3, 1);
    b.rect(8, 36, 68, 4, 6);
    b.ellipse(20, 44, 6, 6, 4);
    b.ellipse(20, 44, 2, 2, 1);
    b.ellipse(64, 44, 6, 6, 4);
    b.ellipse(64, 44, 2, 2, 1);
    b.outline(8, false);
    return b;
}

gs::Bitmap locoArt() {
    gs::Bitmap b(70, 44);
    b.rect(6, 16, 42, 16, 2);
    b.rect(6, 16, 42, 4, 1);
    b.rect(40, 6, 22, 26, 3);
    b.rect(44, 10, 12, 10, 6);
    b.rect(46, 12, 8, 6, 1);
    b.rect(14, 4, 8, 14, 7);
    b.rect(12, 2, 12, 4, 4);
    b.rect(8, 30, 54, 4, 5);
    b.ellipse(18, 36, 5, 5, 8);
    b.ellipse(18, 36, 2, 2, 3);
    b.ellipse(48, 36, 5, 5, 8);
    b.ellipse(48, 36, 2, 2, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(28, 36);
    b.ellipse(14, 8, 10, 4, 1);
    b.rect(4, 8, 20, 20, 6);
    b.rect(4, 8, 5, 20, 7);
    b.ellipse(14, 28, 10, 4, 7);
    b.rect(4, 14, 20, 3, 1);
    b.rect(4, 20, 20, 2, 4);
    b.outline(8, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(26, 30);
    b.ellipse(13, 16, 10, 12, 2);
    b.ellipse(13, 10, 7, 5, 1);
    b.rect(11, 4, 4, 6, 3);
    b.line(8, 14, 18, 20, 3, 1.2f);
    b.outline(8, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 56);
    b.rect(4, 8, 4, 44, 2);
    b.rect(3, 50, 6, 4, 3);
    b.rect(2, 6, 8, 3, 1);
    b.outline(8, false);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(22, 80);
    b.rect(6, 12, 10, 68, 1);
    b.rect(8, 14, 4, 62, 2);
    b.rect(3, 6, 16, 8, 3);
    b.rect(4, 2, 14, 5, 2);
    b.rect(6, 40, 10, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 14, 16, 7, 2);
    b.rect(4, 14, 32, 14, 1);
    b.ellipse(20, 28, 16, 6, 3);
    b.rect(6, 8, 28, 4, 2);
    b.line(8, 30, 4, 62, 3, 2.f);
    b.line(32, 30, 36, 62, 3, 2.f);
    b.line(14, 32, 12, 62, 1, 2.f);
    b.line(26, 32, 28, 62, 1, 2.f);
    b.line(6, 46, 34, 46, 3, 2.f);
    b.outline(4, false);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(26, 72);
    b.rect(11, 0, 4, 34, 1);
    b.rect(12, 0, 2, 34, 2);
    b.rect(9, 30, 8, 8, 3);
    b.rect(8, 34, 10, 4, 5);
    b.poly({{8, 38}, {18, 38}, {22, 66}, {11, 66}}, 4);
    b.rect(14, 48, 3, 10, 5);
    b.rect(10, 62, 12, 6, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(22, 22);
    for (int i = 0; i < 28; i++) {
        if ((i % 7) < 2) continue;
        float a = i * kTau / 28.f;
        int x = int(std::lround(11 + std::cos(a) * 8));
        int y = int(std::lround(11 + std::sin(a) * 8));
        b.set(x, y, 1);
        b.set(x, y + 1, 2);
    }
    b.rect(10, 2, 2, 3, 1);
    b.rect(10, 17, 2, 3, 1);
    b.rect(2, 10, 3, 2, 1);
    b.rect(17, 10, 3, 2, 1);
    b.set(11, 11, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 5, 1);
    b.ellipse(9, 9, 3, 2, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(24, 14);
    b.ellipse(12, 8, 10, 4, 2);
    b.ellipse(8, 7, 4, 3, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(32, 10);
    for (int x = 0; x < 32; x += 8) {
        b.rect(x, 1, 4, 8, 1);
        b.rect(x + 4, 1, 4, 8, 3);
    }
    return b;
}

gs::Bitmap steamArt() {
    gs::Bitmap b(40, 24);
    b.ellipse(14, 14, 10, 7, 2);
    b.ellipse(24, 12, 12, 8, 1);
    b.ellipse(30, 15, 8, 5, 3);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12), gs::rgb4(4, 4, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 2), gs::rgb4(15, 14, 10), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2),
                           gs::rgb4(10, 7, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 12, 10), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 12), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(13, 11, 9), gs::rgb4(11, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(2, 2, 3),
                            gs::rgb4(1, 1, 2), gs::rgb4(6, 5, 4), gs::rgb4(15, 12, 4), gs::rgb4(1, 1, 1),
                            gs::rgb4(13, 7, 5)});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(11, 8, 4), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 10, 7),
                             gs::rgb4(2, 2, 4), gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(1, 1, 1),
                             gs::rgb4(14, 12, 6), gs::rgb4(5, 3, 2), gs::rgb4(3, 3, 5)});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 10, 2), gs::rgb4(8, 5, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(12, 15, 8), gs::rgb4(5, 12, 6), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 9, 5), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(7, 7, 8),
                           gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 8, 3), gs::rgb4(8, 7, 6), gs::rgb4(12, 11, 9)});
    setPal(vdp, PAL_SOOT, {0, gs::rgb4(6, 6, 8), gs::rgb4(9, 9, 11), gs::rgb4(3, 3, 5), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_METAL, {0, gs::rgb4(12, 12, 14), gs::rgb4(8, 8, 10), gs::rgb4(3, 3, 5), gs::rgb4(9, 6, 3),
                            gs::rgb4(5, 3, 2), gs::rgb4(9, 4, 2), gs::rgb4(5, 2, 1), gs::rgb4(1, 1, 2)});

    const uint16_t road[16] = {
        0,
        gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 3), gs::rgb4(5, 5, 4),
        gs::rgb4(9, 7, 3), gs::rgb4(5, 4, 2),
        gs::rgb4(8, 8, 8), gs::rgb4(6, 6, 7),
        gs::rgb4(7, 6, 5), gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8),
        gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5),
        gs::rgb4(12, 9, 3), gs::rgb4(10, 9, 8),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.walk[0] = gs::uploadMipped(vdp, yardman(0));
    art.walk[1] = gs::uploadMipped(vdp, yardman(1));
    art.fallen = gs::uploadMipped(vdp, fallenMan());
    art.shed = gs::uploadMipped(vdp, shedWall());
    art.girder = gs::uploadMipped(vdp, girderBar());
    art.sign = gs::uploadMipped(vdp, yardSign());
    art.clock[0] = gs::uploadMipped(vdp, clockFace(0));
    art.clock[1] = gs::uploadMipped(vdp, clockFace(1));
    art.lamp = gs::uploadMipped(vdp, lampHead());
    for (int i = 0; i < 3; i++) art.crate[i] = gs::uploadMipped(vdp, crateBox(i == 2 ? 2 : 1, i));
    art.boxcar = gs::uploadMipped(vdp, boxcarArt());
    art.loco = gs::uploadMipped(vdp, locoArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.steam = gs::uploadMipped(vdp, steamArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(5, 4, 4));
}

}  // namespace depotpace
