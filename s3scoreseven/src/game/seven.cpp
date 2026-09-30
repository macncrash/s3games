#include "game/seven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace scoreseven {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap deskArt() {
    Bitmap b(280, 48);
    b.rect(0, 6, 280, 22, 2);
    b.rect(0, 6, 280, 5, 1);
    b.rect(0, 22, 280, 6, 3);
    b.rect(18, 26, 8, 18, 3);
    b.rect(254, 26, 8, 18, 3);
    b.outline(4, false);
    return b;
}

Bitmap sheetArt() {
    Bitmap b(248, 120);
    b.rect(0, 0, 248, 120, 1);
    b.rect(5, 5, 238, 110, 2);
    for (int i = 0; i < 5; i++) b.rect(34, 22 + i * 7, 196, 1, 3);
    for (int i = 0; i < 5; i++) b.rect(34, 72 + i * 7, 196, 1, 3);
    b.rect(220, 20, 2, 30, 3);
    b.rect(226, 20, 1, 30, 3);
    b.rect(220, 70, 2, 30, 3);
    b.outline(4, false);
    return b;
}

Bitmap clefArt() {
    Bitmap b(22, 48);
    b.ellipse(10, 32, 6, 8, 1);
    b.rect(13, 6, 3, 32, 1);
    b.ellipse(14, 12, 5, 5, 1);
    b.ellipse(10, 32, 2, 3, 2);
    return b;
}

Bitmap noteArt() {
    Bitmap b(18, 32);
    b.ellipse(6, 24, 6, 4, 1);
    b.rect(11, 3, 2, 22, 1);
    b.rect(11, 3, 6, 2, 1);
    return b;
}

Bitmap barArt() {
    Bitmap b(6, 40);
    b.rect(2, 0, 2, 40, 1);
    return b;
}

Bitmap handArt() {
    Bitmap b(36, 22);
    b.ellipse(14, 12, 11, 7, 1);
    b.rect(20, 9, 14, 3, 2);
    b.rect(30, 6, 2, 10, 3);
    return b;
}

Bitmap lampArt() {
    Bitmap b(26, 58);
    b.rect(11, 18, 4, 36, 2);
    b.poly({{3, 20}, {13, 3}, {23, 20}}, 1);
    b.rect(5, 18, 16, 5, 3);
    b.ellipse(13, 12, 3, 2, 4);
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

const float kFreq[4] = {294.f, 349.f, 440.f, 523.f};
const float kLaneY[4] = {148.f, 140.f, 132.f, 124.f};

}  // namespace

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 6), gs::rgb4(15, 11, 4), gs::rgb4(12, 4, 3),
                          gs::rgb4(6, 12, 8), gs::rgb4(8, 11, 14), shadow, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(9, 6, 4),
                           gs::rgb4(14, 11, 7), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(15, 14, 11), gs::rgb4(13, 12, 9), gs::rgb4(3, 2, 2), gs::rgb4(8, 7, 6),
                            shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 8), gs::rgb4(1, 1, 2), shadow, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(11, 7, 2), gs::rgb4(6, 3, 1), gs::rgb4(15, 15, 9),
                           shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOM, {0, gs::rgb4(13, 10, 6), gs::rgb4(5, 3, 2), gs::rgb4(14, 8, 3), gs::rgb4(15, 13, 7),
                           shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    loadFont(vdp, art_);
    art_.desk = gs::uploadMipped(vdp, deskArt());
    art_.sheet = gs::uploadMipped(vdp, sheetArt());
    art_.clef = gs::uploadMipped(vdp, clefArt());
    art_.note = gs::uploadMipped(vdp, noteArt());
    art_.bar = gs::uploadMipped(vdp, barArt());
    art_.hand = gs::uploadMipped(vdp, handArt());
    art_.lamp = gs::uploadMipped(vdp, lampArt());
    gs::TextStyle st;
    st.scale = 2;
    st.color = 1;
    st.shadow = 2;
    st.spacing = 1;
    art_.word = gs::uploadMipped(vdp, gs::textBitmap("SEVEN", st));
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    sys.apu.setMaster(0.4f);
    gs::FMPatch tone;
    tone.alg = 0;
    tone.vol = 0.2f;
    tone.op[0] = {1, 1, 0.01f, 0.12f, 0.5f, 0.16f, 0};
    tone.op[1] = {2, 0.25f, 0.01f, 0.14f, 0.28f, 0.2f, 0};
    tone.op[2] = {1, 0.1f, 0.02f, 0.18f, 0.18f, 0.28f, 0};
    tone.op[3] = {1, 0.3f, 0.01f, 0.16f, 0.36f, 0.18f, 0};
    sys.apu.setPatch(0, tone);
    sys.apu.setPatch(1, tone);
    msg_ = "FIRST TO SEVEN";
    msgT_ = 4;
    draw();
}

void Game::blip(float freq) { sys_->apu.keyOn(0, freq, 0.22f); }

void Game::fanfare() {
    sys_->apu.keyOn(0, 523, 0.24f);
    sys_->apu.keyOn(1, 659, 0.16f);
}

void Game::hud(int col, int row, const std::string& s) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], PAL_HUD));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

bool Game::press(gs::Button b) const { return sys_->pad.pressed(b); }

int Game::lanePressed() const {
    if (press(gs::BTN_LEFT) || press(gs::BTN_A)) return 0;
    if (press(gs::BTN_DOWN) || press(gs::BTN_B)) return 1;
    if (press(gs::BTN_UP) || press(gs::BTN_C)) return 2;
    if (press(gs::BTN_RIGHT) || press(gs::BTN_X)) return 3;
    return -1;
}

void Game::spawn() {
    live_ = 1;
    nx_ = 292.f;
    lane_ = (you_ + them_) % 4;
}

void Game::hit() {
    live_ = 2;
    flash_ = 0.18f;
    you_++;
    blip(kFreq[lane_]);
    if (you_ >= 7 && them_ < 7) {
        mode_ = Mode::Hold;
        t_ = 0;
        won_ = true;
        msg_ = "FIRST TO SEVEN";
        msgT_ = 3;
        fanfare();
        return;
    }
    spawn();
}

void Game::miss() {
    live_ = 3;
    flash_ = 0.12f;
    blip(90.f);
    msg_ = "LATE";
    msgT_ = 0.5f;
    spawn();
}

void Game::rivalPoint() {
    if (them_ >= 7) return;
    them_++;
    sys_->apu.keyOn(1, 196.f, 0.12f);
    if (them_ >= 7 && you_ < 7) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        msg_ = "THEY LED";
        msgT_ = 3;
        blip(110.f);
    }
}

void Game::backdrop() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        vdp.lineBackdrop[y] = gs::rgb4(2 + int((1.f - u) * 2), 2 + int(u), 4 + int(u * 3));
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::update(float dt) {
    t_ += dt;
    if (msgT_ > 0) msgT_ -= dt;
    if (flash_ > 0) flash_ -= dt;

    if (mode_ == Mode::Title) {
        if (bot_ ? t_ > 0.35f : (press(gs::BTN_START) || press(gs::BTN_A))) {
            mode_ = Mode::Play;
            t_ = 0;
            you_ = 0;
            them_ = 0;
            rival_ = 0;
            msg_ = "READ YOUR STAFF";
            msgT_ = 1.6f;
            blip(440);
            spawn();
        }
        return;
    }

    if (mode_ == Mode::Play) {
        rival_ += dt;
        if (rival_ >= 1.2f) {
            rival_ -= 1.2f;
            rivalPoint();
            if (over_) return;
        }
        if (live_ == 1) {
            nx_ -= kSpeed * dt;
            if (nx_ < kLine - 16.f) miss();
        }
        if (over_ || mode_ != Mode::Play) return;
        if (bot_) {
            if (live_ == 1 && nx_ <= kLine + 6.f && nx_ >= kLine - 10.f) hit();
        } else if (live_ == 1) {
            int lane = lanePressed();
            if (lane >= 0) {
                float d = std::fabs(nx_ - kLine);
                if (d < 18.f) {
                    if (lane == lane_) hit();
                    else miss();
                }
            }
        }
        return;
    }

    if (mode_ == Mode::Hold) {
        if (t_ > 1.1f) {
            mode_ = Mode::Over;
            over_ = true;
        }
        return;
    }

    if (mode_ == Mode::Over && !bot_ && (press(gs::BTN_START) || press(gs::BTN_A))) {
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
        you_ = 0;
        them_ = 0;
        live_ = 0;
        t_ = 0;
        msg_ = "FIRST TO SEVEN";
        msgT_ = 4;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();

    spr(art_.lamp, 26, 70, 56, PAL_ROOM);
    spr(art_.desk, 160, 188, 42, PAL_WOOD);
    spr(art_.sheet, 168, 108, 108, PAL_PAPER);
    spr(art_.clef, 58, 78, 32, PAL_INK);
    spr(art_.clef, 58, 138, 32, PAL_INK);
    spr(art_.bar, kLine, 78, 30, PAL_INK);
    spr(art_.bar, kLine, 138, 30, mode_ == Mode::Play ? PAL_GOLD : PAL_INK);

    const float topY[4] = {92.f, 84.f, 76.f, 68.f};
    for (int i = 0; i < 7; i++) {
        float x = 132.f + i * 16.f;
        int pal = i < them_ ? PAL_INK : PAL_PAPER;
        spr(art_.note, x, topY[i % 4], i < them_ ? 18.f : 14.f, pal);
    }

    if (mode_ == Mode::Title) {
        for (int i = 0; i < 7; i++) spr(art_.note, 120.f + i * 18.f, kLaneY[i % 4], 16.f, PAL_INK);
        spr(art_.word, 168, 40, 18, PAL_GOLD);
    } else if (live_ != 0) {
        int pal = live_ == 2 || (won_ && you_ >= 7) ? PAL_GOLD : PAL_INK;
        float x = (mode_ == Mode::Play && live_ == 1) ? nx_ : kLine;
        if (mode_ != Mode::Play) x = kLine + 8.f;
        spr(art_.note, x, kLaneY[lane_], 22.f, pal);
    }

    float youHand = (mode_ == Mode::Play && live_ == 1) ? std::clamp(nx_, 90.f, 240.f) : kLine;
    spr(art_.hand, youHand, 168, 16, PAL_GOLD, false);
    float themHand = 132.f + std::min(them_, 6) * 16.f;
    spr(art_.hand, themHand, 48, 14, PAL_INK, true);

    hud(12, 1, "S3 SCORE SEVEN");
    if (mode_ == Mode::Title) {
        hud(9, 3, "A SHORT SCORE");
        hud(3, 25, "ARROWS ON THE BAR  START");
    } else {
        char line[32];
        std::snprintf(line, sizeof(line), "YOU %d", you_);
        hud(1, 1, line);
        std::snprintf(line, sizeof(line), "THEM %d", them_);
        hud(30, 1, line);
        if (msgT_ > 0) {
            int col = std::max(0, 20 - int(msg_.size()) / 2);
            hud(col, 26, msg_);
        }
        if (won_) hud(12, 24, "YOU LED");
        if (over_ && !won_) hud(13, 24, "SHORT");
        if (mode_ == Mode::Play) hud(1, 26, "L D U R");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update(1.0f / 60.0f);
    draw();
}

}  // namespace scoreseven
