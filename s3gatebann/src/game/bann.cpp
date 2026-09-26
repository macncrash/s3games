#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace bann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 1960.0f;
constexpr float kFloor = 190.0f;
constexpr float kWin = 128.0f;
constexpr float kSpawn = 268.0f;
constexpr float kBanner0 = 1664.0f;
constexpr float kMinX = 48.0f;
constexpr float kMaxX = 1880.0f;
constexpr float kRun = 172.0f;
constexpr float kCarry = 132.0f;
constexpr float kAccel = 2600.0f;
constexpr float kBody = 20.0f;
constexpr float kLunge = 26.0f;
constexpr float kStrike = 52.0f;
constexpr float kGrab = 28.0f;
constexpr float kShove = 100.0f;

constexpr float kStakes[] = {360.0f, 640.0f, 980.0f, 1320.0f, 1540.0f};

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (swing_ > 0) return art_.swing;
    if (std::abs(vx_) > 22.0f) return (int(step_ / 6.0f) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

float Game::phaseU(const Sentry& s) const {
    float p = s.period > 0.1f ? s.period : 1.0f;
    float u = std::fmod(playT_ + s.offset, p) / p;
    if (u < 0) u += 1.0f;
    return u;
}

bool Game::pikeHits(const Sentry& s) const {
    if (s.stun > 0 || has_) return false;
    float u = phaseU(s);
    if (u < 0.70f || u >= 0.88f) return false;
    float rel = s.x - px_;
    if (s.dir > 0) return rel <= 12.0f && rel >= -kLunge;
    return rel >= -12.0f && rel <= kLunge;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1200.0f) return 2;
    return 3;
}

void Game::layout() {
    auto set = [&](int i, float a, float b, float sp, float off, float per, bool bearer) {
        Sentry& s = sentries_[i];
        s.minX = a;
        s.maxX = b;
        s.x = a;
        s.speed = sp;
        s.dir = 1.0f;
        s.stun = 0;
        s.offset = off;
        s.period = per;
        s.bearer = bearer;
    };
    set(0, 400.0f, 590.0f, 34.0f, 0.20f, 2.20f, false);
    set(1, 820.0f, 1040.0f, 30.0f, 1.10f, 2.05f, false);
    set(2, 1220.0f, 1420.0f, 36.0f, 0.55f, 2.30f, false);
    set(3, 1776.0f, 1868.0f, 22.0f, 0.40f, 2.50f, true);
}

void Game::resetRun() {
    layout();
    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    lives_ = 4;
    step_ = 0;
    strikeCd_ = swing_ = inv_ = stun_ = dropLock_ = 0;
    shake_ = 0;
    fan_ = -1;
    playT_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 150.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(520.0f, 0.05f, 0.06f);
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
    fan_ = 0;
    fanT_ = 0;
    shake_ = 0.4f;
    sys_->rumble(0.3f, 0.7f, 200);
    sys_->setLight(40, 180, 80);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    fan_ = 0;
    fanT_ = 0;
    sys_->rumble(0.8f, 0.25f, 180);
    sys_->setLight(180, 30, 24);
    sys_->apu.noiseBurst(0.4f, 160.0f, 0.28f);
}

void Game::hurt(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.15f;
    stun_ = 0.28f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 170.0f;
    px_ = std::clamp(px_ + away * 10.0f, kMinX, kMaxX);
    shake_ = 1.0f;
    sys_->apu.noiseBurst(0.42f, 480.0f, 0.14f);
    sys_->rumble(0.65f, 0.3f, 110);
    sys_->setLight(200, 40, 30);
    if (has_) {
        has_ = false;
        float drop = px_;
        if (drop < 300.0f) drop = 300.0f;
        bannerX_ = std::clamp(drop, 300.0f, 1720.0f);
        dropLock_ = 0.5f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& strike) {
    left = right = false;
    strike = false;
    const float goal = has_ ? (kWin - 24.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 3.0f) travel = 1;
    else if (goal < px_ - 3.0f) travel = -1;

    int threat = -1;
    float best = 1.0e9f;
    bool block = false;
    for (int i = 0; i < 4; i++) {
        const Sentry& s = sentries_[i];
        if (s.stun > 0) continue;
        float rel = s.x - px_;
        float ahead = travel > 0 ? rel : travel < 0 ? -rel : std::abs(rel);
        float u = phaseU(s);
        bool pike = false;
        if (!has_ && !s.bearer && u >= 0.58f && u < 0.90f) {
            float soonLeft = (0.90f - u) * s.period;
            float reach = kRun * soonLeft + 52.0f;
            pike = ahead > -10.0f && ahead < reach;
        }
        // Ahead is positive in the direction of travel. Negative means behind.
        bool chase = has_ && ahead > -56.0f && ahead < 78.0f && std::abs(rel) < 90.0f;
        if (!pike && !chase) continue;
        if (ahead < 60.0f && ahead > -56.0f && std::abs(rel) < best) {
            best = std::abs(rel);
            threat = i;
        }
        block = true;
    }
    if (threat >= 0 && strikeCd_ <= 0) {
        float rel = sentries_[threat].x - px_;
        if (!has_) face_ = rel >= 0.0f ? 1 : -1;
        strike = true;
        return;
    }
    if (block) return;
    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = -1;
    }
}

void Game::step(bool left, bool right, bool strike) {
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;
    if (strikeCd_ > 0) strikeCd_ -= DT;
    if (swing_ > 0) swing_ -= DT;

    const bool roused = has_;
    for (Sentry& s : sentries_) {
        if (s.stun > 0) {
            s.stun -= DT;
            continue;
        }
        if (roused) {
            s.dir = px_ >= s.x ? 1.0f : -1.0f;
            float spd = s.bearer ? 150.0f : 100.0f;
            s.x += s.dir * spd * DT;
        } else {
            s.x += s.dir * s.speed * DT;
            if (s.x >= s.maxX) {
                s.x = s.maxX;
                s.dir = -1.0f;
            } else if (s.x <= s.minX) {
                s.x = s.minX;
                s.dir = 1.0f;
            }
        }
        s.x = std::clamp(s.x, 240.0f, 1900.0f);
    }

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 360.0f * DT);
    } else {
        float target = 0;
        if (right && !left) target = has_ ? kCarry : kRun;
        if (left && !right) target = has_ ? -kCarry : -kRun;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }
    px_ += vx_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);

    if (std::abs(vx_) > 28.0f) {
        step_ += std::abs(vx_) * DT * 0.16f;
        if (step_ > 1000.0f) step_ -= 1000.0f;
    }

    if (has_ && px_ <= kWin) {
        win();
        return;
    }

    if (strike && strikeCd_ <= 0 && stun_ <= 0 && mode_ == Mode::Play) {
        strikeCd_ = 0.24f;
        swing_ = 0.14f;
        bool hit = false;
        for (Sentry& s : sentries_) {
            if (s.stun > 0) continue;
            float rel = s.x - px_;
            bool struck = has_ ? std::abs(rel) < kStrike
                               : (face_ > 0 ? (rel > -10.0f && rel < kStrike) : (rel < 10.0f && rel > -kStrike));
            if (!struck) continue;
            s.stun = 2.0f;
            float back = has_ ? kShove : -kShove;
            s.x = std::clamp(px_ + back, 300.0f, 1860.0f);
            hit = true;
        }
        if (hit) {
            blip(210.0f, 0.06f, 0.05f);
            sys_->apu.noiseBurst(0.18f, 900.0f, 0.05f);
            sys_->rumble(0.2f, 0.35f, 40);
        } else {
            blip(320.0f, 0.03f, 0.03f);
        }
    }

    if (inv_ <= 0 && stun_ <= 0 && mode_ == Mode::Play) {
        for (const Sentry& s : sentries_) {
            if (s.stun > 0) continue;
            bool body = has_ && std::abs(s.x - px_) < kBody;
            if (body || pikeHits(s)) {
                hurt(s.x);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && mode_ == Mode::Play && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(740.0f, 0.07f, 0.09f);
        sys_->rumble(0.25f, 0.5f, 90);
        sys_->setLight(200, 80, 30);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
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
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false, false);
        x += g.w * scale + scale;
    }
}

void Game::drawWorld(float view) {
    // Earlier sprites sit in front. Actors first, then the yard, then the sky.
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true) {
        spr(m, wx - view, foot, h, pal, flip, feet, false);
    };
    int flutter = int(t_ * 8.0f) & 1;
    int fi = int(t_ * 10.0f) & 1;

    bool show = inv_ <= 0 || (int(inv_ * 16.0f) & 1) == 0;
    if (show) {
        if (has_) world(art_.banner[flutter], px_ + face_ * 16.0f, kFloor - 18.0f, 58, PAL_BANNER, face_ < 0);
        spr(hero(), px_ - view, kFloor, 48, PAL_PLAYER, face_ < 0, true, false);
        spr(art_.shadow, px_ - view, kFloor + 3.0f, 8, PAL_FX, false, true, true);
    }
    if (!has_) world(art_.banner[flutter], bannerX_ + 10.0f, kFloor - 4.0f, 64, PAL_BANNER, false);

    for (const Sentry& s : sentries_) {
        int pose = 0;
        if (s.stun > 0.4f) pose = 2;
        else if (has_ || phaseU(s) >= 0.58f) pose = 1;
        world(art_.sentry[pose], s.x, kFloor, 54, PAL_SENTRY, s.dir < 0);
        spr(art_.shadow, s.x - view, kFloor + 3.0f, 8, PAL_FX, false, true, true);
    }

    world(art_.flame[fi], 230.0f, kFloor - 26.0f, 16, PAL_FX, false);
    world(art_.brazier, 230.0f, kFloor, 28, PAL_WOOD, false);
    for (float x : kStakes) world(art_.stake, x, kFloor, 40, PAL_WOOD, false);
    world(art_.tent, 1828.0f, kFloor, 48, PAL_BANNER, false);
    world(art_.pole, kBanner0, kFloor, 78, PAL_BANNER, false);
    world(art_.pennant, 148.0f, 70.0f, 16, PAL_PLAYER, false, false);
    world(art_.bars, 112.0f, 96.0f, 18, PAL_STONE, false, false);
    world(art_.lintel, 112.0f, 78.0f, 22, PAL_STONE, false, false);
    world(art_.door, 78.0f, kFloor, 70, PAL_WOOD, false);
    world(art_.tower, 48.0f, kFloor, 132, PAL_STONE, false);
    world(art_.tower, 176.0f, kFloor, 132, PAL_STONE, true);
    world(art_.slab, 112.0f, kFloor + 2.0f, 12, PAL_STONE, false);
    world(art_.sun, view + 236.0f, 58.0f, 22, PAL_FX, false, false);
}

void Game::drawPoster() {
    int flutter = int(t_ * 7.0f) & 1;
    int fi = int(t_ * 10.0f) & 1;
    text("S3 GATE BANN", 160, 16, 1.35f, PAL_HUD, 0);
    text("ONE GATE", 160, 42, 1.0f, PAL_FX, 0);
    text("BRING THE BANNER BACK", 160, 60, 1.0f, PAL_HUD, 0);
    if (int(t_ * 2.0f) & 1) text("START", 160, 128, 1.0f, PAL_HUD, 0);
    spr(art_.stand, 118, kFloor, 46, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 118, kFloor + 3, 8, PAL_FX, false, true, true);
    spr(art_.banner[flutter], 268, kFloor - 8, 70, PAL_BANNER, false, true, false);
    spr(art_.pole, 258, kFloor, 74, PAL_BANNER, false, true, false);
    spr(art_.flame[fi], 214, kFloor - 24, 14, PAL_FX, false, true, false);
    spr(art_.brazier, 214, kFloor, 26, PAL_WOOD, false, true, false);
    spr(art_.pennant, 146, 84, 14, PAL_PLAYER, false, false, false);
    spr(art_.bars, 110, 108, 16, PAL_STONE, false, false, false);
    spr(art_.lintel, 110, 92, 20, PAL_STONE, false, false, false);
    spr(art_.door, 78, kFloor, 64, PAL_WOOD, false, true, false);
    spr(art_.tower, 52, kFloor, 118, PAL_STONE, false, true, false);
    spr(art_.tower, 168, kFloor, 118, PAL_STONE, true, true, false);
    spr(art_.slab, 110, kFloor + 2, 12, PAL_STONE, false, true, false);
    spr(art_.sun, 250, 36, 20, PAL_FX, false, false, false);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float view = 0;
    if (mode_ != Mode::Title) {
        view = cam_;
        if (shake_ > 0) view += std::sin(t_ * 80.0f) * shake_ * 3.0f;
        view = std::clamp(view, 0.0f, kWorld - gs::SCREEN_W);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) {
            float u = y / 78.0f;
            c = gs::rgb4(2 + int(4 * u), 2 + int(2 * u), 8 - int(2 * u));
        } else if (y < 132) {
            float u = (y - 78) / 54.0f;
            c = gs::rgb4(6 + int(5 * u), 4 + int(2 * u), 6 - int(3 * u));
        } else if (y < 184) {
            float u = (y - 132) / 52.0f;
            c = gs::rgb4(11 - int(4 * u), 6 - int(2 * u), 3);
        } else {
            c = gs::rgb4(4, 3, 2);
        }
        vdp.lineBackdrop[y] = c;
        vdp.A.hscroll[y] = int16_t(std::lround(-view * 0.35f));
        vdp.B.hscroll[y] = int16_t(std::lround(-view));
        vdp.A.vscroll[y] = 0;
        vdp.B.vscroll[y] = 0;
        vdp.lineFog[y] = 0;
    }

    if (mode_ == Mode::Title) {
        drawPoster();
        hudC(26, "ARROWS MOVE   Z STRIKE   START", PAL_HUD);
        return;
    }

    if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 36, 1.15f, PAL_HUD, 0);
        text("THROUGH THE GATE", 160, 58, 1.0f, PAL_FX, 0);
    } else if (mode_ == Mode::Over) {
        text("THE YARD KEEPS IT", 160, 40, 1.15f, PAL_ALERT, 0);
        text("THE GATE IS EMPTY", 160, 62, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 48, 1.5f, PAL_HUD, 0);
    }
    drawWorld(view);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char lives[16];
        std::snprintf(lives, sizeof lives, "LIVES %d", lives_);
        hud(1, 0, lives, lives_ > 1 ? PAL_HUD : PAL_ALERT);
        hud(30, 0, has_ ? "BANNER" : "YARD", has_ ? PAL_ALERT : PAL_HUD);
        if (!has_) hudC(1, "TAKE THE BANNER", PAL_HUD);
        else hudC(1, px_ < 280.0f ? "THE GATE" : "BRING IT BACK", PAL_HUD);
        hud(1, 26, "ARROWS MOVE   Z STRIKE", PAL_HUD);
    } else if (!bot_) {
        hudC(26, "START", PAL_HUD);
    }
}

void Game::serviceAudio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (fan_ >= 0) {
        static const float good[] = {523.0f, 659.0f, 784.0f, 1046.0f};
        static const float bad[] = {196.0f, 155.0f, 123.0f, 98.0f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.045f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, has_ ? 73.0f : 55.0f, 0.018f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 82.0f, 0.014f);
    else sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    cam_ = 0;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(440.0f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            resetRun();
            mode_ = Mode::Title;
            t_ = 0;
        }
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Play) {
        playT_ += DT;
        bool left = false, right = false, strike = false;
        if (bot_) {
            bot(left, right, strike);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            strike = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Mode::Pause;
                blip(280.0f, 0.04f, 0.04f);
            }
        }
        if (mode_ == Mode::Play) step(left, right, strike);
        float lead = face_ * 28.0f;
        float want = std::clamp(px_ + lead - 150.0f, 0.0f, kWorld - gs::SCREEN_W);
        cam_ += (want - cam_) * std::min(1.0f, DT * 7.0f);
        if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.2f);
    }

    if (mode_ == Mode::Play && !has_) sys.setLight(170, 96, 42);
    else if (mode_ == Mode::Play && has_) sys.setLight(190, 64, 32);
    else if (mode_ == Mode::Title) sys.setLight(80, 48, 72);

    serviceAudio();
    draw();
}

}  // namespace bann
