#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace fairtape {
namespace {

constexpr float kPi = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap jarArt() {
    gs::Bitmap b(16, 36);
    b.rect(5, 1, 6, 4, 4);
    b.rect(6, 2, 2, 3, 1);
    b.rect(4, 5, 8, 5, 2);
    b.rect(5, 6, 2, 3, 1);
    b.ellipse(8, 20, 6.4f, 9.2f, 2);
    b.ellipse(5.6f, 17.5f, 2.1f, 5.4f, 1);
    b.rect(3, 14, 10, 4, 5);
    b.rect(4, 15, 8, 1, 1);
    b.ellipse(8, 29, 5.2f, 2.2f, 3);
    b.rect(4, 31, 8, 3, 3);
    return b;
}

gs::Bitmap roseArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 4, 2.6f, 2.3f, 2);
    b.ellipse(4, 7, 2.5f, 2.2f, 2);
    b.ellipse(12, 7, 2.5f, 2.2f, 2);
    b.ellipse(6, 11, 2.4f, 2.1f, 2);
    b.ellipse(10, 11, 2.4f, 2.1f, 2);
    b.ellipse(8, 8, 2.2f, 2.2f, 1);
    b.rect(7, 12, 2, 2, 4);
    return b;
}

gs::Bitmap budArt() {
    gs::Bitmap b(10, 14);
    b.ellipse(5, 6, 3.1f, 4.2f, 2);
    b.ellipse(4, 5, 1.3f, 1.8f, 1);
    b.poly({{2, 9}, {5, 13}, {8, 9}}, 4);
    b.rect(4, 12, 2, 2, 4);
    return b;
}

gs::Bitmap caneArt() {
    gs::Bitmap b(14, 20);
    b.ellipse(8, 5, 4.2f, 3.6f, 2);
    b.ellipse(8, 5, 2.4f, 1.8f, 0);
    for (int y = 6; y < 20; y++) {
        int c = ((y / 3) & 1) ? 2 : 1;
        b.rect(6, y, 3, 1, c);
    }
    b.rect(5, 18, 5, 2, 3);
    return b;
}

gs::Bitmap stickArt() {
    gs::Bitmap b(8, 20);
    b.rect(3, 1, 2, 17, 2);
    b.rect(3, 1, 2, 2, 1);
    b.rect(2, 17, 4, 2, 3);
    for (int y = 4; y < 16; y += 4) b.set(3, y, 3);
    return b;
}

gs::Bitmap bearArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(4, 4, 2.6f, 2.6f, 2);
    b.ellipse(14, 4, 2.6f, 2.6f, 2);
    b.ellipse(9, 8, 6.2f, 5.4f, 2);
    b.ellipse(6.5f, 7, 1.8f, 2.2f, 1);
    b.ellipse(9, 11, 2.4f, 1.6f, 1);
    b.set(7, 8, 15);
    b.set(11, 8, 15);
    b.set(9, 10, 15);
    b.rect(7, 13, 4, 2, 4);
    b.rect(6, 14, 6, 1, 4);
    return b;
}

gs::Bitmap cubArt() {
    gs::Bitmap b(14, 12);
    b.ellipse(3, 3, 2.1f, 2.1f, 2);
    b.ellipse(11, 3, 2.1f, 2.1f, 2);
    b.ellipse(7, 6, 4.6f, 4.0f, 2);
    b.ellipse(7, 8, 1.8f, 1.2f, 1);
    b.set(5, 6, 15);
    b.set(9, 6, 15);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8.2f, 8.2f, 2);
    b.ellipse(9, 9, 6.2f, 6.2f, 3);
    b.ellipse(9, 9, 4.6f, 4.6f, 1);
    b.ellipse(7.2f, 7.0f, 1.5f, 1.1f, 4);
    b.ellipse(9, 9, 2.8f, 2.8f, 0);
    return b;
}

gs::Bitmap kidArt(bool step) {
    gs::Bitmap b(16, 34);
    b.rect(3, 0, 10, 3, 4);
    b.rect(2, 2, 12, 2, 4);
    b.rect(4, 3, 8, 2, 6);
    b.ellipse(8, 9, 4.1f, 4.0f, 1);
    b.set(6, 9, 15);
    b.set(10, 9, 15);
    b.set(7, 11, 2);
    b.set(8, 12, 2);
    b.set(9, 11, 2);
    b.rect(4, 14, 8, 8, 2);
    b.rect(2, 15, 2, 6, 2);
    b.rect(12, 15, 2, 6, 1);
    b.rect(4, 21, 8, 2, 5);
    if (step) {
        b.rect(4, 23, 3, 8, 3);
        b.rect(9, 24, 3, 7, 3);
        b.rect(3, 30, 4, 3, 5);
        b.rect(9, 30, 4, 3, 5);
    } else {
        b.rect(5, 23, 3, 8, 3);
        b.rect(9, 23, 3, 8, 3);
        b.rect(4, 30, 4, 3, 5);
        b.rect(9, 30, 4, 3, 5);
    }
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(12, 12);
    b.poly({{6, 1}, {11, 11}, {1, 11}}, 2);
    b.poly({{6, 4}, {9, 10}, {3, 10}}, 1);
    b.rect(5, 0, 2, 2, 3);
    return b;
}

gs::Bitmap bulbArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 4, 3.0f, 3.1f, 2);
    b.ellipse(3.1f, 3.2f, 1.2f, 1.2f, 1);
    b.rect(3, 7, 2, 2, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 40);
    b.rect(2, 0, 6, 40, 2);
    b.rect(3, 0, 2, 40, 1);
    b.rect(7, 0, 1, 40, 3);
    for (int y = 6; y < 38; y += 8) b.rect(2, y, 6, 2, 4);
    return b;
}

gs::Bitmap shelfArt() {
    gs::Bitmap b(24, 8);
    b.rect(0, 2, 24, 5, 2);
    b.rect(0, 0, 24, 2, 1);
    b.rect(0, 6, 24, 2, 3);
    return b;
}

gs::Bitmap awningArt() {
    gs::Bitmap b(48, 20);
    for (int x = 0; x < 48; x++) {
        int stripe = (x / 6) % 3;
        int c = stripe == 0 ? 1 : (stripe == 1 ? 2 : 3);
        for (int y = 0; y < 12; y++) b.set(x, y, c);
        int scallop = x % 8;
        int cut = scallop < 4 ? scallop : 7 - scallop;
        for (int y = 12; y < 12 + cut && y < 20; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 48, 2, 4);
    return b;
}

gs::Bitmap clothArt() {
    gs::Bitmap b(16, 28);
    for (int x = 0; x < 16; x++) {
        int c = ((x / 4) & 1) ? 2 : 1;
        for (int y = 0; y < 28; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 16, 2, 3);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 14.5f, 14.5f, 1);
    b.ellipse(16, 16, 11.5f, 11.5f, 0);
    b.ellipse(16, 16, 10.6f, 10.6f, 2);
    b.ellipse(16, 16, 8.4f, 8.4f, 0);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * kPi / 4.f;
        b.line(16, 16, 16 + std::cos(a) * 12.f, 16 + std::sin(a) * 12.f, 5, 1.f);
    }
    b.ellipse(16, 16, 2.2f, 2.2f, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5.6f, 5.6f, 1);
    b.ellipse(9.5f, 6, 4.4f, 4.4f, 0);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(9, 9);
    b.line(4, 0, 4, 8, 1, 1);
    b.line(0, 4, 8, 4, 1, 1);
    b.line(1, 1, 7, 7, 2, 1);
    b.line(7, 1, 1, 7, 2, 1);
    return b;
}

gs::Bitmap balloonArt() {
    gs::Bitmap b(12, 16);
    b.ellipse(6, 6, 4.6f, 5.2f, 2);
    b.ellipse(4.4f, 4.6f, 1.6f, 1.8f, 1);
    b.poly({{6, 10}, {8, 13}, {4, 13}}, 3);
    b.line(6, 13, 6, 15, 3, 1);
    return b;
}

gs::Bitmap gondolaArt() {
    gs::Bitmap b(8, 6);
    b.rect(1, 1, 6, 4, 4);
    b.rect(2, 2, 4, 2, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.4f, 1.4f, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(14, 5);
    b.ellipse(7, 2, 6.2f, 1.6f, 3);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(196, 6);
    b.rect(0, 1, 196, 4, 2);
    b.rect(0, 0, 196, 1, 1);
    b.rect(0, 5, 196, 1, 3);
    for (int x = 8; x < 196; x += 18) b.rect(x, 1, 2, 4, 3);
    return b;
}

gs::Bitmap tapeArt() {
    const float left = slotX(0) - 46.f;
    const float right = slotX(2) + 46.f;
    const int w = int(right - left);
    const int h = 18;
    gs::Bitmap b(w, h);
    b.rect(0, 0, w, h, 2);
    b.rect(0, 0, w, 2, 4);
    b.rect(0, h - 2, w, 2, 4);
    b.rect(0, 2, 3, h - 4, 1);
    for (int x = 6; x < w - 2; x += 12) {
        b.rect(x, 1, 3, 3, 0);
        b.rect(x, h - 4, 3, 3, 0);
    }
    for (int i = 0; i < 2; i++) {
        int cut = int(slotX(i) + 36.f - left);
        if (cut > 2 && cut < w - 2) b.rect(cut, 3, 1, h - 6, 4);
    }
    return b;
}

gs::Bitmap drawerArt() {
    const float left = slotX(0) - 40.f;
    const float right = slotX(2) + 40.f;
    const int w = int(right - left);
    const int h = 28;
    gs::Bitmap b(w, h);
    b.rect(0, 0, w, h, 2);
    b.rect(0, 0, w, 4, 1);
    b.rect(0, h - 5, w, 5, 3);
    b.rect(2, 4, w - 4, 2, 3);
    for (int i = 0; i < kTapeN; i++) {
        int cx = int(slotX(i) - left);
        b.rect(cx - 22, 8, 44, 14, 3);
        b.rect(cx - 20, 10, 40, 10, 4);
        b.rect(cx - 6, h - 4, 12, 2, 1);
    }
    return b;
}

gs::Bitmap slipArt(const char* name) {
    gs::Bitmap word = gs::textBitmap(name, {1, 3, 0, 0, 1});
    gs::Bitmap b(word.w + 8, word.h + 6);
    b.rect(0, 0, b.w, b.h, 1);
    b.rect(1, 1, b.w - 2, b.h - 2, 2);
    b.blit(word, 4, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
        a.font[c - 32] = t;
    }
}

gs::Image phrase(gs::VDP& vdp, const char* s, int scale, int outline) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, outline, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 10), gs::rgb4(3, 3, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 4, 1), gs::rgb4(15, 10, 2), gs::rgb4(5, 2, 1),
                           gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(12, 15, 9), gs::rgb4(2, 6, 2), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 8, 6), gs::rgb4(8, 1, 1), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(14, 11, 6), gs::rgb4(9, 6, 3), gs::rgb4(4, 2, 1), gs::rgb4(2, 1, 1),
                           gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_ROSE, {0, gs::rgb4(15, 12, 12), gs::rgb4(13, 2, 3), gs::rgb4(6, 1, 1), gs::rgb4(15, 14, 8),
                           gs::rgb4(2, 6, 2), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_CANE, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 2, 3), gs::rgb4(6, 1, 1), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_BEAR, {0, gs::rgb4(15, 12, 8), gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(12, 2, 2),
                           gs::rgb4(15, 10, 4)});
    setPal(vdp, PAL_RING, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 8, 2), gs::rgb4(6, 4, 1), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_KID, {0, gs::rgb4(15, 12, 9), gs::rgb4(12, 3, 4), gs::rgb4(2, 3, 7), gs::rgb4(14, 10, 3),
                          gs::rgb4(3, 2, 1), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_BULB, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 12, 4), gs::rgb4(6, 4, 2), gs::rgb4(15, 7, 2)});
    setPal(vdp, PAL_AWN, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 8), gs::rgb4(15, 10, 2), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(10, 12, 14), gs::rgb4(3, 3, 6), gs::rgb4(14, 11, 4), gs::rgb4(12, 3, 4),
                            gs::rgb4(8, 8, 12)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(10, 7, 4), gs::rgb4(15, 14, 10), gs::rgb4(7, 2, 2), gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 15, 13), gs::rgb4(14, 12, 7), gs::rgb4(8, 6, 3), gs::rgb4(3, 7, 3)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(12, 14, 15), gs::rgb4(3, 6, 13), gs::rgb4(1, 2, 6), gs::rgb4(15, 14, 10)});

    loadFont(vdp, art);
    art.jar = gs::uploadMipped(vdp, jarArt());
    art.rose = gs::uploadMipped(vdp, roseArt());
    art.bud = gs::uploadMipped(vdp, budArt());
    art.cane = gs::uploadMipped(vdp, caneArt());
    art.stick = gs::uploadMipped(vdp, stickArt());
    art.bear = gs::uploadMipped(vdp, bearArt());
    art.cub = gs::uploadMipped(vdp, cubArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.kid[0] = gs::uploadMipped(vdp, kidArt(false));
    art.kid[1] = gs::uploadMipped(vdp, kidArt(true));
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.shelf = gs::uploadMipped(vdp, shelfArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.cloth = gs::uploadMipped(vdp, clothArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.balloon = gs::uploadMipped(vdp, balloonArt());
    art.gondola = gs::uploadMipped(vdp, gondolaArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.rail = gs::uploadImage(vdp, railArt());
    art.tape = gs::uploadImage(vdp, tapeArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    art.logo = phrase(vdp, "S3 FAIR TAPE", 2, 2);
    art.leave = phrase(vdp, "LEAVE", 2, 2);
    art.stay = phrase(vdp, "STAY", 2, 2);
    art.sign = phrase(vdp, "RINGS", 1, 2);
    for (int i = 0; i < kShelfN; i++)
        art.tag[i] = gs::uploadImage(vdp, gs::textBitmap(kShelf[i].name, {1, 1, 0, 15, 1}));
    for (int i = 0; i < kTapeN; i++) art.slip[i] = gs::uploadImage(vdp, slipArt(tapeName(i)));
}

}  // namespace fairtape
