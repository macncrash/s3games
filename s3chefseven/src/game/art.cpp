#include "game/art.h"

namespace chefseven {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Mipped say(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 15;
    st.shadow = 0;
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    (void)vdp;
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        a.font[c - 32] = tiles.shared(px);
    }
}

gs::Bitmap chefArt(bool reach) {
    gs::Bitmap b(40, 48);
    b.ellipse(20, 10, 8, 8, 2);
    b.rect(12, 4, 16, 5, 4);
    b.rect(14, 2, 12, 4, 4);
    b.rect(14, 18, 12, 16, 4);
    b.rect(12, 20, 16, 10, 6);
    b.rect(16, 34, 4, 10, 7);
    b.rect(22, 34, 4, 10, 7);
    if (reach) {
        b.rect(4, 22, 10, 4, 2);
        b.rect(26, 18, 10, 4, 2);
    } else {
        b.rect(6, 24, 6, 10, 2);
        b.rect(28, 24, 6, 10, 2);
    }
    b.set(16, 10, 1);
    b.set(24, 10, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(32, 40);
    b.ellipse(16, 9, 7, 7, 2);
    b.rect(10, 3, 12, 4, 3);
    b.rect(11, 16, 10, 12, 3);
    b.rect(10, 18, 12, 8, 5);
    b.rect(12, 28, 3, 8, 7);
    b.rect(18, 28, 3, 8, 7);
    b.rect(4, 20, 6, 8, 2);
    b.set(13, 9, 1);
    b.set(19, 9, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap dishArt(int kind) {
    gs::Bitmap b(40, 28);
    b.ellipse(20, 22, 16, 5, 12);
    b.ellipse(20, 21, 12, 3, 2);
    if (kind == 0) {
        b.ellipse(20, 14, 12, 5, 3);
        b.ellipse(20, 11, 11, 3, 7);
        b.ellipse(20, 8, 12, 4, 3);
    } else if (kind == 1) {
        b.ellipse(20, 14, 13, 6, 10);
        b.ellipse(28, 12, 2, 2, 1);
        b.poly({{8, 14}, {16, 10}, {16, 18}}, 10);
    } else if (kind == 2) {
        b.ellipse(20, 13, 13, 7, 4);
        b.rect(10, 12, 16, 2, 14);
    } else {
        b.ellipse(20, 14, 12, 6, 8);
        b.ellipse(16, 12, 5, 3, 13);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap burntArt() {
    gs::Bitmap b(40, 28);
    b.ellipse(20, 22, 16, 5, 12);
    b.ellipse(20, 14, 13, 7, 14);
    b.ellipse(14, 12, 3, 2, 1);
    b.ellipse(24, 16, 4, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap panArt() {
    gs::Bitmap b(48, 16);
    b.rect(4, 4, 40, 8, 3);
    b.rect(6, 5, 36, 4, 2);
    b.rect(0, 6, 6, 3, 4);
    b.rect(42, 6, 6, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(28, 16);
    int bob = frame;
    b.poly({{14, float(2 + bob)}, {6, 14}, {22, 14}}, 3);
    b.poly({{14, float(5 + bob)}, {9, 14}, {19, 14}}, 2);
    b.poly({{14, float(8 + (bob % 2))}, {11, 14}, {17, 14}}, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(12, 14);
    b.ellipse(6, 8, 5, 5, 1);
    b.rect(5, 1, 2, 4, 1);
    b.ellipse(6, 12, 2, 1, 1);
    return b;
}

void stampKitchen(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t wall[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) wall[y * 8 + x] = ((x + y) & 3) == 0 ? 3 : 2;
    int t = tiles.shared(wall);
    vdp.A.resize(64, 32);
    vdp.A.clear();
    for (int y = 0; y < 28; y++)
        for (int x = 0; x < 40; x++) vdp.A.set(x, y, gs::entry(t, PAL_TRACK));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[16] = {0, gs::rgb4(15, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1)};
    const uint16_t gold[16] = {0, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)};
    const uint16_t red[16] = {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 0, 0)};
    const uint16_t green[16] = {0, gs::rgb4(5, 14, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)};
    const uint16_t chef[16] = {0,
                               gs::rgb4(2, 1, 1),
                               gs::rgb4(15, 12, 9),
                               gs::rgb4(12, 8, 6),
                               gs::rgb4(15, 15, 15),
                               gs::rgb4(12, 13, 14),
                               gs::rgb4(13, 2, 2),
                               gs::rgb4(2, 1, 1),
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               gs::rgb4(1, 1, 1)};
    const uint16_t food[16] = {0,
                               gs::rgb4(2, 1, 1),
                               gs::rgb4(15, 14, 11),
                               gs::rgb4(13, 8, 3),
                               gs::rgb4(9, 5, 2),
                               gs::rgb4(5, 2, 1),
                               gs::rgb4(13, 2, 1),
                               gs::rgb4(3, 11, 2),
                               gs::rgb4(15, 12, 2),
                               gs::rgb4(15, 7, 1),
                               gs::rgb4(15, 9, 8),
                               gs::rgb4(15, 15, 14),
                               gs::rgb4(10, 11, 12),
                               gs::rgb4(14, 10, 3),
                               gs::rgb4(3, 2, 1),
                               gs::rgb4(1, 0, 0)};
    const uint16_t steel[16] = {0, gs::rgb4(1, 1, 1), gs::rgb4(14, 15, 15), gs::rgb4(9, 10, 11), gs::rgb4(5, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t fire[16] = {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 2), gs::rgb4(15, 7, 1), gs::rgb4(13, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t paper[16] = {0, gs::rgb4(15, 14, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(6, 4, 3)};
    const uint16_t rival[16] = {0,
                                gs::rgb4(2, 1, 1),
                                gs::rgb4(14, 11, 8),
                                gs::rgb4(4, 6, 12),
                                gs::rgb4(15, 15, 15),
                                gs::rgb4(8, 10, 14),
                                gs::rgb4(2, 3, 6),
                                gs::rgb4(2, 1, 1),
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                0,
                                gs::rgb4(1, 1, 1)};
    const uint16_t track[16] = {0, gs::rgb4(3, 2, 2), gs::rgb4(12, 8, 6), gs::rgb4(8, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 0, 0)};
    const uint16_t zone[16] = {0, gs::rgb4(10, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t raw[16] = {0, gs::rgb4(3, 8, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t ok[16] = {0, gs::rgb4(15, 12, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t hot[16] = {0, gs::rgb4(15, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const uint16_t decor[16] = {0, gs::rgb4(12, 1, 1), gs::rgb4(15, 15, 15), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_RED, red);
    setPal(vdp, PAL_GREEN, green);
    setPal(vdp, PAL_CHEF, chef);
    setPal(vdp, PAL_FOOD, food);
    setPal(vdp, PAL_STEEL, steel);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_PAPER, paper);
    setPal(vdp, PAL_RIVAL, rival);
    setPal(vdp, PAL_TRACK, track);
    setPal(vdp, PAL_ZONE, zone);
    setPal(vdp, PAL_RAW, raw);
    setPal(vdp, PAL_OK, ok);
    setPal(vdp, PAL_HOT, hot);
    setPal(vdp, PAL_DECOR, decor);

    gs::TileAlloc tiles(vdp);
    stampKitchen(vdp, tiles);
    loadFont(vdp, tiles, art);

    art.title = say(vdp, "S3 CHEF SEVEN", 3);
    art.sub = say(vdp, "FIRST TO SEVEN", 2);
    art.win = say(vdp, "SEVEN", 3);
    art.lose = say(vdp, "SHORT", 3);
    art.chef[0] = gs::uploadMipped(vdp, chefArt(false));
    art.chef[1] = gs::uploadMipped(vdp, chefArt(true));
    art.rival = gs::uploadMipped(vdp, rivalArt());
    for (int i = 0; i < 4; i++) art.dish[i] = gs::uploadMipped(vdp, dishArt(i));
    art.burnt = gs::uploadMipped(vdp, burntArt());
    art.pan = gs::uploadMipped(vdp, panArt());
    for (int i = 0; i < 3; i++) art.flame[i] = gs::uploadMipped(vdp, flameArt(i));
    art.bell = gs::uploadMipped(vdp, bellArt());
    gs::Bitmap solid(4, 4);
    solid.rect(0, 0, 4, 4, 1);
    art.solid = gs::uploadMipped(vdp, solid);
    gs::Bitmap shade(28, 10);
    shade.ellipse(14, 5, 12, 3, 1);
    art.shade = gs::uploadMipped(vdp, shade);

    vdp.A.enabled = true;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace chefseven
