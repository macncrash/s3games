#include "game/parade.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace paradeseven {
namespace {

constexpr float START_Y = 202.f;
constexpr float GOAL_Y = 30.f;
constexpr float YOU_SPD = 1.85f;
constexpr float THEM_SPD = 0.95f;
constexpr float X_MIN = 24.f;
constexpr float X_MAX = 296.f;
constexpr float YOU_X = 118.f;
constexpr float THEM_X = 210.f;

const float kBands[] = {202.f, 156.f, 118.f, 80.f, 42.f, 30.f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::March: return "march";
    case Mode::Pause: return "pause";
    case Mode::Win: return "seven";
    case Mode::Dead: return "lost";
    }
    return "march";
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    you_ = 0;
    them_ = 0;
    rows_ = 0;
    fan_ = -1;
    youW_ = {};
    themW_ = {};
    youW_.x = YOU_X;
    youW_.y = START_Y;
    themW_.x = THEM_X;
    themW_.y = START_Y;
    age_ = 0;
}

void Game::begin() {
    toTitle();
    mode_ = Mode::March;
    frame_ = 0;
    youW_.inv = 20;
    themW_.inv = 20;
    sys_->setLight(40, 80, 180);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 6, 8));
    lanes_[0] = {174.f, 0.48f, 230.f, 46.f, 20.f, PAL_RED};
    lanes_[1] = {136.f, -0.55f, 240.f, 44.f, 90.f, PAL_BLUE};
    lanes_[2] = {98.f, 0.5f, 236.f, 46.f, 40.f, PAL_RED};
    lanes_[3] = {60.f, -0.52f, 244.f, 44.f, 140.f, PAL_BLUE};
    toTitle();
}

bool Game::danger(float x, float y, int fr) const {
    const float hw = 6.f;
    const float hh = 7.f;
    for (const Lane& L : lanes_) {
        if (std::fabs(y - L.y) > 10.f + hh) continue;
        float origin = L.phase + float(fr) * L.speed;
        float first = std::fmod(origin, L.spacing);
        if (first < 0.f) first += L.spacing;
        for (float cx = first - L.spacing; cx < 360.f; cx += L.spacing) {
            if (std::fabs(x - cx) < L.width * 0.5f + hw) return true;
        }
    }
    return false;
}

bool Game::crossSafe(const Who& w, float x, float y0, float y1) const {
    float spd = (&w == &youW_) ? YOU_SPD : THEM_SPD;
    int align = int(std::fabs(x - w.x) / spd) + 1;
    for (int a = 0; a <= align; a++) {
        float u = std::min(1.f, (a * spd) / std::max(0.01f, std::fabs(x - w.x)));
        float sx = w.x + (x - w.x) * u;
        if (danger(sx, y0, frame_ + a)) return false;
    }
    float dist = std::fabs(y1 - y0);
    if (dist < 0.5f) return !danger(x, y1, frame_ + align);
    int frames = int(dist / spd) + 2;
    for (int a = 0; a <= frames + 10; a++) {
        float u = std::min(1.f, (a * spd) / dist);
        float y = y0 + (y1 - y0) * u;
        if (danger(x, y, frame_ + align + a)) return false;
    }
    return true;
}

void Game::arrive(bool you) {
    if (you) {
        you_++;
        rows_++;
        youW_.x = YOU_X;
        youW_.y = START_Y;
        youW_.committed = false;
        youW_.inv = 16;
        sys_->apu.keyOn(2, 660.f + you_ * 40.f, 0.16f);
        sys_->setLight(40, 180, 70);
        if (you_ >= 7 && you_ > them_) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            fan_ = 0;
            sys_->rumble(0.25f, 0.5f, 180);
        }
    } else {
        them_++;
        themW_.x = THEM_X;
        themW_.y = START_Y;
        themW_.committed = false;
        themW_.inv = 16;
        sys_->apu.keyOn(3, 330.f, 0.14f);
        sys_->setLight(40, 60, 200);
        if (them_ >= 7 && them_ > you_) {
            won_ = false;
            over_ = true;
            mode_ = Mode::Dead;
            sys_->rumble(0.5f, 0.2f, 140);
        }
    }
}

void Game::bump(Who& w, bool you) {
    w.flash = 18;
    w.inv = 28;
    w.committed = false;
    w.x = you ? YOU_X : THEM_X;
    w.y = START_Y;
    sys_->apu.noiseBurst(0.4f, 1200.f, 0.14f);
    if (you) sys_->rumble(0.45f, 0.2f, 80);
}

void Game::humanStep(Who& w) {
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
    float hurry = (p.down(gs::BTN_C) || p.down(gs::BTN_TURBO)) ? 1.3f : 1.f;
    if (x < -0.1f) w.face = -1.f;
    else if (x > 0.1f) w.face = 1.f;
    w.x += x * YOU_SPD * hurry;
    w.y += y * YOU_SPD * hurry;
}

void Game::stepWho(Who& w, bool rival) {
    float spd = rival ? THEM_SPD : YOU_SPD;
    float home = rival ? THEM_X : YOU_X;
    if (!rival && !bot_) {
        humanStep(w);
        return;
    }
    if (w.committed) {
        if (std::fabs(w.commitX - w.x) > 1.1f) {
            float nx = w.x + clampf(w.commitX - w.x, -spd, spd);
            if (!danger(nx, w.y, frame_)) w.x = nx;
            else w.committed = false;
            return;
        }
        w.x = w.commitX;
        w.y += clampf(w.commitY - w.y, -spd, spd);
        if (std::fabs(w.y - w.commitY) < 1.2f) {
            w.y = w.commitY;
            w.committed = false;
        }
        return;
    }
    int shelf = 0;
    for (int i = 0; i < 5; i++) {
        if (w.y <= kBands[i] + 6.f) shelf = i;
    }
    if (shelf >= 4) {
        float dx = clampf(home - w.x, -spd, spd);
        float dy = clampf(GOAL_Y - w.y, -spd, spd);
        if (!danger(w.x + dx, w.y, frame_ + 1)) w.x += dx;
        if (!danger(w.x, w.y + dy, frame_ + 1)) w.y += dy;
        return;
    }
    float y1 = kBands[shelf + 1];
    float prefer = (shelf + 1 >= 4) ? home : w.x;
    // The rival prefers the right curb so the two marchers do not share a gap.
    if (rival) prefer = clampf(home + (shelf & 1 ? 18.f : -12.f), X_MIN, X_MAX);
    float bestX = w.x;
    float bestD = 1e9f;
    bool found = false;
    int span = rival ? 18 : 32;
    for (int s = 0; s <= span; s++) {
        int dir = (s == 0) ? 0 : ((s & 1) ? (s + 1) / 2 : -(s / 2));
        float x = clampf(w.x + float(dir) * 8.f, X_MIN, X_MAX);
        if (!crossSafe(w, x, w.y, y1)) continue;
        float d = std::fabs(x - prefer);
        if (d < bestD) {
            bestD = d;
            bestX = x;
            found = true;
        }
    }
    if (found) {
        w.committed = true;
        w.commitX = bestX;
        w.commitY = y1;
        return;
    }
    float nx = clampf(w.x + w.slide * spd, X_MIN, X_MAX);
    if (std::fabs(nx - w.x) < 0.4f || danger(nx, w.y, frame_) || danger(nx, w.y, frame_ + 8)) w.slide = -w.slide;
    else w.x = nx;
}

void Game::logic() {
    if (mode_ != Mode::March) return;
    frame_++;
    float ox = youW_.x;
    float tx = themW_.x;
    stepWho(youW_, false);
    if (mode_ != Mode::March) return;
    stepWho(themW_, true);
    youW_.x = clampf(youW_.x, X_MIN, X_MAX);
    youW_.y = clampf(youW_.y, 16.f, START_Y);
    themW_.x = clampf(themW_.x, X_MIN, X_MAX);
    themW_.y = clampf(themW_.y, 16.f, START_Y);
    if (youW_.x < ox - 0.2f) youW_.face = -1.f;
    else if (youW_.x > ox + 0.2f) youW_.face = 1.f;
    if (themW_.x < tx - 0.2f) themW_.face = -1.f;
    else if (themW_.x > tx + 0.2f) themW_.face = 1.f;

    rows_ = you_;

    auto atCurb = [](const Who& w) { return w.y <= GOAL_Y + 2.f; };
    if (youW_.inv > 0) youW_.inv--;
    else if (danger(youW_.x, youW_.y, frame_)) bump(youW_, true);
    else if (atCurb(youW_)) arrive(true);

    if (mode_ != Mode::March) return;
    if (themW_.inv > 0) themW_.inv--;
    else if (danger(themW_.x, themW_.y, frame_)) bump(themW_, false);
    else if (atCurb(themW_)) arrive(false);

    if (youW_.flash > 0) youW_.flash--;
    if (themW_.flash > 0) themW_.flash--;
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
    } else if (mode_ == Mode::Dead && (start || (bot_ && age_ > 40))) {
        toTitle();
        if (bot_) over_ = true;
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
        if (y < 40) v.lineBackdrop[y] = gs::rgb4(3, 6, 4);
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
        spr(art_.lamp, 20.f + i * 70.f, 190.f, 24.f, PAL_GOLD);
        spr(art_.flag, 16.f + i * 74.f, 14.f + ((i & 1) ? 2.f : 0.f), 18.f, PAL_FLAG, i & 1);
    }
    spr(art_.curb, 160.f, 26.f, 16.f, PAL_GOLD);

    for (const Lane& L : lanes_) {
        float origin = L.phase + float(frame_) * L.speed;
        float first = std::fmod(origin, L.spacing);
        if (first < 0.f) first += L.spacing;
        for (float cx = first - L.spacing; cx < 360.f; cx += L.spacing) {
            spr(art_.shadow, cx, L.y + 10.f, 8.f, PAL_INK, false, true);
            spr(art_.float_, cx, L.y, 20.f, L.pal, L.speed < 0.f);
        }
    }

    if (mode_ == Mode::Win || mode_ == Mode::March) {
        for (int i = 0; i < 12; i++) {
            float t = age_ * 0.6f + i * 41.f;
            float x = std::fmod(18.f + i * 26.f + t * 0.35f, 320.f);
            float y = std::fmod(8.f + i * 19.f, 40.f);
            spr(art_.confetti, x, y, 5.f, (i & 1) ? PAL_PAPER : PAL_GOLD);
        }
    }

    auto body = [&](const Who& w, const gs::Mipped& img, int pal) {
        bool blink = w.flash > 0 && ((w.flash / 3) & 1);
        if (blink) return;
        spr(art_.shadow, w.x, w.y + 12.f, 8.f, PAL_INK, false, true);
        spr(img, w.x, w.y, 24.f, pal, w.face < 0.f);
    };
    body(themW_, art_.them, PAL_THEM);
    body(youW_, art_.you, PAL_YOU);

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

    char buf[40];
    if (mode_ == Mode::Title) {
        word(art_.logo, 160.f, 64.f, PAL_GOLD);
        hudC(15, "PLAY PARADE", PAL_INK);
        hudC(17, "FIRST TO SEVEN", PAL_GOLD);
        hudC(19, "REACH THE CURB", PAL_INK);
        hudC(24, bot_ ? "MARCHING" : "START", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        word(art_.done, 160.f, 72.f, PAL_GOLD);
        hudC(16, "YOU LEAVE AT SEVEN", PAL_YOU);
    } else if (mode_ == Mode::Dead) {
        word(art_.miss, 160.f, 72.f, PAL_THEM);
        hudC(16, "THEY WERE FIRST", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_GOLD);
    }

    if (mode_ != Mode::Title) {
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 1, buf, PAL_YOU);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(30, 1, buf, PAL_THEM);
        hudC(26, "FIRST TO SEVEN", PAL_GOLD);
    }
}

}  // namespace paradeseven
