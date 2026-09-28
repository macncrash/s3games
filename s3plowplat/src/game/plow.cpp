#include "game/plow.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace plow {

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.HUD.clear();
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::begin() {
    mode_ = Mode::Leg;
    over_ = false;
    won_ = false;
    score_ = 0;
    miss_ = 0;
    x_ = 36.f;
    v_ = 0.9f;
    drop_ = 0;
    braking_ = false;
    ticks_ = 0;
    sys_->apu.tone(0, 180.f, 0.05f);
    beep_ = 8;
}

void Game::stepPhys(float& x, float& v, bool brake) const {
    float g = x < FLAT ? GRADE : 0.f;
    v = v * DRAG + g;
    if (brake) v -= BRAKE;
    if (v < 0.02f) v = 0.f;
    x += v;
}

float Game::coastStop(float x, float v, int coast) const {
    for (int i = 0; i < 1200; i++) {
        bool br = i >= coast;
        stepPhys(x, v, br);
        if (br && v <= 0.f) break;
    }
    return x;
}

bool Game::wantBrake() const {
    float now = coastStop(x_, v_, 0);
    float wait = coastStop(x_, v_, 1);
    return std::fabs(now - LIP) <= std::fabs(wait - LIP);
}

float Game::cam() const {
    float c = x_ - 120.f;
    float lock = LIP - 250.f;
    if (c > lock) c = lock;
    if (c < 0) c = 0;
    return c;
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

void Game::hudC(int row, const std::string& s) { hud(20 - int(s.size()) / 2, row, s); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float c = cam();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t col;
        if (y < 90) {
            int t = y * 6 / 90;
            col = gs::rgb4(5, 7 + t / 2, 11 + t / 2);
        } else if (y < 150) {
            col = gs::rgb4(11, 12, 14);
        } else {
            col = gs::rgb4(14, 15, 15);
        }
        vdp.lineBackdrop[y] = col;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    for (int i = 0; i < 4; i++) spr(art_.barn, 160.f + i * 320.f - c, 112, 40, PAL_BARN);
    for (int i = 0; i < 10; i++) {
        float h = 46.f + float((i * 13) % 22);
        spr(art_.pine, 30.f + i * 130.f - c, 124, h, PAL_PINE);
    }
    for (int i = -1; i < 28; i++) spr(art_.snow, i * 52.f - c + 26.f, 176, 20, PAL_SNOW);

    float lipScreen = LIP - c;
    spr(art_.deck, lipScreen + 52.f, 148, 62, PAL_DECK);
    spr(art_.post, lipScreen, 100, 78, PAL_MARK);

    float blade = x_ - c;
    float sy = 152.f + drop_;
    if (braking_ && v_ > 0.15f && (ticks_ & 1)) {
        spr(art_.spray, blade - 90.f, sy + 12.f, 12, PAL_SPRAY);
        spr(art_.spray, blade - 108.f, sy + 6.f, 9, PAL_SPRAY);
    }
    // Art blade sits near the right of a 128-wide bitmap drawn at height 48.
    spr(art_.plow, blade - 36.f, sy - 6.f, 48, PAL_PLOW);

    if (mode_ == Mode::Title) {
        hudC(5, "S3 PLOWPLAT");
        hudC(8, "STOP LEVEL WITH THE PLATFORM");
        hudC(11, "MISSING THE END FAILS THE LEG");
        hudC(14, "HOLD DOWN OR B TO BRAKE");
        hudC(18, "PRESS START");
    } else {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "SPEED %3.0f", v_ * 60.f);
        hud(1, 1, buf);
        if (mode_ == Mode::Leg) {
            std::snprintf(buf, sizeof(buf), "TO END %+5.0f", LIP - x_);
            hud(22, 1, buf);
            if (braking_) hud(1, 3, "BRAKE");
        } else if (won_) {
            hudC(8, "LEVEL");
            std::snprintf(buf, sizeof(buf), "MISS %4.1f PX", std::fabs(miss_));
            hudC(10, buf);
            std::snprintf(buf, sizeof(buf), "SCORE %d", score_);
            hudC(12, buf);
        } else if (miss_ > 0) {
            hudC(8, "LEG FAILED");
            hudC(10, "PAST THE END");
        } else {
            hudC(8, "LEG FAILED");
            hudC(10, "SHORT OF THE END");
            std::snprintf(buf, sizeof(buf), "GAP %4.1f PX", std::fabs(miss_));
            hudC(12, buf);
        }
        if (mode_ == Mode::Done && !bot_) hudC(20, "START TO PLOW AGAIN");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Leg) {
        ticks_++;
        braking_ = bot_ ? wantBrake() : (sys.pad.down(gs::BTN_DOWN) || sys.pad.down(gs::BTN_B) || sys.pad.brake > 0.25f);
        if (braking_ && v_ > 0.25f && (ticks_ % 6) == 0) sys.apu.noiseBurst(0.10f, 1800.f, 0.04f);
        stepPhys(x_, v_, braking_);
        if (x_ > LIP + PAST && v_ > 0.f) drop_ = std::min(90.f, drop_ + v_ * 0.7f);
        if (v_ <= 0.f || drop_ >= 36.f) {
            miss_ = x_ - LIP;
            won_ = std::fabs(miss_) <= TOL && drop_ < 1.f;
            score_ = won_ ? int(1000.f - std::fabs(miss_) * 100.f) : 0;
            mode_ = Mode::Done;
            over_ = true;
            if (won_) {
                sys.apu.tone(0, 520.f, 0.08f);
                beep_ = 22;
            } else {
                sys.apu.tone(0, 90.f, 0.07f);
                beep_ = 16;
            }
        }
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        begin();
    }
    draw();
}

}  // namespace plow
