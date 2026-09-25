#include "pictures.h"

#include <cmath>
#include <cstring>

namespace row {
namespace {

using gs::Bitmap;
using gs::Pt;
using gs::rgb4;

constexpr int HULL = 1, HULL_D = 2, SHIRT = 3, SKIN = 4, HAIR = 5, SHORTS = 6, WHITE = 7;
constexpr int SHAFT = 8, BLADE = 9, METAL = 10, SEAT = 11, INK = 13;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void oar(Bitmap& b, float px, float py, float ang, bool feather) {
    float ca = std::cos(ang), sa = std::sin(ang);
    float nx = -sa, ny = ca;
    b.line(px - ca * 11.f, py - sa * 11.f, px + ca * 30.f, py + sa * 30.f, SHAFT, 2.1f);
    float bw = feather ? 1.4f : 4.4f;
    float x0 = px + ca * 24.f, y0 = py + sa * 24.f;
    float x1 = px + ca * 36.f, y1 = py + sa * 36.f;
    b.poly({{x0 + nx * bw, y0 + ny * bw},
            {x1 + nx * bw * 0.75f, y1 + ny * bw * 0.75f},
            {x1 - nx * bw * 0.75f, y1 - ny * bw * 0.75f},
            {x0 - nx * bw, y0 - ny * bw}},
           feather ? WHITE : BLADE);
    if (!feather) b.line(x0 + ca * 2.f, y0 + sa * 2.f, x1, y1, WHITE, 1.2f);
}

void rower(Bitmap& b, int frame, float cx) {
    // Chase view: the sculler faces the camera. Bow is up, stern is down.
    struct Pose {
        float head, shoulder, hip, knee, handY, handX;
        bool knees;
    };
    const Pose p[4] = {
        {64, 56, 46, 34, 76, 16, true},  // catch, arms toward the stern
        {58, 52, 46, 36, 66, 13, true},  // drive
        {46, 52, 50, 40, 54, 8, false},  // finish, leaned toward the bow
        {56, 54, 48, 38, 64, 12, false}, // recovery, blades feathered
    };
    const Pose& s = p[frame];
    b.line(cx - 3, 30, cx - 2, s.hip, SKIN, 2.2f);
    b.line(cx + 3, 30, cx + 2, s.hip, SKIN, 2.2f);
    b.ellipse(cx, 30, 5, 2.2f, METAL);  // stretcher
    if (s.knees) {
        b.ellipse(cx - 5, s.knee, 3.2f, 2.6f, SKIN);
        b.ellipse(cx + 5, s.knee, 3.2f, 2.6f, SKIN);
    }
    b.rect(cx - 5, s.hip - 3, 10, 6, SHORTS);
    b.ellipse(cx, s.hip, 5, 3, SEAT);
    b.line(cx, s.hip, cx, s.shoulder, SHIRT, 6.f);
    b.ellipse(cx, s.shoulder, 7, 3.2f, SHIRT);
    b.line(cx - 6, s.shoulder + 1, cx - s.handX, s.handY, SKIN, 2.2f);
    b.line(cx + 6, s.shoulder + 1, cx + s.handX, s.handY, SKIN, 2.2f);
    b.ellipse(cx - s.handX, s.handY, 1.7f, 1.7f, SKIN);
    b.ellipse(cx + s.handX, s.handY, 1.7f, 1.7f, SKIN);
    b.ellipse(cx, s.head, 4.6f, 5.f, SKIN);
    b.ellipse(cx, s.head - 2.4f, 4.4f, 2.6f, HAIR);
    b.rect(cx - 3, s.head - 1, 2, 1, INK);
    b.rect(cx + 2, s.head - 1, 2, 1, INK);
}

void shell(Bitmap& b, int frame) {
    const float cx = 56;
    b.poly({{cx, 6}, {cx + 6, 28}, {cx + 9, 58}, {cx + 7, 86}, {cx - 7, 86}, {cx - 9, 58}, {cx - 6, 28}}, HULL_D);
    b.poly({{cx, 10}, {cx + 4, 30}, {cx + 6, 58}, {cx + 4, 82}, {cx - 4, 82}, {cx - 6, 58}, {cx - 4, 30}}, HULL);
    b.line(cx, 16, cx, 80, HULL_D, 1.f);
    b.line(cx - 5, 36, cx - 6, 78, SHIRT, 1.2f);
    b.line(cx + 5, 36, cx + 6, 78, SHIRT, 1.2f);
    b.line(cx - 7, 60, cx - 20, 60, METAL, 2.2f);
    b.line(cx + 7, 60, cx + 20, 60, METAL, 2.2f);
    b.ellipse(cx - 20, 60, 2.1f, 2.1f, METAL);
    b.ellipse(cx + 20, 60, 2.1f, 2.1f, METAL);
    rower(b, frame, cx);
    const float angL[4] = {-2.35f, 3.14f, 2.25f, -2.55f};
    const float angR[4] = {-0.79f, 0.00f, 0.89f, -0.59f};
    bool feather = frame == 3;
    oar(b, cx - 20, 60, angL[frame], feather);
    oar(b, cx + 20, 60, angR[frame], feather);
    b.outline(INK, false);
    b.ellipse(cx, 8, 2.1f, 2.1f, WHITE);  // bow ball, after the rim
}

void buoy(Bitmap& b) {
    b.ellipse(7, 9, 5, 6.5f, 1);
    b.rect(3, 7, 8, 3, 2);
    b.ellipse(7, 4, 2.2f, 1.6f, 3);
    b.ellipse(5, 7, 1.2f, 1.f, 5);
    b.line(7, 14, 7, 18, 4, 1.2f);
    b.outline(4, false);
}

void tree(Bitmap& b, int kind) {
    b.rect(8, 16, 3, 8, 3);
    b.ellipse(9, 10, 7, 6, 1);
    b.ellipse(kind ? 7 : 11, 8, 4, 3.5f, 2);
    b.ellipse(9, 6, 2, 1.4f, 15);
}

void house(Bitmap& b) {
    b.poly({{2, 16}, {24, 10}, {46, 16}}, 5);
    b.rect(6, 16, 36, 16, 4);
    b.rect(20, 22, 8, 10, 6);
    b.rect(10, 20, 6, 5, 7);
    b.rect(32, 20, 6, 5, 7);
    b.line(6, 31, 42, 31, 3, 1.4f);
}

void stand(Bitmap& b) {
    b.rect(2, 10, 60, 16, 4);
    b.poly({{0, 12}, {32, 2}, {64, 12}}, 5);
    b.line(4, 26, 4, 32, 3, 1.6f);
    b.line(60, 26, 60, 32, 3, 1.6f);
    const int crowd[] = {8, 9, 2, 10, 8, 9, 10, 2, 8};
    for (int i = 0; i < 9; i++) b.ellipse(8 + i * 6, 16, 2.1f, 2.4f, crowd[i]);
}

void launch(Bitmap& b) {
    b.poly({{11, 2}, {18, 10}, {17, 30}, {11, 34}, {5, 30}, {4, 10}}, 1);
    b.rect(7, 12, 8, 8, 3);
    b.ellipse(11, 15, 2, 2, 4);
    b.rect(9, 28, 4, 5, 2);
    b.line(5, 8, 17, 8, 2, 1.2f);
    b.outline(6, false);
}

void cloud(Bitmap& b) {
    b.ellipse(16, 10, 12, 5, 2);
    b.ellipse(10, 9, 6, 4, 1);
    b.ellipse(22, 9, 7, 4.5f, 1);
}

void sun(Bitmap& b) {
    b.ellipse(10, 10, 8, 8, 3);
    b.ellipse(10, 10, 5, 5, 4);
}

void bird(Bitmap& b) {
    b.line(0, 4, 4, 1, 1, 1.2f);
    b.line(4, 1, 8, 5, 1, 1.2f);
}

void splashBmp(Bitmap& b) {
    b.ellipse(8, 8, 6, 3, 1);
    b.ellipse(4, 6, 2, 2, 2);
    b.ellipse(12, 5, 2.2f, 2.2f, 2);
    b.line(8, 2, 8, 0, 1, 1.f);
}

void wakeBmp(Bitmap& b) {
    b.line(2, 3, 16, 8, 1, 1.6f);
    b.line(2, 13, 16, 8, 1, 1.6f);
    b.line(6, 8, 18, 8, 2, 1.2f);
}

void shadeBmp(Bitmap& b) { b.ellipse(16, 6, 14, 4, 1); }

Bitmap label(const char* s, int scale, int fill, int edge) {
    return gs::textBitmap(s, {scale, fill, edge, 0, 1});
}

void sign(Bitmap& b, const char* digits) {
    b.rect(0, 4, 4, 20, 3);
    b.rect(4, 2, 40, 18, 8);
    b.line(4, 2, 44, 2, 5, 1.2f);
    Bitmap t = label(digits, 2, 9, 0);
    b.blit(t, 8, 4);
}

void banner(Bitmap& b) {
    b.rect(4, 0, 3, 36, 3);
    b.rect(89, 0, 3, 36, 3);
    b.rect(6, 6, 84, 20, 2);
    b.line(6, 6, 90, 6, 4, 1.4f);
    b.line(6, 25, 90, 25, 4, 1.4f);
    for (int i = 0; i < 8; i++) b.rect(8 + i * 10, 28, 5, 4, i & 1 ? 1 : 3);
    Bitmap t = label("500", 2, 1, 0);
    b.blit(t, 31, 9);
}

void shore(gs::VDP& vdp, int tile) {
    uint8_t grass[64], top[64];
    std::memset(grass, 15, sizeof grass);
    std::memset(top, 0, sizeof top);
    for (int x = 0; x < 8; x++) {
        grass[6 * 8 + x] = (x % 3 == 0) ? 1 : 15;
        top[7 * 8 + x] = 1;
        if (x == 2 || x == 5) {
            top[4 * 8 + x] = 2;
            top[5 * 8 + x] = 2;
            top[6 * 8 + x] = 3;
        }
    }
    vdp.loadTile(tile, top);
    vdp.loadTile(tile + 1, grass);
    for (int x = 0; x < 64; x++) {
        vdp.B.set(x, 11, gs::entry(tile, PAL_SCENE));
        vdp.B.set(x, 12, gs::entry(tile + 1, PAL_SCENE));
    }
}

void font(gs::VDP& vdp, int* out) {
    uint8_t px[64];
    int tile = 1;
    for (int c = 32; c < 128; c++) {
        std::memset(px, 0, sizeof px);
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        vdp.loadTile(tile, px);
        out[c - 32] = tile++;
    }
    shore(vdp, tile);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, rgb4(15, 15, 15)});
    setPal(vdp, PAL_SHELL,
           {0, rgb4(14, 13, 11), rgb4(5, 6, 8), rgb4(12, 2, 3), rgb4(13, 8, 6), rgb4(3, 2, 1), rgb4(2, 2, 6),
            rgb4(15, 15, 15), rgb4(9, 6, 3), rgb4(12, 1, 2), rgb4(7, 8, 9), rgb4(1, 1, 2), 0, rgb4(1, 1, 3),
            rgb4(13, 10, 3)});
    setPal(vdp, PAL_BUOY, {0, rgb4(15, 7, 1), rgb4(15, 15, 15), rgb4(15, 13, 3), rgb4(3, 2, 1), rgb4(15, 12, 8)});
    setPal(vdp, PAL_WAKE, {0, rgb4(14, 15, 15), rgb4(8, 12, 14)});
    setPal(vdp, PAL_LAUNCH, {0, rgb4(15, 15, 14), rgb4(12, 2, 2), rgb4(4, 6, 8), rgb4(13, 8, 6), 0, rgb4(1, 2, 4)});
    setPal(vdp, PAL_BANNER, {0, rgb4(15, 15, 15), rgb4(11, 2, 3), rgb4(2, 2, 3), rgb4(14, 11, 3)});
    setPal(vdp, PAL_TITLE, {0, rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb4(1, 2, 6)});
    setPal(vdp, PAL_SKY, {0, rgb4(15, 15, 15), rgb4(12, 13, 14), rgb4(15, 13, 5), rgb4(15, 15, 12)});
    setPal(vdp, PAL_AMBER, {0, rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb4(2, 2, 4)});
    setPal(vdp, PAL_ALERT, {0, rgb4(15, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, rgb4(8, 15, 8)});
    setPal(vdp, PAL_SCENE,
           {0, rgb4(1, 5, 2), rgb4(2, 8, 3), rgb4(6, 4, 2), rgb4(12, 11, 8), rgb4(10, 3, 2), rgb4(3, 4, 6),
            rgb4(8, 10, 12), rgb4(15, 15, 14), rgb4(1, 2, 6), rgb4(13, 8, 6), 0, 0, 0, 0, rgb4(3, 7, 3)});
    setPal(vdp, PAL_ROAD,
           {0, rgb4(2, 6, 4), rgb4(1, 5, 3), rgb4(4, 8, 5), rgb4(13, 13, 10), rgb4(10, 11, 8), rgb4(3, 8, 12),
            rgb4(2, 6, 11), rgb4(7, 12, 14), 0, 0, rgb4(1, 4, 8), rgb4(1, 5, 9), rgb4(5, 10, 13), rgb4(15, 15, 12),
            rgb4(8, 13, 15)});
    setPal(vdp, PAL_DIM, {0, rgb4(8, 10, 12)});
    vdp.setFogColor(rgb4(12, 13, 12));

    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.clear();
    vdp.B.enabled = true;
    font(vdp, art.font);

    for (int i = 0; i < 4; i++) {
        Bitmap b(112, 96);
        shell(b, i);
        art.shell[i] = gs::uploadMipped(vdp, b);
    }
    Bitmap bu(14, 20);
    buoy(bu);
    art.buoy = gs::uploadMipped(vdp, bu);
    Bitmap w(20, 16);
    wakeBmp(w);
    art.wake = gs::uploadMipped(vdp, w);
    Bitmap sp(16, 12);
    splashBmp(sp);
    art.splash = gs::uploadMipped(vdp, sp);
    Bitmap sh(32, 12);
    shadeBmp(sh);
    art.shade = gs::uploadMipped(vdp, sh);
    Bitmap la(22, 36);
    launch(la);
    art.launch = gs::uploadMipped(vdp, la);
    for (int i = 0; i < 2; i++) {
        Bitmap tr(20, 24);
        tree(tr, i);
        art.tree[i] = gs::uploadMipped(vdp, tr);
    }
    Bitmap ho(48, 34);
    house(ho);
    art.house = gs::uploadMipped(vdp, ho);
    Bitmap st(64, 34);
    stand(st);
    art.stand = gs::uploadMipped(vdp, st);
    Bitmap cl(36, 16);
    cloud(cl);
    art.cloud = gs::uploadMipped(vdp, cl);
    Bitmap su(20, 20);
    sun(su);
    art.sun = gs::uploadMipped(vdp, su);
    Bitmap bi(9, 7);
    bird(bi);
    art.bird = gs::uploadMipped(vdp, bi);
    const char* marks[] = {"100", "200", "300", "400", "500"};
    for (int i = 0; i < 5; i++) {
        Bitmap sg(46, 24);
        sign(sg, marks[i]);
        art.sign[i] = gs::uploadMipped(vdp, sg);
    }
    Bitmap ban(96, 36);
    banner(ban);
    art.banner = gs::uploadMipped(vdp, ban);
    art.title = gs::uploadMipped(vdp, label("S3 ROW", 4, 1, 13));
    art.sub = gs::uploadMipped(vdp, label("FIVE HUNDRED METERS", 2, 1, 13));
    art.stay = gs::uploadMipped(vdp, label("STAY IN THE LANE", 2, 1, 13));
    art.win = gs::uploadMipped(vdp, label("IN THE LANE", 2, 1, 13));
    art.out = gs::uploadMipped(vdp, label("OUT OF THE LANE", 2, 1, 13));
}

}  // namespace row
