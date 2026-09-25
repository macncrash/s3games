#include "game/art.h"

#include "game/course.h"

#include <cstdint>
#include <initializer_list>

namespace golf {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    bool wroteShadow = i > 15;
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    if (!wroteShadow) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

int rowOf(float worldY) { return kBaseRow - int(std::lround(worldY)); }

void paint(gs::Bitmap& b, int x, int y, int c) {
    if (c <= 0) return;
    b.set(x, y, c);
}

void column(gs::Bitmap& b, int x, int top, int bot, int c) {
    if (top < 0) top = 0;
    if (bot >= b.h) bot = b.h - 1;
    for (int y = top; y <= bot; y++) paint(b, x, y, c);
}

int hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 0x8da6b343u ^ uint32_t(y) * 0xd8163841u;
    h ^= h >> 13;
    return int(h);
}

void drawGround(gs::Bitmap& b, const Hole& h) {
    for (int x = 0; x < b.w; x++) {
        float wx = float(x) + 0.5f;
        bool mouth = std::fabs(wx - h.cup) <= h.cupHalf;
        if (mouth) {
            int lip = rowOf(h.lip);
            int floor = rowOf(h.lip - h.cupDepth);
            if (floor < lip) std::swap(floor, lip);
            column(b, x, lip, floor, 12);
            column(b, x, floor + 1, b.h - 1, 13);
            if (x == int(std::lround(h.cup - h.cupHalf)) || x == int(std::lround(h.cup + h.cupHalf)))
                column(b, x, lip - 1, lip + 2, 6);
            continue;
        }
        Surf s = groundAt(h, wx, false);
        if (s.kind == KIND_WATER) {
            int surface = rowOf(kWaterY);
            for (int y = surface; y < b.h; y++) {
                bool hi = ((y / 3 + x / 5) & 1) == 0;
                paint(b, x, y, hi ? 11 : 10);
            }
            if (((x / 4) & 1) == 0) paint(b, x, surface, 11);
            continue;
        }
        int top = rowOf(s.y);
        int body = 1;
        int mid = 2;
        int cap = 3;
        if (s.kind == KIND_GREEN) {
            body = 4;
            mid = 5;
            cap = 6;
        } else if (s.kind == KIND_SAND) {
            body = 9;
            mid = 8;
            cap = 8;
        } else if (s.kind == KIND_ROUGH) {
            body = 7;
            mid = 7;
            cap = 9;
        }
        for (int y = top; y < b.h; y++) {
            int c = (y > top + 5) ? body : mid;
            if ((hash2(x, y) & 7) == 0) c = body;
            paint(b, x, y, c);
        }
        bool stripe = s.kind == KIND_GREEN ? (((x / 4) & 1) == 0) : (((x / 8) & 1) == 0);
        paint(b, x, top, stripe ? cap : mid);
        if (top + 1 < b.h) paint(b, x, top + 1, stripe ? mid : cap);
    }
}

void drawTree(gs::Bitmap& b, int x, int groundRow) {
    if (x < 8 || x >= b.w - 8) return;
    b.rect(float(x - 1), float(groundRow - 14), 3, 14, 15);
    b.ellipse(float(x), float(groundRow - 18), 11, 9, 14);
    b.ellipse(float(x - 4), float(groundRow - 16), 6, 5, 5);
    b.ellipse(float(x + 3), float(groundRow - 20), 5, 4, 6);
}

void drawTrees(gs::Bitmap& b, const Hole& h) {
    const float spots[] = {70, 118, 168, 214};
    for (float sx : spots) {
        if (std::fabs(sx - h.cup) < 36.f || std::fabs(sx - h.tee) < 28.f) continue;
        Surf s = groundAt(h, sx, false);
        if (s.kind != KIND_FAIR && s.kind != KIND_ROUGH) continue;
        if (s.y > 36.f) continue;
        drawTree(b, int(sx), rowOf(s.y));
    }
}

void drawTee(gs::Bitmap& b, const Hole& h) {
    int x = int(std::lround(h.tee));
    int y = rowOf(groundAt(h, h.tee, false).y);
    b.rect(float(x - 3), float(y - 3), 2, 3, 6);
    b.rect(float(x + 2), float(y - 3), 2, 3, 13);
}

gs::Bitmap ballArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.2f, 6.2f, 4);
    b.ellipse(7, 7, 5.2f, 5.2f, 3);
    b.ellipse(6.6f, 6.4f, 4.4f, 4.4f, 1);
    b.ellipse(4.6f, 4.6f, 1.6f, 1.2f, 2);
    b.set(8, 8, 3);
    b.set(9, 6, 5);
    b.set(6, 9, 5);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(14, 6);
    b.ellipse(7, 3, 6, 2.2f, 1);
    return b;
}

gs::Bitmap flagArt(int frame) {
    gs::Bitmap b(18, 28);
    b.rect(3, 2, 2, 24, 1);
    b.rect(2, 25, 4, 2, 4);
    float tip = frame ? 15.5f : 16.8f;
    float mid = frame ? 7.5f : 9.2f;
    b.poly({{5, 3}, {tip, mid}, {5, 14}}, 2);
    b.poly({{5, 5}, {tip - 2.4f, mid}, {5, 11}}, 3);
    return b;
}

gs::Bitmap golferArt(int swing) {
    gs::Bitmap b(24, 32);
    b.ellipse(13, 6, 4.2f, 4.0f, 1);
    b.ellipse(13, 4.5f, 4.4f, 2.2f, 6);
    b.rect(15, 4, 5, 2, 7);
    b.rect(11, 10, 6, 8, 2);
    b.rect(10, 12, 2, 5, 2);
    b.rect(12, 18, 3, 8, 3);
    b.rect(16, 18, 3, 8, 3);
    b.rect(11, 26, 4, 2, 4);
    b.rect(16, 26, 4, 2, 4);
    if (!swing) {
        b.line(16, 13, 22, 27, 5, 1);
        b.set(22, 27, 8);
    } else {
        b.line(14, 12, 4, 8, 5, 1);
        b.line(4, 8, 20, 6, 5, 1);
        b.set(20, 6, 8);
    }
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(18, 6, 8, 4.2f, 1);
    b.ellipse(14, 5, 6, 3.4f, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(7.4f, 7.2f, 3, 3, 2);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 2.0f, 2.0f, 1);
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

gs::Bitmap rasterHole(const Hole& h) {
    gs::Bitmap b(std::max(8, int(std::lround(h.length))), kViewH);
    drawGround(b, h);
    drawTrees(b, h);
    drawTee(b, h);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_WHITE, {0, ink, gs::rgb4(8, 8, 10)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 5, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 7), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 15, 15), gs::rgb4(9, 10, 11), gs::rgb4(3, 3, 4),
                           gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(14, 14, 12), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(14, 10, 7), gs::rgb4(15, 15, 15), gs::rgb4(2, 3, 8), gs::rgb4(2, 2, 2),
                          gs::rgb4(10, 11, 12), gs::rgb4(4, 3, 2), gs::rgb4(12, 2, 2), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_WORLD,
           {0, gs::rgb4(2, 7, 3), gs::rgb4(3, 11, 4), gs::rgb4(7, 14, 6), gs::rgb4(2, 9, 4), gs::rgb4(4, 13, 5),
            gs::rgb4(9, 15, 8), gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 7), gs::rgb4(10, 8, 4), gs::rgb4(2, 5, 11),
            gs::rgb4(6, 12, 15), gs::rgb4(1, 1, 2), gs::rgb4(11, 9, 6), gs::rgb4(1, 6, 2), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(13, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 1), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_SHADOW, {0, gs::rgb4(1, 2, 1)});

    loadFont(vdp, art);
    for (int i = 0; i < kHoles; i++) art.course[i] = gs::uploadImage(vdp, rasterHole(holeAt(i)));
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.flag[0] = gs::uploadMipped(vdp, flagArt(0));
    art.flag[1] = gs::uploadMipped(vdp, flagArt(1));
    art.golfer[0] = gs::uploadMipped(vdp, golferArt(0));
    art.golfer[1] = gs::uploadMipped(vdp, golferArt(1));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 GOLF", {3, 1, 2, 0, 1}));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("IN THE HOLE", {2, 3, 2, 0, 1}));
}

}  // namespace golf
