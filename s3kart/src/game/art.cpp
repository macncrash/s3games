#include "game/art.h"

#include "game/track.h"

namespace kart {

static void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

static void textPal(gs::VDP& v, int p, int r, int g, int b) {
    pal(v, p, 1, r, g, b);
    pal(v, p, 15, 0, 0, 0);
}

static void kartPal(gs::VDP& v, int p, int br, int bg, int bb, int dr, int dg, int db, int sr, int sg, int sb, int hr, int hg,
                    int hb) {
    pal(v, p, 1, 0, 0, 0);
    pal(v, p, 2, br, bg, bb);
    pal(v, p, 3, dr, dg, db);
    pal(v, p, 4, sr, sg, sb);
    pal(v, p, 5, 6, 6, 7);
    pal(v, p, 6, hr, hg, hb);
    pal(v, p, 7, 3, 8, 14);
    pal(v, p, 8, 1, 1, 1);
    pal(v, p, 9, 11, 11, 12);
    pal(v, p, 10, 4, 4, 5);
    pal(v, p, 11, 15, 15, 15);
    pal(v, p, 12, 15, 2, 2);
    pal(v, p, 13, 1, 1, 2);
}

static void paintKart(gs::Bitmap& b, int number) {
    b.ellipse(13, 42, 9, 14, 8);
    b.ellipse(51, 42, 9, 14, 8);
    b.ellipse(13, 42, 3, 8, 9);
    b.ellipse(51, 42, 3, 8, 9);
    b.ellipse(16, 36, 11, 7, 4);
    b.ellipse(48, 36, 11, 7, 4);
    b.ellipse(32, 34, 15, 13, 2);
    b.ellipse(32, 31, 9, 8, 3);
    b.rect(14, 44, 36, 5, 3);
    b.rect(16, 45, 8, 3, 12);
    b.rect(40, 45, 8, 3, 12);
    b.rect(22, 30, 4, 7, 10);
    b.rect(38, 30, 4, 7, 10);
    b.rect(8, 8, 48, 5, 4);
    b.rect(10, 13, 44, 3, 3);
    b.rect(18, 16, 3, 8, 5);
    b.rect(43, 16, 3, 8, 5);
    b.ellipse(32, 24, 7, 7, 6);
    b.ellipse(32, 26, 5, 3, 7);
    b.rect(25, 36, 14, 9, 11);
    gs::TextStyle st;
    st.scale = 1;
    st.color = 13;
    st.outline = 0;
    gs::Bitmap num = gs::textBitmap(std::to_string(number), st);
    b.blit(num, 32 - num.w / 2, 37);
    b.outline(1, true);
}

static Stamp bake(gs::VDP& v, const std::string& s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.outline = 15;
    st.spacing = 1;
    gs::Bitmap b = gs::textBitmap(s, st);
    Stamp out;
    out.img = gs::uploadImage(v, b);
    out.w = b.w;
    out.h = b.h;
    return out;
}

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, 15, 15, 15);
    textPal(vdp, PAL_GOLD, 15, 13, 3);
    textPal(vdp, PAL_ALERT, 15, 3, 2);

    const int body[6][3] = {{14, 2, 2}, {2, 6, 14}, {2, 12, 4}, {15, 8, 1}, {14, 12, 2}, {10, 3, 14}};
    const int dark[6][3] = {{8, 0, 0}, {1, 2, 8}, {1, 6, 2}, {8, 3, 0}, {8, 6, 0}, {5, 1, 7}};
    const int stripe[6][3] = {{15, 14, 3}, {14, 14, 15}, {15, 15, 15}, {15, 15, 4}, {2, 2, 2}, {15, 12, 15}};
    const int helm[6][3] = {{15, 15, 15}, {15, 12, 2}, {14, 4, 4}, {2, 2, 2}, {14, 2, 2}, {15, 15, 4}};
    const int numbers[6] = {7, 1, 2, 3, 4, 5};
    for (int i = 0; i < 6; i++) {
        int p = PAL_PLAYER + i;
        kartPal(vdp, p, body[i][0], body[i][1], body[i][2], dark[i][0], dark[i][1], dark[i][2], stripe[i][0], stripe[i][1],
                stripe[i][2], helm[i][0], helm[i][1], helm[i][2]);
        gs::Bitmap b(64, 58);
        paintKart(b, numbers[i]);
        art.kart[i] = gs::uploadMipped(vdp, b);
    }

    pal(vdp, PAL_PROP, 1, 0, 0, 0);
    pal(vdp, PAL_PROP, 2, 7, 4, 1);
    pal(vdp, PAL_PROP, 3, 2, 8, 2);
    pal(vdp, PAL_PROP, 4, 7, 13, 4);
    pal(vdp, PAL_PROP, 5, 8, 8, 9);
    pal(vdp, PAL_PROP, 6, 12, 3, 3);
    pal(vdp, PAL_PROP, 7, 14, 4, 4);
    pal(vdp, PAL_PROP, 8, 3, 6, 14);
    pal(vdp, PAL_PROP, 9, 14, 12, 3);
    pal(vdp, PAL_PROP, 10, 15, 15, 15);
    pal(vdp, PAL_PROP, 11, 1, 1, 1);
    pal(vdp, PAL_PROP, 12, 8, 8, 9);
    pal(vdp, PAL_PROP, 13, 15, 15, 6);
    pal(vdp, PAL_PROP, 14, 15, 10, 2);
    pal(vdp, PAL_PROP, 15, 15, 15, 15);

    {
        gs::Bitmap b(36, 14);
        b.ellipse(18, 7, 16, 5, 1);
        art.shadow = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(40, 52);
        b.rect(17, 30, 6, 20, 2);
        b.ellipse(20, 20, 16, 15, 3);
        b.ellipse(15, 16, 8, 7, 4);
        b.outline(1, false);
        art.tree = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(80, 44);
        b.rect(4, 16, 72, 20, 5);
        b.rect(2, 12, 76, 6, 6);
        b.rect(10, 36, 8, 6, 5);
        b.rect(62, 36, 8, 6, 5);
        b.rect(6, 32, 68, 3, 12);
        for (int i = 0; i < 16; i++) b.rect(8 + i * 4, 20, 3, 5, 7 + (i % 3));
        b.outline(1, false);
        art.stand = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(30, 40);
        b.rect(4, 4, 3, 34, 12);
        for (int y = 2; y < 24; y++)
            for (int x = 8; x < 28; x++) b.set(x, y, (((x / 4) ^ (y / 4)) & 1) ? 11 : 10);
        art.flag = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(56, 22);
        b.ellipse(16, 12, 14, 8, 15);
        b.ellipse(32, 10, 16, 9, 15);
        b.ellipse(44, 13, 10, 7, 15);
        b.outline(1, false);
        art.cloud = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(100, 30);
        b.poly({{0, 30}, {12, 16}, {24, 22}, {38, 7}, {52, 18}, {66, 5}, {80, 16}, {100, 30}}, 5);
        b.poly({{32, 13}, {38, 7}, {44, 13}}, 15);
        b.poly({{60, 11}, {66, 5}, {72, 11}}, 15);
        art.hill = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(24, 24);
        b.ellipse(12, 12, 10, 10, 14);
        b.ellipse(12, 12, 6, 6, 13);
        art.sun = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(12, 12);
        b.ellipse(6, 6, 5, 4, 12);
        art.puff = gs::uploadImage(vdp, b);
    }
    {
        gs::Bitmap b(5, 5);
        b.ellipse(2.5f, 2.5f, 2.2f, 2.2f, 2);
        art.dot = gs::uploadImage(vdp, b);
    }

    pal(vdp, PAL_LAMP, 1, 3, 0, 0);
    pal(vdp, PAL_LAMP, 2, 15, 2, 1);
    pal(vdp, PAL_LAMP, 3, 15, 14, 4);
    pal(vdp, PAL_LAMP, 4, 1, 1, 1);
    {
        gs::Bitmap on(12, 12);
        on.ellipse(6, 6, 5, 5, 2);
        on.ellipse(6, 6, 2, 2, 3);
        art.lampOn = gs::uploadImage(vdp, on);
        gs::Bitmap off(12, 12);
        off.ellipse(6, 6, 5, 5, 1);
        off.ellipse(6, 6, 2, 2, 4);
        art.lampOff = gs::uploadImage(vdp, off);
    }

    pal(vdp, PAL_MAP, 1, 1, 2, 4);
    pal(vdp, PAL_MAP, 2, 2, 9, 3);
    pal(vdp, PAL_MAP, 6, 5, 5, 6);
    pal(vdp, PAL_MAP, 7, 12, 12, 8);
    pal(vdp, PAL_MAP, 14, 15, 15, 15);
    {
        gs::Bitmap m(MAP_W, MAP_H);
        m.rect(0, 0, MAP_W, MAP_H, 1);
        m.rect(2, 2, MAP_W - 4, MAP_H - 4, 2);
        for (float s = 0; s < LAP; s += 1.5f) {
            Pose c = centerline(s);
            Pose l = poseAt(s, -HALF_W);
            Pose r = poseAt(s, HALF_W);
            auto plot = [&](const Pose& p, int col, int rad) {
                int x = int(std::lround(MAP_CX + p.x * MAP_S));
                int y = int(std::lround(MAP_CY - p.z * MAP_S));
                m.rect(float(x - rad), float(y - rad), float(rad * 2 + 1), float(rad * 2 + 1), col);
            };
            int mark = wrap(s, LAP) < 7.f ? 14 : 7;
            plot(l, 6, 1);
            plot(r, 6, 1);
            plot(c, mark, 0);
        }
        art.map = gs::uploadImage(vdp, m);
    }

    auto road = [&](int p, int a6r, int a6g, int a6b, int a7r, int a7g, int a7b) {
        pal(vdp, p, 1, 4, 11, 3);
        pal(vdp, p, 2, 2, 8, 2);
        pal(vdp, p, 3, 8, 13, 4);
        pal(vdp, p, 4, 13, 3, 2);
        pal(vdp, p, 5, 15, 15, 13);
        pal(vdp, p, 6, a6r, a6g, a6b);
        pal(vdp, p, 7, a7r, a7g, a7b);
        pal(vdp, p, 8, 8, 8, 8);
        pal(vdp, p, 14, 15, 15, 13);
        pal(vdp, p, 15, 7, 7, 8);
    };
    road(PAL_ROAD, 5, 5, 6, 3, 3, 4);
    road(PAL_FINISH, 15, 15, 15, 14, 2, 2);

    vdp.setFogColor(gs::rgb4(10, 13, 15));

    art.gh = 9;
    for (int i = 0; i < 96; i++) {
        char ch = char(32 + i);
        if (ch == ' ') {
            art.gw[i] = 5;
            continue;
        }
        gs::TextStyle st;
        st.scale = 1;
        st.color = 1;
        st.outline = 15;
        st.spacing = 1;
        std::string s(1, ch);
        gs::Bitmap b = gs::textBitmap(s, st);
        art.glyph[i] = gs::uploadImage(vdp, b);
        art.gw[i] = b.w;
        art.gh = b.h;
    }

    art.title = bake(vdp, "S3 KART", 4);
    art.sub = bake(vdp, "EIGHT LAPS, ONE OVAL", 2);
    art.pitch = bake(vdp, "FINISH AHEAD OF THE PACK", 1);
    art.help = bake(vdp, "ARROWS STEER   Z GAS   X BRAKE", 1);
    art.press = bake(vdp, "PRESS START", 2);
    art.youwin = bake(vdp, "YOU WIN", 4);
    art.ahead = bake(vdp, "AHEAD OF THE PACK", 2);
    art.pack = bake(vdp, "PACK WINS", 3);
    art.paused = bake(vdp, "PAUSED", 3);
    art.go = bake(vdp, "GO", 4);
    art.d1 = bake(vdp, "1", 4);
    art.d2 = bake(vdp, "2", 4);
    art.d3 = bake(vdp, "3", 4);
}

}  // namespace kart
