#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace trenchbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2480.0f;
constexpr float kFloor = 184.0f;
constexpr float kHome = 200.0f;
constexpr float kSpawn = 280.0f;
constexpr float kBanner0 = 2260.0f;
constexpr float kMinX = 90.0f;
constexpr float kMaxX = 2360.0f;
constexpr float kRun = 168.0f;
constexpr float kCarry = 128.0f;
constexpr float kDuck = 96.0f;
constexpr float kDuckCarry = 74.0f;
constexpr float kAccel = 2400.0f;
constexpr float kGrab = 30.0f;
constexpr float kWatch = 96.0f;

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (ducked_) return art_.crouch;
    if (std::abs(vx_) > 18.0f) return (int(step_ / 7.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1200.0f) return 2;
    return 3;
}

bool Game::flareHot(const Flare& f, float t) const {
    float u = std::fmod(t + f.phase, f.period);
    if (u < 0) u += f.period;
    return u < f.on;
}

bool Game::inMud(float x) const {
    return (x > 430.0f && x < 560.0f) || (x > 1480.0f && x < 1600.0f);
}

void Game::resetRun() {
    flare_[0] = {620.0f, 880.0f, 3.4f, 0.2f, 0.95f};
    flare_[1] = {1080.0f, 1360.0f, 3.1f, 1.6f, 0.9f};
    flare_[2] = {1680.0f, 1980.0f, 3.6f, 0.8f, 1.0f};
    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    has_ = false;
    ducked_ = false;
    bannerX_ = kBanner0;
    lives_ = 4;
    watch_ = kWatch;
    step_ = inv_ = stun_ = dropLock_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(380.0f, 0.05f, 0.07f);
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = 0;
    shake_ = 0.3f;
    sys_->rumble(0.25f, 0.55f, 180);
    sys_->setLight(40, 120, 40);
    blip(580.0f, 0.08f, 0.2f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    sys_->rumble(0.7f, 0.2f, 160);
    sys_->setLight(140, 30, 10);
    sys_->apu.noiseBurst(0.35f, 120.0f, 0.25f);
}

void Game::stumble(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.1f;
    stun_ = 0.22f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 140.0f;
    px_ = std::clamp(px_ + away * 10.0f, kMinX, kMaxX);
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.4f, 420.0f, 0.12f);
    sys_->rumble(0.5f, 0.2f, 90);
    if (has_) {
        has_ = false;
        bannerX_ = std::clamp(px_, 400.0f, 2100.0f);
        dropLock_ = 0.5f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& crouch) {
    left = right = crouch = false;
    const float goal = has_ ? (kHome - 12.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 5.0f) travel = 1;
    else if (goal < px_ - 5.0f) travel = -1;

    float speed = has_ ? kDuckCarry : kDuck;
    float reach = speed * 0.35f + 36.0f;
    for (const Flare& f : flare_) {
        bool soon = false;
        for (float ahead = 0; ahead <= 0.45f; ahead += 0.15f)
            if (flareHot(f, playT_ + ahead)) soon = true;
        if (!soon) continue;
        float lo = f.a - reach;
        float hi = f.b + reach;
        if (px_ > lo && px_ < hi) crouch = true;
    }

    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = -1;
    }
}

void Game::stepPlay(bool left, bool right, bool crouch) {
    playT_ += DT;
    watch_ -= DT;
    if (watch_ <= 0.0f) {
        watch_ = 0;
        lose();
        return;
    }
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;

    ducked_ = crouch && stun_ <= 0;

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 480.0f * DT);
    } else {
        float target = 0;
        if (right && !left) target = ducked_ ? (has_ ? kDuckCarry : kDuck) : (has_ ? kCarry : kRun);
        if (left && !right) target = -(ducked_ ? (has_ ? kDuckCarry : kDuck) : (has_ ? kCarry : kRun));
        if (inMud(px_)) target *= 0.5f;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }
    px_ += vx_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);
    if (std::abs(vx_) > 24.0f && !ducked_) step_ += std::abs(vx_) * DT * 0.18f;

    if (has_ && px_ <= kHome) {
        win();
        return;
    }

    if (inv_ <= 0 && stun_ <= 0 && !ducked_) {
        for (const Flare& f : flare_) {
            if (!flareHot(f, playT_)) continue;
            if (px_ > f.a && px_ < f.b) {
                stumble((f.a + f.b) * 0.5f);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(660.0f, 0.07f, 0.1f);
        sys_->rumble(0.2f, 0.4f, 80);
        sys_->setLight(150, 40, 16);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, float(g.h) * scale, pal, false, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float view = cam_;
    if (shake_ > 0.02f) view += std::sin(t_ * 48.0f) * shake_ * 3.0f;
    vdp.A.scroll(int(view * 0.55f), 0);
    vdp.B.scroll(int(view), 0);

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    if ((inv_ <= 0) || (int(t_ * 18.0f) & 1)) {
        world(art_.shadow, px_, kFloor, 8, PAL_SKY, false);
        float hh = ducked_ ? 36.0f : 62.0f;
        world(hero(), px_, kFloor, hh, PAL_COAT, face_ < 0);
        if (has_) world(art_.banner, px_ + face_ * 14.0f, kFloor - (ducked_ ? 18.0f : 28.0f), 32, PAL_BANNER, face_ < 0);
    }

    if (!has_) {
        world(art_.pole, bannerX_, kFloor, 78, PAL_BAG, false);
        float wave = std::sin(t_ * 2.6f) * 2.0f;
        world(art_.banner, bannerX_ + wave, kFloor - 8.0f, 40, PAL_BANNER, false);
    }

    const float bags[] = {360, 980, 1420, 2060};
    for (float x : bags) world(art_.bag, x, kFloor - 6.0f, 20, PAL_BAG, false);

    const float muds[] = {495, 1540};
    for (float x : muds) world(art_.duck, x, kFloor + 2.0f, 14, PAL_MUD, false);

    for (const Flare& f : flare_) {
        float mid = (f.a + f.b) * 0.5f;
        bool hot = mode_ != Mode::Title && flareHot(f, playT_);
        world(art_.stake, f.a, kFloor - 40.0f, 48, PAL_FLARE, false);
        world(art_.stake, f.b, kFloor - 40.0f, 48, PAL_FLARE, false);
        if (hot) {
            world(art_.flare, mid, kFloor - 70.0f, 36, PAL_FLARE, false);
            float span = f.b - f.a;
            int n = std::max(1, int(span / 70.0f));
            for (int i = 0; i < n; i++) {
                float x = f.a + span * (i + 0.5f) / float(n);
                world(art_.flare, x, kFloor - 52.0f, 16, PAL_FLARE, false);
            }
        }
    }

    world(art_.pole, 130.0f, kFloor, 90, PAL_BAG, false);
    world(art_.banner, 148.0f + std::sin(t_ * 2.0f), kFloor - 10.0f, 28, PAL_BANNER, false);

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    int sec = int(watch_ + 0.999f);
    if (sec < 0) sec = 0;
    char clock[20];
    std::snprintf(clock, sizeof(clock), "HOLD %02d", sec);
    hud(16, 1, clock, watch_ < 12.0f ? PAL_BANNER : PAL_HUD);
    if (has_) hud(28, 1, "BANNER", PAL_BANNER);
    else hud(30, 1, "OUT", PAL_HUD);
    if (ducked_ && mode_ == Mode::Play) hud(1, 2, "DOWN", PAL_FLARE);

    if (mode_ == Mode::Title) {
        text("TRENCH BANN", 160, 28, 1.05f, PAL_HUD, 0);
        text("ONE TRENCH", 160, 54, 0.55f, PAL_FLARE, 0);
        text("BRING THE BANNER BACK", 160, 74, 0.48f, PAL_BANNER, 0);
        text("THEN IT IS DONE", 160, 92, 0.48f, PAL_BAG, 0);
        text("DOWN UNDER THE FLARE", 160, 114, 0.4f, PAL_MUD, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 136, 0.65f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 48, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 36, 0.62f, PAL_BANNER, 0);
        text("THE TRENCH IS DONE", 160, 58, 0.55f, PAL_HUD, 0);
    } else if (mode_ == Mode::Over) {
        text("THE TRENCH HELD", 160, 40, 0.66f, PAL_HUD, 0);
        text("START TO TRY AGAIN", 160, 66, 0.46f, PAL_FLARE, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleHold_ = 0;
    t_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
    bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
    bool crouch = pad.down(gs::BTN_DOWN) || pad.axisY < -0.45f;
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, crouch);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, crouch);
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && start) {
            mode_ = Mode::Title;
            titleHold_ = 0;
            over_ = false;
            won_ = false;
            resetRun();
        }
    }

    float want = std::clamp(px_ - 150.0f, 0.0f, kWorld - float(gs::SCREEN_W));
    cam_ = approach(cam_, want, 480.0f * DT);
    draw();
}

}  // namespace trenchbann
