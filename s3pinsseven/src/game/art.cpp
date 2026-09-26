#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace pinsseven {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap pinStanding() {
    gs::Bitmap b(32, 44);
    b.ellipse(16, 36, 10, 5, 2);
    b.ellipse(16, 34, 9, 4, 1);
    b.ellipse(16, 27, 12, 8, 1);
    b.ellipse(12, 24, 4, 4, 6);
    b.rect(7, 20, 18, 5, 3);
    b.rect(8, 21, 16, 2, 4);
    b.rect(13, 14, 6, 8, 1);
    b.ellipse(16, 12, 7, 6, 1);
    b.ellipse(13, 10, 2, 2, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap pinFallen() {
    gs::Bitmap b(50, 22);
    b.ellipse(18, 12, 14, 8, 1);
    b.ellipse(14, 10, 5, 3, 6);
    b.rect(28, 7, 6, 10, 3);
    b.rect(29, 9, 4, 4, 4);
    b.ellipse(40, 11, 7, 7, 1);
    b.ellipse(38, 9, 2, 2, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap ballArt(int frame) {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 11, 11, 1);
    b.ellipse(10, 10, 5, 4, 3);
    for (int y = 12; y <= 16; y++)
        for (int x = 4; x <= 24; x++)
            if (b.get(x, y) == 1) b.set(x, y, 6);
    float ang = frame * TAU / 3.0f;
    for (int i = 0; i < 3; i++) {
        float a = ang + i * TAU / 3.0f;
        b.ellipse(14 + std::cos(a) * 4.5f, 14 + std::sin(a) * 3.6f, 2.1f, 2.1f, 4);
    }
    b.outline(5, false);
    return b;
}

gs::Bitmap bowlerArt(int pose) {
    gs::Bitmap b(48, 72);
    b.ellipse(24, 11, 10, 6, 4);
    b.rect(12, 13, 22, 4, 4);
    b.ellipse(24, 20, 8, 8, 3);
    b.rect(21, 26, 6, 4, 3);
    b.poly({{14, 30}, {34, 30}, {38, 50}, {10, 50}}, 1);
    b.poly({{20, 32}, {30, 32}, {29, 46}, {19, 46}}, 2);
    b.rect(14, 48, 20, 3, 7);
    int s = pose == 1 ? 3 : 0;
    b.rect(15 - s, 51, 8, 14, 5);
    b.rect(26 + s, 51, 8, 14, 5);
    b.ellipse(17 - s, 66, 7, 3, 6);
    b.ellipse(32 + s, 66, 7, 3, 6);
    if (pose == 0) {
        b.rect(6, 32, 6, 16, 1);
        b.rect(36, 32, 6, 16, 1);
        b.ellipse(9, 48, 4, 4, 3);
        b.ellipse(39, 48, 4, 4, 3);
    } else if (pose == 1) {
        b.poly({{8, 32}, {14, 32}, {6, 50}, {1, 46}}, 1);
        b.ellipse(4, 50, 4, 4, 3);
        b.poly({{34, 34}, {40, 34}, {46, 20}, {40, 18}}, 1);
        b.ellipse(44, 19, 4, 4, 3);
    } else {
        b.rect(6, 32, 6, 14, 1);
        b.ellipse(9, 46, 4, 4, 3);
        b.poly({{32, 30}, {40, 28}, {46, 8}, {40, 6}}, 1);
        b.ellipse(44, 8, 4, 4, 3);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(14, 20);
    b.poly({{7, 1}, {13, 18}, {1, 18}}, 1);
    b.poly({{7, 7}, {10, 16}, {4, 16}}, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap foulArt() {
    gs::Bitmap b(12, 4);
    b.rect(0, 0, 12, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 4, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 0, 2, 5, 3);
    b.poly({{2, 5}, {12, 5}, {13, 9}, {1, 9}}, 3);
    b.ellipse(7, 13, 6, 5, 1);
    b.ellipse(6, 12, 2, 2, 2);
    return b;
}

gs::Bitmap pitArt() {
    gs::Bitmap b(104, 30);
    b.rect(0, 0, 104, 8, 2);
    b.rect(0, 8, 104, 22, 1);
    b.rect(42, 11, 20, 3, 3);
    b.rect(58, 11, 4, 14, 3);
    b.rect(46, 18, 14, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(18, 46);
    b.rect(7, 8, 5, 32, 1);
    b.rect(4, 4, 10, 6, 2);
    b.ellipse(9, 14, 5, 5, 3);
    b.rect(6, 38, 7, 5, 4);
    b.outline(5, false);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(9, 10, 12), gs::rgb4(15, 6, 5), gs::rgb4(4, 5, 7)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 11), gs::rgb4(10, 7, 1)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 10, 2), gs::rgb4(15, 14, 8), gs::rgb4(8, 4, 1)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 8), gs::rgb4(13, 15, 12), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_PIN, {0, gs::rgb4(15, 14, 12), gs::rgb4(12, 10, 8), gs::rgb4(13, 2, 3), gs::rgb4(8, 1, 2),
                          gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(2, 8, 4), gs::rgb4(1, 4, 2), gs::rgb4(10, 15, 11), gs::rgb4(1, 1, 1),
                           gs::rgb4(1, 2, 1), gs::rgb4(14, 13, 8)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(8, 8, 9), gs::rgb4(0, 0, 0),
                            gs::rgb4(1, 1, 1), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(13, 12, 9), gs::rgb4(8, 8, 6), gs::rgb4(15, 12, 9), gs::rgb4(3, 3, 5),
                          gs::rgb4(2, 3, 7), gs::rgb4(14, 13, 12), gs::rgb4(9, 7, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(11, 2, 3), gs::rgb4(6, 1, 2), gs::rgb4(15, 12, 9), gs::rgb4(2, 1, 1),
                           gs::rgb4(1, 1, 2), gs::rgb4(8, 8, 8), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 13), gs::rgb4(4, 4, 6)});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 3), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_PIT, {0, gs::rgb4(3, 2, 4), gs::rgb4(8, 6, 4), gs::rgb4(15, 12, 4), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(4, 4, 6), gs::rgb4(8, 7, 5), gs::rgb4(2, 7, 4), gs::rgb4(3, 2, 2),
                           gs::rgb4(1, 1, 2)});

    const uint16_t lane[16] = {
        0,
        gs::rgb4(2, 6, 7), gs::rgb4(1, 4, 5), gs::rgb4(3, 8, 7),
        gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 4),
        gs::rgb4(13, 9, 4), gs::rgb4(10, 7, 3),
        gs::rgb4(8, 6, 3), gs::rgb4(12, 8, 4), gs::rgb4(14, 12, 8),
        gs::rgb4(2, 3, 4), gs::rgb4(2, 4, 5), gs::rgb4(3, 5, 6),
        gs::rgb4(15, 14, 11), gs::rgb4(12, 8, 4),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_LANE * 16 + i, lane[i]);
    vdp.setFogColor(gs::rgb4(1, 2, 4));

    loadFont(vdp, art);
    art.pin = gs::uploadMipped(vdp, pinStanding());
    art.pinFlat = gs::uploadMipped(vdp, pinFallen());
    for (int i = 0; i < 3; i++) {
        art.ball[i] = gs::uploadMipped(vdp, ballArt(i));
        art.bowler[i] = gs::uploadMipped(vdp, bowlerArt(i));
    }
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.foul = gs::uploadMipped(vdp, foulArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pit = gs::uploadMipped(vdp, pitArt());
    art.post = gs::uploadMipped(vdp, postArt());
}

}  // namespace pinsseven
