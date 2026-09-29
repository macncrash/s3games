#include "seven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace strikerseven {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kTau = 6.2831853f;
constexpr float kPeriod = 1.45f;
constexpr float kGrav = 3.6f;
constexpr int kRace = 7;
constexpr int kLogMax = 24;
constexpr float kManX = 104.f;
constexpr float kManY = 168.f;

}  // namespace

float Game::meter() const { return 0.5f * (1.f - std::cos(phase_)); }

int Game::pointsFor(float power) const {
    if (power >= kBellH) return 3;
    if (power >= kPairH) return 2;
    if (power >= kTickH) return 1;
    return 0;
}

int Game::pose() const {
    if (mode_ == Mode::Strike) return modeT_ < 0.1f ? 1 : 2;
    if (mode_ == Mode::Rise) return 2;
    return 0;
}

bool Game::ledgerOk() const {
    int y = 0, t = 0;
    if (hits_ < 1 || hits_ > kLogMax) return false;
    for (int i = 0; i < hits_; i++) {
        if (log_[i].pts < 0 || log_[i].pts > 3) return false;
        if (log_[i].yours) y += log_[i].pts;
        else t += log_[i].pts;
    }
    return y == you_ && t == them_ && you_ >= kRace && them_ < kRace && shortSix_;
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
    if (im.w < 1 || w < 1.f || h < 1.f) return;
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
    const int cols = 26;
    const int x0 = 7;
    float m = (mode_ == Mode::Ready || mode_ == Mode::Title) ? meter() : power_;
    int mark = std::clamp(int(std::lround(m * (cols - 1))), 0, cols - 1);
    int bell = int(std::ceil(kBellH * (cols - 1)));
    for (int i = 0; i < cols; i++) {
        char ch[2] = {char(i == mark ? 'O' : (i >= bell ? '=' : '-')), 0};
        int pal = i >= bell ? PAL_GOLD : PAL_HUD;
        if (i == mark) pal = m >= kBellH ? PAL_GREEN : PAL_RED;
        hud(x0 + i, row, ch, pal);
    }
}

void Game::blip(bool high) { sys_->apu.tone(1, high ? 880.f : 330.f, 0.06f); }

void Game::thunk() {
    sys_->apu.noiseBurst(0.5f, 140.f, 0.1f);
    sys_->rumble(0.55f, 0.25f, 70);
}

void Game::chime() {
    sys_->apu.tone(2, 784.f, 0.12f);
    sys_->apu.tone(3, 1175.f, 0.1f);
    sys_->setLight(220, 170, 40);
}

void Game::whistle(float h) {
    if (h < 0.02f) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    sys_->apu.tone(0, 180.f + h * 860.f, 0.04f);
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    gain_ = 0;
    hits_ = 0;
    yours_ = true;
    won_ = false;
    over_ = false;
    shortSix_ = false;
    rules_ = true;
    power_ = 0;
    apex_ = 0;
    puck_ = 0;
    puckV_ = 0;
    phase_ = 0.15f;
    modeT_ = 0;
    bellT_ = 0;
    leaveT_ = 0;
    say_ = "FIRST TO SEVEN";
    mode_ = Mode::Ready;
    sys_->setLight(160, 40, 50);
}

void Game::launch(float power) {
    if (mode_ != Mode::Ready) return;
    power_ = std::clamp(power, 0.f, 1.f);
    apex_ = power_;
    puck_ = 0;
    puckV_ = std::sqrt(2.f * kGrav * std::max(0.04f, apex_));
    mode_ = Mode::Strike;
    modeT_ = 0;
    thunk();
}

void Game::win() {
    if (!ledgerOk()) {
        rules_ = false;
        lose();
        say_ = "RULES";
        return;
    }
    won_ = true;
    over_ = true;
    rules_ = true;
    mode_ = Mode::Win;
    modeT_ = 0;
    bellT_ = 0;
    say_ = "FIRST TO SEVEN";
    chime();
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    modeT_ = 0;
    say_ = "THEY HIT SEVEN";
    blip(false);
    if (sys_) sys_->setLight(140, 32, 28);
}

void Game::settle() {
    int pts = pointsFor(apex_);
    if (hits_ >= kLogMax) {
        rules_ = false;
        lose();
        say_ = "RULES";
        return;
    }
    log_[hits_].pts = pts;
    log_[hits_].yours = yours_;
    hits_++;
    gain_ = pts;
    whistle(0);
    puck_ = apex_;

    if (yours_) you_ += pts;
    else them_ += pts;
    if (you_ == 6) shortSix_ = true;

    if (yours_ && you_ >= kRace) {
        win();
        return;
    }
    if (!yours_ && them_ >= kRace && you_ < kRace) {
        lose();
        return;
    }

    if (pts == 0) say_ = "MISS";
    else if (you_ == 6) say_ = yours_ ? "SIX IS SHORT" : "THEIR SIX IS SHORT";
    else if (pts == 3) say_ = "BELL COUNTS 3";
    else if (pts == 2) say_ = "PAIR COUNTS 2";
    else say_ = "TICK COUNTS 1";

    if (pts == 3) chime();
    else blip(pts > 0);
    mode_ = Mode::Show;
    modeT_ = 0;
}

void Game::nextTurn() {
    yours_ = !yours_;
    puck_ = 0;
    power_ = 0;
    phase_ = 0.12f;
    mode_ = Mode::Ready;
    modeT_ = 0;
}

void Game::botPlay() {
    if (mode_ != Mode::Ready) return;
    bool rising = std::sin(phase_) > 0.f;
    float m = meter();
    if (!yours_) {
        if (modeT_ > 0.2f && rising && m >= 0.52f && m <= 0.68f) launch(m);
        return;
    }
    if (!bot_) return;
    if (modeT_ > 0.12f && rising && m >= 0.90f) launch(m);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    sys.apu.setMaster(0.7f);
    mode_ = Mode::Title;
    modeT_ = 0;
    over_ = won_ = shortSix_ = false;
    rules_ = true;
    say_ = "FIRST TO SEVEN";
    sys.setLight(80, 20, 40);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool go = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_START);
    modeT_ += kDt;
    if (mode_ == Mode::Title || mode_ == Mode::Ready) phase_ += kTau * kDt / kPeriod;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE) && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (go || (bot_ && modeT_ > 0.4f)) begin();
    } else if (mode_ == Mode::Ready && yours_ && go) {
        launch(meter());
    } else if (mode_ == Mode::Win) {
        leaveT_ += kDt;
        if (leaveT_ > 1.15f) sys.quit();
    }

    botPlay();

    if (mode_ == Mode::Strike) {
        if (modeT_ > 0.16f) {
            mode_ = Mode::Rise;
            modeT_ = 0;
        }
    } else if (mode_ == Mode::Rise) {
        puckV_ -= kGrav * kDt;
        puck_ += puckV_ * kDt;
        if (puck_ < 0.f) puck_ = 0.f;
        whistle(puck_);
        if (puckV_ <= 0.f || puck_ >= apex_) {
            puck_ = std::min(puck_, apex_);
            settle();
        }
    } else if (mode_ == Mode::Show) {
        float hold = (you_ == 6 && say_[0] == 'S') ? 0.85f : 0.48f;
        if (modeT_ > hold) nextTurn();
    } else if (mode_ == Mode::Win) {
        bellT_ += kDt;
    }

    draw();
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.road[y].on = false;
        float t = float(y) / float(gs::SCREEN_H - 1);
        int r = int(2 + (1.f - t) * 2);
        int g = int(1 + t * 2);
        int b = int(6 - t * 3);
        if (y > 188) {
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
    backdrop();

    float wob = (mode_ == Mode::Rise) ? std::sin(modeT_ * 36.f) * 1.4f : 0.f;
    float towerCx = kTowerX + wob;

    for (int i = 0; i < 8; i++)
        spr(art_.bunt, 20.f + i * 40.f, 12.f, 14.f, (i & 1) ? PAL_RED : PAL_GOLD);
    spr(art_.crowd, 46.f, 178.f, 38.f, PAL_CROWD);
    spr(art_.crowd, 292.f, 180.f, 34.f, PAL_CROWD, true);
    spr(art_.lamp, 34.f, 148.f, 26.f, PAL_NIGHT);
    spr(art_.lamp, 302.f, 146.f, 26.f, PAL_NIGHT);

    spr(art_.tower, towerCx, kTowerTop + kTowerH * 0.5f, kTowerH, PAL_WOOD);
    bool lit = (gain_ == 3 && (mode_ == Mode::Show || mode_ == Mode::Win)) || mode_ == Mode::Win;
    int bellI = (lit && int(modeT_ * 8.f) % 2 == 0) ? 1 : 0;
    float bellY = kTowerTop + 8.f + (mode_ == Mode::Win ? std::sin(bellT_ * 26.f) * 2.f : 0.f);
    spr(art_.bell[bellI], towerCx, bellY, 22.f, PAL_BRASS);

    float py = slotScreenY(std::clamp(puck_, 0.f, 1.f));
    spr(art_.puck, towerCx, py, 10.f, PAL_BRASS);

    int p = pose();
    bool them = !yours_ && mode_ != Mode::Title && mode_ != Mode::Win;
    float mx0 = them ? 246.f : kManX;
    spr(art_.man[p], mx0, kManY, 76.f, them ? PAL_THEM : PAL_MAN, them);
    float mx = mx0 + (them ? -1.f : 1.f) * (p == 1 ? 16.f : p == 2 ? -6.f : 8.f);
    float my = kManY - (p == 1 ? 28.f : p == 2 ? 4.f : 10.f);
    spr(art_.mallet, mx, my, p == 1 ? 34.f : 28.f, PAL_WOOD, them);

    if (mode_ == Mode::Title) {
        img(art_.title, 160.f, 72.f, float(art_.title.w) * 2.f, float(art_.title.h) * 2.f, PAL_GOLD);
        hudC(15, "PLAY UNTIL FIRST TO SEVEN", PAL_HUD);
        hudC(17, "BELL 3   PAIR 2   TICK 1", PAL_GOLD);
        hudC(19, "SIX IS STILL SHORT", PAL_RED);
        hudC(24, "A START", PAL_GREEN);
        meterBar(22);
    } else {
        img(art_.banner, 100.f, 22.f, float(art_.banner.w) * 2.f, float(art_.banner.h) * 2.f, PAL_WIN);
        char line[40];
        std::snprintf(line, sizeof line, "YOU %d", you_);
        hud(1, 1, line, PAL_GREEN);
        std::snprintf(line, sizeof line, "THEM %d", them_);
        hud(30, 1, line, PAL_RED);
        meterBar(25);
        hudC(23, yours_ ? "YOUR SWING" : "THEIR SWING", yours_ ? PAL_GREEN : PAL_RED);
        if (mode_ == Mode::Ready && yours_) hudC(21, "A SWING", PAL_GREEN);
        if (say_ && say_[0] && mode_ != Mode::Rise && mode_ != Mode::Strike)
            hudC(mode_ == Mode::Win || mode_ == Mode::Lose ? 19 : 20, say_,
                 mode_ == Mode::Win ? PAL_WIN : (you_ == 6 ? PAL_GOLD : PAL_HUD));
        if (mode_ == Mode::Win) hudC(21, "LEAVE", PAL_GREEN);
        if (mode_ == Mode::Lose) hudC(21, "THEY LEAVE WITH IT", PAL_RED);
    }
}

}  // namespace strikerseven
