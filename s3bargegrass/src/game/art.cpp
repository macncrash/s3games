#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace bargegrass {
namespace {

using gs::Bitmap;
using gs::Pt;

constexpr float kScale = 3.15f;

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

// Long cargo barge, bow at local +x. Heading 0 paints the bow to the right.
Bitmap paintBarge(float heading) {
    Bitmap b(168, 96);
    const float cx = 84.f, cy = 48.f;
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

    polyB({{9.1f, 0.f}, {7.6f, 2.7f}, {-8.2f, 3.05f}, {-9.0f, 2.2f}, {-9.0f, -2.2f}, {-8.2f, -3.05f}, {7.6f, -2.7f}}, 2);
    polyB({{8.7f, 0.f}, {7.2f, 2.35f}, {-7.8f, 2.65f}, {-8.55f, 1.9f}, {-8.55f, -1.9f}, {-7.8f, -2.65f}, {7.2f, -2.35f}}, 12);
    polyB({{8.3f, 0.f}, {6.8f, 2.0f}, {-7.4f, 2.25f}, {-8.1f, 1.55f}, {-8.1f, -1.55f}, {-7.4f, -2.25f}, {6.8f, -2.0f}}, 3);
    polyB({{5.4f, -1.55f}, {5.4f, 1.55f}, {1.6f, 1.55f}, {1.6f, -1.55f}}, 8);
    polyB({{5.1f, -1.25f}, {5.1f, 1.25f}, {1.9f, 1.25f}, {1.9f, -1.25f}}, 9);
    polyB({{0.6f, -1.45f}, {0.6f, 1.45f}, {-2.6f, 1.45f}, {-2.6f, -1.45f}}, 10);
    polyB({{0.3f, -1.15f}, {0.3f, 1.15f}, {-2.3f, 1.15f}, {-2.3f, -1.15f}}, 11);
    polyB({{-3.4f, -1.35f}, {-3.4f, 1.35f}, {-6.4f, 1.35f}, {-6.4f, -1.35f}}, 4);
    polyB({{-3.7f, -1.05f}, {-3.7f, 1.05f}, {-6.1f, 1.05f}, {-6.1f, -1.05f}}, 5);
    polyB({{-4.2f, -0.55f}, {-4.2f, 0.55f}, {-5.6f, 0.55f}, {-5.6f, -0.55f}}, 6);
    blob(-7.4f, 0.f, 4.2f, 2.6f, 7);
    blob(8.2f, 0.f, 3.4f, 2.2f, 1);
    blob(7.2f, 1.5f, 2.2f, 1.6f, 14);
    blob(7.2f, -1.5f, 2.2f, 1.6f, 14);
    b.outline(15, false);
    return b.cropToContent(1);
}

Bitmap paintShade() {
    Bitmap b(48, 16);
    b.ellipse(24, 8, 22, 6, 1);
    return b;
}

Bitmap paintWall() {
    Bitmap b(18, 36);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x, y);
            int c = 2;
            if (x > 12) c = 1;
            if (x > 15) c = 4;
            if ((h & 11) == 0) c = x > 12 ? 5 : 3;
            b.set(x, y, c);
        }
    }
    return b;
}

Bitmap paintShed() {
    Bitmap b(40, 22);
    b.rect(2, 8, 36, 12, 2);
    b.poly({{1, 8}, {20, 2}, {39, 8}}, 3);
    b.rect(6, 11, 8, 6, 4);
    b.rect(22, 11, 10, 4, 5);
    b.outline(7, false);
    return b.cropToContent(0);
}

Bitmap paintBulk() {
    Bitmap b(56, 14);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) b.set(x, y, (y < 2 || y > 11) ? 1 : ((x / 5) & 1) ? 3 : 2);
    return b;
}

Bitmap paintPost() {
    Bitmap b(10, 24);
    b.rect(4, 5, 2, 16, 2);
    b.ellipse(5, 4, 3.f, 2.6f, 1);
    b.rect(4, 20, 2, 4, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(8, 18);
    b.rect(3, 7, 2, 10, 2);
    b.ellipse(4, 4, 3.f, 3.f, 1);
    return b;
}

Bitmap paintBuoy(bool green) {
    Bitmap b(12, 18);
    b.ellipse(6, 7, 4, 5, green ? 4 : 1);
    b.rect(4, 3, 4, 10, 2);
    b.rect(5, 13, 2, 4, 3);
    return b;
}

Bitmap paintReed() {
    Bitmap b(14, 26);
    b.line(3, 24, 2, 5, 1, 1.3f);
    b.line(7, 24, 8, 2, 2, 1.4f);
    b.line(11, 24, 12, 7, 1, 1.2f);
    b.ellipse(8, 3, 2.2f, 1.5f, 3);
    return b;
}

Bitmap paintTuft() {
    Bitmap b(18, 14);
    b.ellipse(9, 9, 7, 4, 2);
    b.ellipse(6, 7, 3, 3, 1);
    b.ellipse(13, 6, 3, 3, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(14, 20);
    b.rect(2, 3, 2, 16, 2);
    b.poly({{4, 4}, {12, 7}, {4, 11}}, 1);
    return b;
}

Bitmap paintDash() {
    Bitmap b(10, 4);
    b.rect(0, 1, 10, 2, 1);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 8);
    b.ellipse(8, 4, 7, 3, 1);
    b.ellipse(8, 4, 3, 1.2f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(12, 10);
    b.ellipse(6, 5, 5, 3.5f, 1);
    return b;
}

Bitmap paintGull(int flap) {
    Bitmap b(18, 10);
    float tip = flap ? 2.f : 7.f;
    b.line(1, tip, 8, 5, 1, 1.3f);
    b.line(17, tip, 10, 5, 1, 1.3f);
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
    Bitmap b(48, 58);
    b.rect(1, 1, 46, 56, 1);
    b.rect(3, 3, 42, 52, 2);
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
    c[1] = lip ? gs::rgb4(9, 13, 4) : gs::rgb4(3, 10, 2);
    c[2] = lip ? gs::rgb4(6, 11, 3) : gs::rgb4(2, 7, 2);
    c[3] = gs::rgb4(10, 14, 5);
    c[4] = lip ? gs::rgb4(7, 12, 4) : gs::rgb4(4, 9, 2);
    c[5] = gs::rgb4(2, 6, 2);
    c[6] = gs::rgb4(11, 14, 6);
    c[7] = gs::rgb4(5, 10, 3);
    c[8] = gs::rgb4(8, 12, 4);
    c[9] = gs::rgb4(2, 8, 2);
    c[10] = gs::rgb4(12, 14, 7);
    c[11] = gs::rgb4(2, 7, 10);
    c[12] = gs::rgb4(1, 4, 7);
    c[13] = gs::rgb4(9, 14, 13);
    c[14] = gs::rgb4(13, 14, 6);
    c[15] = gs::rgb4(4, 9, 3);
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 11), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(2, 2, 3), gs::rgb4(5, 6, 7), gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 5),
            gs::rgb4(6, 12, 13), gs::rgb4(3, 3, 3), gs::rgb4(7, 4, 2), gs::rgb4(11, 7, 3), gs::rgb4(8, 5, 2),
            gs::rgb4(13, 10, 5), gs::rgb4(4, 5, 6), gs::rgb4(8, 8, 7), gs::rgb4(14, 12, 4), ink});
    setPal(vdp, PAL_BANK,
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
    setPal(vdp, PAL_CARGO,
           {0, gs::rgb4(14, 4, 2), gs::rgb4(5, 4, 3), gs::rgb4(14, 13, 8), gs::rgb4(2, 8, 3), gs::rgb4(12, 5, 2),
            gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, ink});
    fieldPal(vdp, PAL_FIELD, false);
    fieldPal(vdp, PAL_LIP, true);

    loadFont(vdp, art);
    const float tau = 6.28318530718f;
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintBarge(i * tau / 16.f));
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.wall = gs::uploadMipped(vdp, paintWall());
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
    art.title = words(vdp, "BARGE GRASS", 3);
    art.fullStop = words(vdp, "FULL STOP", 3);
    art.onGrass = words(vdp, "ON THE GRASS", 2);
    art.ranOff = words(vdp, "MISSED THE END", 2);
    art.offGrass = words(vdp, "OFF THE GRASS", 2);
    art.shortStop = words(vdp, "SHORT OF THE END", 2);
    art.notFull = words(vdp, "NOT ON THE END", 2);
    art.timed = words(vdp, "TIMED OUT", 2);
    art.legFail = words(vdp, "THE LEG FAILS", 2);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(1, 4, 7));
}

}  // namespace bargegrass
