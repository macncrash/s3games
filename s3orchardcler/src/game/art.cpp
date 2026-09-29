#include "game/art.h"

namespace orchard {

static void pal(gs::VDP& vdp, int p, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, i < n ? cs[i] : 0);
}

static void fontTiles(gs::VDP& vdp, gs::TileAlloc& alloc, Art& a) {
    for (int c = 0; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c + 32));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x] = 1;
        int t = alloc.alloc(1);
        vdp.loadTile(t, px);
        a.font[c] = t;
    }
}

static gs::Bitmap grassTile(int n) {
    gs::Bitmap b(8, 8);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = ((x + y + n) & 3) == 0 ? 2 : 1;
            if (((x * 3 + y * 5 + n * 7) & 7) == 0) c = 3;
            b.set(x, y, c);
        }
    return b;
}

static gs::Bitmap soilTile() {
    gs::Bitmap b(8, 8);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 4;
            if (((x + y) & 3) == 0) c = 5;
            if (((x * 2 + y) & 7) == 1) c = 2;
            b.set(x, y, c);
        }
    return b;
}

static gs::Image treeImage(gs::VDP& vdp) {
    gs::Bitmap b(32, 40);
    b.rect(14, 26, 5, 13, 1);
    b.rect(15, 28, 2, 8, 2);
    b.ellipse(16, 16, 14, 13, 3);
    b.ellipse(11, 14, 7, 6, 4);
    b.ellipse(20, 12, 6, 5, 5);
    b.ellipse(9, 20, 2.2f, 2.2f, 6);
    b.ellipse(22, 19, 2.2f, 2.2f, 6);
    b.ellipse(16, 9, 1.6f, 1.6f, 6);
    return gs::uploadImage(vdp, b);
}

static gs::Image heroImage(gs::VDP& vdp) {
    gs::Bitmap b(20, 28);
    b.rect(3, 1, 12, 3, 2);
    b.rect(5, 3, 8, 2, 2);
    b.rect(6, 5, 7, 5, 1);
    b.set(8, 7, 5);
    b.set(11, 7, 5);
    b.rect(5, 10, 10, 8, 3);
    b.rect(6, 12, 3, 4, 7);
    b.rect(6, 18, 4, 6, 4);
    b.rect(11, 18, 4, 6, 4);
    b.rect(5, 24, 5, 3, 5);
    b.rect(11, 24, 5, 3, 5);
    b.line(15, 6, 18, 25, 6, 1.2f);
    b.rect(16, 4, 3, 2, 7);
    return gs::uploadImage(vdp, b);
}

static gs::Image pileImage(gs::VDP& vdp, int kind) {
    gs::Bitmap b(18, 14);
    if (kind == 0) {
        b.ellipse(5, 8, 4, 3.2f, 1);
        b.ellipse(11, 9, 4.2f, 3.2f, 1);
        b.ellipse(8, 5, 3.4f, 3, 2);
        b.ellipse(13, 5, 2.6f, 2.4f, 2);
        b.set(8, 3, 3);
    } else if (kind == 1) {
        b.ellipse(9, 8, 7, 4, 3);
        b.ellipse(5, 6, 3, 2, 4);
        b.ellipse(12, 6, 3.5f, 2.2f, 4);
        b.ellipse(8, 9, 3, 2, 8);
    } else if (kind == 2) {
        b.rect(2, 4, 14, 8, 5);
        b.rect(3, 5, 12, 2, 9);
        b.line(3, 7, 15, 7, 6, 1);
        b.line(9, 5, 9, 11, 6, 1);
    } else {
        b.line(4, 12, 4, 3, 8, 1.4f);
        b.line(8, 12, 7, 2, 3, 1.4f);
        b.line(12, 12, 13, 4, 8, 1.4f);
        b.ellipse(6, 4, 2, 1.4f, 3);
        b.ellipse(12, 5, 2.2f, 1.4f, 4);
    }
    return gs::uploadImage(vdp, b);
}

static gs::Image sparkImage(gs::VDP& vdp) {
    gs::Bitmap b(14, 14);
    b.line(1, 7, 12, 7, 1, 1);
    b.line(7, 1, 7, 12, 1, 1);
    b.line(3, 3, 11, 11, 2, 1);
    b.line(11, 3, 3, 11, 2, 1);
    return gs::uploadImage(vdp, b);
}

static gs::Image label(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.shadow = 3;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 14, 10), gs::rgb4(4, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(14, 3, 2),
                            gs::rgb4(14, 11, 3)};
    const uint16_t field[] = {0, gs::rgb4(1, 6, 2), gs::rgb4(2, 9, 3), gs::rgb4(4, 12, 4), gs::rgb4(6, 4, 2),
                              gs::rgb4(8, 6, 3)};
    const uint16_t tree[] = {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(1, 6, 1), gs::rgb4(2, 9, 2),
                             gs::rgb4(5, 12, 3), gs::rgb4(12, 2, 2)};
    const uint16_t clutter[] = {0, gs::rgb4(12, 2, 1), gs::rgb4(15, 6, 3), gs::rgb4(2, 8, 2), gs::rgb4(12, 10, 2),
                                gs::rgb4(8, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(11, 12, 13), gs::rgb4(1, 7, 2),
                                gs::rgb4(11, 7, 3)};
    const uint16_t hero[] = {0, gs::rgb4(13, 8, 5), gs::rgb4(9, 3, 1), gs::rgb4(2, 5, 12), gs::rgb4(3, 3, 6),
                             gs::rgb4(2, 2, 2), gs::rgb4(8, 5, 2), gs::rgb4(12, 13, 14)};
    const uint16_t banner[] = {0, gs::rgb4(15, 14, 8), gs::rgb4(3, 2, 1), gs::rgb4(6, 3, 1), gs::rgb4(14, 4, 2)};
    pal(vdp, PAL_HUD, hud, 6);
    pal(vdp, PAL_FIELD, field, 6);
    pal(vdp, PAL_TREE, tree, 7);
    pal(vdp, PAL_CLUTTER, clutter, 10);
    pal(vdp, PAL_HERO, hero, 8);
    pal(vdp, PAL_BANNER, banner, 5);
    vdp.setFogColor(gs::rgb4(6, 8, 4));

    gs::TileAlloc alloc(vdp);
    fontTiles(vdp, alloc, art);
    for (int i = 0; i < 4; i++) {
        gs::Bitmap g = grassTile(i);
        art.grass[i] = alloc.shared(g.px.data());
    }
    gs::Bitmap soil = soilTile();
    art.soil = alloc.shared(soil.px.data());

    art.tree = treeImage(vdp);
    art.hero = heroImage(vdp);
    for (int k = 0; k < 4; k++) art.pile[k] = pileImage(vdp, k);
    art.spark = sparkImage(vdp);
    art.title = label(vdp, "ORCHARD", 3);
    art.sub = label(vdp, "CLEAR THE GROUND", 1);
    art.win = label(vdp, "GROUND CLEAR", 2);
    art.lose = label(vdp, "CLOCK DIED", 2);
}

}  // namespace orchard
