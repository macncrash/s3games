#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace depotdoor {
namespace {

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

// Clerk indices: 1 coat, 2 shade, 3 cap, 4 skin, 5 shirt, 6 boot, 7 buckle, 8 outline.
gs::Bitmap clerkArt(int pose) {
    gs::Bitmap b(pose == 2 ? 72 : 40, pose == 2 ? 36 : 68);
    if (pose == 2) {
        b.ellipse(14, 20, 7, 6, 4);
        b.ellipse(14, 12, 9, 4, 3);
        b.poly({{22, 14}, {62, 16}, {66, 28}, {20, 26}}, 1);
        b.poly({{22, 14}, {40, 15}, {42, 26}, {20, 24}}, 2);
        b.rect(48, 18, 10, 6, 5);
        b.rect(58, 22, 10, 4, 6);
        b.outline(8, false);
        return b;
    }
    b.ellipse(20, 8, 10, 4, 3);
    b.rect(11, 8, 18, 3, 3);
    b.ellipse(20, 16, 6, 6, 4);
    b.set(17, 15, 8);
    b.set(23, 15, 8);
    b.rect(17, 20, 6, 2, 2);
    b.poly({{9, 24}, {31, 24}, {34, 50}, {6, 50}}, 1);
    b.poly({{9, 24}, {20, 24}, {18, 50}, {6, 50}}, 2);
    b.rect(18, 28, 3, 14, 5);
    b.rect(17, 40, 6, 3, 7);
    if (pose == 0) {
        b.line(12, 28, 6, 42, 2, 3.f);
        b.line(28, 28, 34, 40, 2, 3.f);
        b.ellipse(6, 44, 3, 3, 4);
        b.ellipse(34, 42, 3, 3, 4);
    } else {
        b.line(14, 28, 16, 8, 2, 3.f);
        b.line(26, 28, 24, 8, 2, 3.f);
        b.ellipse(20, 8, 4, 3, 4);
    }
    b.rect(12, 50, 6, 12, 6);
    b.rect(22, 50, 6, 12, 2);
    b.rect(10, 60, 10, 4, 8);
    b.rect(21, 60, 10, 4, 8);
    b.outline(8, false);
    return b;
}

// Docker: 1 coat, 2 shade, 3 cap, 4 skin, 5 bar, 6 boot, 7 glove, 8 outline.
gs::Bitmap dockerArt(int step) {
    gs::Bitmap b(44, 64);
    b.ellipse(16, 8, 8, 3, 3);
    b.rect(9, 8, 14, 2, 3);
    b.ellipse(16, 15, 5, 5, 4);
    b.set(14, 14, 8);
    b.set(18, 14, 8);
    b.poly({{8, 22}, {26, 22}, {28, 46}, {6, 46}}, 1);
    b.poly({{8, 22}, {16, 22}, {15, 46}, {6, 46}}, 2);
    b.line(10, 26, 4, 36, 2, 3.f);
    b.ellipse(4, 38, 2, 2, 7);
    b.line(22, 24, 40, 18, 5, 2.4f);
    b.line(36, 16, 42, 22, 5, 2.f);
    if (step == 0) {
        b.rect(10, 46, 5, 12, 6);
        b.rect(18, 46, 5, 10, 2);
    } else {
        b.rect(10, 46, 5, 10, 2);
        b.rect(18, 46, 5, 12, 6);
    }
    b.rect(8, 56, 8, 3, 8);
    b.rect(17, 54, 8, 3, 8);
    b.outline(8, false);
    return b;
}

// Freight: 1 wood hi, 2 wood, 3 dark, 4 iron, 5 crate band, 6 wheel, 7 hub, 8 outline.
gs::Bitmap truckArt() {
    gs::Bitmap b(52, 40);
    b.rect(8, 4, 28, 16, 2);
    b.rect(8, 4, 28, 3, 1);
    b.rect(10, 8, 10, 8, 5);
    b.rect(22, 9, 10, 7, 3);
    b.rect(4, 20, 36, 5, 4);
    b.rect(4, 20, 36, 2, 1);
    b.line(38, 22, 50, 8, 4, 2.2f);
    b.line(40, 24, 48, 14, 3, 1.6f);
    b.ellipse(16, 32, 6, 6, 6);
    b.ellipse(16, 32, 2, 2, 7);
    b.ellipse(32, 32, 6, 6, 6);
    b.ellipse(32, 32, 2, 2, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap slatArt() {
    gs::Bitmap b(64, 14);
    b.rect(0, 1, 64, 12, 2);
    b.rect(0, 1, 64, 3, 1);
    b.rect(0, 10, 64, 3, 3);
    for (int x = 4; x < 62; x += 10) {
        b.rect(float(x), 4, 2, 6, 4);
        b.set(x, 6, 5);
    }
    b.rect(0, 6, 64, 1, 6);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(28, 96);
    b.rect(2, 8, 24, 86, 2);
    b.rect(18, 8, 8, 86, 3);
    b.rect(2, 8, 6, 86, 1);
    for (int i = 0; i < 9; i++) {
        int y = 12 + i * 9;
        b.rect(3, float(y), 20, 2, 4);
        if (i % 2 == 0) b.rect(8, float(y + 3), 8, 3, 6);
    }
    b.rect(8, 28, 10, 14, 5);
    b.rect(10, 30, 6, 8, 7);
    b.rect(0, 0, 28, 8, 5);
    b.rect(0, 0, 28, 3, 1);
    b.rect(4, 88, 20, 6, 3);
    b.outline(8, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(96, 18);
    b.rect(0, 4, 96, 12, 2);
    b.rect(0, 4, 96, 3, 1);
    b.rect(0, 13, 96, 3, 3);
    for (int i = 0; i < 8; i++) b.rect(float(4 + i * 12), 6, 3, 8, 4);
    b.rect(0, 0, 96, 4, 5);
    return b;
}

gs::Bitmap sprocketArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 2);
    b.ellipse(9, 9, 3, 3, 3);
    for (int i = 0; i < 6; i++) {
        float a = i * kTau / 6.f;
        b.rect(8 + std::cos(a) * 6.f, 8 + std::sin(a) * 6.f, 2, 2, 1);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap chainArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 5, 3, 4, 2);
    b.ellipse(4, 5, 1, 2, 0);
    b.rect(3, 1, 2, 2, 1);
    b.rect(3, 7, 2, 2, 3);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(16, 10);
    b.rect(1, 2, 10, 6, 2);
    b.rect(1, 2, 10, 2, 1);
    b.rect(8, 3, 7, 4, 3);
    b.rect(12, 1, 3, 8, 4);
    b.set(4, 4, 5);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(28, 24);
    b.ellipse(14, 14, 12, 8, 2);
    b.ellipse(14, 10, 8, 6, 1);
    b.rect(10, 6, 8, 4, 5);
    b.line(8, 12, 20, 16, 3, 1.2f);
    b.outline(8, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(32, 28);
    b.rect(2, 4, 28, 20, 2);
    b.rect(2, 4, 28, 4, 1);
    b.rect(2, 12, 28, 3, 5);
    b.rect(8, 4, 3, 20, 3);
    b.rect(20, 4, 3, 20, 3);
    b.set(6, 8, 7);
    b.set(24, 8, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(22, 28);
    b.ellipse(11, 6, 8, 3, 1);
    b.rect(3, 6, 16, 16, 4);
    b.rect(3, 6, 4, 16, 1);
    b.ellipse(11, 22, 8, 3, 3);
    b.rect(3, 12, 16, 2, 5);
    b.outline(8, false);
    return b;
}

// Boxcar: 1 red hi, 2 red, 3 shade, 4 roof, 5 door, 6 wheel, 7 rust, 8 outline.
gs::Bitmap boxcarArt() {
    gs::Bitmap b(88, 48);
    b.rect(4, 6, 80, 8, 4);
    b.rect(6, 14, 76, 22, 2);
    b.rect(6, 14, 76, 4, 1);
    b.rect(10, 18, 22, 16, 5);
    b.rect(36, 18, 16, 16, 3);
    b.rect(56, 18, 22, 16, 5);
    b.rect(8, 34, 72, 4, 7);
    b.ellipse(22, 40, 6, 6, 6);
    b.ellipse(22, 40, 2, 2, 4);
    b.ellipse(66, 40, 6, 6, 6);
    b.ellipse(66, 40, 2, 2, 4);
    b.outline(8, false);
    return b;
}

gs::Bitmap lampArt(int hot) {
    gs::Bitmap b(14, 22);
    b.rect(6, 0, 2, 4, 3);
    b.poly({{2, 5}, {12, 5}, {10, 14}, {4, 14}}, hot ? 1 : 2);
    b.ellipse(7, 10, 3, 3, hot ? 5 : 4);
    b.rect(5, 14, 4, 4, 3);
    b.outline(8, false);
    return b;
}

// Bay interior: 1 dark, 2 floor, 3 crate, 4 crate hi, 5 bulb, 6 beam, 7 deep, 8 outline.
gs::Bitmap bayArt() {
    gs::Bitmap b(64, 80);
    b.rect(0, 0, 64, 80, 1);
    b.rect(0, 0, 8, 80, 7);
    b.rect(56, 0, 8, 80, 7);
    b.rect(0, 68, 64, 12, 2);
    b.rect(8, 52, 16, 16, 3);
    b.rect(8, 52, 16, 3, 4);
    b.rect(28, 58, 14, 12, 3);
    b.rect(46, 48, 12, 20, 4);
    b.rect(46, 48, 12, 3, 3);
    b.rect(30, 8, 4, 10, 6);
    b.ellipse(32, 20, 4, 4, 5);
    return b;
}

gs::Bitmap signArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap label = gs::textBitmap("BAY 4", st);
    gs::Bitmap b(label.w + 12, label.h + 10);
    b.rect(0, 0, float(b.w), float(b.h), 3);
    b.rect(2, 2, float(b.w - 4), float(b.h - 4), 2);
    b.blit(label, 6, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 2);
    b.ellipse(11, 11, 8, 8, 1);
    for (int i = 0; i < 12; i++) {
        float a = -1.5708f + i * kTau / 12.f;
        int x = int(std::lround(11 + std::cos(a) * 6));
        int y = int(std::lround(11 + std::sin(a) * 6));
        b.set(x, y, 3);
    }
    b.line(11, 11, 11, 5, 4, 1.4f);
    b.line(11, 11, 15, 12, 4, 1.f);
    b.set(11, 11, 5);
    b.outline(8, false);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(6, 6);
    b.rect(2, 0, 2, 6, 1);
    b.rect(0, 2, 6, 2, 2);
    return b;
}

gs::Bitmap chipArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 3, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(11, 7, 5, 5, 0);
    b.set(6, 6, 2);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(4, 2, 1);
    b.set(2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 10, 11), gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 3),
                           gs::rgb4(8, 8, 9), gs::rgb4(6, 6, 7), gs::rgb4(12, 12, 13), gs::rgb4(1, 1, 2),
                           gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 8), gs::rgb4(9, 9, 10),
                           gs::rgb4(11, 11, 12), gs::rgb4(13, 13, 14), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1), gs::rgb4(10, 7, 2),
                           gs::rgb4(12, 9, 3), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12), gs::rgb4(1, 1, 1),
                           gs::rgb4(4, 3, 1), gs::rgb4(9, 6, 2), gs::rgb4(13, 10, 4), gs::rgb4(7, 5, 2),
                           gs::rgb4(11, 8, 3), gs::rgb4(14, 11, 5), gs::rgb4(5, 4, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 10, 8), gs::rgb4(6, 1, 1), gs::rgb4(10, 2, 2),
                            gs::rgb4(12, 5, 3), gs::rgb4(8, 2, 2), gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1),
                            gs::rgb4(4, 1, 1), gs::rgb4(9, 3, 2), gs::rgb4(13, 4, 3), gs::rgb4(7, 2, 1),
                            gs::rgb4(11, 3, 2), gs::rgb4(14, 6, 4), gs::rgb4(5, 1, 1), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 5), gs::rgb4(14, 15, 10), gs::rgb4(1, 4, 1), gs::rgb4(4, 10, 3),
                           gs::rgb4(6, 12, 4), gs::rgb4(2, 6, 2), gs::rgb4(12, 15, 8), gs::rgb4(1, 1, 1),
                           gs::rgb4(2, 3, 1), gs::rgb4(5, 11, 3), gs::rgb4(7, 13, 4), gs::rgb4(3, 8, 2),
                           gs::rgb4(9, 14, 5), gs::rgb4(10, 15, 6), gs::rgb4(1, 5, 1), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(12, 9, 7), gs::rgb4(13, 5, 3), gs::rgb4(7, 2, 2), gs::rgb4(4, 3, 3),
                            gs::rgb4(8, 8, 9), gs::rgb4(15, 11, 4), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 2),
                            gs::rgb4(5, 3, 2), gs::rgb4(10, 4, 3), gs::rgb4(9, 7, 6), gs::rgb4(6, 4, 3),
                            gs::rgb4(14, 8, 5), gs::rgb4(3, 2, 2), gs::rgb4(11, 6, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(13, 14, 15), gs::rgb4(8, 9, 11), gs::rgb4(4, 5, 7), gs::rgb4(6, 6, 8),
                            gs::rgb4(10, 6, 3), gs::rgb4(5, 6, 8), gs::rgb4(15, 13, 8), gs::rgb4(1, 1, 2),
                            gs::rgb4(3, 3, 5), gs::rgb4(9, 10, 12), gs::rgb4(7, 8, 10), gs::rgb4(2, 2, 4),
                            gs::rgb4(12, 13, 14), gs::rgb4(6, 7, 9), gs::rgb4(11, 8, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(10, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(14, 10, 7),
                           gs::rgb4(12, 12, 10), gs::rgb4(3, 2, 2), gs::rgb4(15, 12, 4), gs::rgb4(1, 1, 1),
                           gs::rgb4(5, 3, 2), gs::rgb4(8, 6, 3), gs::rgb4(12, 8, 5), gs::rgb4(7, 5, 3),
                           gs::rgb4(9, 7, 4), gs::rgb4(2, 2, 2), gs::rgb4(13, 9, 6), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_DOCKER, {0, gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 3), gs::rgb4(13, 9, 6),
                             gs::rgb4(9, 9, 11), gs::rgb4(2, 2, 2), gs::rgb4(12, 8, 4), gs::rgb4(1, 1, 1),
                             gs::rgb4(4, 4, 6), gs::rgb4(7, 7, 9), gs::rgb4(8, 6, 3), gs::rgb4(6, 5, 4),
                             gs::rgb4(10, 8, 6), gs::rgb4(3, 2, 2), gs::rgb4(11, 10, 8), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_FREIGHT, {0, gs::rgb4(13, 10, 6), gs::rgb4(10, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(7, 7, 8),
                              gs::rgb4(8, 5, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 12, 13), gs::rgb4(1, 1, 1),
                              gs::rgb4(4, 3, 2), gs::rgb4(11, 8, 5), gs::rgb4(9, 6, 3), gs::rgb4(5, 5, 6),
                              gs::rgb4(14, 11, 7), gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 5), gs::rgb4(12, 8, 2), gs::rgb4(4, 3, 2), gs::rgb4(15, 10, 3),
                           gs::rgb4(15, 15, 10), gs::rgb4(8, 5, 2), gs::rgb4(10, 7, 3), gs::rgb4(1, 1, 1),
                           gs::rgb4(3, 2, 1), gs::rgb4(14, 9, 3), gs::rgb4(11, 6, 2), gs::rgb4(6, 4, 1),
                           gs::rgb4(13, 11, 6), gs::rgb4(9, 6, 2), gs::rgb4(7, 5, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(10, 11, 12), gs::rgb4(14, 14, 15), gs::rgb4(5, 6, 7), gs::rgb4(8, 5, 3),
                           gs::rgb4(12, 9, 5), gs::rgb4(3, 3, 4), gs::rgb4(7, 8, 9), gs::rgb4(1, 1, 2),
                           gs::rgb4(4, 4, 5), gs::rgb4(9, 9, 10), gs::rgb4(6, 6, 7), gs::rgb4(11, 11, 12),
                           gs::rgb4(13, 13, 14), gs::rgb4(2, 2, 3), gs::rgb4(8, 7, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(14, 14, 10), gs::rgb4(8, 8, 12), gs::rgb4(4, 4, 6), gs::rgb4(15, 15, 13),
                            gs::rgb4(6, 6, 8), gs::rgb4(10, 10, 8), gs::rgb4(12, 12, 14), gs::rgb4(1, 1, 2),
                            gs::rgb4(2, 2, 4), gs::rgb4(9, 9, 11), gs::rgb4(5, 5, 7), gs::rgb4(7, 7, 9),
                            gs::rgb4(11, 11, 13), gs::rgb4(3, 3, 5), gs::rgb4(13, 13, 11), gs::rgb4(1, 1, 3)});
    const uint16_t road[16] = {0,
                                gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 6),
                                gs::rgb4(7, 6, 3), gs::rgb4(4, 4, 3),
                                gs::rgb4(7, 7, 8), gs::rgb4(5, 5, 6),
                                gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 5),
                                gs::rgb4(3, 3, 5), gs::rgb4(2, 2, 4), gs::rgb4(4, 4, 6),
                                gs::rgb4(14, 11, 3), gs::rgb4(9, 9, 10)};
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);
    setPal(vdp, PAL_CAR, {0, gs::rgb4(14, 4, 3), gs::rgb4(11, 3, 2), gs::rgb4(6, 2, 2), gs::rgb4(5, 5, 6),
                          gs::rgb4(8, 7, 6), gs::rgb4(3, 3, 4), gs::rgb4(9, 5, 3), gs::rgb4(1, 1, 1),
                          gs::rgb4(4, 2, 2), gs::rgb4(12, 4, 3), gs::rgb4(8, 3, 2), gs::rgb4(7, 6, 5),
                          gs::rgb4(13, 6, 4), gs::rgb4(2, 2, 3), gs::rgb4(10, 8, 6), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BAY, {0, gs::rgb4(2, 2, 4), gs::rgb4(4, 4, 5), gs::rgb4(8, 5, 3), gs::rgb4(11, 7, 4),
                          gs::rgb4(15, 13, 5), gs::rgb4(6, 6, 7), gs::rgb4(1, 1, 2), gs::rgb4(1, 1, 1),
                          gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(7, 5, 3), gs::rgb4(9, 6, 4),
                          gs::rgb4(4, 3, 5), gs::rgb4(2, 2, 3), gs::rgb4(12, 10, 6), gs::rgb4(1, 1, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.clerk[0] = gs::uploadMipped(vdp, clerkArt(0));
    art.clerk[1] = gs::uploadMipped(vdp, clerkArt(1));
    art.clerk[2] = gs::uploadMipped(vdp, clerkArt(2));
    art.docker[0] = gs::uploadMipped(vdp, dockerArt(0));
    art.docker[1] = gs::uploadMipped(vdp, dockerArt(1));
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.slat = gs::uploadMipped(vdp, slatArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.sprocket = gs::uploadMipped(vdp, sprocketArt());
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.boxcar = gs::uploadMipped(vdp, boxcarArt());
    art.lamp[0] = gs::uploadMipped(vdp, lampArt(0));
    art.lamp[1] = gs::uploadMipped(vdp, lampArt(1));
    art.bay = gs::uploadMipped(vdp, bayArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.chip = gs::uploadMipped(vdp, chipArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
}

}  // namespace depotdoor
