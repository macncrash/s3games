#include "game/trench.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace trenchpouc {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2360.0f;
constexpr float kFloor = 168.0f;
constexpr float kGoal = 2140.0f;
constexpr float kSpawn = 200.0f;
constexpr float kPouch0 = 340.0f;
constexpr float kMinX = 80.0f;
constexpr float kMaxX = 2260.0f;
constexpr float kRun = 176.0f;
constexpr float kCarry = 142.0f;
constexpr float kDuck = 92.0f;
constexpr float kDuckCarry = 74.0f;
constexpr float kAccel = 2200.0f;
constexpr float kGrab = 28.0f;
constexpr float kWatch = 72.0f;
constexpr float kWireA = 1040.0f;
constexpr float kWireB = 1220.0f;

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
    if (won_) return 3;
    if (over_) return 4;
    if (!has_) return 1;
    return 2;
}

bool Game::flareHot(const Flare& f, float t) const {
    float u = std::fmod(t + f.phase, f.period);
    if (u < 0) u += f.period;
    return u < f.on;
}

bool Game::inMud(float x) const {
    return (x > 560.0f && x < 720.0f) || (x > 1560.0f && x < 1720.0f);
}

bool Game::inWire(float x) const { return x > kWireA && x < kWireB; }

void Game::resetRun() {
    flare_[0] = {480.0f, 700.0f, 3.2f, 0.4f, 0.85f};
    flare_[1] = {1280.0f, 1520.0f, 2.9f, 1.4f, 0.8f};
    flare_[2] = {1760.0f, 2020.0f, 3.5f, 0.6f, 0.9f};
    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    has_ = false;
    ducked_ = false;
    pouchX_ = kPouch0;
    lives_ = 3;
    watch_ = kWatch;
    reason_ = "";
    step_ = inv_ = stun_ = dropLock_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 120.0f, 0.0f, kWorld - gs::SCREEN_W);
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
    reason_ = "CROSSED";
    vx_ = 0;
    shake_ = 0.25f;
    sys_->rumble(0.2f, 0.5f, 160);
    sys_->setLight(40, 110, 50);
    blip(620.0f, 0.08f, 0.2f);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    reason_ = why;
    vx_ = 0;
    sys_->rumble(0.65f, 0.2f, 150);
    sys_->setLight(130, 28, 12);
    sys_->apu.noiseBurst(0.32f, 110.0f, 0.24f);
}

void Game::stumble(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 0.9f;
    stun_ = 0.2f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 130.0f;
    px_ = std::clamp(px_ + away * 12.0f, kMinX, kMaxX);
    shake_ = 0.65f;
    sys_->apu.noiseBurst(0.36f, 380.0f, 0.12f);
    sys_->rumble(0.45f, 0.15f, 80);
    if (has_) {
        has_ = false;
        pouchX_ = std::clamp(px_ - face_ * 36.0f, 160.0f, kGoal - 80.0f);
        dropLock_ = 0.45f;
    }
    if (lives_ <= 0) lose("THE TRENCH TOOK YOU");
}

void Game::bot(bool& left, bool& right, bool& crouch) {
    left = right = crouch = false;
    if (stun_ > 0) return;
    const float goal = has_ ? kGoal : pouchX_;
    int travel = 0;
    if (goal > px_ + 1.0f) travel = 1;
    else if (goal < px_ - 1.0f) travel = -1;

    float speed = has_ ? kDuckCarry : kDuck;
    float reach = speed * 0.4f + 28.0f;
    if (px_ > kWireA - reach && px_ < kWireB + reach) crouch = true;
    for (const Flare& f : flare_) {
        bool soon = false;
        for (float ahead = 0; ahead <= 0.5f; ahead += 0.1f)
            if (flareHot(f, playT_ + ahead)) soon = true;
        if (!soon) continue;
        if (px_ > f.a - reach && px_ < f.b + reach) crouch = true;
    }
    if (travel > 0) right = true;
    else if (travel < 0) left = true;
}

void Game::stepPlay(bool left, bool right, bool crouch) {
    playT_ += DT;
    watch_ -= DT;
    if (watch_ <= 0.0f) {
        watch_ = 0;
        lose("THE WATCH IS OVER");
        return;
    }
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;

    ducked_ = crouch && stun_ <= 0;

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 460.0f * DT);
    } else {
        float target = 0;
        if (right && !left) target = ducked_ ? (has_ ? kDuckCarry : kDuck) : (has_ ? kCarry : kRun);
        if (left && !right) target = -(ducked_ ? (has_ ? kDuckCarry : kDuck) : (has_ ? kCarry : kRun));
        if (inMud(px_)) target *= 0.48f;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }
    px_ += vx_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);
    if (std::abs(vx_) > 20.0f) step_ += std::abs(vx_) * DT * (ducked_ ? 0.12f : 0.2f);

    if (px_ >= kGoal) {
        if (has_) win();
        else lose("MISSED THE POUCH");
        return;
    }

    if (inv_ <= 0 && stun_ <= 0 && !ducked_) {
        if (inWire(px_)) {
            stumble((kWireA + kWireB) * 0.5f);
        } else {
            for (const Flare& f : flare_) {
                if (!flareHot(f, playT_)) continue;
                if (px_ > f.a && px_ < f.b) {
                    stumble((f.a + f.b) * 0.5f);
                    break;
                }
            }
        }
    }
    if (mode_ != Mode::Play) return;

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && std::abs(px_ - pouchX_) < kGrab) {
        has_ = true;
        blip(700.0f, 0.07f, 0.1f);
        sys_->rumble(0.15f, 0.35f, 70);
        sys_->setLight(140, 90, 30);
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
    if (shake_ > 0.02f) view += std::sin(t_ * 46.0f) * shake_ * 3.0f;
    vdp.A.scroll(int(view * 0.45f), 0);
    vdp.B.scroll(int(view), 0);

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    for (float x = 40.0f; x < kWorld; x += 48.0f) world(art_.plank, x, kFloor + 4.0f, 12, PAL_CHALK, false);

    const float bags[] = {260, 860, 1480, 1980};
    for (float x : bags) world(art_.bag, x, kFloor - 4.0f, 22, PAL_CHALK, false);

    const float muds[] = {640, 1640};
    for (float x : muds) world(art_.plank, x, kFloor + 8.0f, 10, PAL_MUD, false);

    world(art_.coil, (kWireA + kWireB) * 0.5f, kFloor - 2.0f, 22, PAL_WIRE, false);

    for (const Flare& f : flare_) {
        float mid = (f.a + f.b) * 0.5f;
        bool hot = mode_ != Mode::Title && flareHot(f, playT_);
        world(art_.stake, f.a, kFloor - 28.0f, 52, PAL_FLARE, false);
        world(art_.stake, f.b, kFloor - 28.0f, 52, PAL_FLARE, false);
        if (hot) {
            world(art_.burst, mid, kFloor - 78.0f, 28, PAL_FLARE, false);
            float span = f.b - f.a;
            int n = std::max(1, int(span / 80.0f));
            for (int i = 0; i < n; i++) {
                float x = f.a + span * (i + 0.5f) / float(n);
                world(art_.burst, x, kFloor - 46.0f, 14, PAL_FLARE, false);
            }
        }
    }

    world(art_.stake, kGoal, kFloor - 20.0f, 70, PAL_POUCH, false);
    world(art_.bag, kGoal + 18.0f, kFloor - 6.0f, 26, PAL_POUCH, false);

    if ((inv_ <= 0) || (int(t_ * 16.0f) & 1)) {
        world(art_.shadow, px_, kFloor, 8, PAL_SKY, false);
        float hh = ducked_ ? 34.0f : 64.0f;
        world(hero(), px_, kFloor, hh, PAL_KHAKI, face_ < 0);
        if (has_) world(art_.pouch, px_ + face_ * 12.0f, kFloor - (ducked_ ? 16.0f : 30.0f), 18, PAL_POUCH, face_ < 0);
    }
    if (!has_) {
        float bob = std::sin(t_ * 3.0f) * 1.5f;
        world(art_.pouch, pouchX_, kFloor - 8.0f + bob, 22, PAL_POUCH, false);
    }

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    int sec = int(watch_ + 0.999f);
    if (sec < 0) sec = 0;
    char clock[24];
    std::snprintf(clock, sizeof(clock), "WATCH %02d", sec);
    hud(14, 1, clock, watch_ < 12.0f ? PAL_FLARE : PAL_HUD);
    if (has_) hud(28, 1, "POUCH", PAL_POUCH);
    else hud(31, 1, "OUT", PAL_HUD);
    if (ducked_ && mode_ == Mode::Play) hud(1, 2, "DOWN", PAL_FLARE);

    if (mode_ == Mode::Title) {
        text("TRENCH POUC", 160, 30, 1.0f, PAL_HUD, 0);
        text("CARRY THE POUCH ACROSS", 160, 58, 0.48f, PAL_POUCH, 0);
        text("MISS THAT AND THE WATCH IS OVER", 160, 78, 0.38f, PAL_FLARE, 0);
        text("DOWN UNDER THE FLARE AND THE WIRE", 160, 100, 0.36f, PAL_MUD, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 128, 0.62f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 48, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE POUCH CROSSED", 160, 36, 0.62f, PAL_POUCH, 0);
        text("THE WATCH STILL RUNS", 160, 58, 0.5f, PAL_HUD, 0);
    } else if (mode_ == Mode::Over) {
        text("THE WATCH IS OVER", 160, 36, 0.62f, PAL_FLARE, 0);
        text(reason_ ? reason_ : "", 160, 58, 0.42f, PAL_HUD, 0);
        text("START TO TRY AGAIN", 160, 82, 0.42f, PAL_MUD, 0);
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
        else if (start || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
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

    float want = std::clamp(px_ - 140.0f, 0.0f, kWorld - float(gs::SCREEN_W));
    cam_ = approach(cam_, want, 520.0f * DT);
    draw();
}

}  // namespace trenchpouc
