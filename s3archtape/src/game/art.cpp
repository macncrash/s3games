#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace archtape {
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
            if (r <= kBoss) c = 7;
            for (int i = 5; i >= 0; i--) {
                if (r <= kBand[i].rOut) c = kBand[i].ink;
            }
            if (r <= 2.1f) c = 9;
            if (r > 2.4f && r <= kBoss) {
                for (const Band& band : kBand) {
                    if (std::fabs(r - band.rOut) < 0.75f) c = 8;
                }
                if (std::fabs(r - kBoss) < 0.9f) c = 8;
            }
            if (c == 7) {
                uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
                if ((h & 7u) == 0) c = 10;
            }
            b.set(x, y, c);
        }
    }
}

void paintArcher(gs::Bitmap& b, int pose) {
    b.rect(16, 30, 5, 20, 4);
    b.rect(22, 24, 9, 7, 3);
    b.ellipse(26, 17, 6.f, 6.f, 2);
    b.rect(18, 34, 14, 18, 5);
    b.rect(17, 50, 5, 16, 6);
    b.rect(27, 50, 5, 16, 6);
    b.rect(15, 64, 8, 3, 7);
    b.rect(25, 64, 8, 3, 7);
    float bowX = 46.f;
    b.line(bowX, 6, bowX + 1, 50, 8, 2.f);
    b.line(bowX, 6, bowX - 12, 28, 8, 1.5f);
    b.line(bowX, 50, bowX - 12, 28, 8, 1.5f);
    if (pose == 0) {
        b.line(24, 38, bowX - 8, 28, 9, 2.f);
        b.rect(40, 26, 8, 2, 10);
    } else if (pose == 1) {
        b.line(16, 36, bowX - 6, 28, 9, 2.f);
        b.line(bowX - 10, 28, 16, 36, 11, 1.1f);
        b.rect(14, 34, 8, 3, 10);
    } else {
        b.line(26, 34, bowX - 2, 26, 9, 2.f);
        b.rect(42, 24, 14, 2, 10);
    }
    b.rect(23, 28, 3, 3, 1);
}

void paintArrow(gs::Bitmap& b) {
    b.rect(0, 2, 20, 2, 1);
    b.poly({{20, 0}, {26, 3}, {20, 6}}, 2);
    b.rect(0, 1, 4, 1, 3);
    b.rect(0, 4, 4, 1, 3);
}

void paintSight(gs::Bitmap& b) {
    b.rect(5, 0, 1, 11, 1);
    b.rect(0, 5, 11, 1, 1);
    b.set(5, 5, 0);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FACE,
           {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 3), gs::rgb4(13, 12, 8), gs::rgb4(15, 11, 1), gs::rgb4(12, 10, 5),
            gs::rgb4(13, 2, 2), gs::rgb4(8, 3, 2), gs::rgb4(11, 8, 4), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 14),
            gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_ARCH,
           {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 8), gs::rgb4(12, 7, 4), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 3),
            gs::rgb4(2, 5, 9), gs::rgb4(2, 3, 5), gs::rgb4(4, 3, 2), gs::rgb4(9, 6, 3), gs::rgb4(14, 11, 7),
            gs::rgb4(13, 12, 9), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_ARROW, {gs::rgb4(0, 0, 0), gs::rgb4(12, 9, 5), gs::rgb4(14, 14, 12), gs::rgb4(11, 2, 2)});
    setPal(vdp, PAL_SIGHT, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_MARK, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 15), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(8, 5, 2), gs::rgb4(12, 9, 4), gs::rgb4(4, 3, 1)});
    textPal(vdp, PAL_WORD, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 1));
    textPal(vdp, PAL_HUD, gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 3));
    textPal(vdp, PAL_HUD_GOLD, gs::rgb4(15, 12, 2), gs::rgb4(4, 2, 0));
    textPal(vdp, PAL_HUD_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(3, 1, 1));
    setPal(vdp, PAL_FILL, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 2), gs::rgb4(5, 4, 3)});

    loadFont(vdp, art);

    gs::Bitmap face(kFace, kFace);
    paintFace(face);
    art.face = gs::uploadImage(vdp, face);

    for (int pose = 0; pose < 3; pose++) {
        gs::Bitmap body(60, 72);
        paintArcher(body, pose);
        art.archer[pose] = gs::uploadImage(vdp, body);
    }

    gs::Bitmap arrow(27, 7);
    paintArrow(arrow);
    art.arrow = gs::uploadImage(vdp, arrow);

    gs::Bitmap sight(11, 11);
    paintSight(sight);
    art.sight = gs::uploadImage(vdp, sight);

    gs::Bitmap pip(5, 5);
    pip.ellipse(2.5f, 2.5f, 2.1f, 2.1f, 1);
    pip.set(2, 2, 2);
    art.pip = gs::uploadImage(vdp, pip);

    gs::Bitmap drawer(168, 18);
    drawer.rect(0, 0, 168, 18, 1);
    drawer.rect(2, 2, 164, 14, 2);
    drawer.rect(4, 4, 48, 10, 3);
    drawer.rect(60, 4, 48, 10, 3);
    drawer.rect(116, 4, 48, 10, 3);
    art.drawer = gs::uploadImage(vdp, drawer);

    art.wordArch = gs::uploadImage(vdp, gs::textBitmap("ARCH", {3, 1, 2, 0, 1}));
    art.wordTape = gs::uploadImage(vdp, gs::textBitmap("MATCH THE TAPE", {2, 1, 2, 0, 1}));
    art.wordLeave = gs::uploadImage(vdp, gs::textBitmap("YOU LEAVE", {3, 1, 2, 0, 1}));
    gs::Bitmap px(1, 1);
    px.set(0, 0, 1);
    art.px = gs::uploadImage(vdp, px);
}

}  // namespace archtape
