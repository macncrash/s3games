#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace bunkerbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2200.0f;
constexpr float kFloor = 186.0f;
constexpr float kHome = 168.0f;
constexpr float kSpawn = 280.0f;
constexpr float kBanner0 = 1960.0f;
constexpr float kMinX = 80.0f;
constexpr float kMaxX = 2100.0f;
constexpr float kRun = 176.0f;
constexpr float kCarry = 138.0f;
constexpr float kAccel = 2600.0f;
constexpr float kBody = 22.0f;
constexpr float kStrike = 60.0f;
constexpr float kGrab = 32.0f;
constexpr float kWatch = 72.0f;

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (swing_ > 0) return art_.shove;
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

void Game::resetRun() {
    auto set = [&](int i, float a, float b, float sp) {
        Sentry& w = sentry_[i];
        w.minX = a;
        w.maxX = b;
        w.x = (a + b) * 0.5f;
        w.speed = sp;
        w.dir = (i & 1) ? -1.0f : 1.0f;
        w.stun = 0;
    };
    set(0, 520.0f, 820.0f, 36.0f);
    set(1, 1020.0f, 1360.0f, 40.0f);
    set(2, 1520.0f, 1840.0f, 38.0f);
    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    lives_ = 4;
    watch_ = kWatch;
    step_ = strikeCd_ = swing_ = inv_ = stun_ = dropLock_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(420.0f, 0.05f, 0.07f);
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
    shake_ = 0.35f;
    sys_->rumble(0.25f, 0.6f, 180);
    sys_->setLight(40, 140, 50);
    blip(620.0f, 0.08f, 0.2f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    sys_->rumble(0.7f, 0.2f, 160);
    sys_->setLight(140, 20, 16);
    sys_->apu.noiseBurst(0.35f, 120.0f, 0.25f);
}

void Game::stumble(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.05f;
    stun_ = 0.24f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 150.0f;
    px_ = std::clamp(px_ + away * 8.0f, kMinX, kMaxX);
    shake_ = 0.8f;
    sys_->apu.noiseBurst(0.36f, 380.0f, 0.12f);
    sys_->rumble(0.55f, 0.25f, 90);
    if (has_) {
        has_ = false;
        bannerX_ = std::clamp(px_, 360.0f, 1880.0f);
        dropLock_ = 0.45f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& strike) {
    left = right = strike = false;
    const float goal = has_ ? (kHome - 18.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 4.0f) travel = 1;
    else if (goal < px_ - 4.0f) travel = -1;

    int threat = -1;
    float best = 1.0e9f;
    for (int i = 0; i < 3; i++) {
        const Sentry& w = sentry_[i];
        if (w.stun > 0) continue;
        float rel = w.x - px_;
        float ahead = travel == 0 ? std::abs(rel) : (travel > 0 ? rel : -rel);
        if (ahead < -16.0f || ahead > 78.0f) continue;
        if (std::abs(rel) < best) {
            best = std::abs(rel);
            threat = i;
        }
    }
    if (threat >= 0 && strikeCd_ <= 0.0f) {
        float rel = sentry_[threat].x - px_;
        face_ = rel >= 0.0f ? 1 : -1;
        strike = true;
        return;
    }
    if (threat >= 0 && best < 50.0f) {
        if (sentry_[threat].x >= px_) left = true;
        else right = true;
        face_ = left ? -1 : 1;
        return;
    }
    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = -1;
    }
}

void Game::stepPlay(bool left, bool right, bool strike) {
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
    if (strikeCd_ > 0) strikeCd_ -= DT;
    if (swing_ > 0) swing_ -= DT;

    const bool roused = has_;
    for (Sentry& w : sentry_) {
        if (w.stun > 0) {
            w.stun -= DT;
            continue;
        }
        if (roused) {
            w.dir = px_ >= w.x ? 1.0f : -1.0f;
            w.x += w.dir * 92.0f * DT;
        } else {
            w.x += w.dir * w.speed * DT;
            if (w.x >= w.maxX) {
                w.x = w.maxX;
                w.dir = -1.0f;
            } else if (w.x <= w.minX) {
                w.x = w.minX;
                w.dir = 1.0f;
            }
        }
        w.x = std::clamp(w.x, 340.0f, 2060.0f);
    }

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 420.0f * DT);
    } else {
        float target = 0;
        if (right && !left) target = has_ ? kCarry : kRun;
        if (left && !right) target = has_ ? -kCarry : -kRun;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }
    px_ += vx_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);
    if (std::abs(vx_) > 24.0f) step_ += std::abs(vx_) * DT * 0.18f;

    if (has_ && px_ <= kHome) {
        win();
        return;
    }

    if (strike && strikeCd_ <= 0 && stun_ <= 0) {
        strikeCd_ = 0.26f;
        swing_ = 0.12f;
        bool hit = false;
        for (Sentry& w : sentry_) {
            if (w.stun > 0) continue;
            float rel = w.x - px_;
            bool struck = face_ > 0 ? (rel > -12.0f && rel < kStrike) : (rel < 12.0f && rel > -kStrike);
            if (!struck) continue;
            w.stun = 1.85f;
            float shove = face_ > 0 ? 100.0f : -100.0f;
            w.x = std::clamp(w.x + shove, 360.0f, 2040.0f);
            hit = true;
        }
        if (hit) {
            blip(170.0f, 0.06f, 0.05f);
            sys_->apu.noiseBurst(0.16f, 760.0f, 0.05f);
        } else {
            blip(250.0f, 0.03f, 0.03f);
        }
    }

    if (inv_ <= 0 && stun_ <= 0) {
        for (const Sentry& w : sentry_) {
            if (w.stun > 0) continue;
            if (std::abs(w.x - px_) < kBody) {
                stumble(w.x);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(700.0f, 0.07f, 0.1f);
        sys_->rumble(0.2f, 0.45f, 80);
        sys_->setLight(160, 50, 20);
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
    vdp.A.scroll(int(view * 0.28f), 0);
    vdp.B.scroll(int(view * 0.82f), 0);

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    if ((inv_ <= 0) || (int(t_ * 18.0f) & 1)) {
        world(art_.shadow, px_, kFloor, 10, PAL_DUSK, false);
        world(hero(), px_, kFloor, 64, PAL_COAT, face_ < 0);
        if (has_) world(art_.banner, px_ + face_ * 16.0f, kFloor - 30.0f, 36, PAL_BANNER, face_ < 0);
    }
    for (const Sentry& w : sentry_) {
        bool step = int(playT_ * 6.0f + w.x * 0.01f) & 1;
        world(art_.shadow, w.x, kFloor, 10, PAL_DUSK, false);
        world(step && w.stun <= 0 ? art_.sentry[1] : art_.sentry[0], w.x, w.stun > 0 ? kFloor - 4.0f : kFloor, 62,
              PAL_SENTRY, w.dir < 0);
        world(art_.lamp, w.x + (w.dir < 0 ? -12.0f : 12.0f), kFloor - 20.0f, 20, PAL_LAMP, false);
    }
    if (!has_) {
        world(art_.staff, bannerX_, kFloor, 86, PAL_SAND, false);
        float wave = std::sin(t_ * 3.2f) * 2.0f;
        world(art_.banner, bannerX_ + wave, kFloor - 6.0f, 42, PAL_BANNER, false);
    }

    const float bags[] = {420, 700, 980, 1240, 1500, 1760};
    for (int i = 0; i < 6; i++) {
        float x = bags[i];
        world(art_.bag, x, kFloor, 22, PAL_SAND, false);
        if (i % 2 == 0) world(art_.wire, x + 40.0f, kFloor - 8.0f, 28, PAL_SENTRY, false);
        if (i % 3 == 0) world(art_.slit, x - 30.0f, kFloor - 92.0f, 18, PAL_CONCRETE, false);
    }
    world(art_.staff, 110.0f, kFloor, 96, PAL_SAND, false);
    world(art_.slit, 150.0f, kFloor - 70.0f, 28, PAL_CONCRETE, false);

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    int sec = int(watch_ + 0.999f);
    if (sec < 0) sec = 0;
    char clock[16];
    std::snprintf(clock, sizeof(clock), "WATCH %02d", sec);
    hud(16, 1, clock, watch_ < 12.0f ? PAL_BANNER : PAL_HUD);
    if (has_) hud(30, 1, "BANNER", PAL_BANNER);
    else hud(32, 1, "OUT", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("BUNKER BANN", 160, 62, 1.1f, PAL_HUD, 0);
        text("BRING THE BANNER BACK", 160, 92, 0.55f, PAL_LAMP, 0);
        text("MISS IT AND THE WATCH IS OVER", 160, 112, 0.42f, PAL_SAND, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 146, 0.7f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 90, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 74, 0.68f, PAL_BANNER, 0);
        text("AT THE BUNKER", 160, 100, 0.62f, PAL_HUD, 0);
    } else if (mode_ == Mode::Over) {
        text("THE WATCH IS OVER", 160, 78, 0.7f, PAL_HUD, 0);
        text("START TO TRY AGAIN", 160, 110, 0.5f, PAL_LAMP, 0);
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
    bool strike = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, strike);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, strike);
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
    cam_ = approach(cam_, want, 460.0f * DT);
    draw();
}

}  // namespace bunkerbann
