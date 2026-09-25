#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace striker {
namespace {

constexpr float PI = 3.14159265f;
constexpr int MAN_W = 250;
constexpr int MAN_H = 236;
constexpr float ARM = 26.f;
constexpr float HANDLE = 78.f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void mallet(Bitmap& b, float x0, float y0, float deg) {
    float r = deg * PI / 180.f;
    float c = std::cos(r), s = std::sin(r);
    float len = ARM + HANDLE;
    float hx = x0 + c * len;
    float hy = y0 - s * len;
    b.line(x0, y0, hx, hy, 6, 5.0f);
    auto corner = [&](float along, float side) {
        return Pt{hx + c * along + s * side, hy - s * along + c * side};
    };
    b.poly({corner(-8, -18), corner(9, -18), corner(9, 18), corner(-8, 18)}, 8);
    b.poly({corner(-8, -18), corner(3, -18), corner(3, 18), corner(-8, 18)}, 9);
    b.poly({corner(-3, -18), corner(9, -18), corner(9, -12), corner(-3, -12)}, 9);
    b.ellipse(x0 + c * ARM, y0 - s * ARM, 6, 6, 1);
}

Bitmap manArt(float deg) {
    Bitmap b(MAN_W, MAN_H);
    float r = deg * PI / 180.f;
    bool behind = deg > 150.f && deg < 250.f;
    const float sx = 128, sy = 130;
    if (behind) mallet(b, sx, sy, deg);

    float lean = std::cos(r) * 7.f;
    float hx = 112 + lean;
    b.ellipse(104 + lean * 0.3f, 224, 34, 7, 10);
    b.rect(86 + lean * 0.2f, 196, 16, 28, 6);
    b.rect(118 + lean * 0.2f, 196, 16, 28, 6);
    b.ellipse(94 + lean * 0.2f, 222, 12, 6, 12);
    b.ellipse(126 + lean * 0.2f, 222, 12, 6, 12);
    b.rect(88 + lean * 0.2f, 214, 14, 4, 4);
    b.rect(120 + lean * 0.2f, 214, 14, 4, 4);
    b.rect(84 + lean * 0.35f, 168, 52, 32, 3);
    b.rect(84 + lean * 0.35f, 176, 52, 6, 4);
    b.rect(84 + lean * 0.35f, 190, 52, 5, 4);
    b.poly({Pt{80 + lean * 0.5f, 168}, Pt{140 + lean * 0.5f, 168}, Pt{134 + lean, 118}, Pt{86 + lean, 118}}, 3);
    b.rect(88 + lean * 0.7f, 132, 44, 6, 4);
    b.rect(88 + lean * 0.7f, 146, 44, 5, 4);
    b.rect(86 + lean * 0.4f, 164, 48, 6, 7);
    b.rect(104 + lean * 0.4f, 160, 10, 8, 9);
    b.line(110 + lean, 124, 110 + lean, 140, 4, 2.0f);
    b.line(102 + lean, 132, 118 + lean, 132, 4, 2.0f);
    b.ellipse(hx, 96, 18, 20, 1);
    b.ellipse(hx - 4, 92, 10, 12, 2);
    b.ellipse(hx, 82, 18, 10, 5);
    b.rect(hx - 16, 78, 32, 5, 3);
    b.ellipse(hx + 8, 104, 8, 4, 5);
    b.rect(hx + 6, 92, 4, 4, 13);
    b.rect(hx + 7, 93, 2, 2, 14);
    b.line(hx + 4, 88, hx + 14, 90, 5, 2.0f);
    b.line(96 + lean, 126, 78 + lean, 160, 1, 7.0f);
    b.ellipse(76 + lean, 162, 5, 5, 1);
    if (!behind) {
        b.line(sx - 8, sy - 4, sx + std::cos(r) * ARM * 0.85f, sy - std::sin(r) * ARM * 0.85f, 1, 8.0f);
        mallet(b, sx, sy, deg);
    } else {
        b.line(sx - 6, sy, sx + std::cos(r) * 18.f, sy - std::sin(r) * 18.f, 2, 7.0f);
    }
    b.outline(10, false);
    return b;
}

Bitmap towerArt(Art& a) {
    Bitmap b(56, 184);
    b.poly({Pt{6, 18}, Pt{18, 18}, Pt{22, 158}, Pt{2, 158}}, 2);
    b.poly({Pt{8, 18}, Pt{16, 18}, Pt{18, 158}, Pt{6, 158}}, 1);
    b.poly({Pt{38, 18}, Pt{50, 18}, Pt{54, 158}, Pt{34, 158}}, 2);
    b.poly({Pt{40, 18}, Pt{48, 18}, Pt{50, 158}, Pt{38, 158}}, 3);
    b.rect(22, 20, 12, 138, 6);
    b.rect(24, 20, 3, 138, 7);
    for (int i = 0; i < 7; i++) {
        int y = 28 + i * 18;
        b.rect(4, y, 48, 4, 4);
        b.rect(4, y, 48, 1, 5);
    }
    b.rect(0, 16, 8, 3, 10);
    b.rect(0, 50, 8, 2, 8);
    b.rect(0, 84, 10, 2, 8);
    b.rect(0, 118, 8, 2, 8);
    b.rect(18, 18, 20, 3, 10);
    b.rect(10, 6, 36, 8, 4);
    b.rect(14, 2, 28, 6, 5);
    b.rect(4, 156, 48, 8, 5);
    b.rect(0, 162, 56, 14, 4);
    b.rect(8, 160, 40, 6, 5);
    b.rect(22, 154, 12, 8, 6);
    b.ellipse(28, 168, 8, 3, 5);
    b.blit(gs::textBitmap("BELL", {1, 10, 0, 0, 0}), 16, 8);
    b.outline(9, false);
    a.towerSlotX = 28;
    a.towerSlotTop = 22;
    a.towerSlotBot = 154;
    a.towerAnvilY = 168;
    return b;
}

Bitmap bellArt(int swing) {
    Bitmap b(40, 36);
    b.rect(17, 0, 6, 7, 3);
    b.ellipse(20, 6, 5, 3, 2);
    b.poly({Pt{8, 10}, Pt{32, 10}, Pt{37, 26}, Pt{3, 26}}, 2);
    b.poly({Pt{12, 12}, Pt{28, 12}, Pt{31, 22}, Pt{9, 22}}, 1);
    b.ellipse(15, 14, 4, 3, 8);
    b.rect(3, 24, 34, 5, 3);
    b.rect(1, 28, 38, 4, 2);
    b.rect(6, 28, 28, 2, 1);
    int cx = 20 + swing * 5;
    b.line(20, 12, float(cx), 22, 5, 2.0f);
    b.ellipse(float(cx), 23, 3, 3, 5);
    b.outline(9, false);
    return b;
}

Bitmap puckArt() {
    Bitmap b(22, 16);
    b.ellipse(11, 8, 10, 6, 6);
    b.ellipse(11, 6, 9, 4, 7);
    b.rect(3, 6, 16, 4, 7);
    b.ellipse(11, 11, 9, 3, 6);
    b.ellipse(7, 5, 3, 2, 8);
    b.rect(4, 7, 14, 2, 10);
    b.outline(9, false);
    return b;
}

Bitmap flashArt() {
    Bitmap b(36, 36);
    for (int i = 0; i < 10; i++) {
        float a = i * (PI * 2.f / 10.f);
        float x = 18 + std::cos(a) * 15;
        float y = 18 + std::sin(a) * 15;
        b.line(18, 18, x, y, i & 1 ? 3 : 2, 2.0f);
    }
    b.ellipse(18, 18, 6, 6, 1);
    b.ellipse(18, 18, 3, 3, 2);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(9, 9);
    b.line(0, 4, 8, 4, 1, 1.5f);
    b.line(4, 0, 4, 8, 2, 1.5f);
    b.set(4, 4, 3);
    return b;
}

Bitmap groundArt() {
    Bitmap b(320, 48);
    b.rect(0, 0, 320, 10, 11);
    for (int i = 0; i < 10; i++) b.ellipse(16.f + i * 32, 6, 14, 5, 13);
    for (int x = 0; x < 320; x += 16) {
        int plank = (x / 16) & 1 ? 2 : 1;
        b.rect(float(x), 10, 15, 38, plank);
        b.rect(float(x), 10, 15, 2, 3);
        b.set(x + 4, 18, 12);
        b.set(x + 11, 36, 12);
    }
    return b;
}

Bitmap tentArt() {
    Bitmap b(168, 96);
    b.poly({Pt{8, 92}, Pt{84, 10}, Pt{160, 92}}, 2);
    b.poly({Pt{20, 92}, Pt{84, 22}, Pt{148, 92}}, 3);
    for (int i = 0; i < 5; i++) {
        float x0 = 24.f + i * 26.f;
        b.poly({Pt{x0, 90}, Pt{x0 + 10, 90}, Pt{84, 18}}, 4);
    }
    b.rect(80, 40, 8, 54, 5);
    b.rect(78, 8, 4, 16, 5);
    b.poly({Pt{82, 8}, Pt{100, 10}, Pt{82, 18}}, 7);
    for (int i = 0; i < 7; i++) b.ellipse(18.f + i * 22.f, 90, 12, 8, i & 1 ? 4 : 2);
    b.outline(6, false);
    return b;
}

Bitmap lightsArt(int phase) {
    Bitmap b(304, 22);
    b.line(2, 3, 300, 5, 6, 1.5f);
    for (int i = 0; i < 16; i++) {
        float x = 12.f + i * 18.f;
        float y = 10.f + std::sin(i * 0.7f) * 2.f;
        int c = (i + phase) % 3 == 0 ? 7 : (i + phase) % 3 == 1 ? 8 : 9;
        if (((i + phase) & 1) == 0) c = 1;
        b.line(x, 3, x, y - 3, 6, 1.0f);
        b.ellipse(x, y, 4, 5, c);
        b.ellipse(x - 1, y - 1, 1.5f, 1.5f, 1);
    }
    return b;
}

Bitmap buntingArt() {
    Bitmap b(304, 28);
    b.line(2, 2, 300, 2, 6, 1.5f);
    const int cols[] = {2, 4, 8, 9, 7, 4};
    for (int i = 0; i < 14; i++) {
        float x = 12.f + i * 21.f;
        b.poly({Pt{x - 8, 3}, Pt{x + 8, 3}, Pt{x, 22}}, cols[i % 6]);
        b.poly({Pt{x - 4, 5}, Pt{x + 4, 5}, Pt{x, 16}}, 1);
    }
    return b;
}

Bitmap personArt(int coat) {
    Bitmap b(30, 54);
    b.ellipse(15, 50, 9, 3, 6);
    b.rect(11, 40, 3, 9, 5);
    b.rect(16, 40, 3, 9, 5);
    b.rect(8, 26, 14, 16, coat);
    b.rect(5, 28, 4, 12, coat);
    b.rect(21, 28, 4, 12, coat);
    b.ellipse(15, 16, 7, 8, 11);
    b.ellipse(15, 12, 8, 5, 12);
    b.rect(18, 16, 2, 2, 1);
    b.ellipse(15, 22, 4, 2, 11);
    b.outline(6, false);
    return b;
}

Bitmap moonArt() {
    Bitmap b(30, 30);
    b.ellipse(15, 15, 12, 12, 7);
    b.ellipse(18, 13, 9, 9, 0);
    b.ellipse(11, 12, 3, 2, 1);
    return b;
}

Bitmap starArt() {
    Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1.0f);
    b.line(0, 3, 6, 3, 1, 1.0f);
    b.set(3, 3, 2);
    return b;
}

Bitmap bulbArt() {
    Bitmap b(8, 10);
    b.rect(3, 0, 2, 2, 5);
    b.ellipse(4, 6, 3, 3, 7);
    b.set(3, 5, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 14, 12);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 8, 10), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(10, 1, 1), gs::rgb4(15, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 10, 3), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(14, 10, 7), gs::rgb4(10, 6, 4), gs::rgb4(13, 2, 2), gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1),
            gs::rgb4(8, 5, 2), gs::rgb4(14, 11, 3), gs::rgb4(8, 9, 11), gs::rgb4(13, 14, 15), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 7, 6), gs::rgb4(4, 2, 1), gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), 0});

    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 6),
            gs::rgb4(2, 1, 1), gs::rgb4(6, 7, 8), gs::rgb4(15, 14, 12), gs::rgb4(1, 1, 1), gs::rgb4(14, 3, 2),
            gs::rgb4(4, 3, 2), gs::rgb4(3, 3, 3), gs::rgb4(5, 8, 3), gs::rgb4(2, 2, 2), 0});

    setPal(vdp, PAL_BRASS,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(13, 10, 3), gs::rgb4(8, 6, 2), gs::rgb4(6, 3, 2), gs::rgb4(3, 2, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 13), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 6),
            gs::rgb4(4, 4, 5), 0, 0, 0, 0});

    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(15, 14, 12), gs::rgb4(13, 2, 3), gs::rgb4(8, 1, 2), gs::rgb4(14, 12, 10), gs::rgb4(8, 5, 2),
            gs::rgb4(2, 1, 4), gs::rgb4(15, 12, 3), gs::rgb4(14, 3, 3), gs::rgb4(4, 6, 13), gs::rgb4(2, 3, 8),
            gs::rgb4(13, 9, 7), gs::rgb4(2, 1, 1), gs::rgb4(8, 1, 3), gs::rgb4(3, 8, 4), gs::rgb4(1, 1, 2)});

    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 3), gs::rgb4(15, 7, 2), gs::rgb4(14, 3, 2), gs::rgb4(5, 8, 14),
            gs::rgb4(2, 2, 4), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0});

    vdp.setFogColor(gs::rgb4(2, 1, 4));

    static const float kAng[7] = {205, 175, 140, 110, 88, 68, -30};
    for (int i = 0; i < 7; i++) art.man[i] = gs::uploadMipped(vdp, manArt(kAng[i]));
    art.footX = 114;
    art.footY = 228;
    art.shoulderX = 128;
    art.shoulderY = 130;

    art.tower = gs::uploadMipped(vdp, towerArt(art));
    for (int i = 0; i < 3; i++) art.bell[i] = gs::uploadMipped(vdp, bellArt(i - 1));
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.ground = gs::uploadMipped(vdp, groundArt());
    art.tent = gs::uploadMipped(vdp, tentArt());
    art.lights[0] = gs::uploadMipped(vdp, lightsArt(0));
    art.lights[1] = gs::uploadMipped(vdp, lightsArt(1));
    art.bunting = gs::uploadMipped(vdp, buntingArt());
    art.person[0] = gs::uploadMipped(vdp, personArt(10));
    art.person[1] = gs::uploadMipped(vdp, personArt(13));
    art.person[2] = gs::uploadMipped(vdp, personArt(14));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    loadFont(vdp, art);
}

}  // namespace striker
