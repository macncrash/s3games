#include "game/art.h"

#include <cmath>
#include <cstring>
#include <initializer_list>
#include <string>

namespace gauntlet {
namespace {

constexpr float PI = 3.14159265f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void arc(gs::Bitmap& b, float cx, float cy, float r, float a0, float a1, int c, float thick) {
    const int n = 12;
    for (int i = 0; i < n; i++) {
        float t0 = a0 + (a1 - a0) * i / n;
        float t1 = a0 + (a1 - a0) * (i + 1) / n;
        b.line(cx + std::cos(t0) * r, cy + std::sin(t0) * r, cx + std::cos(t1) * r, cy + std::sin(t1) * r, c, thick);
    }
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const uint8_t* px) {
    int id = tiles.alloc(1);
    vdp.loadTile(id, px);
    return id;
}

void makeFloor(uint8_t* p, int variant) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int c = (x == 0 || y == 0) ? 3 : ((x + y * 2 + variant) & 1 ? 1 : 2);
            if (variant == 3) {
                if ((x == 3 && y > 1 && y < 6) || (y == 4 && x > 1 && x < 6)) c = 4;
                if (x > 5 && y > 5) c = 8;
            }
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void makeWall(uint8_t* p, int variant) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            bool bed = y == 0 || y == 4;
            bool stud = (y < 4) ? (x == (variant ? 2 : 0)) : (x == (variant ? 6 : 4));
            int c = (bed || stud) ? 7 : ((x + variant) % 3 == 0 ? 5 : 6);
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void makeCarpet(uint8_t* p, int variant) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int c = (x == 0 || x == 7) ? 14 : (((y + variant) & 1) ? 12 : 13);
            if (x == 3 || x == 4) c = 14;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void makeSkirt(uint8_t* p) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) p[y * 8 + x] = uint8_t((x == 0 || y == 0) ? 7 : 5);
}

void makeHeart(uint8_t* p) {
    static const char* rows[8] = {
        "........", ".11.11..", "1111111.", "1111111.", ".11111..", "..111...", "...1....", "........",
    };
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) p[y * 8 + x] = rows[y][x] == '1' ? 1 : 0;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        a.font[c - 32] = uploadTile(vdp, tiles, px);
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap knight(int face, int step) {
    gs::Bitmap b(40, 48);
    const int hi = 1, steel = 2, dark = 3, cloak = 4, shade = 5, skin = 6, gold = 7, leather = 8, blade = 9, mail = 10;
    int leg = step ? 3 : 0;
    if (face == 0) {
        b.ellipse(20, 13, 8, 8, steel);
        b.ellipse(20, 12, 5, 5, hi);
        b.rect(13, 18, 14, 5, dark);
        b.rect(11, 22, 18, 14, cloak);
        b.rect(13, 24, 14, 8, shade);
        b.rect(18, 22, 4, 12, mail);
        b.poly({{6, 22}, {13, 20}, {14, 38}, {6, 36}}, steel);
        b.poly({{8, 24}, {12, 23}, {12, 34}, {8, 33}}, gold);
        b.rect(30, 10, 3, 22, blade);
        b.rect(28, 18, 7, 3, gold);
        b.rect(14, 36, 5, 8, leather);
        b.rect(22, 36, 5, 8, leather);
        b.rect(13, 43, 6, 3, dark);
        b.rect(21, 42 + (step ? 1 : 0), 6, 3, dark);
        b.rect(14, 44 - leg, 5, 2, leather);
    } else if (face == 2) {
        b.ellipse(20, 13, 8, 9, steel);
        b.ellipse(20, 12, 6, 6, hi);
        b.rect(14, 15, 12, 3, dark);
        b.rect(16, 18, 3, 2, skin);
        b.rect(22, 18, 3, 2, skin);
        b.rect(11, 21, 18, 14, cloak);
        b.rect(13, 23, 14, 9, shade);
        b.poly({{5, 20}, {14, 18}, {15, 40}, {4, 36}}, steel);
        b.poly({{7, 22}, {12, 21}, {12, 36}, {6, 34}}, gold);
        b.rect(16, 19, 8, 3, gold);
        b.rect(31, 22, 3, 18, blade);
        b.rect(29, 22, 7, 3, gold);
        b.rect(14, 35, 5, 8 + leg, leather);
        b.rect(22, 35, 5, 10 - leg, leather);
        b.rect(13, 43, 6, 3, dark);
        b.rect(21, 43, 6, 3, dark);
    } else {
        b.ellipse(15, 13, 7, 8, steel);
        b.ellipse(17, 12, 4, 5, hi);
        b.rect(14, 15, 8, 3, dark);
        b.rect(18, 16, 3, 2, skin);
        b.rect(9, 21, 14, 14, cloak);
        b.rect(11, 23, 10, 8, shade);
        b.poly({{6, 22}, {12, 20}, {12, 38}, {5, 35}}, steel);
        b.line(12, 24, 12, 34, gold, 1.5f);
        b.rect(20, 18, 16, 3, blade);
        b.rect(18, 16, 3, 8, gold);
        b.rect(11, 35, 5, 9 - leg, leather);
        b.rect(17, 35, 5, 9 + leg, leather);
        b.rect(10, 43, 6, 3, dark);
        b.rect(17, 43, 6, 3, dark);
    }
    b.outline(dark, false);
    return b;
}

gs::Bitmap grunt(int step) {
    gs::Bitmap b(36, 44);
    int leg = step ? 3 : 0;
    b.ellipse(18, 12, 9, 9, 2);
    b.ellipse(18, 11, 6, 6, 1);
    b.ellipse(18, 15, 5, 5, 9);
    b.ellipse(18, 16, 3, 3, 4);
    b.rect(16, 15, 2, 2, 6);
    b.rect(20, 15, 2, 2, 6);
    b.rect(10, 20, 16, 14, 2);
    b.rect(12, 22, 12, 8, 1);
    b.rect(16, 20, 4, 13, 7);
    b.rect(26, 14, 4, 16, 5);
    b.ellipse(28, 12, 5, 5, 5);
    b.ellipse(27, 11, 2, 2, 8);
    b.rect(12, 34, 5, 8 + leg, 3);
    b.rect(19, 34, 5, 10 - leg, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap archer(int step) {
    gs::Bitmap b(40, 44);
    int leg = step ? 3 : 0;
    b.ellipse(15, 11, 7, 8, 2);
    b.ellipse(16, 13, 4, 4, 5);
    b.rect(14, 12, 2, 2, 3);
    b.rect(18, 12, 2, 2, 3);
    b.rect(11, 18, 11, 15, 6);
    b.rect(13, 20, 7, 9, 1);
    b.rect(15, 18, 3, 12, 9);
    b.line(28, 8, 33, 22, 4, 2.2f);
    b.line(33, 22, 28, 36, 4, 2.2f);
    b.line(28, 9, 28, 35, 7, 1.2f);
    b.line(16, 21, 30, 21, 8, 1.6f);
    b.rect(29, 19, 3, 4, 8);
    b.rect(12, 33, 5, 8 + leg, 2);
    b.rect(18, 33, 5, 9 - leg, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap warden(int step) {
    gs::Bitmap b(52, 60);
    int leg = step ? 3 : 0;
    b.ellipse(26, 16, 11, 11, 2);
    b.ellipse(26, 15, 8, 8, 1);
    b.rect(18, 18, 16, 4, 8);
    b.rect(22, 17, 3, 3, 6);
    b.rect(28, 17, 3, 3, 6);
    b.ellipse(14, 10, 4, 5, 7);
    b.ellipse(38, 10, 4, 5, 7);
    b.rect(16, 6, 4, 6, 9);
    b.rect(32, 6, 4, 6, 9);
    b.rect(12, 26, 28, 16, 10);
    b.rect(14, 28, 24, 10, 2);
    b.rect(22, 26, 8, 16, 4);
    b.rect(24, 28, 4, 12, 5);
    b.rect(10, 24, 8, 8, 2);
    b.rect(34, 24, 8, 8, 1);
    b.line(40, 30, 48, 8, 6, 2.4f);
    b.poly({{44, 6}, {50, 8}, {46, 16}}, 1);
    b.rect(16, 42, 7, 12 + leg, 10);
    b.rect(28, 42, 7, 14 - leg, 10);
    b.rect(15, 52, 8, 4, 3);
    b.rect(28, 52, 8, 4, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap slashNorth() {
    gs::Bitmap b(44, 34);
    arc(b, 22, 28, 18, -2.7f, -0.45f, 3, 3.2f);
    arc(b, 22, 28, 13, -2.55f, -0.6f, 2, 2.2f);
    arc(b, 22, 28, 8, -2.4f, -0.75f, 1, 1.5f);
    return b;
}

gs::Bitmap slashEast() {
    gs::Bitmap b(36, 40);
    arc(b, 8, 20, 16, -1.15f, 1.15f, 3, 3.2f);
    arc(b, 8, 20, 11, -1.0f, 1.0f, 2, 2.2f);
    arc(b, 8, 20, 6, -0.8f, 0.8f, 1, 1.4f);
    return b;
}

gs::Bitmap slashSouth() {
    gs::Bitmap b(44, 34);
    arc(b, 22, 6, 18, 0.45f, 2.7f, 3, 3.2f);
    arc(b, 22, 6, 13, 0.6f, 2.55f, 2, 2.2f);
    arc(b, 22, 6, 8, 0.75f, 2.4f, 1, 1.5f);
    return b;
}

gs::Bitmap boltArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 7);
    b.ellipse(7, 7, 4, 4, 4);
    b.ellipse(7, 7, 2, 2, 5);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 6);
    b.ellipse(5, 5, 2, 2, 1);
    return b;
}

gs::Bitmap doorClosed() {
    gs::Bitmap b(96, 112);
    b.rect(0, 0, 96, 112, 9);
    b.rect(6, 8, 84, 96, 5);
    b.rect(12, 14, 32, 82, 2);
    b.rect(52, 14, 32, 82, 1);
    b.rect(14, 16, 28, 78, 2);
    b.rect(54, 16, 28, 78, 3);
    b.rect(12, 28, 72, 6, 4);
    b.rect(12, 58, 72, 6, 4);
    b.rect(12, 78, 72, 5, 5);
    b.rect(40, 40, 16, 22, 4);
    b.rect(44, 44, 8, 14, 6);
    b.ellipse(36, 52, 4, 4, 6);
    b.ellipse(60, 52, 4, 4, 6);
    b.ellipse(36, 52, 2, 2, 5);
    b.ellipse(60, 52, 2, 2, 5);
    for (int y : {32, 62})
        for (int x : {16, 36, 56, 76}) b.rect(float(x), float(y), 3, 3, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap doorOpened() {
    gs::Bitmap b(96, 112);
    b.rect(0, 0, 96, 112, 9);
    b.rect(8, 10, 80, 94, 12);
    b.rect(18, 16, 60, 80, 10);
    b.rect(28, 22, 40, 68, 11);
    b.rect(36, 30, 24, 52, 7);
    b.rect(42, 40, 12, 28, 8);
    b.rect(8, 12, 10, 88, 2);
    b.rect(78, 12, 10, 88, 1);
    b.rect(8, 30, 10, 5, 4);
    b.rect(78, 30, 10, 5, 4);
    b.rect(8, 64, 10, 5, 4);
    b.rect(78, 64, 10, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap keyArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(5, 7, 5, 5, 1);
    b.ellipse(5, 7, 2, 2, 0);
    b.rect(9, 6, 8, 3, 1);
    b.rect(13, 9, 2, 3, 2);
    b.rect(16, 9, 2, 2, 1);
    b.outline(2, false);
    b.ellipse(5, 7, 2, 2, 0);
    return b;
}

gs::Bitmap flaskArt() {
    gs::Bitmap b(16, 22);
    b.rect(6, 1, 4, 4, 5);
    b.rect(7, 4, 2, 3, 3);
    b.ellipse(8, 14, 6, 7, 3);
    b.ellipse(8, 15, 4, 5, 4);
    b.ellipse(6, 12, 2, 2, 1);
    b.outline(2, false);
    return b;
}

gs::Bitmap torchArt(int frame) {
    gs::Bitmap b(16, 28);
    b.rect(6, 14, 4, 12, 8);
    b.rect(4, 24, 8, 3, 2);
    float fy = frame ? 8.f : 10.f;
    float ry = frame ? 7.f : 6.f;
    b.ellipse(8, fy, 5, ry, 6);
    b.ellipse(8, fy - 1, 3, ry - 2, 10);
    b.ellipse(8, fy - 2, 1.6f, 2.2f, 7);
    return b;
}

gs::Bitmap pillarArt() {
    gs::Bitmap b(28, 56);
    b.rect(4, 6, 20, 6, 10);
    b.rect(6, 12, 16, 34, 11);
    b.rect(8, 14, 4, 30, 6);
    b.rect(4, 44, 20, 6, 10);
    b.rect(2, 48, 24, 4, 7);
    b.outline(7, false);
    return b;
}

gs::Bitmap skullArt() {
    gs::Bitmap b(16, 14);
    b.ellipse(8, 6, 6, 5, 9);
    b.rect(5, 7, 2, 2, 2);
    b.rect(9, 7, 2, 2, 2);
    b.rect(6, 10, 4, 2, 8);
    b.outline(2, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(14, 6, 12, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const auto C = gs::rgb4;
    setPal(vdp, PAL_WHITE, {0, C(15, 15, 15), C(11, 11, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(1, 1, 2)});
    setPal(vdp, PAL_GOLD, {0, C(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(2, 1, 0)});
    setPal(vdp, PAL_RED, {0, C(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(2, 0, 0)});
    setPal(vdp, PAL_GREEN, {0, C(5, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(0, 2, 1)});
    setPal(vdp, PAL_DIM, {0, C(5, 5, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(1, 1, 2)});
    setPal(vdp, PAL_KNIGHT, {0, C(14, 15, 15), C(9, 10, 12), C(3, 4, 6), C(13, 2, 2), C(8, 1, 1), C(14, 10, 7),
                             C(14, 11, 3), C(5, 3, 2), C(15, 15, 15), C(6, 7, 8), 0, 0, 0, 0, C(1, 1, 2)});
    setPal(vdp, PAL_GRUNT, {0, C(8, 9, 5), C(5, 6, 3), C(2, 2, 1), C(12, 8, 6), C(8, 5, 2), C(15, 13, 6), C(4, 2, 2),
                            C(13, 12, 9), C(3, 3, 2), 0, 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_ARCHER, {0, C(11, 8, 4), C(7, 5, 3), C(2, 2, 1), C(6, 4, 2), C(13, 9, 6), C(4, 7, 4), C(12, 11, 9),
                             C(14, 14, 15), C(3, 4, 3), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WARDEN, {0, C(13, 13, 15), C(8, 8, 10), C(2, 2, 4), C(12, 1, 2), C(6, 0, 1), C(14, 11, 4),
                             C(9, 8, 7), C(1, 1, 2), C(15, 4, 5), C(5, 5, 7), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, C(15, 15, 14), C(15, 12, 4), C(15, 5, 2), C(12, 14, 15), C(15, 15, 15), C(15, 8, 2),
                         C(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ITEM, {0, C(15, 12, 3), C(9, 6, 1), C(9, 14, 15), C(13, 2, 3), C(8, 5, 2), C(15, 9, 2),
                           C(15, 15, 8), C(6, 3, 1), C(14, 13, 11), C(15, 6, 2), 0, 0, 0, 0, C(1, 1, 1)});
    setPal(vdp, PAL_DOOR, {0, C(11, 7, 3), C(7, 4, 2), C(4, 2, 1), C(8, 8, 10), C(3, 3, 5), C(14, 12, 7), C(15, 12, 5),
                           C(15, 15, 12), C(6, 6, 7), C(3, 2, 4), C(12, 8, 3), C(2, 1, 2), 0, 0, 0});
    setPal(vdp, PAL_FLOOR, {0, C(9, 9, 10), C(6, 6, 8), C(4, 4, 5), C(3, 3, 4), C(5, 5, 6), C(8, 8, 9), C(2, 2, 3),
                            C(4, 6, 4), C(12, 7, 3), C(11, 11, 12), C(7, 7, 8), C(9, 1, 2), C(6, 1, 2), C(12, 9, 3),
                            C(1, 1, 2)});

    gs::TileAlloc tiles(vdp);
    uint8_t px[64];
    for (int i = 0; i < 4; i++) {
        makeFloor(px, i);
        art.fl[i] = uploadTile(vdp, tiles, px);
    }
    for (int i = 0; i < 2; i++) {
        makeWall(px, i);
        art.wall[i] = uploadTile(vdp, tiles, px);
        makeCarpet(px, i);
        art.carpet[i] = uploadTile(vdp, tiles, px);
    }
    makeSkirt(px);
    art.skirt = uploadTile(vdp, tiles, px);
    makeHeart(px);
    art.heart = uploadTile(vdp, tiles, px);
    loadFont(vdp, tiles, art);

    for (int face = 0; face < 3; face++)
        for (int step = 0; step < 2; step++) art.knight[face][step] = gs::uploadMipped(vdp, knight(face, step));
    for (int step = 0; step < 2; step++) {
        art.grunt[step] = gs::uploadMipped(vdp, grunt(step));
        art.archer[step] = gs::uploadMipped(vdp, archer(step));
        art.warden[step] = gs::uploadMipped(vdp, warden(step));
        art.torch[step] = gs::uploadMipped(vdp, torchArt(step));
    }
    art.slash[0] = gs::uploadMipped(vdp, slashNorth());
    art.slash[1] = gs::uploadMipped(vdp, slashEast());
    art.slash[2] = gs::uploadMipped(vdp, slashSouth());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.door = gs::uploadMipped(vdp, doorClosed());
    art.doorOpen = gs::uploadMipped(vdp, doorOpened());
    art.key = gs::uploadMipped(vdp, keyArt());
    art.flask = gs::uploadMipped(vdp, flaskArt());
    art.pillar = gs::uploadMipped(vdp, pillarArt());
    art.skull = gs::uploadMipped(vdp, skullArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    (void)PI;
}

void paintHall(gs::VDP& vdp, const Art& art) {
    auto tileAt = [&](int cx, int cy) {
        if (cx <= 2 || cx >= 37) return art.wall[cy & 1];
        if (cx == 3 || cx == 36) return art.skirt;
        if (cx >= 17 && cx <= 22) return art.carpet[(cx + cy) & 1];
        uint32_t h = uint32_t(cx) * 0x9e3779b1u ^ uint32_t(cy) * 0x85ebca6bu;
        if ((h % 19u) == 0) return art.fl[3];
        return art.fl[h % 3u];
    };
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = cx >= 40 ? art.wall[cy & 1] : tileAt(cx, cy);
            vdp.B.set(cx, cy, gs::entry(tile, PAL_FLOOR));
        }
    }
    vdp.A.enabled = false;
    vdp.B.enabled = true;
}

}  // namespace gauntlet
