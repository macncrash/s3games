#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace pinsmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
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

gs::Image phrase(gs::VDP& vdp, const char* s) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {2, 1, 2, 0, 1}));
}

void paintPin(gs::Bitmap& b) {
    b.ellipse(9, 30, 6.2f, 7.2f, 1);
    b.ellipse(7, 28, 2.4f, 3.2f, 4);
    b.rect(6, 16, 6, 10, 1);
    b.rect(4, 18, 10, 4, 2);
    b.rect(5, 22, 8, 2, 3);
    b.ellipse(9, 9, 4.2f, 5.0f, 1);
    b.ellipse(7, 7, 1.6f, 1.8f, 4);
    b.rect(8, 33, 2, 2, 3);
    b.outline(5, false);
}

void paintFlat(gs::Bitmap& b) {
    b.ellipse(14, 7, 11, 4.2f, 1);
    b.ellipse(10, 6, 3, 2, 4);
    b.rect(16, 4, 5, 5, 2);
    b.rect(17, 6, 4, 2, 3);
    b.ellipse(24, 7, 3.2f, 3.0f, 1);
    b.outline(5, false);
}

void paintBall(gs::Bitmap& b, int frame) {
    b.ellipse(11, 11, 9.2f, 9.2f, 1);
    b.ellipse(8, 7, 3.2f, 2.4f, 4);
    float ox = frame == 0 ? 0 : frame == 1 ? 1.4f : -1.2f;
    float oy = frame == 2 ? -1.2f : 0.4f;
    b.ellipse(8 + ox, 9 + oy, 1.5f, 1.5f, 3);
    b.ellipse(13 + ox, 9 + oy, 1.5f, 1.5f, 3);
    b.ellipse(10.5f + ox, 13 + oy, 1.5f, 1.5f, 3);
    b.rect(14, 4, 3, 3, 2);
    b.outline(5, false);
}

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(20, 8, 7.2f, 6.4f, 4);
    b.ellipse(13, 9, 1.6f, 2.2f, 3);
    b.ellipse(27, 9, 1.6f, 2.2f, 3);
    b.rect(17, 13, 6, 3, 3);
    b.rect(10, 16, 20, 18, 1);
    b.rect(10, 16, 20, 4, 2);
    b.rect(18, 16, 4, 16, 2);
    b.rect(11, 33, 18, 3, 6);
    b.rect(12, 36, 7, 16, 5);
    b.rect(21, 36, 7, 16, 5);
    b.rect(11, 50, 9, 4, 6);
    b.rect(21, 50, 9, 4, 6);
    b.rect(11, 53, 9, 2, 7);
    b.rect(21, 53, 9, 2, 7);
    if (pose == 0) {
        b.rect(4, 18, 6, 16, 1);
        b.rect(30, 18, 6, 14, 1);
        b.rect(30, 30, 6, 4, 3);
    } else if (pose == 1) {
        b.rect(4, 18, 6, 14, 1);
        b.rect(30, 4, 6, 16, 1);
        b.rect(30, 2, 6, 4, 3);
        b.rect(14, 36, 6, 16, 5);
        b.rect(22, 38, 6, 14, 5);
    } else {
        b.rect(2, 20, 6, 12, 1);
        b.rect(28, 2, 6, 18, 1);
        b.rect(28, 1, 6, 4, 3);
        b.rect(13, 36, 6, 14, 5);
        b.rect(23, 40, 6, 12, 5);
        b.rect(12, 48, 10, 4, 6);
        b.rect(24, 50, 8, 3, 6);
    }
    b.outline(8, false);
}

void paintArrow(gs::Bitmap& b) {
    b.poly({{6, 1}, {11, 14}, {1, 14}}, 1);
    b.poly({{6, 5}, {9, 12}, {3, 12}}, 2);
}

void paintMachine(gs::Bitmap& b) {
    b.rect(2, 2, 16, 4, 4);
    b.rect(3, 6, 14, 22, 1);
    b.rect(5, 8, 10, 8, 2);
    b.ellipse(10, 22, 3.4f, 3.4f, 3);
    b.rect(14, 8, 2, 2, 6);
    b.rect(2, 28, 16, 4, 4);
    b.outline(5, false);
}

void paintLamp(gs::Bitmap& b) {
    b.rect(6, 0, 2, 5, 3);
    b.poly({{3, 6}, {11, 6}, {13, 11}, {1, 11}}, 2);
    b.ellipse(7, 12, 4.2f, 3.0f, 1);
    b.outline(3, false);
}

void paintBack(gs::Bitmap& b) {
    b.rect(0, 2, 8, 16, 1);
    b.rect(0, 0, 8, 3, 2);
    b.rect(0, 8, 8, 2, 4);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 3)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 14, 11), gs::rgb4(6, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 2)});
    setPal(vdp, PAL_PIN, {0, gs::rgb4(15, 14, 12), gs::rgb4(13, 2, 2), gs::rgb4(2, 1, 1), gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(1, 1, 1), gs::rgb4(13, 2, 3), gs::rgb4(1, 1, 4), gs::rgb4(8, 8, 9), gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_BOWLER,
           {0, gs::rgb4(2, 3, 8), gs::rgb4(15, 15, 14), gs::rgb4(12, 8, 5), gs::rgb4(3, 2, 1), gs::rgb4(13, 11, 8),
            gs::rgb4(6, 3, 2), gs::rgb4(14, 12, 9), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 13, 5), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(11, 15, 8), gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 6, 3), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(2, 3, 6), gs::rgb4(6, 12, 13), gs::rgb4(1, 1, 1), gs::rgb4(10, 11, 12), gs::rgb4(1, 1, 2),
                           gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_ARROW, {0, gs::rgb4(15, 14, 12), gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 6), gs::rgb4(6, 2, 2), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_LANE,
           {0, gs::rgb4(7, 2, 2), gs::rgb4(4, 1, 1), gs::rgb4(9, 3, 2), gs::rgb4(1, 5, 5), gs::rgb4(0, 3, 3),
            gs::rgb4(14, 10, 5), gs::rgb4(11, 7, 3), gs::rgb4(8, 5, 2), gs::rgb4(9, 6, 3), gs::rgb4(15, 12, 7),
            gs::rgb4(2, 2, 3), gs::rgb4(2, 2, 3), gs::rgb4(2, 2, 3), gs::rgb4(13, 2, 2), gs::rgb4(15, 13, 8)});

    loadFont(vdp, art);
    gs::Bitmap pin(18, 40);
    paintPin(pin);
    art.pin = gs::uploadMipped(vdp, pin);
    gs::Bitmap flat(30, 14);
    paintFlat(flat);
    art.pinFlat = gs::uploadMipped(vdp, flat);
    for (int i = 0; i < 3; i++) {
        gs::Bitmap ball(22, 22);
        paintBall(ball, i);
        art.ball[i] = gs::uploadMipped(vdp, ball);
    }
    for (int i = 0; i < 3; i++) {
        gs::Bitmap body(40, 58);
        paintBowler(body, i);
        art.bowler[i] = gs::uploadMipped(vdp, body);
    }
    gs::Bitmap arrow(12, 16);
    paintArrow(arrow);
    art.arrow = gs::uploadMipped(vdp, arrow);
    gs::Bitmap dot(8, 8);
    dot.ellipse(4, 4, 3, 3, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    gs::Bitmap spot(8, 6);
    spot.ellipse(4, 3, 3, 2, 1);
    art.spot = gs::uploadMipped(vdp, spot);
    gs::Bitmap foul(8, 4);
    foul.rect(0, 1, 8, 2, 1);
    art.foul = gs::uploadMipped(vdp, foul);
    gs::Bitmap shadow(24, 8);
    shadow.ellipse(12, 4, 10, 3, 1);
    art.shadow = gs::uploadMipped(vdp, shadow);
    gs::Bitmap machine(20, 34);
    paintMachine(machine);
    art.machine = gs::uploadMipped(vdp, machine);
    gs::Bitmap back(8, 20);
    paintBack(back);
    art.backstop = gs::uploadMipped(vdp, back);
    gs::Bitmap lamp(14, 16);
    paintLamp(lamp);
    art.lamp = gs::uploadMipped(vdp, lamp);

    art.title = phrase(vdp, "S3 PINSMARK");
    art.finished = phrase(vdp, "FINISHED MARK");
    art.leave = phrase(vdp, "LEAVE");
    art.open = phrase(vdp, "OPEN SHEET");
    art.markOpen = phrase(vdp, "MARK OPEN");
}

}  // namespace pinsmark
