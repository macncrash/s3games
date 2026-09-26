#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace sledboom {
namespace {

constexpr float kTau = 6.2831853f;
constexpr int kTeam = 120;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    uint16_t c[16] = {};
    int i = 0;
    for (uint16_t v : cs)
        if (i < 16) c[i++] = v;
    for (int k = 0; k < 16; k++) vdp.setColor(pal * 16 + k, c[k]);
}

// Source art faces north. Heading 0 stores a rig whose nose points east.
Bitmap spinRig(const Bitmap& src, float heading) {
    Bitmap dst(src.w, src.h);
    const float c = std::cos(heading), s = std::sin(heading);
    const float cx = (src.w - 1) * 0.5f, cy = (src.h - 1) * 0.5f;
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            float dx = x - cx, dy = y - cy;
            int ix = int(std::lround(cx + s * dx + c * dy));
            int iy = int(std::lround(cy - c * dx + s * dy));
            if (ix < 0 || iy < 0 || ix >= src.w || iy >= src.h) continue;
            int v = src.get(ix, iy);
            if (v) dst.set(x, y, v);
        }
    }
    return dst;
}

Bitmap paintTeam() {
    Bitmap b(kTeam, kTeam);
    const float cx = 59.5f, cy = 59.5f;
    auto blob = [&](float lx, float fy, float rx, float ry, int col) { b.ellipse(cx + lx, cy - fy, rx, ry, col); };
    auto stroke = [&](float x0, float f0, float x1, float f1, int col, float th) {
        b.line(cx + x0, cy - f0, cx + x1, cy - f1, col, th);
    };

    stroke(-16.f, 4.f, -12.f, -40.f, 14, 3.4f);
    stroke(16.f, 4.f, 12.f, -40.f, 14, 3.4f);
    stroke(-16.f, 4.f, -12.f, -40.f, 4, 1.5f);
    stroke(16.f, 4.f, 12.f, -40.f, 4, 1.5f);
    stroke(-14.f, -40.f, -6.f, -46.f, 3, 2.4f);
    stroke(14.f, -40.f, 6.f, -46.f, 3, 2.4f);
    stroke(-15.f, 2.f, -9.f, 8.f, 3, 2.2f);
    stroke(15.f, 2.f, 9.f, 8.f, 3, 2.2f);

    b.rect(cx - 18.f, cy - 12.f, 36.f, 22.f, 1);
    b.rect(cx - 15.f, cy - 9.f, 30.f, 16.f, 2);
    stroke(-16.f, 8.f, 16.f, 8.f, 13, 1.6f);
    stroke(-15.f, -4.f, 15.f, -4.f, 13, 1.4f);
    stroke(-14.f, 2.f, 14.f, -6.f, 9, 1.2f);
    stroke(14.f, 2.f, -14.f, -6.f, 9, 1.2f);

    auto dog = [&](float lx, float fy, float sc) {
        blob(lx, fy, 6.4f * sc, 3.6f * sc, 7);
        blob(lx - 1.4f * sc, fy + 0.6f * sc, 3.0f * sc, 2.0f * sc, 8);
        blob(lx, fy + 5.6f * sc, 3.3f * sc, 2.7f * sc, 7);
        blob(lx - 1.5f * sc, fy + 7.6f * sc, 1.2f * sc, 1.7f * sc, 7);
        blob(lx + 1.5f * sc, fy + 7.5f * sc, 1.15f * sc, 1.5f * sc, 8);
        blob(lx + 0.2f * sc, fy + 8.0f * sc, 1.5f * sc, 1.15f * sc, 8);
        blob(lx + 0.2f * sc, fy + 9.2f * sc, 0.7f * sc, 0.55f * sc, 11);
        blob(lx + 1.3f * sc, fy + 6.2f * sc, 0.5f * sc, 0.45f * sc, 10);
        stroke(lx, fy - 3.0f * sc, lx - 2.4f * sc, fy - 6.6f * sc, 7, 1.5f);
        stroke(lx - 2.6f * sc, fy - 0.4f, lx - 3.1f * sc, fy - 4.8f * sc, 7, 1.4f);
        stroke(lx + 2.2f * sc, fy - 0.4f, lx + 2.7f * sc, fy - 4.8f * sc, 7, 1.4f);
    };
    dog(-7.2f, 36.f, 1.0f);
    dog(7.4f, 33.f, 0.96f);
    dog(-6.4f, 22.f, 0.92f);
    dog(6.6f, 19.5f, 0.9f);
    stroke(0.f, 8.f, -7.f, 30.f, 9, 1.4f);
    stroke(-7.f, 32.f, 7.f, 28.f, 9, 1.3f);
    stroke(0.f, 12.f, 6.f, 18.f, 9, 1.2f);
    stroke(-6.f, 20.f, 6.f, 16.f, 9, 1.15f);

    blob(0.f, -16.f, 5.6f, 6.4f, 5);
    blob(0.f, -9.5f, 4.0f, 3.3f, 12);
    blob(0.f, -10.f, 2.3f, 2.2f, 6);
    blob(-0.8f, -9.6f, 0.45f, 0.4f, 10);
    blob(0.8f, -9.6f, 0.45f, 0.4f, 10);
    stroke(-4.2f, -14.f, -9.f, -24.f, 5, 2.2f);
    stroke(4.2f, -14.f, 9.f, -24.f, 5, 2.2f);
    stroke(-10.f, -24.f, 10.f, -24.f, 3, 2.4f);
    blob(0.f, -27.f, 2.2f, 2.6f, 5);

    b.outline(15, false);
    return b;
}

Bitmap paintDrive() {
    Bitmap b(40, 40);
    const float cx = 19.5f, cy = 19.5f;
    for (int i = 0; i < 12; i++) {
        float a = i * kTau / 12.f;
        b.rect(cx + std::cos(a) * 13.f - 2.f, cy + std::sin(a) * 13.f - 2.f, 4.f, 4.f, (i & 1) ? 5 : 6);
    }
    b.ellipse(cx, cy, 11.f, 11.f, 1);
    b.ellipse(cx, cy, 7.5f, 7.5f, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * kTau / 8.f + 0.2f;
        b.line(cx + std::cos(a) * 3.2f, cy + std::sin(a) * 3.2f, cx + std::cos(a) * 9.2f, cy + std::sin(a) * 9.2f, 4,
               1.4f);
    }
    b.ellipse(cx, cy, 3.4f, 3.4f, 5);
    b.ellipse(cx - 1.2f, cy - 1.4f, 1.1f, 1.1f, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintTower() {
    Bitmap b(44, 78);
    b.line(8, 70, 18, 10, 1, 4.f);
    b.line(36, 70, 26, 10, 1, 4.f);
    b.line(8, 70, 18, 10, 2, 1.6f);
    b.line(36, 70, 26, 10, 2, 1.6f);
    b.rect(10, 28, 24, 4, 3);
    b.rect(12, 46, 20, 3, 3);
    b.ellipse(22, 14, 8, 8, 7);
    b.ellipse(22, 14, 3.2f, 3.2f, 5);
    b.rect(20, 4, 4, 8, 5);
    b.ellipse(22, 68, 12, 4, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintBeam() {
    Bitmap b(168, 22);
    b.rect(2, 6, 164, 12, 1);
    b.rect(2, 6, 164, 4, 2);
    for (int i = 0; i < 8; i++) b.rect(6 + i * 20, 7, 3, 10, 5);
    b.rect(2, 4, 164, 3, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintChain() {
    Bitmap b(12, 48);
    for (int i = 0; i < 6; i++) {
        b.ellipse(6, 6 + i * 7, 3.4f, 2.6f, 5);
        b.ellipse(6, 6 + i * 7, 1.6f, 1.0f, 0);
    }
    b.outline(15, false);
    return b;
}

Bitmap paintHook() {
    Bitmap b(28, 36);
    b.rect(12, 2, 4, 10, 5);
    b.line(14, 10, 14, 22, 5, 3.2f);
    b.line(14, 22, 20, 28, 5, 3.2f);
    b.line(20, 28, 18, 18, 5, 2.6f);
    b.ellipse(14, 20, 3.2f, 3.2f, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintRing() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 1);
    b.ellipse(20, 20, 9, 9, 0);
    b.ellipse(20, 20, 4, 4, 2);
    b.ellipse(20, 20, 1.6f, 1.6f, 0);
    for (int i = 0; i < 8; i++) {
        float a = i * kTau / 8.f;
        b.line(20 + std::cos(a) * 10.f, 20 + std::sin(a) * 10.f, 20 + std::cos(a) * 14.f, 20 + std::sin(a) * 14.f, 2,
               1.5f);
    }
    b.outline(15, false);
    return b;
}

Bitmap paintSpruce() {
    Bitmap b(48, 64);
    b.rect(22, 40, 5, 18, 4);
    b.poly({{24, 6}, {44, 28}, {4, 28}}, 1);
    b.poly({{24, 16}, {40, 36}, {8, 36}}, 2);
    b.poly({{24, 26}, {36, 46}, {12, 46}}, 1);
    b.ellipse(16, 18, 4, 2, 3);
    b.ellipse(30, 24, 5, 2.2f, 3);
    b.ellipse(22, 34, 4, 2, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintCornice() {
    Bitmap b(52, 30);
    b.ellipse(26, 18, 22, 9, 1);
    b.ellipse(14, 16, 10, 6, 2);
    b.ellipse(34, 15, 9, 5, 3);
    b.ellipse(24, 14, 6, 3, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintMill() {
    Bitmap b(96, 72);
    b.ellipse(48, 58, 36, 8, 6);
    b.rect(18, 32, 60, 28, 1);
    b.rect(22, 36, 52, 8, 2);
    b.poly({{10, 36}, {48, 12}, {86, 36}}, 3);
    b.poly({{18, 34}, {48, 16}, {78, 34}}, 6);
    b.rect(42, 42, 12, 18, 8);
    b.rect(26, 40, 10, 8, 4);
    b.rect(60, 40, 10, 8, 4);
    b.rect(64, 16, 8, 18, 5);
    b.rect(60, 12, 14, 5, 3);
    b.rect(70, 8, 10, 6, 7);
    b.outline(15, false);
    return b;
}

Bitmap paintSmoke(bool high) {
    Bitmap b(22, 22);
    b.ellipse(11, high ? 8 : 13, 6, 4, 1);
    b.ellipse(8, high ? 6 : 11, 3, 2.4f, 2);
    b.ellipse(14, high ? 9 : 12, 2.2f, 1.8f, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(20, 42);
    b.rect(9, 16, 3, 20, 5);
    b.ellipse(10, 34, 6, 3, 5);
    b.ellipse(10, 12, 7, 6, 4);
    b.ellipse(10, 12, 3.5f, 3, 2);
    b.rect(9, 4, 2, 5, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintStake() {
    Bitmap b(16, 36);
    b.rect(7, 8, 3, 24, 5);
    b.poly({{8, 4}, {15, 10}, {8, 16}}, 3);
    b.ellipse(8, 6, 1.6f, 1.6f, 1);
    return b;
}

Bitmap paintPuff() {
    Bitmap b(16, 12);
    b.ellipse(8, 7, 6, 3.2f, 1);
    b.ellipse(5, 6, 3, 2, 2);
    return b;
}

Bitmap paintFlake() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st{scale, 1, 14, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 12), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, gs::rgb4(3, 4, 6), line});
    setPal(vdp, PAL_SLED,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(11, 8, 4), gs::rgb4(12, 14, 15), gs::rgb4(14, 3, 3),
            gs::rgb4(15, 12, 9), gs::rgb4(4, 4, 5), gs::rgb4(9, 8, 7), gs::rgb4(13, 10, 3), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 6, 5), gs::rgb4(14, 15, 15), gs::rgb4(12, 9, 5), gs::rgb4(5, 6, 7), line});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(11, 12, 13), gs::rgb4(10, 5, 2), gs::rgb4(14, 11, 4),
            gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(13, 10, 6), gs::rgb4(5, 4, 2), gs::rgb4(12, 9, 4), gs::rgb4(7, 8, 9),
            gs::rgb4(15, 14, 10), gs::rgb4(4, 5, 6), 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(2, 3, 7), gs::rgb4(1, 2, 5), gs::rgb4(8, 6, 3), gs::rgb4(10, 12, 13), gs::rgb4(3, 6, 12),
            gs::rgb4(13, 10, 8), gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8), gs::rgb4(12, 10, 4), gs::rgb4(1, 1, 2),
            gs::rgb4(12, 6, 5), gs::rgb4(13, 14, 15), gs::rgb4(11, 8, 5), gs::rgb4(2, 2, 3), line});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(1, 5, 2), gs::rgb4(2, 8, 3), gs::rgb4(14, 15, 15), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0,
            line, line});
    setPal(vdp, PAL_MILL,
           {0, gs::rgb4(7, 5, 3), gs::rgb4(10, 7, 4), gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 4), gs::rgb4(5, 2, 2),
            gs::rgb4(14, 15, 15), gs::rgb4(13, 2, 2), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(9, 11, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, line, line});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 7), gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 3), 0, 0,
            0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 7), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1),
                          line});
    setPal(vdp, PAL_ALERT,
           {0, gs::rgb4(15, 6, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(15, 13, 7), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1), line});
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(14, 15, 15), gs::rgb4(12, 13, 15), gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(10, 12, 14),
            gs::rgb4(11, 14, 15), gs::rgb4(8, 12, 14), gs::rgb4(9, 11, 13), gs::rgb4(7, 11, 14), gs::rgb4(13, 15, 15),
            gs::rgb4(3, 6, 9), gs::rgb4(2, 5, 8), gs::rgb4(5, 9, 12), gs::rgb4(15, 15, 15), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_PAD,
           {0, gs::rgb4(14, 15, 15), gs::rgb4(12, 13, 14), gs::rgb4(15, 15, 15), gs::rgb4(14, 13, 11), gs::rgb4(11, 10, 8),
            gs::rgb4(14, 13, 10), gs::rgb4(12, 11, 8), gs::rgb4(10, 9, 7), gs::rgb4(9, 8, 6), gs::rgb4(15, 14, 11),
            gs::rgb4(4, 5, 6), gs::rgb4(3, 4, 5), gs::rgb4(8, 7, 5), gs::rgb4(15, 14, 8), gs::rgb4(13, 12, 9)});
    vdp.setFogColor(gs::rgb4(8, 10, 13));

    loadFont(vdp, art);
    Bitmap team = paintTeam();
    for (int i = 0; i < 16; i++) art.team[i] = gs::uploadMipped(vdp, spinRig(team, i * kTau / 16.f));
    art.drive = gs::uploadMipped(vdp, paintDrive());
    art.tower = gs::uploadMipped(vdp, paintTower());
    art.beam = gs::uploadMipped(vdp, paintBeam());
    art.chain = gs::uploadMipped(vdp, paintChain());
    art.hook = gs::uploadMipped(vdp, paintHook());
    art.ring = gs::uploadMipped(vdp, paintRing());
    art.spruce = gs::uploadMipped(vdp, paintSpruce());
    art.cornice = gs::uploadMipped(vdp, paintCornice());
    art.mill = gs::uploadMipped(vdp, paintMill());
    art.smoke[0] = gs::uploadMipped(vdp, paintSmoke(false));
    art.smoke[1] = gs::uploadMipped(vdp, paintSmoke(true));
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.stake = gs::uploadMipped(vdp, paintStake());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.flake = gs::uploadMipped(vdp, paintFlake());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = word(vdp, "SLED BOOM", 3);
    art.delivered = word(vdp, "DELIVERED", 3);
    art.onBoom = word(vdp, "ON THE BOOM", 2);
    art.paused = word(vdp, "PAUSED", 3);
    art.leg = word(vdp, "LEG FAILED", 2);
    art.crewTook = word(vdp, "CREW TOOK THE BOOM", 2);
    art.wentOver = word(vdp, "DRIVE WENT OVER", 2);
    art.broke = word(vdp, "BROKE THE BOOM", 2);
    art.spilled = word(vdp, "DRIVE SPILLED", 2);
    art.leftIce = word(vdp, "LEFT THE ICE", 2);
}

}  // namespace sledboom
