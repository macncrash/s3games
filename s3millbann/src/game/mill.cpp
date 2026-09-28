#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace mill {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 1880.0f;
constexpr float kFloor = 188.0f;
constexpr float kWin = 168.0f;
constexpr float kSpawn = 210.0f;
constexpr float kBanner0 = 1620.0f;
constexpr float kMinX = 56.0f;
constexpr float kMaxX = 1800.0f;
constexpr float kRun = 176.0f;
constexpr float kCarry = 138.0f;
constexpr float kAccel = 2800.0f;
constexpr float kBody = 18.0f;
constexpr float kLunge = 24.0f;
constexpr float kStrike = 54.0f;
constexpr float kGrab = 30.0f;
constexpr float kWheel0 = 292.0f;
constexpr float kWheel1 = 448.0f;
constexpr float kWheelPer = 2.35f;
constexpr float kWatch = 52.0f;
constexpr float kSacks[] = {620.0f, 980.0f, 1340.0f};

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

float Game::phaseU(const Hand& h) const {
    float p = h.period > 0.1f ? h.period : 1.0f;
    float u = std::fmod(playT_ + h.offset, p) / p;
    if (u < 0) u += 1.0f;
    return u;
}

bool Game::rakeHits(const Hand& h) const {
    if (h.stun > 0 || has_) return false;
    float u = phaseU(h);
    if (u < 0.68f || u >= 0.86f) return false;
    float rel = h.x - px_;
    if (h.dir > 0) return rel <= 10.0f && rel >= -kLunge;
    return rel >= -10.0f && rel <= kLunge;
}

bool Game::wheelHot() const {
    float u = std::fmod(playT_, kWheelPer);
    if (u < 0) u += kWheelPer;
    return u >= 0.18f && u < 1.02f;
}

bool Game::inWheel(float x) const { return x > kWheel0 && x < kWheel1; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 980.0f) return 2;
    return 3;
}

void Game::layout() {
    auto set = [&](int i, float a, float b, float sp, float off, float per, bool bearer) {
        Hand& h = hands_[i];
        h.minX = a;
        h.maxX = b;
        h.x = a;
        h.speed = sp;
        h.dir = 1.0f;
        h.stun = 0;
        h.offset = off;
        h.period = per;
        h.bearer = bearer;
    };
    set(0, 520.0f, 740.0f, 32.0f, 0.15f, 2.15f, false);
    set(1, 900.0f, 1120.0f, 30.0f, 0.90f, 2.05f, false);
    set(2, 1240.0f, 1460.0f, 34.0f, 0.40f, 2.25f, false);
    set(3, 1688.0f, 1788.0f, 20.0f, 0.30f, 2.40f, true);
}

void Game::resetRun() {
    layout();
    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    lives_ = 4;
    watchLeft_ = int(kWatch);
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
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(480.0f, 0.05f, 0.06f);
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
    shake_ = 0.35f;
    sys_->rumble(0.25f, 0.65f, 180);
    sys_->setLight(40, 160, 70);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    fan_ = 0;
    fanT_ = 0;
    sys_->rumble(0.75f, 0.2f, 160);
    sys_->setLight(160, 28, 18);
    sys_->apu.noiseBurst(0.38f, 140.0f, 0.26f);
}

void Game::hurt(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.05f;
    stun_ = 0.26f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 160.0f;
    px_ = std::clamp(px_ + away * 12.0f, kMinX, kMaxX);
    shake_ = 1.0f;
    sys_->apu.noiseBurst(0.4f, 420.0f, 0.12f);
    sys_->rumble(0.6f, 0.25f, 90);
    sys_->setLight(190, 36, 24);
    if (has_) {
        has_ = false;
        float drop = std::clamp(px_, 500.0f, 1680.0f);
        bannerX_ = drop;
        dropLock_ = 0.45f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& strike) {
    left = right = false;
    strike = false;
    const float goal = has_ ? (kWin - 18.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 4.0f) travel = 1;
    else if (goal < px_ - 4.0f) travel = -1;

    float u = std::fmod(playT_, kWheelPer);
    if (u < 0) u += kWheelPer;
    bool hot = u >= 0.05f && u < 1.15f;
    bool crossing = (travel > 0 && px_ > kWheel0 - 36.0f && px_ < kWheel1) ||
                    (travel < 0 && px_ < kWheel1 + 36.0f && px_ > kWheel0);
    if (hot && crossing) travel = 0;
    if (inWheel(px_) && hot) travel = px_ < (kWheel0 + kWheel1) * 0.5f ? -1 : 1;

    int threat = -1;
    float best = 1.0e9f;
    for (int i = 0; i < 4; i++) {
        const Hand& h = hands_[i];
        if (h.stun > 0) continue;
        float rel = h.x - px_;
        float ahead = travel > 0 ? rel : travel < 0 ? -rel : std::abs(rel);
        float pu = phaseU(h);
        bool rake = !has_ && !h.bearer && pu >= 0.55f && pu < 0.90f && ahead > -8.0f && ahead < 70.0f;
        bool chase = has_ && ahead > -40.0f && ahead < 70.0f && std::abs(rel) < 84.0f;
        if (!rake && !chase) continue;
        if (std::abs(rel) < best) {
            best = std::abs(rel);
            threat = i;
        }
    }
    if (threat >= 0 && strikeCd_ <= 0) {
        float rel = hands_[threat].x - px_;
        float ahead = travel > 0 ? rel : travel < 0 ? -rel : std::abs(rel);
        if (ahead > -10.0f && ahead < (has_ ? 28.0f : 46.0f)) {
            if (!has_) face_ = rel >= 0.0f ? 1 : -1;
            strike = true;
        }
    }
    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = has_ ? face_ : -1;
        if (!has_) face_ = -1;
    }
}

void Game::step(bool left, bool right, bool strike) {
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;
    if (strikeCd_ > 0) strikeCd_ -= DT;
    if (swing_ > 0) swing_ -= DT;

    const bool roused = has_;
    for (Hand& h : hands_) {
        if (h.stun > 0) {
            h.stun -= DT;
            continue;
        }
        if (roused) {
            h.dir = px_ >= h.x ? 1.0f : -1.0f;
            float spd = h.bearer ? 96.0f : 70.0f;
            h.x += h.dir * spd * DT;
        } else {
            h.x += h.dir * h.speed * DT;
            if (h.x >= h.maxX) {
                h.x = h.maxX;
                h.dir = -1.0f;
            } else if (h.x <= h.minX) {
                h.x = h.minX;
                h.dir = 1.0f;
            }
        }
        h.x = std::clamp(h.x, 500.0f, 1820.0f);
    }

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 400.0f * DT);
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

    if (playT_ >= kWatch) {
        lose();
        return;
    }
    watchLeft_ = std::max(0, int(std::ceil(kWatch - playT_)));

    if (strike && strikeCd_ <= 0 && stun_ <= 0 && mode_ == Mode::Play) {
        strikeCd_ = 0.22f;
        swing_ = 0.13f;
        bool hit = false;
        int travel = vx_ >= 0 ? 1 : -1;
        if (left && !right) travel = -1;
        if (right && !left) travel = 1;
        for (Hand& h : hands_) {
            if (h.stun > 0) continue;
            float rel = h.x - px_;
            bool struck = has_ ? std::abs(rel) < kStrike
                               : (face_ > 0 ? (rel > -8.0f && rel < kStrike) : (rel < 8.0f && rel > -kStrike));
            if (!struck) continue;
            h.stun = 1.8f;
            float knock = travel >= 0 ? -96.0f : 96.0f;
            h.x = std::clamp(px_ + knock, 500.0f, 1800.0f);
            hit = true;
        }
        if (hit) {
            blip(190.0f, 0.06f, 0.05f);
            sys_->apu.noiseBurst(0.16f, 800.0f, 0.04f);
            sys_->rumble(0.15f, 0.3f, 36);
        } else {
            blip(300.0f, 0.03f, 0.03f);
        }
    }

    if (inv_ <= 0 && stun_ <= 0 && mode_ == Mode::Play && inWheel(px_) && wheelHot()) {
        hurt((kWheel0 + kWheel1) * 0.5f);
    }

    if (inv_ <= 0 && stun_ <= 0 && mode_ == Mode::Play) {
        for (const Hand& h : hands_) {
            if (h.stun > 0) continue;
            bool body = has_ && std::abs(h.x - px_) < kBody;
            if (body || rakeHits(h)) {
                hurt(h.x);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && mode_ == Mode::Play && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(700.0f, 0.07f, 0.08f);
        sys_->rumble(0.2f, 0.45f, 80);
        sys_->setLight(180, 90, 28);
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
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true) {
        spr(m, wx - view, foot, h, pal, flip, feet, false);
    };
    int flutter = int(t_ * 8.0f) & 1;
    int wi = int(t_ * 6.0f) & 1;
    bool show = inv_ <= 0 || (int(inv_ * 16.0f) & 1) == 0;
    if (show) {
        if (has_) world(art_.banner[flutter], px_ + face_ * 18.0f, kFloor - 16.0f, 56, PAL_BANNER, face_ < 0);
        spr(hero(), px_ - view, kFloor, 46, PAL_PLAYER, face_ < 0, true, false);
        spr(art_.shadow, px_ - view, kFloor + 2.0f, 8, PAL_FX, false, true, true);
    }
    if (!has_) world(art_.banner[flutter], bannerX_ + 8.0f, kFloor - 2.0f, 62, PAL_BANNER, false);

    for (const Hand& h : hands_) {
        int pose = 0;
        if (h.stun > 0.35f) pose = 2;
        else if (has_ || phaseU(h) >= 0.55f) pose = 1;
        world(art_.hand[pose], h.x, kFloor, 52, PAL_HAND, h.dir < 0);
        spr(art_.shadow, h.x - view, kFloor + 2.0f, 8, PAL_FX, false, true, true);
    }

    for (float x : kSacks) world(art_.sack, x, kFloor, 28, PAL_EARTH, false);
    world(art_.reed, 560.0f, kFloor, 36, PAL_FX, false);
    world(art_.reed, 1100.0f, kFloor, 34, PAL_FX, true);
    world(art_.race, (kWheel0 + kWheel1) * 0.5f, kFloor + 4.0f, 14, PAL_WOOD, false);
    world(art_.wheel[wi], (kWheel0 + kWheel1) * 0.5f, kFloor - 8.0f, wheelHot() ? 62 : 54, PAL_WOOD, false);
    world(art_.pole, kBanner0, kFloor, 76, PAL_BANNER, false);
    world(art_.mill, 96.0f, kFloor, 128, PAL_STONE, false);
    world(art_.sun, view + 250.0f, 46.0f, 20, PAL_FX, false, false);
}

void Game::drawPoster() {
    int flutter = int(t_ * 7.0f) & 1;
    int wi = int(t_ * 6.0f) & 1;
    text("S3 MILL BANN", 160, 14, 1.3f, PAL_HUD, 0);
    text("AT THE MILL", 160, 40, 1.0f, PAL_FX, 0);
    text("BRING THE BANNER BACK", 160, 58, 1.0f, PAL_HUD, 0);
    if (int(t_ * 2.0f) & 1) text("START", 160, 124, 1.0f, PAL_HUD, 0);
    spr(art_.mill, 78, kFloor, 110, PAL_STONE, false, true, false);
    spr(art_.wheel[wi], 168, kFloor - 6, 48, PAL_WOOD, false, true, false);
    spr(art_.stand, 214, kFloor, 44, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 214, kFloor + 2, 8, PAL_FX, false, true, true);
    spr(art_.banner[flutter], 286, kFloor - 6, 64, PAL_BANNER, false, true, false);
    spr(art_.pole, 274, kFloor, 70, PAL_BANNER, false, true, false);
    spr(art_.sack, 248, kFloor, 24, PAL_EARTH, false, true, false);
    spr(art_.sun, 250, 34, 18, PAL_FX, false, false, false);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float view = 0;
    if (mode_ != Mode::Title) {
        view = cam_;
        if (shake_ > 0) view += std::sin(t_ * 76.0f) * shake_ * 3.0f;
        view = std::clamp(view, 0.0f, kWorld - gs::SCREEN_W);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 70) {
            float u = y / 70.0f;
            c = gs::rgb4(3 + int(6 * u), 3 + int(3 * u), 8 - int(3 * u));
        } else if (y < 140) {
            float u = (y - 70) / 70.0f;
            c = gs::rgb4(9 + int(3 * u), 6 + int(u), 5 - int(2 * u));
        } else {
            c = gs::rgb4(5, 4, 2);
        }
        vdp.lineBackdrop[y] = c;
        vdp.A.hscroll[y] = int16_t(std::lround(-view * 0.28f));
        vdp.B.hscroll[y] = int16_t(std::lround(-view));
        vdp.A.vscroll[y] = 0;
        vdp.B.vscroll[y] = 0;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        drawPoster();
        hudC(26, "ARROWS MOVE   Z STRIKE   START", PAL_HUD);
        return;
    }

    if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 34, 1.1f, PAL_HUD, 0);
        text("AT THE MILL", 160, 56, 1.0f, PAL_FX, 0);
    } else if (mode_ == Mode::Over) {
        text("THE WATCH IS OVER", 160, 36, 1.1f, PAL_ALERT, 0);
        text("THE MILL IS EMPTY", 160, 58, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 46, 1.4f, PAL_HUD, 0);
    }
    drawWorld(view);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char lives[24];
        std::snprintf(lives, sizeof lives, "LIVES %d", lives_);
        hud(1, 0, lives, lives_ > 1 ? PAL_HUD : PAL_ALERT);
        char watch[24];
        std::snprintf(watch, sizeof watch, "WATCH %d", watchLeft_);
        hud(28, 0, watch, watchLeft_ > 12 ? PAL_HUD : PAL_ALERT);
        if (!has_) hudC(1, "TAKE THE BANNER", PAL_HUD);
        else hudC(1, px_ < 260.0f ? "THE MILL" : "BRING IT BACK", PAL_HUD);
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
        static const float good[] = {392.0f, 523.0f, 659.0f, 784.0f};
        static const float bad[] = {180.0f, 140.0f, 110.0f, 90.0f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.04f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, has_ ? 68.0f : 52.0f, 0.016f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 78.0f, 0.012f);
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
            blip(420.0f, 0.04f, 0.04f);
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
                blip(260.0f, 0.04f, 0.04f);
            }
        }
        if (mode_ == Mode::Play) step(left, right, strike);
        float lead = face_ * 24.0f;
        float want = std::clamp(px_ + lead - 150.0f, 0.0f, kWorld - gs::SCREEN_W);
        cam_ += (want - cam_) * std::min(1.0f, DT * 7.0f);
        if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 2.2f);
    }

    if (mode_ == Mode::Play && !has_) sys.setLight(160, 100, 40);
    else if (mode_ == Mode::Play && has_) sys.setLight(180, 70, 30);
    else if (mode_ == Mode::Title) sys.setLight(90, 50, 30);

    serviceAudio();
    draw();
}

}  // namespace mill
