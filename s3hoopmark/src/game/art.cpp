#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace hoopmark {
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

void disc(gs::Bitmap& b, float cx, float cy, float rx, float ry, int c) {
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = (x + 0.5f - cx) / rx;
            float dy = (y + 0.5f - cy) / ry;
            if (dx * dx + dy * dy <= 1.f) b.set(x, y, c);
        }
    }
}

gs::Bitmap shooterArt() {
    gs::Bitmap b(36, 54);
    b.line(15, 38, 12, 48, 8, 3.2f);
    b.line(22, 38, 26, 48, 8, 3.2f);
    b.rect(8, 47, 8, 4, 9);
    b.rect(22, 47, 9, 4, 9);
    b.rect(8, 47, 8, 1, 1);
    b.rect(22, 47, 9, 1, 1);
    b.rect(12, 32, 13, 8, 8);
    b.rect(18, 32, 2, 8, 6);
    b.rect(11, 18, 15, 16, 5);
    b.rect(11, 18, 15, 3, 7);
    b.rect(11, 31, 15, 2, 7);
    b.rect(16, 22, 2, 7, 10);
    b.rect(20, 22, 5, 7, 10);
    b.rect(21, 24, 3, 3, 5);
    disc(b, 18, 11, 6.6f, 6.4f, 2);
    disc(b, 18, 8.5f, 6.8f, 3.6f, 4);
    b.rect(22, 11, 2, 2, 11);
    b.rect(23, 11, 1, 1, 1);
    b.line(25, 22, 31, 34, 2, 2.8f);
    disc(b, 32, 35, 2.1f, 2.0f, 2);
    b.line(13, 22, 7, 30, 2, 2.6f);
    b.outline(1, false);
    return b;
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(26, 26);
    disc(b, 13, 13, 11.2f, 11.2f, 1);
    disc(b, 10, 9, 3.6f, 2.6f, 2);
    b.line(13, 3, 13, 23, 3, 1.15f);
    if (seam == 0) {
        b.line(4, 8, 22, 18, 3, 1.1f);
        b.line(4, 18, 22, 8, 3, 1.1f);
    } else {
        b.line(4, 13, 22, 13, 3, 1.1f);
        b.line(7, 5, 8, 21, 3, 1.05f);
        b.line(19, 5, 18, 21, 3, 1.05f);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(48, 34);
    b.rect(0, 0, 48, 34, 6);
    b.rect(2, 2, 44, 30, 2);
    b.rect(4, 4, 40, 26, 1);
    b.rect(14, 7, 20, 14, 3);
    b.rect(16, 9, 16, 10, 4);
    b.rect(22, 12, 4, 4, 3);
    for (int y = 6; y < 28; y += 6) b.rect(6, y, 36, 1, 5);
    return b;
}

gs::Bitmap rimArt() {
    gs::Bitmap b(42, 16);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < 34; x++) {
            float dx = (x + 0.5f - 16.f) / 14.5f;
            float dy = (y + 0.5f - 8.f) / 5.6f;
            float e = dx * dx + dy * dy;
            float ix = (x + 0.5f - 16.f) / 8.4f;
            float iy = (y + 0.5f - 8.f) / 2.5f;
            float inner = ix * ix + iy * iy;
            if (e > 1.f || inner < 1.f) continue;
            b.set(x, y, y < 8 ? 1 : 2);
        }
    }
    b.rect(30, 6, 11, 4, 5);
    b.rect(31, 7, 8, 2, 6);
    return b;
}

gs::Bitmap netArt(int sway) {
    gs::Bitmap b(30, 28);
    float lean = sway ? 2.2f : -0.6f;
    for (int i = 0; i < 7; i++) {
        float x0 = 3.f + i * 3.8f;
        float x1 = 7.f + i * 2.4f + lean;
        b.line(x0, 1, x1, 26, 3, 1.05f);
    }
    for (int y = 4; y < 26; y += 5) {
        float shrink = (y - 2) * 0.16f;
        b.line(3 + shrink, float(y), 26 - shrink + lean, float(y + (sway ? 1 : 0)), 4, 1.0f);
    }
    return b;
}

gs::Bitmap glassArt() {
    const int n = 64;
    gs::Bitmap b(n, n);
    const float c = (n - 1) * 0.5f;
    const float rimR = 12.4f;
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            float dx = (x + 0.5f) - c;
            float dy = (y + 0.5f) - c;
            float r = std::hypot(dx, dy);
            int col = 0;
            if (r <= rimR + 8.f) {
                if (r > rimR + 6.2f) col = 6;
                else if (r > rimR + 4.4f) col = ((x + y) & 3) == 0 ? 7 : 5;
                else if (r > rimR + 1.2f) col = 3;
                else if (r > rimR - 1.4f) col = 2;
                else col = ((int(dx / 7.f) + int(dy / 6.f)) & 1) ? 4 : 1;
            }
            b.set(x, y, col);
        }
    }
    return b;
}

gs::Bitmap treesArt() {
    gs::Bitmap b(320, 46);
    for (int i = 0; i < 14; i++) {
        float cx = 16.f + i * 22.f;
        float h = 16.f + float((i * 7) % 10);
        b.ellipse(cx, 40.f, 13.f + (i % 3) * 2.f, h, (i & 1) ? 1 : 2);
        b.rect(cx - 1.5f, 34.f, 3.f, 12.f, 3);
    }
    for (int h = 0; h < 3; h++) {
        float x = 48.f + h * 96.f;
        b.rect(x, 22.f, 22.f, 16.f, 5);
        b.rect(x + 3, 26.f, 4.f, 5.f, 6);
        b.rect(x + 14, 26.f, 4.f, 5.f, 6);
        b.rect(x + 6, 14.f, 10.f, 8.f, 3);
    }
    b.rect(0, 40, 320, 2, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 70);
    b.rect(7, 16, 3, 54, 1);
    b.rect(3, 10, 10, 8, 2);
    b.rect(5, 8, 6, 4, 3);
    b.ellipse(8, 11, 2.2f, 1.8f, 4);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (std::hypot(x - 7.5f, y - 8.f) <= 6.2f) b.set(x, y, 4);
            if (std::hypot(x - 11.f, y - 6.f) <= 5.3f) b.set(x, y, 0);
        }
    }
    b.set(5, 8, 2);
    b.set(6, 11, 2);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(16, 16);
    disc(b, 8, 8, 7.1f, 7.1f, 1);
    disc(b, 8, 8, 5.1f, 5.1f, 2);
    disc(b, 8, 8, 2.0f, 2.0f, 3);
    disc(b, 6, 6, 1.3f, 1.0f, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(40, 40);
    float c = 19.5f;
    for (int y = 0; y < 40; y++) {
        for (int x = 0; x < 40; x++) {
            float r = std::hypot(x + 0.5f - c, y + 0.5f - c);
            if (r > 15.2f && r < 18.6f) b.set(x, y, 1);
            else if (r > 13.6f && r <= 15.2f) b.set(x, y, 2);
        }
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_PARK,
           {0, gs::rgb4(1, 4, 2), gs::rgb4(2, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 4),
            gs::rgb4(12, 10, 6), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_GLASS,
           {0, gs::rgb4(11, 13, 14), gs::rgb4(15, 15, 15), gs::rgb4(13, 2, 2), gs::rgb4(14, 15, 15),
            gs::rgb4(8, 12, 14), gs::rgb4(4, 5, 6), gs::rgb4(9, 9, 10), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(15, 7, 2), gs::rgb4(10, 4, 1), gs::rgb4(13, 13, 14), gs::rgb4(8, 8, 9), gs::rgb4(6, 6, 7),
            gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 12, 5), gs::rgb4(2, 1, 1), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 9, 6), gs::rgb4(9, 6, 4), gs::rgb4(2, 1, 1), gs::rgb4(3, 5, 12),
            gs::rgb4(2, 3, 8), gs::rgb4(15, 12, 3), gs::rgb4(2, 2, 3), gs::rgb4(14, 14, 15), gs::rgb4(15, 14, 8),
            gs::rgb4(15, 15, 15)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7), gs::rgb4(0, 2, 1));
    setPal(vdp, PAL_JUDGE,
           {0, gs::rgb4(1, 2, 6), gs::rgb4(4, 4, 5), gs::rgb4(15, 7, 2), gs::rgb4(8, 12, 15), gs::rgb4(10, 12, 14),
            gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 10)});
    setPal(vdp, PAL_METER, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 14, 5), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 7), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(15, 12, 2), gs::rgb4(11, 7, 1), gs::rgb4(15, 15, 8), gs::rgb4(4, 2, 0)});

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.pole = gs::uploadImage(vdp, [] {
        gs::Bitmap b(8, 8);
        b.rect(0, 0, 8, 8, 6);
        b.rect(2, 0, 4, 8, 5);
        return b;
    }());
    art.rim = gs::uploadImage(vdp, rimArt());
    art.net[0] = gs::uploadImage(vdp, netArt(0));
    art.net[1] = gs::uploadImage(vdp, netArt(1));
    art.ball[0] = gs::uploadImage(vdp, ballArt(0));
    art.ball[1] = gs::uploadImage(vdp, ballArt(1));
    art.shooter = gs::uploadImage(vdp, shooterArt());
    art.glass = gs::uploadImage(vdp, glassArt());
    art.pip = gs::uploadImage(vdp, [] {
        gs::Bitmap b(10, 10);
        disc(b, 5, 5, 4.0f, 4.0f, 1);
        disc(b, 4, 4, 1.3f, 1.0f, 2);
        b.outline(4, false);
        return b;
    }());
    art.bracket = gs::uploadImage(vdp, [] {
        gs::Bitmap b(11, 11);
        b.rect(4, 0, 3, 2, 1);
        b.rect(4, 9, 3, 2, 1);
        b.rect(0, 4, 2, 3, 1);
        b.rect(9, 4, 2, 3, 1);
        b.set(5, 5, 1);
        return b;
    }());
    for (int i = 1; i <= 3; i++) {
        gs::Bitmap b(1, 1);
        b.set(0, 0, i);
        art.blot[i] = gs::uploadImage(vdp, b);
    }
    art.shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap b(18, 6);
        b.ellipse(9, 3, 8, 2.4f, 1);
        return b;
    }());
    art.trees = gs::uploadImage(vdp, treesArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.moon = gs::uploadImage(vdp, moonArt());
    art.coin = gs::uploadImage(vdp, coinArt());
    art.ring = gs::uploadImage(vdp, ringArt());
    art.title = phrase(vdp, "HOOPMARK", 3);
    art.swish = phrase(vdp, "SWISH", 2);
    art.count = phrase(vdp, "COUNT IT", 2);
    art.bank = phrase(vdp, "BANK", 2);
    art.rimWord = phrase(vdp, "RIM", 2);
    art.shortWord = phrase(vdp, "SHORT", 2);
    art.longWord = phrase(vdp, "LONG", 2);
    art.airWord = phrase(vdp, "AIR", 2);
    art.liftWord = phrase(vdp, "LIFT THE COIN", 2);
    art.leaveWord = phrase(vdp, "LEAVE", 2);
    art.finished = phrase(vdp, "FINISHED", 2);
    art.stillOpen = phrase(vdp, "STILL OPEN", 2);
    art.pauseWord = phrase(vdp, "PAUSE", 2);
}

}  // namespace hoopmark
