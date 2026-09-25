#include "game/art.h"

#include <cstdint>
#include <cstdio>
#include <initializer_list>

#include "console/gfx.h"

namespace mosaic {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void joints(Bitmap& b) {
    for (int y = 3; y < b.h; y += 4)
        for (int x = 3; x < b.w; x += 4) b.set(x, y, 2);
}

uint32_t cellHash(const Bitmap& b, int cell) {
    int c = cell % COLS, r = cell / COLS;
    uint32_t h = 2166136261u;
    for (int y = 0; y < TILE; y++)
        for (int x = 0; x < TILE; x++) {
            h ^= uint32_t(b.get(c * TILE + x, r * TILE + y));
            h *= 16777619u;
        }
    return h;
}

// Two cells of flat sky would make the puzzle ambiguous. A clash gets a tessera.
int ensureUnique(Bitmap& b) {
    int patches = 0;
    for (int guard = 0; guard < 6; guard++) {
        uint32_t h[CELLS];
        bool clash = false;
        for (int i = 0; i < CELLS; i++) h[i] = cellHash(b, i);
        for (int i = 0; i < CELLS; i++) {
            bool dup = false;
            for (int j = 0; j < i; j++)
                if (h[i] == h[j]) dup = true;
            if (!dup) continue;
            clash = true;
            patches++;
            int c = i % COLS, r = i / COLS;
            int x = c * TILE + 10 + guard * 3;
            int y = r * TILE + 10 + (i % 4) * 3;
            int col = 3 + (i + guard * 3) % 13;
            for (int yy = 0; yy < 6; yy++)
                for (int xx = 0; xx < 6; xx++) b.set(x + xx, y + yy, col);
        }
        if (!clash) return patches;
    }
    return patches;
}

void bevel(Bitmap& t) {
    for (int x = 0; x < t.w; x++) {
        t.set(x, 0, 1);
        t.set(x, t.h - 1, 2);
    }
    for (int y = 0; y < t.h; y++) {
        t.set(0, y, 1);
        t.set(t.w - 1, y, 2);
    }
    t.set(0, t.h - 1, 2);
    t.set(t.w - 1, 0, 2);
}

void slice(gs::VDP& vdp, Art& art, int pic, const Bitmap& src) {
    for (int i = 0; i < CELLS; i++) {
        int c = i % COLS, r = i / COLS;
        Bitmap tile(TILE, TILE);
        for (int y = 0; y < TILE; y++)
            for (int x = 0; x < TILE; x++) {
                int p = src.get(c * TILE + x, r * TILE + y);
                tile.set(x, y, p ? p : 3);
            }
        bevel(tile);
        art.tile[pic][i] = gs::uploadImage(vdp, tile);
    }
    Bitmap small = src.resample(THUMB, THUMB);
    Bitmap box(THUMB + 8, THUMB + 8);
    box.rect(0, 0, box.w, box.h, 2);
    box.rect(2, 2, box.w - 4, box.h - 4, 1);
    box.blit(small, 4, 4);
    art.thumb[pic] = gs::uploadImage(vdp, box);
}

enum Coast {
    C_SKY = 3,
    C_SKY2,
    C_HAZE,
    C_SUN,
    C_CORE,
    C_CLOUD,
    C_SEA,
    C_DEEP,
    C_FOAM,
    C_SAND,
    C_ROCK,
    C_WHITE,
    C_RED
};

Bitmap drawCoast() {
    Bitmap b(PIC, PIC);
    b.rect(0, 0, PIC, 86, C_SKY);
    b.rect(0, 28, PIC, 28, C_SKY2);
    b.rect(0, 56, PIC, 30, C_HAZE);

    b.ellipse(92, 22, 16, 16, C_SUN);
    b.ellipse(92, 22, 8, 8, C_CORE);
    b.ellipse(22, 18, 16, 9, C_CLOUD);
    b.ellipse(40, 16, 12, 7, C_CLOUD);
    b.ellipse(128, 14, 12, 7, C_CLOUD);
    b.poly({{108, 78}, {126, 48}, {144, 70}, {144, 92}, {108, 92}}, C_ROCK);

    b.rect(0, 86, PIC, 58, C_SEA);
    b.rect(0, 112, PIC, 32, C_DEEP);
    b.rect(0, 86, PIC, 5, C_FOAM);
    b.rect(8, 108, 120, 4, C_FOAM);
    b.rect(24, 128, 80, 4, C_FOAM);

    // Beam under the lantern, then the tower covers its root.
    b.poly({{40, 48}, {86, 40}, {90, 56}, {40, 58}}, C_CORE);
    b.poly({{4, 44}, {20, 22}, {36, 44}}, C_RED);
    b.rect(8, 40, 24, 10, C_RED);
    b.rect(14, 42, 10, 8, C_CORE);
    b.rect(10, 50, 20, 48, C_WHITE);
    b.rect(10, 62, 20, 8, C_RED);
    b.rect(16, 74, 8, 10, C_SKY2);
    b.rect(16, 90, 8, 8, C_SKY);
    b.rect(4, 94, 32, 8, C_ROCK);

    b.rect(78, 58, 5, 28, C_ROCK);
    b.poly({{83, 82}, {83, 56}, {112, 82}}, C_CLOUD);
    b.poly({{78, 82}, {78, 66}, {62, 82}}, C_WHITE);
    b.rect(60, 82, 56, 10, C_RED);
    b.poly({{60, 90}, {54, 98}, {118, 98}, {122, 90}}, C_ROCK);

    b.ellipse(52, 100, 7, 7, C_RED);
    b.rect(49, 106, 6, 12, C_WHITE);
    b.rect(48, 116, 8, 4, C_RED);

    b.rect(116, 90, 28, 6, C_ROCK);
    b.rect(120, 96, 5, 22, C_ROCK);
    b.rect(134, 96, 5, 26, C_ROCK);
    b.rect(122, 78, 14, 12, C_WHITE);
    b.rect(125, 81, 8, 6, C_SUN);

    b.poly({{0, 118}, {46, 112}, {34, 144}, {0, 144}}, C_SAND);
    b.ellipse(16, 128, 12, 7, C_ROCK);
    b.ellipse(32, 136, 8, 5, C_ROCK);

    // Gulls, chunky enough to survive the grout dots.
    b.poly({{46, 36}, {58, 44}, {58, 40}}, C_WHITE);
    b.poly({{58, 40}, {72, 34}, {68, 44}}, C_WHITE);
    b.poly({{100, 34}, {112, 42}, {112, 38}}, C_WHITE);
    b.poly({{112, 38}, {126, 32}, {122, 42}}, C_WHITE);

    // Crab on the wet sand, so the bottom-middle tile is not bare water.
    b.ellipse(70, 132, 7, 4, C_RED);
    b.rect(60, 130, 6, 3, C_RED);
    b.rect(74, 130, 8, 3, C_RED);
    b.rect(66, 136, 3, 5, C_RED);
    b.rect(72, 136, 3, 5, C_RED);

    b.rect(96, 124, 16, 6, C_RED);
    return b;
}

enum Garden {
    G_SKY = 3,
    G_HILL,
    G_HILLD,
    G_GRASS,
    G_GRASSD,
    G_LEAF,
    G_LEAFL,
    G_TRUNK,
    G_WALL,
    G_ROOF,
    G_PATH,
    G_PINK,
    G_GOLD
};

Bitmap drawGarden() {
    Bitmap b(PIC, PIC);
    b.rect(0, 0, PIC, 78, G_SKY);
    b.ellipse(24, 20, 12, 12, G_GOLD);
    b.ellipse(24, 20, 6, 6, G_PINK);
    b.ellipse(78, 16, 18, 8, G_WALL);
    b.ellipse(96, 18, 12, 6, G_WALL);

    b.poly({{0, 78}, {0, 58}, {36, 44}, {70, 62}, {110, 40}, {144, 58}, {144, 78}}, G_HILL);
    b.poly({{0, 86}, {0, 70}, {48, 58}, {96, 74}, {144, 60}, {144, 86}}, G_HILLD);

    b.rect(0, 86, PIC, 58, G_GRASS);
    b.rect(0, 118, PIC, 26, G_GRASSD);

    b.ellipse(34, 62, 28, 22, G_LEAF);
    b.ellipse(18, 58, 14, 12, G_LEAFL);
    b.ellipse(48, 54, 16, 14, G_LEAFL);
    b.ellipse(34, 48, 10, 8, G_LEAFL);
    b.rect(28, 78, 12, 40, G_TRUNK);

    b.poly({{76, 78}, {108, 42}, {140, 78}}, G_ROOF);
    b.rect(84, 74, 48, 42, G_WALL);
    b.rect(116, 48, 10, 22, G_TRUNK);
    b.rect(114, 44, 14, 6, G_ROOF);
    b.rect(100, 90, 16, 26, G_TRUNK);
    b.rect(106, 102, 4, 4, G_GOLD);
    b.rect(90, 82, 16, 14, G_SKY);
    b.rect(96, 82, 3, 14, G_TRUNK);
    b.rect(90, 88, 16, 3, G_TRUNK);

    b.poly({{70, 144}, {96, 116}, {120, 116}, {108, 144}}, G_PATH);

    for (int x = 4; x <= 64; x += 16) {
        b.rect(x, 100, 4, 16, G_TRUNK);
        b.rect(x, 104, 14, 3, G_TRUNK);
    }

    b.ellipse(12, 128, 7, 7, G_PINK);
    b.ellipse(12, 128, 3, 3, G_GOLD);
    b.ellipse(48, 124, 6, 6, G_GOLD);
    b.ellipse(60, 134, 7, 6, G_PINK);
    b.ellipse(132, 126, 8, 7, G_PINK);
    b.ellipse(132, 126, 3, 3, G_GOLD);
    b.ellipse(118, 136, 6, 5, G_GOLD);
    b.rect(8, 134, 3, 8, G_LEAF);
    b.rect(58, 138, 3, 6, G_LEAF);

    b.poly({{64, 30}, {76, 38}, {76, 34}}, G_TRUNK);
    b.poly({{76, 34}, {90, 28}, {86, 38}}, G_TRUNK);
    return b;
}

enum Night {
    N_SKY = 3,
    N_SKY2,
    N_MOON,
    N_BLDG,
    N_BLDG2,
    N_WIN,
    N_WIN2,
    N_STREET,
    N_LAMP,
    N_AWN,
    N_SIGN,
    N_PAPER,
    N_CRATE
};

void window(Bitmap& b, int x, int y, int c) {
    b.rect(x, y, 8, 10, c);
    b.rect(x + 3, y, 2, 10, 2);
    b.rect(x, y + 4, 8, 2, 2);
}

void stars(Bitmap& b) {
    static const int s[][2] = {{8, 8},   {22, 28}, {48, 10}, {60, 24}, {70, 6},  {104, 12},
                               {138, 8}, {18, 44}, {96, 30}, {40, 18}, {128, 40}, {84, 36}};
    for (auto& p : s) b.rect(p[0], p[1], 2, 2, N_MOON);
}

Bitmap drawNight() {
    Bitmap b(PIC, PIC);
    b.rect(0, 0, PIC, 118, N_SKY);
    b.rect(0, 70, PIC, 48, N_SKY2);
    stars(b);
    b.ellipse(118, 26, 14, 14, N_MOON);
    b.ellipse(114, 24, 4, 4, N_SKY);

    b.rect(4, 46, 34, 72, N_BLDG);
    b.rect(42, 32, 34, 86, N_BLDG2);
    b.rect(80, 54, 28, 64, N_BLDG);
    b.rect(112, 40, 28, 78, N_BLDG2);

    window(b, 10, 54, N_WIN);
    window(b, 22, 54, N_WIN2);
    window(b, 10, 72, N_WIN2);
    window(b, 22, 74, N_WIN);
    window(b, 10, 92, N_WIN);

    window(b, 48, 40, N_WIN2);
    window(b, 60, 40, N_WIN);
    window(b, 48, 58, N_WIN);
    window(b, 60, 60, N_WIN2);
    window(b, 48, 78, N_WIN2);
    window(b, 60, 80, N_WIN);
    window(b, 52, 98, N_WIN);

    window(b, 86, 62, N_WIN);
    window(b, 96, 74, N_WIN2);
    window(b, 86, 90, N_WIN2);

    window(b, 118, 48, N_WIN2);
    window(b, 130, 48, N_WIN);
    window(b, 118, 66, N_WIN);
    window(b, 130, 68, N_WIN2);

    b.rect(108, 86, 36, 8, N_AWN);
    b.rect(110, 94, 32, 8, N_PAPER);
    b.ellipse(118, 97, 4, 4, N_LAMP);
    b.ellipse(128, 97, 4, 4, N_SIGN);
    b.ellipse(136, 97, 3, 3, N_WIN);

    b.rect(78, 78, 4, 40, N_CRATE);
    b.ellipse(80, 74, 5, 5, N_LAMP);
    b.rect(36, 70, 4, 48, N_CRATE);
    b.ellipse(38, 66, 5, 5, N_LAMP);

    b.rect(96, 46, 18, 8, N_SIGN);
    b.rect(100, 48, 10, 4, N_PAPER);

    b.rect(0, 118, PIC, 26, N_STREET);
    b.rect(12, 124, 10, 4, N_WIN);
    b.rect(50, 126, 8, 3, N_WIN2);
    b.rect(90, 124, 12, 4, N_LAMP);
    b.rect(122, 128, 10, 3, N_WIN);

    b.rect(8, 108, 16, 14, N_CRATE);
    b.rect(22, 114, 12, 12, N_PAPER);
    b.ellipse(58, 128, 10, 6, N_CRATE);
    b.ellipse(68, 122, 6, 5, N_CRATE);
    b.poly({{64, 118}, {66, 112}, {70, 118}}, N_CRATE);
    b.poly({{70, 118}, {76, 112}, {74, 118}}, N_CRATE);
    b.rect(69, 121, 3, 3, N_WIN);

    b.rect(96, 108, 14, 10, N_CRATE);
    b.rect(112, 112, 12, 10, N_AWN);
    return b;
}

Bitmap makeFrame() {
    const int w = BOARD + FRAME * 2;
    const int h = BOARD + FRAME * 2;
    Bitmap b(w, h);
    b.rect(0, 0, w, h, 3);
    b.rect(1, 1, w - 2, h - 2, 2);
    b.rect(2, 2, w - 4, h - 4, 1);
    b.rect(4, 4, w - 8, h - 8, 3);
    b.rect(FRAME, FRAME, BOARD, BOARD, 6);
    const int nail[4][2] = {{2, 2}, {w - 5, 2}, {2, h - 5}, {w - 5, h - 5}};
    for (auto& n : nail) b.rect(n[0], n[1], 3, 3, 5);
    return b;
}

Bitmap arrowUp() {
    Bitmap b(11, 9);
    b.rect(5, 0, 1, 9, 1);
    b.set(4, 1, 1);
    b.set(6, 1, 1);
    b.rect(3, 2, 5, 1, 1);
    b.rect(2, 3, 7, 1, 1);
    b.rect(1, 4, 9, 1, 1);
    return b;
}

Bitmap arrowDown() {
    Bitmap b(11, 9);
    b.rect(5, 0, 1, 9, 1);
    b.rect(1, 4, 9, 1, 1);
    b.rect(2, 5, 7, 1, 1);
    b.rect(3, 6, 5, 1, 1);
    b.set(4, 7, 1);
    b.set(6, 7, 1);
    return b;
}

Bitmap arrowLeft() {
    Bitmap b(9, 11);
    b.rect(0, 5, 9, 1, 1);
    b.set(1, 4, 1);
    b.set(1, 6, 1);
    b.rect(2, 3, 1, 5, 1);
    b.rect(3, 2, 1, 7, 1);
    b.rect(4, 1, 1, 9, 1);
    return b;
}

Bitmap arrowRight() {
    Bitmap b(9, 11);
    b.rect(0, 5, 9, 1, 1);
    b.rect(4, 1, 1, 9, 1);
    b.rect(5, 2, 1, 7, 1);
    b.rect(6, 3, 1, 5, 1);
    b.set(7, 4, 1);
    b.set(7, 6, 1);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_CREAM, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(7, 15, 8));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 9, 11));
    textPal(vdp, PAL_CORAL, gs::rgb4(15, 7, 4));

    const uint16_t hi = gs::rgb4(15, 14, 12);
    setPal(vdp, PAL_COAST,
           {0, hi, gs::rgb4(2, 2, 4), gs::rgb4(2, 5, 12), gs::rgb4(4, 8, 14), gs::rgb4(9, 12, 15), gs::rgb4(15, 12, 3),
            gs::rgb4(15, 15, 12), gs::rgb4(15, 15, 15), gs::rgb4(2, 9, 12), gs::rgb4(1, 5, 9), gs::rgb4(13, 15, 14),
            gs::rgb4(14, 12, 7), gs::rgb4(6, 5, 4), gs::rgb4(15, 15, 13), gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_GARDEN,
           {0, gs::rgb4(14, 15, 12), gs::rgb4(1, 2, 1), gs::rgb4(6, 11, 15), gs::rgb4(8, 13, 8), gs::rgb4(5, 10, 4),
            gs::rgb4(3, 11, 3), gs::rgb4(2, 8, 2), gs::rgb4(1, 7, 2), gs::rgb4(6, 13, 4), gs::rgb4(7, 5, 2),
            gs::rgb4(15, 13, 10), gs::rgb4(13, 4, 3), gs::rgb4(12, 9, 5), gs::rgb4(15, 6, 8), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(12, 12, 14), gs::rgb4(1, 1, 2), gs::rgb4(1, 1, 4), gs::rgb4(2, 2, 8), gs::rgb4(15, 15, 11),
            gs::rgb4(3, 3, 6), gs::rgb4(6, 5, 8), gs::rgb4(15, 12, 4), gs::rgb4(5, 11, 13), gs::rgb4(2, 2, 3),
            gs::rgb4(15, 10, 3), gs::rgb4(12, 2, 4), gs::rgb4(2, 12, 8), gs::rgb4(15, 14, 11), gs::rgb4(9, 6, 3)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(13, 10, 6), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), 0, gs::rgb4(15, 12, 4), gs::rgb4(2, 2, 4), 0,
            0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, art);

    Bitmap pics[PICTURES] = {drawCoast(), drawGarden(), drawNight()};
    for (int i = 0; i < PICTURES; i++) {
        joints(pics[i]);
        int patches = ensureUnique(pics[i]);
        if (patches) std::fprintf(stderr, "s3mosaic: picture %d needed %d uniqueness patches\n", i + 1, patches);
        slice(vdp, art, i, pics[i]);
    }
    art.frame = gs::uploadImage(vdp, makeFrame());
    art.arrow[0] = gs::uploadImage(vdp, arrowUp());
    art.arrow[1] = gs::uploadImage(vdp, arrowRight());
    art.arrow[2] = gs::uploadImage(vdp, arrowDown());
    art.arrow[3] = gs::uploadImage(vdp, arrowLeft());

    gs::TextStyle ink{1, 1, 0, 15, 1};
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 MOSAIC", {3, 1, 0, 15, 1}));
    art.sub = gs::uploadImage(vdp, gs::textBitmap("SLIDE THE TILES", {2, 1, 0, 15, 1}));
    art.tag = gs::uploadImage(vdp, gs::textBitmap("THE PICTURE HAS TO COMPLETE", ink));
    art.prompt = gs::uploadImage(vdp, gs::textBitmap("PRESS START", {2, 1, 0, 15, 1}));
    art.goal = gs::uploadImage(vdp, gs::textBitmap("GOAL", ink));
}

}  // namespace mosaic
