#include "pictures.h"

#include <cmath>
#include <initializer_list>

namespace choir {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void arch(gs::Bitmap& b, int cx, int glass) {
    const int top = 104;
    const int bot = 154;
    const int hw = 32;
    b.rect(cx - hw - 4, top + 12, hw * 2 + 8, bot - top - 8, 1);
    b.ellipse(float(cx), float(top + 14), float(hw + 4), 16, 1);
    b.rect(cx - hw, top + 14, hw * 2, bot - top - 14, glass);
    b.ellipse(float(cx), float(top + 14), float(hw), 12, glass);
    b.rect(cx - hw + 4, top + 30, 12, 26, 6);
    b.rect(cx + hw - 16, top + 30, 12, 26, 9);
    b.rect(cx - 7, top + 26, 14, 18, 8);
    b.rect(cx - 1, top + 6, 2, bot - top - 10, 10);
    b.rect(cx - hw + 3, top + 44, hw * 2 - 6, 2, 10);
    b.rect(cx - hw + 3, top + 62, hw * 2 - 6, 2, 10);
}

void candle(gs::Bitmap& b, int x, int y) {
    b.rect(x, y, 5, 14, 14);
    b.ellipse(float(x + 2), float(y - 3), 3, 5, 13);
    b.ellipse(float(x + 2), float(y - 2), 1.4f, 2.4f, 1);
}

void stall(gs::Bitmap& b, int cx) {
    b.rect(cx - 24, 156, 48, 30, 4);
    b.rect(cx - 24, 156, 48, 4, 5);
    b.rect(cx - 20, 164, 40, 16, 11);
    b.rect(cx - 2, 160, 4, 22, 5);
}

gs::Bitmap singer(int kind, bool open) {
    gs::Bitmap b(40, 64);
    const int cx = 20;
    const int sh = kind == 2 ? 15 : kind == 1 ? 12 : 10;
    b.ellipse(float(cx), 61, float(sh - 1), 2, 14);
    b.poly({{float(cx - sh), 28}, {float(cx + sh), 28}, {float(cx + sh - 3), 57}, {float(cx - sh + 3), 57}}, 2);
    b.poly({{float(cx - sh + 2), 30}, {float(cx - 1), 30}, {float(cx - 1), 55}, {float(cx - sh + 4), 55}}, 1);
    b.poly({{float(cx + 2), 32}, {float(cx + sh - 3), 32}, {float(cx + sh - 5), 55}, {float(cx + 2), 55}}, 3);
    b.ellipse(float(cx - 6), 57, 4, 2, 10);
    b.ellipse(float(cx + 6), 57, 4, 2, 10);
    b.rect(cx - 3, 23, 6, 6, 4);
    b.poly({{float(cx - 7), 25}, {float(cx), 34}, {float(cx + 7), 25}}, 8);
    b.ellipse(float(cx), 15, 7, 8, 4);
    b.rect(cx - 8, 14, 2, 4, 4);
    b.rect(cx + 6, 14, 2, 4, 4);
    if (kind == 0) {
        b.ellipse(float(cx), 9, 8, 5, 5);
        b.rect(cx - 8, 10, 3, 8, 5);
        b.rect(cx + 5, 10, 3, 8, 5);
    } else if (kind == 1) {
        b.ellipse(float(cx), 9, 8, 5, 5);
        b.rect(cx - 9, 11, 3, 16, 5);
        b.rect(cx + 6, 11, 3, 16, 5);
    } else {
        b.rect(cx - 7, 8, 5, 4, 5);
        b.rect(cx + 2, 8, 5, 4, 5);
        b.ellipse(float(cx), 20, 6, 4, 5);
        b.rect(cx - 5, 17, 10, 5, 5);
    }
    b.outline(15, true);
    b.set(cx - 3, 15, 7);
    b.set(cx - 2, 15, 7);
    b.set(cx + 2, 15, 7);
    b.set(cx + 3, 15, 7);
    b.set(cx - 4, 17, 11);
    b.set(cx + 3, 17, 11);
    if (open) {
        b.rect(cx - 2, 19, 5, 3, 6);
        b.rect(cx - 1, 19, 3, 1, 9);
    } else {
        b.rect(cx - 2, 20, 5, 1, 6);
    }
    b.rect(cx - 1, 29, 3, 3, 12);
    b.ellipse(float(cx), 40, 5, 2, 4);
    if (kind == 1) {
        b.rect(cx - 4, 36, 8, 6, 9);
        b.rect(cx - 4, 38, 8, 1, 7);
    }
    return b;
}

gs::Bitmap noteBitmap() {
    gs::Bitmap b(10, 12);
    b.ellipse(4, 7, 4, 3, 1);
    b.rect(7, 0, 2, 8, 7);
    return b;
}

gs::Bitmap diamondBitmap() {
    gs::Bitmap b(9, 9);
    b.poly({{4, 0}, {8, 4}, {4, 8}, {0, 4}}, 1);
    b.poly({{4, 2}, {6, 4}, {4, 6}, {2, 4}}, 12);
    return b;
}

gs::Bitmap fermataBitmap() {
    gs::Bitmap b(18, 12);
    for (int x = 1; x <= 16; x++) {
        float u = (x - 8.5f) / 8.f;
        float y = 6.f - std::sqrt(std::max(0.f, 1.f - u * u)) * 5.f;
        b.set(x, int(y), 1);
        b.set(x, int(y) + 1, 1);
    }
    b.ellipse(9, 9, 1.6f, 1.6f, 1);
    return b;
}

gs::Bitmap haloBitmap() {
    gs::Bitmap b(32, 14);
    for (int a = 0; a < 40; a++) {
        float t = a * 6.2831853f / 40.f;
        int x = int(std::lround(16 + std::cos(t) * 12));
        int y = int(std::lround(6 + std::sin(t) * 4));
        b.set(x, y, 1);
    }
    return b;
}

gs::Bitmap moteBitmap() {
    gs::Bitmap b(3, 3);
    b.set(1, 0, 2);
    b.set(0, 1, 2);
    b.set(1, 1, 2);
    b.set(2, 1, 2);
    b.set(1, 2, 2);
    return b;
}

gs::Bitmap solid(int index) {
    gs::Bitmap b(2, 4);
    b.rect(0, 0, 2, 4, index);
    return b;
}

void paintChapel(gs::VDP& vdp, gs::TileAlloc& tiles) {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, 320, 86, 3);
    b.rect(32, 28, 256, 52, 10);
    b.rect(32, 28, 256, 1, 8);
    b.rect(32, 79, 256, 1, 8);
    b.rect(32, 28, 1, 52, 8);
    b.rect(287, 28, 1, 52, 8);
    b.rect(0, 86, 320, 8, 4);
    b.rect(0, 94, 320, 68, 2);
    for (int y = 100; y < 156; y += 8) b.rect(0, y, 320, 1, 3);
    arch(b, 56, 7);
    arch(b, 160, 7);
    arch(b, 264, 7);
    b.rect(156, 118, 8, 28, 8);
    b.rect(146, 126, 28, 5, 8);
    b.rect(96, 100, 12, 62, 1);
    b.rect(98, 100, 3, 62, 2);
    b.rect(212, 100, 12, 62, 1);
    b.rect(214, 100, 3, 62, 2);
    b.rect(0, 154, 320, 14, 5);
    stall(b, 56);
    stall(b, 160);
    stall(b, 264);
    b.rect(0, 186, 320, 38, 11);
    for (int i = 0; i < 5; i++) b.rect(0, 192 + i * 7, 320, 1, 5);
    b.rect(146, 186, 28, 38, 6);
    b.rect(36, 196, 248, 4, 8);
    candle(b, 18, 142);
    candle(b, 108, 146);
    candle(b, 206, 146);
    candle(b, 296, 142);
    b.rect(128, 168, 64, 8, 4);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, b, PAL_NAVE);
}

gs::Mipped banner(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 0, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 10, 12), gs::rgb4(15, 14, 10), gs::rgb4(15, 4, 3),
                          gs::rgb4(8, 14, 8), gs::rgb4(15, 12, 6), gs::rgb4(6, 8, 12), gs::rgb4(4, 4, 6),
                          0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NAVE, {0, gs::rgb4(10, 10, 12), gs::rgb4(6, 6, 9), gs::rgb4(2, 2, 5), gs::rgb4(8, 5, 3),
                           gs::rgb4(4, 3, 2), gs::rgb4(13, 2, 4), gs::rgb4(3, 5, 12), gs::rgb4(14, 11, 4),
                           gs::rgb4(3, 11, 7), gs::rgb4(1, 1, 2), gs::rgb4(7, 5, 3), gs::rgb4(10, 8, 5),
                           gs::rgb4(15, 13, 5), gs::rgb4(15, 14, 11), shadow});
    auto robe = [&](int pal, uint16_t hi, uint16_t mid, uint16_t lo, uint16_t skin, uint16_t hair) {
        setPal(vdp, pal, {0, hi, mid, lo, skin, hair, gs::rgb4(12, 4, 5), gs::rgb4(2, 1, 2), ink,
                          gs::rgb4(15, 14, 12), gs::rgb4(3, 2, 2), gs::rgb4(15, 10, 9), gs::rgb4(15, 13, 6),
                          0, gs::rgb4(2, 1, 3), shadow});
    };
    robe(PAL_TREBLE, gs::rgb4(15, 14, 12), gs::rgb4(13, 11, 9), gs::rgb4(8, 7, 6), gs::rgb4(15, 12, 9),
         gs::rgb4(14, 11, 5));
    robe(PAL_ALTO, gs::rgb4(13, 7, 8), gs::rgb4(9, 3, 5), gs::rgb4(5, 2, 3), gs::rgb4(14, 11, 8),
         gs::rgb4(6, 4, 2));
    robe(PAL_BASS, gs::rgb4(7, 8, 12), gs::rgb4(4, 5, 9), gs::rgb4(2, 3, 6), gs::rgb4(13, 10, 8),
         gs::rgb4(4, 4, 5));
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 14), gs::rgb4(8, 6, 3), gs::rgb4(12, 9, 3),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), ink, gs::rgb4(15, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintChapel(vdp, tiles);
    vdp.A.enabled = false;

    for (int i = 0; i < 3; i++) {
        art.singer[i][0] = gs::uploadMipped(vdp, singer(i, false));
        art.singer[i][1] = gs::uploadMipped(vdp, singer(i, true));
    }
    art.note = gs::uploadMipped(vdp, noteBitmap());
    art.diamond = gs::uploadMipped(vdp, diamondBitmap());
    art.fermata = gs::uploadMipped(vdp, fermataBitmap());
    art.halo = gs::uploadMipped(vdp, haloBitmap());
    art.mote = gs::uploadMipped(vdp, moteBitmap());
    art.title = banner(vdp, "S3 CHOIR", 3);
    art.together = banner(vdp, "TOGETHER", 3);
    art.stopped = banner(vdp, "PIECE STOPS", 2);
    art.anthem = banner(vdp, "ANTHEM SUNG", 2);
    const char* letters = "ABC";
    for (int i = 0; i < 3; i++) art.letter[i] = banner(vdp, std::string(1, letters[i]).c_str(), 2);
    art.rule = gs::uploadImage(vdp, solid(1));
    art.staff = gs::uploadImage(vdp, solid(3));
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace choir
