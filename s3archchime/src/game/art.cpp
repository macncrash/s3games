#include "game/art.h"

#include <cstdint>
#include <initializer_list>

#include "console/gfx.h"

namespace archchime {
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
    vdp.setColor(pal * 16 + 15, edge);
}

int faceInk(float r) {
    const float rings[] = {kGold, kRed, kBlue, kBlack, kWhite, kStraw};
    for (float rr : rings)
        if (std::fabs(r - rr) <= 0.8f) return 7;
    if (r <= kGold) return 1;
    if (r <= kRed) return 2;
    if (r <= kBlue) return 3;
    if (r <= kBlack) return 4;
    if (r <= kWhite) return 5;
    if (r <= kStraw) return 6;
    return 0;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kFace, kFace);
    float c = kFaceMid;
    for (int y = 0; y < kFace; y++) {
        for (int x = 0; x < kFace; x++) {
            float dx = float(x) + 0.5f - c;
            float dy = float(y) + 0.5f - c;
            float r = std::hypot(dx, dy);
            int ink = faceInk(r);
            if (ink) b.set(x, y, ink);
        }
    }
    b.ellipse(c, c, 2.2f, 2.2f, 8);
    return b;
}

gs::Bitmap archerArt() {
    gs::Bitmap b(92, 108);
    b.ellipse(30.f, 14.f, 8.f, 9.f, 1);
    b.rect(24, 22, 12, 5, 1);
    b.rect(22, 28, 16, 30, 2);
    b.rect(18, 32, 8, 18, 2);
    b.rect(38, 36, 28, 5, 3);
    b.line(66, 22, 84, 46, 4, 2.4f);
    b.line(66, 70, 84, 46, 4, 2.4f);
    b.line(68, 30, 80, 46, 5, 1.3f);
    b.rect(24, 56, 8, 32, 6);
    b.rect(36, 56, 8, 32, 6);
    b.rect(22, 86, 12, 6, 7);
    b.rect(36, 86, 12, 6, 7);
    b.rect(14, 40, 8, 6, 1);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(26, 7);
    b.rect(2, 3, 18, 1, 1);
    b.poly({{18, 0}, {25, 3}, {18, 6}}, 2);
    b.rect(0, 1, 3, 5, 3);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(15, 15);
    b.rect(7, 0, 1, 5, 1);
    b.rect(7, 10, 1, 5, 1);
    b.rect(0, 7, 5, 1, 1);
    b.rect(10, 7, 5, 1, 1);
    b.set(7, 7, 1);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(36, 36);
    b.ellipse(18.f, 18.f, 16.f, 16.f, 1);
    b.ellipse(18.f, 18.f, 13.f, 13.f, 2);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 0.5235988f;
        int x = int(std::lround(18.f + std::sin(a) * 11.f));
        int y = int(std::lround(18.f - std::cos(a) * 11.f));
        b.set(x, y, i % 3 == 0 ? 4 : 3);
    }
    b.set(18, 18, 4);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(28, 70);
    b.rect(12, 0, 4, 58, 1);
    b.poly({{2, 68}, {14, 50}, {26, 68}}, 2);
    b.rect(4, 64, 20, 4, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 48);
    b.rect(12, 28, 4, 18, 1);
    b.ellipse(14.f, 16.f, 12.f, 14.f, 2);
    b.ellipse(14.f, 14.f, 6.f, 7.f, 3);
    return b;
}

gs::Bitmap belfryArt() {
    gs::Bitmap b(22, 40);
    b.rect(4, 12, 14, 26, 1);
    b.poly({{1, 14}, {11, 1}, {21, 14}}, 2);
    b.rect(8, 4, 6, 6, 3);
    b.rect(9, 22, 4, 8, 4);
    b.rect(2, 36, 18, 3, 5);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(3, 3);
    b.rect(0, 0, 3, 3, 1);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FACE,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(12, 2, 2), gs::rgb4(2, 4, 12), gs::rgb4(1, 1, 2), gs::rgb4(14, 14, 12),
            gs::rgb4(10, 8, 3), gs::rgb4(2, 1, 1), gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_ARCH,
           {0, gs::rgb4(12, 8, 5), gs::rgb4(3, 6, 11), gs::rgb4(8, 5, 3), gs::rgb4(5, 3, 2), gs::rgb4(14, 12, 8),
            gs::rgb4(2, 3, 6), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ARROW, {0, gs::rgb4(10, 7, 3), gs::rgb4(13, 13, 12), gs::rgb4(12, 3, 2)});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WORLD,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), gs::rgb4(8, 8, 8), gs::rgb4(5, 4, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 13), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_WORD, gs::rgb4(15, 14, 8), gs::rgb4(2, 1, 0));

    art.face = gs::uploadImage(vdp, faceArt());
    art.archer = gs::uploadImage(vdp, archerArt());
    art.arrow = gs::uploadImage(vdp, arrowArt());
    art.sight = gs::uploadImage(vdp, sightArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    art.stand = gs::uploadImage(vdp, standArt());
    art.tree = gs::uploadImage(vdp, treeArt());
    art.belfry = gs::uploadImage(vdp, belfryArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.shadow = 15;
    st.spacing = 1;
    art.word = gs::uploadImage(vdp, gs::textBitmap("ARCH", st));
    loadFont(vdp, art);
}

}  // namespace archchime
