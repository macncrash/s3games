#include "game/bedsmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace bedsmark {
namespace {

constexpr int N = 6;
constexpr float DT = 1.f / 60.f;
constexpr float SUN_SECS = 22.f;
constexpr float SPEED = 108.f;
constexpr float POUR = 1.15f;
constexpr float LOX = 8.f, HIX = 250.f, LOY = 52.f, HIY = 210.f;

struct Bed {
    float x, y, w, h, zx, zy, zw, zh;
    int crop;
};

constexpr Bed kBeds[N] = {
    {16, 58, 60, 40, 16, 100, 60, 16, 0},   {94, 58, 60, 40, 94, 100, 60, 16, 1},
    {172, 58, 60, 40, 172, 100, 60, 16, 2}, {16, 150, 60, 40, 16, 132, 60, 16, 0},
    {94, 150, 60, 40, 94, 132, 60, 16, 1},  {172, 150, 60, 40, 172, 132, 60, 16, 2},
};

bool overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

}  // namespace

bool Game::blocked(float x, float y) const {
    if (x < LOX || x > HIX || y < LOY || y > HIY) return true;
    for (int i = 0; i < N; i++) {
        const Bed& b = kBeds[i];
        if (overlap(x - 5.f, y - 3.f, 10.f, 4.f, b.x, b.y, b.w, b.h)) return true;
    }
    return false;
}

int Game::atBed() const {
    for (int i = 0; i < N; i++) {
        const Bed& b = kBeds[i];
        if (px_ >= b.zx && px_ < b.zx + b.zw && py_ >= b.zy && py_ < b.zy + b.zh) return i;
    }
    return -1;
}

void Game::move(float ax, float ay, float dt) {
    if (ax < -0.2f) face_ = -1;
    else if (ax > 0.2f) face_ = 1;
    if (ax != 0.f || ay != 0.f) walk_ += dt;
    else walk_ = 0;
    float step = SPEED * dt;
    float nx = px_ + ax * step;
    float ny = py_ + ay * step;
    if (!blocked(nx, py_)) px_ = nx;
    if (!blocked(px_, ny)) py_ = ny;
}

void Game::startRound() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    opened_ = false;
    lifted_ = false;
    finished_ = false;
    pourHeard_ = false;
    sunT_ = 0;
    walk_ = 0;
    for (int i = 0; i < N; i++) wet_[i] = 0;
    px_ = bot_ ? 202.f : 36.f;
    py_ = bot_ ? 108.f : 128.f;
    face_ = 1;
    bits_.clear();
}

void Game::lift() {
    if (lifted_) return;
    lifted_ = true;
    finished_ = true;
    opened_ = true;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    const Bed& b = kBeds[mark_];
    for (int k = 0; k < 8; k++) {
        float a = (k / 8.f) * 6.28318f;
        bits_.push_back({b.x + b.w * 0.5f, b.y + 8.f, std::cos(a) * 36.f, std::sin(a) * 22.f - 20.f, 0.55f, 1});
    }
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.noise(0, 2000, false);
    sys_->apu.tone(1, 523.f, 0.12f);
    sys_->apu.tone(2, 659.f, 0.1f);
    sys_->apu.tone(3, 784.f, 0.08f);
    sys_->rumble(0.3f, 0.5f, 140);
    sys_->setLight(40, 150, 60);
}

void Game::lose() {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    finished_ = false;
    sunT_ = SUN_SECS;
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.noise(0, 2000, false);
    sys_->apu.tone(1, 110.f, 0.16f);
    sys_->setLight(170, 40, 20);
}

void Game::put(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip) {
    if (!sys_ || w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    float p = std::clamp(sunT_ / SUN_SECS, 0.f, 1.f);
    if (mode_ == Mode::Title) p = 0.12f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 46) {
            int r = 2 + y / 8 + int(p * 8);
            int g = 4 + y / 16;
            int b = 12 - int(p * 4) - y / 20;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 5, 2);
        }
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    for (int cy = 6; cy < 28; cy++)
        for (int cx = 0; cx < 40; cx++) v.B.set(cx, cy, gs::entry(art_.grass, PAL_YARD));

    if (mode_ == Mode::Title) {
        put(art_.title, 70, 8, float(art_.title.w), float(art_.title.h), PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        put(art_.done, 48, 96, float(art_.done.w), float(art_.done.h), PAL_GOOD);
    }

    for (const Bit& d : bits_) {
        if (d.kind == 0) put(art_.drop, d.x, d.y, 6, 8, PAL_WATER);
        else put(art_.spark, d.x, d.y, 7, 7, PAL_GOLD);
    }

    int here = mode_ == Mode::Play ? atBed() : -1;
    for (int i = N - 1; i >= 0; i--) {
        const Bed& b = kBeds[i];
        float m = wet_[i];
        int stage = 0;
        if (mode_ == Mode::Lose && m < 1.f) stage = 2;
        else if (m >= 1.f) stage = 1;
        else if (m > 0.35f) stage = 0;
        float ph = stage == 1 ? 30.f : 18.f;
        put(art_.plant[b.crop][stage], b.x + 10, b.y - (stage == 1 ? 10.f : 2.f), 40, ph, PAL_PLANT);
        put(art_.soil, b.x + 8, b.y + 10, 44, 16, m >= 0.5f ? PAL_WET : PAL_DRY);
        put(art_.frame, b.x, b.y, b.w, b.h, PAL_WOOD);
        if (i == mark_ && !lifted_) {
            const gs::Mipped& st = opened_ ? art_.stakeUp : art_.stake;
            float bob = opened_ ? std::sin(t_ * 6.f) * 2.f : 0.f;
            float sh = opened_ ? 28.f : 18.f;
            put(st, b.x + b.w * 0.5f - 5.f, b.y + 4.f - bob - (opened_ ? 8.f : 0.f), 10, sh, PAL_GOLD);
        }
    }

    if (here == mark_ && mode_ == Mode::Play && wet_[mark_] < 1.f) {
        float bob = std::sin(t_ * 7.f) * 2.f;
        put(art_.drop, kBeds[mark_].x + 27, kBeds[mark_].y - 6 + bob, 6, 8, PAL_WATER);
    }

    int pose = (int(walk_ * 8.f) & 1) ? 1 : 0;
    float mx = mode_ == Mode::Title ? 28.f : px_;
    float my = mode_ == Mode::Title ? 128.f : py_;
    put(art_.man[pose], mx - 12, my - 26, 24, 26, PAL_MAN, face_ < 0);

    float sx = 30.f + p * 200.f;
    put(art_.sun, sx, 8, 22, 22, PAL_SUN);
    float c0 = std::fmod(t_ * 8.f, 380.f) - 40.f;
    put(art_.cloud, c0, 18, 36, 14, PAL_SUN);

    if (mode_ == Mode::Title) {
        hudC(24, "WATER THE MARKED BED", PAL_HUD);
        hudC(25, "THEN LIFT THE STAKE", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "ENTER", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        hudC(25, "THE MARK IS FINISHED", PAL_GOOD);
    } else if (mode_ == Mode::Lose) {
        hudC(25, "THE SUN HIT THE WALL", PAL_WARN);
    } else {
        hud(1, 0, opened_ ? "MARK OPEN" : "MARK SHUT", opened_ ? PAL_GOOD : PAL_GOLD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "SUN %d", int(p * 100));
        hud(30, 0, buf, p > 0.75f ? PAL_WARN : PAL_HUD);
        if (here == mark_ && wet_[mark_] >= 1.f) hudC(26, "PRESS A TO LIFT", PAL_GOOD);
        else if (here >= 0 && wet_[here] < 1.f) hudC(26, "HOLD A TO POUR", PAL_HUD);
        else hudC(26, "FIND THE GOLD STAKE", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.B.scroll(0, 0);
    sys.apu.setMaster(0.8f);
    sys.setLight(40, 90, 140);
    t_ = 0;
    if (bot_) startRound();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    rng_ = rng_ * 1664525u + 1013904223u;

    float ax = 0, ay = 0;
    bool pour = false;
    bool start = false;

    if (bot_) {
        if (mode_ == Mode::Play && !over_) {
            const Bed& b = kBeds[mark_];
            float tx = b.zx + b.zw * 0.5f;
            float ty = b.zy + b.zh * 0.5f;
            float dx = tx - px_;
            float dy = ty - py_;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 2.5f) {
                ax = dx / dist;
                ay = dy / dist;
            } else {
                px_ = tx;
                py_ = ty;
                pour = true;
            }
        }
    } else {
        const gs::Pad& pad = sys.pad;
        if (pad.pressed(gs::BTN_MODE) && mode_ == Mode::Title) sys.quit();
        start = pad.pressed(gs::BTN_START);
        ax = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        ay = float(pad.down(gs::BTN_DOWN)) - float(pad.down(gs::BTN_UP));
        if (std::fabs(pad.axisX) > 0.25f && ax == 0.f) ax = pad.axisX;
        if (std::fabs(pad.axisY) > 0.25f && ay == 0.f) ay = -pad.axisY;
        float mag = std::sqrt(ax * ax + ay * ay);
        if (mag > 1.f) {
            ax /= mag;
            ay /= mag;
        }
        pour = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z);
    }

    if (mode_ == Mode::Title) {
        if (start) startRound();
    } else if (mode_ == Mode::Play) {
        move(ax, ay, DT);
        int at = atBed();
        if (pour && at == mark_ && wet_[mark_] >= 1.f && opened_) {
            lift();
        } else if (pour && at >= 0 && wet_[at] < 1.f) {
            float before = wet_[at];
            wet_[at] = std::min(1.f, wet_[at] + POUR * DT);
            if (at == mark_ && before < 1.f && wet_[at] >= 1.f) opened_ = true;
            if ((rng_ & 3) == 0 && bits_.size() < 28) {
                const Bed& b = kBeds[at];
                bits_.push_back({px_, py_ - 14.f, (b.x + 20.f - px_) * 2.f, (b.y + 12.f - py_) * 2.f, 0.35f, 0});
            }
            sys.apu.tone(0, 180.f + wet_[at] * 240.f, 0.03f);
            sys.apu.noise(0.035f, 1400.f, false);
            pourHeard_ = true;
        } else if (pourHeard_) {
            sys.apu.tone(0, 0, 0);
            sys.apu.noise(0, 2000, false);
            pourHeard_ = false;
        }
        if (!over_) {
            sunT_ += DT;
            if (sunT_ >= SUN_SECS) lose();
        }
    }

    for (int i = int(bits_.size()) - 1; i >= 0; --i) {
        Bit& d = bits_[size_t(i)];
        d.life -= DT;
        d.vy += 40.f * DT;
        d.x += d.vx * DT;
        d.y += d.vy * DT;
        if (d.life <= 0.f) bits_.erase(bits_.begin() + i);
    }
    draw();
}

}  // namespace bedsmark
