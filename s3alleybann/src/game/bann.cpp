#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace alleybann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 1960.0f;
constexpr float kFloor = 186.0f;
constexpr float kHome = 150.0f;
constexpr float kSpawn = 240.0f;
constexpr float kBanner0 = 1760.0f;
constexpr float kMinX = 70.0f;
constexpr float kMaxX = 1880.0f;
constexpr float kRun = 168.0f;
constexpr float kCarry = 132.0f;
constexpr float kAccel = 2400.0f;
constexpr float kBody = 22.0f;
constexpr float kStrike = 58.0f;
constexpr float kGrab = 30.0f;

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
    if (px_ > 1100.0f) return 2;
    return 3;
}

void Game::resetRun() {
    auto set = [&](int i, float a, float b, float sp) {
        Watch& w = watch_[i];
        w.minX = a;
        w.maxX = b;
        w.x = (a + b) * 0.5f;
        w.speed = sp;
        w.dir = (i & 1) ? -1.0f : 1.0f;
        w.stun = 0;
    };
    set(0, 460.0f, 720.0f, 38.0f);
    set(1, 900.0f, 1180.0f, 42.0f);
    set(2, 1320.0f, 1600.0f, 40.0f);
    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    lives_ = 4;
    step_ = strikeCd_ = swing_ = inv_ = stun_ = dropLock_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(480.0f, 0.05f, 0.07f);
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
    sys_->setLight(40, 160, 70);
    blip(660.0f, 0.08f, 0.2f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    sys_->rumble(0.7f, 0.2f, 160);
    sys_->setLight(160, 24, 20);
    sys_->apu.noiseBurst(0.35f, 140.0f, 0.25f);
}

void Game::stumble(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.05f;
    stun_ = 0.26f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 150.0f;
    px_ = std::clamp(px_ + away * 8.0f, kMinX, kMaxX);
    shake_ = 0.8f;
    sys_->apu.noiseBurst(0.36f, 420.0f, 0.12f);
    sys_->rumble(0.55f, 0.25f, 90);
    if (has_) {
        has_ = false;
        bannerX_ = std::clamp(px_, 320.0f, 1680.0f);
        dropLock_ = 0.45f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& strike) {
    left = right = strike = false;
    const float goal = has_ ? (kHome - 16.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 4.0f) travel = 1;
    else if (goal < px_ - 4.0f) travel = -1;

    int threat = -1;
    float best = 1.0e9f;
    for (int i = 0; i < 3; i++) {
        const Watch& w = watch_[i];
        if (w.stun > 0) continue;
        float rel = w.x - px_;
        float ahead = travel == 0 ? std::abs(rel) : (travel > 0 ? rel : -rel);
        if (ahead < -16.0f || ahead > 72.0f) continue;
        if (std::abs(rel) < best) {
            best = std::abs(rel);
            threat = i;
        }
    }
    if (threat >= 0 && strikeCd_ <= 0.0f) {
        float rel = watch_[threat].x - px_;
        face_ = rel >= 0.0f ? 1 : -1;
        strike = true;
        return;
    }
    if (threat >= 0 && best < 48.0f) {
        if (watch_[threat].x >= px_) left = true;
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
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;
    if (strikeCd_ > 0) strikeCd_ -= DT;
    if (swing_ > 0) swing_ -= DT;

    const bool roused = has_;
    for (Watch& w : watch_) {
        if (w.stun > 0) {
            w.stun -= DT;
            continue;
        }
        if (roused) {
            w.dir = px_ >= w.x ? 1.0f : -1.0f;
            w.x += w.dir * 86.0f * DT;
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
        w.x = std::clamp(w.x, 280.0f, 1840.0f);
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
        strikeCd_ = 0.28f;
        swing_ = 0.12f;
        bool hit = false;
        for (Watch& w : watch_) {
            if (w.stun > 0) continue;
            float rel = w.x - px_;
            bool struck = face_ > 0 ? (rel > -12.0f && rel < kStrike) : (rel < 12.0f && rel > -kStrike);
            if (!struck) continue;
            w.stun = 1.7f;
            float shove = face_ > 0 ? 96.0f : -96.0f;
            w.x = std::clamp(w.x + shove, 300.0f, 1820.0f);
            hit = true;
        }
        if (hit) {
            blip(190.0f, 0.06f, 0.05f);
            sys_->apu.noiseBurst(0.16f, 820.0f, 0.05f);
        } else {
            blip(280.0f, 0.03f, 0.03f);
        }
    }

    if (inv_ <= 0 && stun_ <= 0) {
        for (const Watch& w : watch_) {
            if (w.stun > 0) continue;
            if (std::abs(w.x - px_) < kBody) {
                stumble(w.x);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(720.0f, 0.07f, 0.1f);
        sys_->rumble(0.2f, 0.45f, 80);
        sys_->setLight(180, 70, 30);
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
    vdp.A.scroll(int(view * 0.35f), 0);
    vdp.B.scroll(int(view * 0.85f), 0);

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    if ((inv_ <= 0) || (int(t_ * 18.0f) & 1)) {
        world(art_.shadow, px_, kFloor, 10, PAL_NIGHT, false);
        world(hero(), px_, kFloor, 62, PAL_COAT, face_ < 0);
        if (has_) world(art_.banner, px_ + face_ * 16.0f, kFloor - 28.0f, 34, PAL_BANNER, face_ < 0);
    }
    for (const Watch& w : watch_) {
        bool step = int(playT_ * 6.0f + w.x * 0.01f) & 1;
        world(art_.shadow, w.x, kFloor, 10, PAL_NIGHT, false);
        world(step && w.stun <= 0 ? art_.watcher[1] : art_.watcher[0], w.x, w.stun > 0 ? kFloor - 4.0f : kFloor, 60,
              PAL_WATCH, w.dir < 0);
        world(art_.lamp, w.x + (w.dir < 0 ? -10.0f : 10.0f), kFloor - 18.0f, 22, PAL_LAMP, false);
    }
    if (!has_) {
        world(art_.pole, bannerX_, kFloor, 78, PAL_WOOD, false);
        float wave = std::sin(t_ * 3.0f) * 2.0f;
        world(art_.banner, bannerX_ + wave, kFloor - 8.0f, 40, PAL_BANNER, false);
    }

    const float props[] = {340, 560, 820, 1040, 1280, 1500, 1688};
    for (int i = 0; i < 7; i++) {
        float x = props[i];
        if (i & 1) world(art_.bin, x, kFloor, 28, PAL_WOOD, false);
        else world(art_.crate, x, kFloor, 24, PAL_WOOD, false);
        if (i % 3 == 0) world(art_.fire, x - 18.0f, kFloor - 86.0f - (i % 2) * 10.0f, 16, PAL_LAMP, false);
    }
    world(art_.pole, 120.0f, kFloor, 90, PAL_WOOD, false);

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    if (has_) hud(28, 1, "BANNER", PAL_BANNER);
    else hud(30, 1, "EMPTY", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("ALLEY BANN", 160, 64, 1.15f, PAL_HUD, 0);
        text("BRING THE BANNER BACK", 160, 92, 0.55f, PAL_LAMP, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 140, 0.7f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 90, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 78, 0.7f, PAL_BANNER, 0);
    } else if (mode_ == Mode::Over) {
        text("THE ALLEY KEPT IT", 160, 78, 0.7f, PAL_HUD, 0);
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
    cam_ = approach(cam_, want, 420.0f * DT);
    draw();
}

}  // namespace alleybann
