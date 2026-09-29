#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace palbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 1960.0f;
constexpr float kFloor = 188.0f;
constexpr float kHome = 210.0f;
constexpr float kSpawn = 300.0f;
constexpr float kBanner0 = 1760.0f;
constexpr float kMinX = 96.0f;
constexpr float kMaxX = 1860.0f;
constexpr float kRun = 188.0f;
constexpr float kCarry = 176.0f;
constexpr float kAccel = 2400.0f;
constexpr float kJump = -520.0f;
constexpr float kGrav = 980.0f;
constexpr float kPitL = 540.0f;
constexpr float kPitR = 645.0f;
constexpr float kBody = 20.0f;
constexpr float kStrike = 58.0f;
constexpr float kGrab = 30.0f;
constexpr float kWatch = 78.0f;

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (swing_ > 0) return art_.thrust;
    if (std::abs(vx_) > 18.0f && onGround_) return (int(step_ / 7.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

bool Game::overPit() const { return px_ > kPitL && px_ < kPitR; }

bool Game::grounded() const { return onGround_; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > kPitR + 40.0f) return 2;
    return 3;
}

void Game::resetRun() {
    auto set = [&](int i, float a, float b, float sp) {
        Raider& w = raid_[i];
        w.minX = a;
        w.maxX = b;
        w.x = (a + b) * 0.5f;
        w.speed = sp;
        w.dir = (i & 1) ? -1.0f : 1.0f;
        w.stun = 0;
    };
    set(0, 860.0f, 1180.0f, 42.0f);
    set(1, 1320.0f, 1620.0f, 46.0f);
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    face_ = 1;
    has_ = false;
    onGround_ = true;
    bannerX_ = kBanner0;
    lives_ = 3;
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
    blip(380.0f, 0.05f, 0.08f);
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
    vx_ = vy_ = 0;
    shake_ = 0.3f;
    sys_->rumble(0.2f, 0.55f, 180);
    sys_->setLight(40, 120, 40);
    blip(640.0f, 0.08f, 0.22f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = vy_ = 0;
    sys_->rumble(0.65f, 0.15f, 160);
    sys_->setLight(120, 24, 12);
    sys_->apu.noiseBurst(0.32f, 140.0f, 0.22f);
}

void Game::hurt(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.1f;
    stun_ = 0.22f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 160.0f;
    vy_ = -160.0f;
    onGround_ = false;
    px_ = std::clamp(px_ + away * 6.0f, kMinX, kMaxX);
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.34f, 420.0f, 0.1f);
    sys_->rumble(0.5f, 0.2f, 80);
    if (has_) {
        has_ = false;
        float bank = px_ < (kPitL + kPitR) * 0.5f ? kPitL - 36.0f : kPitR + 36.0f;
        if (px_ > kPitR) bank = std::clamp(px_, kPitR + 40.0f, 1680.0f);
        else if (px_ < kPitL) bank = std::clamp(px_, 280.0f, kPitL - 30.0f);
        bannerX_ = bank;
        dropLock_ = 0.4f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& jump, bool& strike) {
    left = right = jump = strike = false;
    const float goal = has_ ? (kHome - 8.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 6.0f) travel = 1;
    else if (goal < px_ - 6.0f) travel = -1;

    if (onGround_) {
        if (travel > 0 && px_ > kPitL - 36.0f && px_ < kPitL - 8.0f) jump = true;
        if (travel < 0 && px_ < kPitR + 42.0f && px_ > kPitR + 14.0f) jump = true;
    }

    int threat = -1;
    float best = 1.0e9f;
    for (int i = 0; i < 2; i++) {
        const Raider& w = raid_[i];
        if (w.stun > 0) continue;
        float rel = w.x - px_;
        float ahead = travel == 0 ? std::abs(rel) : (travel > 0 ? rel : -rel);
        if (ahead < -14.0f || ahead > 72.0f) continue;
        if (std::abs(rel) < best) {
            best = std::abs(rel);
            threat = i;
        }
    }
    if (threat >= 0 && strikeCd_ <= 0.0f && onGround_) {
        float rel = raid_[threat].x - px_;
        face_ = rel >= 0.0f ? 1 : -1;
        strike = true;
        jump = false;
        return;
    }
    if (threat >= 0 && best < 46.0f && onGround_) {
        if (raid_[threat].x >= px_) left = true;
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

void Game::stepPlay(bool left, bool right, bool jump, bool strike) {
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
    for (Raider& w : raid_) {
        if (w.stun > 0) {
            w.stun -= DT;
            continue;
        }
        if (roused) {
            w.dir = px_ >= w.x ? 1.0f : -1.0f;
            w.x += w.dir * 78.0f * DT;
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
        w.x = std::clamp(w.x, kPitR + 40.0f, kMaxX - 20.0f);
    }

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 500.0f * DT);
    } else {
        float target = 0;
        if (right && !left) target = has_ ? kCarry : kRun;
        if (left && !right) target = has_ ? -kCarry : -kRun;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }

    if (jump && onGround_ && stun_ <= 0) {
        vy_ = kJump;
        onGround_ = false;
        blip(220.0f, 0.04f, 0.04f);
    }
    if (!onGround_) vy_ += kGrav * DT;
    px_ += vx_ * DT;
    py_ += vy_ * DT;

    bool floorHere = !overPit();
    if (floorHere && py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0;
        onGround_ = true;
    } else if (!floorHere && py_ >= kFloor) {
        onGround_ = false;
    }
    if (py_ > kFloor + 36.0f) {
        float side = vx_ >= 0 ? kPitL - 28.0f : kPitR + 28.0f;
        if (px_ > (kPitL + kPitR) * 0.5f && vx_ > 0) side = kPitR + 28.0f;
        if (px_ < (kPitL + kPitR) * 0.5f && vx_ < 0) side = kPitL - 28.0f;
        px_ = side;
        py_ = kFloor;
        vy_ = 0;
        vx_ = 0;
        onGround_ = true;
        hurt(px_ + (side < kPitL ? 40.0f : -40.0f));
        if (mode_ != Mode::Play) return;
    }

    px_ = std::clamp(px_, kMinX, kMaxX);
    if (std::abs(vx_) > 24.0f && onGround_) step_ += std::abs(vx_) * DT * 0.18f;

    if (has_ && onGround_ && px_ <= kHome) {
        win();
        return;
    }

    if (strike && strikeCd_ <= 0 && stun_ <= 0 && onGround_) {
        strikeCd_ = 0.28f;
        swing_ = 0.12f;
        bool hit = false;
        for (Raider& w : raid_) {
            if (w.stun > 0) continue;
            float rel = w.x - px_;
            bool struck = face_ > 0 ? (rel > -10.0f && rel < kStrike) : (rel < 10.0f && rel > -kStrike);
            if (!struck) continue;
            w.stun = 1.7f;
            float shove = face_ > 0 ? 90.0f : -90.0f;
            w.x = std::clamp(w.x + shove, kPitR + 50.0f, kMaxX - 16.0f);
            hit = true;
        }
        if (hit) {
            blip(160.0f, 0.06f, 0.05f);
            sys_->apu.noiseBurst(0.14f, 700.0f, 0.05f);
        } else {
            blip(240.0f, 0.03f, 0.03f);
        }
    }

    if (inv_ <= 0 && stun_ <= 0 && onGround_ && !overPit()) {
        for (const Raider& w : raid_) {
            if (w.stun > 0) continue;
            if (std::abs(w.x - px_) < kBody && std::abs(py_ - kFloor) < 8.0f) {
                hurt(w.x);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && onGround_ && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(720.0f, 0.07f, 0.1f);
        sys_->rumble(0.15f, 0.4f, 70);
        sys_->setLight(150, 40, 18);
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
    if (s.x > gs::SCREEN_W + 48 || s.y > gs::SCREEN_H + 48 || s.x + s.w < -48 || s.y + s.h < -48) return;
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
    vdp.B.scroll(int(view), 0);

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    const float stakes[] = {70, 108, 146, 184, 248, 286};
    for (float x : stakes) world(art_.stake, x, kFloor + 6.0f, 108, PAL_TIMBER, false);
    world(art_.gate, 214.0f, kFloor + 2.0f, 78, PAL_TIMBER, false);

    for (float x = kPitL + 24.0f; x < kPitR; x += 46.0f) world(art_.mudpad, x, kFloor + 18.0f, 22, PAL_DITCH, false);
    for (float x = kPitL + 16.0f; x < kPitR; x += 28.0f) {
        float sway = std::sin(t_ * 2.4f + x * 0.05f) * 3.0f;
        world(art_.reed, x + sway, kFloor + 4.0f, 26, PAL_DITCH, false);
    }

    if ((inv_ <= 0) || (int(t_ * 18.0f) & 1)) {
        world(art_.shadow, px_, py_ + 2.0f, 8, PAL_FIELD, false);
        world(hero(), px_, py_, 66, PAL_CLOAK, face_ < 0);
        if (has_) world(art_.cloth, px_ + face_ * 14.0f, py_ - 34.0f, 30, PAL_CLOTH, face_ < 0);
    }
    for (const Raider& w : raid_) {
        bool step = int(playT_ * 6.0f + w.x * 0.02f) & 1;
        world(art_.shadow, w.x, kFloor + 2.0f, 8, PAL_FIELD, false);
        world(step && w.stun <= 0 ? art_.raider[1] : art_.raider[0], w.x, w.stun > 0 ? kFloor - 6.0f : kFloor, 58,
              PAL_RAID, w.dir < 0);
    }
    if (!has_) {
        world(art_.pole, bannerX_, kFloor, 92, PAL_TIMBER, false);
        float wave = std::sin(t_ * 3.0f) * 3.0f;
        world(art_.cloth, bannerX_ + 8.0f + wave, kFloor - 48.0f, 34, PAL_CLOTH, false);
    }

    std::string marks;
    for (int i = 0; i < lives_; i++) marks += "I ";
    hud(1, 1, marks, PAL_HUD);
    int sec = int(watch_ + 0.999f);
    if (sec < 0) sec = 0;
    char clock[20];
    std::snprintf(clock, sizeof(clock), "WATCH %02d", sec);
    hud(15, 1, clock, watch_ < 12.0f ? PAL_CLOTH : PAL_HUD);
    if (has_) hud(28, 1, "BANNER", PAL_CLOTH);
    else hud(31, 1, "FIELD", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("PALISADE BANN", 160, 58, 1.0f, PAL_HUD, 0);
        text("BRING THE BANNER BACK", 160, 90, 0.52f, PAL_CLOTH, 0);
        text("ANYTHING ELSE IS A LOSS", 160, 110, 0.42f, PAL_TIMBER, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 148, 0.68f, PAL_HUD, 0);
        hud(2, 25, "ARROWS MOVE", PAL_HUD);
        hud(22, 25, "A JUMP  B SPEAR", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 88, 1.0f, PAL_HUD, 0);
        text("START RESUMES", 160, 116, 0.48f, PAL_TIMBER, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 72, 0.66f, PAL_CLOTH, 0);
        text("AT THE PALISADE", 160, 98, 0.58f, PAL_HUD, 0);
    } else if (mode_ == Mode::Over) {
        text("THE WATCH IS OVER", 160, 76, 0.66f, PAL_HUD, 0);
        text("START TO TRY AGAIN", 160, 108, 0.48f, PAL_TIMBER, 0);
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
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
    bool strike = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, jump, strike);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump, strike);
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
    cam_ = approach(cam_, want, 520.0f * DT);
    draw();
}

}  // namespace palbann
