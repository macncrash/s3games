#include "striker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace strikermark {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float TAU = 6.2831853f;
constexpr float PERIOD = 1.55f;
constexpr float GRAV = 3.4f;
constexpr float MAN_X = 108.f;
constexpr float MAN_Y = 168.f;

const char* kName[kMarkN] = {"10", "20", "30", "40", "GOLD"};

}  // namespace

float Game::meter() const { return 0.5f * (1.f - std::cos(phase_)); }

int Game::pose() const {
    if (mode_ == Mode::Strike) return modeT_ < 0.12f ? 1 : 2;
    if (mode_ == Mode::Rise) return 2;
    return 0;
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
    int gold = int(std::ceil(kFinishAt * (cols - 1)));
    for (int i = 0; i < cols; i++) {
        char ch[2] = {char(i == mark ? 'O' : (i >= gold ? '=' : '-')), 0};
        int pal = i >= gold ? PAL_GOLD : PAL_HUD;
        if (i == mark) pal = m >= kFinishAt ? PAL_GREEN : PAL_RED;
        hud(x0 + i, row, ch, pal);
    }
}

void Game::blip(bool high) { sys_->apu.tone(1, high ? 880.f : 440.f, 0.06f); }

void Game::thunk() {
    sys_->apu.noiseBurst(0.55f, 140.f, 0.12f);
    sys_->rumble(0.6f, 0.3f, 80);
}

void Game::chime() {
    sys_->apu.tone(2, 784.f, 0.12f);
    sys_->apu.tone(3, 1175.f, 0.1f);
    sys_->setLight(220, 160, 40);
}

void Game::whistle(float h) {
    if (h < 0.02f) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    sys_->apu.tone(0, 180.f + h * 900.f, 0.04f);
}

void Game::begin() {
    left_ = 3;
    used_ = 0;
    landed_ = -1;
    best_ = -1;
    won_ = false;
    over_ = false;
    finished_ = false;
    rules_ = false;
    power_ = 0;
    apex_ = 0;
    puck_ = 0;
    puckV_ = 0;
    phase_ = 0.4f;
    modeT_ = 0;
    bellT_ = 0;
    flash_ = 0;
    mode_ = Mode::Ready;
    sys_->setLight(160, 40, 50);
}

void Game::launch(float power) {
    if (mode_ != Mode::Ready || left_ <= 0) return;
    power_ = std::clamp(power, 0.f, 1.f);
    used_++;
    left_--;
    bool will = power_ >= kFinishAt;
    apex_ = will ? 1.f : power_ * 0.78f;
    puck_ = 0;
    puckV_ = std::sqrt(2.f * GRAV * std::max(0.05f, apex_));
    mode_ = Mode::Strike;
    modeT_ = 0;
    thunk();
}

void Game::settle() {
    int hit = -1;
    for (int i = 0; i < kMarkN; i++)
        if (apex_ + 0.02f >= kMarkH[i]) hit = i;
    landed_ = hit;
    if (hit > best_) best_ = hit;
    puck_ = hit >= 0 ? kMarkH[hit] : apex_ * 0.5f;
    whistle(0);
    if (hit == kGold && apex_ >= kFinishAt) finishMark();
    else if (left_ <= 0) fail();
    else {
        mode_ = Mode::Show;
        modeT_ = 0;
        blip(false);
    }
}

void Game::finishMark() {
    if (landed_ != kGold || used_ < 1 || used_ > 3 || apex_ < kFinishAt) return;
    finished_ = true;
    won_ = true;
    rules_ = true;
    mode_ = Mode::Win;
    modeT_ = 0;
    bellT_ = 0;
    flash_ = 1.f;
    chime();
    over_ = true;
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    rules_ = true;
    mode_ = Mode::Lose;
    modeT_ = 0;
    over_ = true;
}

void Game::botPlay() {
    if (!bot_) return;
    if (mode_ == Mode::Title && modeT_ > 0.35f) {
        begin();
        return;
    }
    if (mode_ == Mode::Ready) {
        float m = meter();
        float rising = std::sin(phase_) > 0.f;
        if (m >= 0.93f && rising) launch(m);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    modeT_ = 0;
    over_ = won_ = finished_ = rules_ = false;
    landed_ = -1;
    sys.setLight(80, 20, 40);
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
        if (modeT_ > 0.18f) {
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
        if (modeT_ > 0.7f) {
            puck_ = 0;
            mode_ = Mode::Ready;
            modeT_ = 0;
            phase_ = 0.2f;
        }
    }
    if (mode_ == Mode::Win) {
        bellT_ += DT;
        flash_ = std::max(0.f, flash_ - DT * 0.4f);
    }
    draw();
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.road[y].on = false;
        float t = float(y) / float(gs::SCREEN_H - 1);
        int r = int(2 + t * 8);
        int g = int(1 + (1.f - t) * 2);
        int b = int(6 - t * 3);
        if (y > 186) {
            r = 3;
            g = 5;
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

    float wob = (mode_ == Mode::Rise) ? std::sin(modeT_ * 40.f) * 1.5f : 0.f;
    float towerCx = kTowerX + wob;

    for (int i = 0; i < 8; i++)
        spr(art_.bunt, 20.f + i * 40.f, 12.f, 14.f, (i & 1) ? PAL_RED : PAL_GOLD);
    spr(art_.crowd, 48.f, 176.f, 40.f, PAL_CROWD);
    spr(art_.crowd, 292.f, 178.f, 36.f, PAL_CROWD, true);
    spr(art_.lamp, 36.f, 150.f, 28.f, PAL_NIGHT);
    spr(art_.lamp, 300.f, 148.f, 28.f, PAL_NIGHT);

    spr(art_.tower, towerCx, kTowerTop + kTowerH * 0.5f, kTowerH, PAL_WOOD);
    int bellI = (mode_ == Mode::Win && int(bellT_ * 8.f) % 2 == 0) ? 1 : 0;
    float bellY = kTowerTop + 10.f + (mode_ == Mode::Win ? std::sin(bellT_ * 28.f) * 2.f : 0.f);
    spr(art_.bell[bellI], towerCx, bellY, 22.f, PAL_BRASS);

    float py = slotScreenY(std::clamp(puck_, 0.f, 1.f));
    spr(art_.puck, towerCx, py, 10.f, PAL_BRASS);

    int p = pose();
    spr(art_.man[p], MAN_X, MAN_Y, 78.f, PAL_MAN);
    float mx = MAN_X + (p == 1 ? 16.f : p == 2 ? -6.f : 8.f);
    float my = MAN_Y - (p == 1 ? 28.f : p == 2 ? 4.f : 10.f);
    spr(art_.mallet, mx, my, p == 1 ? 36.f : 30.f, PAL_WOOD);

    if (mode_ == Mode::Title) {
        img(art_.title, 160.f, 78.f, float(art_.title.w) * 2.f, float(art_.title.h) * 2.f, PAL_GOLD);
        hudC(16, "A FINISHED MARK ENDS IT", PAL_HUD);
        hudC(18, "SWING ON THE GOLD", PAL_GOLD);
        hudC(24, "A START", PAL_GREEN);
    } else {
        img(art_.banner, 100.f, 28.f, 110.f, 16.f, PAL_WIN);
        char line[32];
        std::snprintf(line, sizeof line, "SWINGS %d", left_);
        hud(1, 1, line, PAL_HUD);
        if (landed_ >= 0 && mode_ != Mode::Rise && mode_ != Mode::Strike) {
            std::snprintf(line, sizeof line, "MARK %s", kName[landed_]);
            hud(28, 1, line, landed_ == kGold ? PAL_GOLD : PAL_RED);
        } else {
            hud(28, 1, "MARK OPEN", PAL_HUD);
        }
        meterBar(25);
        hudC(23, "GOLD", PAL_GOLD);
        if (mode_ == Mode::Ready) hudC(21, "A SWING", PAL_GREEN);
        if (mode_ == Mode::Win) hudC(20, "FINISHED MARK", PAL_WIN);
        if (mode_ == Mode::Lose) hudC(20, "STILL OPEN", PAL_RED);
        if (mode_ == Mode::Show && landed_ >= 0 && landed_ < kGold) hudC(20, "NOT THE MARK", PAL_RED);
    }
    if (flash_ > 0.f) {
        for (int y = 0; y < 8; y++) sys_->vdp.lineBackdrop[y] = gs::rgb4(15, 14, 6);
    }
}

}  // namespace strikermark
