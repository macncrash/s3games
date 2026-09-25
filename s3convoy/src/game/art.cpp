#include "game/art.h"

#include <initializer_list>

namespace convoy {

namespace {

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) v.setColor(p * 16 + i++, c);
}

gs::Mipped bake(gs::VDP& v, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 2;
    st.spacing = 1;
    return gs::uploadMipped(v, gs::textBitmap(s, st));
}

void wheel(gs::Bitmap& b, float x, float y, float rx, float ry, int tire, int hub) {
    b.ellipse(x, y, rx, ry, tire);
    b.ellipse(x, y, rx * 0.42f, ry * 0.42f, hub);
}

void truck(gs::Bitmap& b) {
    // Box truck, rear view. White stripe and red lamps so it reads small.
    b.rect(8, 10, 32, 36, 1);
    b.rect(10, 12, 28, 10, 3);
    b.rect(8, 46, 32, 14, 2);
    b.rect(10, 50, 28, 4, 4);
    b.rect(6, 60, 36, 5, 5);
    b.rect(14, 6, 20, 5, 5);
    b.rect(16, 7, 4, 3, 9);
    b.rect(28, 7, 4, 3, 9);
    b.rect(10, 52, 6, 4, 8);
    b.rect(12, 53, 3, 2, 7);
    b.rect(32, 52, 6, 4, 8);
    b.rect(34, 53, 3, 2, 7);
    wheel(b, 14, 66, 6, 6, 6, 11);
    wheel(b, 34, 66, 6, 6, 6, 11);
    gs::TextStyle st;
    st.scale = 2;
    st.color = 4;
    st.outline = 6;
    st.spacing = 1;
    gs::Bitmap tag = gs::textBitmap("S3", st);
    b.blit(tag, 16, 28);
    b.outline(6, false);
}

void jeep(gs::Bitmap& b) {
    b.rect(18, 2, 4, 22, 6);
    b.rect(16, 2, 8, 4, 4);
    b.rect(10, 16, 20, 3, 4);
    b.rect(10, 16, 3, 16, 4);
    b.rect(27, 16, 3, 16, 4);
    b.rect(8, 28, 24, 16, 1);
    b.rect(10, 30, 20, 6, 3);
    b.rect(12, 31, 6, 4, 10);
    b.rect(22, 31, 6, 4, 10);
    b.ellipse(20, 40, 5, 5, 5);
    b.ellipse(20, 40, 2, 2, 8);
    b.rect(6, 42, 28, 4, 2);
    b.rect(8, 40, 5, 3, 7);
    b.rect(27, 40, 5, 3, 7);
    wheel(b, 10, 48, 5, 5, 5, 11);
    wheel(b, 30, 48, 5, 5, 5, 11);
    b.outline(5, false);
}

void bike(gs::Bitmap& b) {
    b.ellipse(14, 8, 5, 5, 6);
    b.rect(11, 12, 6, 8, 2);
    b.rect(8, 14, 12, 3, 3);
    b.rect(6, 16, 4, 6, 3);
    b.rect(10, 20, 8, 10, 1);
    b.rect(8, 22, 10, 3, 8);
    b.rect(12, 28, 4, 6, 4);
    wheel(b, 14, 36, 6, 6, 5, 7);
    b.rect(4, 30, 8, 2, 4);
    b.rect(16, 30, 8, 2, 4);
    b.outline(5, false);
}

void wreck(gs::Bitmap& b) {
    wheel(b, 16, 24, 7, 6, 4, 7);
    wheel(b, 62, 25, 7, 6, 4, 7);
    b.poly({{8, 22}, {18, 12}, {48, 10}, {70, 16}, {74, 24}, {64, 28}, {12, 28}}, 2);
    b.poly({{22, 14}, {30, 8}, {46, 9}, {50, 15}}, 1);
    b.rect(28, 12, 12, 5, 11);
    b.rect(36, 4, 10, 8, 10);
    b.rect(40, 1, 6, 6, 7);
    b.rect(14, 16, 5, 3, 6);
    b.rect(58, 14, 6, 3, 5);
    b.outline(4, false);
}

void mine(gs::Bitmap& b) {
    b.ellipse(18, 10, 14, 6, 5);
    b.ellipse(18, 10, 6, 3, 4);
    b.rect(17, 1, 2, 4, 7);
    b.rect(17, 15, 2, 3, 7);
    b.rect(4, 9, 4, 2, 7);
    b.rect(28, 9, 4, 2, 7);
    b.ellipse(18, 10, 2, 1.4f, 6);
}

void blast(gs::Bitmap& b, int frame) {
    float cx = b.w * 0.5f, cy = b.h * 0.5f;
    if (frame == 0) {
        b.ellipse(cx, cy, 8, 8, 3);
        b.ellipse(cx, cy, 4, 4, 1);
    } else if (frame == 1) {
        b.ellipse(cx, cy, 16, 14, 4);
        b.ellipse(cx, cy, 10, 9, 3);
        b.ellipse(cx, cy, 5, 5, 2);
    } else {
        b.ellipse(cx, cy, 18, 16, 6);
        b.ellipse(cx - 4, cy - 2, 8, 7, 5);
        b.ellipse(cx + 5, cy + 2, 6, 5, 4);
    }
}

void cactus(gs::Bitmap& b) {
    b.rect(12, 16, 6, 28, 1);
    b.rect(13, 8, 4, 10, 2);
    b.rect(4, 20, 10, 4, 1);
    b.rect(4, 10, 4, 12, 1);
    b.rect(16, 24, 8, 4, 2);
    b.rect(20, 14, 4, 12, 1);
    b.rect(13, 18, 2, 20, 3);
    b.outline(13, false);
}

void butte(gs::Bitmap& b) {
    b.poly({{4, 40}, {16, 18}, {28, 26}, {44, 8}, {60, 22}, {76, 38}}, 5);
    b.poly({{18, 40}, {32, 22}, {50, 28}, {66, 40}}, 6);
    b.poly({{8, 40}, {24, 30}, {40, 40}}, 7);
}

void arch(gs::Bitmap& b) {
    b.rect(6, 16, 10, 46, 8);
    b.rect(104, 16, 10, 46, 8);
    b.rect(4, 10, 112, 12, 8);
    b.rect(8, 14, 104, 4, 15);
    b.rect(10, 4, 14, 10, 10);
    b.rect(96, 4, 14, 10, 11);
    b.rect(8, 2, 6, 8, 9);
    b.rect(108, 2, 6, 8, 9);
    gs::TextStyle st;
    st.scale = 2;
    st.color = 9;
    st.outline = 13;
    st.spacing = 1;
    gs::Bitmap word = gs::textBitmap("DEPOT", st);
    b.blit(word, (b.w - word.w) / 2, 12);
}

void sky(gs::VDP& v) {
    gs::Bitmap clouds(512, 36);
    auto blob = [&](float x, float y, float rx) {
        clouds.ellipse(x, y, rx, rx * 0.42f, 1);
        clouds.ellipse(x - rx * 0.28f, y + 2, rx * 0.62f, rx * 0.28f, 2);
        clouds.ellipse(x + rx * 0.32f, y + 1, rx * 0.5f, rx * 0.24f, 1);
    };
    blob(70, 16, 26);
    blob(190, 12, 20);
    blob(320, 18, 30);
    blob(450, 13, 22);

    gs::Bitmap mesa(512, 84);
    mesa.ellipse(250, 30, 20, 20, 7);
    mesa.ellipse(248, 28, 11, 11, 8);
    mesa.rect(0, 72, 512, 12, 5);
    for (int i = 0; i < 8; i++) {
        float x = i * 64.f;
        float peak = (i % 3 == 0) ? 24.f : (i % 3 == 1) ? 40.f : 32.f;
        int c = (i & 1) ? 2 : 4;
        mesa.poly({{x, 80}, {x + 18, peak}, {x + 36, peak + 10}, {x + 58, 78}, {x + 64, 84}, {x, 84}}, c);
    }
    mesa.poly({{0, 84}, {40, 64}, {120, 76}, {200, 58}, {300, 74}, {400, 60}, {512, 78}, {512, 84}}, 1);

    gs::TileAlloc alloc(v);
    gs::bitmapToPlane(alloc, v.A, 0, 0, clouds, PAL_CLOUD);
    gs::bitmapToPlane(alloc, v.B, 0, 1, mesa, PAL_MESA);
}

}  // namespace

void buildArt(gs::VDP& v, Art& a) {
    using gs::rgb4;
    pal(v, PAL_WHITE, {0, rgb4(15, 15, 15), rgb4(0, 0, 0), rgb4(8, 8, 9)});
    pal(v, PAL_AMBER, {0, rgb4(15, 12, 3), rgb4(2, 1, 0)});
    pal(v, PAL_RED, {0, rgb4(15, 3, 2), rgb4(3, 0, 0)});
    pal(v, PAL_GREEN, {0, rgb4(7, 15, 6), rgb4(0, 2, 0)});
    pal(v, PAL_TRUCK, {0, rgb4(6, 8, 3), rgb4(3, 5, 2), rgb4(11, 9, 5), rgb4(15, 15, 14), rgb4(4, 4, 5), rgb4(1, 1, 2),
                       rgb4(15, 2, 1), rgb4(8, 1, 1), rgb4(15, 13, 2), rgb4(8, 10, 12), rgb4(9, 9, 8)});
    pal(v, PAL_JEEP, {0, rgb4(12, 9, 5), rgb4(8, 6, 3), rgb4(5, 7, 3), rgb4(4, 4, 5), rgb4(1, 1, 1), rgb4(2, 2, 2),
                      rgb4(15, 2, 2), rgb4(14, 12, 8), 0, rgb4(6, 8, 9), rgb4(10, 10, 9)});
    pal(v, PAL_RAID, {0, rgb4(2, 2, 3), rgb4(12, 8, 5), rgb4(15, 2, 2), rgb4(6, 6, 7), rgb4(1, 1, 1), rgb4(3, 3, 4),
                      rgb4(15, 12, 2), rgb4(4, 4, 6)});
    pal(v, PAL_HAZ, {0, rgb4(10, 5, 2), rgb4(7, 3, 2), rgb4(3, 2, 2), rgb4(1, 1, 1), rgb4(15, 12, 2), rgb4(15, 6, 1),
                     rgb4(8, 8, 7), 0, 0, rgb4(6, 4, 2), rgb4(2, 2, 2), rgb4(4, 6, 7)});
    pal(v, PAL_FX, {0, rgb4(15, 15, 14), rgb4(15, 13, 3), rgb4(15, 8, 1), rgb4(13, 3, 1), rgb4(8, 2, 1), rgb4(6, 6, 6),
                    rgb4(10, 8, 5)});
    pal(v, PAL_PROP, {0, rgb4(4, 12, 3), rgb4(2, 8, 2), rgb4(7, 14, 5), rgb4(8, 5, 6), rgb4(10, 6, 5), rgb4(13, 9, 6),
                      rgb4(5, 3, 4), rgb4(9, 8, 7), rgb4(15, 15, 15), rgb4(3, 6, 14), rgb4(14, 2, 2), rgb4(8, 5, 2),
                      rgb4(1, 1, 1), rgb4(12, 9, 5), rgb4(14, 11, 3)});
    pal(v, PAL_CLOUD, {0, rgb4(15, 14, 12), rgb4(12, 10, 9)});
    pal(v, PAL_MESA, {0, rgb4(11, 6, 5), rgb4(8, 5, 6), rgb4(6, 3, 4), rgb4(13, 8, 6), rgb4(5, 3, 4), 0, rgb4(15, 12, 4),
                      rgb4(15, 15, 10)});
    pal(v, PAL_ROAD, {0, rgb4(12, 10, 6), rgb4(9, 7, 4), rgb4(6, 5, 3), rgb4(10, 8, 4), rgb4(7, 5, 3), rgb4(5, 5, 6),
                      rgb4(3, 3, 4), rgb4(8, 7, 6), rgb4(2, 2, 3), rgb4(4, 4, 5), rgb4(2, 3, 6), rgb4(3, 4, 7),
                      rgb4(5, 6, 8), rgb4(14, 12, 2), rgb4(7, 7, 8)});
    pal(v, PAL_DIM, {0, rgb4(2, 2, 3), rgb4(0, 0, 0)});

    gs::Bitmap t(48, 74);
    truck(t);
    a.truck = gs::uploadMipped(v, t);
    gs::Bitmap j(40, 54);
    jeep(j);
    a.jeep = gs::uploadMipped(v, j);
    gs::Bitmap k(28, 44);
    bike(k);
    a.bike = gs::uploadMipped(v, k);
    gs::Bitmap w(80, 34);
    wreck(w);
    a.wreck = gs::uploadMipped(v, w);
    gs::Bitmap m(36, 20);
    mine(m);
    a.mine = gs::uploadMipped(v, m);
    for (int i = 0; i < 3; i++) {
        gs::Bitmap boom(40, 40);
        blast(boom, i);
        a.boom[i] = gs::uploadMipped(v, boom);
    }
    gs::Bitmap shot(8, 8);
    shot.ellipse(4, 4, 3, 3, 2);
    shot.ellipse(4, 4, 1.4f, 1.4f, 1);
    a.shot = gs::uploadMipped(v, shot);
    gs::Bitmap sh(32, 12);
    sh.ellipse(16, 6, 14, 5, 1);
    a.shadow = gs::uploadMipped(v, sh);
    gs::Bitmap pf(16, 16);
    pf.ellipse(8, 8, 7, 6, 7);
    a.puff = gs::uploadMipped(v, pf);
    gs::Bitmap cac(28, 48);
    cactus(cac);
    a.cactus = gs::uploadMipped(v, cac);
    gs::Bitmap bu(80, 44);
    butte(bu);
    a.butte = gs::uploadMipped(v, bu);
    gs::Bitmap gate(120, 64);
    arch(gate);
    a.arch = gs::uploadMipped(v, gate);
    gs::Bitmap ar(16, 14);
    ar.poly({{2, 7}, {13, 2}, {13, 12}}, 1);
    a.arrow = gs::uploadMipped(v, ar);
    gs::Bitmap re(16, 16);
    re.rect(7, 0, 2, 5, 1);
    re.rect(7, 11, 2, 5, 1);
    re.rect(0, 7, 5, 2, 1);
    re.rect(11, 7, 5, 2, 1);
    re.rect(7, 7, 2, 2, 1);
    a.reticle = gs::uploadMipped(v, re);
    gs::Bitmap q(4, 4);
    q.rect(0, 0, 4, 4, 1);
    a.quad = gs::uploadMipped(v, q);

    for (int c = 0; c < 96; c++) {
        char s[2] = {char(32 + c), 0};
        gs::TextStyle st;
        st.scale = 2;
        st.color = 1;
        st.outline = 2;
        st.spacing = 0;
        a.font[c] = gs::uploadMipped(v, gs::textBitmap(s, st));
    }
    a.title = bake(v, "S3 CONVOY", 4);
    a.sub1 = bake(v, "THE TRUCK HAS TO ARRIVE", 2);
    a.sub2 = bake(v, "LOSE IT AND THE ROAD ENDS", 2);
    a.help1 = bake(v, "ARROWS STEER   C FIRE", 2);
    a.help2 = bake(v, "Z LEFT LANE   X RIGHT LANE", 2);
    a.roll = bake(v, "ENTER TO ROLL", 2);
    a.arrived = bake(v, "THE TRUCK ARRIVED", 3);
    a.ends = bake(v, "THE ROAD ENDS", 3);
    a.paused = bake(v, "PAUSED", 3);
    sky(v);
}

}  // namespace convoy
