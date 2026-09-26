#include "game/curltape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace curltape {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kDecel = 0.00128f;
constexpr float kCurl = 0.00042f;
constexpr float kStop = 0.008f;
constexpr int kCap = 420;
constexpr int kMaxRocks = 6;
constexpr float kVyLo = 0.05f;
constexpr float kVyHi = 0.36f;
constexpr float kVxLim = 0.12f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::shove(Rock& r, float sweep) {
    float sp = std::hypot(r.vx, r.vy);
    if (sp < kStop) {
        r.vx = r.vy = 0;
        return;
    }
    float sw = clampf(sweep, 0.f, 1.f);
    float decel = kDecel * (1.f - 0.48f * sw);
    float omega = float(r.handle) * kCurl * (0.55f / (sp + 0.07f));
    omega *= (1.f - 0.75f * sw);
    float c = std::cos(omega);
    float s = std::sin(omega);
    float rvx = r.vx * c - r.vy * s;
    float rvy = r.vx * s + r.vy * c;
    float rsp = std::hypot(rvx, rvy);
    if (rsp < 1e-8f) {
        r.vx = r.vy = 0;
        return;
    }
    if (decel >= rsp) {
        r.x += rvx / rsp * rsp * 0.5f;
        r.y += rvy / rsp * rsp * 0.5f;
        r.vx = r.vy = 0;
        return;
    }
    float nx = rvx / rsp;
    float ny = rvy / rsp;
    r.vx = rvx - nx * decel;
    r.vy = rvy - ny * decel;
    r.x += r.vx;
    r.y += r.vy;
    if (std::fabs(r.x) > kSide - 0.02f) r.dead = true;
    if (r.y > kBack + 0.45f) r.dead = true;
}

Game::End Game::fly(float vx, float vy, int handle, bool record) {
    Rock r;
    r.x = 0;
    r.y = kHack;
    r.vx = vx;
    r.vy = vy;
    r.handle = handle < 0 ? -1 : 1;
    End e;
    e.vx = vx;
    e.vy = vy;
    e.handle = r.handle;
    if (record) {
        e.pathX[0] = r.x;
        e.pathY[0] = r.y;
        e.pathN = 1;
    }
    for (int f = 0; f < kCap; f++) {
        float sp = std::hypot(r.vx, r.vy);
        if (r.dead || sp < kStop) break;
        shove(r, 0.f);
        e.frames = f + 1;
        if (record && (e.frames % 8) == 0 && e.pathN < 16) {
            e.pathX[e.pathN] = r.x;
            e.pathY[e.pathN] = r.y;
            e.pathN++;
        }
        if (r.dead || (r.vx == 0.f && r.vy == 0.f)) break;
    }
    e.x = r.x;
    e.y = r.y;
    e.dead = r.dead;
    e.lie = classify(r.x, r.y);
    if (record && e.pathN < 16) {
        e.pathX[e.pathN] = r.x;
        e.pathY[e.pathN] = r.y;
        e.pathN++;
    }
    return e;
}

bool Game::solve() {
    auto toY = [](float vx, int handle, float wantY) {
        float lo = kVyLo;
        float hi = kVyHi;
        End best = fly(vx, lo, handle, false);
        float bestDy = 1e9f;
        auto take = [&](const End& e) {
            if (e.dead || e.frames < 20 || e.frames >= kCap - 1) return;
            float dy = std::fabs(e.y - wantY);
            if (dy < bestDy) {
                bestDy = dy;
                best = e;
            }
        };
        take(best);
        take(fly(vx, hi, handle, false));
        for (int i = 0; i < 14; i++) {
            float mid = (lo + hi) * 0.5f;
            End e = fly(vx, mid, handle, false);
            take(e);
            if (e.dead || e.y > wantY) hi = mid;
            else lo = mid;
        }
        take(fly(vx, lo, handle, false));
        return best;
    };

    const float wantX[3] = {0.f, 0.f, 0.f};
    const float wantY[3] = {kTee, guardY(), biteY()};
    const Lie wantL[3] = {Lie::Button, Lie::Guard, Lie::Bite};
    bool all = true;
    for (int line = 0; line < 3; line++) {
        float bestErr = 1e9f;
        Shot best;
        bool ok = false;
        auto relax = [&](float vx, float vy, int handle, bool solveY) {
            End e = solveY ? toY(vx, handle, wantY[line]) : fly(vx, vy, handle, false);
            if (e.dead || e.frames < 24 || e.frames >= kCap - 1) return;
            float err = std::fabs(e.x - wantX[line]) + std::fabs(e.y - wantY[line]);
            if (e.lie != wantL[line]) err += 8.f;
            if (err < bestErr) {
                bestErr = err;
                best.vx = e.vx;
                best.vy = e.vy;
                best.handle = handle;
                ok = e.lie == wantL[line];
            }
        };
        for (int h : {1, -1}) {
            for (float vx = -0.05f; vx <= 0.0501f; vx += 0.005f) relax(vx, 0.f, h, true);
        }
        for (int iter = 0; iter < 8 && !ok; iter++) {
            float step = 0.016f * std::pow(0.62f, float(iter));
            float vx0 = best.vx;
            int h = best.handle;
            relax(vx0 - step, best.vy, h, true);
            relax(vx0 + step, best.vy, h, true);
            relax(best.vx, best.vy - 0.0012f, h, false);
            relax(best.vx, best.vy + 0.0012f, h, false);
        }
        if (!ok) {
            for (int h : {1, -1}) {
                for (float dvx = -0.02f; dvx <= 0.0201f; dvx += 0.0025f) {
                    for (float dvy = -0.012f; dvy <= 0.0121f; dvy += 0.0015f)
                        relax(best.vx + dvx, best.vy + dvy, h, false);
                    if (ok) break;
                }
                if (ok) break;
            }
        }
        End fin = fly(best.vx, best.vy, best.handle, false);
        ok = !fin.dead && fin.lie == wantL[line] && fin.frames > 24 && fin.frames < kCap - 1;
        shot_[line] = best;
        if (!ok) {
            std::fprintf(stderr, "s3curltape no %s  x %.3f y %.3f lie %s vx %.4f vy %.4f h %d\n", lieName(wantL[line]),
                         fin.x, fin.y, lieName(fin.lie), best.vx, best.vy, best.handle);
            all = false;
        }
    }
    return all;
}

bool Game::audit() {
    bool ok = rules_ && solve();
    auto bad = [&](const char* tag, Lie got, float x, float y) {
        std::fprintf(stderr, "s3curltape %s got %s at %.3f %.3f\n", tag, lieName(got), x, y);
        ok = false;
    };
    struct Sample {
        float x, y;
        Lie lie;
        const char* tag;
    };
    const Sample samples[] = {
        {0.f, kTee, Lie::Button, "button"},
        {0.f, kTee - (kButton + kFour) * 0.5f, Lie::Four, "four-front"},
        {kFour * 0.65f, kTee, Lie::Four, "four-side"},
        {0.f, guardY(), Lie::Guard, "guard"},
        {kGuardLane * 0.55f, guardY(), Lie::Guard, "guard-side"},
        {0.f, kHog - 0.55f, Lie::Short, "short"},
        {0.f, kHog - 0.08f, Lie::Short, "short-hog"},
        {0.f, kHog + 0.20f, Lie::Guard, "guard-hog"},
        {0.f, biteY(), Lie::Bite, "bite"},
        {0.f, biteY() + 0.35f, Lie::Ring, "past-bite"},
        {0.f, kTee - (kEight + kHouse) * 0.5f, Lie::Ring, "ring"},
        {0.f, kTee - (kFour + kEight) * 0.5f, Lie::House, "house"},
        {0.f, kTee - (kButton + 0.08f), Lie::Four, "almost"},
        {0.f, kHack + 0.25f, Lie::Hog, "hack"},
        {kSide, kTee, Lie::Wide, "board"},
        {0.f, kBack + 0.15f, Lie::Through, "through"},
        {1.95f, 11.2f, Lie::Back, "back"},
        {0.f, kTee - (kHouse + 0.92f * kStoneR), Lie::Miss, "gap"},
    };
    for (const Sample& s : samples) {
        Lie got = classify(s.x, s.y);
        if (got != s.lie) bad(s.tag, got, s.x, s.y);
    }
    if (liePay(Lie::Button) != 12 || liePay(Lie::Four) != 12 || tapeOf(Lie::Four) != -1 || twinOf(Lie::Four) != 0)
        bad("pay-button", Lie::Four, 0, 0);
    if (liePay(Lie::Guard) != 6 || liePay(Lie::Short) != 6 || tapeOf(Lie::Short) != -1 || twinOf(Lie::Short) != 1)
        bad("pay-guard", Lie::Short, 0, 0);
    if (liePay(Lie::Bite) != 4 || liePay(Lie::Ring) != 4 || tapeOf(Lie::Ring) != -1 || twinOf(Lie::Ring) != 2)
        bad("pay-bite", Lie::Ring, 0, 0);
    if (tapeOf(Lie::Button) != 0 || tapeOf(Lie::Guard) != 1 || tapeOf(Lie::Bite) != 2) bad("tape-index", Lie::Miss, 0, 0);
    if (tapePay(0) + tapePay(1) + tapePay(2) != 22) bad("due", Lie::Miss, 0, 0);
    const Lie wantL[3] = {Lie::Button, Lie::Guard, Lie::Bite};
    End landed[3];
    for (int i = 0; i < 3; i++) {
        landed[i] = fly(shot_[i].vx, shot_[i].vy, shot_[i].handle, false);
        if (landed[i].dead || landed[i].lie != wantL[i] || shot_[i].handle == 0) bad(tapeName(i), landed[i].lie, landed[i].x, landed[i].y);
    }
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 3; j++) {
            float d = std::hypot(landed[i].x - landed[j].x, landed[i].y - landed[j].y);
            if (d < 0.8f) bad("same-lie", landed[i].lie, landed[i].x, landed[i].y);
        }
    }
    if (!ok) std::snprintf(reason_, sizeof reason_, "NO LINE");
    return ok;
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < 3; i++)
        if (held_[i]) s += tapePay(i);
    return s;
}

const char* Game::modeName() const {
    switch (mode_) {
    case Mode::Title: return "TITLE";
    case Mode::Aim: return "AIM";
    case Mode::Slide: return "SLIDE";
    case Mode::Pocket: return "POCKET";
    case Mode::Judge: return "JUDGE";
    case Mode::Leave: return "LEAVE";
    case Mode::Lose: return "LOSE";
    case Mode::Pause: return "PAUSE";
    case Mode::Over: return "OVER";
    }
    return "?";
}

int Game::phase() const {
    if (!rules_ || mode_ == Mode::Lose) return 4;
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Slide) return 2;
    if (mode_ == Mode::Pocket || mode_ == Mode::Leave || mode_ == Mode::Over || matched()) return 3;
    return 1;
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(2, freq, 0.05f);
    beep_ = 8;
}

void Game::tickAudio() {
    if (!sys_) return;
    if (beep_ > 0 && --beep_ == 0 && mode_ != Mode::Leave && !(mode_ == Mode::Over && won_)) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Slide)
        sys_->apu.noise(sweep_ > 0.5f ? 0.05f : 0.016f, sweep_ > 0.5f ? 2200.f : 680.f, false);
    else if (mode_ != Mode::Leave && !(mode_ == Mode::Over && won_)) sys_->apu.noise(0.f, 800.f, false);
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    left_ = false;
    stones_ = 0;
    traps_ = 0;
    shake_ = 0;
    think_ = 0;
    slipI_ = -1;
    holdX_ = holdY_ = 0;
    aimVx_ = 0;
    aimVy_ = 0.16f;
    handle_ = 1;
    sweep_ = 0;
    clock_ = 0;
    pocketT_ = judgeT_ = leaveT_ = 0;
    lastLie_ = Lie::Miss;
    if (rules_) reason_[0] = 0;
    for (int i = 0; i < 3; i++) held_[i] = false;
    for (Puff& p : puff_) p.life = 0;
    rock_ = {};
    if (sys_) sys_->apu.silence();
}

void Game::newGame() {
    won_ = false;
    over_ = false;
    left_ = false;
    stones_ = 0;
    traps_ = 0;
    shake_ = 0;
    slipI_ = -1;
    sweep_ = 0;
    pocketT_ = judgeT_ = leaveT_ = 0;
    lastLie_ = Lie::Miss;
    reason_[0] = 0;
    for (int i = 0; i < 3; i++) held_[i] = false;
    for (Puff& p : puff_) p.life = 0;
    beginAim();
}

void Game::beginAim() {
    think_ = 0;
    holdX_ = holdY_ = 0;
    sweep_ = 0;
    slipI_ = -1;
    pocketT_ = 0;
    judgeT_ = 0;
    mode_ = Mode::Aim;
}

int Game::nextOpen() const {
    for (int i = 0; i < 3; i++)
        if (!held_[i]) return i;
    return 0;
}

void Game::launch() {
    if (!rules_ || stones_ >= kMaxRocks || mode_ != Mode::Aim) return;
    if (bot_) {
        int i = nextOpen();
        aimVx_ = shot_[i].vx;
        aimVy_ = shot_[i].vy;
        handle_ = shot_[i].handle;
    }
    rock_.x = 0;
    rock_.y = kHack;
    rock_.vx = aimVx_;
    rock_.vy = aimVy_;
    rock_.handle = handle_ < 0 ? -1 : 1;
    rock_.dead = false;
    stones_++;
    slideFrames_ = 0;
    sweep_ = 0;
    mode_ = Mode::Slide;
    if (sys_) sys_->apu.noiseBurst(0.12f, 1600.f, 0.04f);
    blip(520.f);
}

void Game::settle() {
    rock_.vx = rock_.vy = 0;
    sweep_ = 0;
    Lie L = classify(rock_.x, rock_.y);
    lastLie_ = L;
    int tape = tapeOf(L);
    if (tape >= 0) {
        if (held_[tape]) {
            std::snprintf(reason_, sizeof reason_, "ALREADY IN");
            mode_ = Mode::Judge;
            judgeT_ = 0;
            shake_ = 6;
            blip(140.f);
            return;
        }
        held_[tape] = true;
        slipI_ = tape;
        slipSX_ = screenX(rock_.x);
        slipSY_ = screenY(rock_.y);
        pocketT_ = 0;
        mode_ = Mode::Pocket;
        std::snprintf(reason_, sizeof reason_, "IN THE DRAWER");
        blip(520.f + float(tape) * 70.f);
        if (sys_) {
            sys_->rumble(0.15f, 0.4f, 70);
            sys_->setLight(70, 160, 80);
        }
        return;
    }
    int twin = twinOf(L);
    if (twin >= 0) {
        traps_++;
        std::snprintf(reason_, sizeof reason_, "%s IS NOT THE %s", lieName(L), tapeName(twin));
        if (sys_) {
            sys_->rumble(0.4f, 0.1f, 80);
            sys_->setLight(160, 40, 30);
        }
    } else {
        std::snprintf(reason_, sizeof reason_, "%s", lieName(L));
    }
    mode_ = Mode::Judge;
    judgeT_ = 0;
    shake_ = 6;
    blip(twin >= 0 ? 110.f : 180.f);
}

void Game::beginLeave() {
    if (!matched()) return;
    mode_ = Mode::Leave;
    leaveT_ = 0;
    left_ = true;
    won_ = true;
    slipI_ = -1;
    std::snprintf(reason_, sizeof reason_, "THE DRAWER MATCHES THE TAPE");
    if (!sys_) return;
    sys_->apu.tone(0, 523.f, 0.06f);
    sys_->apu.tone(1, 659.f, 0.05f);
    sys_->apu.tone(2, 784.f, 0.04f);
    beep_ = 50;
    sys_->rumble(0.25f, 0.65f, 140);
    sys_->setLight(255, 200, 80);
}

void Game::beginLose() {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    left_ = false;
    if (!reason_[0]) std::snprintf(reason_, sizeof reason_, "DOES NOT MATCH");
    blip(90.f);
    if (sys_) sys_->setLight(150, 30, 30);
}

void Game::afterPocket() {
    if (matched()) beginLeave();
    else if (stones_ >= kMaxRocks) beginLose();
    else beginAim();
}

void Game::afterJudge() {
    if (matched()) beginLeave();
    else if (stones_ >= kMaxRocks) beginLose();
    else beginAim();
}

void Game::stepSlide() {
    slideFrames_++;
    float sweep = 0.f;
    if (!bot_ && sys_ && slideFrames_ > 6) {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO)) sweep = 1.f;
    }
    sweep_ = sweep;
    if (sweep_ > 0.5f) {
        Puff& pf = puff_[puffN_++ % 6];
        pf.x = rock_.x + ((slideFrames_ & 1) ? 0.12f : -0.12f);
        pf.y = rock_.y - 0.05f;
        pf.life = 1.f;
    }
    float sp = std::hypot(rock_.vx, rock_.vy);
    if (rock_.dead || sp < kStop) {
        rock_.vx = rock_.vy = 0;
        settle();
        return;
    }
    shove(rock_, sweep_);
    sp = std::hypot(rock_.vx, rock_.vy);
    if (rock_.dead || sp < kStop) {
        rock_.vx = rock_.vy = 0;
        settle();
    }
}

void Game::predict() { preview_ = fly(aimVx_, aimVy_, handle_, true); }

void Game::botAim() {
    int i = nextOpen();
    const Shot& s = shot_[i];
    aimVx_ += (s.vx - aimVx_) * 0.45f;
    aimVy_ += (s.vy - aimVy_) * 0.45f;
    handle_ = s.handle;
    think_++;
    if (think_ > 16) launch();
}

void Game::humanAim(const gs::Pad& p) {
    float ax = 0.f;
    float ay = 0.f;
    if (p.down(gs::BTN_LEFT)) ax -= 1.f;
    if (p.down(gs::BTN_RIGHT)) ax += 1.f;
    if (p.down(gs::BTN_UP)) ay += 1.f;
    if (p.down(gs::BTN_DOWN)) ay -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) ax += p.axisX;
    if (std::fabs(p.axisY) > 0.18f) ay += p.axisY;
    if (ax != 0.f) {
        holdX_++;
        float step = holdX_ > 16 ? 0.00040f : 0.00014f;
        aimVx_ = clampf(aimVx_ + ax * step, -kVxLim, kVxLim);
    } else holdX_ = 0;
    if (ay != 0.f) {
        holdY_++;
        float step = holdY_ > 16 ? 0.00032f : 0.00012f;
        aimVy_ = clampf(aimVy_ + ay * step, kVyLo, kVyHi);
    } else holdY_ = 0;
    if (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X)) handle_ = -handle_;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO)) launch();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.18f, 0.28f, 0.14f);
    rules_ = true;
    rules_ = audit();
    showTitle();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (shake_ > 0) shake_--;
    for (Puff& pf : puff_)
        if (pf.life > 0.f) pf.life -= kDt * 1.7f;

    Mode before = mode_;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool back = p.pressed(gs::BTN_MODE);
    bool fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = heldMode_;
        else if (back) showTitle();
    } else if (mode_ == Mode::Title) {
        if (bot_ && clock_ > 0.40f && rules_) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if ((start || fire) && rules_) newGame();
    } else if (mode_ == Mode::Over || mode_ == Mode::Lose) {
        if (!bot_ && (start || fire) && rules_) newGame();
        else if (!bot_ && back) showTitle();
    } else if (!bot_ && (mode_ == Mode::Aim || mode_ == Mode::Slide) && start) {
        heldMode_ = mode_;
        mode_ = Mode::Pause;
    } else if (!bot_ && back && mode_ != Mode::Leave) {
        showTitle();
    } else if (mode_ == Mode::Aim) {
        if (bot_) botAim();
        else humanAim(p);
    }

    if (mode_ == Mode::Slide && before == Mode::Slide) stepSlide();
    else if (mode_ == Mode::Pocket && before == Mode::Pocket) {
        pocketT_ += kDt;
        if (pocketT_ > 0.48f) afterPocket();
    } else if (mode_ == Mode::Judge && before == Mode::Judge) {
        judgeT_ += kDt;
        if (judgeT_ > 0.70f) afterJudge();
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += kDt;
        if (leaveT_ > 0.85f) {
            mode_ = Mode::Over;
            over_ = true;
            won_ = true;
        }
    }

    if (mode_ == Mode::Aim || (mode_ == Mode::Pause && heldMode_ == Mode::Aim)) predict();
    tickAudio();
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 8 || s.y > gs::SCREEN_H + 8 || s.x + s.w < -8 || s.y + s.h < -8) return;
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * float(m.w) / float(m.h);
    spr(m.pick(h), cx, cy, w, h, pal, flip, false);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
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
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::chrome() {
    hud(1, 2, "DRAWER", PAL_GOLD);
    hud(28, 2, "TAPE", PAL_GOLD);
    for (int i = 0; i < 3; i++) {
        int row = 4 + i * 3;
        int pal = held_[i] ? PAL_GREEN : PAL_DIM;
        hud(1, row, tapeName(i), pal);
        char pay[8];
        std::snprintf(pay, sizeof pay, "%d", tapePay(i));
        hud(1, row + 1, pay, pal);
        hud(28, row, tapeName(i), PAL_GOLD);
        hud(28, row + 1, pay, PAL_INK);
    }
    hud(28, 13, "DUE", PAL_INK);
    hud(28, 14, "22", PAL_GOLD);
    char till[8];
    std::snprintf(till, sizeof till, "%d", drawerScore());
    hud(1, 16, "TILL", PAL_INK);
    hud(1, 17, till, matched() ? PAL_WIN : PAL_INK);
    int left = std::max(0, kMaxRocks - stones_);
    char buf[12];
    std::snprintf(buf, sizeof buf, "LEFT %d", left);
    hud(1, 19, buf, left <= 1 ? PAL_ALERT : PAL_DIM);
    hud(1, 22, "SAME PAY", PAL_ALERT);
    hud(1, 23, "STAYS OUT", PAL_DIM);

    char line[12];
    std::snprintf(line, sizeof line, "%+.2f", aimVx_);
    hud(28, 16, "LINE", PAL_DIM);
    hud(28, 17, line, PAL_AIM);
    std::snprintf(line, sizeof line, "%.2f", aimVy_);
    hud(28, 19, "WEIGHT", PAL_DIM);
    hud(28, 20, line, PAL_AIM);
    hud(28, 22, handle_ > 0 ? "OUT TURN" : "IN TURN", PAL_GOLD);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.lineBackdrop[y] = gs::rgb4(2, 1, 1);
    }

    const Mode view = mode_ == Mode::Pause ? heldMode_ : mode_;
    const bool victory = view == Mode::Leave || (view == Mode::Over && won_);
    const bool aiming = view == Mode::Title || view == Mode::Aim;
    float jx = 0.f;
    if (shake_ > 0) jx = (shake_ & 1) ? 1.6f : -1.6f;

    auto shadowAt = [&](float sx, float sy, float h) {
        spr(art_.shadow, sx + 1.f, sy + 3.f, h * 0.9f, h * 0.28f, PAL_INK, false, true);
    };
    auto rockAt = [&](float sx, float sy, int pal, int handle, float h) {
        sprM(art_.rock, sx, sy, h, pal, handle < 0);
        shadowAt(sx, sy, h);
    };

    if (view == Mode::Aim) {
        int pal = PAL_DIM;
        int tape = tapeOf(preview_.lie);
        if (tape >= 0 && !held_[tape]) pal = PAL_GREEN;
        else if (twinOf(preview_.lie) >= 0) pal = PAL_ALERT;
        else if (tape >= 0) pal = PAL_GOLD;
        for (int i = 0; i < preview_.pathN; i++)
            spr(art_.dot, screenX(preview_.pathX[i]), screenY(preview_.pathY[i]), 3.f, 3.f, pal);
        rockAt(screenX(preview_.x), screenY(preview_.y), pal == PAL_GREEN ? PAL_GHOST : PAL_GHOST, handle_, 14.f);
    }

    float hx = screenX(0.f);
    float hy = screenY(kHack);
    bool live = view == Mode::Slide || view == Mode::Judge || view == Mode::Lose;
    if (aiming) rockAt(hx, hy, PAL_ROCK, handle_, 16.f);
    if (live) rockAt(screenX(rock_.x) + jx, screenY(rock_.y), PAL_ROCK, rock_.handle, 16.f);

    if (view == Mode::Pocket && slipI_ >= 0) {
        float u = clampf(pocketT_ / 0.48f, 0.f, 1.f);
        float sx = slipSX_ + (slotX(slipI_) - slipSX_) * u;
        float sy = slipSY_ + (slotY(slipI_) - slipSY_) * u;
        spr(art_.slip, sx, sy, 22.f, 12.f, PAL_SLIP);
    }
    for (int i = 0; i < 3; i++) {
        if (!held_[i] || i == slipI_) continue;
        spr(art_.slip, slotX(i), slotY(i), 22.f, 12.f, PAL_SLIP);
    }
    if (victory) {
        for (int i = 0; i < 3; i++) spr(art_.slip, slotX(i), slotY(i), 22.f, 12.f, PAL_SLIP);
        float pulse = 8.f + std::sin(clock_ * 8.f) * 3.f;
        for (int i = 0; i < 6; i++) {
            float a = clock_ * 3.f + float(i);
            spr(art_.dot, slotX(1) + std::cos(a) * pulse, slotY(1) + std::sin(a) * pulse * 0.6f, 4.f, 4.f, PAL_WIN);
        }
    }

    float skipX = hx + 26.f;
    float skipY = hy + 2.f;
    if (victory) skipX += leaveT_ * 36.f;
    float bob = std::sin(clock_ * 3.f) * 1.2f;
    bool sweeping = view == Mode::Slide && sweep_ > 0.5f;
    int pose = sweeping ? 1 : 0;
    sprM(art_.skip[pose], skipX, skipY + bob, pose ? 20.f : 30.f, PAL_SKIP, false);
    float bx = sweeping ? screenX(rock_.x) + 8.f : skipX - 8.f;
    float by = sweeping ? screenY(rock_.y) - 6.f : skipY - 4.f;
    float wag = sweeping ? std::sin(clock_ * 18.f) * 3.f : 0.f;
    spr(art_.broom, bx + wag, by, float(art_.broom.w), float(art_.broom.h), PAL_BROOM, handle_ < 0);
    if (aiming) spr(art_.arrow, hx, hy + 14.f, 14.f, 8.f, PAL_GOLD, handle_ > 0);
    spr(art_.dot, screenX(0.f), screenY(kTee), 4.f, 4.f, PAL_GOLD);

    for (const Puff& pf : puff_) {
        if (pf.life <= 0.f) continue;
        float h = 4.f + pf.life * 8.f;
        spr(art_.puff, screenX(pf.x), screenY(pf.y), h, h * 0.8f, PAL_PUFF);
    }

    chrome();
    if (view == Mode::Title) {
        hudC(0, "S3 CURLTAPE", PAL_GOLD);
        hudC(1, "A SHORT CURL", PAL_INK);
        hudC(26, "ARROWS AIM   X HANDLE   Z THROW", PAL_INK);
        hudC(27, rules_ ? "HOLD Z TO SWEEP    ENTER" : "NO LINE", rules_ ? PAL_DIM : PAL_ALERT);
        return;
    }
    if (view == Mode::Pause) {
        hudC(0, "PAUSED", PAL_GOLD);
        hudC(1, "ENTER", PAL_DIM);
        return;
    }
    if (victory) {
        hudC(0, "DRAWER MATCHES", PAL_WIN);
        hudC(1, "YOU LEAVE", PAL_GOLD);
        if (view == Mode::Over && !bot_) hudC(27, "ENTER", PAL_DIM);
        return;
    }
    if (view == Mode::Lose || (view == Mode::Over && !won_)) {
        hudC(0, "STILL OPEN", PAL_ALERT);
        hudC(1, reason_[0] ? reason_ : "DOES NOT MATCH", PAL_ALERT);
        if (!bot_) hudC(27, "ENTER", PAL_DIM);
        return;
    }
    if (view == Mode::Slide) {
        hudC(0, "SLIDING", PAL_INK);
        hudC(1, sweep_ > 0.5f ? "SWEEP" : (handle_ > 0 ? "OUT TURN" : "IN TURN"), PAL_GOLD);
        return;
    }
    if (view == Mode::Pocket) {
        hudC(0, "IN THE DRAWER", PAL_GREEN);
        hudC(1, slipI_ >= 0 ? tapeName(slipI_) : "", PAL_GOLD);
        return;
    }
    if (view == Mode::Judge) {
        hudC(0, reason_, twinOf(lastLie_) >= 0 ? PAL_ALERT : PAL_INK);
        hudC(1, twinOf(lastLie_) >= 0 ? "STAYS OUT" : "NOT THE TAPE", PAL_ALERT);
        return;
    }

    char top[24];
    const char* sub = "NOT THE TAPE";
    int pal = PAL_INK;
    int subPal = PAL_DIM;
    int tape = tapeOf(preview_.lie);
    if (tape >= 0 && !held_[tape]) {
        std::snprintf(top, sizeof top, "%s %d", lieName(preview_.lie), liePay(preview_.lie));
        sub = "OPEN";
        pal = PAL_GREEN;
        subPal = PAL_GREEN;
    } else if (tape >= 0) {
        std::snprintf(top, sizeof top, "%s %d", lieName(preview_.lie), liePay(preview_.lie));
        sub = "ALREADY IN";
        pal = PAL_GOLD;
        subPal = PAL_ALERT;
    } else if (twinOf(preview_.lie) >= 0) {
        std::snprintf(top, sizeof top, "%s %d", lieName(preview_.lie), liePay(preview_.lie));
        sub = twinOf(preview_.lie) == 0 ? "NOT THE BUTTON" : twinOf(preview_.lie) == 1 ? "NOT THE GUARD" : "NOT THE BITE";
        pal = PAL_ALERT;
        subPal = PAL_ALERT;
    } else {
        std::snprintf(top, sizeof top, "%s", lieName(preview_.lie));
    }
    hudC(0, top, pal);
    hudC(1, sub, subPal);
    hudC(27, "Z THROW", PAL_DIM);
}

}  // namespace curltape
