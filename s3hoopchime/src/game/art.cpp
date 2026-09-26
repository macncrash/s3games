#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace hoopchime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
    vdp.setColor(pal * 16 + 15, edge);
}

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
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

gs::Bitmap playerArt() {
    gs::Bitmap b(40, 64);
    b.rect(8, 58, 8, 4, 7);
    b.rect(24, 58, 8, 4, 7);
    b.rect(9, 56, 6, 3, 11);
    b.rect(25, 56, 6, 3, 11);
    b.line(12, 44, 12, 58, 2, 3.2f);
    b.line(26, 44, 27, 58, 2, 3.2f);
    b.rect(10, 40, 18, 8, 6);
    b.rect(10, 40, 18, 2, 1);
    b.rect(12, 22, 16, 18, 4);
    b.rect(12, 22, 4, 18, 5);
    b.rect(16, 28, 3, 6, 8);
    b.rect(20, 28, 3, 6, 8);
    b.ellipse(20, 12, 6.2f, 6.4f, 2);
    b.ellipse(20, 9, 6.4f, 3.2f, 9);
    b.rect(14, 8, 12, 3, 10);
    b.rect(25, 12, 2, 2, 1);
    b.line(26, 24, 36, 34, 2, 2.8f);
    b.ellipse(36, 36, 2.4f, 2.2f, 2);
    b.line(14, 24, 6, 16, 2, 2.6f);
    b.ellipse(5, 14, 2.2f, 2.0f, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap ballArt(int frame) {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12.2f, 12.2f, 1);
    b.ellipse(10, 9, 4.0f, 2.8f, 2);
    b.line(14, 2, 14, 26, 3, 1.2f);
    if (frame == 0) {
        b.line(3, 9, 25, 19, 3, 1.15f);
        b.line(3, 19, 25, 9, 3, 1.15f);
    } else {
        b.line(2, 14, 26, 14, 3, 1.15f);
        b.line(6, 4, 9, 24, 3, 1.1f);
        b.line(22, 4, 19, 24, 3, 1.1f);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap rimArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(20, 8, 16.8f, 5.6f, 1);
    b.ellipse(20, 9.4f, 16.2f, 4.0f, 2);
    b.rect(34, 6, 12, 4, 3);
    b.rect(36, 7, 8, 2, 4);
    return b;
}

gs::Bitmap netArt(int sway) {
    gs::Bitmap b(34, 30);
    float lean = sway ? 2.4f : -0.8f;
    for (int i = 0; i < 7; i++) {
        float x0 = 4.f + float(i) * 4.2f;
        float x1 = 8.f + float(i) * 2.6f + lean;
        b.line(x0, 1, x1, 28, (i & 1) ? 6 : 5, 1.05f);
    }
    for (int y = 5; y < 28; y += 5) {
        float shrink = float(y) * 0.22f;
        b.line(4.f + shrink, float(y), 30.f - shrink + lean, float(y + sway), 6, 1.0f);
    }
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(54, 40);
    b.rect(1, 1, 52, 38, 1);
    b.rect(1, 1, 52, 3, 2);
    b.rect(1, 36, 52, 3, 2);
    b.rect(1, 1, 3, 38, 2);
    b.rect(50, 1, 3, 38, 2);
    b.rect(16, 8, 22, 16, 3);
    b.rect(24, 13, 6, 6, 4);
    for (int i = 0; i < 4; i++) b.rect(8.f + float(i) * 11.f, 4, 2, 2, 5);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(10, 16);
    b.rect(2, 0, 6, 16, 3);
    b.rect(3, 0, 2, 16, 4);
    return b;
}

gs::Bitmap armArt() {
    gs::Bitmap b(28, 6);
    b.rect(0, 1, 28, 4, 3);
    b.rect(0, 1, 28, 1, 4);
    return b;
}

gs::Bitmap glassArt() {
    const int n = 48;
    gs::Bitmap b(n, n);
    const float c = (n - 1) * 0.5f;
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            float dx = x + 0.5f - c;
            float dy = y + 0.5f - c;
            float r = std::hypot(dx, dy);
            int col = 0;
            if (r <= 4.6f) col = 6;
            else if (r <= 6.4f) col = 3;
            else if (r <= 15.2f) col = 1;
            else if (r <= 17.2f) col = 2;
            else if (r <= 19.4f) col = 4;
            else if (r <= 21.2f) col = 5;
            b.set(x, y, col);
        }
    }
    return b;
}

gs::Bitmap crossArt() {
    gs::Bitmap b(13, 13);
    b.rect(5, 1, 3, 11, 1);
    b.rect(1, 5, 11, 3, 1);
    b.set(6, 6, 0);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.1f, 3.1f, 1);
    return b;
}

gs::Bitmap blotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.0f, 2.2f, 1);
    return b;
}

gs::Bitmap schoolArt() {
    gs::Bitmap b(78, 108);
    b.poly({{22.f, 2.f}, {6.f, 18.f}, {40.f, 18.f}}, 6);
    b.rect(8, 16, 30, 90, 1);
    b.rect(6, 16, 34, 5, 7);
    b.rect(12, 28, 22, 20, 3);
    for (int i = 0; i < 4; i++) b.rect(16, 54.f + float(i) * 12.f, 8, 8, 4);
    b.rect(38, 58, 36, 48, 1);
    b.rect(36, 54, 40, 6, 7);
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < 2; c++) b.rect(44.f + float(c) * 14.f, 66.f + float(r) * 16.f, 8, 10, 4);
    b.rect(54, 90, 8, 16, 5);
    b.rect(14, 96, 10, 10, 5);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 8.2f, 8.2f, 1);
    b.ellipse(14, 9, 6.4f, 6.4f, 0);
    b.ellipse(8, 8, 1.3f, 1.3f, 2);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    float c = (kDial - 1) * 0.5f;
    for (int y = 0; y < kDial; y++) {
        for (int x = 0; x < kDial; x++) {
            float dx = x + 0.5f - c;
            float dy = y + 0.5f - c;
            float r = std::sqrt(dx * dx + dy * dy);
            if (r > 22.4f) continue;
            if (r > 19.2f) {
                b.set(x, y, 1);
                continue;
            }
            b.set(x, y, r < 2.4f ? 4 : 2);
            float ang = std::atan2(dx, -dy);
            if (ang < 0) ang += 6.2831853f;
            float tick = std::fmod(ang + 0.08f, 0.5235988f);
            bool hour = tick < 0.08f || tick > 0.5235988f - 0.08f;
            if (hour && r > 14.6f && r < 18.6f) b.set(x, y, 3);
            if (r > 16.8f && r < 18.2f && ang < 0.18f) b.set(x, y, 5);
        }
    }
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(kDial + 10, kDial + 10);
    float c = (b.w - 1) * 0.5f;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - c;
            float dy = y + 0.5f - c;
            float r = std::sqrt(dx * dx + dy * dy);
            if (r > 24.2f && r < 27.4f) b.set(x, y, (int(std::atan2(dy, dx) * 6.f) & 1) ? 1 : 2);
        }
    }
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(3.5f, 3.5f, 3.0f, 3.0f, 1);
    b.ellipse(3.5f, 3.5f, 1.3f, 1.3f, 2);
    return b;
}

void paintHand(gs::Bitmap& b, int kind, int step) {
    float theta = float(step) * 0.10471976f;
    float dx = std::sin(theta);
    float dy = -std::cos(theta);
    float len = kind == 0 ? 11.f : kind == 1 ? 16.f : 19.f;
    float tail = kind == 2 ? 2.0f : 3.0f;
    float thick = kind == 2 ? 0.55f : 1.35f;
    int col = kind == 2 ? 3 : kind == 1 ? 2 : 1;
    float c = (kDial - 1) * 0.5f;
    for (int y = 0; y < kDial; y++) {
        for (int x = 0; x < kDial; x++) {
            float px = x + 0.5f - c;
            float py = y + 0.5f - c;
            float along = px * dx + py * dy;
            float side = std::fabs(px * -dy + py * dx);
            if (along > -tail && along < len && side <= thick) b.set(x, y, col);
        }
    }
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 22);
    b.rect(8, 0, 2, 3, 4);
    b.ellipse(9.f, 6.f, 3.4f, 2.4f, 1);
    b.ellipse(9.f, 12.f, 7.2f, 5.6f, 1);
    b.ellipse(9.f, 11.4f, 4.8f, 3.6f, 2);
    b.ellipse(7.f, 10.f, 1.5f, 1.6f, 3);
    b.rect(2, 16, 14, 3, 1);
    b.rect(3, 16, 12, 1, 3);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(6, 9);
    b.rect(2, 0, 2, 4, 1);
    b.ellipse(3.f, 6.4f, 2.0f, 2.0f, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_COURT,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 9), gs::rgb4(8, 8, 9), gs::rgb4(15, 15, 12),
            gs::rgb4(12, 10, 6), gs::rgb4(9, 8, 7), gs::rgb4(4, 3, 3)});
    setPal(vdp, PAL_BOARD,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(6, 7, 8), gs::rgb4(12, 3, 2), gs::rgb4(14, 14, 13), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(13, 5, 1), gs::rgb4(8, 3, 1), gs::rgb4(7, 7, 8), gs::rgb4(12, 12, 13), gs::rgb4(14, 14, 14),
            gs::rgb4(9, 9, 10)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(14, 6, 1), gs::rgb4(15, 12, 6), gs::rgb4(5, 2, 1), gs::rgb4(6, 2, 0)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 8, 5), gs::rgb4(9, 5, 3), gs::rgb4(14, 11, 3), gs::rgb4(10, 7, 1),
            gs::rgb4(2, 3, 7), gs::rgb4(3, 3, 4), gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 1), gs::rgb4(12, 2, 2),
            gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_TOWER,
           {0, gs::rgb4(8, 4, 3), gs::rgb4(5, 3, 3), gs::rgb4(4, 3, 4), gs::rgb4(15, 12, 5), gs::rgb4(3, 2, 2),
            gs::rgb4(4, 2, 2), gs::rgb4(10, 9, 8), gs::rgb4(12, 8, 6)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 13), gs::rgb4(2, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(0, 2, 1));
    setPal(vdp, PAL_GLASS,
           {0, gs::rgb4(2, 5, 4), gs::rgb4(8, 10, 9), gs::rgb4(13, 5, 1), gs::rgb4(12, 11, 8), gs::rgb4(4, 4, 5),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_METER, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 13, 5), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 8), gs::rgb4(2, 1, 1), gs::rgb4(5, 3, 2), gs::rgb4(15, 12, 4),
            gs::rgb4(10, 8, 5)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 1, 1), gs::rgb4(4, 3, 3), gs::rgb4(13, 2, 2), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(10, 8, 3), gs::rgb4(6, 4, 2), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 2)});
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 8), gs::rgb4(3, 2, 0));

    loadFont(vdp, art);
    art.player = gs::uploadMipped(vdp, playerArt());
    art.ball[0] = gs::uploadMipped(vdp, ballArt(0));
    art.ball[1] = gs::uploadMipped(vdp, ballArt(1));
    art.rim = gs::uploadImage(vdp, rimArt());
    art.net[0] = gs::uploadImage(vdp, netArt(0));
    art.net[1] = gs::uploadImage(vdp, netArt(1));
    art.board = gs::uploadImage(vdp, boardArt());
    art.pole = gs::uploadImage(vdp, poleArt());
    art.arm = gs::uploadImage(vdp, armArt());
    art.glass = gs::uploadImage(vdp, glassArt());
    art.cross = gs::uploadImage(vdp, crossArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.blot = gs::uploadImage(vdp, blotArt());
    art.shadow = gs::uploadImage(vdp, shadowArt());
    art.school = gs::uploadImage(vdp, schoolArt());
    art.moon = gs::uploadImage(vdp, moonArt());
    art.face = gs::uploadImage(vdp, faceArt());
    art.ring = gs::uploadImage(vdp, ringArt());
    art.cap = gs::uploadImage(vdp, capArt());
    for (int kind = 0; kind < 3; kind++) {
        for (int step = 0; step < 60; step++) {
            gs::Bitmap b(kDial, kDial);
            paintHand(b, kind, step);
            art.hand[kind][step] = gs::uploadImage(vdp, b);
        }
    }
    art.bell = gs::uploadImage(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.hoopW = phrase(vdp, "HOOP", 3);
    art.chimeW = phrase(vdp, "CHIME", 3);
    art.twelveW = phrase(vdp, "TWELVE", 2);
    art.earlyW = phrase(vdp, "EARLY", 2);
    art.lateW = phrase(vdp, "LATE", 2);
    art.shortW = phrase(vdp, "SHORT", 2);
    art.hotW = phrase(vdp, "HOT", 2);
    art.wideW = phrase(vdp, "WIDE", 2);
    art.ironW = phrase(vdp, "IRON", 2);
    art.missW = phrase(vdp, "MISS", 2);
}

}  // namespace hoopchime
