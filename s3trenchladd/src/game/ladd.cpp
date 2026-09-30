#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace trenchladd {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2280.0f;
constexpr float kFloor = 172.0f;
constexpr float kSpawn = 96.0f;
constexpr float kLadder = 2088.0f;
constexpr float kMinX = 48.0f;
constexpr float kMaxX = 2160.0f;
constexpr float kRun = 158.0f;
constexpr float kDuck = 72.0f;
constexpr float kAccel = 2400.0f;
constexpr float kGrav = 860.0f;
constexpr float kJump = -320.0f;
constexpr float kWatch = 52.0f;
constexpr float kClimbNeed = 0.85f;

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (!grounded_) return art_.leap;
    if (ducked_) return art_.crouch;
    if (std::abs(vx_) > 18.0f) return (int(step_ / 8.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (won_ || mode_ == Mode::Victory) return 3;
    if (over_ || mode_ == Mode::Over) return 4;
    if (atLadder() || climb_ > 0.05f) return 3;
    for (const Gun& g : gun_) {
        if (gunHot(g, playT_) && px_ > g.a && px_ < g.b) return 2;
    }
    return 1;
}

bool Game::gunHot(const Gun& g, float t) const {
    float u = std::fmod(t + g.phase, g.period);
    if (u < 0) u += g.period;
    return u < g.on;
}

bool Game::inGap(float x) const {
    for (const Span& s : gap_)
        if (x > s.a && x < s.b) return true;
    return false;
}

bool Game::inGun(float x) const {
    for (const Gun& g : gun_)
        if (x > g.a && x < g.b) return true;
    return false;
}

bool Game::atLadder() const { return std::abs(px_ - kLadder) < 18.0f && grounded_ && py_ >= kFloor - 2.0f; }

void Game::resetRun() {
    gap_[0] = {440.0f, 490.0f};
    gap_[1] = {880.0f, 930.0f};
    gap_[2] = {1320.0f, 1370.0f};
    gap_[3] = {1740.0f, 1790.0f};
    gun_[0] = {560.0f, 720.0f, 2.6f, 1.15f, 0.2f};
    gun_[1] = {1020.0f, 1188.0f, 2.4f, 1.05f, 0.9f};
    gun_[2] = {1460.0f, 1620.0f, 2.8f, 1.2f, 0.4f};
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    face_ = 1;
    ducked_ = false;
    grounded_ = true;
    lives_ = 3;
    watch_ = kWatch;
    climb_ = 0;
    reason_ = "";
    step_ = coyote_ = jumpBuf_ = inv_ = shake_ = 0;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 110.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(390.0f, 0.05f, 0.08f);
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
    reason_ = "LADDER";
    vx_ = vy_ = 0;
    shake_ = 0.2f;
    sys_->rumble(0.2f, 0.45f, 160);
    sys_->setLight(40, 110, 40);
    blip(640.0f, 0.08f, 0.22f);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    reason_ = why;
    vx_ = vy_ = 0;
    sys_->rumble(0.6f, 0.2f, 150);
    sys_->setLight(120, 24, 10);
    sys_->apu.noiseBurst(0.32f, 110.0f, 0.22f);
}

void Game::fall() {
    if (inv_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 0.7f;
    float back = 70.0f;
    for (const Span& s : gap_) {
        if (px_ > s.a && px_ < s.b) {
            px_ = s.a - 90.0f;
            break;
        }
    }
    px_ = std::clamp(px_ - 0.0f, kMinX, kMaxX);
    (void)back;
    py_ = kFloor;
    vy_ = 0;
    vx_ = -40.0f;
    grounded_ = true;
    climb_ = 0;
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.4f, 240.0f, 0.14f);
    sys_->rumble(0.5f, 0.15f, 90);
    if (lives_ <= 0) lose("THE TRENCH TOOK YOU");
}

void Game::bot(bool& left, bool& right, bool& jump, bool& crouch, bool& climb) {
    left = right = jump = crouch = climb = false;
    if (atLadder() || px_ > kLadder - 14.0f) {
        climb = true;
        if (px_ < kLadder - 6.0f) right = true;
        else if (px_ > kLadder + 6.0f) left = true;
        return;
    }
    bool under = false;
    for (const Gun& g : gun_) {
        float pad = 22.0f;
        if (px_ > g.a - pad && px_ < g.b + pad) under = true;
    }
    crouch = under;
    bool holeSoon = false;
    for (const Span& s : gap_) {
        if (px_ > s.a - 40.0f && px_ < s.a - 20.0f && grounded_) holeSoon = true;
    }
    if (holeSoon && !under) jump = true;
    if (!under || !crouch) right = true;
    else right = true;
}

void Game::stepPlay(bool left, bool right, bool jump, bool crouch, bool climbHeld) {
    playT_ += DT;
    watch_ -= DT;
    if (watch_ <= 0.0f) {
        watch_ = 0;
        lose("THE WATCH IS OVER");
        return;
    }
    if (inv_ > 0) inv_ -= DT;
    if (jump) jumpBuf_ = 0.12f;
    else if (jumpBuf_ > 0) jumpBuf_ -= DT;

    ducked_ = crouch && grounded_ && climb_ <= 0.0f;
    if (grounded_) coyote_ = 0.1f;
    else if (coyote_ > 0) coyote_ -= DT;

    bool wantJump = jumpBuf_ > 0 && coyote_ > 0 && !ducked_ && climb_ <= 0;
    if (wantJump) {
        vy_ = kJump;
        grounded_ = false;
        coyote_ = 0;
        jumpBuf_ = 0;
        py_ -= 1.5f;
        blip(520.0f, 0.04f, 0.05f);
    }

    if (climb_ > 0.02f && atLadder()) {
        vx_ = approach(vx_, 0.0f, kAccel * DT);
    } else {
        float target = 0;
        if (right && !left) target = ducked_ ? kDuck : kRun;
        if (left && !right) target = ducked_ ? -kDuck : -kRun;
        if (!grounded_) target *= 1.0f;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }
    px_ += vx_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);

    vy_ += kGrav * DT;
    py_ += vy_ * DT;
    bool overHole = inGap(px_);
    if (py_ >= kFloor) {
        if (overHole && vy_ >= 0) {
            py_ = kFloor + 8.0f;
            fall();
            if (mode_ != Mode::Play) return;
        } else if (!overHole) {
            py_ = kFloor;
            vy_ = 0;
            grounded_ = true;
        }
    } else {
        grounded_ = false;
    }

    if (std::abs(vx_) > 16.0f && grounded_) step_ += std::abs(vx_) * DT * (ducked_ ? 0.14f : 0.22f);

    if (inv_ <= 0 && grounded_ && ducked_ == false && climb_ <= 0) {
        for (const Gun& g : gun_) {
            if (!gunHot(g, playT_)) continue;
            if (px_ > g.a && px_ < g.b) {
                lives_--;
                inv_ = 0.85f;
                vx_ = px_ < (g.a + g.b) * 0.5f ? -150.0f : 150.0f;
                px_ = std::clamp(px_ + (vx_ > 0 ? 10.0f : -10.0f), kMinX, kMaxX);
                shake_ = 0.55f;
                sys_->apu.noiseBurst(0.34f, 500.0f, 0.1f);
                sys_->rumble(0.4f, 0.2f, 70);
                if (lives_ <= 0) {
                    lose("THE GUN FOUND YOU");
                    return;
                }
                break;
            }
        }
    }

    if (atLadder() && climbHeld && grounded_) {
        vx_ = approach(vx_, 0, 800.0f * DT);
        if (std::abs(px_ - kLadder) < 10.0f && std::abs(vx_) < 40.0f) {
            climb_ += DT;
            if (climb_ >= kClimbNeed) win();
        }
    } else if (climb_ > 0 && grounded_ && py_ > kFloor - 1.0f) {
        climb_ = std::max(0.0f, climb_ - DT * 1.4f);
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
    vdp.B.scroll(int(view), 0);

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    const float bags[] = {240.f, 780.f, 1220.f, 1880.f};
    for (float x : bags) world(art_.bag, x, kFloor - 2.0f, 20, PAL_CHALK, false);

    for (const Span& s : gap_) world(art_.crater, (s.a + s.b) * 0.5f, kFloor + 6.0f, 18, PAL_MUD, false);

    for (const Gun& g : gun_) {
        bool hot = mode_ != Mode::Title && gunHot(g, playT_);
        world(art_.post, g.a, kFloor - 8.0f, 48, PAL_IRON, false);
        world(art_.post, g.b, kFloor - 8.0f, 48, PAL_IRON, true);
        if (hot) {
            float span = g.b - g.a;
            int n = std::max(2, int(span / 36.0f));
            for (int i = 0; i < n; i++) {
                float x = g.a + span * (i + 0.35f) / float(n);
                float bob = std::sin(t_ * 30.0f + i) * 3.0f;
                world(art_.tracer, x, kFloor - 34.0f + bob, 8, PAL_TRACE, i & 1);
            }
        }
    }

    float ladY = kFloor + 4.0f;
    world(art_.ladder, kLadder, ladY, 92, PAL_LADDER, false);
    world(art_.bag, kLadder + 28.0f, kFloor - 4.0f, 24, PAL_LADDER, false);

    float foot = py_;
    if (climb_ > 0.02f) foot = kFloor - climb_ * 36.0f;
    if ((inv_ <= 0) || (int(t_ * 16.0f) & 1)) {
        world(art_.shadow, px_, kFloor + 2.0f, 7, PAL_SKY, false);
        float hh = ducked_ ? 30.0f : (grounded_ ? 58.0f : 46.0f);
        world(hero(), px_, foot, hh, PAL_KHAKI, face_ < 0);
    }

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    int sec = int(watch_ + 0.999f);
    if (sec < 0) sec = 0;
    char clock[24];
    std::snprintf(clock, sizeof(clock), "WATCH %02d", sec);
    hud(14, 1, clock, watch_ < 12.0f ? PAL_TRACE : PAL_HUD);
    if (climb_ > 0.05f) hud(28, 1, "CLIMB", PAL_LADDER);
    else hud(27, 1, "TRENCH", PAL_HUD);
    if (ducked_ && mode_ == Mode::Play) hud(1, 2, "DOWN", PAL_TRACE);

    if (mode_ == Mode::Title) {
        text("TRENCH LADD", 160, 28, 1.0f, PAL_HUD, 0);
        text("REACH THE FAR LADDER", 160, 56, 0.5f, PAL_LADDER, 0);
        text("ANYTHING ELSE IS A LOSS", 160, 76, 0.4f, PAL_TRACE, 0);
        text("A JUMPS THE HOLES   DOWN UNDER THE GUN", 160, 98, 0.34f, PAL_MUD, 0);
        text("HOLD UP ON THE LADDER", 160, 116, 0.38f, PAL_CHALK, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 142, 0.6f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 48, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE FAR LADDER", 160, 36, 0.7f, PAL_LADDER, 0);
        text("THE TRENCH IS BEHIND YOU", 160, 60, 0.42f, PAL_HUD, 0);
    } else if (mode_ == Mode::Over) {
        text("THE TRENCH KEPT YOU", 160, 36, 0.55f, PAL_TRACE, 0);
        text(reason_ ? reason_ : "", 160, 58, 0.4f, PAL_HUD, 0);
        text("START TO TRY AGAIN", 160, 82, 0.4f, PAL_MUD, 0);
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
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP);
    bool climbHeld = pad.down(gs::BTN_UP) || pad.axisY > 0.45f || pad.down(gs::BTN_B);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, jump, crouch, climbHeld);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump, crouch, climbHeld);
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && start) {
            mode_ = Mode::Title;
            titleHold_ = 0;
            over_ = false;
            won_ = false;
            resetRun();
        }
    }

    float want = std::clamp(px_ - 120.0f, 0.0f, kWorld - float(gs::SCREEN_W));
    cam_ = approach(cam_, want, 640.0f * DT);
    draw();
}

}  // namespace trenchladd
