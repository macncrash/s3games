#include "game/art.h"

namespace plow {
namespace {

gs::Bitmap boatBitmap(int yaw) {
    // yaw -2..2 shears the plow so the bow points off the slip centre.
    gs::Bitmap b(40, 58);
    const float shear = yaw * 0.11f;
    auto put = [&](int x, int y, int c) {
        int sx = x + int((y - 28) * shear);
        b.set(sx, y, c);
    };
    for (int y = 0; y < 58; y++) {
        float bow = y < 16 ? (y / 16.f) : 1.f;
        int half = int(3 + bow * 12);
        if (y > 48) half -= (y - 48);
        if (half < 2) half = 2;
        for (int x = 20 - half; x <= 20 + half; x++) {
            int c = 1;
            if (y < 6) c = 4;                  // plow blade
            else if (y < 14 && (x < 20 - half + 2 || x > 20 + half - 2)) c = 5;
            else if (y > 22 && y < 40 && x > 12 && x < 28) c = 2;  // cabin
            else if (y > 26 && y < 34 && x > 15 && x < 25) c = 6;  // window
            else if (x == 20 && y > 40) c = 3;  // keel stripe
            put(x, y, c);
        }
        if (y == 8 || y == 9) {
            for (int x = 20 - half - 1; x <= 20 + half + 1; x++) put(x, y, 4);
        }
    }
    return b;
}

gs::Bitmap pierBitmap() {
    gs::Bitmap b(28, 96);
    for (int y = 0; y < 96; y++) {
        for (int x = 2; x < 26; x++) {
            int c = ((y / 6) & 1) ? 1 : 2;
            if (x < 5 || x > 22) c = 3;
            if ((y % 12) == 0) c = 4;
            b.set(x, y, c);
        }
        b.set(8, y, 5);
        b.set(19, y, 5);
    }
    return b;
}

gs::Bitmap bulkBitmap() {
    gs::Bitmap b(88, 16);
    for (int y = 2; y < 14; y++) {
        for (int x = 0; x < 88; x++) {
            int c = (x / 8) & 1 ? 2 : 1;
            if (y < 4) c = 4;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap buoyBitmap() {
    gs::Bitmap b(14, 18);
    b.ellipse(7, 8, 5, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    b.rect(6, 13, 2, 5, 3);
    return b;
}

gs::Bitmap crewBitmap() {
    gs::Bitmap b(12, 18);
    b.ellipse(6, 4, 3, 3, 1);
    b.rect(4, 8, 4, 6, 2);
    b.rect(3, 14, 2, 4, 3);
    b.rect(7, 14, 2, 4, 3);
    return b;
}

gs::Bitmap wakeBitmap() {
    gs::Bitmap b(20, 10);
    b.ellipse(10, 5, 8, 3, 1);
    b.ellipse(10, 5, 4, 1, 2);
    return b;
}

void pal(gs::VDP& vdp, int p, const uint16_t* c, int n) {
    for (int i = 0; i < n; i++) vdp.setColor(p * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(8, 14, 10),
                            gs::rgb4(14, 6, 4), gs::rgb4(4, 6, 8)};
    const uint16_t pier[] = {0, gs::rgb4(7, 5, 3), gs::rgb4(9, 7, 4), gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 6),
                             gs::rgb4(5, 5, 5)};
    const uint16_t plowC[] = {0, gs::rgb4(10, 11, 12), gs::rgb4(13, 8, 3), gs::rgb4(14, 3, 2), gs::rgb4(12, 13, 14),
                              gs::rgb4(6, 7, 8), gs::rgb4(6, 12, 14)};
    const uint16_t crewC[] = {0, gs::rgb4(6, 7, 9), gs::rgb4(10, 4, 3), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 8)};
    const uint16_t buoy[] = {0, gs::rgb4(14, 6, 2), gs::rgb4(15, 14, 6), gs::rgb4(3, 3, 3)};
    const uint16_t wake[] = {0, gs::rgb4(10, 13, 14), gs::rgb4(14, 15, 15)};
    const uint16_t mud[] = {0, gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(4, 5, 3)};
    const uint16_t foam[] = {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 11, 13)};
    pal(vdp, PAL_HUD, hud, 6);
    pal(vdp, PAL_PIER, pier, 6);
    pal(vdp, PAL_PLOW, plowC, 7);
    pal(vdp, PAL_CREW, crewC, 5);
    pal(vdp, PAL_BUOY, buoy, 4);
    pal(vdp, PAL_WAKE, wake, 3);
    pal(vdp, PAL_MUD, mud, 4);
    pal(vdp, PAL_FOAM, foam, 3);
    vdp.setFogColor(gs::rgb4(4, 6, 8));

    for (int i = 0; i < 5; i++) art.plow[i] = gs::uploadMipped(vdp, boatBitmap(i - 2));
    art.crewBoat = gs::uploadMipped(vdp, boatBitmap(1));
    art.pier = gs::uploadMipped(vdp, pierBitmap());
    art.bulk = gs::uploadMipped(vdp, bulkBitmap());
    art.buoy = gs::uploadMipped(vdp, buoyBitmap());
    art.crew = gs::uploadMipped(vdp, crewBitmap());
    art.wake = gs::uploadMipped(vdp, wakeBitmap());

    gs::TextStyle st;
    st.scale = 1;
    st.color = 1;
    st.spacing = 0;
    for (int ch = 0; ch < 96; ch++) {
        auto bm = gs::textBitmap(std::string(1, char(32 + ch)), st);
        if (bm.w < 1) bm = gs::Bitmap(4, 7);
        art.glyph[ch] = gs::uploadMipped(vdp, bm);
    }
}

}  // namespace plow
