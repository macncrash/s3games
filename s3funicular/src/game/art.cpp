#include "art.h"

#include <cstdint>

namespace funicular {
namespace {

void color(gs::VDP& v, int pal, int i, int r, int g, int b) { v.setColor(pal * 16 + i, gs::rgb4(r, g, b)); }

void palettes(gs::VDP& v) {
    color(v, PAL_HUD, 1, 15, 15, 15);
    color(v, PAL_HUD, 2, 9, 10, 12);
    color(v, PAL_HUD, 3, 15, 12, 4);
    color(v, PAL_HUD, 4, 15, 5, 3);
    color(v, PAL_HUD, 5, 5, 14, 7);
    color(v, PAL_HUD, 6, 1, 1, 2);
    color(v, PAL_HUD, 7, 14, 14, 15);
    color(v, PAL_HUD, 8, 6, 8, 11);

    color(v, PAL_RED, 1, 6, 1, 1);
    color(v, PAL_RED, 2, 12, 2, 2);
    color(v, PAL_RED, 3, 14, 12, 8);
    color(v, PAL_RED, 4, 4, 7, 10);
    color(v, PAL_RED, 5, 10, 13, 15);
    color(v, PAL_RED, 6, 2, 2, 3);
    color(v, PAL_RED, 7, 13, 10, 3);
    color(v, PAL_RED, 8, 15, 15, 14);
    color(v, PAL_RED, 9, 2, 2, 2);
    color(v, PAL_RED, 10, 8, 8, 9);
    color(v, PAL_RED, 11, 9, 6, 3);
    color(v, PAL_RED, 12, 4, 3, 3);
    color(v, PAL_RED, 13, 8, 2, 2);
    color(v, PAL_RED, 14, 15, 14, 6);
    color(v, PAL_RED, 15, 1, 0, 0);

    color(v, PAL_GREEN, 1, 1, 4, 2);
    color(v, PAL_GREEN, 2, 2, 8, 3);
    color(v, PAL_GREEN, 3, 14, 12, 8);
    color(v, PAL_GREEN, 4, 4, 7, 10);
    color(v, PAL_GREEN, 5, 10, 13, 15);
    color(v, PAL_GREEN, 6, 2, 2, 3);
    color(v, PAL_GREEN, 7, 13, 10, 3);
    color(v, PAL_GREEN, 8, 15, 15, 14);
    color(v, PAL_GREEN, 9, 2, 2, 2);
    color(v, PAL_GREEN, 10, 8, 8, 9);
    color(v, PAL_GREEN, 11, 9, 6, 3);
    color(v, PAL_GREEN, 12, 3, 4, 3);
    color(v, PAL_GREEN, 13, 1, 5, 2);
    color(v, PAL_GREEN, 14, 15, 14, 6);
    color(v, PAL_GREEN, 15, 0, 1, 0);

    color(v, PAL_DECK, 1, 9, 6, 3);
    color(v, PAL_DECK, 2, 12, 9, 5);
    color(v, PAL_DECK, 3, 5, 4, 3);
    color(v, PAL_DECK, 4, 10, 3, 2);
    color(v, PAL_DECK, 5, 14, 12, 8);
    color(v, PAL_DECK, 6, 3, 2, 2);
    color(v, PAL_DECK, 7, 15, 14, 10);
    color(v, PAL_DECK, 8, 15, 12, 2);
    color(v, PAL_DECK, 9, 2, 2, 1);
    color(v, PAL_DECK, 10, 7, 7, 6);
    color(v, PAL_DECK, 11, 4, 5, 3);
    color(v, PAL_DECK, 15, 1, 1, 1);

    color(v, PAL_STEEL, 1, 3, 3, 4);
    color(v, PAL_STEEL, 2, 8, 9, 10);
    color(v, PAL_STEEL, 3, 13, 14, 15);
    color(v, PAL_STEEL, 4, 10, 6, 2);
    color(v, PAL_STEEL, 5, 1, 1, 1);
    color(v, PAL_STEEL, 6, 15, 12, 2);
    color(v, PAL_STEEL, 7, 6, 6, 7);

    color(v, PAL_FOLK, 1, 13, 8, 6);
    color(v, PAL_FOLK, 2, 12, 2, 2);
    color(v, PAL_FOLK, 3, 3, 5, 10);
    color(v, PAL_FOLK, 4, 2, 2, 4);
    color(v, PAL_FOLK, 5, 1, 1, 1);
    color(v, PAL_FOLK, 6, 4, 2, 1);
    color(v, PAL_FOLK, 7, 14, 12, 9);

    color(v, PAL_HOUSE, 1, 8, 8, 7);
    color(v, PAL_HOUSE, 2, 11, 11, 10);
    color(v, PAL_HOUSE, 3, 8, 2, 2);
    color(v, PAL_HOUSE, 4, 2, 2, 3);
    color(v, PAL_HOUSE, 5, 10, 11, 12);
    color(v, PAL_HOUSE, 6, 14, 11, 3);
    color(v, PAL_HOUSE, 7, 5, 3, 2);
    color(v, PAL_HOUSE, 8, 15, 13, 6);
    color(v, PAL_HOUSE, 9, 1, 1, 1);
    color(v, PAL_HOUSE, 10, 6, 7, 8);

    color(v, PAL_GAUGE, 1, 2, 3, 6);
    color(v, PAL_GAUGE, 2, 12, 13, 14);
    color(v, PAL_GAUGE, 3, 1, 2, 4);
    color(v, PAL_GAUGE, 4, 10, 8, 5);
    color(v, PAL_GAUGE, 5, 15, 12, 2);
    color(v, PAL_GAUGE, 6, 14, 14, 15);
    color(v, PAL_GAUGE, 7, 15, 10, 4);
    color(v, PAL_GAUGE, 8, 6, 10, 14);
    color(v, PAL_GAUGE, 9, 15, 15, 14);

    color(v, PAL_HILL, 1, 7, 9, 12);
    color(v, PAL_HILL, 2, 4, 6, 9);
    color(v, PAL_HILL, 3, 15, 15, 15);
    color(v, PAL_HILL, 4, 12, 13, 14);
    color(v, PAL_HILL, 5, 9, 8, 6);
    color(v, PAL_HILL, 6, 6, 6, 5);
    color(v, PAL_HILL, 7, 4, 4, 3);
    color(v, PAL_HILL, 8, 4, 8, 3);
    color(v, PAL_HILL, 9, 2, 5, 2);
    color(v, PAL_HILL, 10, 1, 4, 1);
    color(v, PAL_HILL, 11, 3, 7, 2);
    color(v, PAL_HILL, 12, 8, 5, 3);
    color(v, PAL_HILL, 13, 9, 3, 2);
    color(v, PAL_HILL, 14, 14, 12, 5);
    color(v, PAL_HILL, 15, 2, 2, 2);
}

void loadFont(gs::TileAlloc& tiles, int font[6][96]) {
    for (int color = 1; color <= 5; color++) {
        for (int ch = 32; ch < 128; ch++) {
            uint8_t px[64] = {};
            const uint8_t* g = gs::glyph(char(ch));
            for (int y = 0; y < 7; y++)
                for (int x = 0; x < 5; x++)
                    if (g[y * 5 + x]) px[y * 8 + x] = uint8_t(color);
            for (int y = 0; y < 7; y++)
                for (int x = 0; x < 5; x++) {
                    if (!g[y * 5 + x] || x + 1 >= 8 || y + 1 >= 8) continue;
                    if (!px[(y + 1) * 8 + (x + 1)]) px[(y + 1) * 8 + (x + 1)] = 6;
                }
            font[color][ch - 32] = tiles.shared(px);
        }
    }
}

gs::Image words(gs::VDP& v, const char* s, int scale, int ink) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = ink;
    st.outline = 6;
    st.shadow = 6;
    st.spacing = 1;
    return gs::uploadImage(v, gs::textBitmap(s, st));
}

gs::Bitmap carBitmap() {
    gs::Bitmap b(40, 34);
    b.poly({{6, 22}, {34, 22}, {36, 28}, {4, 32}}, 12);
    b.ellipse(11, 28, 3.2f, 3.2f, 9);
    b.ellipse(11, 28, 1.3f, 1.3f, 10);
    b.ellipse(28, 25, 3.2f, 3.2f, 9);
    b.ellipse(28, 25, 1.3f, 1.3f, 10);
    b.rect(4, 10, 32, 13, 2);
    b.rect(4, 10, 32, 4, 3);
    b.poly({{3, 11}, {8, 5}, {32, 5}, {37, 11}}, 6);
    b.rect(8, 9, 24, 2, 7);
    b.rect(6, 12, 7, 6, 4);
    b.rect(7, 13, 2, 2, 5);
    b.rect(27, 12, 7, 6, 4);
    b.rect(28, 13, 2, 2, 5);
    b.rect(15, 12, 10, 11, 13);
    b.rect(17, 14, 6, 5, 4);
    b.line(20, 12, 20, 23, 15, 1);
    b.rect(3, 22, 34, 2, 8);
    b.ellipse(35, 16, 1.8f, 1.8f, 14);
    b.outline(15, false);
    return b;
}

gs::Bitmap deckBitmap() {
    // Lip is the left edge. The shelter sits to the right so the car can meet the deck.
    gs::Bitmap b(48, 30);
    b.poly({{16, 8}, {22, 2}, {46, 2}, {46, 8}}, 4);
    b.rect(18, 7, 26, 3, 5);
    b.rect(20, 10, 2, 8, 3);
    b.rect(40, 10, 2, 8, 3);
    b.rect(0, 16, 46, 6, 1);
    b.rect(0, 16, 46, 2, 2);
    for (int i = 0; i < 6; i++) b.rect(0, float(14 + i), 7, 1, i % 2 ? 9 : 8);
    b.rect(22, 22, 3, 6, 3);
    b.rect(40, 22, 3, 6, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap houseBitmap() {
    gs::Bitmap b(48, 42);
    b.rect(6, 16, 36, 22, 1);
    b.rect(8, 16, 32, 3, 2);
    b.poly({{2, 18}, {24, 4}, {46, 18}}, 3);
    b.ellipse(24, 26, 9, 9, 4);
    b.ellipse(24, 26, 6, 6, 5);
    b.ellipse(24, 26, 2, 2, 6);
    b.line(15, 26, 33, 26, 6, 1);
    b.line(24, 17, 24, 35, 6, 1);
    b.line(18, 20, 30, 32, 6, 1);
    b.line(30, 20, 18, 32, 6, 1);
    b.rect(20, 32, 8, 8, 7);
    b.rect(28, 18, 6, 5, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap personBitmap(int coat) {
    gs::Bitmap b(12, 18);
    b.ellipse(6, 4, 2.3f, 2.3f, 1);
    b.rect(4, 1, 4, 2, 6);
    b.rect(3, 7, 6, 6, coat);
    b.rect(2, 8, 2, 4, coat);
    b.rect(8, 8, 2, 4, coat);
    b.rect(3, 13, 2, 4, 4);
    b.rect(7, 13, 2, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap gaugeBitmap() {
    gs::Bitmap b(128, 70);
    b.rect(0, 0, 128, 70, 2);
    b.rect(3, 3, 122, 64, 1);
    b.rect(34, 16, 88, 48, 3);
    b.rect(78, 36, 40, 10, 4);
    b.rect(78, 36, 40, 2, 5);
    b.rect(70, 33, 10, 8, 5);
    b.line(36, 38, 120, 38, 6, 1);
    b.line(18, 24, 26, 16, 7, 1);
    b.line(26, 16, 34, 24, 7, 1);
    b.line(18, 52, 26, 60, 8, 1);
    b.line(26, 60, 34, 52, 8, 1);
    return b;
}

gs::Bitmap sillBitmap() {
    gs::Bitmap b(22, 6);
    b.rect(0, 0, 22, 4, 2);
    b.rect(0, 3, 22, 2, 8);
    return b;
}

gs::Bitmap beadBitmap() {
    gs::Bitmap b(5, 4);
    b.ellipse(2.5f, 2.f, 2.2f, 1.6f, 2);
    b.rect(1, 1, 2, 1, 3);
    return b;
}

gs::Bitmap bufferBitmap() {
    gs::Bitmap b(12, 16);
    b.rect(4, 2, 4, 12, 5);
    for (int i = 0; i < 6; i++) b.rect(2, float(2 + i * 2), 8, 1, i % 2 ? 6 : 5);
    b.rect(3, 13, 6, 2, 7);
    return b;
}

gs::Bitmap tickBitmap() {
    gs::Bitmap b(6, 10);
    b.rect(2, 0, 2, 10, 8);
    b.rect(0, 8, 6, 2, 9);
    return b;
}

gs::Bitmap shadowBitmap() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 13, 3.2f, 1);
    return b;
}

gs::Bitmap birdBitmap() {
    gs::Bitmap b(12, 6);
    b.line(0, 3, 4, 1, 1, 1);
    b.line(4, 1, 6, 3, 1, 1);
    b.line(6, 3, 8, 1, 1, 1);
    b.line(8, 1, 12, 3, 1, 1);
    return b;
}

gs::Bitmap cloudBitmap() {
    gs::Bitmap b(42, 16);
    b.ellipse(16, 9, 12, 5, 2);
    b.ellipse(26, 8, 11, 6, 1);
    b.ellipse(10, 10, 7, 4, 1);
    return b;
}

gs::Bitmap sunBitmap() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 5, 5, 3);
    b.ellipse(8, 8, 2.4f, 2.4f, 1);
    return b;
}

int noise(int x, int y) {
    uint32_t n = uint32_t(x * 131 + y * 977);
    n ^= n << 13;
    n ^= n >> 17;
    n ^= n << 5;
    return int(n & 255);
}

void pine(gs::Bitmap& b, float x, float y, float h) {
    b.poly({{x, y - h}, {x - h * 0.38f, y}, {x + h * 0.38f, y}}, 10);
    b.poly({{x, y - h * 0.78f}, {x - h * 0.26f, y - h * 0.34f}, {x + h * 0.26f, y - h * 0.34f}}, 11);
    b.rect(x - 1.2f, y - h * 0.12f, 2.4f, h * 0.18f, 12);
}

void chalet(gs::Bitmap& b, float x, float y, float w, float h) {
    b.rect(x - w * 0.5f, y - h, w, h, 12);
    b.poly({{x - w * 0.62f, y - h + 1}, {x, y - h - w * 0.42f}, {x + w * 0.62f, y - h + 1}}, 13);
    b.rect(x - w * 0.28f, y - h + 3, 3, 3, 14);
    b.rect(x + w * 0.08f, y - h + 3, 3, 3, 14);
    b.rect(x - 2, y - 6, 4, 6, 7);
}

void paintRails(gs::Bitmap& hill) {
    const float drop = 8.f;
    for (int side : {1, -1}) {
        for (float s = 0.f; s <= kLen; s += 0.7f) {
            float x0, y0, x1, y1;
            railPoint(s, side, x0, y0, drop - 2.2f);
            railPoint(s, side, x1, y1, drop + 2.2f);
            hill.rect(x0 - 0.5f, y0 - 0.5f, 2, 2, 4);
            hill.rect(x1 - 0.5f, y1 - 0.5f, 2, 2, 4);
        }
        for (float s = 0.6f; s < kLen; s += 2.6f) {
            float x0, y0, x1, y1;
            railPoint(s, side, x0, y0, drop - 5.f);
            railPoint(s, side, x1, y1, drop + 5.f);
            hill.line(x0, y0, x1, y1, 15, 2.f);
        }
    }
    for (float s = 6.f; s < kLen; s += 9.f) {
        float x, y;
        railPoint(s, 1, x, y, drop + 4.f);
        if (y < 198.f) hill.rect(x - 2.f, y, 4, 200.f - y, 7);
    }
}

gs::Bitmap hillBitmap() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    const float dx = kX1 - kX0, dy = kY1 - kY0;
    const float len = std::sqrt(dx * dx + dy * dy);
    auto distAt = [&](float x, float y) { return (dx * (y - kY0) - dy * (x - kX0)) / len; };
    for (int y = 0; y < gs::SCREEN_H; y++) {
        for (int x = 0; x < gs::SCREEN_W; x++) {
            float dist = distAt(float(x), float(y));
            float ridge = 96.f + 16.f * std::sin(x * 0.028f) + 8.f * std::sin(x * 0.061f + 1.2f);
            int n = noise(x, y);
            if (y >= 200) {
                b.set(x, y, 15);
                continue;
            }
            if (dist < -8.f && y > int(ridge - 34.f) && y < int(ridge)) {
                int c = (n & 3) == 0 ? 2 : 1;
                if (y < int(ridge - 26.f) && x > 150) c = (n & 1) ? 3 : 4;
                b.set(x, y, c);
                continue;
            }
            if (x > 200 && y < 78 && dist < -6.f) {
                float px = (x - 248) / 28.f, py = (y - 48) / 22.f;
                if (px * px + py * py < 1.f) {
                    b.set(x, y, py < -0.15f ? 3 : 2);
                    continue;
                }
            }
            if (dist > -46.f) {
                int c;
                if (dist < -10.f) c = (n > 180) ? 6 : 7;
                else if (dist < 22.f) c = (n > 140) ? 6 : 5;
                else if (y > 168 && x < 78) c = (n & 1) ? 1 : 2;
                else c = (n > 160) ? 9 : 8;
                if (dist > -4.f && dist < 16.f && (n & 15) == 0) c = 6;
                b.set(x, y, c);
            }
        }
    }
    pine(b, 70, 168, 28);
    pine(b, 96, 160, 22);
    pine(b, 188, 132, 26);
    pine(b, 214, 118, 18);
    pine(b, 250, 104, 22);
    for (float s : {16.f, 30.f, 72.f, 84.f}) {
        float x, y;
        railPoint(s, 1, x, y, 36.f);
        pine(b, x, y, 16.f + float(int(s) % 9));
    }
    for (float s : {22.f, 58.f}) {
        float x, y;
        railPoint(s, -1, x, y, -18.f);
        pine(b, x, y, 14.f);
    }
    {
        float x, y;
        railPoint(18.f, 1, x, y, 46.f);
        chalet(b, x, y, 18, 12);
        railPoint(74.f, 1, x, y, 42.f);
        chalet(b, x, y, 16, 11);
        railPoint(46.f, -1, x, y, -28.f);
        chalet(b, x, y, 14, 10);
    }
    paintRails(b);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    vdp.setFogColor(gs::rgb4(8, 10, 12));
    gs::TileAlloc tiles(vdp);
    loadFont(tiles, art.font);
    art.hill = gs::uploadImage(vdp, hillBitmap());
    art.car = gs::uploadMipped(vdp, carBitmap());
    art.deck = gs::uploadImage(vdp, deckBitmap());
    art.house = gs::uploadImage(vdp, houseBitmap());
    art.folk[0] = gs::uploadMipped(vdp, personBitmap(2));
    art.folk[1] = gs::uploadMipped(vdp, personBitmap(3));
    art.bead = gs::uploadImage(vdp, beadBitmap());
    art.buffer = gs::uploadImage(vdp, bufferBitmap());
    art.tick = gs::uploadImage(vdp, tickBitmap());
    art.shadow = gs::uploadImage(vdp, shadowBitmap());
    art.sill = gs::uploadImage(vdp, sillBitmap());
    art.gauge = gs::uploadImage(vdp, gaugeBitmap());
    art.bird = gs::uploadImage(vdp, birdBitmap());
    art.cloud = gs::uploadImage(vdp, cloudBitmap());
    art.sun = gs::uploadImage(vdp, sunBitmap());
    art.title = words(vdp, "S3 FUNICULAR", 2, 1);
    art.levelWord = words(vdp, "LEVEL", 3, 5);
    art.missWord = words(vdp, "MISSED", 2, 4);
    art.lipY = 38.f;
    art.sillX = 20.f;
    art.sillY = 23.f;
    art.deckX = 3.f;
    art.deckY = 16.f;
    art.houseX = 24.f;
    art.houseY = 40.f;
}

}  // namespace funicular
