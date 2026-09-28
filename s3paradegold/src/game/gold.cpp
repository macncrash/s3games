#include "game/gold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace paradegold {
namespace {

// Four gold faces of 10, counted double, are the line (80). The faces alone
// are 40, still short. Three golds are 60. Cream is the same face and cannot
// buy the double.
constexpr int kFace = 10;
constexpr int kLine = 80;
constexpr float SPD = 2.1f;
constexpr float X_MIN = 22.f;
constexpr float X_MAX = 298.f;

const float kY[5] = {208.f, 164.f, 120.f, 76.f, 34.f};
const float kGoldX[4] = {64.f, 240.f, 80.f, 220.f};

static_assert(4 * (kFace * 2) >= kLine && 4 * kFace < kLine && 3 * (kFace * 2) < kLine, "gold double is the leave");

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::March: return "march";
    case Mode::Pause: return "pause";
    case Mode::Win: return "double";
    case Mode::Dead: return "dead";
    }
    return "march";
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    goldOut_ = false;
    lives_ = 3;
    score_ = 0;
    bare_ = 0;
    golds_ = 0;
    cream_ = 0;
    age_ = 0;
    inv_ = 0;
    flash_ = 0;
    fan_ = -1;
    px_ = 160.f;
    py_ = kY[0];
    committed_ = false;
    for (Token& t : tok_) t.taken = false;
}

void Game::begin() {
    toTitle();
    mode_ = Mode::March;
    inv_ = 24;
    frame_ = 0;
    sys_->setLight(40, 70, 160);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(5, 6, 8));
    lanes_[0] = {186.f, 0.72f, 196.f, 48.f, 10.f};
    lanes_[1] = {142.f, -0.84f, 210.f, 46.f, 70.f};
    lanes_[2] = {98.f, 0.66f, 188.f, 50.f, 40.f};
    lanes_[3] = {55.f, -0.78f, 204.f, 46.f, 110.f};
    // Gold sits on the four shelves. Cream drifts on the same shelf, starting
    // at the far end so a straight march to the coin does not meet it.
    tok_[0] = {kGoldX[0], kY[1], 0.f, true, false};
    tok_[1] = {kGoldX[1], kY[2], 0.f, true, false};
    tok_[2] = {kGoldX[2], kY[3], 0.f, true, false};
    tok_[3] = {kGoldX[3], kY[4], 0.f, true, false};
    tok_[4] = {292.f, kY[1], -0.35f, false, false};
    tok_[5] = {28.f, kY[2], 0.32f, false, false};
    tok_[6] = {286.f, kY[3], -0.38f, false, false};
    tok_[7] = {30.f, kY[4], 0.3f, false, false};
    toTitle();
}

bool Game::danger(float x, float y, int fr) const {
    const float hw = 8.f;
    for (const Lane& L : lanes_) {
        if (std::fabs(y - L.y) > 15.f) continue;
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
    int frames = int(dist / SPD) + 2;
    for (int a = 0; a <= frames + 6; a++) {
        float u = std::min(1.f, (a * SPD) / std::max(0.5f, dist));
        float y = y0 + (y1 - y0) * u;
        if (danger(x, y, frame_ + align + a)) return false;
    }
    return true;
}

void Game::finish() {
    if (goldOut_) return;
    goldOut_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    fan_ = 0;
    sys_->setLight(255, 190, 40);
    sys_->rumble(0.25f, 0.55f, 140);
    sys_->apu.keyOn(2, 880.f, 0.2f);
}

void Game::takeToken(Token& t) {
    if (t.taken || mode_ != Mode::March) return;
    if (std::fabs(px_ - t.x) > 12.f || std::fabs(py_ - t.y) > 12.f) return;
    t.taken = true;
    if (t.gold) {
        score_ += kFace * 2;
        bare_ += kFace;
        golds_++;
        sys_->apu.noiseBurst(0.08f, 2400.f, 0.06f);
        sys_->setLight(255, 200, 40);
        if (score_ >= kLine && bare_ < kLine) finish();
        return;
    }
    if (score_ + kFace >= kLine) {
        cream_++;
        sys_->apu.noiseBurst(0.14f, 380.f, 0.07f);
        sys_->setLight(180, 150, 100);
        return;
    }
    score_ += kFace;
    bare_ += kFace;
    cream_++;
    sys_->apu.tone(0, 440.f, 0.05f);
}

void Game::hurt() {
    lives_--;
    flash_ = 22;
    inv_ = 36;
    committed_ = false;
    px_ = 160.f;
    py_ = kY[0];
    sys_->apu.noiseBurst(0.4f, 1200.f, 0.16f);
    sys_->rumble(0.55f, 0.25f, 100);
    sys_->setLight(200, 30, 30);
    if (lives_ <= 0) {
        mode_ = Mode::Dead;
        over_ = true;
        won_ = false;
        goldOut_ = false;
    }
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
    if (x < -0.1f) face_ = -1.f;
    else if (x > 0.1f) face_ = 1.f;
    px_ += x * SPD;
    py_ += y * SPD;
}

void Game::botStep() {
    if (committed_) {
        if (std::fabs(commitX_ - px_) > 1.2f) {
            float nx = px_ + clampf(commitX_ - px_, -SPD, SPD);
            if (!danger(nx, py_, frame_)) px_ = nx;
            else committed_ = false;
            return;
        }
        px_ = commitX_;
        py_ += clampf(commitY_ - py_, -SPD, SPD);
        if (std::fabs(py_ - commitY_) < 1.3f) {
            py_ = commitY_;
            committed_ = false;
        }
        return;
    }
    int shelf = 0;
    for (int i = 0; i < 5; i++) {
        if (py_ <= kY[i] + 5.f) shelf = i;
    }
    if (shelf >= 4) return;
    float y1 = kY[shelf + 1];
    float prefer = kGoldX[shelf];
    float bestX = px_;
    float bestD = 1e9f;
    bool found = false;
    for (int s = 0; s <= 36; s++) {
        int dir = (s == 0) ? 0 : ((s & 1) ? (s + 1) / 2 : -(s / 2));
        float x = clampf(prefer + float(dir) * 6.f, X_MIN, X_MAX);
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
    if (std::fabs(nx - px_) < 0.4f || danger(nx, py_, frame_) || danger(nx, py_, frame_ + 8)) slide_ = -slide_;
    else px_ = nx;
}

void Game::logic() {
    if (mode_ != Mode::March) return;
    frame_++;
    float ox = px_;
    if (bot_) botStep();
    else humanStep();
    px_ = clampf(px_, X_MIN, X_MAX);
    py_ = clampf(py_, 16.f, kY[0]);
    if (px_ < ox - 0.2f) face_ = -1.f;
    else if (px_ > ox + 0.2f) face_ = 1.f;

    for (int i = 4; i < 8; i++) {
        Token& t = tok_[i];
        if (t.taken) continue;
        t.x += t.vx;
        if (t.x < X_MIN) t.vx = std::fabs(t.vx);
        if (t.x > X_MAX) t.vx = -std::fabs(t.vx);
    }
    for (Token& t : tok_) takeToken(t);

    if (inv_ > 0) inv_--;
    else if (danger(px_, py_, frame_)) hurt();
    if (flash_ > 0) flash_--;
}

void Game::audio() {
    if (!sys_) return;
    int eighth = int(sys_->frame / 15);
    if (eighth == lastBeat_) return;
    lastBeat_ = eighth;
    if (mode_ == Mode::Pause || mode_ == Mode::Title || mode_ == Mode::Dead) return;
    static const float fan[8] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 659.25f, 880.f, 1174.7f};
    if (fan_ >= 0) {
        if (fan_ < 8) {
            sys_->apu.keyOn(1, fan[fan_], 0.16f);
            fan_++;
        }
        return;
    }
    static const float mel[8] = {349.23f, 440.f, 523.25f, 440.f, 392.f, 523.25f, 329.63f, 392.f};
    sys_->apu.keyOn(1, mel[eighth & 7], 0.08f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);
    if (mode_ == Mode::Title) {
        age_++;
        if (start || (bot_ && age_ > 10)) begin();
    } else if (mode_ == Mode::March && start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Pause && start) {
        mode_ = held_;
    } else if (mode_ == Mode::Dead && (start || (bot_ && age_ > 40))) {
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
        if (y < 28) v.lineBackdrop[y] = gs::rgb4(3, 5, 8);
        else if (y < 200) {
            int stripe = ((y / 10) & 1) ? 0 : 1;
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

    for (int i = 0; i < 6; i++) {
        spr(art_.flag, 16.f + i * 56.f, 14.f, 18.f, PAL_BUNT, i & 1);
    }
    for (const Lane& L : lanes_) {
        float origin = L.phase + float(frame_) * L.speed;
        float first = std::fmod(origin, L.spacing);
        if (first < 0.f) first += L.spacing;
        for (float cx = first - L.spacing; cx < 360.f; cx += L.spacing) {
            spr(art_.shadow, cx, L.y + 9.f, 7.f, PAL_INK, false, true);
            spr(art_.wagon, cx, L.y, 20.f, PAL_WAGON, L.speed < 0.f);
        }
    }
    float pulse = 1.f + std::sin(age_ * 0.18f) * 0.08f;
    for (const Token& t : tok_) {
        if (t.taken) continue;
        if (t.gold) {
            spr(art_.shadow, t.x, t.y + 8.f, 6.f, PAL_INK, false, true);
            spr(art_.coin, t.x, t.y, 16.f * pulse, PAL_GOLD);
        } else {
            spr(art_.cream, t.x, t.y, 12.f, PAL_CREAM);
        }
    }

    bool blink = flash_ > 0 && ((flash_ / 3) & 1);
    if (!blink && mode_ != Mode::Title) {
        spr(art_.shadow, px_, py_ + 12.f, 7.f, PAL_INK, false, true);
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

    char line[48];
    std::snprintf(line, sizeof(line), "SCORE %d  BARE %d  GOLD %d", score_, bare_, golds_);
    hud(1, 0, line, PAL_INK);
    std::snprintf(line, sizeof(line), "LIVES %d", lives_);
    hud(32, 0, line, PAL_INK);

    if (mode_ == Mode::Title) {
        word(art_.logo, 160.f, 64.f, PAL_GOLD);
        hudC(14, "A SHORT PARADE", PAL_INK);
        hudC(16, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(18, "CREAM IS ITS FACE", PAL_INK);
        hudC(21, "PRESS START", PAL_INK);
        spr(art_.me, 160.f, 168.f, 28.f, PAL_ME, false);
    } else if (mode_ == Mode::Win) {
        word(art_.done, 160.f, 8.f, PAL_GOLD);
        hudC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
    } else if (mode_ == Mode::Dead) {
        hudC(12, "THE STREET WINS", PAL_INK);
        hudC(14, "THE DOUBLE WAS NOT GOLD", PAL_CREAM);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_INK);
    } else {
        hudC(26, "GOLD X2   CREAM IS FACE", PAL_GOLD);
    }
}

}  // namespace paradegold
