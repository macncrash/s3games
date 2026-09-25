#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace mower {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const gs::Bitmap& b) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8 && y < b.h; y++)
        for (int x = 0; x < 8 && x < b.w; x++) px[y * 8 + x] = uint8_t(b.get(x, y) & 15);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

gs::Bitmap spin(const gs::Bitmap& src, float ang) {
    gs::Bitmap d(src.w, src.h);
    float cx = (src.w - 1) * 0.5f;
    float cy = (src.h - 1) * 0.5f;
    float c = std::cos(ang);
    float s = std::sin(ang);
    for (int y = 0; y < src.h; y++) {
        for (int x = 0; x < src.w; x++) {
            float dx = float(x) - cx;
            float dy = float(y) - cy;
            int sx = int(std::lround(cx + c * dx + s * dy));
            int sy = int(std::lround(cy - s * dx + c * dy));
            d.set(x, y, src.get(sx, sy));
        }
    }
    return d;
}

gs::Bitmap mowerEast() {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 13.0f, 12.2f, 5);
    b.ellipse(20, 20, 11.2f, 10.4f, 4);
    b.ellipse(13, 9.5f, 3.2f, 3.2f, 1);
    b.ellipse(27, 9.5f, 3.2f, 3.2f, 1);
    b.ellipse(13, 30.5f, 3.2f, 3.2f, 1);
    b.ellipse(27, 30.5f, 3.2f, 3.2f, 1);
    b.ellipse(13, 9.5f, 1.3f, 1.3f, 6);
    b.ellipse(27, 9.5f, 1.3f, 1.3f, 6);
    b.ellipse(13, 30.5f, 1.3f, 1.3f, 6);
    b.ellipse(27, 30.5f, 1.3f, 1.3f, 6);
    b.ellipse(17, 20, 6.0f, 5.2f, 2);
    b.ellipse(17, 20, 4.8f, 4.0f, 3);
    b.rect(14, 18, 5, 3, 12);
    b.ellipse(26.5f, 20, 3.4f, 3.0f, 10);
    b.ellipse(29.2f, 20, 1.15f, 1.15f, 8);
    b.ellipse(14.2f, 18.6f, 2.3f, 1.8f, 13);
    b.ellipse(14.6f, 19.4f, 0.8f, 0.7f, 14);
    b.rect(22, 7, 7, 3, 5);
    b.rect(27, 6, 3, 3, 11);
    b.outline(1, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4.2f, 1);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(44, 32);
    b.rect(8, 14, 28, 16, 2);
    b.rect(8, 14, 28, 2, 3);
    b.poly({{4, 16}, {22, 4}, {40, 16}}, 1);
    b.poly({{8, 15}, {22, 7}, {36, 15}}, 4);
    b.rect(18, 18, 8, 12, 5);
    b.set(23, 24, 6);
    b.rect(11, 18, 5, 5, 7);
    b.rect(28, 18, 5, 5, 7);
    b.line(13, 18, 13, 22, 3, 1);
    b.line(30, 18, 30, 22, 3, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(32, 36);
    b.rect(14, 22, 4, 12, 3);
    b.rect(12, 32, 8, 2, 3);
    b.ellipse(16, 16, 13, 11, 2);
    b.ellipse(11, 14, 7, 6, 1);
    b.ellipse(21, 17, 6, 5, 4);
    b.ellipse(16, 10, 4, 4, 1);
    return b;
}

gs::Bitmap mailArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 8, 2, 9, 3);
    b.rect(2, 4, 10, 6, 1);
    b.rect(2, 4, 10, 2, 2);
    b.rect(11, 1, 2, 4, 4);
    return b;
}

gs::Bitmap gnomeArt() {
    gs::Bitmap b(14, 20);
    b.ellipse(7, 4.5f, 4.2f, 3.2f, 1);
    b.rect(3, 4, 8, 2, 1);
    b.ellipse(7, 8, 2.2f, 2.0f, 3);
    b.rect(4, 10, 6, 6, 2);
    b.rect(5, 12, 2, 3, 5);
    b.rect(3, 16, 3, 3, 4);
    b.rect(8, 16, 3, 3, 4);
    b.set(6, 8, 6);
    b.set(8, 8, 6);
    return b;
}

gs::Bitmap flowerArt(int petal) {
    gs::Bitmap b(12, 14);
    b.line(6, 13, 6, 7, 4, 1);
    b.line(6, 10, 3, 8, 4, 1);
    b.ellipse(6, 5.2f, 2.1f, 2.1f, petal);
    b.ellipse(3.6f, 6.2f, 1.7f, 1.7f, petal);
    b.ellipse(8.4f, 6.2f, 1.7f, 1.7f, petal);
    b.ellipse(6, 3.3f, 1.5f, 1.5f, petal);
    b.ellipse(6, 5.3f, 0.9f, 0.9f, 5);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(16, 9, 12, 5.5f, 2);
    b.ellipse(28, 7, 13, 6, 1);
    b.ellipse(36, 10, 8, 4.5f, 2);
    b.ellipse(24, 10, 9, 3.5f, 1);
    return b;
}

gs::Bitmap rainArt() {
    gs::Bitmap b(3, 8);
    b.line(1, 0, 1, 6, 1, 1);
    b.set(1, 7, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.rect(8, 0, 2, 3, 1);
    b.rect(8, 15, 2, 3, 1);
    b.rect(0, 8, 3, 2, 1);
    b.rect(15, 8, 3, 2, 1);
    b.line(3, 3, 5, 5, 1, 1);
    b.line(14, 3, 12, 5, 1, 1);
    b.line(3, 14, 5, 12, 1, 1);
    b.line(14, 14, 12, 12, 1, 1);
    b.ellipse(9, 9, 5.2f, 5.2f, 1);
    b.ellipse(8, 8, 2.0f, 2.0f, 2);
    return b;
}

gs::Bitmap plaqueArt() {
    gs::Bitmap b(168, 28);
    b.rect(0, 0, 168, 28, 2);
    b.rect(0, 26, 168, 2, 4);
    return b;
}

gs::Bitmap tallGrass(int variant) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    static const int blades[3][6][2] = {
        {{1, 1}, {3, 0}, {5, 2}, {6, 4}, {2, 5}, {4, 6}},
        {{0, 2}, {2, 1}, {4, 3}, {7, 2}, {3, 6}, {6, 5}},
        {{1, 3}, {4, 1}, {6, 0}, {2, 4}, {5, 6}, {7, 5}},
    };
    for (int i = 0; i < 6; i++) {
        int x = blades[variant][i][0];
        int y = blades[variant][i][1];
        b.set(x, y, (i & 1) ? 4 : 3);
        if (y + 1 < 8) b.set(x, y + 1, 5);
    }
    if (variant == 2) b.set(4, 4, 6);
    b.set(0, 7, 1);
    b.set(7, 0, 1);
    return b;
}

gs::Bitmap cutStripe(bool vertical, bool dark) {
    gs::Bitmap b(8, 8);
    int field = dark ? 4 : 1;
    int band = dark ? 5 : 2;
    int edge = dark ? 6 : 3;
    b.rect(0, 0, 8, 8, field);
    if (vertical) {
        b.rect(1, 0, 1, 8, edge);
        b.rect(4, 0, 2, 8, band);
        b.rect(7, 0, 1, 8, edge);
    } else {
        b.rect(0, 1, 8, 1, edge);
        b.rect(0, 4, 8, 2, band);
        b.rect(0, 7, 8, 1, edge);
    }
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 6);
    b.rect(0, 2, 8, 1, 7);
    b.rect(0, 3, 8, 2, 4);
    b.rect(0, 3, 8, 1, 5);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b = railArt();
    b.rect(3, 0, 2, 8, 4);
    b.rect(3, 0, 1, 8, 5);
    return b;
}

gs::Bitmap gravelArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(1, 1, 2);
    b.set(2, 2, 3);
    b.set(5, 1, 2);
    b.set(6, 4, 3);
    b.set(3, 5, 2);
    b.set(7, 6, 2);
    b.set(1, 6, 3);
    b.set(4, 3, 2);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 7, 8, 1, 4);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64];
        for (int i = 0; i < 64; i++) px[i] = 2;
        for (int x = 0; x < 8; x++) px[7 * 8 + x] = 4;
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                int sy = y + 1;
                int sx = x + 2;
                if (sy < 7 && sx < 8) px[sy * 8 + sx] = 15;
            }
        }
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
            }
        }
        for (int x = 0; x < 8; x++) px[7 * 8 + x] = 4;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 3;
    st.shadow = 2;
    st.spacing = 1;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 12), gs::rgb4(1, 3, 2), 0, gs::rgb4(9, 12, 5)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(3, 1, 1), 0, gs::rgb4(10, 4, 3)});
    setPal(vdp, PAL_GRASS,
           {0, gs::rgb4(1, 4, 1), gs::rgb4(2, 7, 2), gs::rgb4(3, 11, 3), gs::rgb4(8, 13, 4), gs::rgb4(1, 5, 2),
            gs::rgb4(14, 13, 3)});
    setPal(vdp, PAL_CUT,
           {0, gs::rgb4(11, 14, 6), gs::rgb4(14, 15, 10), gs::rgb4(6, 11, 4), gs::rgb4(2, 7, 3), gs::rgb4(1, 4, 2),
            gs::rgb4(4, 9, 4)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(9, 8, 5), gs::rgb4(12, 11, 8), gs::rgb4(6, 5, 3), gs::rgb4(9, 6, 3), gs::rgb4(13, 9, 5),
            gs::rgb4(2, 6, 2), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_MOWER,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 1, 1), gs::rgb4(14, 3, 2), gs::rgb4(11, 12, 12), gs::rgb4(5, 6, 6),
            gs::rgb4(15, 13, 3), gs::rgb4(6, 4, 2), gs::rgb4(15, 15, 13), 0, gs::rgb4(4, 4, 5), gs::rgb4(15, 8, 2),
            gs::rgb4(3, 6, 12), gs::rgb4(13, 2, 2), gs::rgb4(14, 11, 8)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 13, 4), gs::rgb4(2, 8, 3), gs::rgb4(6, 4, 2), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_SHED,
           {0, gs::rgb4(8, 1, 1), gs::rgb4(13, 11, 7), gs::rgb4(8, 6, 4), gs::rgb4(12, 3, 2), gs::rgb4(5, 3, 2),
            gs::rgb4(14, 12, 5), gs::rgb4(6, 11, 14)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 15), gs::rgb4(8, 10, 13), gs::rgb4(10, 12, 15)});
    setPal(vdp, PAL_FLOWER,
           {0, gs::rgb4(14, 3, 3), gs::rgb4(15, 13, 3), gs::rgb4(15, 15, 14), gs::rgb4(2, 8, 3), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_GNOME,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(3, 6, 12), gs::rgb4(14, 11, 8), gs::rgb4(2, 2, 2), gs::rgb4(14, 14, 14),
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 14, 6), gs::rgb4(4, 2, 1), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});
    vdp.setFogColor(gs::rgb4(6, 7, 8));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    for (int i = 0; i < 3; i++) art.tall[i] = uploadTile(vdp, tiles, tallGrass(i));
    art.cutH = uploadTile(vdp, tiles, cutStripe(false, false));
    art.cutD = uploadTile(vdp, tiles, cutStripe(false, true));
    art.cutV = uploadTile(vdp, tiles, cutStripe(true, false));
    art.cutVD = uploadTile(vdp, tiles, cutStripe(true, true));
    art.rail = uploadTile(vdp, tiles, railArt());
    art.post = uploadTile(vdp, tiles, postArt());
    art.gravel = uploadTile(vdp, tiles, gravelArt());
    art.bar = uploadTile(vdp, tiles, barArt());

    gs::Bitmap east = mowerEast();
    const float step = 6.2831853f / 16.f;
    for (int i = 0; i < 16; i++) art.mower[i] = gs::uploadMipped(vdp, spin(east, step * float(i)));
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.mail = gs::uploadMipped(vdp, mailArt());
    art.gnome = gs::uploadMipped(vdp, gnomeArt());
    for (int i = 0; i < 3; i++) art.flower[i] = gs::uploadMipped(vdp, flowerArt(i + 1));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.rain = gs::uploadMipped(vdp, rainArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.plaque = gs::uploadMipped(vdp, plaqueArt());
    art.logo = words(vdp, "S3 MOWER", 3);
    art.tag = words(vdp, "CUT THE FIELD BEFORE THE RAIN", 1);
    art.hint = words(vdp, "ARROWS DRIVE", 1);
    art.turn = words(vdp, "LET OFF TO TURN", 1);
    art.win = words(vdp, "FIELD CUT", 3);
    art.lose = words(vdp, "RAINED OUT", 2);
    art.stripe = words(vdp, "STRIPE", 1);
}

}  // namespace mower
