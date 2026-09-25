#include "sprites.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace juggle {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap ballArt(int phase) {
    Bitmap b(40, 40);
    const float cx = 19.5f, cy = 19.5f, R = 16.f;
    for (int y = 0; y < 40; y++) {
        for (int x = 0; x < 40; x++) {
            float dx = x - cx, dy = y - cy;
            float r = std::sqrt(dx * dx + dy * dy);
            if (r > R) continue;
            float rim = r / R;
            float light = (-0.45f * dx - 0.75f * dy) / R;
            bool stripe = ((x + phase * 5) / 6) % 2 == 0;
            int c = 3;
            if (rim > 0.9f) c = 5;
            else if (light > 0.45f && rim < 0.5f) c = 1;
            else if (light > 0.12f) c = 2;
            else if (light > -0.28f) c = 3;
            else c = 4;
            if (stripe && rim < 0.9f && rim > 0.25f) c = (c >= 4) ? 7 : 6;
            b.set(x, y, c);
        }
    }
    b.ellipse(14, 13, 3.2f, 2.2f, 1);
    return b;
}

Bitmap ringArt() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 1);
    b.ellipse(18, 18, 11, 11, 0);
    return b;
}

Bitmap starArt() {
    Bitmap b(16, 16);
    b.line(8, 1, 8, 15, 1, 2);
    b.line(1, 8, 15, 8, 1, 2);
    b.line(3, 3, 13, 13, 1, 1.4f);
    b.line(13, 3, 3, 13, 1, 1.4f);
    b.ellipse(8, 8, 2.2f, 2.2f, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

Bitmap curtainArt() {
    Bitmap b(56, 150);
    for (int y = 0; y < 150; y++) {
        for (int x = 0; x < 56; x++) {
            int fold = (x / 7) % 2;
            int c = (x % 7 == 0) ? 3 : fold ? 2 : 1;
            if (y > 136) c = 3;
            b.set(x, y, c);
        }
    }
    // Tie-back on the inner edge. The right curtain is this sprite flipped.
    b.rect(40, 48, 14, 8, 4);
    b.ellipse(50, 70, 6, 10, 4);
    b.ellipse(50, 78, 3, 5, 5);
    for (int i = 0; i < 5; i++) b.line(44.f + i * 2.f, 136.f, 43.f + i * 2.f, 148.f, 4, 1.2f);
    return b;
}

Bitmap valanceArt() {
    Bitmap b(320, 36);
    for (int y = 0; y < 36; y++) {
        for (int x = 0; x < 320; x++) {
            float scallop = std::fabs(std::sin(x * 0.09f)) * 6.f;
            int c = ((x / 12) % 2) ? 2 : 1;
            if (y > 24 + scallop) c = 0;
            else if (y > 21 + scallop) c = 4;
            b.set(x, y, c);
        }
    }
    b.rect(0, 0, 320, 4, 3);
    return b;
}

Bitmap lampArt() {
    Bitmap b(18, 22);
    b.rect(8, 10, 2, 10, 8);
    b.ellipse(9, 8, 7, 5, 6);
    b.ellipse(8, 7, 3, 2, 9);
    b.rect(4, 20, 10, 2, 5);
    return b;
}

Bitmap poolArt() {
    Bitmap b(168, 28);
    b.ellipse(84, 16, 80, 12, 1);
    b.ellipse(84, 15, 46, 7, 2);
    return b;
}

// Chibi juggler. Gloves sit on the anchor line so the balls meet the hands.
Bitmap bodyArt() {
    Bitmap b(kBodyW, kBodyH);
    // Legs and shoes.
    b.poly({{70, 112}, {82, 112}, {78, 140}, {66, 140}}, 9);
    b.poly({{94, 112}, {106, 112}, {112, 140}, {98, 140}}, 9);
    b.ellipse(72, 142, 12, 6, 11);
    b.ellipse(106, 142, 12, 6, 11);
    b.rect(62, 144, 20, 3, 15);
    b.rect(96, 144, 22, 3, 15);

    // Coat tails.
    b.poly({{64, 100}, {78, 96}, {76, 124}, {60, 128}}, 6);
    b.poly({{112, 100}, {98, 96}, {100, 124}, {116, 128}}, 6);

    // Sleeves, then skin forearms.
    b.poly({{62, 70}, {78, 66}, {46, 100}, {34, 92}}, 7);
    b.poly({{114, 70}, {98, 66}, {130, 100}, {142, 92}}, 7);
    b.poly({{34, 92}, {48, 102}, {30, 112}, {18, 104}}, 2);
    b.poly({{142, 92}, {128, 102}, {146, 112}, {158, 104}}, 2);

    // Vest over the sleeves' inner ends.
    b.poly({{68, 64}, {108, 64}, {114, 114}, {62, 114}}, 5);
    b.poly({{88, 66}, {100, 92}, {76, 92}}, 7);
    b.ellipse(88, 78, 2.2f, 2.2f, 12);
    b.ellipse(88, 90, 2.2f, 2.2f, 12);
    b.ellipse(88, 102, 2.2f, 2.2f, 12);
    b.rect(74, 108, 28, 5, 12);

    // Bow tie.
    b.poly({{78, 62}, {88, 68}, {78, 74}}, 5);
    b.poly({{98, 62}, {88, 68}, {98, 74}}, 5);
    b.ellipse(88, 68, 3, 3, 12);

    // Neck, head, hair, ears.
    b.rect(82, 54, 12, 12, 2);
    b.ellipse(88, 46, 16, 17, 2);
    b.ellipse(88, 36, 17, 11, 4);
    b.poly({{72, 40}, {80, 28}, {96, 28}, {104, 40}}, 4);
    b.ellipse(72, 48, 4, 5, 2);
    b.ellipse(104, 48, 4, 5, 2);
    b.ellipse(72, 48, 2, 3, 3);
    b.ellipse(104, 48, 2, 3, 3);

    // Face.
    b.ellipse(82, 46, 2.4f, 2.6f, 15);
    b.ellipse(94, 46, 2.4f, 2.6f, 15);
    b.ellipse(82.6f, 46.2f, 1.1f, 1.2f, 7);
    b.ellipse(94.6f, 46.2f, 1.1f, 1.2f, 7);
    b.line(82, 54, 88, 57, 13, 1.3f);
    b.line(88, 57, 94, 54, 13, 1.3f);
    b.ellipse(76, 52, 3, 2, 13);
    b.ellipse(100, 52, 3, 2, 13);

    // Gloves last, centered on the catch points.
    b.ellipse(kGloveL, kGloveY, 11, 8, 7);
    b.ellipse(kGloveR, kGloveY, 11, 8, 7);
    b.ellipse(kGloveL - 6, kGloveY - 4, 4, 3, 7);
    b.ellipse(kGloveR + 6, kGloveY - 4, 4, 3, 7);
    b.ellipse(kGloveL - 2, kGloveY - 2, 3, 2, 8);
    b.ellipse(kGloveR + 2, kGloveY - 2, 3, 2, 8);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64];
        for (int i = 0; i < 64; i++) px[i] = 2;
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
            }
        }
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                int sy = y + 1, sx = x + 2;
                if (sy < 8 && sx < 8 && px[sy * 8 + sx] == 2) px[sy * 8 + sx] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t panel = gs::rgb4(3, 1, 5);
    const uint16_t ink = gs::rgb4(15, 15, 15);
    const uint16_t shade = gs::rgb4(1, 0, 2);
    auto textPal = [&](int pal, uint16_t main) {
        setPal(vdp, pal, {0, main, panel, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    };
    textPal(PAL_HUD, ink);
    textPal(PAL_GOLD, gs::rgb4(15, 13, 4));
    textPal(PAL_RED, gs::rgb4(15, 5, 4));
    textPal(PAL_GREEN, gs::rgb4(8, 15, 7));
    textPal(PAL_DIM, gs::rgb4(10, 9, 12));

    setPal(vdp, PAL_BODY,
           {0, gs::rgb4(15, 12, 9), gs::rgb4(13, 9, 6), gs::rgb4(9, 6, 4), gs::rgb4(4, 2, 2), gs::rgb4(12, 2, 3),
            gs::rgb4(8, 1, 2), gs::rgb4(15, 14, 13), gs::rgb4(12, 11, 12), gs::rgb4(2, 2, 4), gs::rgb4(4, 4, 6),
            gs::rgb4(5, 3, 2), gs::rgb4(14, 11, 4), gs::rgb4(13, 6, 6), gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2)});

    auto ballPal = [&](int pal, uint16_t hi, uint16_t lit, uint16_t mid, uint16_t lo, uint16_t edge, uint16_t band,
                       uint16_t bandLo) {
        setPal(vdp, pal, {0, hi, lit, mid, lo, edge, band, bandLo, 0, 0, 0, 0, 0, 0, 0, shade});
    };
    ballPal(PAL_BALL0, gs::rgb4(15, 14, 13), gs::rgb4(15, 7, 6), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
            gs::rgb4(4, 0, 0), gs::rgb4(15, 12, 4), gs::rgb4(10, 7, 1));
    ballPal(PAL_BALL1, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 6), gs::rgb4(14, 10, 2), gs::rgb4(9, 6, 1),
            gs::rgb4(5, 3, 0), gs::rgb4(15, 15, 12), gs::rgb4(8, 7, 3));
    ballPal(PAL_BALL2, gs::rgb4(13, 15, 15), gs::rgb4(6, 14, 13), gs::rgb4(2, 11, 11), gs::rgb4(1, 6, 7),
            gs::rgb4(0, 3, 4), gs::rgb4(15, 14, 8), gs::rgb4(8, 8, 3));

    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(12, 3, 5), gs::rgb4(8, 1, 3), gs::rgb4(4, 0, 2), gs::rgb4(14, 11, 4), gs::rgb4(8, 6, 2),
            gs::rgb4(15, 14, 10), gs::rgb4(10, 8, 4), gs::rgb4(2, 2, 3), gs::rgb4(15, 12, 8), 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 12, 7), gs::rgb4(15, 15, 12), gs::rgb4(15, 15, 15), gs::rgb4(15, 13, 5), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, shade});

    loadFont(vdp, art);
    art.body = gs::uploadMipped(vdp, bodyArt());
    for (int i = 0; i < 4; i++) art.ball[i] = gs::uploadMipped(vdp, ballArt(i));
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.curtain = gs::uploadMipped(vdp, curtainArt());
    art.valance = gs::uploadMipped(vdp, valanceArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pool = gs::uploadMipped(vdp, poolArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace juggle
