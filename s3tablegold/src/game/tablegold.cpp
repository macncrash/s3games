#include "game/tablegold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tablegold {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLeft = 56.f;
constexpr float kRight = 264.f;
constexpr float kTop = 64.f;
constexpr float kBot = 168.f;
constexpr float kMouthL = 124.f;
constexpr float kMouthR = 196.f;
constexpr float kPuckR = 7.f;
constexpr float kMalR = 12.f;
constexpr float kCenter = 160.f;
constexpr float kMid = (kTop + kBot) * 0.5f;

// Gold, cream, gold finishes the short line. Later marks are spare.
constexpr bool kGoldMark[kRallies] = {true, false, true, true, false};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

void clampHalf(float& x, float& y, bool you) {
    x = clampf(x, kLeft + kMalR, kRight - kMalR);
    if (you) y = clampf(y, kMid + 8.f, kBot - kMalR);
    else y = clampf(y, kTop + kMalR, kMid - 8.f);
}

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Face: return "face";
    case Mode::Play: return "play";
    case Mode::Call: return "call";
    case Mode::Win: return "win";
    case Mode::Lose: return "lose";
    case Mode::Pause: return "pause";
    }
    return "?";
}

bool Game::paid() const {
    const int bare = gold_ + cream_;
    return finisherGold_ && gold_ >= 1 && score_ >= kLine && bare < kLine && score_ == gold_ * 2 + cream_;
}

bool Game::markGold() const {
    if (mark_ < 0 || mark_ >= kRallies) return false;
    return kGoldMark[mark_];
}

bool Game::crossed(bool top) const {
    if (puckX_ < kMouthL + kPuckR * 0.35f || puckX_ > kMouthR - kPuckR * 0.35f) return false;
    if (top) return puckY_ < kTop - kPuckR * 0.4f;
    return puckY_ > kBot + kPuckR * 0.4f;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.09f);
}

void Game::chord(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.09f);
    sys_->apu.tone(1, b, 0.06f);
    sys_->apu.tone(2, c, 0.05f);
    beep_ = std::max(beep_, hold);
}

void Game::hush() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.scroll(0, 0);
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 2, 4));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.10f, 0.14f, 0.08f);
    for (int i = 0; i < kRallies; i++) result_[i] = -2;
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::toTitle() {
    gold_ = cream_ = score_ = goals_ = them_ = mark_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kRallies; i++) result_[i] = -2;
    say_ = "";
    mode_ = Mode::Title;
}

void Game::beginRound() {
    gold_ = cream_ = score_ = goals_ = them_ = mark_ = 0;
    finisherGold_ = won_ = over_ = false;
    for (int i = 0; i < kRallies; i++) result_[i] = -2;
    say_ = "";
    faceoff();
}

void Game::faceoff() {
    puckX_ = kCenter;
    puckY_ = kMid + 6.f;
    puckVX_ = puckVY_ = 0;
    px_ = kCenter;
    py_ = kBot - 22.f;
    pvx_ = pvy_ = 0;
    ox_ = kCenter + 62.f;
    oy_ = kTop + 22.f;
    ovx_ = ovy_ = 0;
    faceT_ = 0.35f;
    mode_ = Mode::Face;
    blip(markGold() ? 523.f : 330.f);
    if (sys_) sys_->setLight(markGold() ? 255 : 90, markGold() ? 170 : 70, markGold() ? 40 : 36);
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    say_ = "DOUBLE";
    chord(392.f, 523.f, 784.f, 0.9f);
    if (sys_) {
        sys_->setLight(255, 190, 40);
        if (!bot_) sys_->rumble(0.4f, 0.7f, 160);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    say_ = "NO DOUBLE";
    blip(90.f);
    if (sys_) sys_->setLight(180, 30, 24);
}

void Game::finishCross(bool you) {
    puckVX_ = puckVY_ = 0;
    int stamp = 0;
    if (!you) {
        them_++;
        say_ = "THEIRS";
        blip(110.f);
        if (them_ >= kLine) {
            lose();
            return;
        }
        callT_ = 0.45f;
        mode_ = Mode::Call;
        return;
    }
    const bool gold = markGold();
    if (!gold) {
        if (score_ + 1 >= kLine) {
            stamp = -1;
            say_ = "NOT DOUBLE";
            blip(98.f);
        } else {
            cream_++;
            score_ += 1;
            goals_++;
            stamp = 1;
            say_ = "CREAM";
            blip(392.f);
        }
    } else {
        gold_++;
        score_ += 2;
        goals_++;
        stamp = 2;
        say_ = "CROSSED";
        if (score_ >= kLine) finisherGold_ = true;
        chord(523.f, 659.f, 784.f, 0.28f);
        if (sys_ && !bot_) sys_->rumble(0.25f, 0.55f, 80);
    }
    if (mark_ >= 0 && mark_ < kRallies) result_[mark_] = stamp;
    mark_++;
    callT_ = 0.42f;
    mode_ = Mode::Call;
}

void Game::afterCall() {
    if (paid()) win();
    else if (mode_ == Mode::Lose) return;
    else if (mark_ >= kRallies) lose();
    else faceoff();
}

void Game::bounceMallet(float mx, float my, float mvx, float mvy) {
    float dx = puckX_ - mx;
    float dy = puckY_ - my;
    float d2 = dx * dx + dy * dy;
    const float rad = kPuckR + kMalR;
    if (d2 > rad * rad || d2 < 0.25f) return;
    float d = std::sqrt(d2);
    float nx = dx / d;
    float ny = dy / d;
    puckX_ = mx + nx * (rad + 0.4f);
    puckY_ = my + ny * (rad + 0.4f);
    float vn = (puckVX_ - mvx) * nx + (puckVY_ - mvy) * ny;
    if (vn < 0.f) {
        puckVX_ -= 1.65f * vn * nx;
        puckVY_ -= 1.65f * vn * ny;
    }
    puckVX_ += mvx * 0.25f;
    puckVY_ += mvy * 0.25f;
    float sp = std::sqrt(puckVX_ * puckVX_ + puckVY_ * puckVY_);
    if (sp > 340.f) {
        puckVX_ *= 340.f / sp;
        puckVY_ *= 340.f / sp;
    }
}

void Game::driveYou(const Input& in, float dt) {
    float tx = px_ + in.x * 150.f * dt;
    float ty = py_ - in.y * 150.f * dt;
    if (in.slap) {
        ty -= 26.f;
        pvy_ = -220.f;
    }
    pvx_ = (tx - px_) / std::max(dt, 0.001f) * 0.35f;
    if (!in.slap) pvy_ = (ty - py_) / std::max(dt, 0.001f) * 0.35f;
    px_ = tx;
    py_ = ty;
    clampHalf(px_, py_, true);
}

void Game::driveThem(float dt) {
    float tx = kCenter + 70.f;
    float ty = kTop + 24.f;
    if (!bot_) {
        tx = puckX_;
        if (std::fabs(puckVY_) > 180.f && puckVY_ < 0.f) tx = kCenter + (puckX_ < kCenter ? 54.f : -54.f);
        ty = clampf(puckY_ - 18.f, kTop + 16.f, kMid - 14.f);
    } else if (puckVY_ < -40.f) {
        tx = kCenter + 78.f;
    }
    ovx_ = (tx - ox_) * 6.f;
    ovy_ = (ty - oy_) * 6.f;
    ox_ += ovx_ * dt;
    oy_ += ovy_ * dt;
    clampHalf(ox_, oy_, false);
    (void)dt;
}

void Game::physics(float dt) {
    puckX_ += puckVX_ * dt;
    puckY_ += puckVY_ * dt;
    puckVX_ *= 0.992f;
    puckVY_ *= 0.992f;

    if (crossed(true)) {
        finishCross(true);
        return;
    }
    if (crossed(false)) {
        finishCross(false);
        return;
    }

    if (puckX_ < kLeft + kPuckR) {
        puckX_ = kLeft + kPuckR;
        puckVX_ = std::fabs(puckVX_);
    } else if (puckX_ > kRight - kPuckR) {
        puckX_ = kRight - kPuckR;
        puckVX_ = -std::fabs(puckVX_);
    }

    const bool inMouth = puckX_ > kMouthL && puckX_ < kMouthR;
    if (!inMouth && puckY_ < kTop + kPuckR) {
        puckY_ = kTop + kPuckR;
        puckVY_ = std::fabs(puckVY_);
    } else if (!inMouth && puckY_ > kBot - kPuckR) {
        puckY_ = kBot - kPuckR;
        puckVY_ = -std::fabs(puckVY_);
    } else if (puckY_ < kTop - 28.f || puckY_ > kBot + 28.f) {
        puckVY_ = -puckVY_;
        puckY_ = clampf(puckY_, kTop - 10.f, kBot + 10.f);
    }

    bounceMallet(px_, py_, pvx_, pvy_);
    bounceMallet(ox_, oy_, ovx_, ovy_);
}

Game::Input Game::readPad(const gs::Pad& pad) const {
    Input in;
    if (pad.down(gs::BTN_LEFT)) in.x -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) in.x += 1.f;
    if (pad.down(gs::BTN_UP)) in.y += 1.f;
    if (pad.down(gs::BTN_DOWN)) in.y -= 1.f;
    if (std::fabs(pad.axisX) > 0.2f) in.x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.2f) in.y = pad.axisY;
    in.slap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    in.start = pad.pressed(gs::BTN_START);
    in.back = pad.pressed(gs::BTN_MODE);
    return in;
}

Game::Input Game::botInput() const {
    Input in;
    if (mode_ == Mode::Title && clock_ > 0.45f) {
        in.start = true;
        return in;
    }
    if (mode_ != Mode::Play && mode_ != Mode::Face) return in;
    float tx = puckX_;
    float ty = puckY_ + kMalR + kPuckR - 1.f;
    if (puckY_ < kMid) {
        tx = kCenter;
        ty = kBot - 24.f;
    }
    float dx = tx - px_;
    float dy = py_ - ty;
    float mag = std::sqrt(dx * dx + dy * dy);
    if (mag > 1.f) {
        in.x = clampf(dx / 18.f, -1.f, 1.f);
        in.y = clampf(dy / 18.f, -1.f, 1.f);
    }
    if (mode_ == Mode::Play && faceT_ <= 0.f && puckY_ > kMid - 4.f && std::fabs(px_ - puckX_) < 10.f &&
        py_ > puckY_ && py_ - puckY_ < kMalR + kPuckR + 6.f)
        in.slap = true;
    return in;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) clock_ += kDt;
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) hush();
    }

    const Input in = bot_ ? botInput() : readPad(sys.pad);

    if (mode_ == Mode::Title) {
        puckX_ = kCenter + std::sin(clock_ * 1.4f) * 18.f;
        puckY_ = kMid + 8.f;
        puckVX_ = puckVY_ = 0;
        px_ = kCenter;
        py_ = kBot - 22.f;
        ox_ = kCenter + 50.f;
        oy_ = kTop + 22.f;
        if (in.back && !bot_) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (in.start || in.slap) beginRound();
    } else if (mode_ == Mode::Pause) {
        if (in.start || in.slap) mode_ = held_;
        else if (in.back) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (in.start || in.slap)) beginRound();
        else if (!bot_ && in.back) toTitle();
    } else if (mode_ == Mode::Call) {
        callT_ -= kDt;
        if (callT_ <= 0.f || (!bot_ && (in.slap || in.start))) afterCall();
    } else if (!bot_ && in.start) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Face) {
        faceT_ -= kDt;
        driveYou(in, kDt);
        driveThem(kDt);
        if (faceT_ <= 0.f) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        driveYou(in, kDt);
        driveThem(kDt);
        physics(kDt);
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - (s ? (int)std::strlen(s) / 2 : 0), row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.lineBackdrop[y] = gs::rgb4(1, 2, 5);
    }

    const bool goldNow = (mode_ == Mode::Title) ? true : markGold();
    blob(art_.blot, kCenter, kTop - 6.f, kMouthR - kMouthL, 8.f, goldNow ? PAL_GOLD : PAL_CREAM);

    blob(art_.shadow, ox_, oy_ + 8.f, 22.f, 8.f, PAL_SHADOW, true);
    blob(art_.shadow, px_, py_ + 8.f, 22.f, 8.f, PAL_SHADOW, true);
    blob(art_.shadow, puckX_, puckY_ + 6.f, 14.f, 5.f, PAL_SHADOW, true);
    spr(art_.malletThem, ox_, oy_, 26.f, PAL_THEM);
    spr(art_.malletYou, px_, py_, 26.f, PAL_YOU);
    spr(art_.puck, puckX_, puckY_, 14.f, PAL_PUCK);

    char buf[72];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 TABLE GOLD", PAL_WORD);
        hudC(3, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(5, "GOLD CROSS 2   CREAM CROSS 1", PAL_CREAM);
        hudC(7, "SHORT TABLE    LINE 4", PAL_GOOD);
        hudC(24, "MOVE   A SLAP   START", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_WORD);
    } else if (mode_ == Mode::Win) {
        hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(3, buf, PAL_GOOD);
        hudC(6, "DOUBLE", PAL_WORD);
    } else if (mode_ == Mode::Lose) {
        hudC(1, "NO DOUBLE", PAL_BAD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(3, buf, PAL_INK);
    } else {
        hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_WORD);
        std::snprintf(buf, sizeof buf, "MARK %d   %s", std::min(mark_ + 1, kRallies), goldNow ? "GOLD X2" : "CREAM X1");
        hudC(3, buf, goldNow ? PAL_GOLD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hudC(5, buf, PAL_INK);
        if (mode_ == Mode::Call && say_[0]) {
            int pal = (std::strcmp(say_, "NOT DOUBLE") == 0 || std::strcmp(say_, "THEIRS") == 0) ? PAL_BAD : PAL_GOOD;
            hudC(8, say_, pal);
        }
    }

    for (int i = 0; i < kRallies; i++) {
        const char* ch = ".";
        int pal = PAL_INK;
        if (result_[i] == 2) {
            ch = "G";
            pal = PAL_GOLD;
        } else if (result_[i] == 1) {
            ch = "C";
            pal = PAL_CREAM;
        } else if (result_[i] == 0 || result_[i] == -1) {
            ch = "X";
            pal = PAL_BAD;
        }
        hud(16 + i * 2, 26, ch, pal);
    }
}

}  // namespace tablegold
