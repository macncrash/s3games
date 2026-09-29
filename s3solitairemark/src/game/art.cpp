#include "game/art.h"

#include <cstring>

namespace solitaire {
namespace {

const char* rankName(int rank) {
    static const char* names[] = {"", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
    if (rank < 1 || rank > 13) return "";
    return names[rank];
}

void heart(gs::Bitmap& b, int cx, int cy, int s, int c) {
    b.ellipse(float(cx - s / 3), float(cy - s / 5), float(s / 2), float(s / 2), c);
    b.ellipse(float(cx + s / 3), float(cy - s / 5), float(s / 2), float(s / 2), c);
    b.poly({{float(cx - s), float(cy)}, {float(cx + s), float(cy)}, {float(cx), float(cy + s + s / 2)}}, c);
}

void paintCard(gs::Bitmap& b, int rank) {
    b.rect(1, 1, float(kCardW - 2), float(kCardH - 2), 1);
    b.rect(0, 2, 1, float(kCardH - 4), 2);
    b.rect(float(kCardW - 1), 2, 1, float(kCardH - 4), 2);
    b.rect(2, 0, float(kCardW - 4), 1, 2);
    b.rect(2, float(kCardH - 1), float(kCardW - 4), 1, 2);
    b.set(1, 1, 2);
    b.set(kCardW - 2, 1, 2);
    b.set(1, kCardH - 2, 2);
    b.set(kCardW - 2, kCardH - 2, 2);
    heart(b, kCardW / 2, 30, 6, 3);
    gs::TextStyle st;
    st.color = 3;
    st.scale = 1;
    gs::Bitmap label = gs::textBitmap(rankName(rank), st);
    int lx = rank == 10 ? 3 : 5;
    b.blit(label, lx, 3);
    b.blit(label, kCardW - 4 - label.w, kCardH - 4 - label.h);
}

void pal(gs::VDP& vdp, int p, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
    vdp.setColor(p * 16 + 0, 0);
    vdp.setColor(p * 16 + 1, a);
    vdp.setColor(p * 16 + 2, b);
    vdp.setColor(p * 16 + 3, c);
    vdp.setColor(p * 16 + 4, d);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    pal(vdp, PAL_FELT, gs::rgb4(1, 6, 3), gs::rgb4(0, 4, 2), gs::rgb4(2, 8, 4), gs::rgb4(8, 12, 6));
    pal(vdp, PAL_CARD, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3), gs::rgb4(13, 2, 3), gs::rgb4(8, 1, 2));
    pal(vdp, PAL_INK, gs::rgb4(15, 14, 10), gs::rgb4(2, 1, 0), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0));
    pal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(8, 5, 1), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0));
    pal(vdp, PAL_TITLE, gs::rgb4(15, 14, 8), gs::rgb4(6, 2, 2), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0));
    pal(vdp, PAL_WIN, gs::rgb4(15, 15, 12), gs::rgb4(2, 6, 2), gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0));
    pal(vdp, PAL_DIM, gs::rgb4(6, 7, 6), gs::rgb4(2, 3, 2), gs::rgb4(4, 2, 2), gs::rgb4(2, 1, 1));

    for (int r = 1; r <= kCards; r++) {
        gs::Bitmap b(kCardW, kCardH);
        paintCard(b, r);
        art.card[r - 1] = gs::uploadImage(vdp, b);
    }

    gs::Bitmap cur(kCardW + 6, kCardH + 6);
    cur.rect(0, 0, float(kCardW + 6), 2, 1);
    cur.rect(0, float(kCardH + 4), float(kCardW + 6), 2, 1);
    cur.rect(0, 0, 2, float(kCardH + 6), 1);
    cur.rect(float(kCardW + 4), 0, 2, float(kCardH + 6), 1);
    art.cursor = gs::uploadImage(vdp, cur);

    gs::TextStyle st;
    st.color = 1;
    for (int c = 32; c < 127; c++) {
        char s[2] = {char(c), 0};
        art.glyph[c - 32] = gs::uploadImage(vdp, gs::textBitmap(s, st));
    }

    gs::Bitmap h(14, 14);
    heart(h, 7, 5, 4, 1);
    art.heart = gs::uploadImage(vdp, h);
}

}  // namespace solitaire
