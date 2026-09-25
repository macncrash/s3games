#include "game/art.h"

#include <initializer_list>

namespace heli {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

void loadFont(gs::VDP& vdp, Art& art) {
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
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

void paintHeli(Bitmap& b) {
    b.poly({{6, 28}, {40, 24}, {42, 31}, {8, 33}}, 2);
    b.poly({{4, 12}, {18, 12}, {16, 30}, {6, 32}}, 1);
    b.poly({{6, 14}, {14, 14}, {13, 22}, {7, 22}}, 3);
    b.ellipse(8, 20, 2.2f, 2.2f, 10);
    b.poly({{34, 22}, {78, 18}, {96, 24}, {94, 34}, {86, 38}, {38, 38}}, 1);
    b.poly({{40, 30}, {88, 28}, {86, 36}, {42, 36}}, 9);
    b.poly({{36, 26}, {70, 23}, {70, 28}, {38, 30}}, 3);
    b.poly({{74, 20}, {94, 24}, {90, 32}, {72, 30}}, 5);
    b.poly({{78, 22}, {90, 24}, {88, 29}, {77, 28}}, 6);
    b.ellipse(58, 20, 14, 8, 2);
    b.rect(46, 14, 18, 8, 2);
    b.rect(54, 5, 4, 14, 10);
    b.ellipse(56, 5, 2.2f, 2.2f, 8);
    b.rect(48, 24, 12, 10, 12);
    b.rect(50, 26, 4, 6, 5);
    b.rect(64, 26, 8, 2, 8);
    b.rect(67, 23, 2, 8, 8);
    b.line(36, 47, 88, 47, 7, 2.f);
    b.line(46, 38, 44, 47, 7, 1.6f);
    b.line(78, 38, 80, 47, 7, 1.6f);
    b.ellipse(96, 26, 2, 2, 8);
    b.ellipse(10, 14, 1.6f, 1.6f, 14);
    b.line(94, 24, 102, 23, 10, 1.2f);
    b.outline(15, false);
}

void paintRotor(Bitmap& b, float y0, float y1, bool edge) {
    b.ellipse(50, 8, 5, 3.2f, 10);
    if (edge) {
        b.rect(42, 4, 16, 8, 2);
        b.line(18, 8, 82, 8, 1, 1.2f);
    } else {
        b.line(4, y0, 96, y1, 2, 2.4f);
        b.line(12, (y0 + 8.f) * 0.5f, 88, (y1 + 8.f) * 0.5f, 1, 1.3f);
    }
    b.ellipse(50, 8, 2.2f, 2.2f, 11);
}

void paintTail(Bitmap& b, bool spin) {
    b.ellipse(7, 7, 2.2f, 2.2f, 10);
    if (spin) {
        b.line(2, 2, 12, 12, 2, 1.6f);
        b.line(2, 12, 12, 2, 1, 1.4f);
    } else {
        b.line(1, 7, 13, 7, 2, 1.8f);
        b.line(7, 1, 7, 13, 1, 1.6f);
    }
}

void paintDeck(Bitmap& b) {
    b.rect(0, 4, b.w, 12, 1);
    b.rect(0, 4, b.w, 3, 2);
    b.rect(0, 14, b.w, 3, 4);
    int cx = b.w / 2;
    b.rect(cx - 10, 7, 3, 8, 3);
    b.rect(cx + 7, 7, 3, 8, 3);
    b.rect(cx - 10, 10, 20, 3, 3);
    b.ellipse(8, 10, 2.3f, 2.3f, 6);
    b.ellipse(b.w - 8, 10, 2.3f, 2.3f, 6);
    b.line(18, 13, 28, 7, 5, 1.3f);
    b.line(b.w - 18, 13, b.w - 28, 7, 5, 1.3f);
    b.outline(15, false);
}

void paintPier(Bitmap& b) {
    b.rect(0, 0, b.w, b.h, 3);
    for (int x = 2; x < b.w - 2; x += 8) {
        b.rect(x, 0, 5, b.h, (x / 8) % 2 ? 1 : 2);
        b.rect(x, b.h - 7, 5, 7, 7);
    }
    for (int y = 8; y < b.h - 4; y += 11) b.line(0, float(y), float(b.w), float(y), 4, 1.2f);
    b.rect(0, 0, b.w, 5, 6);
    gs::TextStyle st{2, 8, 4, 0, 1};
    Bitmap n = gs::textBitmap("1", st);
    b.blit(n, 10, b.h / 2 - n.h / 2);
    b.outline(15, false);
}

void paintRoof(Bitmap& b) {
    b.rect(0, 0, b.w, b.h, 1);
    for (int y = 10; y < b.h; y += 4) {
        int off = ((y / 4) & 1) ? 6 : 0;
        for (int x = -10; x < b.w; x += 12) b.rect(x + off, y, 11, 3, ((x + y) / 12) % 4 == 0 ? 2 : 1);
    }
    for (int row = 16; row < b.h - 14; row += 14) {
        for (int col = 8; col < b.w - 10; col += 16) {
            bool lit = ((row / 14 + col / 16) % 3) != 0;
            b.rect(col, row, 9, 8, lit ? 4 : 5);
            b.rect(col + 2, row + 2, 4, 3, lit ? 6 : 5);
        }
    }
    b.rect(0, 0, b.w, 6, 8);
    b.rect(0, 6, b.w, 3, 3);
    b.rect(0, b.h - 8, b.w, 8, 2);
    gs::TextStyle st{2, 6, 5, 0, 1};
    Bitmap n = gs::textBitmap("2", st);
    b.blit(n, b.w / 2 - n.w / 2, 14);
    b.outline(15, false);
}

void paintField(Bitmap& b) {
    b.rect(0, 0, b.w, b.h, 1);
    b.rect(0, 0, 6, b.h, 2);
    b.rect(b.w - 6, 0, 6, b.h, 2);
    for (int y = 8; y < b.h - 20; y += 12) b.rect(10, y, b.w - 20, 2, 2);
    int cx = b.w / 2;
    int cy = b.h / 2 - 2;
    b.rect(cx - 5, cy - 16, 10, 32, 4);
    b.rect(cx - 16, cy - 5, 32, 10, 4);
    b.rect(cx - 9, b.h - 22, 18, 18, 6);
    b.rect(cx - 5, b.h - 18, 10, 12, 3);
    b.rect(0, b.h - 6, b.w, 6, 7);
    b.rect(0, b.h - 4, b.w, 2, 8);
    gs::TextStyle st{2, 3, 2, 0, 1};
    Bitmap n = gs::textBitmap("3", st);
    b.blit(n, 12, 8);
    b.outline(15, false);
}

void paintSock(Bitmap& b, int lean) {
    b.rect(4, 8, 3, 16, 3);
    float tip = lean < 0 ? 4.f : lean > 0 ? 20.f : 12.f;
    b.poly({{7, 8}, {tip, 4}, {tip, 10}, {7, 12}}, 1);
    b.line(7, 9, tip, lean == 0 ? 6.f : 5.f, 2, 1.3f);
    b.ellipse(5, 22, 3, 2, 4);
}

void paintChevron(Bitmap& b) {
    b.poly({{8, 13}, {1, 3}, {6, 3}, {8, 7}, {10, 3}, {15, 3}}, 1);
}

void paintCloud(Bitmap& b) {
    b.ellipse(16, 14, 12, 7, 2);
    b.ellipse(28, 12, 14, 8, 1);
    b.ellipse(38, 15, 10, 6, 3);
    b.ellipse(22, 16, 8, 4, 3);
}

void paintBird(Bitmap& b, bool up) {
    float tip = up ? 3.f : 12.f;
    b.line(14, 8, 2, tip, 1, 1.6f);
    b.line(14, 8, 26, tip, 1, 1.6f);
    b.ellipse(14, 8, 2.4f, 1.8f, 2);
    b.set(18, 7, 3);
}

void paintSun(Bitmap& b) {
    b.ellipse(16, 16, 14, 14, 3);
    b.ellipse(16, 16, 10, 10, 2);
    b.ellipse(16, 16, 5, 5, 1);
}

void paintDust(Bitmap& b) {
    b.ellipse(16, 8, 14, 5, 1);
    b.ellipse(10, 8, 5, 3, 3);
    b.ellipse(22, 7, 4, 2.5f, 3);
}

void paintShadow(Bitmap& b) {
    b.ellipse(16, 6, 14, 4, 2);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(4, 5, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 12, 5), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HELI,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(10, 10, 11), gs::rgb4(15, 8, 2), gs::rgb4(11, 5, 1), gs::rgb4(8, 13, 14),
            gs::rgb4(3, 8, 10), gs::rgb4(5, 5, 6), gs::rgb4(14, 2, 2), gs::rgb4(7, 8, 9), gs::rgb4(3, 3, 4),
            gs::rgb4(15, 13, 5), gs::rgb4(4, 5, 6), gs::rgb4(3, 3, 3), gs::rgb4(6, 15, 8), shadow});
    setPal(vdp, PAL_DECK,
           {0, gs::rgb4(9, 9, 10), gs::rgb4(13, 13, 14), gs::rgb4(15, 12, 3), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 15),
            gs::rgb4(15, 14, 6), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DECK_OK,
           {0, gs::rgb4(9, 10, 9), gs::rgb4(13, 14, 13), gs::rgb4(6, 15, 6), gs::rgb4(3, 5, 3), gs::rgb4(15, 15, 15),
            gs::rgb4(8, 15, 8), gs::rgb4(5, 7, 5), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(10, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4), gs::rgb4(12, 10, 6),
            gs::rgb4(5, 5, 6), gs::rgb4(3, 4, 5), gs::rgb4(15, 13, 8), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOF,
           {0, gs::rgb4(11, 5, 4), gs::rgb4(7, 3, 3), gs::rgb4(12, 9, 8), gs::rgb4(15, 13, 7), gs::rgb4(2, 3, 5),
            gs::rgb4(15, 15, 12), gs::rgb4(8, 8, 9), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIELD,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(10, 11, 11), gs::rgb4(6, 10, 12), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1),
            gs::rgb4(4, 5, 6), gs::rgb4(6, 6, 5), gs::rgb4(4, 8, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(13, 13, 15), gs::rgb4(9, 9, 12), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(6, 6, 8), gs::rgb4(15, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 10, 7), gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 4), gs::rgb4(13, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SOCK,
           {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 15), gs::rgb4(7, 7, 8), gs::rgb4(10, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(7, 4, 2), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);

    Bitmap heli(kHeliBmpW, kHeliBmpH);
    paintHeli(heli);
    art.heli = gs::uploadMipped(vdp, heli);

    const float bladeY0[4] = {8, 3, 8, 13};
    const float bladeY1[4] = {8, 13, 8, 3};
    for (int i = 0; i < 4; i++) {
        Bitmap blade(100, 16);
        paintRotor(blade, bladeY0[i], bladeY1[i], i == 2);
        art.rotor[i] = gs::uploadMipped(vdp, blade);
    }
    for (int i = 0; i < 2; i++) {
        Bitmap tail(14, 14);
        paintTail(tail, i == 1);
        art.tail[i] = gs::uploadMipped(vdp, tail);
    }

    Bitmap deck(96, 20);
    paintDeck(deck);
    art.deck = gs::uploadMipped(vdp, deck);

    Bitmap pier(kPads[0].bw, kPads[0].bh);
    paintPier(pier);
    art.bldg[0] = gs::uploadMipped(vdp, pier);
    Bitmap roof(kPads[1].bw, kPads[1].bh);
    paintRoof(roof);
    art.bldg[1] = gs::uploadMipped(vdp, roof);
    Bitmap field(kPads[2].bw, kPads[2].bh);
    paintField(field);
    art.bldg[2] = gs::uploadMipped(vdp, field);

    Bitmap lamp(12, 12);
    lamp.ellipse(6, 6, 4.2f, 4.2f, 1);
    art.lamp = gs::uploadMipped(vdp, lamp);

    for (int i = 0; i < 3; i++) {
        Bitmap sock(24, 26);
        paintSock(sock, i - 1);
        art.sock[i] = gs::uploadMipped(vdp, sock);
    }

    Bitmap chev(16, 14);
    paintChevron(chev);
    art.chevron = gs::uploadMipped(vdp, chev);

    Bitmap dust(32, 16);
    paintDust(dust);
    art.dust = gs::uploadMipped(vdp, dust);
    Bitmap shade(32, 12);
    paintShadow(shade);
    art.shadow = gs::uploadMipped(vdp, shade);

    Bitmap cloud(52, 26);
    paintCloud(cloud);
    art.cloud = gs::uploadMipped(vdp, cloud);
    for (int i = 0; i < 2; i++) {
        Bitmap bird(28, 16);
        paintBird(bird, i == 0);
        art.bird[i] = gs::uploadMipped(vdp, bird);
    }
    Bitmap sun(32, 32);
    paintSun(sun);
    art.sun = gs::uploadMipped(vdp, sun);

    for (int d = 0; d < 10; d++) {
        char s[2] = {char('0' + d), 0};
        art.digit[d] = words(vdp, s, 3, 1, 2);
    }
    art.colon = words(vdp, ":", 3, 1, 2);
    art.times = words(vdp, "x3", 2, 1, 2);
    art.title = words(vdp, "S3 HELI", 3, 1, 2);
    art.three = words(vdp, "THREE PADS", 2, 1, 2);
    art.down = words(vdp, "PADS DOWN", 2, 1, 2);
    art.clocked = words(vdp, "CLOCKED OUT", 2, 1, 2);
    art.airframe = words(vdp, "AIRFRAME", 2, 1, 2);
}

}  // namespace heli
