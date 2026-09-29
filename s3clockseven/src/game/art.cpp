#include "game/art.h"

#include <cmath>
#include <string>

namespace c7 {
namespace {

constexpr float kTau = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void paintHand(gs::Bitmap& b, int kind, int step) {
    float th = step * (kTau / float(kHandN));
    float s = std::sin(th), c = -std::cos(th);
    float len = kind == 0 ? 14.f : kind == 1 ? 20.f : 24.f;
    float tail = kind == 2 ? 7.f : 4.f;
    float thick = kind == 0 ? 2.6f : kind == 1 ? 1.8f : 1.05f;
    b.line(kPivot - s * tail, kPivot - c * tail, kPivot + s * len, kPivot + c * len, 1, thick);
    b.line(kPivot - s * (tail * 0.4f), kPivot - c * (tail * 0.4f), kPivot + s * (len * 0.72f),
           kPivot + c * (len * 0.72f), 2, thick * 0.45f);
    if (kind == 2) {
        b.ellipse(kPivot + s * 15.f, kPivot + c * 15.f, 2.1f, 2.1f, 3);
        b.ellipse(kPivot - s * 6.f, kPivot - c * 6.f, 1.6f, 1.6f, 1);
    }
}

void paintFace(gs::Bitmap& b) {
    b.ellipse(48, 48, 46, 46, 1);
    b.ellipse(48, 48, 42, 42, 2);
    b.ellipse(48, 47, 38, 38, 3);
    b.ellipse(48, 48, 36, 36, 4);
    for (int i = 0; i < 60; i++) {
        float a = i * (kTau / 60.f);
        float s = std::sin(a), c = std::cos(a);
        bool hour = (i % 5) == 0;
        float r0 = hour ? 28.f : 32.f;
        b.line(48 + s * r0, 48 - c * r0, 48 + s * 35.f, 48 - c * 35.f, hour ? 6 : 5, hour ? 2.f : 1.f);
    }
    for (int n = 1; n <= 12; n++) {
        float a = n * (kTau / 12.f);
        gs::Bitmap g = gs::textBitmap(std::to_string(n), gs::TextStyle{1, 7, 0, 0, 1});
        float r = n >= 10 ? 22.f : 23.f;
        b.blit(g, int(std::lround(48 + std::sin(a) * r - g.w * 0.5f)),
               int(std::lround(48 - std::cos(a) * r - g.h * 0.5f)));
    }
    b.ellipse(48, 48, 3.2f, 3.2f, 2);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 1);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        a.font[c - 32] = tiles.shared(px);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 4), gs::rgb4(15, 13, 8), gs::rgb4(9, 8, 6), gs::rgb4(4, 3, 2),
            gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_HOUR, {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_MIN, {0, gs::rgb4(3, 3, 2), gs::rgb4(10, 10, 6), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_SEC, {0, gs::rgb4(8, 1, 1), gs::rgb4(14, 3, 2), gs::rgb4(15, 8, 4)});
    setPal(vdp, PAL_LIT, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 10, 2), gs::rgb4(8, 6, 1)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 8, 3), gs::rgb4(15, 13, 5), gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(4, 12, 6), gs::rgb4(8, 15, 9)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(12, 5, 3), gs::rgb4(15, 8, 4)});

    loadFont(vdp, art);

    gs::Bitmap face(96, 96);
    paintFace(face);
    art.face = gs::uploadImage(vdp, face);

    for (int kind = 0; kind < 3; kind++) {
        for (int step = 0; step < kHandN; step++) {
            gs::Bitmap hand(kPivot * 2, kPivot * 2);
            paintHand(hand, kind, step);
            art.hand[kind][step] = gs::uploadImage(vdp, hand);
        }
    }

    gs::Bitmap cap(10, 10);
    cap.ellipse(5, 5, 4, 4, 1);
    cap.ellipse(5, 5, 2, 2, 2);
    art.cap = gs::uploadImage(vdp, cap);

    gs::Bitmap bell(22, 18);
    bell.rect(9, 0, 4, 3, 3);
    bell.poly({{5, 3}, {17, 3}, {20, 13}, {2, 13}}, 1);
    bell.poly({{8, 4}, {14, 4}, {16, 11}, {6, 11}}, 2);
    bell.ellipse(11, 13, 9, 3, 1);
    bell.ellipse(11, 12, 2, 2, 4);
    art.bell = gs::uploadImage(vdp, bell);

    gs::Bitmap rope(4, 36);
    rope.rect(1, 0, 2, 28, 1);
    rope.ellipse(2, 31, 2, 3, 2);
    art.rope = gs::uploadImage(vdp, rope);

    gs::Bitmap on(10, 10);
    on.ellipse(5, 5, 4, 4, 1);
    on.ellipse(5, 5, 2, 2, 2);
    art.lampOn = gs::uploadImage(vdp, on);
    gs::Bitmap off(10, 10);
    off.ellipse(5, 5, 4, 4, 1);
    off.ellipse(5, 5, 2, 2, 3);
    art.lampOff = gs::uploadImage(vdp, off);

    gs::Bitmap star(7, 7);
    star.line(3, 0, 3, 6, 1, 1);
    star.line(0, 3, 6, 3, 1, 1);
    star.set(2, 2, 1);
    star.set(4, 2, 1);
    star.set(2, 4, 1);
    star.set(4, 4, 1);
    art.star = gs::uploadImage(vdp, star);

    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace c7
