#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace militia {
namespace {

constexpr float TAU = 6.2831853f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void weapon(gs::Bitmap& b, float x, float y, float dx, float dy, int kind, int step) {
    const float x1 = x + dx * 18.f, y1 = y + dy * 18.f;
    if (kind == 0) {
        b.line(x, y, x1, y1, 6, 2.6f);
        b.line(x + dx * 11.f, y + dy * 11.f, x1, y1, 7, 1.5f);
        b.rect(x - dy * 3.f - 2.f, y + dx * 3.f - 2.f, 5, 4, 6);
    } else if (kind == 1) {
        b.line(x, y, x1, y1, 6, 2.8f);
        b.ellipse(x1, y1, 5, 4, 7);
        b.ellipse(x1 - dx * 2.f, y1 - dy * 2.f, 3, 2.4f, 6);
    } else if (kind == 2) {
        b.line(x, y, x + dx * 14.f, y + dy * 14.f, 6, 2.4f);
        float fx = x1 + dx * 2.f, fy = y1 + dy * 2.f;
        if (step) {
            b.ellipse(fx, fy, 7, 6, 7);
            b.ellipse(fx - dx, fy - dy * 2.f, 3.5f, 3.f, 8);
        } else {
            b.ellipse(fx + 1.f, fy, 6, 7, 7);
            b.ellipse(fx, fy - 2.f, 3.f, 3.5f, 8);
        }
    } else {
        b.line(x, y, x + dx * 12.f, y + dy * 12.f, 7, 1.8f);
        b.rect(x - 2.f, y - 2.f, 4, 4, 6);
    }
}

void hat(gs::Bitmap& b, float x, float y, int style) {
    if (style == 0) {
        b.ellipse(x, y, 8, 6, 5);
        b.rect(x - 10, y + 1, 20, 4, 5);
        b.rect(x - 2, y - 5, 4, 4, 8);
        b.ellipse(x, y + 5, 5, 4, 4);
    } else if (style == 3) {
        b.ellipse(x, y, 9, 8, 3);
        b.ellipse(x, y + 2, 5, 4, 4);
    } else if (style == 2) {
        b.ellipse(x, y + 1, 6, 6, 4);
        b.rect(x - 7, y - 1, 14, 3, 8);
    } else {
        b.ellipse(x, y, 8, 7, 5);
        b.ellipse(x, y + 3, 6, 5, 4);
        b.rect(x - 6, y + 1, 12, 2, 8);
    }
}

gs::Bitmap person(int dir, int step, int weap, int hatStyle, int bulk) {
    gs::Bitmap b(48, 56);
    const float cx = 24.f;
    const int leg = step ? 3 : 0;
    const int bw = 6 + bulk;
    if (dir == 2) {
        hat(b, 16, 16, hatStyle);
        b.rect(12, 22, 10 + bulk, 12, 2);
        b.rect(12, 22, 10 + bulk, 3, 1);
        b.rect(14, 36, 4, 11 - leg, 3);
        b.rect(20, 36 + leg, 4, 11 - leg, 3);
        b.rect(14, 46, 4, 4, 9);
        b.rect(20, 46, 4, 4, 9);
        b.ellipse(24, 28, 4, 3, 4);
        weapon(b, 26, 28, 1, 0, weap, step);
        if (hatStyle == 2) b.rect(22, 30, 5, 6, 6);
    } else if (dir == 1) {
        hat(b, cx, 18, hatStyle);
        b.rect(cx - bw, 24, float(bw * 2), 12, 2);
        b.rect(cx - bw, 24, float(bw * 2), 3, 1);
        b.rect(cx - bw + 1, 36, 4, 10 - (step ? 2 : 0), 3);
        b.rect(cx + 1, 38 - (step ? 0 : 2), 4, 10, 3);
        b.rect(cx - bw + 1, 46, 4, 3, 9);
        b.rect(cx + 1, 46, 4, 3, 9);
        weapon(b, cx + 6, 22, 0, -1, weap, step);
    } else {
        hat(b, cx, 14, hatStyle);
        if (hatStyle != 3) {
            b.set(int(cx - 3), 18, 3);
            b.set(int(cx + 2), 18, 3);
        }
        b.rect(cx - bw, 22, float(bw * 2), 13, 2);
        b.rect(cx - bw, 22, float(bw * 2), 3, 1);
        b.rect(cx - 2, 28, 3, 3, 8);
        b.rect(cx - bw + 1, 35, 4, 11 - leg, 3);
        b.rect(cx + 2, 35 + (step ? 2 : 0), 4, 11 - (step ? 2 : 0), 3);
        b.rect(cx - bw + 1, 46, 4, 4, 9);
        b.rect(cx + 2, 46, 4, 4, 9);
        b.ellipse(cx + bw, 30, 3.5f, 3, 4);
        weapon(b, cx + bw, 32, 0, 1, weap, step);
        if (hatStyle == 2) b.rect(cx - bw - 1, 26, 5, 6, 6);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap wellArt() {
    gs::Bitmap b(96, 96);
    for (int i = 0; i < 12; ++i) {
        float a = (i + 0.5f) * TAU / 12.f;
        float x = 48.f + std::cos(a) * 30.f;
        float y = 50.f + std::sin(a) * 24.f;
        b.ellipse(x, y, 11, 9, (i & 1) ? 2 : 1);
        b.ellipse(x - 2.f, y - 2.f, 5, 4, 1);
    }
    b.ellipse(48, 50, 20, 16, 3);
    b.ellipse(48, 49, 16, 12, 6);
    b.ellipse(48, 48, 13, 10, 4);
    b.ellipse(43, 45, 5, 3, 5);
    b.rect(22, 32, 6, 26, 7);
    b.rect(68, 32, 6, 26, 7);
    b.rect(18, 28, 60, 7, 7);
    b.rect(20, 30, 56, 3, 8);
    b.rect(60, 34, 8, 11, 9);
    b.line(48, 34, 48, 54, 10, 1.6f);
    b.rect(42, 50, 12, 8, 11);
    b.rect(43, 51, 10, 3, 1);
    b.outline(15, false);
    return b;
}

gs::Bitmap crackArt() {
    gs::Bitmap b(36, 48);
    b.line(10, 4, 18, 20, 15, 1.5f);
    b.line(18, 20, 12, 42, 15, 1.4f);
    b.line(18, 18, 30, 28, 3, 1.3f);
    return b;
}

gs::Bitmap glintArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 2);
    b.ellipse(6, 4, 2.5f, 1.5f, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(48, 64);
    b.rect(21, 36, 7, 24, 4);
    b.rect(23, 34, 3, 6, 6);
    b.ellipse(24, 28, 18, 16, 2);
    b.ellipse(18, 24, 10, 9, 1);
    b.ellipse(30, 30, 8, 7, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap bucketArt() {
    gs::Bitmap b(20, 22);
    b.poly({{3, 7}, {16, 7}, {14, 18}, {5, 18}}, 11);
    b.rect(3, 5, 14, 3, 1);
    b.line(6, 6, 10, 1, 10, 1.4f);
    b.line(14, 6, 10, 1, 10, 1.4f);
    b.outline(15, false);
    return b;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 8);
    b.ellipse(4, 4, 1.4f, 1.4f, 3);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 4);
    b.ellipse(8, 8, 4, 3, 3);
    b.ellipse(8, 8, 2, 1.4f, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 18, 12, 10, 5);
    b.ellipse(12, 14, 7, 6, 6);
    b.ellipse(20, 16, 5, 4, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(16, 7, 14, 5, 1);
    return b;
}

void tileFrom(uint8_t px[64], const char* const rows[8]) {
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            char c = rows[y][x];
            px[y * 8 + x] = c == '.' ? 0 : uint8_t(c - '0');
        }
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void paintYard(gs::VDP& vdp, const Art& art) {
    vdp.A.clear();
    vdp.B.clear();
    for (int cy = 0; cy < 32; ++cy) {
        for (int cx = 0; cx < 64; ++cx) {
            float dx = (cx + 0.5f) * 8.f - 160.f;
            float dy = (cy + 0.5f) * 8.f - 118.f;
            float d = std::sqrt(dx * dx + dy * dy);
            bool lane = (cx >= 18 && cx <= 21) || (cy >= 13 && cy <= 16);
            int tile = d < 40.f ? art.cobble : (d < 72.f || lane) ? art.dirt : art.grass[(cx * 3 + cy * 5) % 3];
            vdp.B.set(cx, cy, gs::entry(tile, PAL_GROUND));
        }
    }
    for (int cx = 0; cx < 40; ++cx) {
        bool gate = cx >= 17 && cx <= 22;
        if (!gate) {
            vdp.A.set(cx, 1, gs::entry(art.fenceH, PAL_TREE));
            vdp.A.set(cx, 26, gs::entry(art.fenceH, PAL_TREE));
        }
    }
    for (int cy = 2; cy <= 25; ++cy) {
        bool gate = cy >= 12 && cy <= 17;
        if (!gate) {
            vdp.A.set(1, cy, gs::entry(art.fenceV, PAL_TREE));
            vdp.A.set(38, cy, gs::entry(art.fenceV, PAL_TREE));
        }
    }
}

}  // namespace

void yardTint(gs::VDP& vdp, int wave, uint16_t& sky) {
    if (wave < 0) wave = 0;
    if (wave > 2) wave = 2;
    const uint16_t hi[3] = {gs::rgb4(7, 12, 4), gs::rgb4(8, 11, 3), gs::rgb4(5, 7, 3)};
    const uint16_t mid[3] = {gs::rgb4(4, 9, 3), gs::rgb4(5, 8, 2), gs::rgb4(3, 5, 2)};
    const uint16_t dk[3] = {gs::rgb4(2, 6, 2), gs::rgb4(3, 5, 1), gs::rgb4(2, 3, 1)};
    const uint16_t sk[3] = {gs::rgb4(3, 5, 2), gs::rgb4(6, 6, 3), gs::rgb4(4, 2, 2)};
    vdp.setColor(PAL_GROUND * 16 + 1, hi[wave]);
    vdp.setColor(PAL_GROUND * 16 + 2, mid[wave]);
    vdp.setColor(PAL_GROUND * 16 + 3, dk[wave]);
    sky = sk[wave];
}

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 14, 5));

    const uint16_t ink = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_MIL, {0, gs::rgb4(9, 11, 14), gs::rgb4(4, 6, 11), gs::rgb4(2, 3, 6), gs::rgb4(13, 9, 6),
                          gs::rgb4(3, 4, 7), gs::rgb4(8, 5, 2), gs::rgb4(12, 13, 14), gs::rgb4(14, 11, 3),
                          gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_RAID, {0, gs::rgb4(14, 8, 6), gs::rgb4(11, 3, 3), gs::rgb4(6, 2, 2), gs::rgb4(13, 9, 6),
                           gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 2), gs::rgb4(14, 12, 8),
                           gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TORCH, {0, gs::rgb4(12, 6, 4), gs::rgb4(8, 3, 2), gs::rgb4(4, 2, 2), gs::rgb4(13, 9, 6),
                            gs::rgb4(3, 2, 2), gs::rgb4(7, 4, 2), gs::rgb4(15, 7, 1), gs::rgb4(15, 14, 4),
                            gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_RUN, {0, gs::rgb4(13, 12, 8), gs::rgb4(9, 8, 5), gs::rgb4(5, 4, 3), gs::rgb4(13, 9, 6),
                          gs::rgb4(5, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(12, 12, 13), gs::rgb4(12, 3, 3),
                          gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WELL, {0, gs::rgb4(13, 12, 11), gs::rgb4(9, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(2, 5, 9),
                           gs::rgb4(8, 12, 14), gs::rgb4(1, 2, 5), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4),
                           gs::rgb4(3, 5, 11), gs::rgb4(11, 10, 7), gs::rgb4(6, 6, 7), 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), gs::rgb4(15, 14, 4), gs::rgb4(15, 8, 1),
                         gs::rgb4(8, 8, 7), gs::rgb4(13, 13, 12), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 10), 0, 0, 0, 0,
                         0, 0, ink});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(9, 14, 5), gs::rgb4(4, 9, 3), gs::rgb4(2, 5, 2), gs::rgb4(7, 5, 2),
                           gs::rgb4(1, 1, 1), gs::rgb4(12, 9, 4), gs::rgb4(7, 5, 3), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GROUND, {0, gs::rgb4(7, 12, 4), gs::rgb4(4, 9, 3), gs::rgb4(2, 6, 2), gs::rgb4(13, 12, 4),
                             gs::rgb4(9, 7, 4), gs::rgb4(6, 5, 3), gs::rgb4(10, 9, 8), gs::rgb4(6, 6, 7),
                             gs::rgb4(5, 8, 4), 0, 0, 0, 0, 0, ink});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);

    const char* g0[8] = {"22212222", "22122232", "22222222", "32222212", "22223222", "21222222", "22222122", "22322222"};
    const char* g1[8] = {"22222222", "22242222", "22122222", "22222212", "92222222", "22222322", "22221222", "23222222"};
    const char* g2[8] = {"32222222", "22223222", "22212222", "21222229", "22222222", "22322222", "22222122", "12222232"};
    const char* dirt[8] = {"55555555", "56555555", "55565555", "55555565", "55655555", "55555555", "55556555", "65555555"};
    const char* cob[8] = {"77778888", "77779888", "77778888", "88887777", "88887777", "77777788", "77777789", "88888877"};
    const char* fh[8] = {"........", "........", "66666666", "77777777", "........", "66666666", "77777777", "........"};
    const char* fv[8] = {"..66....", "..77....", "..66....", "..77....", "..66....", "..77....", "..66....", "..77...."};
    const char* const* grows[3] = {g0, g1, g2};
    uint8_t px[64];
    for (int i = 0; i < 3; ++i) {
        tileFrom(px, grows[i]);
        art.grass[i] = tiles.shared(px);
    }
    tileFrom(px, dirt);
    art.dirt = tiles.shared(px);
    tileFrom(px, cob);
    art.cobble = tiles.shared(px);
    tileFrom(px, fh);
    art.fenceH = tiles.shared(px);
    tileFrom(px, fv);
    art.fenceV = tiles.shared(px);
    paintYard(vdp, art);

    for (int d = 0; d < 3; ++d) {
        for (int s = 0; s < 2; ++s) {
            art.mil[d][s] = gs::uploadMipped(vdp, person(d, s, 0, 0, 1));
            art.run[d][s] = gs::uploadMipped(vdp, person(d, s, 3, 2, 0));
            art.raid[d][s] = gs::uploadMipped(vdp, person(d, s, 1, 1, 2));
            art.torch[d][s] = gs::uploadMipped(vdp, person(d, s, 2, 3, 2));
        }
    }
    art.well = gs::uploadMipped(vdp, wellArt());
    art.crack = gs::uploadMipped(vdp, crackArt());
    art.glint = gs::uploadMipped(vdp, glintArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.bucket = gs::uploadMipped(vdp, bucketArt());
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    uint16_t sky = 0;
    yardTint(vdp, 0, sky);
}

}  // namespace militia
