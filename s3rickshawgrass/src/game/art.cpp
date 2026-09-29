#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace rickgrass {
namespace {

constexpr float kPi = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

gs::Bitmap banner(const char* s, int scale) {
    gs::TextStyle st{scale, 1, 0, 0, 1};
    return gs::textBitmap(s, st);
}

// Top-down rickshaw, nose up, then spun so frame i faces heading i * 45 deg from east.
gs::Bitmap shawFrame(int frame) {
    gs::Bitmap src(48, 72);
    src.ellipse(24, 40, 13, 16, 1);
    src.ellipse(24, 38, 9, 11, 2);
    src.rect(18, 46, 12, 8, 3);
    src.ellipse(24, 28, 4, 5, 4);
    src.rect(22, 22, 4, 6, 5);
    src.line(16, 36, 8, 18, 6, 1.6f);
    src.line(32, 36, 40, 18, 6, 1.6f);
    src.ellipse(10, 16, 4, 5, 4);
    src.rect(8, 10, 4, 6, 5);
    src.ellipse(16, 52, 5, 6, 7);
    src.ellipse(32, 52, 5, 6, 7);
    src.ellipse(16, 52, 2, 3, 6);
    src.ellipse(32, 52, 2, 3, 6);
    src.rect(21, 18, 6, 3, 8);

    const float ang = frame * (kPi / 4.f);
    const float c = std::cos(ang), s = std::sin(ang);
    gs::Bitmap b(72, 72);
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            int p = src.get(x, y);
            if (!p) continue;
            float lx = float(x) - 24.f;
            float fy = 36.f - float(y);
            float bx = 36.f + lx * s + fy * c;
            float by = 36.f + lx * c - fy * s;
            b.set(int(std::lround(bx)), int(std::lround(by)), p);
            b.set(int(std::lround(bx)) + 1, int(std::lround(by)), p);
        }
    }
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(36, 16);
    b.ellipse(18, 8, 15, 5, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(26, 34);
    b.rect(11, 20, 4, 14, 1);
    b.ellipse(13, 13, 11, 10, 2);
    b.ellipse(9, 11, 4, 4, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 30);
    b.rect(4, 8, 2, 22, 1);
    b.ellipse(5, 5, 4, 4, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 20);
    b.rect(3, 4, 2, 16, 1);
    b.rect(1, 0, 6, 5, 2);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(12, 14);
    b.line(2, 13, 4, 2, 1, 1.4f);
    b.line(6, 13, 6, 1, 2, 1.4f);
    b.line(10, 13, 8, 3, 1, 1.4f);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(16, 22);
    b.rect(2, 2, 2, 20, 1);
    b.poly({{4, 2}, {15, 6}, {4, 10}}, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    return b;
}

void roadColors(gs::VDP& vdp) {
    auto put = [&](int pal, int i, uint16_t c) { vdp.setColor(pal * 16 + i, c); };
    for (int i = 0; i < 16; i++) put(PAL_LANE, i, gs::rgb4(5, 4, 3));
    put(PAL_LANE, 0, 0);
    put(PAL_LANE, 1, gs::rgb4(8, 7, 5));
    put(PAL_LANE, 2, gs::rgb4(6, 5, 4));
    put(PAL_LANE, 3, gs::rgb4(9, 8, 6));
    put(PAL_LANE, 4, gs::rgb4(7, 6, 4));
    put(PAL_LANE, 5, gs::rgb4(4, 4, 3));
    put(PAL_LANE, 6, gs::rgb4(3, 3, 3));
    put(PAL_LANE, 7, gs::rgb4(5, 4, 4));
    put(PAL_LANE, 8, gs::rgb4(7, 6, 5));
    put(PAL_LANE, 9, gs::rgb4(2, 2, 2));
    put(PAL_LANE, 10, gs::rgb4(6, 5, 3));
    put(PAL_LANE, 14, gs::rgb4(14, 12, 4));
    put(PAL_LANE, 15, gs::rgb4(9, 8, 6));

    const uint16_t greens[] = {
        0,
        gs::rgb4(1, 7, 2),
        gs::rgb4(1, 5, 1),
        gs::rgb4(2, 9, 2),
        gs::rgb4(2, 6, 2),
        gs::rgb4(1, 4, 1),
        gs::rgb4(2, 8, 3),
        gs::rgb4(3, 10, 3),
        gs::rgb4(4, 11, 3),
        gs::rgb4(1, 7, 2),
        gs::rgb4(1, 6, 2),
        gs::rgb4(1, 5, 3),
        gs::rgb4(2, 6, 4),
        gs::rgb4(3, 8, 4),
        gs::rgb4(7, 12, 3),
        gs::rgb4(4, 12, 3),
    };
    for (int i = 0; i < 16; i++) put(PAL_FIELD, i, greens[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 12));
    textPal(vdp, PAL_WIN, gs::rgb4(6, 15, 7));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 3));
    loadFont(vdp, art);

    setPal(vdp, PAL_SHAW,
           {0, gs::rgb4(14, 6, 2), gs::rgb4(12, 10, 4), gs::rgb4(8, 4, 2), gs::rgb4(13, 9, 6), gs::rgb4(3, 3, 6),
            gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 2), gs::rgb4(2, 7, 2), gs::rgb4(4, 11, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(9, 8, 5)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(4, 4, 5), gs::rgb4(14, 3, 2)});
    roadColors(vdp);

    for (int i = 0; i < 8; i++) art.shaw[i] = gs::uploadMipped(vdp, shawFrame(i));
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.title = gs::uploadMipped(vdp, banner("RICKSHAW GRASS", 2));
    art.fullStop = gs::uploadMipped(vdp, banner("FULL STOP", 2));
    art.onGrass = gs::uploadMipped(vdp, banner("ON THE GRASS", 2));
    art.ranOff = gs::uploadMipped(vdp, banner("RAN OFF THE GRASS", 2));
    art.offGrass = gs::uploadMipped(vdp, banner("OFF THE GRASS", 2));
    art.shortStop = gs::uploadMipped(vdp, banner("STOPPED SHORT", 2));
    art.notFull = gs::uploadMipped(vdp, banner("NOT FULLY ON", 2));
    art.timed = gs::uploadMipped(vdp, banner("TIMED OUT", 2));
    art.legFail = gs::uploadMipped(vdp, banner("LEG FAILED", 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2));
    vdp.setFogColor(gs::rgb4(5, 7, 4));
}

}  // namespace rickgrass
