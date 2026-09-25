#include "game/curl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace curl {
namespace {

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

bool anyMoving(const Sheet& sh) {
    for (int i = 0; i < sh.n; i++)
        if (sh.r[i].moving && !sh.r[i].dead) return true;
    return false;
}

void park(Rock& r) {
    if (std::hypot(r.vx, r.vy) < kStop) {
        r.vx = r.vy = 0;
        r.moving = false;
    }
}

void stepRock(Rock& r, float sweep, float dt) {
    float sp = std::hypot(r.vx, r.vy);
    if (sp < kStop) {
        r.vx = r.vy = 0;
        r.moving = false;
        return;
    }
    if (sp > 75.f) {
        r.vx *= 75.f / sp;
        r.vy *= 75.f / sp;
        sp = 75.f;
    }
    float sw = clampf(sweep, 0.f, 1.f);
    float fr = kA0 * (1.f - kSweepCut * sw);
    float pace = clampf((20.f - sp) / 20.f, 0.f, 1.f);
    float curlSign = r.curl > 0 ? 1.f : r.curl < 0 ? -1.f : 0.f;
    float curlA = curlSign * kCurl * (1.f - kCurlSweep * sw) * (0.42f + 0.58f * pace);
    float dpsi = (curlA / sp) * dt;
    dpsi = clampf(dpsi, -0.35f, 0.35f);
    float cs = std::cos(dpsi);
    float sn = std::sin(dpsi);
    float nvx = r.vx * cs - r.vy * sn;
    float nvy = r.vx * sn + r.vy * cs;
    float ns = sp - fr * dt;
    if (ns <= kStop) {
        r.vx = r.vy = 0;
        r.moving = false;
        return;
    }
    r.vx = nvx / sp * ns;
    r.vy = nvy / sp * ns;
    r.x += r.vx * dt;
    r.y += r.vy * dt;
}

bool collide(Rock& a, Rock& b) {
    if (a.dead || b.dead) return false;
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float d2 = dx * dx + dy * dy;
    const float minD = kStoneR * 2.f;
    if (d2 >= minD * minD || d2 < 1e-8f) return false;
    float d = std::sqrt(d2);
    float nx = dx / d;
    float ny = dy / d;
    float overlap = minD - d;
    bool aWas = a.moving;
    bool bWas = b.moving;
    a.x -= nx * overlap * 0.5f;
    a.y -= ny * overlap * 0.5f;
    b.x += nx * overlap * 0.5f;
    b.y += ny * overlap * 0.5f;
    if (!aWas && !bWas) return false;
    float rv = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
    if (rv >= 0.f) return false;
    float j = -(1.f + kRestitution) * rv * 0.5f;
    a.vx -= j * nx;
    a.vy -= j * ny;
    b.vx += j * nx;
    b.vy += j * ny;
    if (!aWas) a.curl = 0;
    if (!bWas) b.curl = 0;
    a.moving = true;
    b.moving = true;
    park(a);
    park(b);
    return j > 0.35f;
}

void bury(Rock& r) {
    if (r.dead || r.moving) return;
    if (!inPlay(r.x, r.y)) r.dead = true;
}

bool stepSheet(Sheet& sh, int sweepIx, float sweep, float dt, bool* hit) {
    bool moving = false;
    for (int i = 0; i < sh.n; i++) {
        Rock& r = sh.r[i];
        if (!r.moving || r.dead) continue;
        moving = true;
        stepRock(r, i == sweepIx ? sweep : 0.f, dt);
        if (r.y > kBackLine + 3.f || r.y < kImgBot - 2.f || std::fabs(r.x) > kSideLine + 3.f) {
            r.dead = true;
            r.moving = false;
            r.vx = r.vy = 0;
        }
    }
    for (int iter = 0; iter < 3; iter++) {
        for (int i = 0; i < sh.n; i++) {
            if (sh.r[i].dead) continue;
            for (int j = i + 1; j < sh.n; j++) {
                if (sh.r[j].dead) continue;
                if (collide(sh.r[i], sh.r[j]) && hit) *hit = true;
            }
        }
    }
    for (int i = 0; i < sh.n; i++) bury(sh.r[i]);
    return moving;
}

void forceRest(Sheet& sh) {
    for (int i = 0; i < sh.n; i++) {
        Rock& r = sh.r[i];
        r.vx = r.vy = 0;
        r.moving = false;
        bury(r);
    }
}

void simulate(Sheet& sh, int sweepIx, float sweep) {
    for (int n = 0; n < kSimSteps; n++) {
        if (!anyMoving(sh)) break;
        stepSheet(sh, sweepIx, sweep, kDt, nullptr);
    }
    forceRest(sh);
}

void compact(Sheet& sh) {
    int w = 0;
    for (int i = 0; i < sh.n; i++)
        if (!sh.r[i].dead) sh.r[w++] = sh.r[i];
    sh.n = w;
}

int launchRock(Sheet& sh, const Shot& s, int side) {
    if (sh.n >= 8) return -1;
    Rock& r = sh.r[sh.n];
    r = Rock{};
    r.x = clampf(s.aim, -5.4f, 5.4f);
    r.y = kRelease;
    r.vy = speedForPower(s.power);
    r.side = side;
    r.curl = s.curl > 0 ? 1 : -1;
    r.moving = true;
    return sh.n++;
}

float closest(const Sheet& sh, int side) {
    float b = 80.f;
    for (int i = 0; i < sh.n; i++) {
        const Rock& r = sh.r[i];
        if (r.dead || r.moving || r.side != side) continue;
        float d = buttonDist(r.x, r.y);
        if (d < b) b = d;
    }
    return b;
}

float evalFor(const Sheet& sh, int side) {
    float mine = closest(sh, side);
    float opp = closest(sh, 1 - side);
    float pts = 0.f;
    if (mine < opp - 0.05f) pts = 1.f;
    else if (opp < mine - 0.05f) pts = -1.f;
    float margin = clampf(opp - mine, -12.f, 12.f);
    float bite = clampf(6.f - mine, 0.f, 6.f);
    return pts * 30.f + margin + bite * 0.35f;
}

Sheet play(Sheet sh, const Shot& s, int side) {
    int ix = launchRock(sh, s, side);
    if (ix < 0) return sh;
    simulate(sh, ix, clampf(s.sweep, 0.f, 1.f));
    compact(sh);
    return sh;
}

std::string weightBar(float p) {
    const int n = 16;
    int fill = int(std::lround(clampf(p, 0.f, 1.f) * float(n)));
    if (fill > n) fill = n;
    int sweet = int(std::lround(0.55f * float(n - 1)));
    std::string s;
    s.resize(size_t(n), '-');
    for (int i = 0; i < n; i++) {
        if (i == sweet) s[size_t(i)] = '+';
        else if (i < fill) s[size_t(i)] = '#';
    }
    return s;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    camTop_ = camFor(kTee, 0.48f);
    camSet_ = true;
    if (bot_) beginMatch();
    else mode_ = Mode::Title;
}

void Game::beginMatch() {
    red_ = yel_ = 0;
    end_ = 1;
    endsDone_ = 0;
    shot_ = 0;
    over_ = false;
    won_ = false;
    sheet_ = Sheet{};
    aim_ = 0;
    curl_ = 1;
    power_ = 0.55f;
    planSweep_ = 0;
    osc_ = 0.4f;
    lastTaker_ = -1;
    fanStep_ = -1;
    redDist_ = yelDist_ = 99.f;
    ghostN_ = 0;
    ghostIn_ = false;
    for (auto& p : puffs_) p.life = 0;
    enterAim();
}

void Game::enterAim() {
    mode_ = Mode::Aim;
    picked_ = false;
    manual_ = false;
    think_ = 0;
    thrown_ = -1;
    aiThrow_ = false;
    slideFrames_ = 0;
    if (!sys_) return;
    if (throwSide() == 0) sys_->setLight(190, 36, 36);
    else sys_->setLight(210, 160, 36);
}

void Game::blip(float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = hold;
}

void Game::tickAudio() {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= kFrameDt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
        }
    }
    if (fanStep_ < 0 || mode_ != Mode::Result) return;
    fanT_ -= kFrameDt;
    if (fanT_ > 0.f) return;
    const float winN[] = {523.f, 659.f, 784.f, 1046.f};
    const float loseN[] = {392.f, 330.f, 262.f};
    if (won_) {
        if (fanStep_ < 4) {
            blip(winN[fanStep_], 0.09f, 0.15f);
            sys_->apu.tone(1, winN[fanStep_] * 0.5f, 0.04f);
            fanStep_++;
            fanT_ = 0.15f;
        }
    } else if (fanStep_ < 3) {
        blip(loseN[fanStep_], 0.06f, 0.18f);
        fanStep_++;
        fanT_ = 0.18f;
    }
}

Shot Game::choose(int side) const {
    Shot best;
    best.aim = -1.5f;
    best.power = 0.55f;
    best.curl = 1;
    best.sweep = 0;
    float bestV = -1e9f;
    bool any = false;
    auto consider = [&](Shot s) {
        s.power = clampf(s.power, 0.02f, 1.f);
        s.aim = clampf(s.aim, -5.2f, 5.2f);
        s.curl = s.curl >= 0 ? 1 : -1;
        s.sweep = clampf(s.sweep, 0.f, 1.f);
        float v = evalFor(play(sheet_, s, side), side);
        if (!any || v > bestV) {
            any = true;
            bestV = v;
            best = s;
        }
    };

    const bool fine = side == 0;
    const float aimsFine[] = {-4.8f, -4.f, -3.2f, -2.4f, -1.6f, -0.8f, 0.f, 0.8f, 1.6f, 2.4f, 3.2f, 4.f, 4.8f};
    const float aimsCoarse[] = {-2.4f, -1.2f, 0.f, 1.2f, 2.4f};
    const float powFine[] = {0.24f, 0.36f, 0.46f, 0.54f, 0.62f, 0.72f, 0.84f, 0.94f, 1.f};
    const float powCoarse[] = {0.42f, 0.52f, 0.62f, 0.78f, 0.92f};
    const float* aims = fine ? aimsFine : aimsCoarse;
    const float* pows = fine ? powFine : powCoarse;
    int nAim = fine ? 13 : 5;
    int nPow = fine ? 9 : 5;
    int nSw = fine ? 2 : 1;
    for (int ia = 0; ia < nAim; ia++) {
        for (int ip = 0; ip < nPow; ip++) {
            for (int c = -1; c <= 1; c += 2) {
                for (int is = 0; is < nSw; is++) {
                    Shot s;
                    s.aim = aims[ia];
                    s.power = pows[ip];
                    s.curl = c;
                    s.sweep = is ? 0.5f : 0.f;
                    consider(s);
                }
            }
        }
    }
    for (int i = 0; i < sheet_.n; i++) {
        const Rock& o = sheet_.r[i];
        if (o.dead || o.moving || o.side == side) continue;
        for (int c = -1; c <= 1; c += 2) {
            for (float lead : {0.8f, 1.6f, 2.4f}) {
                for (float p : {0.78f, 0.9f, 1.f}) {
                    Shot s;
                    s.aim = o.x - float(c) * lead;
                    s.power = p;
                    s.curl = c;
                    s.sweep = 0;
                    consider(s);
                }
            }
        }
    }
    if (fine) {
        Shot base = best;
        for (int ia = -6; ia <= 6; ia++) {
            for (int ip = -4; ip <= 4; ip++) {
                Shot s = base;
                s.aim = base.aim + float(ia) * 0.14f;
                s.power = base.power + float(ip) * 0.012f;
                consider(s);
                s.sweep = base.sweep > 0.25f ? 0.f : 0.55f;
                consider(s);
            }
        }
        if (bestV < 25.f) {
            for (int ia = 0; ia <= 32; ia++) {
                float aim = -4.8f + float(ia) * 0.3f;
                for (int ip = 0; ip <= 16; ip++) {
                    float p = 0.38f + float(ip) * 0.038f;
                    for (int c = -1; c <= 1; c += 2) {
                        Shot s;
                        s.aim = aim;
                        s.power = p;
                        s.curl = c;
                        s.sweep = 0;
                        consider(s);
                        s.sweep = 0.5f;
                        consider(s);
                    }
                }
            }
        }
    }
    if (side != 0) {
        float dir = best.aim > 0.18f ? 1.f : best.aim < -0.18f ? -1.f : float(best.curl >= 0 ? 1 : -1);
        best.aim = clampf(best.aim + 0.72f * dir, -5.f, 5.f);
        best.power = clampf(best.power + 0.045f, 0.02f, 1.f);
    }
    return best;
}

void Game::predict(int side, float sweep) {
    ghostN_ = 0;
    ghostIn_ = false;
    Shot s;
    s.aim = aim_;
    s.power = power_;
    s.curl = curl_;
    s.sweep = sweep;
    Sheet sh = sheet_;
    int ix = launchRock(sh, s, side);
    if (ix < 0) return;
    for (int n = 0; n < kSimSteps; n++) {
        if (!sh.r[ix].moving || sh.r[ix].dead) break;
        stepSheet(sh, ix, sweep, kDt, nullptr);
        if ((n % 14) == 0 && ghostN_ < 40 && !sh.r[ix].dead) {
            ghostX_[ghostN_] = sh.r[ix].x;
            ghostY_[ghostN_] = sh.r[ix].y;
            ghostN_++;
        }
    }
    if (!sh.r[ix].dead) {
        ghostStopY_ = sh.r[ix].y;
        ghostIn_ = true;
        if (ghostN_ < 40) {
            ghostX_[ghostN_] = sh.r[ix].x;
            ghostY_[ghostN_] = sh.r[ix].y;
            ghostN_++;
        }
    } else if (ghostN_ > 0) {
        ghostStopY_ = ghostY_[ghostN_ - 1];
        ghostIn_ = true;
    }
}

void Game::launch(int side, bool ai) {
    Shot s;
    s.aim = aim_;
    s.power = power_;
    s.curl = curl_;
    s.sweep = ai ? planSweep_ : 0.f;
    int ix = launchRock(sheet_, s, side);
    if (ix < 0) return;
    thrown_ = ix;
    aiThrow_ = ai;
    if (!ai) planSweep_ = 0;
    slideFrames_ = 0;
    mode_ = Mode::Slide;
    snap_ = true;
    ghostN_ = 0;
    ghostIn_ = false;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.11f, 280.f, 0.09f);
    sys_->rumble(0.22f, 0.08f, 60);
}

void Game::scoreEnd() {
    redDist_ = closest(sheet_, 0);
    yelDist_ = closest(sheet_, 1);
    if (redDist_ < yelDist_ - 0.05f) {
        red_++;
        lastTaker_ = 0;
        blip(659.f, 0.08f, 0.14f);
        if (sys_) sys_->rumble(0.15f, 0.35f, 80);
    } else if (yelDist_ < redDist_ - 0.05f) {
        yel_++;
        lastTaker_ = 1;
        blip(311.f, 0.07f, 0.16f);
    } else {
        lastTaker_ = -1;
        blip(196.f, 0.05f, 0.12f);
    }
    endsDone_++;
    end_++;
    hold_ = bot_ ? 20 : 100;
    mode_ = Mode::Between;
}

void Game::finishMatch() {
    over_ = true;
    won_ = endsDone_ == kEnds && red_ > yel_;
    mode_ = Mode::Result;
    fanStep_ = 0;
    fanT_ = 0.05f;
    if (sys_) sys_->setLight(won_ ? 40 : 40, won_ ? 180 : 30, won_ ? 60 : 30);
}

void Game::onRest() {
    forceRest(sheet_);
    compact(sheet_);
    thrown_ = -1;
    aiThrow_ = false;
    shot_++;
    if (shot_ >= kPerSide * 2) scoreEnd();
    else enterAim();
}

void Game::updateAim() {
    int side = throwSide();
    bool ai = bot_ || side == 1;
    if (ai) {
        if (!picked_) {
            Shot s = choose(side);
            aim_ = s.aim;
            power_ = s.power;
            curl_ = s.curl >= 0 ? 1 : -1;
            planSweep_ = s.sweep;
            picked_ = true;
            think_ = bot_ ? 3 : 28;
        } else if (--think_ <= 0) {
            launch(side, true);
            return;
        }
        predict(side, planSweep_);
        return;
    }
    float dir = sys_->pad.axisX;
    if (sys_->pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) dir += 1.f;
    aim_ = clampf(aim_ + clampf(dir, -1.f, 1.f) * 2.5f * kFrameDt, -4.8f, 4.8f);
    if (sys_->pad.pressed(gs::BTN_B)) curl_ = -curl_;
    if (sys_->pad.down(gs::BTN_UP)) {
        manual_ = true;
        power_ = clampf(power_ + 0.75f * kFrameDt, 0.05f, 1.f);
    }
    if (sys_->pad.down(gs::BTN_DOWN)) {
        manual_ = true;
        power_ = clampf(power_ - 0.75f * kFrameDt, 0.05f, 1.f);
    }
    if (sys_->pad.pressed(gs::BTN_A)) {
        launch(side, false);
        return;
    }
    if (!manual_) {
        osc_ += kFrameDt;
        float s = 0.5f + 0.5f * std::sin(osc_ * 2.15f);
        power_ = 0.14f + 0.82f * s;
    }
    predict(side, 0.f);
}

void Game::updateSlide() {
    if (thrown_ < 0 || thrown_ >= sheet_.n) {
        onRest();
        return;
    }
    float sw = 0.f;
    if (aiThrow_) sw = planSweep_;
    else if (sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_TURBO) || sys_->pad.down(gs::BTN_A))
        sw = 1.f;
    bool hit = false;
    for (int i = 0; i < kSubs; i++) stepSheet(sheet_, thrown_, sw, kDt, &hit);
    if (hit && hitCool_ == 0 && sys_) {
        sys_->apu.noiseBurst(0.2f, 700.f, 0.07f);
        sys_->rumble(0.45f, 0.25f, 50);
        hitCool_ = 8;
    }
    if (sw > 0.4f && thrown_ >= 0 && thrown_ < sheet_.n && sheet_.r[thrown_].moving && (slideFrames_ % 4) == 0) {
        const Rock& r = sheet_.r[thrown_];
        for (auto& p : puffs_) {
            if (p.life > 0.f) continue;
            p.x = r.x;
            p.y = r.y - 0.7f;
            p.life = 0.32f;
            break;
        }
        if (sys_ && (slideFrames_ % 8) == 0) sys_->apu.noiseBurst(0.04f, 1700.f, 0.03f);
    }
    slideFrames_++;
    if (!anyMoving(sheet_) || slideFrames_ > 60 * 6) onRest();
}

float Game::focusY() const {
    if (mode_ == Mode::Slide && thrown_ >= 0 && thrown_ < sheet_.n && sheet_.r[thrown_].moving && !sheet_.r[thrown_].dead)
        return sheet_.r[thrown_].y;
    if (mode_ == Mode::Aim && ghostIn_) return clampf(ghostStopY_, kHogLine, kBackLine);
    float best = 1e9f;
    float by = kTee;
    bool found = false;
    for (int i = 0; i < sheet_.n; i++) {
        const Rock& r = sheet_.r[i];
        if (r.dead || r.moving) continue;
        float d = buttonDist(r.x, r.y);
        if (d < best) {
            best = d;
            by = r.y;
            found = true;
        }
    }
    if (mode_ == Mode::Title) return kTee;
    return found ? by : kTee;
}

void Game::seekCam(float focus, float bias, bool snap) {
    float want = camFor(focus, bias);
    if (!camSet_ || snap) {
        camTop_ = want;
        camSet_ = true;
        return;
    }
    camTop_ += (want - camTop_) * 0.22f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kFrameDt;
    if (hitCool_ > 0) hitCool_--;
    for (auto& p : puffs_)
        if (p.life > 0.f) p.life -= kFrameDt;
    tickAudio();

    if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = held_;
        draw();
        return;
    }
    if (!bot_ && mode_ != Mode::Title && mode_ != Mode::Result && sys.pad.pressed(gs::BTN_START)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginMatch();
        else if (sys.pad.pressed(gs::BTN_MODE)) sys.eject();
    } else if (mode_ == Mode::Aim) {
        updateAim();
    } else if (mode_ == Mode::Slide) {
        updateSlide();
    } else if (mode_ == Mode::Between) {
        bool skip = !bot_ && (sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_C));
        if (--hold_ <= 0 || skip) {
            if (endsDone_ >= kEnds) finishMatch();
            else {
                sheet_ = Sheet{};
                shot_ = 0;
                enterAim();
            }
        }
    } else if (mode_ == Mode::Result) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) beginMatch();
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            fanStep_ = -1;
        }
    }

    float bias = (mode_ == Mode::Slide) ? 0.7f : 0.48f;
    seekCam(focusY(), bias, snap_);
    snap_ = false;
    draw();
}

void Game::blit(const gs::Image& img, float cx, float cy, int w, int h, int pal, int fog) {
    if (!sys_ || w < 1 || h < 1) return;
    if (cx < -40 || cy < -40 || cx > gs::SCREEN_W + 40 || cy > gs::SCREEN_H + 40) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.x = int16_t(std::lround(cx - float(w) * 0.5f));
    s.y = int16_t(std::lround(cy - float(h) * 0.5f));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(clampf(float(fog), 0.f, 16.f));
    sys_->vdp.sprite(s);
}

void Game::drawRock(float x, float y, int side, bool shadow) {
    float sx = screenX(x);
    float sy = screenY(y, camTop_);
    int d = int(std::lround(kStoneR * 2.f * kPx));
    if (d < 9) d = 9;
    const gs::Image& img = art_.stone.pick(float(d));
    if (shadow && sy > -20 && sy < gs::SCREEN_H + 20) {
        gs::Sprite s;
        s.img = img;
        s.w = int16_t(d);
        s.h = int16_t(d);
        s.x = int16_t(std::lround(sx - float(d) * 0.5f + 1.f));
        s.y = int16_t(std::lround(sy - float(d) * 0.5f + 3.f));
        s.shadow = true;
        sys_->vdp.sprite(s);
    }
    blit(img, sx, sy, d, d, side == 0 ? PAL_RED : PAL_YEL);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int band = (y / 5) & 3;
        v.lineBackdrop[y] = band == 0 ? gs::rgb4(1, 1, 3) : band == 1 ? gs::rgb4(1, 2, 4) : gs::rgb4(0, 1, 2);
    }

    if (mode_ == Mode::Title) {
        drawRock(0.f, kTee, 0, true);
        drawRock(1.85f, kTee + 0.45f, 1, true);
    }
    for (int i = 0; i < sheet_.n; i++) {
        const Rock& r = sheet_.r[i];
        if (r.dead) continue;
        drawRock(r.x, r.y, r.side, true);
    }
    if (mode_ == Mode::Aim) {
        for (int i = 0; i < ghostN_; i++) {
            float sx = screenX(ghostX_[i]);
            float sy = screenY(ghostY_[i], camTop_);
            blit(art_.dot, sx, sy, 5, 5, PAL_DOT);
        }
        if (ghostN_ > 0) {
            float bx = screenX(ghostX_[ghostN_ - 1]);
            float by = screenY(ghostY_[ghostN_ - 1], camTop_);
            const gs::Image& br = art_.broom.pick(22);
            blit(br, bx, by - 8.f, 10, 22, PAL_BROOM);
        }
    }
    for (const auto& p : puffs_) {
        if (p.life <= 0.f) continue;
        float sx = screenX(p.x);
        float sy = screenY(p.y, camTop_);
        int fog = int((1.f - p.life / 0.32f) * 12.f);
        blit(art_.puff.pick(12), sx, sy, 12, 10, PAL_PUFF, fog);
    }
    {
        float sx = screenX(kSideLine + 0.45f);
        float sy = screenY(kTee - 0.4f, camTop_);
        blit(art_.skip.pick(22), sx, sy, 16, 22, PAL_SKIP);
    }

    gs::Sprite rink;
    rink.img = art_.rink;
    rink.x = int16_t(iceX());
    rink.y = int16_t(std::lround(screenY(kImgTop, camTop_)));
    rink.w = int16_t(art_.rink.w);
    rink.h = int16_t(art_.rink.h);
    rink.pal = PAL_ICE;
    rink.fog = mode_ == Mode::Title ? 5 : 0;
    v.sprite(rink);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(0, "S3 CURL", PAL_GOLD);
        hudC(1, "CLOSEST TO THE BUTTON TAKES THE END", PAL_HUD);
        hudC(26, "FOUR ENDS   YOU KEEP THE HAMMER", PAL_GOLD);
        hudC(27, "ARROWS AIM  X CURL  Z THROW  C SWEEP", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(13, "PAUSED", PAL_WIN);
        hudC(27, "START", PAL_HUD);
    }

    int showEnd = end_ > kEnds ? kEnds : end_;
    std::snprintf(line, sizeof line, "END %d/%d", showEnd, kEnds);
    hud(0, 0, line, PAL_GOLD);
    std::snprintf(line, sizeof line, "RED %d", red_);
    hud(12, 0, line, PAL_ALERT);
    std::snprintf(line, sizeof line, "YEL %d", yel_);
    hud(22, 0, line, PAL_GOLD);
    int rk = shot_ < 8 ? shot_ + 1 : 8;
    std::snprintf(line, sizeof line, "RK %d/8", rk);
    hud(31, 0, line, PAL_DIM);

    if (mode_ == Mode::Result) {
        if (red_ > yel_) hudC(1, "RED TAKES THE MATCH", PAL_WIN);
        else if (yel_ > red_) hudC(1, "YELLOW TAKES THE MATCH", PAL_ALERT);
        else hudC(1, "THE MATCH IS TIED", PAL_GOLD);
        hudC(26, "FOUR ENDS", PAL_DIM);
        hudC(27, "START PLAYS AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Between) {
        if (lastTaker_ == 0) hudC(1, "RED IS CLOSER", PAL_ALERT);
        else if (lastTaker_ == 1) hudC(1, "YELLOW IS CLOSER", PAL_GOLD);
        else hudC(1, "BLANK END", PAL_DIM);
        char a[24], b[24];
        if (redDist_ > 40.f) std::snprintf(a, sizeof a, "RED --");
        else std::snprintf(a, sizeof a, "RED %.1fFT", redDist_);
        if (yelDist_ > 40.f) std::snprintf(b, sizeof b, "YEL --");
        else std::snprintf(b, sizeof b, "YEL %.1fFT", yelDist_);
        std::snprintf(line, sizeof line, "%s  %s", a, b);
        hudC(26, line, PAL_HUD);
        hudC(27, "C CONTINUES", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Pause) return;

    if (mode_ == Mode::Slide || mode_ == Mode::Aim) {
        bool you = throwSide() == 0;
        hudC(1, you ? "YOUR HAMMER ROCK" : "YELLOW TO THROW", you ? PAL_ALERT : PAL_GOLD);
    }
    if (mode_ == Mode::Slide) {
        bool sweeping = aiThrow_ ? planSweep_ > 0.2f
                                 : sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_TURBO) || sys_->pad.down(gs::BTN_A);
        hudC(26, curl_ > 0 ? "HANDLE RIGHT" : "HANDLE LEFT", PAL_HUD);
        hudC(27, sweeping ? "SWEEPING" : "HOLD C TO SWEEP", PAL_GOLD);
        return;
    }
    hudC(26, curl_ > 0 ? "HANDLE RIGHT   X TURNS" : "HANDLE LEFT   X TURNS", PAL_HUD);
    std::string bar = weightBar(power_);
    hud(0, 27, manual_ ? "HAND" : "AUTO", PAL_DIM);
    hud(6, 27, bar, PAL_GOLD);
    hud(24, 27, "Z THROW", PAL_HUD);
}

}  // namespace curl
