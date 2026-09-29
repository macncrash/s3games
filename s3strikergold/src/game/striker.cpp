#include "striker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace strikergold {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float TAU = 6.2831853f;
constexpr float PERIOD = 1.62f;
constexpr float GRAV = 3.2f;
constexpr float MAN_X = 96.f;
constexpr float MAN_Y = 172.f;

}  // namespace

float Game::meter() const { return 0.5f * (1.f - std::cos(phase_)); }

int Game::pose() const {
    if (mode_ == Mode::Strike) return modeT_ < 0.12f ? 1 : 2;
    if (mode_ == Mode::Rise) return 2;
    return 0;
}

int Game::markOf(float power) const {
    if (power >= kGoldAt) return kGold;
    if (power >= 0.64f) return 2;
    if (power >= 0.40f) return 1;
    if (power >= 0.18f) return 0;
    return -1;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::img(const gs::Image& im, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = im;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::meterBar(int row) {
    const int cols = 28;
    const int x0 = 6;
    float m = (mode_ == Mode::Ready || mode_ == Mode::Title) ? meter() : power_;
    int mark = std::clamp(int(std::lround(m * (cols - 1))), 0, cols - 1);
    int gold = int(std::ceil(kGoldAt * (cols - 1)));
    for (int i = 0; i < cols; i++) {
        char ch[2] = {char(i == mark ? 'O' : (i >= gold ? '=' : '-')), 0};
        int pal = i >= gold ? PAL_GOLD : PAL_CREAM;
        if (i == mark) pal = m >= kGoldAt ? PAL_GOLD : PAL_RED;
        hud(x0 + i, row, ch, pal);
    }
}

void Game::blip(bool high) { sys_->apu.tone(1, high ? 880.f : 330.f, 0.06f); }

void Game::thunk() {
    sys_->apu.noiseBurst(0.5f, 120.f, 0.1f);
    sys_->rumble(0.55f, 0.25f, 70);
}

void Game::chime() {
    sys_->apu.tone(2, 660.f, 0.14f);
    sys_->apu.tone(3, 990.f, 0.12f);
    sys_->setLight(220, 170, 40);
}

void Game::whistle(float h) {
    if (h < 0.02f) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    sys_->apu.tone(0, 160.f + h * 820.f, 0.04f);
}

void Game::begin() {
    leftSwings_ = 3;
    used_ = 0;
    landed_ = -1;
    score_ = 0;
    bare_ = 0;
    golds_ = 0;
    cream_ = 0;
    won_ = false;
    over_ = false;
    left_ = false;
    why_ = "";
    power_ = 0;
    apex_ = 0;
    puck_ = 0;
    puckV_ = 0;
    phase_ = 0.35f;
    modeT_ = 0;
    bellT_ = 0;
    flash_ = 0;
    mode_ = Mode::Ready;
    sys_->setLight(140, 40, 50);
}

void Game::launch(float power) {
    if (mode_ != Mode::Ready || leftSwings_ <= 0) return;
    power_ = std::clamp(power, 0.f, 1.f);
    used_++;
    leftSwings_--;
    int mk = markOf(power_);
    apex_ = mk == kGold ? 1.f : (mk < 0 ? power_ * 0.16f : kMarkH[mk]);
    puck_ = 0;
    puckV_ = std::sqrt(2.f * GRAV * std::max(0.04f, apex_));
    mode_ = Mode::Strike;
    modeT_ = 0;
    thunk();
}

void Game::settle() {
    landed_ = markOf(power_);
    if (landed_ >= 0 && apex_ + 0.03f < kMarkH[landed_]) landed_ = -1;
    puck_ = landed_ >= 0 ? kMarkH[landed_] : apex_ * 0.45f;
    whistle(0);
    if (landed_ == kGold) {
        golds_++;
        bare_ += kFace[kGold];
        score_ += scored(kGold);
    } else if (landed_ >= 0) {
        cream_++;
        bare_ += kFace[landed_];
        score_ += scored(landed_);
    }
    judge();
}

void Game::judge() {
    const bool doubled = golds_ > 0 && score_ >= kLine && bare_ < kLine && score_ == bare_ + golds_ * kFace[kGold];
    if (doubled) {
        won_ = true;
        left_ = true;
        why_ = "only the gold counts double";
        mode_ = Mode::Win;
        modeT_ = 0;
        bellT_ = 0;
        flash_ = 1.f;
        chime();
        over_ = true;
        return;
    }
    if (score_ >= kLine || leftSwings_ <= 0) {
        won_ = false;
        left_ = false;
        why_ = score_ >= kLine ? "bare bought the line" : "short of the line";
        mode_ = Mode::Lose;
        modeT_ = 0;
        over_ = true;
        blip(false);
        return;
    }
    mode_ = Mode::Show;
    modeT_ = 0;
    blip(landed_ == kGold);
}

void Game::botPlay() {
    if (!bot_) return;
    if (mode_ == Mode::Title && modeT_ > 0.3f) {
        begin();
        return;
    }
    if (mode_ == Mode::Ready) {
        float m = meter();
        bool rising = std::sin(phase_) > 0.f;
        if (m >= 0.93f && rising) launch(m);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    modeT_ = 0;
    over_ = won_ = left_ = false;
    landed_ = -1;
    why_ = "";
    sys.setLight(70, 20, 40);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool go = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_START);
    modeT_ += DT;
    if (mode_ == Mode::Title || mode_ == Mode::Ready) phase_ += TAU * DT / PERIOD;
    if (mode_ == Mode::Title && go) begin();
    else if (mode_ == Mode::Ready && go) launch(meter());
    else if ((mode_ == Mode::Win || mode_ == Mode::Lose) && go && !bot_) {
        over_ = false;
        mode_ = Mode::Title;
        modeT_ = 0;
    }
    botPlay();

    if (mode_ == Mode::Strike) {
        if (modeT_ > 0.16f) {
            mode_ = Mode::Rise;
            modeT_ = 0;
        }
    } else if (mode_ == Mode::Rise) {
        puckV_ -= GRAV * DT;
        puck_ += puckV_ * DT;
        if (puck_ < 0.f) puck_ = 0.f;
        whistle(puck_);
        if (puckV_ <= 0.f || puck_ >= apex_) {
            puck_ = std::min(puck_, apex_);
            settle();
        }
    } else if (mode_ == Mode::Show) {
        if (modeT_ > 0.55f) {
            puck_ = 0;
            mode_ = Mode::Ready;
            modeT_ = 0;
            phase_ = 0.15f;
        }
    }
    if (mode_ == Mode::Win) {
        bellT_ += DT;
        flash_ = std::max(0.f, flash_ - DT * 0.35f);
    }
    draw();
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.road[y].on = false;
        float t = float(y) / float(gs::SCREEN_H - 1);
        int r = int(1 + (1.f - t) * 3);
        int g = int(1 + t * 2);
        int b = int(5 + (1.f - t) * 4);
        if (y > 180) {
            r = 4;
            g = 3;
            b = 2;
        }
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys_->vdp.lineFog[y] = 0;
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sys_->vdp.A.clear();
    sys_->vdp.B.clear();
    backdrop();

    float wob = (mode_ == Mode::Rise) ? std::sin(modeT_ * 36.f) * 1.2f : 0.f;
    float towerCx = kTowerX + wob;

    for (int i = 0; i < 5; i++)
        spr(art_.flag, 28.f + i * 22.f, 16.f + (i & 1) * 4.f, 16.f, (i & 1) ? PAL_RED : PAL_GOLD);
    spr(art_.lamp, 40.f, 156.f, 26.f, PAL_NIGHT);
    spr(art_.lamp, 292.f, 154.f, 26.f, PAL_NIGHT);

    spr(art_.tower, towerCx, kTowerTop + kTowerH * 0.5f, kTowerH, PAL_WOOD);
    int bellI = (mode_ == Mode::Win && int(bellT_ * 8.f) % 2 == 0) ? 1 : 0;
    float bellY = kTowerTop + 8.f + (mode_ == Mode::Win ? std::sin(bellT_ * 26.f) * 2.f : 0.f);
    spr(art_.bell[bellI], towerCx, bellY, 20.f, PAL_BRASS);

    float py = slotScreenY(std::clamp(puck_, 0.f, 1.f));
    spr(art_.puck, towerCx, py, 11.f, PAL_BRASS);

    int p = pose();
    spr(art_.man[p], MAN_X, MAN_Y, 74.f, PAL_MAN);
    float mx = MAN_X + (p == 1 ? 18.f : p == 2 ? -8.f : 10.f);
    float my = MAN_Y - (p == 1 ? 30.f : p == 2 ? 2.f : 12.f);
    spr(art_.mallet, mx, my, p == 1 ? 34.f : 28.f, PAL_WOOD);

    char line[48];
    if (mode_ == Mode::Title) {
        img(art_.title, 150.f, 78.f, float(art_.title.w), float(art_.title.h), PAL_GOLD);
        hudC(16, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(18, "CREAM IS FACE VALUE", PAL_CREAM);
        hudC(20, "LINE 80   GOLD 40 X2", PAL_HUD);
        hudC(24, "A START", PAL_GREEN);
    } else {
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 1, line, PAL_GOLD);
        std::snprintf(line, sizeof line, "BARE %d", bare_);
        hud(14, 1, line, PAL_CREAM);
        std::snprintf(line, sizeof line, "LINE %d", kLine);
        hud(26, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "SWINGS %d", leftSwings_);
        hud(1, 3, line, PAL_HUD);
        std::snprintf(line, sizeof line, "GOLD %d", golds_);
        hud(14, 3, line, PAL_GOLD);
        std::snprintf(line, sizeof line, "CREAM %d", cream_);
        hud(26, 3, line, PAL_CREAM);
        meterBar(25);
        hudC(23, "GOLD X2", PAL_GOLD);
        if (mode_ == Mode::Ready) hudC(21, "A SWING", PAL_GREEN);
        if (mode_ == Mode::Win) hudC(20, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        if (mode_ == Mode::Lose) hudC(20, why_, PAL_RED);
        if (mode_ == Mode::Show && landed_ >= 0 && landed_ < kGold) hudC(21, "FACE ONLY", PAL_CREAM);
        if (mode_ == Mode::Show && landed_ == kGold) hudC(21, "DOUBLED", PAL_GOLD);
    }
    if (flash_ > 0.f) {
        for (int y = 0; y < 10; y++) sys_->vdp.lineBackdrop[y] = gs::rgb4(15, 13, 5);
    }
}

}  // namespace strikergold
