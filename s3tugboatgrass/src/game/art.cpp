#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace tuggrass {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kScale = 4.2f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// Harbor tug, bow at local +x. Heading 0 paints the bow to the right.
Bitmap paintTug(float heading) {
    Bitmap b(148, 148);
    const float cx = 74.f, cy = 74.f;
    const float co = std::cos(heading), sn = std::sin(heading);
    auto polyB = [&](std::initializer_list<Pt> meters, int col) {
        std::vector<Pt> p;
        p.reserve(meters.size());
        for (const Pt& m : meters) {
            float dx = (m.first * co + m.second * sn) * kScale;
            float dy = (-m.first * sn + m.second * co) * kScale;
            p.push_back({cx + dx, cy + dy});
        }
        b.poly(p, col);
    };
    auto blob = [&](float bx, float by, float rx, float ry, int col) {
        float dx = (bx * co + by * sn) * kScale;
        float dy = (-bx * sn + by * co) * kScale;
        b.ellipse(cx + dx, cy + dy, rx, ry, col);
    };

    polyB({{6.7f, 0.f}, {4.9f, 2.55f}, {-5.4f, 2.85f}, {-6.5f, 1.9f}, {-6.5f, -1.9f}, {-5.4f, -2.85f}, {4.9f, -2.55f}}, 2);
    polyB({{6.35f, 0.f}, {4.65f, 2.25f}, {-5.15f, 2.5f}, {-6.15f, 1.65f}, {-6.15f, -1.65f}, {-5.15f, -2.5f}, {4.65f, -2.25f}}, 12);
    polyB({{6.05f, 0.f}, {4.4f, 2.0f}, {-4.9f, 2.2f}, {-5.85f, 1.45f}, {-5.85f, -1.45f}, {-4.9f, -2.2f}, {4.4f, -2.0f}}, 3);
    polyB({{5.55f, 0.f}, {4.05f, 1.55f}, {-4.7f, 1.75f}, {-5.5f, 1.15f}, {-5.5f, -1.15f}, {-4.7f, -1.75f}, {4.05f, -1.55f}}, 4);
    polyB({{2.55f, -1.35f}, {2.55f, 1.35f}, {-0.35f, 1.35f}, {-0.35f, -1.35f}}, 1);
    polyB({{2.25f, -1.05f}, {2.25f, 1.05f}, {-0.05f, 1.05f}, {-0.05f, -1.05f}}, 11);
    polyB({{2.15f, -0.72f}, {1.15f, -0.72f}, {1.15f, 0.72f}, {2.15f, 0.72f}}, 5);
    polyB({{-0.15f, -0.42f}, {-1.05f, -0.42f}, {-1.05f, 0.42f}, {-0.15f, 0.42f}}, 7);
    blob(-1.55f, 0.f, 7.4f, 5.6f, 6);
    blob(-1.55f, 0.f, 4.6f, 3.4f, 2);
    blob(5.7f, 0.f, 5.2f, 3.6f, 2);
    blob(4.5f, 1.55f, 3.3f, 2.6f, 11);
    blob(4.5f, -1.55f, 3.3f, 2.6f, 11);
    blob(-5.15f, 1.15f, 2.5f, 2.2f, 9);
    blob(-5.15f, -1.15f, 2.5f, 2.2f, 9);
    blob(-5.7f, 0.f, 2.2f, 1.8f, 14);
    polyB({{-3.3f, 0.16f}, {-4.7f, 0.16f}, {-4.7f, -0.16f}, {-3.3f, -0.16f}}, 2);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(40, 22);
    b.ellipse(20, 11, 18, 8, 1);
    return b;
}

Bitmap paintQuay() {
    Bitmap b(22, 40);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 2;
            if (x > 15) c = 1;
            if (x > 18) c = 4;
            if ((h & 11) == 0) c = x > 15 ? 5 : 3;
            b.set(x, y, c);
        }
    }
    b.rect(4, 16, 8, 5, 6);
    return b;
}

Bitmap paintShed() {
    Bitmap b(36, 26);
    b.rect(2, 9, 32, 15, 2);
    b.poly({{1, 9}, {18, 2}, {35, 9}}, 3);
    b.rect(6, 13, 7, 7, 4);
    b.rect(20, 12, 8, 5, 5);
    b.rect(15, 16, 5, 8, 6);
    b.outline(7, false);
    return b.cropToContent(0);
}

Bitmap paintBulk() {
    Bitmap b(48, 16);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, (y < 3 || y > 12) ? 1 : ((x / 4) & 1) ? 3 : 2);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 26);
    b.rect(5, 6, 3, 18, 2);
    b.rect(3, 8, 6, 3, 1);
    b.ellipse(6, 4, 3.5f, 3.f, 1);
    b.rect(5, 22, 2, 4, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(10, 20);
    b.rect(4, 8, 2, 12, 2);
    b.ellipse(5, 5, 3.5f, 3.5f, 1);
    b.ellipse(5, 5, 1.8f, 1.8f, 3);
    return b;
}

Bitmap paintBuoy(bool green) {
    Bitmap b(14, 20);
    b.ellipse(7, 8, 5, 6, green ? 4 : 1);
    b.rect(5, 3, 4, 11, 2);
    b.ellipse(7, 8, 2.f, 2.8f, green ? 5 : 3);
    b.rect(6, 14, 2, 5, 3);
    return b;
}

Bitmap paintReed() {
    Bitmap b(16, 28);
    b.line(4, 26, 3, 6, 1, 1.4f);
    b.line(8, 26, 9, 3, 2, 1.5f);
    b.line(12, 26, 13, 8, 1, 1.3f);
    b.ellipse(3, 6, 2.2f, 1.6f, 3);
    b.ellipse(9, 3, 2.4f, 1.7f, 3);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(22, 18);
    b.ellipse(11, 12, 8, 5, 2);
    b.ellipse(7, 9, 4, 4, 1);
    b.ellipse(15, 8, 3.5f, 3.5f, 3);
    b.line(6, 12, 4, 4, 1, 1.2f);
    b.line(11, 12, 11, 3, 1, 1.2f);
    b.line(16, 12, 18, 5, 3, 1.2f);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(16, 22);
    b.rect(3, 4, 2, 17, 2);
    b.poly({{5, 5}, {14, 8}, {5, 12}}, 1);
    b.poly({{5, 6}, {12, 8}, {5, 11}}, 3);
    return b;
}

Bitmap paintDash() {
    Bitmap b(10, 4);
    b.rect(0, 1, 10, 2, 1);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 3.5f, 1);
    b.ellipse(8, 5, 3, 1.4f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(14, 12);
    b.ellipse(7, 6, 6, 4.5f, 1);
    b.ellipse(6, 5, 3, 2.2f, 2);
    return b;
}

Bitmap paintGull(int flap) {
    Bitmap b(20, 12);
    float tip = flap ? 2.f : 8.f;
    b.line(2, tip, 9, 6, 1, 1.4f);
    b.line(18, tip, 11, 6, 1, 1.4f);
    b.ellipse(10, 6, 1.6f, 1.2f, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(9, 9);
    b.poly({{4, 0}, {8, 4}, {4, 8}, {0, 4}}, 1);
    return b;
}

Bitmap paintDot() {
    Bitmap b(5, 5);
    b.ellipse(2.5f, 2.5f, 2.f, 2.f, 1);
    return b;
}

Bitmap paintPanel() {
    Bitmap b(52, 64);
    b.rect(1, 1, 50, 62, 1);
    b.rect(3, 3, 46, 58, 2);
    b.outline(3, false);
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

void fieldPal(gs::VDP& vdp, int pal, bool lip) {
    uint16_t c[16] = {};
    c[1] = lip ? gs::rgb4(8, 13, 4) : gs::rgb4(4, 11, 3);
    c[2] = lip ? gs::rgb4(5, 10, 3) : gs::rgb4(2, 8, 2);
    c[3] = gs::rgb4(9, 13, 5);
    c[4] = lip ? gs::rgb4(7, 12, 4) : gs::rgb4(3, 9, 2);
    c[5] = gs::rgb4(2, 6, 2);
    c[6] = lip ? gs::rgb4(10, 14, 5) : gs::rgb4(5, 12, 3);
    c[7] = lip ? gs::rgb4(6, 11, 3) : gs::rgb4(3, 9, 2);
    c[8] = gs::rgb4(7, 12, 4);
    c[9] = gs::rgb4(2, 7, 2);
    c[10] = lip ? gs::rgb4(12, 14, 6) : gs::rgb4(6, 13, 4);
    c[11] = gs::rgb4(2, 8, 11);
    c[12] = gs::rgb4(1, 5, 8);
    c[13] = gs::rgb4(8, 14, 14);
    c[14] = gs::rgb4(12, 14, 7);
    c[15] = gs::rgb4(4, 10, 3);
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 11), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(13, 10, 6), gs::rgb4(6, 13, 14),
            gs::rgb4(14, 11, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 8), gs::rgb4(8, 5, 2), gs::rgb4(15, 15, 15),
            gs::rgb4(4, 4, 5), gs::rgb4(6, 1, 1), gs::rgb4(3, 6, 3), gs::rgb4(14, 12, 4), ink});
    setPal(vdp, PAL_QUAY,
           {0, gs::rgb4(10, 10, 9), gs::rgb4(6, 6, 6), gs::rgb4(8, 4, 3), gs::rgb4(4, 5, 6), gs::rgb4(12, 12, 11),
            gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 8), gs::rgb4(3, 3, 2), gs::rgb4(12, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SMOKE, {0, gs::rgb4(12, 12, 12), gs::rgb4(6, 6, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 6), gs::rgb4(4, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_REED, {0, gs::rgb4(6, 12, 3), gs::rgb4(3, 8, 2), gs::rgb4(9, 14, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(8, 14, 4), gs::rgb4(3, 9, 2), gs::rgb4(11, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(14, 3, 2), gs::rgb4(5, 4, 3), gs::rgb4(14, 13, 8), gs::rgb4(2, 8, 3), gs::rgb4(12, 4, 2),
            gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, ink});
    fieldPal(vdp, PAL_FIELD, false);
    fieldPal(vdp, PAL_LIP, true);

    loadFont(vdp, art);
    const float tau = 6.28318530718f;
    for (int i = 0; i < 16; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(i * tau / 16.f));
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.quay = gs::uploadMipped(vdp, paintQuay());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.bulk = gs::uploadMipped(vdp, paintBulk());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.buoyR = gs::uploadMipped(vdp, paintBuoy(false));
    art.buoyG = gs::uploadMipped(vdp, paintBuoy(true));
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(0));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(1));
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.panel = gs::uploadMipped(vdp, paintPanel());
    art.title = words(vdp, "TUGBOAT GRASS", 3);
    art.fullStop = words(vdp, "FULL STOP", 3);
    art.onGrass = words(vdp, "ON THE GRASS", 2);
    art.ranOff = words(vdp, "RAN OFF", 3);
    art.offGrass = words(vdp, "OFF THE GRASS", 2);
    art.shortStop = words(vdp, "SHORT", 3);
    art.notFull = words(vdp, "NOT FULLY ON", 2);
    art.timed = words(vdp, "TIMED OUT", 2);
    art.legFail = words(vdp, "THE LEG FAILS", 2);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(1, 4, 7));
}

}  // namespace tuggrass
