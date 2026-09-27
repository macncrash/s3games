#include "game/art.h"

#include <cmath>
#include <string>

namespace cabgrass {
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

// Nose-up taxi, then spun so frame i faces heading i * 45 degrees from east.
gs::Bitmap cabFrame(int frame) {
    gs::Bitmap src(48, 64);
    src.rect(14, 10, 20, 44, 1);
    src.poly({{14, 18}, {24, 6}, {34, 18}}, 1);
    src.rect(16, 20, 16, 10, 2);
    src.rect(16, 34, 16, 8, 3);
    src.rect(18, 8, 12, 5, 4);
    src.rect(20, 46, 8, 4, 5);
    src.rect(12, 22, 3, 8, 6);
    src.rect(33, 22, 3, 8, 6);
    src.rect(12, 38, 3, 8, 6);
    src.rect(33, 38, 3, 8, 6);
    src.rect(21, 24, 6, 3, 7);

    const float ang = frame * (kPi / 4.f);
    const float c = std::cos(ang), s = std::sin(ang);
    gs::Bitmap b(64, 64);
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            int p = src.get(x, y);
            if (!p) continue;
            float lx = float(x) - 24.f;
            float fy = 32.f - float(y);
            float bx = 32.f + lx * s + fy * c;
            float by = 32.f + lx * c - fy * s;
            b.set(int(std::lround(bx)), int(std::lround(by)), p);
            b.set(int(std::lround(bx)) + 1, int(std::lround(by)), p);
        }
    }
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(40, 18);
    b.ellipse(20, 9, 16, 6, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 36);
    b.rect(12, 22, 4, 14, 1);
    b.ellipse(14, 14, 12, 11, 2);
    b.ellipse(10, 12, 5, 4, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 32);
    b.rect(4, 8, 2, 24, 1);
    b.ellipse(5, 5, 4, 4, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 22);
    b.rect(3, 4, 2, 18, 1);
    b.rect(1, 0, 6, 6, 2);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(12, 14);
    b.line(2, 13, 3, 2, 1, 1.4f);
    b.line(6, 13, 5, 1, 2, 1.4f);
    b.line(10, 13, 9, 3, 1, 1.4f);
    return b;
}

gs::Bitmap meterArt() {
    gs::Bitmap b(18, 10);
    b.rect(0, 0, 18, 10, 1);
    b.rect(2, 2, 14, 6, 2);
    return b;
}

gs::Bitmap smokeArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    return b;
}

void roadColors(gs::VDP& vdp) {
    auto put = [&](int pal, int i, uint16_t c) { vdp.setColor(pal * 16 + i, c); };
    for (int i = 0; i < 16; i++) put(PAL_STREET, i, gs::rgb4(4, 4, 5));
    put(PAL_STREET, 0, 0);
    put(PAL_STREET, 1, gs::rgb4(7, 7, 6));
    put(PAL_STREET, 2, gs::rgb4(5, 5, 4));
    put(PAL_STREET, 3, gs::rgb4(8, 8, 6));
    put(PAL_STREET, 4, gs::rgb4(6, 6, 5));
    put(PAL_STREET, 5, gs::rgb4(4, 4, 4));
    put(PAL_STREET, 6, gs::rgb4(3, 3, 4));
    put(PAL_STREET, 7, gs::rgb4(4, 4, 5));
    put(PAL_STREET, 8, gs::rgb4(6, 6, 6));
    put(PAL_STREET, 9, gs::rgb4(2, 2, 3));
    put(PAL_STREET, 10, gs::rgb4(5, 5, 4));
    put(PAL_STREET, 14, gs::rgb4(14, 12, 3));
    put(PAL_STREET, 15, gs::rgb4(8, 8, 7));

    const uint16_t greens[] = {
        0,
        gs::rgb4(2, 8, 2),
        gs::rgb4(1, 6, 1),
        gs::rgb4(3, 10, 3),
        gs::rgb4(2, 7, 2),
        gs::rgb4(1, 5, 2),
        gs::rgb4(2, 9, 3),
        gs::rgb4(3, 11, 3),
        gs::rgb4(4, 12, 4),
        gs::rgb4(2, 8, 2),
        gs::rgb4(1, 7, 2),
        gs::rgb4(1, 6, 4),
        gs::rgb4(2, 7, 5),
        gs::rgb4(3, 9, 6),
        gs::rgb4(8, 13, 4),
        gs::rgb4(5, 13, 4),
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

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(6, 10, 13), gs::rgb4(3, 5, 7), gs::rgb4(14, 14, 12), gs::rgb4(12, 3, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 2), gs::rgb4(2, 8, 2), gs::rgb4(4, 12, 4)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(4, 4, 5), gs::rgb4(14, 12, 2)});
    roadColors(vdp);

    for (int i = 0; i < 8; i++) art.cab[i] = gs::uploadMipped(vdp, cabFrame(i));
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.meter = gs::uploadMipped(vdp, meterArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.title = gs::uploadMipped(vdp, banner("CAB GRASS", 3));
    art.fullStop = gs::uploadMipped(vdp, banner("FULL STOP", 2));
    art.onGrass = gs::uploadMipped(vdp, banner("ON THE GRASS", 2));
    art.ranOff = gs::uploadMipped(vdp, banner("RAN OFF THE GRASS", 2));
    art.offGrass = gs::uploadMipped(vdp, banner("OFF THE GRASS", 2));
    art.shortStop = gs::uploadMipped(vdp, banner("STOPPED SHORT", 2));
    art.notFull = gs::uploadMipped(vdp, banner("NOT FULLY ON", 2));
    art.timed = gs::uploadMipped(vdp, banner("TIMED OUT", 2));
    art.legFail = gs::uploadMipped(vdp, banner("LEG FAILED", 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2));
    vdp.setFogColor(gs::rgb4(6, 8, 6));
}

}  // namespace cabgrass
