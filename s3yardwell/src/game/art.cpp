#include "game/art.h"

#include <cmath>

namespace yardwell {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs, bool outline = true) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    int n = i;
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    if (outline && n < 16) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Mipped up(gs::VDP& vdp, const gs::Bitmap& b, bool crop = true) {
    return gs::uploadMipped(vdp, crop ? b.cropToContent(1) : b);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    uint8_t blank[64];
    for (int i = 0; i < 64; i++) blank[i] = 2;
    a.panel = tiles.shared(blank);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64];
        for (int i = 0; i < 64; i++) px[i] = 2;
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x] && y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        a.font[c - 32] = tiles.shared(px);
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 0x8da6b343u ^ uint32_t(y) * 0xd8163841u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    return h ^ (h >> 15);
}

void grassPx(uint8_t* px, int variant, bool dark, bool flower) {
    uint8_t base = dark ? 2 : 1;
    for (int i = 0; i < 64; i++) px[i] = base;
    for (int b = 0; b < 3; b++) {
        int x = (variant * 3 + b * 3) & 7;
        int y0 = (b * 2 + variant) & 5;
        px[y0 * 8 + x] = 3;
        if (y0 + 1 < 8) px[(y0 + 1) * 8 + x] = dark ? 3 : 2;
        if (y0 + 2 < 8) px[(y0 + 2) * 8 + x] = base;
    }
    if (flower) {
        int fx = 2 + (variant & 3);
        int fy = 3;
        if (fx + 1 < 8) {
            px[fy * 8 + fx] = 4;
            px[fy * 8 + fx + 1] = 5;
            px[(fy - 1) * 8 + fx] = 4;
            px[(fy + 1) * 8 + fx] = 13;
        }
    }
}

void dirtPx(uint8_t* px, int variant) {
    for (int i = 0; i < 64; i++) px[i] = 6;
    px[(variant & 7) * 8 + ((variant * 3) & 7)] = 7;
    px[((variant * 5) & 7) * 8 + ((variant * 2) & 7)] = 7;
    px[3 * 8 + (variant & 7)] = 12;
}

void gravelPx(uint8_t* px, int variant) {
    for (int i = 0; i < 64; i++) px[i] = 11;
    px[(1 + variant) & 7] = 12;
    px[3 * 8 + ((4 + variant) & 7)] = 12;
    px[6 * 8 + ((2 + variant) & 7)] = 7;
    px[4 * 8 + ((6 + variant) & 7)] = 6;
}

void vegPx(uint8_t* px, int variant) {
    dirtPx(px, variant);
    for (int i = 0; i < 3; i++) {
        int x = 1 + i * 2 + (variant & 1);
        px[2 * 8 + x] = 8;
        px[3 * 8 + x] = 13;
        px[5 * 8 + x] = 8;
        px[6 * 8 + x] = 13;
    }
}

void fenceHPx(uint8_t* px) {
    for (int i = 0; i < 64; i++) px[i] = 9;
    for (int x = 0; x < 8; x++) {
        px[2 * 8 + x] = 10;
        px[5 * 8 + x] = 10;
    }
    for (int y = 0; y < 8; y++) px[y * 8 + 1] = 10;
    px[2 * 8 + 1] = 15;
    px[5 * 8 + 1] = 15;
}

void fenceVPx(uint8_t* px) {
    for (int i = 0; i < 64; i++) px[i] = 9;
    for (int y = 0; y < 8; y++) {
        px[y * 8 + 2] = 10;
        px[y * 8 + 5] = 10;
    }
    px[1 * 8 + 2] = 15;
    px[6 * 8 + 5] = 15;
}

void postPx(uint8_t* px) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) px[y * 8 + x] = (x > 1 && x < 6) ? 9 : 10;
    px[1 * 8 + 3] = 15;
    px[5 * 8 + 4] = 15;
}

int commit(gs::TileAlloc& tiles, uint8_t px[64]) { return tiles.shared(px); }

void paintYard(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t px[64];
    int grass[8], flower[4], dirt[2], gravel[2], veg[2];
    for (int i = 0; i < 8; i++) {
        grassPx(px, i & 3, i >= 4, false);
        grass[i] = commit(tiles, px);
    }
    for (int i = 0; i < 4; i++) {
        grassPx(px, i, false, true);
        flower[i] = commit(tiles, px);
    }
    for (int i = 0; i < 2; i++) {
        dirtPx(px, i + 1);
        dirt[i] = commit(tiles, px);
        gravelPx(px, i + 2);
        gravel[i] = commit(tiles, px);
        vegPx(px, i);
        veg[i] = commit(tiles, px);
    }
    fenceHPx(px);
    int fenceH = commit(tiles, px);
    fenceVPx(px);
    int fenceV = commit(tiles, px);
    postPx(px);
    int post = commit(tiles, px);

    auto yard = [&](int col, int row) {
        float cx = (col + 0.5f) * 8.f - kWellX;
        float cy = (row + 0.5f) * 8.f - kWellY;
        float d = std::hypot(cx, cy);
        uint32_t h = mix(col, row);
        bool path = col >= 19 && col <= 21 && row >= 16 && row <= 25;
        if (d < 16.f) return dirt[1];
        if (d < 34.f) return gravel[h & 1];
        if (path) return ((row + col) & 1) ? gravel[0] : dirt[0];
        if (col >= 3 && col <= 8 && row >= 11 && row <= 17) return veg[h & 1];
        if (col >= 30 && col <= 35 && row >= 15 && row <= 21) return flower[h & 3];
        if ((h % 23u) == 0u) return flower[h & 3];
        int stripe = ((row / 2) & 1) ? 4 : 0;
        return grass[stripe + int(h & 3)];
    };

    for (int row = 0; row < 28; row++) {
        for (int col = 0; col < 40; col++) {
            bool sky = row <= 3;
            bool back = row == 4 || row == 5;
            bool front = row >= 26;
            bool side = col <= 1 || col >= 38;
            bool gate = front && col >= 18 && col <= 22;
            int id = 0;
            if (sky) id = 0;
            else if (gate) id = ((col + row) & 1) ? gravel[0] : dirt[0];
            else if (back || front) id = (col % 8 == 0) ? post : fenceH;
            else if (side) id = (row % 6 == 0) ? post : fenceV;
            else id = yard(col, row);
            if (id) vdp.B.set(col, row, gs::entry(id, PAL_GROUND));
        }
    }
    vdp.B.scroll(0, 0);
    vdp.A.scroll(0, 0);
}

gs::Bitmap keeperArt(int frame) {
    gs::Bitmap b(48, 56);
    int step = frame == 1 ? 3 : 0;
    b.rect(16, 40, 6, 12, 7);
    b.rect(26 + step, 40, 6, 12 - (frame == 1 ? 2 : 0), 7);
    b.rect(16, 32, 6, 10, 5);
    b.rect(26, 32, 6, 10, 6);
    b.rect(17, 20, 16, 16, 3);
    b.rect(17, 20, 16, 4, 4);
    b.ellipse(25, 15, 7, 7, 1);
    b.ellipse(25, 11, 8, 5, 2);
    b.rect(20, 12, 12, 3, 3);
    b.rect(22, 15, 2, 2, 6);
    b.rect(30, 24, 6, 5, 1);
    if (frame == 2) {
        b.line(32, 24, 46, 16, 8, 2.2f);
        b.rect(42, 10, 3, 16, 9);
        b.rect(40, 12, 2, 2, 10);
        b.rect(40, 16, 2, 2, 10);
        b.rect(40, 20, 2, 2, 10);
    } else {
        b.line(32, 26, 44, 40, 8, 2.2f);
        b.rect(40, 36, 3, 12, 9);
        b.rect(38, 38, 2, 2, 10);
        b.rect(38, 42, 2, 2, 10);
    }
    b.outline(15, true);
    return b;
}

gs::Bitmap moleArt(int frame) {
    gs::Bitmap b(40, 28);
    int bob = frame ? 1 : 0;
    b.ellipse(18, 16 + bob, 12, 8, 1);
    b.ellipse(16, 15, 8, 5, 2);
    b.ellipse(30, 15, 6, 5, 1);
    b.ellipse(34, 15, 2.4f, 2.f, 3);
    b.rect(20, 12, 2, 2, 5);
    b.rect(14, 13, 2, 2, 6);
    b.rect(8, 20 + bob, 4, 2, 4);
    b.rect(13, 21, 4, 2, 4);
    b.rect(24, 21 - bob, 4, 2, 4);
    b.outline(15, true);
    return b;
}

gs::Bitmap goatArt(int frame) {
    gs::Bitmap b(48, 40);
    int leg = frame ? 3 : 0;
    b.rect(12, 28, 4, 8, 4);
    b.rect(18, 28 + leg, 4, 8, 4);
    b.rect(30, 28, 4, 8, 4);
    b.rect(36, 28 - leg, 4, 8, 4);
    b.ellipse(24, 22, 14, 9, 1);
    b.ellipse(22, 20, 9, 6, 2);
    b.ellipse(38, 16, 7, 6, 1);
    b.rect(36, 14, 2, 2, 5);
    b.rect(42, 17, 3, 2, 6);
    b.line(34, 12, 28, 3, 3, 2.4f);
    b.line(40, 11, 46, 2, 3, 2.4f);
    b.rect(33, 22, 3, 5, 7);
    b.outline(15, true);
    return b;
}

gs::Bitmap bullArt(int frame) {
    gs::Bitmap b(64, 48);
    int leg = frame ? 3 : 0;
    b.rect(14, 34, 6, 10, 7);
    b.rect(24, 34 + leg, 6, 10, 7);
    b.rect(40, 34, 6, 10, 7);
    b.rect(50, 34 - leg, 6, 10, 7);
    b.ellipse(32, 28, 20, 12, 1);
    b.ellipse(28, 26, 12, 8, 2);
    b.ellipse(50, 22, 10, 8, 1);
    b.ellipse(58, 24, 5, 4, 8);
    b.rect(46, 18, 3, 3, 5);
    b.rect(47, 19, 2, 2, 6);
    b.line(44, 16, 30, 6, 3, 3.f);
    b.line(54, 16, 62, 5, 3, 3.f);
    b.ellipse(60, 26, 3, 3, 4);
    b.outline(15, true);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(72, 84);
    b.rect(16, 30, 5, 28, 5);
    b.rect(51, 30, 5, 28, 6);
    b.poly({{8, 34}, {36, 8}, {64, 34}}, 5);
    b.poly({{18, 32}, {36, 16}, {54, 32}}, 6);
    b.rect(16, 28, 40, 5, 6);
    b.ellipse(36, 58, 26, 16, 1);
    b.ellipse(36, 56, 20, 11, 2);
    b.ellipse(36, 54, 13, 7, 3);
    b.ellipse(36, 55, 9, 4, 7);
    b.ellipse(32, 53, 3, 2, 8);
    b.ellipse(18, 62, 6, 3, 4);
    b.ellipse(52, 64, 5, 3, 4);
    b.line(36, 30, 36, 46, 9, 2.f);
    b.rect(30, 44, 12, 9, 10);
    b.rect(31, 45, 10, 3, 11);
    b.outline(15, true);
    return b;
}

gs::Bitmap rubbleArt() {
    gs::Bitmap b(72, 40);
    b.ellipse(36, 26, 28, 10, 2);
    b.ellipse(20, 22, 12, 8, 1);
    b.ellipse(48, 20, 14, 9, 1);
    b.ellipse(34, 16, 8, 6, 3);
    b.rect(40, 12, 14, 5, 5);
    b.rect(18, 18, 8, 4, 6);
    b.ellipse(28, 24, 4, 2, 7);
    b.outline(15, true);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(36, 44);
    b.line(8, 4, 18, 22, 1, 2.2f);
    b.line(18, 22, 12, 40, 1, 2.2f);
    b.line(16, 18, 30, 12, 1, 2.f);
    b.line(14, 28, 28, 32, 1, 2.f);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 18, 12, 9, 1);
    b.ellipse(12, 16, 7, 6, 2);
    b.ellipse(20, 14, 5, 4, 4);
    return b;
}

gs::Bitmap whooshArt() {
    gs::Bitmap b(32, 32);
    b.line(4, 26, 26, 8, 3, 2.f);
    b.line(8, 28, 30, 14, 1, 2.f);
    b.line(2, 18, 22, 4, 4, 2.f);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(52, 68);
    b.rect(22, 38, 8, 24, 3);
    b.rect(16, 58, 20, 5, 3);
    b.ellipse(26, 28, 22, 18, 1);
    b.ellipse(20, 24, 12, 10, 2);
    b.ellipse(32, 20, 7, 6, 1);
    b.outline(15, true);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(56, 48);
    b.rect(6, 20, 44, 24, 4);
    b.rect(8, 22, 40, 20, 5);
    b.poly({{4, 22}, {28, 4}, {52, 22}}, 6);
    b.rect(22, 28, 12, 16, 3);
    b.rect(12, 26, 8, 7, 12);
    b.rect(36, 26, 8, 7, 12);
    b.outline(15, true);
    return b;
}

gs::Bitmap barrowArt() {
    gs::Bitmap b(44, 30);
    b.ellipse(12, 18, 8, 8, 7);
    b.ellipse(12, 18, 3, 3, 11);
    b.poly({{16, 10}, {40, 8}, {36, 18}, {18, 20}}, 7);
    b.line(34, 12, 42, 4, 3, 2.f);
    b.line(18, 18, 6, 26, 3, 2.f);
    b.outline(15, true);
    return b;
}

gs::Bitmap canArt() {
    gs::Bitmap b(32, 26);
    b.ellipse(14, 16, 9, 7, 10);
    b.ellipse(14, 15, 6, 4, 11);
    b.rect(18, 8, 10, 3, 10);
    b.line(10, 10, 4, 3, 11, 2.f);
    b.outline(15, true);
    return b;
}

gs::Bitmap shirtArt() {
    gs::Bitmap b(28, 26);
    b.rect(6, 6, 16, 16, 8);
    b.rect(6, 6, 16, 5, 9);
    b.rect(2, 8, 5, 8, 8);
    b.rect(21, 8, 5, 8, 8);
    b.outline(15, true);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(88, 22);
    b.rect(2, 2, 4, 16, 5);
    b.rect(80, 2, 4, 16, 6);
    b.line(6, 5, 44, 10, 9, 1.8f);
    b.line(44, 10, 80, 6, 9, 1.8f);
    return b;
}

gs::Bitmap bushArt() {
    gs::Bitmap b(32, 24);
    b.ellipse(16, 14, 13, 8, 1);
    b.ellipse(12, 12, 7, 5, 2);
    b.rect(20, 10, 2, 2, 9);
    b.rect(10, 14, 2, 2, 9);
    b.outline(15, false);
    return b;
}

gs::Bitmap birdArt(int frame) {
    gs::Bitmap b(32, 18);
    float lift = frame ? 4.f : 0.f;
    b.line(2, 12, 9, 6 - lift, 5, 2.f);
    b.line(9, 6 - lift, 16, 12, 5, 2.f);
    b.line(16, 12, 23, 6 - lift, 5, 2.f);
    b.line(23, 6 - lift, 30, 12, 5, 2.f);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(64, 28);
    b.ellipse(18, 16, 14, 8, 1);
    b.ellipse(34, 14, 16, 10, 1);
    b.ellipse(48, 16, 12, 7, 2);
    b.ellipse(30, 12, 8, 5, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(36, 36);
    for (int i = 0; i < 8; i++) {
        float a = i * TAU / 8.f;
        b.line(18 + std::cos(a) * 8.f, 18 + std::sin(a) * 8.f, 18 + std::cos(a) * 16.f, 18 + std::sin(a) * 16.f, 3, 2.f);
    }
    b.ellipse(18, 18, 8, 8, 3);
    b.ellipse(18, 18, 5, 5, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    const uint16_t panel = gs::rgb4(2, 2, 3);
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_TEXT, {0, ink, panel, gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 4), gs::rgb4(8, 15, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), panel});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), panel, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 15, 5), panel});

    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(14, 10, 7), gs::rgb4(5, 3, 2), gs::rgb4(3, 7, 14), gs::rgb4(2, 4, 9), gs::rgb4(4, 5, 8),
            gs::rgb4(2, 2, 4), gs::rgb4(3, 2, 2), gs::rgb4(12, 8, 3), gs::rgb4(13, 14, 15), gs::rgb4(8, 9, 10)});
    setPal(vdp, PAL_MOLE,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(14, 8, 8), gs::rgb4(13, 12, 11), gs::rgb4(2, 1, 1),
            gs::rgb4(15, 14, 13)});
    setPal(vdp, PAL_GOAT,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(11, 11, 9), gs::rgb4(8, 6, 4), gs::rgb4(3, 2, 2), gs::rgb4(2, 1, 1),
            gs::rgb4(13, 8, 8), gs::rgb4(14, 12, 3)});
    setPal(vdp, PAL_BULL,
           {0, gs::rgb4(11, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(14, 13, 10), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 14),
            gs::rgb4(3, 1, 1), gs::rgb4(3, 2, 2), gs::rgb4(13, 8, 7)});
    setPal(vdp, PAL_WELL,
           {0, gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(4, 10, 3), gs::rgb4(11, 7, 3),
            gs::rgb4(6, 4, 2), gs::rgb4(3, 8, 12), gs::rgb4(8, 14, 15), gs::rgb4(9, 7, 4), gs::rgb4(8, 9, 10),
            gs::rgb4(5, 6, 7)});
    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(4, 11, 3), gs::rgb4(2, 7, 2), gs::rgb4(8, 5, 3), gs::rgb4(13, 4, 3), gs::rgb4(8, 2, 2),
            gs::rgb4(5, 5, 7), gs::rgb4(10, 7, 4), gs::rgb4(14, 14, 15), gs::rgb4(13, 3, 3), gs::rgb4(8, 11, 6),
            gs::rgb4(4, 6, 4), gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 11, 8), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 15), gs::rgb4(15, 8, 3),
            gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15),
            gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15), gs::rgb4(15, 15, 15)},
           false);
    setPal(vdp, PAL_GROUND,
           {0, gs::rgb4(8, 13, 4), gs::rgb4(5, 10, 3), gs::rgb4(3, 7, 2), gs::rgb4(14, 3, 3), gs::rgb4(15, 13, 3),
            gs::rgb4(10, 7, 3), gs::rgb4(7, 5, 2), gs::rgb4(6, 13, 3), gs::rgb4(12, 8, 4), gs::rgb4(7, 4, 2),
            gs::rgb4(10, 10, 9), gs::rgb4(6, 6, 5), gs::rgb4(3, 9, 3), gs::rgb4(14, 12, 8), gs::rgb4(3, 2, 2)},
           false);
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 15), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(3, 3, 5)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, a, tiles);
    paintYard(vdp, tiles);

    a.well = up(vdp, wellArt());
    a.rubble = up(vdp, rubbleArt());
    a.crack = up(vdp, crackArt());
    for (int i = 0; i < 3; i++) a.keeper[i] = up(vdp, keeperArt(i));
    for (int i = 0; i < 2; i++) {
        a.mole[i] = up(vdp, moleArt(i));
        a.goat[i] = up(vdp, goatArt(i));
        a.bull[i] = up(vdp, bullArt(i));
        a.bird[i] = up(vdp, birdArt(i));
    }
    a.puff = up(vdp, puffArt());
    a.whoosh = up(vdp, whooshArt());
    a.bar = up(vdp, barArt(), false);
    a.shadow = up(vdp, shadowArt());
    a.tree = up(vdp, treeArt());
    a.shed = up(vdp, shedArt());
    a.barrow = up(vdp, barrowArt());
    a.can = up(vdp, canArt());
    a.shirt = up(vdp, shirtArt());
    a.rope = up(vdp, ropeArt());
    a.bush = up(vdp, bushArt());
    a.cloud = up(vdp, cloudArt());
    a.sun = up(vdp, sunArt());
}

}  // namespace yardwell
