#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace wicketmark {
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

gs::Image up(gs::VDP& vdp, const gs::Bitmap& b) { return gs::uploadImage(vdp, b); }

void paintBowler(gs::Bitmap& b, int pose) {
    b.ellipse(18, 9, 5.2f, 5.4f, 2);
    b.rect(13, 3, 10, 4, 7);
    b.rect(14, 5, 8, 2, 4);
    b.rect(12, 15, 12, 13, 3);
    b.rect(12, 15, 3, 13, 4);
    b.rect(12, 25, 12, 3, 9);
    b.rect(14, 28, 8, 8, 5);
    if (pose == 0) {
        b.rect(14, 36, 4, 14, 5);
        b.rect(20, 36, 4, 14, 5);
        b.rect(13, 48, 6, 3, 6);
        b.rect(19, 48, 6, 3, 6);
        b.rect(10, 18, 4, 10, 2);
        b.rect(22, 18, 4, 8, 2);
    } else if (pose == 1) {
        b.line(16, 36, 8, 50, 5, 3.4f);
        b.line(22, 36, 28, 48, 5, 3.4f);
        b.rect(5, 48, 7, 3, 6);
        b.rect(25, 46, 7, 3, 6);
        b.line(14, 18, 6, 28, 2, 3.f);
        b.line(22, 17, 28, 10, 2, 3.f);
    } else {
        b.line(16, 36, 12, 50, 5, 3.4f);
        b.line(22, 36, 26, 50, 5, 3.4f);
        b.rect(9, 48, 7, 3, 6);
        b.rect(23, 48, 7, 3, 6);
        b.line(16, 18, 3, 8, 2, 3.2f);
        b.ellipse(3, 8, 2.2f, 2.2f, 2);
        b.rect(22, 18, 4, 8, 2);
    }
    b.outline(1, false);
}

void paintBatsman(gs::Bitmap& b, int pose) {
    b.ellipse(16, 8, 6.2f, 6.4f, 9);
    b.rect(12, 3, 9, 3, 9);
    b.ellipse(18, 11, 3.2f, 3.4f, 2);
    b.rect(15, 10, 6, 1, 10);
    b.rect(15, 12, 6, 1, 10);
    b.rect(11, 16, 12, 12, 3);
    b.rect(11, 16, 3, 12, 4);
    b.rect(12, 28, 5, 16, 5);
    b.rect(19, 28, 5, 16, 5);
    b.rect(12, 30, 5, 2, 6);
    b.rect(19, 30, 5, 2, 6);
    b.rect(12, 36, 5, 2, 6);
    b.rect(19, 36, 5, 2, 6);
    b.rect(11, 43, 7, 4, 11);
    b.rect(18, 43, 7, 4, 11);
    b.rect(20, 18, 5, 5, 2);
    if (pose == 0) {
        b.rect(24, 16, 5, 30, 7);
        b.rect(25, 18, 2, 26, 8);
        b.rect(22, 14, 6, 4, 8);
    } else {
        b.line(22, 20, 34, 8, 7, 4.2f);
        b.line(24, 22, 33, 11, 8, 2.f);
        b.rect(20, 18, 4, 4, 8);
    }
    b.outline(1, false);
}

void paintKeeper(gs::Bitmap& b) {
    b.ellipse(14, 8, 5.f, 5.2f, 2);
    b.rect(10, 3, 8, 3, 5);
    b.rect(10, 14, 10, 10, 3);
    b.rect(8, 22, 6, 14, 4);
    b.rect(16, 22, 6, 14, 4);
    b.rect(8, 24, 6, 2, 1);
    b.rect(16, 24, 6, 2, 1);
    b.rect(7, 34, 7, 3, 7);
    b.rect(16, 34, 7, 3, 7);
    b.ellipse(22, 16, 3.4f, 3.f, 6);
    b.ellipse(23, 20, 3.f, 2.6f, 6);
    b.outline(1, false);
}

void paintStumps(gs::Bitmap& b, bool broken) {
    if (!broken) {
        b.rect(4, 10, 3, 28, 3);
        b.rect(10, 10, 3, 28, 2);
        b.rect(16, 10, 3, 28, 3);
        b.rect(5, 10, 1, 28, 4);
        b.rect(11, 10, 1, 28, 4);
        b.rect(4, 8, 8, 3, 5);
        b.rect(11, 8, 8, 3, 5);
        b.rect(4, 8, 15, 1, 4);
    } else {
        b.line(6, 38, 1, 14, 3, 3.f);
        b.line(11, 38, 12, 12, 2, 3.f);
        b.line(16, 38, 22, 16, 3, 3.f);
        b.rect(4, 36, 16, 3, 2);
    }
    b.outline(1, false);
}

void paintBall(gs::Bitmap& b, int frame) {
    b.ellipse(7, 7, 6.f, 6.f, 1);
    b.ellipse(7, 7, 5.f, 5.f, 2);
    b.ellipse(7, 7, 4.f, 4.f, 3);
    b.ellipse(5.4f, 5.2f, 1.6f, 1.2f, 4);
    if (frame == 0) b.line(3, 3, 11, 11, 5, 1.3f);
    else b.line(2, 8, 12, 5, 5, 1.3f);
}

void paintCoin(gs::Bitmap& b) {
    b.ellipse(8, 8, 7.f, 7.f, 1);
    b.ellipse(8, 8, 5.8f, 5.8f, 2);
    b.ellipse(8, 8, 4.6f, 4.6f, 3);
    b.ellipse(8, 8, 3.4f, 3.4f, 4);
    b.rect(6, 4, 1, 6, 1);
    b.rect(8, 4, 1, 6, 1);
    b.rect(10, 4, 1, 6, 1);
    b.rect(6, 3, 5, 1, 5);
    b.set(4, 4, 6);
}

void paintPitch(gs::Bitmap& b) {
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            bool fringe = y < 3 || y > b.h - 4;
            int c = fringe ? 5 : 3;
            if (!fringe && ((x / 10) & 1) == 0) c = 4;
            if (!fringe && y > 12 && y < b.h - 14 && x > 28 && x < 210) c = 2;
            if (!fringe && y > 14 && y < b.h - 16 && x >= 200 && x < 296) c = 4;
            b.set(x, y, c);
        }
    }
    b.line(48, 22, 120, 24, 1, 1.f);
    b.line(140, 30, 190, 28, 1, 1.f);
    b.line(220, 26, 270, 24, 1, 1.f);
}

void paintTree(gs::Bitmap& b) {
    b.rect(11, 20, 4, 16, 6);
    b.rect(10, 34, 6, 3, 5);
    b.ellipse(13, 14, 10.f, 11.f, 2);
    b.ellipse(13, 12, 7.f, 7.f, 3);
    b.ellipse(9, 10, 3.f, 2.4f, 4);
    b.outline(1, false);
}

void paintStand(gs::Bitmap& b) {
    b.rect(2, 16, 68, 18, 2);
    b.rect(2, 16, 68, 4, 3);
    b.poly({{2, 16}, {36, 4}, {70, 16}}, 3);
    b.rect(6, 22, 8, 10, 4);
    b.rect(20, 22, 8, 10, 4);
    b.rect(40, 22, 8, 10, 4);
    b.rect(54, 22, 8, 10, 4);
    b.ellipse(36, 12, 4.f, 4.f, 4);
    b.ellipse(36, 12, 2.6f, 2.6f, 8);
    b.rect(8, 24, 4, 6, 9);
    b.rect(42, 24, 4, 6, 9);
    b.outline(1, false);
}

void paintCrowd(gs::Bitmap& b) {
    for (int i = 0; i < 8; i++) {
        int x = 4 + i * 7;
        int shirt = (i & 1) ? 6 : 7;
        b.ellipse(float(x), 6, 2.6f, 2.6f, 5);
        b.rect(x - 2, 9, 5, 6, shirt);
        b.rect(x - 2, 3, 4, 2, 8);
    }
    b.outline(1, false);
}

void paintSun(gs::Bitmap& b) {
    b.ellipse(8, 8, 5.f, 5.f, 1);
    b.ellipse(8, 8, 3.6f, 3.6f, 2);
    b.ellipse(8, 8, 2.f, 2.f, 3);
    b.rect(7, 0, 2, 2, 2);
    b.rect(7, 14, 2, 2, 2);
    b.rect(0, 7, 2, 2, 2);
    b.rect(14, 7, 2, 2, 2);
}

void paintCloud(gs::Bitmap& b) {
    b.ellipse(10, 8, 7.f, 4.f, 5);
    b.ellipse(16, 8, 6.f, 4.4f, 4);
    b.ellipse(20, 9, 5.f, 3.2f, 5);
}

void paintScreen(gs::Bitmap& b) {
    b.rect(2, 2, 16, 26, 1);
    b.rect(4, 4, 12, 22, 4);
    b.rect(8, 26, 3, 6, 9);
    b.outline(1, false);
}

void paintCrease(gs::Bitmap& b) {
    b.rect(0, 0, 3, 48, 1);
}

void paintScratch(gs::Bitmap& b) {
    b.line(2, 8, 6, 2, 3, 1.5f);
    b.line(7, 8, 11, 1, 2, 1.5f);
    b.line(12, 8, 16, 2, 4, 1.4f);
}

void paintShadow(gs::Bitmap& b) { b.ellipse(9, 4, 8.f, 2.6f, 1); }

void paintTuft(gs::Bitmap& b) {
    b.line(2, 10, 3, 2, 3, 1.4f);
    b.line(5, 10, 6, 1, 4, 1.4f);
    b.line(8, 10, 7, 3, 2, 1.4f);
}

void paintBail(gs::Bitmap& b) {
    b.rect(0, 1, 10, 3, 5);
    b.rect(0, 1, 10, 1, 4);
}

void paintDot(gs::Bitmap& b) { b.ellipse(2, 2, 1.6f, 1.6f, 1); }

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 15), gs::rgb4(2, 2, 4));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 4), gs::rgb4(4, 2, 1));
    textPal(vdp, PAL_GREEN, gs::rgb4(8, 14, 6), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(14, 4, 3), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 11), gs::rgb4(3, 2, 2));
    setPal(vdp, PAL_BALL, {0, gs::rgb4(3, 1, 1), gs::rgb4(8, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(15, 8, 6),
                           gs::rgb4(15, 14, 11), gs::rgb4(15, 6, 4)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(4, 2, 1), gs::rgb4(10, 7, 2), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8),
                           gs::rgb4(12, 8, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(14, 12, 8),
                           gs::rgb4(15, 15, 14), gs::rgb4(6, 3, 1)});
    setPal(vdp, PAL_WHITE,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(13, 9, 6), gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11), gs::rgb4(14, 14, 13),
            gs::rgb4(6, 6, 7), gs::rgb4(11, 7, 3), gs::rgb4(14, 11, 6), gs::rgb4(3, 3, 5), gs::rgb4(1, 1, 2),
            gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_BOWL,
           {0, gs::rgb4(1, 1, 3), gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 8), gs::rgb4(4, 6, 12), gs::rgb4(14, 14, 13),
            gs::rgb4(3, 3, 4), gs::rgb4(1, 2, 5), gs::rgb4(8, 8, 10), gs::rgb4(12, 2, 2)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(1, 3, 1), gs::rgb4(2, 6, 2), gs::rgb4(3, 9, 3), gs::rgb4(6, 12, 4),
                            gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(8, 6, 2), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), gs::rgb4(14, 14, 15),
                          gs::rgb4(10, 11, 13)});
    setPal(vdp, PAL_STAND,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(9, 5, 3), gs::rgb4(6, 2, 2), gs::rgb4(12, 13, 13), gs::rgb4(13, 9, 6),
            gs::rgb4(11, 2, 2), gs::rgb4(2, 3, 8), gs::rgb4(14, 12, 6), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_PITCH, {0, gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 2), gs::rgb4(12, 8, 3), gs::rgb4(13, 10, 5),
                            gs::rgb4(3, 8, 3), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_KEEP, {0, gs::rgb4(1, 2, 2), gs::rgb4(13, 9, 6), gs::rgb4(15, 15, 14), gs::rgb4(13, 13, 12),
                           gs::rgb4(2, 8, 3), gs::rgb4(12, 12, 8), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 1, 2)});

    loadFont(vdp, art);

    gs::Bitmap bow(34, 54);
    for (int i = 0; i < 3; i++) {
        bow = gs::Bitmap(34, 54);
        paintBowler(bow, i);
        art.bowler[i] = up(vdp, bow);
    }
    for (int i = 0; i < 2; i++) {
        gs::Bitmap bat(36, 56);
        paintBatsman(bat, i);
        art.batsman[i] = up(vdp, bat);
    }
    gs::Bitmap keep(28, 42);
    paintKeeper(keep);
    art.keeper = up(vdp, keep);
    for (int i = 0; i < 2; i++) {
        gs::Bitmap st(26, 44);
        paintStumps(st, i == 1);
        art.stump[i] = up(vdp, st);
    }
    gs::Bitmap bail(10, 5);
    paintBail(bail);
    art.bail = up(vdp, bail);
    for (int i = 0; i < 2; i++) {
        gs::Bitmap ball(14, 14);
        paintBall(ball, i);
        art.ball[i] = up(vdp, ball);
    }
    gs::Bitmap coin(16, 16);
    paintCoin(coin);
    art.coin = up(vdp, coin);
    gs::Bitmap pitch(304, 54);
    paintPitch(pitch);
    art.pitch = up(vdp, pitch);
    gs::Bitmap tree(26, 40);
    paintTree(tree);
    art.tree = up(vdp, tree);
    gs::Bitmap stand(72, 40);
    paintStand(stand);
    art.stand = up(vdp, stand);
    gs::Bitmap crowd(60, 18);
    paintCrowd(crowd);
    art.crowd = up(vdp, crowd);
    gs::Bitmap sun(16, 16);
    paintSun(sun);
    art.sun = up(vdp, sun);
    gs::Bitmap cloud(30, 14);
    paintCloud(cloud);
    art.cloud = up(vdp, cloud);
    gs::Bitmap screen(20, 34);
    paintScreen(screen);
    art.screen = up(vdp, screen);
    gs::Bitmap crease(3, 48);
    paintCrease(crease);
    art.crease = up(vdp, crease);
    gs::Bitmap scratch(18, 10);
    paintScratch(scratch);
    art.scratch = up(vdp, scratch);
    gs::Bitmap shadow(18, 8);
    paintShadow(shadow);
    art.shadow = up(vdp, shadow);
    gs::Bitmap tuft(12, 12);
    paintTuft(tuft);
    art.tuft = up(vdp, tuft);
    gs::Bitmap dot(4, 4);
    paintDot(dot);
    art.dot = up(vdp, dot);

    gs::TextStyle st{2, 1, 2, 0, 1};
    art.logo = up(vdp, gs::textBitmap("WICKETMARK", st));
    art.finished = up(vdp, gs::textBitmap("FINISHED", st));
    art.wicketWord = up(vdp, gs::textBitmap("WICKET", st));
}

}  // namespace wicketmark
