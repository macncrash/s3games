#include "game/luge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace luge {

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
    mode_ = Mode::Ride;
    over_ = false;
    won_ = false;
    score_ = 0;
    miss_ = 0;
    x_ = 20.f;
    v_ = 0.6f;
    drop_ = 0;
    braking_ = false;
    ticks_ = 0;
    sys_->apu.tone(0, 220.f, 0.05f);
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
    for (int i = 0; i < 900; i++) {
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
    float c = x_ - 108.f;
    float lock = LIP - 236.f;
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
        if (y < 78) {
            int t = y * 8 / 78;
            col = gs::rgb4(4, 6 + t / 3, 10 + t / 2);
        } else if (y < 132) {
            col = gs::rgb4(9, 11, 13);
        } else {
            col = gs::rgb4(13, 14, 15);
        }
        vdp.lineBackdrop[y] = col;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    for (int i = 0; i < 5; i++) {
        float wx = 80.f + i * 220.f;
        spr(art_.hill, wx - c, 108, 52, PAL_HILL);
    }
    for (int i = 0; i < 8; i++) {
        float wx = 40.f + i * 140.f;
        float h = 50.f + float((i * 17) % 18);
        spr(art_.tree, wx - c, 128, h, PAL_TREE);
    }
    for (int i = -1; i < 24; i++) {
        float wx = i * 48.f;
        spr(art_.ice, wx - c + 24.f, 168, 22, PAL_ICE);
    }

    // Platform deck sits with its lip on LIP. Art lip is near the left of the bitmap.
    float lipScreen = LIP - c;
    spr(art_.plat, lipScreen + 48.f, 156, 58, PAL_WOOD);
    spr(art_.post, lipScreen, 112, 70, PAL_MARK);

    float nose = x_ - c;
    float sy = 150.f + drop_;
    if (braking_ && v_ > 0.2f && (ticks_ & 1)) {
        spr(art_.chip, nose - 70.f, sy + 10.f, 10, PAL_ICE);
        spr(art_.chip, nose - 84.f, sy + 6.f, 8, PAL_ICE);
    }
    spr(art_.sled, nose - 40.f, sy, 40, PAL_SLED);

    if (mode_ == Mode::Title) {
        hudC(6, "S3 LUGE");
        hudC(9, "STOP LEVEL WITH THE PLATFORM");
        hudC(12, "HOLD DOWN OR B TO BRAKE");
        hudC(16, "PRESS START");
        hudC(24, "FEET EVEN WITH THE POST");
    } else {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "SPEED %3.0f", v_ * 60.f);
        hud(1, 1, buf);
        float gap = LIP - x_;
        if (mode_ == Mode::Ride) {
            std::snprintf(buf, sizeof(buf), "TO LEVEL %+5.0f", gap);
            hud(22, 1, buf);
            if (braking_) hud(1, 3, "BRAKE");
        } else if (won_) {
            hudC(8, "LEVEL");
            std::snprintf(buf, sizeof(buf), "MISS %4.1f PX", std::fabs(miss_));
            hudC(10, buf);
            std::snprintf(buf, sizeof(buf), "SCORE %d", score_);
            hudC(12, buf);
        } else if (miss_ > 0) {
            hudC(8, "LONG");
            hudC(10, "PAST THE PLATFORM");
        } else {
            hudC(8, "SHORT");
            std::snprintf(buf, sizeof(buf), "GAP %4.1f PX", std::fabs(miss_));
            hudC(10, buf);
        }
        if (mode_ == Mode::Done && !bot_) hudC(20, "START TO RIDE AGAIN");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Ride) {
        ticks_++;
        braking_ = bot_ ? wantBrake() : (sys.pad.down(gs::BTN_DOWN) || sys.pad.down(gs::BTN_B) || sys.pad.brake > 0.25f);
        if (braking_ && v_ > 0.3f && (ticks_ % 5) == 0) sys.apu.noiseBurst(0.12f, 2400.f, 0.05f);
        stepPhys(x_, v_, braking_);
        if (v_ <= 0.f || x_ > LIP + 18.f) {
            if (x_ > LIP + 18.f && v_ > 0.f) drop_ = std::min(80.f, drop_ + v_);
            if (v_ <= 0.f || drop_ >= 40.f) {
                miss_ = x_ - LIP;
                won_ = std::fabs(miss_) <= TOL && drop_ < 1.f;
                score_ = won_ ? int(1000.f - std::fabs(miss_) * 120.f) : 0;
                mode_ = Mode::Done;
                over_ = true;
                if (won_) {
                    sys.apu.tone(0, 660.f, 0.08f);
                    beep_ = 20;
                } else {
                    sys.apu.tone(0, 110.f, 0.07f);
                    beep_ = 18;
                }
            }
        }
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        begin();
    }
    draw();
}

}  // namespace luge
