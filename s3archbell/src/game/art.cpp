#include "game/art.h"

#include <cstdint>
#include <initializer_list>

#include "console/gfx.h"

namespace archbell {
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
    const float rings[] = {kBellR, kGold, kRed, kBlue, kBlack, kWhite, kBoss};
    for (float rr : rings)
        if (std::fabs(r - rr) <= 0.85f) return 7;
    if (r <= kBellR) return 0;
    if (r <= kGold) return 1;
    if (r <= kRed) return 2;
    if (r <= kBlue) return 3;
    if (r <= kBlack) return 4;
    if (r <= kWhite) return 5;
    return 6;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kFace, kFace);
    float c = kFaceMid;
    for (int y = 0; y < kFace; y++) {
        for (int x = 0; x < kFace; x++) {
            float dx = float(x) + 0.5f - c;
            float dy = float(y) + 0.5f - c;
            float r = std::hypot(dx, dy);
            if (r > kBoss + 1.2f) continue;
            b.set(x, y, faceInk(r));
        }
    }
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(22, 26);
    b.rect(10, 0, 2, 4, 3);
    b.ellipse(11.f, 14.f, 8.2f, 8.6f, 1);
    b.ellipse(11.f, 13.2f, 5.2f, 5.4f, 2);
    b.rect(4, 20, 14, 3, 4);
    b.ellipse(11.f, 22.f, 1.4f, 1.6f, 5);
    return b;
}

gs::Bitmap archerArt() {
    gs::Bitmap b(96, 120);
    b.ellipse(34.f, 16.f, 8.f, 9.f, 1);
    b.rect(28, 24, 12, 6, 1);
    b.rect(26, 30, 16, 28, 2);
    b.rect(22, 32, 8, 22, 2);
    b.rect(42, 34, 22, 6, 3);
    b.line(64, 28, 78, 52, 4, 2.2f);
    b.line(64, 76, 78, 52, 4, 2.2f);
    b.line(66, 36, 74, 52, 5, 1.4f);
    b.rect(30, 56, 8, 34, 6);
    b.rect(40, 56, 8, 34, 6);
    b.rect(28, 88, 10, 6, 7);
    b.rect(40, 88, 10, 6, 7);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(36, 8);
    b.rect(0, 3, 26, 2, 1);
    b.poly({{26, 1}, {35, 4}, {26, 7}}, 2);
    b.line(2, 1, 8, 4, 3, 1.2f);
    b.line(2, 7, 8, 4, 3, 1.2f);
    return b;
}

gs::Bitmap sightArt() {
    gs::Bitmap b(13, 13);
    b.rect(6, 0, 1, 13, 1);
    b.rect(0, 6, 13, 1, 1);
    b.set(6, 6, 0);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(28, 70);
    b.rect(12, 0, 4, 58, 1);
    b.poly({{2, 68}, {14, 52}, {26, 68}}, 2);
    return b;
}

gs::Bitmap quiverArt() {
    gs::Bitmap b(16, 28);
    b.poly({{3, 6}, {13, 6}, {12, 26}, {4, 26}}, 1);
    b.rect(4, 4, 8, 4, 2);
    return b;
}

gs::Bitmap shaftArt() {
    gs::Bitmap b(4, 18);
    b.rect(1, 2, 2, 14, 1);
    b.poly({{0, 2}, {2, 0}, {4, 2}}, 2);
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
           {0, gs::rgb4(15, 12, 2), gs::rgb4(13, 2, 2), gs::rgb4(2, 4, 12), gs::rgb4(1, 1, 2), gs::rgb4(14, 14, 13),
            gs::rgb4(11, 8, 3), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_ARCH,
           {0, gs::rgb4(12, 8, 5), gs::rgb4(4, 8, 12), gs::rgb4(8, 5, 3), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 8),
            gs::rgb4(2, 3, 6), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_ARROW, {0, gs::rgb4(10, 7, 3), gs::rgb4(12, 12, 11), gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WORLD, {0, gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 13), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 3), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_WORD, gs::rgb4(15, 14, 8), gs::rgb4(2, 1, 0));

    art.face = gs::uploadImage(vdp, faceArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.archer = gs::uploadImage(vdp, archerArt());
    art.arrow = gs::uploadImage(vdp, arrowArt());
    art.sight = gs::uploadImage(vdp, sightArt());
    art.stand = gs::uploadImage(vdp, standArt());
    art.quiver = gs::uploadImage(vdp, quiverArt());
    art.shaft = gs::uploadImage(vdp, shaftArt());
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.shadow = 15;
    st.spacing = 1;
    art.word = gs::uploadImage(vdp, gs::textBitmap("ARCH", st));
    loadFont(vdp, art);
}

}  // namespace archbell
