#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace choirchime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void voicePal(gs::VDP& vdp, int pal, uint16_t robe, uint16_t shade, uint16_t hair) {
    setPal(vdp, pal,
           {0, gs::rgb4(15, 15, 14), robe, shade, gs::rgb4(13, 9, 6), hair, gs::rgb4(8, 2, 3), gs::rgb4(1, 1, 2),
            gs::rgb4(14, 12, 6), gs::rgb4(12, 4, 6), gs::rgb4(3, 2, 2), gs::rgb4(12, 7, 6), gs::rgb4(15, 13, 4),
            gs::rgb4(15, 12, 5), gs::rgb4(1, 1, 2), gs::rgb4(2, 1, 3)});
}

gs::Bitmap singerBmp(int kind, bool open) {
    gs::Bitmap b(32, 52);
    const int cx = 16;
    const int sh = kind == 2 ? 12 : kind == 1 ? 10 : 8;
    b.ellipse(float(cx), 49, float(sh - 1), 2, 14);
    b.poly({{float(cx - sh), 22}, {float(cx + sh), 22}, {float(cx + sh - 2), 46}, {float(cx - sh + 2), 46}}, 2);
    b.poly({{float(cx - sh + 2), 24}, {float(cx - 1), 24}, {float(cx - 1), 44}, {float(cx - sh + 3), 44}}, 3);
    b.ellipse(float(cx - 5), 46, 3, 2, 10);
    b.ellipse(float(cx + 5), 46, 3, 2, 10);
    b.rect(cx - 2, 18, 5, 5, 4);
    b.ellipse(float(cx), 12, 6, 7, 4);
    if (kind == 0) {
        b.ellipse(float(cx), 7, 7, 4, 5);
        b.rect(cx - 7, 8, 3, 7, 5);
        b.rect(cx + 4, 8, 3, 7, 5);
    } else if (kind == 1) {
        b.ellipse(float(cx), 7, 7, 4, 5);
        b.rect(cx - 8, 9, 3, 14, 5);
        b.rect(cx + 5, 9, 3, 14, 5);
    } else {
        b.rect(cx - 6, 6, 4, 3, 5);
        b.rect(cx + 2, 6, 4, 3, 5);
        b.ellipse(float(cx), 16, 5, 3, 5);
    }
    b.set(cx - 3, 12, 7);
    b.set(cx + 2, 12, 7);
    if (open) b.rect(cx - 2, 15, 4, 2, 6);
    else b.rect(cx - 2, 16, 4, 1, 6);
    b.rect(cx - 1, 23, 2, 2, 12);
    b.outline(15, true);
    return b;
}

void nave(gs::VDP& vdp, gs::TileAlloc& tiles) {
    gs::Bitmap b(320, 224);
    for (int y = 0; y < 224; y++) {
        int c = y < 96 ? 2 : y < 150 ? 3 : 4;
        b.rect(0, y, 320, 1, c);
    }
    for (int i = 0; i < 5; i++) {
        int cx = 32 + i * 64;
        b.rect(cx - 10, 28, 20, 70, 5);
        b.ellipse(float(cx), 28, 16, 18, 6);
        b.rect(cx - 6, 40, 12, 40, 7);
    }
    b.rect(0, 168, 320, 56, 8);
    b.rect(0, 168, 320, 4, 9);
    for (int i = 0; i < 8; i++) b.rect(8 + i * 40, 176, 6, 40, 10);
    b.rect(148, 40, 24, 36, 11);
    b.ellipse(160, 36, 16, 10, 12);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, b, PAL_NAVE);
}

gs::Bitmap faceBmp() {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 16, 16, 1);
    b.ellipse(18, 18, 13, 13, 2);
    for (int i = 0; i < 12; i++) {
        float a = i * 6.2831853f / 12.f - 1.5708f;
        int x = int(std::lround(18 + std::cos(a) * 11));
        int y = int(std::lround(18 + std::sin(a) * 11));
        b.set(x, y, 3);
    }
    b.rect(17, 8, 2, 8, 4);
    b.rect(18, 17, 7, 2, 5);
    b.outline(6, true);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 3)});
    setPal(vdp, PAL_NAVE,
           {0, gs::rgb4(6, 5, 8), gs::rgb4(2, 1, 4), gs::rgb4(3, 2, 5), gs::rgb4(4, 3, 6), gs::rgb4(5, 4, 6),
            gs::rgb4(8, 10, 13), gs::rgb4(12, 13, 14), gs::rgb4(4, 3, 3), gs::rgb4(8, 6, 4), gs::rgb4(3, 2, 2),
            gs::rgb4(10, 8, 4), gs::rgb4(14, 12, 6), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 2), gs::rgb4(0, 0, 0)});
    voicePal(vdp, PAL_TREBLE, gs::rgb4(12, 12, 14), gs::rgb4(7, 7, 11), gs::rgb4(14, 10, 4));
    voicePal(vdp, PAL_ALTO, gs::rgb4(10, 6, 10), gs::rgb4(6, 3, 7), gs::rgb4(6, 3, 2));
    voicePal(vdp, PAL_BASS, gs::rgb4(4, 5, 8), gs::rgb4(2, 3, 5), gs::rgb4(2, 2, 2));
    setPal(vdp, PAL_GOLD,
           {0, gs::rgb4(15, 13, 5), gs::rgb4(10, 7, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 5), gs::rgb4(8, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(12, 10, 6), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 3), gs::rgb4(10, 2, 2),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    vdp.setFogColor(gs::rgb4(2, 1, 4));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    nave(vdp, tiles);

    for (int k = 0; k < 3; k++) {
        art.singer[k][0] = gs::uploadMipped(vdp, singerBmp(k, false));
        art.singer[k][1] = gs::uploadMipped(vdp, singerBmp(k, true));
    }
    gs::Bitmap note(10, 12);
    note.ellipse(4, 8, 3.5f, 2.5f, 1);
    note.rect(7, 1, 2, 8, 1);
    art.note = gs::uploadMipped(vdp, note);

    gs::Bitmap dia(9, 9);
    dia.poly({{4, 0}, {8, 4}, {4, 8}, {0, 4}}, 1);
    dia.poly({{4, 2}, {6, 4}, {4, 6}, {2, 4}}, 2);
    art.diamond = gs::uploadMipped(vdp, dia);

    gs::Bitmap ferm(18, 10);
    for (int x = 1; x <= 16; x++) {
        float u = (x - 8.5f) / 8.f;
        float y = 5.f - std::sqrt(std::max(0.f, 1.f - u * u)) * 4.f;
        ferm.set(x, int(y), 1);
        ferm.set(x, int(y) + 1, 1);
    }
    ferm.ellipse(9, 8, 1.5f, 1.5f, 1);
    art.fermata = gs::uploadMipped(vdp, ferm);

    gs::Bitmap halo(28, 12);
    for (int a = 0; a < 36; a++) {
        float t = a * 6.2831853f / 36.f;
        halo.set(int(std::lround(14 + std::cos(t) * 11)), int(std::lround(5 + std::sin(t) * 3.5f)), 1);
    }
    art.halo = gs::uploadMipped(vdp, halo);

    gs::Bitmap bell(16, 18);
    bell.poly({{8, 1}, {13, 6}, {12, 14}, {4, 14}, {3, 6}}, 1);
    bell.rect(7, 0, 2, 3, 2);
    bell.ellipse(8, 15, 2, 2, 2);
    bell.rect(5, 13, 6, 2, 3);
    art.bell = gs::uploadMipped(vdp, bell);

    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.outline = 15;
    st.spacing = 1;
    art.title = gs::uploadMipped(vdp, gs::textBitmap("CHOIR", st));

    gs::Bitmap staff(2, 2);
    staff.set(0, 0, 1);
    staff.set(1, 0, 1);
    art.staff = gs::uploadImage(vdp, staff);
    gs::Bitmap rule(2, 2);
    rule.set(0, 0, 1);
    rule.set(0, 1, 1);
    art.rule = gs::uploadImage(vdp, rule);
    art.face = gs::uploadImage(vdp, faceBmp());
}

}  // namespace choirchime
