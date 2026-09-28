#include "game/parademark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace parademark {
namespace {

constexpr float MARK_X = 214.f;
constexpr float MARK_Y = 28.f;
constexpr float START_X = 150.f;
constexpr float START_Y = 202.f;
constexpr float SPD = 1.7f;
constexpr float X_MIN = 28.f;
constexpr float X_MAX = 292.f;

const float kBands[] = {202.f, 150.f, 114.f, 78.f, 42.f, 28.f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::March: return "march";
    case Mode::Pause: return "pause";
    case Mode::Win: return "finished";
    case Mode::Dead: return "dead";
    }
    return "march";
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    finished_ = false;
    lives_ = 3;
    rows_ = 0;
    hold_ = 0;
    inv_ = 0;
    flash_ = 0;
    fan_ = -1;
    px_ = START_X;
    py_ = START_Y;
    age_ = 0;
}

void Game::begin() {
    mode_ = Mode::March;
    over_ = false;
    won_ = false;
    finished_ = false;
    lives_ = 3;
    rows_ = 0;
    hold_ = 0;
    inv_ = 30;
    flash_ = 0;
    fan_ = -1;
    px_ = START_X;
    py_ = START_Y;
    frame_ = 0;
    committed_ = false;
    slide_ = 1.f;
    sys_->setLight(40, 80, 180);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 6, 8));
    lanes_[0] = {168.f, 0.55f, 210.f, 52.f, 24.f, PAL_RED};
    lanes_[1] = {132.f, -0.7f, 220.f, 50.f, 80.f, PAL_BLUE};
    lanes_[2] = {96.f, 0.6f, 214.f, 48.f, 36.f, PAL_RED};
    lanes_[3] = {60.f, -0.65f, 218.f, 50.f, 120.f, PAL_BLUE};
    toTitle();
}

bool Game::danger(float x, float y, int fr) const {
    const float hw = 7.f;
    const float hh = 8.f;
    for (const Lane& L : lanes_) {
        if (std::fabs(y - L.y) > 11.f + hh) continue;
        float origin = L.phase + float(fr) * L.speed;
        float first = std::fmod(origin, L.spacing);
        if (first < 0.f) first += L.spacing;
        for (float cx = first - L.spacing; cx < 360.f; cx += L.spacing) {
            if (std::fabs(x - cx) < L.width * 0.5f + hw) return true;
        }
    }
    return false;
}

bool Game::crossSafe(float x, float y0, float y1) const {
    int align = int(std::fabs(x - px_) / SPD) + 1;
    for (int a = 0; a <= align; a++) {
        float u = std::min(1.f, (a * SPD) / std::max(0.01f, std::fabs(x - px_)));
        float sx = px_ + (x - px_) * u;
        if (danger(sx, y0, frame_ + a)) return false;
    }
    float dist = std::fabs(y1 - y0);
    if (dist < 0.5f) return !danger(x, y1, frame_ + align);
    int frames = int(dist / SPD) + 2;
    for (int a = 0; a <= frames + 8; a++) {
        float u = std::min(1.f, (a * SPD) / dist);
        float y = y0 + (y1 - y0) * u;
        if (danger(x, y, frame_ + align + a)) return false;
    }
    return true;
}

void Game::hurt() {
    lives_--;
    flash_ = 24;
    inv_ = 40;
    hold_ = 0;
    px_ = START_X;
    py_ = START_Y;
    rows_ = 0;
    committed_ = false;
    sys_->apu.noiseBurst(0.45f, 1400.f, 0.18f);
    sys_->rumble(0.6f, 0.3f, 120);
    sys_->setLight(200, 30, 30);
    if (lives_ <= 0) {
        mode_ = Mode::Dead;
        over_ = true;
        won_ = false;
        finished_ = false;
    }
}

void Game::finish() {
    if (finished_) return;
    finished_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    fan_ = 0;
    px_ = MARK_X;
    py_ = MARK_Y;
    sys_->setLight(40, 200, 70);
    sys_->rumble(0.2f, 0.45f, 160);
    sys_->apu.keyOn(2, 880.f, 0.22f);
}

void Game::humanStep() {
    const gs::Pad& p = sys_->pad;
    float x = 0.f, y = 0.f;
    if (p.down(gs::BTN_LEFT)) x -= 1.f;
    if (p.down(gs::BTN_RIGHT)) x += 1.f;
    if (p.down(gs::BTN_UP)) y -= 1.f;
    if (p.down(gs::BTN_DOWN)) y += 1.f;
    float m = std::sqrt(x * x + y * y);
    if (m > 1.f) {
        x /= m;
        y /= m;
    }
    float hurry = (p.down(gs::BTN_C) || p.down(gs::BTN_TURBO)) ? 1.35f : 1.f;
    if (x < -0.1f) face_ = -1.f;
    else if (x > 0.1f) face_ = 1.f;
    px_ += x * SPD * hurry;
    py_ += y * SPD * hurry;
}

void Game::botStep() {
    if (committed_) {
        if (std::fabs(commitX_ - px_) > 1.1f) {
            float nx = px_ + clampf(commitX_ - px_, -SPD, SPD);
            if (!danger(nx, py_, frame_)) px_ = nx;
            else committed_ = false;
            return;
        }
        px_ = commitX_;
        py_ += clampf(commitY_ - py_, -SPD, SPD);
        if (std::fabs(py_ - commitY_) < 1.2f) {
            py_ = commitY_;
            committed_ = false;
        }
        return;
    }
    int shelf = 0;
    for (int i = 0; i < 5; i++) {
        if (py_ <= kBands[i] + 8.f) shelf = i;
    }
    if (shelf >= 4) {
        float dx = clampf(MARK_X - px_, -SPD, SPD);
        float dy = clampf(MARK_Y - py_, -SPD, SPD);
        if (!danger(px_ + dx, py_, frame_ + 1)) px_ += dx;
        if (!danger(px_, py_ + dy, frame_ + 1)) py_ += dy;
        return;
    }
    float y1 = kBands[shelf + 1];
    float prefer = (shelf + 1 >= 4) ? MARK_X : px_;
    float bestX = px_;
    float bestD = 1e9f;
    bool found = false;
    for (int s = 0; s <= 28; s++) {
        int dir = (s == 0) ? 0 : ((s & 1) ? (s + 1) / 2 : -(s / 2));
        float x = clampf(px_ + float(dir) * 8.f, X_MIN, X_MAX);
        if (!crossSafe(x, py_, y1)) continue;
        float d = std::fabs(x - prefer);
        if (d < bestD) {
            bestD = d;
            bestX = x;
            found = true;
        }
    }
    if (found) {
        committed_ = true;
        commitX_ = bestX;
        commitY_ = y1;
        return;
    }
    float nx = clampf(px_ + slide_ * SPD, X_MIN, X_MAX);
    if (std::fabs(nx - px_) < 0.4f || danger(nx, py_, frame_) || danger(nx, py_, frame_ + 6)) slide_ = -slide_;
    else px_ = nx;
}

void Game::logic() {
    if (mode_ != Mode::March) return;
    frame_++;
    float ox = px_;
    if (bot_) botStep();
    else humanStep();
    px_ = clampf(px_, X_MIN, X_MAX);
    py_ = clampf(py_, 16.f, START_Y);
    if (px_ < ox - 0.2f) face_ = -1.f;
    else if (px_ > ox + 0.2f) face_ = 1.f;

    rows_ = 0;
    for (const Lane& L : lanes_) {
        if (py_ < L.y - 14.f) rows_++;
    }

    if (std::fabs(px_ - MARK_X) < 10.f && std::fabs(py_ - MARK_Y) < 8.f && !danger(px_, py_, frame_)) {
        hold_++;
        if (hold_ > 18) finish();
    } else {
        hold_ = 0;
    }

    if (inv_ > 0) inv_--;
    else if (mode_ == Mode::March && danger(px_, py_, frame_)) hurt();
    if (flash_ > 0) flash_--;
}

void Game::audio() {
    if (!sys_) return;
    int eighth = int(sys_->frame / 16);
    if (eighth == lastBeat_) return;
    lastBeat_ = eighth;
    if (mode_ == Mode::Pause || mode_ == Mode::Title) return;
    static const float fan[8] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 1046.5f, 1318.5f, 1046.5f};
    if (fan_ >= 0) {
        if (fan_ < 8) {
            sys_->apu.keyOn(1, fan[fan_], 0.18f);
            fan_++;
        }
        return;
    }
    if (mode_ == Mode::Dead) return;
    static const float mel[8] = {392.f, 440.f, 523.25f, 440.f, 349.23f, 392.f, 523.25f, 329.63f};
    sys_->apu.keyOn(1, mel[eighth & 7], 0.1f);
    if ((eighth & 1) == 0) sys_->apu.noiseBurst(0.05f, 5200.f, 0.04f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
    if (mode_ == Mode::Title) {
        age_++;
        if (start || (bot_ && age_ > 12)) begin();
    } else if (mode_ == Mode::March && start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Pause && start) {
        mode_ = held_;
    } else if ((mode_ == Mode::Dead) && (start || (bot_ && age_ > 30))) {
        toTitle();
    }
    if (mode_ == Mode::March || mode_ == Mode::Win || mode_ == Mode::Dead) age_++;
    logic();
    audio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x >= gs::SCREEN_W || s.y >= gs::SCREEN_H || s.x + s.w <= 0 || s.y + s.h <= 0) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 48) v.lineBackdrop[y] = gs::rgb4(3, 5, 4);
        else if (y < 196) {
            int stripe = ((y / 8) & 1) ? 0 : 1;
            v.lineBackdrop[y] = gs::rgb4(4 + stripe, 4 + stripe, 5);
        } else v.lineBackdrop[y] = gs::rgb4(5, 4, 3);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    backdrop();

    for (int i = 0; i < 5; i++) {
        float x = 24.f + i * 68.f;
        spr(art_.lamp, x, 188.f, 26.f, PAL_GOLD);
        spr(art_.flag, 18.f + i * 72.f, 16.f + ((i & 1) ? 2.f : 0.f), 20.f, PAL_FLAG, i & 1);
    }

    float pulse = 1.f + std::sin(age_ * 0.15f) * 0.06f;
    spr(art_.mark, MARK_X, MARK_Y, 20.f * pulse, PAL_GOLD);
    spr(art_.shadow, MARK_X, MARK_Y + 10.f, 8.f, PAL_INK, false, true);

    for (const Lane& L : lanes_) {
        float origin = L.phase + float(frame_) * L.speed;
        float first = std::fmod(origin, L.spacing);
        if (first < 0.f) first += L.spacing;
        for (float cx = first - L.spacing; cx < 360.f; cx += L.spacing) {
            spr(art_.shadow, cx, L.y + 10.f, 8.f, PAL_INK, false, true);
            spr(art_.float_, cx, L.y, 22.f, L.pal, L.speed < 0.f);
        }
    }

    for (int i = 0; i < 18; i++) {
        float t = age_ * 0.7f + i * 37.f;
        float x = std::fmod(20.f + i * 17.f + t * (0.4f + (i & 3) * 0.15f), 320.f);
        float y = std::fmod(12.f + i * 23.f + t * 0.55f, 210.f);
        spr(art_.confetti, x, y, 5.f, (i & 1) ? PAL_PAPER : PAL_GOLD);
    }

    bool blink = flash_ > 0 && ((flash_ / 3) & 1);
    if (!blink) {
        spr(art_.shadow, px_, py_ + 12.f, 8.f, PAL_INK, false, true);
        spr(art_.me, px_, py_, 26.f, PAL_ME, face_ < 0.f);
    }

    auto word = [&](const gs::Image& img, float cx, float cy, int pal) {
        if (img.w < 1) return;
        gs::Sprite s;
        s.img = img;
        s.w = img.w;
        s.h = img.h;
        s.x = int16_t(std::lround(cx - s.w * 0.5f));
        s.y = int16_t(std::lround(cy));
        s.pal = uint8_t(pal);
        sys_->vdp.sprite(s);
    };

    if (mode_ == Mode::Title) {
        word(art_.logo, 160.f, 70.f, PAL_GOLD);
        hudC(16, "MARCH TO THE GOLD MARK", PAL_INK);
        hudC(18, "STAND ON IT", PAL_INK);
        hudC(24, bot_ ? "MARCHING" : "START", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        word(art_.done, 160.f, 78.f, PAL_GOLD);
        hudC(16, "A FINISHED MARK ENDS IT", PAL_INK);
    } else if (mode_ == Mode::Dead) {
        word(art_.miss, 160.f, 78.f, PAL_RED);
        hudC(16, "THE MARK IS STILL OPEN", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_GOLD);
    }

    if (mode_ == Mode::March || mode_ == Mode::Pause) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hud(1, 1, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "ROWS %d", rows_);
        hud(30, 1, buf, PAL_INK);
        hudC(26, hold_ > 0 ? "ON THE MARK" : "THE GOLD MARK", PAL_GOLD);
    }
}

}  // namespace parademark
