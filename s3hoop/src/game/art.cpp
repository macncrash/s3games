#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace hoop {
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

gs::Bitmap shooterArt() {
    gs::Bitmap b(40, 56);
    b.line(14, 26, 7, 36, 6, 3.4f);
    b.rect(12, 22, 16, 16, 5);
    b.rect(12, 22, 5, 16, 6);
    b.rect(15, 27, 2, 8, 10);
    b.rect(15, 27, 5, 2, 10);
    b.rect(18, 27, 2, 8, 10);
    b.rect(15, 30, 5, 2, 10);
    b.rect(15, 33, 6, 2, 10);
    b.rect(12, 38, 16, 8, 7);
    b.rect(19, 38, 2, 8, 8);
    b.ellipse(20, 13, 7.4f, 7.2f, 2);
    b.ellipse(20, 10, 7.6f, 4.6f, 4);
    b.rect(24, 13, 2, 2, 1);
    b.rect(25, 13, 1, 1, 11);
    b.line(26, 24, 36, 14, 2, 3.2f);
    b.ellipse(36, 13, 2.4f, 2.2f, 2);
    b.line(16, 46, 12, 52, 2, 3.6f);
    b.line(24, 46, 29, 52, 2, 3.6f);
    b.rect(9, 51, 8, 4, 9);
    b.rect(26, 51, 9, 4, 9);
    b.rect(9, 51, 8, 1, 11);
    b.rect(26, 51, 9, 1, 11);
    b.outline(1, false);
    return b;
}

gs::Bitmap ballArt(int seam) {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12.2f, 12.2f, 1);
    b.ellipse(11, 10, 4.2f, 3.0f, 2);
    b.line(14, 3, 14, 25, 3, 1.15f);
    if (seam == 0) {
        b.line(4, 9, 24, 19, 3, 1.15f);
        b.line(4, 19, 24, 9, 3, 1.15f);
    } else {
        b.line(5, 14, 23, 14, 3, 1.15f);
        b.ellipse(14, 14, 5.5f, 11.2f, 3);
        b.ellipse(14, 14, 12.2f, 12.2f, 1);
        b.ellipse(11, 10, 4.2f, 3.0f, 2);
        b.line(14, 3, 14, 25, 3, 1.15f);
        b.line(5, 14, 23, 14, 3, 1.15f);
        b.line(6, 7, 8, 21, 3, 1.05f);
        b.line(22, 7, 20, 21, 3, 1.05f);
    }
    b.outline(4, false);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(52, 36);
    b.rect(1, 1, 50, 34, 1);
    b.rect(1, 1, 50, 3, 2);
    b.rect(1, 32, 50, 3, 2);
    b.rect(1, 1, 3, 34, 2);
    b.rect(48, 1, 3, 34, 2);
    b.rect(14, 8, 24, 18, 3);
    b.rect(16, 10, 20, 14, 1);
    b.rect(24, 15, 4, 4, 4);
    for (int y = 4; y < 32; y += 5) b.rect(4, y, 44, 1, 5);
    return b;
}

gs::Bitmap rimArt() {
    gs::Bitmap b(38, 16);
    b.ellipse(18, 8, 16, 6.2f, 1);
    b.ellipse(18, 9, 16, 5.4f, 2);
    b.ellipse(18, 8, 11, 3.1f, 0);
    b.rect(32, 6, 6, 4, 3);
    b.rect(33, 7, 4, 2, 1);
    return b;
}

gs::Bitmap netArt(int sway) {
    gs::Bitmap b(32, 30);
    float lean = sway ? 1.6f : -0.4f;
    for (int i = 0; i < 7; i++) {
        float x0 = 4.f + i * 4.f;
        float x1 = 8.f + i * 2.6f + lean;
        b.line(x0, 1, x1, 28, 1, 1.05f);
    }
    for (int y = 4; y < 28; y += 5) {
        float shrink = (y - 2) * 0.18f;
        b.line(4 + shrink, float(y), 28 - shrink + lean, float(y + (sway ? 1 : 0)), 2, 1.0f);
    }
    return b;
}

gs::Bitmap glassArt() {
    const int n = 86;
    gs::Bitmap b(n, n);
    const float c = (n - 1) * 0.5f;
    const float rimR = 16.6f;
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            float dx = (x + 0.5f) - c;
            float dy = (y + 0.5f) - c;
            float r = std::hypot(dx, dy);
            int col = 0;
            if (r <= rimR + 10.f) {
                if (r > rimR + 7.2f) col = 6;
                else if (r > rimR + 5.4f) col = ((x + y) & 3) == 0 ? 8 : 7;
                else if (r > rimR + 1.3f) col = ((int(r * 3) + x) & 1) ? 1 : 2;
                else if (r > rimR - 1.6f) col = 3;
                else {
                    col = 4;
                    float nx = dx / 8.f;
                    float ny = dy / 7.f;
                    if (std::fabs(nx - std::round(nx)) < 0.08f || std::fabs(ny - std::round(ny)) < 0.09f) col = 5;
                }
            }
            if (r > rimR + 9.2f && r < rimR + 10.6f) col = 8;
            b.set(x, y, col);
        }
    }
    for (int k = 0; k < 4; k++) {
        float a = k * 1.5708f + 0.4f;
        b.ellipse(c + std::cos(a) * (rimR + 6.4f), c + std::sin(a) * (rimR + 6.4f), 1.6f, 1.6f, 9);
    }
    return b;
}

gs::Bitmap treesArt() {
    gs::Bitmap b(320, 52);
    for (int i = 0; i < 18; i++) {
        float cx = 12.f + i * 18.f;
        float h = 18.f + float((i * 5) % 9);
        b.ellipse(cx, 46.f, 14.f + (i % 3) * 2.f, h, (i % 2) ? 1 : 2);
        b.rect(cx - 1.5f, 40.f, 3.f, 12.f, 3);
    }
    for (int x = 0; x < 320; x += 6) {
        b.rect(float(x), 44.f, 1.f, 8.f, 4);
        b.rect(0, 46.f, 320, 1, 4);
    }
    for (int h = 0; h < 5; h++) {
        float x = 36.f + h * 62.f;
        b.rect(x, 28.f, 16.f, 18.f, 5);
        b.rect(x + 3, 32.f, 3.f, 4.f, 6);
        b.rect(x + 9, 32.f, 3.f, 4.f, 6);
        b.rect(x + 4, 18.f, 8.f, 10.f, 3);
    }
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 64);
    b.rect(8, 14, 3, 50, 1);
    b.poly({{2, 16}, {16, 16}, {13, 6}, {5, 6}}, 2);
    b.rect(6, 6, 6, 3, 3);
    b.ellipse(9, 8, 2.2f, 1.6f, 4);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7.5f, 7.5f, 1);
    b.ellipse(12, 7, 6.2f, 6.2f, 0);
    b.set(6, 8, 2);
    b.set(7, 12, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_PARK,
           {0, gs::rgb4(1, 3, 2), gs::rgb4(2, 5, 3), gs::rgb4(3, 2, 1), gs::rgb4(6, 6, 7), gs::rgb4(4, 3, 3),
            gs::rgb4(12, 9, 4), gs::rgb4(8, 7, 6)});
    setPal(vdp, PAL_GLASS,
           {0, gs::rgb4(10, 12, 13), gs::rgb4(14, 14, 15), gs::rgb4(13, 13, 14), gs::rgb4(15, 4, 3),
            gs::rgb4(12, 14, 15), gs::rgb4(6, 7, 8)});
    setPal(vdp, PAL_IRON,
           {0, gs::rgb4(15, 7, 2), gs::rgb4(11, 4, 1), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(13, 13, 14)});
    setPal(vdp, PAL_BALL,
           {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 12, 6), gs::rgb4(2, 1, 1), gs::rgb4(6, 2, 1), gs::rgb4(15, 10, 4)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 8, 5), gs::rgb4(9, 6, 4), gs::rgb4(2, 1, 1), gs::rgb4(14, 14, 12),
            gs::rgb4(10, 10, 8), gs::rgb4(2, 4, 12), gs::rgb4(1, 2, 7), gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 3),
            gs::rgb4(14, 14, 15)});
    setPal(vdp, PAL_LANE,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(12, 8, 5), gs::rgb4(9, 6, 4), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2),
            gs::rgb4(8, 1, 1), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 12),
            gs::rgb4(14, 14, 15)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7), gs::rgb4(0, 2, 1));
    setPal(vdp, PAL_JUDGE,
           {0, gs::rgb4(15, 8, 2), gs::rgb4(12, 5, 1), gs::rgb4(4, 4, 5), gs::rgb4(1, 2, 5), gs::rgb4(12, 13, 14),
            gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_METER,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 13, 5), gs::rgb4(15, 14, 8), gs::rgb4(15, 6, 3)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 7), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 11), gs::rgb4(14, 14, 10)});
    setPal(vdp, PAL_DOT, {0, gs::rgb4(15, 14, 8), gs::rgb4(6, 15, 7), gs::rgb4(15, 15, 15)});

    loadFont(vdp, art);
    art.board = gs::uploadImage(vdp, boardArt());
    art.pole = gs::uploadImage(vdp, [] {
        gs::Bitmap b(6, 8);
        b.rect(1, 0, 4, 8, 1);
        b.rect(0, 0, 1, 8, 2);
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
        b.ellipse(5, 5, 4.1f, 4.1f, 1);
        b.ellipse(4, 4, 1.4f, 1.1f, 2);
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
    art.title = phrase(vdp, "S3 HOOP", 3);
    art.swish = phrase(vdp, "SWISH", 2);
    art.count = phrase(vdp, "COUNT IT", 2);
    art.bank = phrase(vdp, "BANK", 2);
    art.rimWord = phrase(vdp, "RIM", 2);
    art.shortWord = phrase(vdp, "SHORT", 2);
    art.longWord = phrase(vdp, "LONG", 2);
    art.airWord = phrase(vdp, "AIR", 2);
    art.youWin = phrase(vdp, "YOU WIN", 2);
    art.laneWin = phrase(vdp, "LANE WINS", 2);
    art.pause = phrase(vdp, "PAUSE", 2);
}

}  // namespace hoop
