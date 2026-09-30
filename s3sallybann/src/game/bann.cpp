#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace sallybann {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kGround = 190.f;
constexpr float kSpeed = 96.f;
constexpr float kJump = 248.f;
constexpr float kGrav = 740.f;
constexpr float kBoltV = 132.f;
constexpr float kHome = 34.f;
constexpr float kBanner0 = 292.f;

}  // namespace

void Game::blip(float freq) {
    sys_->apu.tone(2, freq, 0.07f);
    beep_ = 0.09f;
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    has_ = false;
    lives_ = 3;
    face_ = 1;
    px_ = 24.f;
    py_ = kGround;
    vy_ = 0.f;
    bannerX_ = kBanner0;
    clock_ = 36.f;
    spawn_ = 0.7f;
    inv_ = 0.f;
    hold_ = 0.f;
    t_ = 0.f;
    step_ = 0.f;
    shake_ = 0.f;
    bolts_.clear();
    reason_ = "THE BANNER IS STILL OUT";
}

void Game::begin() {
    bootTitle();
    mode_ = Mode::Play;
    blip(420.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    bootTitle();
    if (bot_) begin();
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    has_ = true;
    reason_ = "the banner is back at the sally";
    hold_ = 1.3f;
    blip(660.f);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.3f;
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.28f, 160.f, 0.18f);
}

bool Game::threat(float dir) const {
    if (py_ < kGround - 0.6f || vy_ < 0.f) return false;
    float rel = kBoltV + dir * kSpeed;
    if (rel < 24.f) rel = 24.f;
    for (const Bolt& b : bolts_) {
        if (!b.on) continue;
        float t = (b.x - px_) / rel;
        if (t > 0.04f && t < 0.46f) return true;
    }
    return false;
}

void Game::update() {
    t_ += kDt;
    if (inv_ > 0.f) inv_ -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);

    float dir = 0.f;
    bool jump = false;
    if (bot_) {
        float goal = has_ ? 14.f : bannerX_;
        if (px_ < goal - 5.f) dir = 1.f;
        else if (px_ > goal + 5.f) dir = -1.f;
        jump = threat(dir);
    } else {
        const gs::Pad& pad = sys_->pad;
        bool right = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.axisX > 0.35f;
        bool left = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.axisX < -0.35f;
        if (right && !left) dir = 1.f;
        else if (left && !right) dir = -1.f;
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_UP) || pad.axisY > 0.55f;
    }
    if (dir != 0.f) face_ = dir > 0.f ? 1 : -1;
    if (dir != 0.f) step_ += kDt;
    bool grounded = py_ >= kGround - 0.5f && vy_ >= 0.f;
    if (jump && grounded) {
        vy_ = -kJump;
        grounded = false;
    }
    vy_ += kGrav * kDt;
    py_ += vy_ * kDt;
    px_ += dir * kSpeed * kDt;
    px_ = std::clamp(px_, 12.f, 308.f);
    if (py_ >= kGround) {
        py_ = kGround;
        vy_ = 0.f;
        grounded = true;
    }

    spawn_ -= kDt;
    if (spawn_ <= 0.f) {
        spawn_ = 1.7f;
        Bolt b;
        b.x = 336.f;
        b.y = kGround - 6.f;
        b.on = true;
        bolts_.push_back(b);
    }
    for (Bolt& b : bolts_) {
        if (!b.on) continue;
        b.x -= kBoltV * kDt;
        if (b.x < 48.f) b.on = false;
    }

    if (inv_ <= 0.f && px_ > 46.f) {
        float top = py_ - 30.f;
        for (const Bolt& b : bolts_) {
            if (!b.on) continue;
            if (std::abs(b.x - px_) < 13.f && b.y > top && b.y < py_ + 2.f) {
                inv_ = 1.05f;
                shake_ = 0.25f;
                if (has_) {
                    has_ = false;
                    bannerX_ = std::clamp(px_, 78.f, 300.f);
                }
                px_ = std::max(48.f, px_ - 18.f);
                vy_ = -120.f;
                --lives_;
                blip(150.f);
                sys_->apu.noiseBurst(0.16f, 220.f, 0.08f);
                if (lives_ <= 0) lose("THE SALLY IS OVERRUN");
                break;
            }
        }
    }

    if (mode_ != Mode::Play) return;
    if (!has_ && grounded && std::abs(px_ - bannerX_) < 16.f) {
        has_ = true;
        blip(520.f);
    }
    if (has_ && grounded && px_ <= kHome) {
        win();
        return;
    }
    clock_ -= kDt;
    if (clock_ <= 0.f) lose(has_ ? "TOO SLOW AT THE GATE" : "THE BANNER IS STILL OUT");
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (has_) return 2;
    return 1;
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        int band = y < 150 ? (150 - y) / 28 : 0;
        v.lineBackdrop[y] = gs::rgb4(1 + band / 3, 2, 4 + band);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (!(h > 1.4f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    float sh = shake_ > 0.f ? std::sin(t_ * 48.f) * 2.f : 0.f;

    float bob = (mode_ == Mode::Play && py_ >= kGround - 0.5f && std::abs(std::sin(step_ * 10.f)) > 0.2f) ? 1.f : 0.f;
    bool blink = inv_ > 0.f && (int(inv_ * 14.f) & 1);
    if (!blink) spr(art_.runner, px_ + sh, py_ - bob, 40.f, PAL_COAT, face_ < 0, true);
    if (has_) {
        float wave = 3.f * std::sin(t_ * 7.f);
        spr(art_.banner, px_ + face_ * 14.f + wave + sh, py_ - 6.f, 46.f, PAL_BANNER, face_ < 0, true);
    } else {
        float wave = 4.f * std::sin(t_ * 3.f);
        spr(art_.banner, bannerX_ + wave + sh, kGround, 52.f, PAL_BANNER, false, true);
    }
    for (const Bolt& b : bolts_) {
        if (!b.on) continue;
        spr(art_.bolt, b.x + sh, b.y, 7.f, PAL_BOLT, false, false);
    }
    if (mode_ == Mode::Play && py_ >= kGround - 0.5f && (int(step_ * 8.f) & 1))
        spr(art_.puff, px_ - face_ * 10.f + sh, kGround - 2.f, 8.f, PAL_GROUND, false, false);

    for (int i = 0; i < 10; i++) spr(art_.sod, 18.f + i * 34.f + sh, 214.f, 18.f, PAL_GROUND, false, true);
    spr(art_.jamb, 16.f + sh, kGround + 8.f, 118.f, PAL_STONE, false, true);
    spr(art_.jamb, 58.f + sh, kGround + 8.f, 118.f, PAL_STONE, true, true);
    spr(art_.lintel, 37.f + sh, kGround - 96.f, 18.f, PAL_STONE, false, false);

    if (mode_ == Mode::Title) {
        hudC(3, "S3 SALLY BANN", PAL_GOLD);
        hudC(22, "YOU HAVE THE SALLY", PAL_HUD);
        hudC(23, "BRING THE BANNER BACK", PAL_GOLD);
        hudC(24, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(26, "LEFT RIGHT    A JUMPS", PAL_HUD);
        hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(3, "HELD", PAL_HUD);
        hudC(26, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(2, "THE BANNER IS BACK AT THE SALLY", PAL_GOOD);
        hudC(27, "START", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(2, reason_, PAL_ALERT);
        hudC(3, "ANYTHING ELSE IS A LOSS", PAL_HUD);
        hudC(27, "START RETRIES", PAL_HUD);
    } else {
        char buf[32];
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hud(1, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", int(std::ceil(std::max(0.f, clock_))));
        hud(36, 0, buf, clock_ < 8.f ? PAL_ALERT : PAL_GOLD);
        if (has_) hudC(2, "BACK TO THE SALLY", PAL_GOLD);
        else hudC(2, "TAKE THE BANNER", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || bot_) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else {
        hold_ -= kDt;
        t_ += kDt;
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 150, 60);
    else if (mode_ == Mode::Fail) sys.setLight(170, 36, 28);
    else if (mode_ == Mode::Play) sys.setLight(150, 70, 36);
    else sys.setLight(70, 70, 130);
}

}  // namespace sallybann
