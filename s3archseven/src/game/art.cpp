#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace archseven {
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void paintFace(gs::Bitmap& b) {
    const float cx = kFaceMid;
    const float cy = kFaceMid;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = (x + 0.5f) - cx;
            float dy = (y + 0.5f) - cy;
            float r = std::sqrt(dx * dx + dy * dy);
            int c = 0;
            if (r <= kBoss) c = 4;
            if (r <= kRRed) c = 3;
            if (r <= kRRing) c = 2;
            if (r <= kRGold) c = 1;
            if (r <= 2.2f) c = 6;
            if (c == 4) {
                uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
                if ((h & 7u) == 0) c = 5;
            }
            auto edge = [&](float e) { return r > 2.4f && r <= kBoss && std::fabs(r - e) < 0.9f; };
            if (edge(kRGold) || edge(kRRing) || edge(kRRed) || edge(kBoss)) c = 7;
            b.set(x, y, c);
        }
    }
}

void paintArcher(gs::Bitmap& b, int pose) {
    b.rect(18, 28, 6, 22, 4);
    b.rect(26, 22, 10, 8, 3);
    b.ellipse(31, 16, 7.f, 7.f, 2);
    b.rect(22, 36, 16, 22, 5);
    b.rect(20, 56, 6, 18, 6);
    b.rect(32, 56, 6, 18, 6);
    b.rect(18, 72, 10, 4, 7);
    b.rect(30, 72, 10, 4, 7);
    float bowX = 48.f;
    b.line(bowX, 8, bowX + 2, 48, 8, 2.2f);
    b.line(bowX, 8, bowX - 10, 28, 8, 1.6f);
    b.line(bowX, 48, bowX - 10, 28, 8, 1.6f);
    if (pose == 0) {
        b.line(28, 40, bowX - 8, 28, 9, 2.f);
        b.rect(44, 26, 10, 3, 10);
    } else if (pose == 1) {
        b.line(22, 38, bowX - 6, 28, 9, 2.f);
        b.line(bowX - 8, 28, 22, 38, 11, 1.2f);
        b.rect(20, 36, 8, 3, 10);
    } else {
        b.line(30, 36, bowX - 4, 26, 9, 2.f);
        b.rect(46, 24, 16, 3, 10);
    }
    b.rect(28, 30, 3, 3, 1);
}

void paintArrow(gs::Bitmap& b) {
    b.rect(0, 2, 22, 2, 1);
    b.poly({{22, 0}, {27, 3}, {22, 6}}, 2);
    b.rect(0, 1, 4, 1, 3);
    b.rect(0, 4, 4, 1, 3);
}

void paintSight(gs::Bitmap& b) {
    b.rect(5, 0, 1, 11, 1);
    b.rect(0, 5, 11, 1, 1);
    b.set(5, 5, 0);
}

void paintDot(gs::Bitmap& b, int c) {
    b.ellipse(2.5f, 2.5f, 2.2f, 2.2f, c);
    b.set(2, 2, 2);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FACE,
           {gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 2), gs::rgb4(14, 10, 1), gs::rgb4(13, 2, 2), gs::rgb4(12, 9, 4),
            gs::rgb4(9, 7, 3), gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_ARCH,
           {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 8), gs::rgb4(12, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 3),
            gs::rgb4(3, 6, 9), gs::rgb4(2, 3, 6), gs::rgb4(4, 3, 2), gs::rgb4(10, 7, 3), gs::rgb4(14, 11, 7),
            gs::rgb4(13, 12, 10), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_ARROW, {gs::rgb4(0, 0, 0), gs::rgb4(12, 9, 5), gs::rgb4(14, 14, 12), gs::rgb4(10, 2, 2)});
    setPal(vdp, PAL_SIGHT, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_MARK, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 4), gs::rgb4(2, 2, 2), gs::rgb4(4, 8, 15)});
    textPal(vdp, PAL_WORD, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 1));
    textPal(vdp, PAL_HUD, gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 3));
    textPal(vdp, PAL_HUD_GOLD, gs::rgb4(15, 12, 2), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_HUD_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(3, 1, 1));
    setPal(vdp, PAL_FILL, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 2), gs::rgb4(6, 5, 4)});

    gs::Bitmap face(kFace, kFace);
    paintFace(face);
    art.face = gs::uploadImage(vdp, face);

    for (int pose = 0; pose < 3; pose++) {
        gs::Bitmap body(64, 80);
        paintArcher(body, pose);
        art.archer[pose] = gs::uploadImage(vdp, body);
    }

    gs::Bitmap arrow(28, 7);
    paintArrow(arrow);
    art.arrow = gs::uploadImage(vdp, arrow);

    gs::Bitmap sight(11, 11);
    paintSight(sight);
    art.sight = gs::uploadImage(vdp, sight);

    for (int i = 0; i < 4; i++) {
        gs::Bitmap d(5, 5);
        paintDot(d, i == 3 ? 3 : 1);
        art.dot[i] = gs::uploadImage(vdp, d);
    }

    art.wordArch = gs::uploadImage(vdp, gs::textBitmap("ARCH", {3, 1, 2, 0, 1}));
    art.wordSeven = gs::uploadImage(vdp, gs::textBitmap("FIRST TO SEVEN", {2, 1, 2, 0, 1}));
    art.wordWin = gs::uploadImage(vdp, gs::textBitmap("SEVEN", {3, 1, 2, 0, 1}));
    gs::Bitmap px(1, 1);
    px.set(0, 0, 1);
    art.px = gs::uploadImage(vdp, px);

    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 8, 12));
}

}  // namespace archseven
