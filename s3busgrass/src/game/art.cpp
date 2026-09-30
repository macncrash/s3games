#include "game/art.h"

#include <cmath>
#include <string>

namespace busgrass {
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

// Nose-up city bus. Frame i faces heading i * 45 degrees from east.
gs::Bitmap busFrame(int frame) {
    gs::Bitmap src(36, 84);
    src.rect(8, 8, 20, 68, 1);
    src.rect(8, 6, 20, 6, 4);
    src.rect(10, 12, 16, 8, 2);
    for (int row = 0; row < 5; row++) src.rect(10, 24 + row * 8, 16, 5, 3);
    src.rect(14, 64, 8, 6, 5);
    src.rect(6, 18, 3, 8, 6);
    src.rect(27, 18, 3, 8, 6);
    src.rect(6, 40, 3, 8, 6);
    src.rect(27, 40, 3, 8, 6);
    src.rect(6, 58, 3, 8, 6);
    src.rect(27, 58, 3, 8, 6);
    src.rect(11, 8, 14, 3, 7);
    src.rect(16, 70, 4, 3, 8);

    const float ang = frame * (kPi / 4.f);
    const float c = std::cos(ang), s = std::sin(ang);
    gs::Bitmap b(96, 96);
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            int p = src.get(x, y);
            if (!p) continue;
            float lx = float(x) - 18.f;
            float fy = 42.f - float(y);
            float bx = 48.f + lx * s + fy * c;
            float by = 48.f + lx * c - fy * s;
            b.set(int(std::lround(bx)), int(std::lround(by)), p);
            b.set(int(std::lround(bx)) + 1, int(std::lround(by)), p);
        }
    }
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(24, 8, 20, 5, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(30, 40);
    b.rect(13, 24, 4, 16, 1);
    b.ellipse(15, 15, 13, 12, 2);
    b.ellipse(11, 12, 5, 4, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 36);
    b.rect(4, 10, 2, 26, 1);
    b.rect(1, 6, 8, 3, 1);
    b.ellipse(5, 5, 4, 4, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 26);
    b.rect(4, 6, 2, 20, 1);
    b.rect(1, 0, 8, 7, 2);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(14, 16);
    b.line(2, 15, 3, 2, 1, 1.5f);
    b.line(7, 15, 6, 1, 2, 1.5f);
    b.line(11, 15, 10, 3, 1, 1.5f);
    return b;
}

gs::Bitmap chalkArt() {
    gs::Bitmap b(22, 22);
    b.line(3, 3, 18, 18, 1, 1.6f);
    b.line(18, 3, 3, 18, 1, 1.6f);
    b.ellipse(11, 11, 8, 8, 2);
    return b;
}

gs::Bitmap benchArt() {
    gs::Bitmap b(28, 14);
    b.rect(2, 3, 24, 4, 1);
    b.rect(3, 7, 2, 6, 2);
    b.rect(23, 7, 2, 6, 2);
    return b;
}

gs::Bitmap shelterArt() {
    gs::Bitmap b(34, 28);
    b.rect(2, 2, 30, 4, 1);
    b.rect(3, 6, 2, 20, 2);
    b.rect(29, 6, 2, 20, 2);
    b.rect(4, 16, 26, 8, 3);
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
    put(PAL_STREET, 1, gs::rgb4(6, 6, 6));
    put(PAL_STREET, 2, gs::rgb4(5, 5, 5));
    put(PAL_STREET, 3, gs::rgb4(7, 7, 7));
    put(PAL_STREET, 4, gs::rgb4(5, 5, 6));
    put(PAL_STREET, 5, gs::rgb4(3, 3, 4));
    put(PAL_STREET, 6, gs::rgb4(3, 3, 3));
    put(PAL_STREET, 7, gs::rgb4(4, 4, 5));
    put(PAL_STREET, 8, gs::rgb4(8, 8, 8));
    put(PAL_STREET, 9, gs::rgb4(2, 2, 3));
    put(PAL_STREET, 10, gs::rgb4(5, 5, 4));
    put(PAL_STREET, 14, gs::rgb4(14, 13, 4));
    put(PAL_STREET, 15, gs::rgb4(9, 9, 8));

    const uint16_t greens[] = {
        0,
        gs::rgb4(2, 7, 2),
        gs::rgb4(1, 5, 1),
        gs::rgb4(3, 9, 2),
        gs::rgb4(2, 6, 2),
        gs::rgb4(1, 4, 2),
        gs::rgb4(2, 8, 3),
        gs::rgb4(3, 10, 3),
        gs::rgb4(4, 11, 3),
        gs::rgb4(2, 7, 2),
        gs::rgb4(1, 6, 2),
        gs::rgb4(1, 5, 3),
        gs::rgb4(2, 6, 4),
        gs::rgb4(3, 8, 5),
        gs::rgb4(7, 12, 3),
        gs::rgb4(4, 12, 3),
    };
    for (int i = 0; i < 16; i++) put(PAL_FIELD, i, greens[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_WIN, gs::rgb4(5, 15, 7));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 12, 3));
    loadFont(vdp, art);

    setPal(vdp, PAL_BUS,
           {0, gs::rgb4(14, 13, 11), gs::rgb4(8, 12, 14), gs::rgb4(4, 7, 10), gs::rgb4(2, 5, 11),
            gs::rgb4(12, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 5), gs::rgb4(6, 6, 6)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 1), gs::rgb4(1, 7, 2), gs::rgb4(3, 11, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(5, 5, 6), gs::rgb4(14, 13, 3)});
    setPal(vdp, PAL_BENCH, {0, gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(10, 11, 12)});
    roadColors(vdp);

    for (int i = 0; i < 8; i++) art.bus[i] = gs::uploadMipped(vdp, busFrame(i));
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.chalk = gs::uploadMipped(vdp, chalkArt());
    art.bench = gs::uploadMipped(vdp, benchArt());
    art.shelter = gs::uploadMipped(vdp, shelterArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.title = gs::uploadMipped(vdp, banner("BUS GRASS", 3));
    art.fullStop = gs::uploadMipped(vdp, banner("FULL STOP", 2));
    art.onGrass = gs::uploadMipped(vdp, banner("ON THE GRASS", 2));
    art.ranOff = gs::uploadMipped(vdp, banner("RAN OFF THE GRASS", 2));
    art.offGrass = gs::uploadMipped(vdp, banner("OFF THE GRASS", 2));
    art.shortStop = gs::uploadMipped(vdp, banner("STOPPED SHORT", 2));
    art.notFull = gs::uploadMipped(vdp, banner("NOT FULLY ON", 2));
    art.curbStop = gs::uploadMipped(vdp, banner("CURB IS NOT GRASS", 2));
    art.timed = gs::uploadMipped(vdp, banner("TIMED OUT", 2));
    art.legFail = gs::uploadMipped(vdp, banner("THE RUN FAILED", 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2));
    vdp.setFogColor(gs::rgb4(5, 8, 5));
}

}  // namespace busgrass
