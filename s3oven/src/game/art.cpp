#include "game/art.h"

#include <cmath>
#include <cstdint>

namespace oven {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? c[i] : 0);
}

void paintLoaf(gs::Bitmap& b, int left, int right, int scores, bool burnt) {
    const float cx = b.w * 0.5f;
    const float cy = b.h * 0.50f;
    const float rx = b.w * 0.42f;
    const float ry = std::max(2.f, b.h * 0.30f);
    b.ellipse(cx, cy + ry * 1.05f, rx * 0.9f, std::max(1.2f, ry * 0.28f), 12);
    for (int y = 0; y < b.h; y++) {
        float dy = (y + 0.5f - cy) / ry;
        if (std::fabs(dy) > 1.f) continue;
        float hw = rx * std::sqrt(1.f - dy * dy);
        int x0 = int(std::lround(cx - hw));
        int x1 = int(std::lround(cx + hw));
        for (int x = x0; x < x1; x++) b.set(x, y, x < int(cx) ? left : right);
    }
    if (left != right) b.line(cx, cy - ry * 0.85f, cx, cy + ry * 0.85f, 1, 1.2f);
    if (scores > 0) b.line(cx - rx * 0.5f, cy - ry * 0.05f, cx - rx * 0.08f, cy + ry * 0.45f, 10, 1.2f);
    if (scores > 1) b.line(cx + rx * 0.08f, cy - ry * 0.3f, cx + rx * 0.48f, cy + ry * 0.22f, 10, 1.2f);
    if (burnt) {
        b.ellipse(cx - rx * 0.22f, cy, rx * 0.3f, ry * 0.38f, 8);
        b.ellipse(cx + rx * 0.26f, cy + ry * 0.12f, rx * 0.24f, ry * 0.3f, 8);
        b.ellipse(cx + rx * 0.02f, cy - ry * 0.05f, rx * 0.1f, ry * 0.14f, 9);
    } else if (left >= 5 || right >= 5) {
        b.ellipse(cx - rx * 0.26f, cy - ry * 0.32f, rx * 0.16f, ry * 0.14f, 11);
    }
    if (left == 3 && right == 3) {
        b.set(int(cx) - 4, int(cy), 2);
        b.set(int(cx) + 3, int(cy) + 1, 2);
        b.set(int(cx) - 1, int(cy) + 2, 2);
    }
    b.outline(1, false);
}

gs::Bitmap loafArt(int look) {
    gs::Bitmap b(40, 32);
    switch (look) {
        case LOOK_RAW: paintLoaf(b, 3, 3, 0, false); break;
        case LOOK_PALE: paintLoaf(b, 4, 3, 0, false); break;
        case LOOK_TURN: paintLoaf(b, 5, 4, 2, false); break;
        case LOOK_DANGER: paintLoaf(b, 7, 4, 1, false); break;
        case LOOK_FLIP: paintLoaf(b, 5, 3, 1, false); break;
        case LOOK_BAKE: paintLoaf(b, 6, 4, 1, false); break;
        case LOOK_DRAW: paintLoaf(b, 6, 5, 2, false); break;
        case LOOK_DARK: paintLoaf(b, 6, 7, 1, false); break;
        default: paintLoaf(b, 8, 8, 0, true); break;
    }
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(48, 56);
    b.rect(0, 10, 48, 46, 2);
    for (int y = 16; y < 54; y += 7) b.rect(0, y, 48, 1, 1);
    for (int x = 0; x < 48; x += 12) b.rect(x, 10, 1, 46, 1);
    b.ellipse(24, 34, 16, 15, 7);
    b.ellipse(24, 35, 14, 13, 4);
    b.ellipse(24, 37, 11, 10, 5);
    b.rect(20, 4, 8, 8, 3);
    b.rect(22, 6, 4, 4, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(20, 26);
    float top = 3.f + float(frame);
    b.poly({{10, top}, {3, 16}, {5, 22}, {10, 20}, {15, 22}, {17, 16}}, 1);
    b.poly({{10, top + 6}, {7, 16}, {10, 19}, {13, 16}}, 2);
    b.ellipse(10, 17, 2.2f, 3.0f, 3);
    b.ellipse(10, 18, 1.1f, 1.5f, 4);
    return b;
}

gs::Bitmap peelArt() {
    gs::Bitmap b(46, 16);
    b.ellipse(14, 9, 13, 6, 2);
    b.ellipse(14, 9, 9, 3.4f, 3);
    b.rect(24, 7, 16, 4, 2);
    b.rect(38, 6, 6, 6, 4);
    b.line(26, 8, 36, 8, 3, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(304, 14);
    b.rect(0, 0, 304, 14, 2);
    b.rect(0, 0, 304, 2, 3);
    b.rect(0, 12, 304, 2, 1);
    for (int i = 1; i < 6; i++) b.line(float(i * 48 + 16), 3, float(i * 48 + 16), 11, 1, 1);
    for (int i = 0; i < 16; i++) b.set(14 + i * 18, 6 + (i & 1), 3);
    return b;
}

gs::Bitmap steamArt() {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 12, 3, 2, 1);
    b.ellipse(4, 7, 2, 3, 1);
    b.ellipse(6, 3, 1.3f, 1.8f, 1);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(9, 9);
    b.rect(4, 0, 1, 9, 1);
    b.rect(0, 4, 9, 1, 1);
    b.set(2, 2, 1);
    b.set(6, 2, 1);
    b.set(2, 6, 1);
    b.set(6, 6, 1);
    return b;
}

gs::Bitmap solidArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 1);
    b.ellipse(11, 11, 4.5f, 4.5f, 2);
    return b;
}

void brickPx(uint8_t* px, int variant) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int shift = ((y < 4 ? 0 : 1) + variant) & 1 ? 4 : 0;
            bool mortar = y == 0 || y == 4 || ((x + shift) & 7) == 0;
            bool speck = ((x * 3 + y * 5 + variant * 7) % 11) == 0;
            px[y * 8 + x] = uint8_t(mortar ? 1 : speck ? 3 : 2);
        }
    }
}

void woodPx(uint8_t* px, int variant) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            bool seam = y == 0 || ((x + variant * 3) & 7) == 0;
            px[y * 8 + x] = uint8_t(seam ? 6 : 5);
        }
    }
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(static_cast<char>(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
            }
        }
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x] || y + 1 >= 8) continue;
                int s = (y + 1) * 8 + x + 2;
                if (s < 64 && px[s] == 0) px[s] = 15;
            }
        }
        art.font[c - 32] = tiles.shared(px);
    }
    (void)vdp;
}

void stampRoom(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int brick[4], wood[2];
    for (int i = 0; i < 4; i++) {
        uint8_t px[64];
        brickPx(px, i);
        brick[i] = tiles.shared(px);
    }
    for (int i = 0; i < 2; i++) {
        uint8_t px[64];
        woodPx(px, i);
        wood[i] = tiles.shared(px);
    }
    for (int cy = 6; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) vdp.B.set(cx, cy, gs::entry(brick[(cx + cy) & 3], PAL_BRICK));
    }
    for (int cy = 20; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(wood[cx & 1], PAL_WOOD));
    }
}

gs::Mipped say(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.shadow = 15;
    st.spacing = 1;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink[] = {0, gs::rgb4(15, 14, 12), gs::rgb4(10, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)};
    const uint16_t gold[] = {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 0, 0)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)};
    const uint16_t loaf[] = {0,
                             gs::rgb4(3, 1, 1),
                             gs::rgb4(14, 13, 11),
                             gs::rgb4(13, 10, 6),
                             gs::rgb4(14, 11, 5),
                             gs::rgb4(13, 8, 2),
                             gs::rgb4(11, 6, 1),
                             gs::rgb4(8, 3, 1),
                             gs::rgb4(2, 1, 1),
                             gs::rgb4(12, 3, 1),
                             gs::rgb4(5, 2, 1),
                             gs::rgb4(15, 14, 9),
                             gs::rgb4(5, 3, 2),
                             0,
                             0,
                             gs::rgb4(2, 1, 1)};
    const uint16_t brick[] = {0,
                              gs::rgb4(11, 9, 8),
                              gs::rgb4(9, 4, 3),
                              gs::rgb4(6, 3, 2),
                              gs::rgb4(5, 2, 1),
                              gs::rgb4(3, 2, 1),
                              0,
                              gs::rgb4(13, 8, 5),
                              0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)};
    const uint16_t fire[] = {0, gs::rgb4(10, 2, 1), gs::rgb4(14, 6, 1), gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 10)};
    const uint16_t wood[] = {0,
                             gs::rgb4(4, 2, 1),
                             gs::rgb4(9, 5, 2),
                             gs::rgb4(12, 8, 3),
                             gs::rgb4(6, 3, 1),
                             gs::rgb4(8, 4, 2),
                             gs::rgb4(5, 3, 1)};
    const uint16_t track[] = {0, gs::rgb4(4, 3, 2)};
    const uint16_t mark[] = {0, gs::rgb4(15, 15, 13)};
    const uint16_t ash[] = {0, gs::rgb4(9, 8, 7)};
    const uint16_t flour[] = {0, gs::rgb4(14, 13, 12)};
    const uint16_t soot[] = {0, gs::rgb4(2, 1, 1)};
    setPal(vdp, PAL_INK, ink, 16);
    setPal(vdp, PAL_GOLD, gold, 16);
    setPal(vdp, PAL_ALERT, alert, 16);
    setPal(vdp, PAL_OK, ok, 16);
    setPal(vdp, PAL_LOAF, loaf, 16);
    setPal(vdp, PAL_BRICK, brick, 16);
    setPal(vdp, PAL_FIRE, fire, 5);
    setPal(vdp, PAL_WOOD, wood, 7);
    setPal(vdp, PAL_TRACK, track, 2);
    setPal(vdp, PAL_MARK, mark, 2);
    setPal(vdp, PAL_ASH, ash, 2);
    setPal(vdp, PAL_FLOUR, flour, 2);
    setPal(vdp, PAL_SOOT, soot, 2);
    setPal(vdp, PAL_SELECT, gold, 3);
    vdp.setFogColor(gs::rgb4(3, 2, 1));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    stampRoom(vdp, tiles);

    for (int i = 0; i < LOOK_COUNT; i++) art.loaf[i] = gs::uploadMipped(vdp, loafArt(i));
    {
        gs::Bitmap c(22, 16);
        paintLoaf(c, 6, 5, 2, false);
        art.crumb = gs::uploadMipped(vdp, c);
    }
    art.arch = gs::uploadMipped(vdp, archArt());
    for (int i = 0; i < 3; i++) art.flame[i] = gs::uploadMipped(vdp, flameArt(i));
    art.peel = gs::uploadMipped(vdp, peelArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.steam = gs::uploadMipped(vdp, steamArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.solid = gs::uploadMipped(vdp, solidArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.title = say(vdp, "OVEN", 4);
    art.sub = say(vdp, "SIX LOAVES", 2);
    art.winWord = say(vdp, "THE MORNING HOLDS", 2);
    art.failWord = say(vdp, "BURNED", 3);
    (void)art.crumb;
}

}  // namespace oven
