#include "game/art.h"

#include <initializer_list>

namespace dbann {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const gs::Bitmap& b) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8 && y < b.h; y++)
        for (int x = 0; x < 8 && x < b.w; x++) px[y * 8 + x] = uint8_t(b.get(x, y) & 15);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64];
        for (int i = 0; i < 64; i++) px[i] = 2;
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                int sy = y + 1;
                int sx = x + 2;
                if (sy < 8 && sx < 8) px[sy * 8 + sx] = 4;
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

gs::Bitmap ballastTile(int n) {
    gs::Bitmap b(8, 8);
    int base = n == 2 ? 4 : 2;
    b.rect(0, 0, 8, 8, base);
    b.set(1 + n, 2, 3);
    b.set(5, 6 - n, 3);
    b.set(3, 4, n == 1 ? 8 : 7);
    b.set(6, 1, 4);
    if (n == 0) b.set(2, 6, 8);
    return b;
}

gs::Bitmap railTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(1, 1, 2, 5, 1);
    b.rect(5, 1, 2, 5, 1);
    b.set(0, 3, 7);
    b.set(7, 2, 8);
    b.rect(0, 6, 8, 1, 5);
    b.rect(0, 7, 8, 1, 6);
    return b;
}

gs::Bitmap apronTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 9);
    b.rect(0, 7, 8, 1, 10);
    b.set(2, 2, 10);
    b.set(6, 4, 10);
    return b;
}

gs::Bitmap hazardTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 11);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
            if (((x + y) & 3) == 0) b.set(x, y, 12);
    return b;
}

gs::Bitmap brickTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 0, 8, 1, 1);
    b.rect(0, 4, 8, 1, 1);
    b.rect(0, 1, 1, 3, 1);
    b.rect(4, 5, 1, 3, 1);
    b.set(2, 2, 3);
    b.set(6, 6, 3);
    return b;
}

gs::Bitmap windowTile() {
    gs::Bitmap b = brickTile();
    b.rect(2, 2, 4, 4, 5);
    b.rect(2, 2, 4, 1, 4);
    b.rect(2, 2, 1, 4, 4);
    b.set(3, 3, 4);
    return b;
}

gs::Bitmap doorTile() {
    gs::Bitmap b = brickTile();
    b.rect(2, 1, 4, 7, 11);
    b.rect(2, 1, 4, 1, 12);
    b.set(5, 4, 4);
    return b;
}

gs::Bitmap roofTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 6);
    b.rect(0, 6, 8, 2, 7);
    b.set(1, 2, 7);
    b.set(5, 3, 7);
    return b;
}

gs::Bitmap shedTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 8);
    for (int x = 0; x < 8; x += 2) b.rect(x, 0, 1, 8, 9);
    b.rect(0, 7, 8, 1, 1);
    return b;
}

gs::Bitmap shedWinTile() {
    gs::Bitmap b = shedTile();
    b.rect(2, 2, 4, 3, 4);
    b.set(3, 3, 5);
    return b;
}

gs::Bitmap starTile() {
    gs::Bitmap b(8, 8);
    b.set(3, 2, 3);
    b.set(4, 5, 1);
    return b;
}

gs::Bitmap hillTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 4, 8, 4, 4);
    b.rect(0, 5, 8, 1, 2);
    return b;
}

gs::Bitmap barTile() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 7, 8, 1, 4);
    return b;
}

gs::Bitmap locoArt() {
    gs::Bitmap b(52, 28);
    b.rect(6, 14, 36, 7, 3);
    b.rect(6, 14, 36, 2, 4);
    b.rect(6, 19, 36, 2, 2);
    b.rect(8, 5, 14, 10, 3);
    b.rect(8, 4, 14, 2, 1);
    b.rect(10, 7, 8, 5, 5);
    b.rect(11, 8, 3, 2, 6);
    b.rect(22, 9, 18, 6, 3);
    b.rect(22, 9, 18, 1, 4);
    b.rect(34, 11, 5, 3, 1);
    b.rect(39, 11, 3, 3, 11);
    b.rect(6, 12, 2, 2, 7);
    b.rect(44, 16, 5, 2, 10);
    b.rect(47, 15, 2, 1, 1);
    b.ellipse(14, 22, 4.2f, 4.2f, 8);
    b.ellipse(26, 22, 4.2f, 4.2f, 8);
    b.ellipse(38, 22, 4.2f, 4.2f, 8);
    b.ellipse(14, 22, 1.5f, 1.5f, 9);
    b.ellipse(26, 22, 1.5f, 1.5f, 9);
    b.ellipse(38, 22, 1.5f, 1.5f, 9);
    b.rect(10, 22, 8, 1, 10);
    b.rect(22, 22, 8, 1, 10);
    b.rect(34, 22, 8, 1, 10);
    b.rect(6, 20, 38, 1, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap carArt() {
    gs::Bitmap b(40, 30);
    b.rect(3, 6, 34, 14, 2);
    b.rect(3, 6, 34, 2, 3);
    b.rect(3, 4, 34, 3, 5);
    b.rect(14, 9, 12, 10, 4);
    b.rect(14, 9, 12, 1, 1);
    b.rect(4, 16, 32, 3, 7);
    b.rect(6, 8, 2, 10, 8);
    b.rect(32, 8, 2, 10, 8);
    b.ellipse(12, 23, 3.4f, 3.4f, 6);
    b.ellipse(28, 23, 3.4f, 3.4f, 6);
    b.ellipse(12, 23, 1.2f, 1.2f, 8);
    b.ellipse(28, 23, 1.2f, 1.2f, 8);
    b.rect(1, 12, 3, 3, 1);
    b.rect(36, 12, 3, 3, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap clothArt(int wave) {
    gs::Bitmap b(22, 20);
    int dy = wave ? 1 : 0;
    b.rect(2, 2 + dy, 16, 12, 3);
    b.rect(2, 2 + dy, 16, 2, 4);
    b.rect(2, 7 + dy, 16, 2, 5);
    b.line(5, 5 + dy, 10, 10 + dy, 6, 1.4f);
    b.line(10, 10 + dy, 15, 5 + dy, 6, 1.4f);
    b.rect(2, 13 + dy, 16, 2, 9);
    for (int x = 3; x < 17; x += 3) b.rect(x, 15 + dy, 1, 2, 6);
    b.rect(17, 3, 2, 14, 2);
    b.outline(7, false);
    return b;
}

gs::Bitmap mastArt() {
    gs::Bitmap b(14, 64);
    b.rect(6, 4, 3, 56, 2);
    b.rect(6, 4, 1, 56, 1);
    b.ellipse(7.5f, 4, 3.2f, 3.2f, 5);
    b.ellipse(7.5f, 4, 1.3f, 1.3f, 6);
    b.rect(9, 8, 1, 18, 8);
    b.outline(1, false);
    return b;
}

gs::Bitmap flatArt() {
    gs::Bitmap b(44, 16);
    b.rect(2, 4, 40, 5, 2);
    b.rect(2, 4, 40, 1, 3);
    b.rect(4, 9, 4, 3, 4);
    b.rect(36, 9, 4, 3, 4);
    b.ellipse(10, 13, 2.4f, 2.4f, 1);
    b.ellipse(34, 13, 2.4f, 2.4f, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 44);
    b.rect(6, 10, 2, 32, 1);
    b.rect(7, 10, 1, 32, 2);
    b.rect(2, 4, 10, 8, 3);
    b.rect(4, 6, 6, 4, 4);
    b.rect(1, 12, 12, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(18, 16);
    b.rect(2, 3, 14, 11, 3);
    b.rect(2, 3, 14, 2, 4);
    b.line(2, 3, 16, 14, 1, 1);
    b.line(16, 3, 2, 14, 1, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(28, 52);
    b.rect(4, 16, 4, 34, 1);
    b.rect(20, 16, 4, 34, 1);
    b.rect(5, 16, 2, 34, 2);
    b.rect(21, 16, 2, 34, 2);
    b.rect(2, 12, 24, 5, 2);
    b.rect(2, 12, 24, 1, 3);
    b.rect(12, 18, 4, 10, 4);
    b.ellipse(14, 8, 5, 4, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(8, 9, 7, 7, 1);
    b.ellipse(11, 8, 5.5f, 5.5f, 0);
    b.ellipse(6, 8, 2, 2, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(10, 7, 8, 4, 4);
    b.ellipse(18, 6, 7, 4, 2);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 4, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

gs::Bitmap plateArt() {
    gs::Bitmap b(180, 22);
    b.rect(0, 0, 180, 22, 4);
    b.rect(2, 2, 176, 18, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(2, 2, 4), 0, gs::rgb4(6, 6, 8), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(4, 2, 2), gs::rgb4(9, 3, 2), gs::rgb4(12, 6, 3), gs::rgb4(15, 12, 5), gs::rgb4(3, 2, 4),
            gs::rgb4(3, 3, 4), gs::rgb4(6, 5, 5), gs::rgb4(5, 6, 6), gs::rgb4(8, 9, 9), gs::rgb4(13, 11, 8),
            gs::rgb4(2, 2, 3), gs::rgb4(14, 10, 4)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(5, 3, 2), gs::rgb4(5, 5, 4), gs::rgb4(8, 7, 6), gs::rgb4(3, 3, 3), gs::rgb4(9, 9, 10),
            gs::rgb4(13, 13, 14), gs::rgb4(3, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5),
            gs::rgb4(13, 11, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BANN,
           {0, gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(12, 1, 2), gs::rgb4(15, 4, 4), gs::rgb4(13, 10, 2),
            gs::rgb4(15, 14, 6), gs::rgb4(6, 1, 2), gs::rgb4(15, 14, 12), gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_LOCO,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(8, 6, 1), gs::rgb4(13, 10, 2), gs::rgb4(15, 13, 6), gs::rgb4(2, 5, 7),
            gs::rgb4(8, 12, 14), gs::rgb4(13, 2, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 13),
            gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_CAR,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(10, 2, 2), gs::rgb4(13, 5, 3), gs::rgb4(7, 2, 2), gs::rgb4(4, 4, 5),
            gs::rgb4(2, 2, 2), gs::rgb4(13, 11, 8), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_TEAL,
           {0, gs::rgb4(1, 2, 2), gs::rgb4(2, 7, 8), gs::rgb4(4, 11, 11), gs::rgb4(1, 5, 6), gs::rgb4(3, 4, 5),
            gs::rgb4(2, 2, 2), gs::rgb4(13, 12, 8), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(3, 3, 3), gs::rgb4(7, 7, 7), gs::rgb4(15, 11, 3), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(3, 1, 1), 0, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_TITLE, {0, gs::rgb4(15, 14, 11), gs::rgb4(2, 1, 3), gs::rgb4(6, 2, 2), gs::rgb4(14, 11, 3)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(14, 14, 10), gs::rgb4(8, 8, 10), gs::rgb4(15, 15, 14), gs::rgb4(4, 5, 8)});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12)});
    vdp.setFogColor(gs::rgb4(2, 2, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    for (int i = 0; i < 3; i++) art.ballast[i] = uploadTile(vdp, tiles, ballastTile(i));
    art.rail = uploadTile(vdp, tiles, railTile());
    art.apron = uploadTile(vdp, tiles, apronTile());
    art.hazard = uploadTile(vdp, tiles, hazardTile());
    art.brick = uploadTile(vdp, tiles, brickTile());
    art.window = uploadTile(vdp, tiles, windowTile());
    art.door = uploadTile(vdp, tiles, doorTile());
    art.roof = uploadTile(vdp, tiles, roofTile());
    art.shed = uploadTile(vdp, tiles, shedTile());
    art.shedWin = uploadTile(vdp, tiles, shedWinTile());
    art.star = uploadTile(vdp, tiles, starTile());
    art.hill = uploadTile(vdp, tiles, hillTile());
    art.bar = uploadTile(vdp, tiles, barTile());

    art.loco = gs::uploadMipped(vdp, locoArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.cloth[0] = gs::uploadMipped(vdp, clothArt(0));
    art.cloth[1] = gs::uploadMipped(vdp, clothArt(1));
    art.mast = gs::uploadMipped(vdp, mastArt());
    art.flat = gs::uploadMipped(vdp, flatArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.plate = gs::uploadMipped(vdp, plateArt());
    art.logo = words(vdp, "DEPOT BANN", 3);
    art.tag = words(vdp, "BRING THE BANNER BACK", 1);
    art.hint = words(vdp, "ARROWS RUN THE SIDINGS", 1);
    art.hint2 = words(vdp, "ONLY THE MAST COUNTS", 1);
    art.win = words(vdp, "BANNER HOME", 2);
    art.lose = words(vdp, "STILL OUT", 2);
}

}  // namespace dbann
