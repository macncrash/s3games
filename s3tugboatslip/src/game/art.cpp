#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace tugslip {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

// lx is starboard, ly is the bow. Heading 0 paints the bow toward screen east.
Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintTug(float heading) {
    Bitmap b(120, 120);
    const float cx = 60.f, cy = 60.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };

    poly({{0.f, 36.f}, {14.f, 26.f}, {16.f, 8.f}, {15.f, -8.f}, {12.f, -22.f}, {5.f, -28.f},
          {-5.f, -28.f}, {-12.f, -22.f}, {-15.f, -8.f}, {-16.f, 8.f}, {-14.f, 26.f}},
         2);
    poly({{0.f, 30.f}, {11.f, 22.f}, {12.f, 8.f}, {11.f, -6.f}, {9.f, -18.f}, {4.f, -23.f},
          {-4.f, -23.f}, {-9.f, -18.f}, {-11.f, -6.f}, {-12.f, 8.f}, {-11.f, 22.f}},
         1);
    poly({{0.f, 24.f}, {8.f, 16.f}, {8.f, 2.f}, {6.f, -12.f}, {3.f, -16.f},
          {-3.f, -16.f}, {-6.f, -12.f}, {-8.f, 2.f}, {-8.f, 16.f}},
         4);
    stroke(-12.f, 24.f, 12.f, 24.f, 6, 2.6f);
    stroke(-8.f, 18.f, 8.f, 18.f, 10, 1.5f);
    blob(0.f, 16.f, 2.2f, 2.2f, 10);
    poly({{-8.f, 4.f}, {8.f, 4.f}, {8.f, -12.f}, {-8.f, -12.f}}, 3);
    poly({{-7.f, 1.f}, {-1.f, 1.f}, {-1.f, -6.f}, {-7.f, -6.f}}, 7);
    poly({{1.f, 1.f}, {7.f, 1.f}, {7.f, -6.f}, {1.f, -6.f}}, 7);
    poly({{-4.5f, -14.f}, {4.5f, -14.f}, {4.5f, -26.f}, {-4.5f, -26.f}}, 5);
    poly({{-4.5f, -18.f}, {4.5f, -18.f}, {4.5f, -21.f}, {-4.5f, -21.f}}, 6);
    blob(0.f, -26.f, 3.2f, 2.2f, 6);
    stroke(-6.f, -4.f, 6.f, -4.f, 8, 1.4f);
    blob(0.f, -4.f, 1.5f, 1.5f, 8);
    blob(13.5f, 16.f, 2.4f, 3.1f, 6);
    blob(-13.5f, 16.f, 2.4f, 3.1f, 6);
    blob(13.2f, 2.f, 2.4f, 3.1f, 6);
    blob(-13.2f, 2.f, 2.4f, 3.1f, 6);
    blob(12.6f, -12.f, 2.3f, 3.0f, 6);
    blob(-12.6f, -12.f, 2.3f, 3.0f, 6);
    blob(13.5f, 16.f, 1.0f, 1.2f, 12);
    blob(-13.5f, 16.f, 1.0f, 1.2f, 12);
    blob(9.f, 30.f, 1.3f, 1.3f, 11);
    blob(-9.f, 30.f, 1.3f, 1.3f, 9);
    blob(-6.f, -8.f, 2.0f, 2.0f, 13);
    b.outline(15, false);
    return b;
}

Bitmap paintPier() {
    Bitmap b(40, 78);
    b.rect(4, 2, 32, 74, 1);
    for (int i = 0; i < 8; i++) b.rect(6, 4 + i * 9, 28, 7, (i & 1) ? 2 : 1);
    b.rect(3, 2, 3, 74, 4);
    b.rect(34, 2, 3, 74, 4);
    b.rect(8, 6, 2, 66, 3);
    b.rect(30, 6, 2, 66, 3);
    b.outline(5, false);
    return b;
}

Bitmap paintHead() {
    Bitmap b(168, 22);
    b.rect(2, 4, 164, 14, 1);
    for (int i = 0; i < 16; i++) b.rect(4 + i * 10, 6, 8, 10, (i & 1) ? 2 : 1);
    b.rect(2, 3, 164, 3, 4);
    b.rect(2, 16, 164, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintPile() {
    Bitmap b(20, 32);
    b.ellipse(10, 24, 7, 4, 2);
    b.rect(7, 8, 6, 16, 1);
    b.rect(8, 10, 2, 10, 5);
    b.ellipse(10, 8, 6, 3.5f, 3);
    b.ellipse(10, 7, 2.2f, 1.6f, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintFender() {
    Bitmap b(16, 22);
    b.ellipse(8, 12, 6, 7, 1);
    b.ellipse(8, 12, 3, 4, 2);
    b.rect(7, 2, 2, 6, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintCleat() {
    Bitmap b(18, 12);
    b.rect(2, 5, 14, 3, 1);
    b.rect(3, 2, 3, 8, 2);
    b.rect(12, 2, 3, 8, 2);
    b.ellipse(9, 6, 2, 2, 3);
    return b;
}

Bitmap paintNun() {
    Bitmap b(22, 34);
    b.poly({{11, 4}, {18, 16}, {15, 26}, {7, 26}, {4, 16}}, 1);
    b.poly({{11, 8}, {15, 16}, {13, 22}, {9, 22}, {7, 16}}, 2);
    b.rect(10, 2, 2, 5, 3);
    b.ellipse(11, 28, 3, 2, 4);
    b.outline(3, false);
    return b;
}

Bitmap paintCan() {
    Bitmap b(22, 32);
    b.rect(5, 10, 12, 14, 5);
    b.rect(6, 12, 10, 5, 2);
    b.rect(10, 3, 2, 8, 3);
    b.ellipse(11, 26, 5, 3, 4);
    b.outline(3, false);
    return b;
}

Bitmap paintBarge() {
    Bitmap b(86, 36);
    b.ellipse(43, 28, 36, 6, 5);
    b.rect(6, 8, 74, 16, 1);
    b.rect(10, 11, 28, 10, 2);
    b.rect(42, 11, 30, 10, 3);
    b.rect(8, 8, 70, 3, 4);
    b.outline(6, false);
    return b;
}

Bitmap paintBoat() {
    Bitmap b(40, 18);
    b.poly({{4, 10}, {12, 4}, {30, 4}, {36, 10}, {30, 14}, {12, 14}}, 1);
    b.rect(16, 6, 10, 5, 2);
    b.ellipse(10, 9, 2, 2, 3);
    b.outline(6, false);
    return b;
}

Bitmap paintShed() {
    Bitmap b(64, 48);
    b.ellipse(32, 36, 24, 8, 5);
    b.rect(10, 18, 44, 18, 1);
    b.poly({{8, 20}, {32, 6}, {56, 20}}, 2);
    b.rect(28, 24, 8, 12, 3);
    b.rect(16, 22, 7, 6, 4);
    b.rect(42, 22, 7, 6, 4);
    b.outline(6, false);
    return b;
}

Bitmap paintCrane() {
    Bitmap b(78, 78);
    b.rect(8, 48, 22, 16, 1);
    b.rect(12, 52, 8, 6, 4);
    b.line(24, 52, 68, 16, 2, 3.0f);
    b.line(24, 56, 64, 22, 3, 1.6f);
    b.ellipse(68, 16, 3, 3, 5);
    b.line(68, 18, 68, 30, 3, 1.2f);
    b.rect(64, 30, 8, 3, 5);
    b.outline(6, false);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(24, 22);
    b.ellipse(12, 14, 8, 5, 1);
    b.ellipse(8, 11, 4, 4, 2);
    b.ellipse(16, 10, 4, 4, 3);
    b.line(7, 14, 5, 4, 2, 1.3f);
    b.line(12, 15, 12, 3, 2, 1.3f);
    b.line(17, 14, 19, 5, 3, 1.3f);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(30, 16);
    b.ellipse(15, 9, 3, 2, 1);
    float tip = up ? 2.f : 12.f;
    b.line(15, 8, 2, tip, 1, 1.5f);
    b.line(15, 8, 28, tip, 1, 1.5f);
    b.set(19, 8, 3);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 3.5f, 1);
    b.ellipse(5, 6, 2.5f, 2, 2);
    b.ellipse(13, 5, 2.2f, 1.8f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 5, 1);
    b.ellipse(6, 7, 3, 2.5f, 2);
    b.ellipse(11, 8, 2.5f, 2, 2);
    return b;
}

Bitmap paintEbb() {
    Bitmap b(16, 10);
    b.poly({{2, 2}, {9, 5}, {2, 8}}, 1);
    b.poly({{7, 2}, {14, 5}, {7, 8}}, 2);
    return b;
}

Bitmap paintStaff() {
    Bitmap b(12, 48);
    b.rect(5, 2, 2, 42, 3);
    for (int i = 0; i < 8; i++) b.rect(3, 4 + i * 5, 6, 2, (i & 1) ? 1 : 2);
    b.rect(2, 42, 8, 4, 4);
    return b;
}

Bitmap paintBobber() {
    Bitmap b(12, 12);
    b.poly({{6, 1}, {11, 6}, {6, 11}, {1, 6}}, 1);
    b.poly({{6, 4}, {8, 6}, {6, 8}, {4, 6}}, 2);
    return b;
}

Bitmap paintDash() {
    Bitmap b(10, 4);
    b.rect(0, 1, 10, 2, 1);
    return b;
}

Bitmap paintPin() {
    Bitmap b(12, 12);
    b.poly({{6, 1}, {11, 6}, {6, 11}, {1, 6}}, 1);
    b.poly({{6, 4}, {8, 6}, {6, 8}, {4, 6}}, 2);
    return b;
}

Bitmap paintMark() {
    Bitmap b(18, 18);
    b.poly({{9, 1}, {17, 9}, {9, 17}, {1, 9}}, 1);
    b.poly({{9, 5}, {13, 9}, {9, 13}, {5, 9}}, 2);
    b.rect(8, 8, 2, 2, 3);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(72, 86);
    b.rect(0, 0, 72, 86, 3);
    for (int x = 0; x < 72; x++) {
        b.set(x, 0, 1);
        b.set(x, 85, 1);
    }
    for (int y = 0; y < 86; y++) {
        b.set(0, y, 1);
        b.set(71, y, 1);
    }
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 9, 10), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(12, 10, 7), gs::rgb4(15, 12, 2),
            gs::rgb4(1, 1, 2), gs::rgb4(5, 9, 13), gs::rgb4(15, 14, 12), gs::rgb4(15, 3, 2), gs::rgb4(10, 6, 3),
            gs::rgb4(3, 12, 5), gs::rgb4(5, 5, 6), gs::rgb4(14, 3, 3), gs::rgb4(15, 14, 6), ink});
    setPal(vdp, PAL_PIER,
           {0, gs::rgb4(11, 8, 5), gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PILE,
           {0, gs::rgb4(9, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(12, 10, 8), gs::rgb4(15, 13, 5), gs::rgb4(13, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 9, 10), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TIDE, {0, gs::rgb4(15, 6, 2), gs::rgb4(15, 13, 6), gs::rgb4(4, 3, 2), gs::rgb4(6, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUOY,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 4), gs::rgb4(2, 11, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(10, 6, 3), gs::rgb4(7, 4, 3), gs::rgb4(4, 7, 9), gs::rgb4(12, 11, 9), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2),
            gs::rgb4(8, 10, 5), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(8, 8, 9), gs::rgb4(13, 13, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SLIP,
           {0, gs::rgb4(4, 7, 6), gs::rgb4(2, 5, 5), gs::rgb4(6, 8, 7), gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(3, 8, 10),
            gs::rgb4(2, 6, 8), gs::rgb4(7, 8, 8), 0, 0, gs::rgb4(1, 4, 6), gs::rgb4(2, 6, 9), gs::rgb4(8, 12, 13), 0, ink});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(6, 8, 4), gs::rgb4(4, 6, 3), gs::rgb4(8, 8, 5), gs::rgb4(7, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(10, 8, 5),
            gs::rgb4(7, 5, 3), gs::rgb4(5, 5, 4), gs::rgb4(8, 7, 5), gs::rgb4(12, 10, 7), 0, 0, 0, gs::rgb4(9, 8, 6), gs::rgb4(3, 3, 2), ink});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(15, 13, 7), gs::rgb4(3, 2, 4), gs::rgb4(1, 2, 5), gs::rgb4(6, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(i * kTau / 16.f));
    art.pier = gs::uploadMipped(vdp, paintPier());
    art.head = gs::uploadMipped(vdp, paintHead());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.fender = gs::uploadMipped(vdp, paintFender());
    art.cleat = gs::uploadMipped(vdp, paintCleat());
    art.nun = gs::uploadMipped(vdp, paintNun());
    art.can = gs::uploadMipped(vdp, paintCan());
    art.barge = gs::uploadMipped(vdp, paintBarge());
    art.boat = gs::uploadMipped(vdp, paintBoat());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.crane = gs::uploadMipped(vdp, paintCrane());
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.ebb = gs::uploadMipped(vdp, paintEbb());
    art.staff = gs::uploadMipped(vdp, paintStaff());
    art.bobber = gs::uploadMipped(vdp, paintBobber());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.mark = gs::uploadMipped(vdp, paintMark());
    Bitmap dot(6, 6);
    dot.ellipse(3, 3, 2.2f, 2.2f, 1);
    art.dot = gs::uploadMipped(vdp, dot);
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = words(vdp, "TUGBOAT SLIP", 3);
    art.berthed = words(vdp, "BERTHED", 3);
    art.inSlip = words(vdp, "IN THE SLIP", 2);
    art.tide = words(vdp, "TIDE TURNED", 2);
    art.headMsg = words(vdp, "HIT THE HEAD", 2);
    art.scraped = words(vdp, "SCRAPED A PILE", 2);
    art.past = words(vdp, "PAST THE BERTH", 2);
    art.shortMsg = words(vdp, "SHORT OF THE BERTH", 2);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(6, 8, 10));
}

}  // namespace tugslip
