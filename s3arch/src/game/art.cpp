#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace arch {
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

gs::Image phrase(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 2, 0, 1}));
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
            if (r <= kBoss) c = 7;
            if (r <= kR1) c = 1;
            if (r <= kR3) c = 2;
            if (r <= kR5) c = 3;
            if (r <= kR7) c = 4;
            if (r <= kR9) c = 5;
            if (r <= kR10) c = 6;
            if (r <= 1.6f) c = 10;
            if (c == 7) {
                uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
                if ((h & 15u) == 0) c = 8;
            }
            auto edge = [&](float e) { return r > 2.2f && r <= kBoss && std::fabs(r - e) < 0.85f; };
            if (edge(kR10) || edge(kR9) || edge(kR7) || edge(kR5) || edge(kR3) || edge(kR1) || edge(kBoss)) c = 9;
            if (c == 6 && dx < -1.f && dy < -1.f && r > 3.f) c = 11;
            b.set(x, y, c);
        }
    }
}

void paintArcher(gs::Bitmap& b, int pose) {
    b.rect(40, 40, 7, 28, 8);
    b.rect(41, 32, 2, 10, 11);
    b.rect(44, 30, 2, 12, 10);
    b.rect(47, 32, 2, 10, 11);

    b.rect(58, 72, 8, 38, 5);
    b.rect(70, 72, 8, 38, 5);
    b.rect(56, 106, 12, 8, 6);
    b.rect(70, 106, 12, 8, 6);

    b.rect(54, 44, 24, 32, 3);
    b.rect(54, 44, 8, 32, 4);
    b.rect(54, 70, 24, 5, 12);
    b.rect(62, 71, 4, 3, 13);

    b.ellipse(70, 30, 9.5f, 10.f, 1);
    b.ellipse(68, 22, 10.f, 7.f, 2);
    b.rect(64, 18, 14, 6, 2);
    b.rect(76, 24, 12, 3, 8);
    b.set(79, 30, 14);
    b.rect(77, 34, 4, 1, 14);

    b.rect(78, 34, 16, 5, 3);
    b.rect(92, 34, 8, 5, 1);

    b.rect(96, 16, 4, 44, 7);
    b.line(98, 18, 106, 6, 7, 2.4f);
    b.line(98, 58, 106, 70, 7, 2.4f);
    b.line(99, 18, 104, 8, 8, 1.2f);

    if (pose == 1) {
        b.line(104, 8, 60, 36, 9, 1.15f);
        b.line(60, 36, 104, 68, 9, 1.15f);
        b.ellipse(60, 36, 4.2f, 3.4f, 1);
        b.rect(62, 35, 34, 2, 10);
        b.poly({{104, 36}, {96, 33}, {96, 39}}, 15);
        b.poly({{66, 36}, {56, 30}, {64, 36}}, 11);
        b.poly({{66, 36}, {56, 42}, {64, 36}}, 11);
    } else if (pose == 2) {
        b.line(106, 8, 98, 36, 9, 1.15f);
        b.line(98, 36, 106, 68, 9, 1.15f);
    } else {
        b.line(94, 10, 94, 66, 9, 1.15f);
        b.rect(88, 35, 16, 2, 10);
        b.poly({{106, 36}, {98, 33}, {98, 39}}, 15);
        b.poly({{90, 36}, {82, 31}, {88, 36}}, 11);
    }
}

void paintArrow(gs::Bitmap& b) {
    b.rect(2, 4, 26, 2, 1);
    b.poly({{36, 5}, {28, 1}, {28, 9}}, 2);
    b.poly({{8, 5}, {1, 1}, {6, 5}}, 3);
    b.poly({{8, 5}, {1, 9}, {6, 5}}, 3);
    b.set(3, 4, 4);
    b.set(3, 5, 4);
}

void paintSight(gs::Bitmap& b) {
    b.line(8, 0, 8, 4, 1, 1.4f);
    b.line(8, 12, 8, 16, 1, 1.4f);
    b.line(0, 8, 4, 8, 1, 1.4f);
    b.line(12, 8, 16, 8, 1, 1.4f);
    b.set(8, 5, 2);
    b.set(8, 11, 2);
    b.set(5, 8, 2);
    b.set(11, 8, 2);
}

void paintDot(gs::Bitmap& b, int c) {
    b.ellipse(3.5f, 3.5f, 2.4f, 2.4f, c);
    b.set(3, 3, 7);
}

void paintStand(gs::Bitmap& b) {
    b.rect(8, 2, 48, 6, 3);
    b.rect(8, 2, 48, 2, 8);
    b.line(14, 8, 4, 66, 3, 3.2f);
    b.line(50, 8, 60, 66, 3, 3.2f);
    b.line(32, 6, 32, 64, 8, 3.f);
}

void paintFlag(gs::Bitmap& b, bool limp) {
    b.rect(16, 2, 3, 30, 8);
    if (limp) {
        b.rect(19, 6, 10, 8, 7);
        b.rect(19, 12, 8, 3, 5);
    } else {
        b.poly({{19, 4}, {38, 8}, {34, 14}, {19, 16}}, 7);
        b.poly({{22, 6}, {34, 9}, {30, 13}, {22, 14}}, 5);
    }
}

void paintHill(gs::Bitmap& b) {
    for (int x = 0; x < b.w; x++) {
        float ridge = 16.f + 7.f * std::sin(x * 0.035f) + 3.f * std::sin(x * 0.11f + 1.2f);
        for (int y = 0; y < b.h; y++) {
            if (float(y) < ridge) continue;
            b.set(x, y, (y & 3) == 0 ? 15 : 14);
        }
    }
}

void paintTree(gs::Bitmap& b) {
    b.rect(20, 40, 8, 30, 3);
    b.rect(18, 64, 12, 4, 8);
    b.ellipse(24, 28, 18, 16, 1);
    b.ellipse(16, 24, 10, 9, 2);
    b.ellipse(30, 22, 8, 7, 2);
}

void paintHut(gs::Bitmap& b) {
    b.poly({{4, 22}, {34, 4}, {64, 22}}, 5);
    b.rect(8, 20, 52, 26, 4);
    b.rect(8, 20, 6, 26, 8);
    b.rect(28, 30, 12, 16, 10);
    b.rect(14, 26, 10, 8, 11);
    b.rect(46, 26, 10, 8, 11);
}

void paintCloud(gs::Bitmap& b) {
    b.ellipse(16, 12, 12, 7, 6);
    b.ellipse(30, 10, 14, 8, 6);
    b.ellipse(42, 13, 9, 6, 6);
}

void paintSun(gs::Bitmap& b) {
    b.ellipse(11, 11, 6.5f, 6.5f, 13);
    b.ellipse(11, 11, 8.5f, 8.5f, 12);
    b.ellipse(11, 11, 5.f, 5.f, 13);
    b.line(11, 0, 11, 3, 12, 1.2f);
    b.line(11, 19, 11, 22, 12, 1.2f);
    b.line(0, 11, 3, 11, 12, 1.2f);
    b.line(19, 11, 22, 11, 12, 1.2f);
}

void paintBush(gs::Bitmap& b) {
    b.ellipse(10, 10, 8, 6, 1);
    b.ellipse(18, 12, 7, 5, 2);
}

void paintBadge(gs::Bitmap& b) {
    b.ellipse(20, 20, 18, 18, 4);
    b.ellipse(20, 20, 14.5f, 14.5f, 3);
    gs::Bitmap word = gs::textBitmap("x2", {2, 1, 2, 0, 1});
    b.blit(word, 20 - word.w / 2, 21 - word.h / 2);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3), gs::rgb4(2, 6, 14), gs::rgb4(13, 2, 2), gs::rgb4(13, 10, 2),
            gs::rgb4(15, 14, 4), gs::rgb4(11, 8, 3), gs::rgb4(8, 6, 2), gs::rgb4(1, 1, 1), gs::rgb4(1, 1, 2),
            gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_ARCH,
           {0, gs::rgb4(14, 10, 7), gs::rgb4(3, 2, 1), gs::rgb4(2, 7, 4), gs::rgb4(1, 4, 2), gs::rgb4(13, 13, 12),
            gs::rgb4(3, 2, 2), gs::rgb4(11, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(14, 14, 13), gs::rgb4(12, 9, 5),
            gs::rgb4(13, 2, 2), gs::rgb4(12, 9, 2), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 2), gs::rgb4(12, 13, 14)});
    setPal(vdp, PAL_ARROW, {0, gs::rgb4(12, 9, 5), gs::rgb4(12, 13, 14), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 15, 15), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_GHOST, {0, gs::rgb4(4, 14, 14)});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 13, 2), gs::rgb4(13, 2, 2), gs::rgb4(3, 6, 14), gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 14),
            gs::rgb4(8, 5, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WORLD,
           {0, gs::rgb4(3, 10, 4), gs::rgb4(2, 7, 3), gs::rgb4(8, 5, 2), gs::rgb4(13, 11, 8), gs::rgb4(12, 3, 2),
            gs::rgb4(15, 15, 15), gs::rgb4(14, 3, 3), gs::rgb4(7, 5, 3), gs::rgb4(4, 9, 4), gs::rgb4(4, 3, 2),
            gs::rgb4(14, 12, 6), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 8), gs::rgb4(4, 8, 5), gs::rgb4(3, 6, 4)});
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3));
    textPal(vdp, PAL_HUD_GOLD, gs::rgb4(15, 13, 3), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_HUD_ALERT, gs::rgb4(15, 6, 4), gs::rgb4(4, 1, 1));
    textPal(vdp, PAL_WORD, gs::rgb4(15, 14, 12), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_SHORT, gs::rgb4(15, 6, 4), gs::rgb4(4, 0, 0));
    setPal(vdp, PAL_BADGE, {0, gs::rgb4(4, 2, 0), gs::rgb4(2, 1, 0), gs::rgb4(15, 12, 2), gs::rgb4(10, 7, 1)});
    setPal(vdp, PAL_TRACK, {0, gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_FILL, {0, gs::rgb4(15, 14, 10)});

    loadFont(vdp, art);

    gs::Bitmap face(kFace, kFace);
    paintFace(face);
    art.face = gs::uploadImage(vdp, face);

    for (int pose = 0; pose < 3; pose++) {
        gs::Bitmap body(kArchW, kArchH);
        paintArcher(body, pose);
        art.archer[pose] = gs::uploadImage(vdp, body);
    }

    gs::Bitmap arrow(38, 10);
    paintArrow(arrow);
    art.arrow = gs::uploadImage(vdp, arrow);

    gs::Bitmap sight(17, 17);
    paintSight(sight);
    art.sight = gs::uploadImage(vdp, sight);

    gs::Bitmap ghost(8, 8);
    ghost.ellipse(4, 4, 3.1f, 3.1f, 1);
    art.ghost = gs::uploadImage(vdp, ghost);

    for (int i = 0; i < 6; i++) {
        gs::Bitmap dot(7, 7);
        paintDot(dot, i + 1);
        art.dot[i] = gs::uploadImage(vdp, dot);
    }

    gs::Bitmap stand(64, 70);
    paintStand(stand);
    art.stand = gs::uploadImage(vdp, stand);

    gs::Bitmap flag(40, 34);
    paintFlag(flag, false);
    art.flag = gs::uploadImage(vdp, flag);
    gs::Bitmap limp(40, 34);
    paintFlag(limp, true);
    art.limp = gs::uploadImage(vdp, limp);

    gs::Bitmap hill(320, 40);
    paintHill(hill);
    art.hill = gs::uploadImage(vdp, hill);

    gs::Bitmap tree(48, 74);
    paintTree(tree);
    art.tree = gs::uploadImage(vdp, tree);

    gs::Bitmap hut(68, 48);
    paintHut(hut);
    art.hut = gs::uploadImage(vdp, hut);

    gs::Bitmap cloud(52, 20);
    paintCloud(cloud);
    art.cloud = gs::uploadImage(vdp, cloud);

    gs::Bitmap sun(22, 22);
    paintSun(sun);
    art.sun = gs::uploadImage(vdp, sun);

    gs::Bitmap bush(28, 16);
    paintBush(bush);
    art.bush = gs::uploadImage(vdp, bush);

    gs::Bitmap shadow(36, 12);
    shadow.ellipse(18, 6, 16, 4, 1);
    art.shadow = gs::uploadImage(vdp, shadow);

    gs::Bitmap badge(40, 40);
    paintBadge(badge);
    art.badge = gs::uploadImage(vdp, badge);

    art.wordArch = phrase(vdp, "ARCH", 3);
    art.wordWin = phrase(vdp, "MADE THE LINE", 2);
    art.wordShort = phrase(vdp, "SHORT", 3);

    gs::Bitmap px(8, 8);
    px.rect(0, 0, 8, 8, 1);
    art.px = gs::uploadImage(vdp, px);
}

}  // namespace arch
