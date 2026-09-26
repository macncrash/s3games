#include "game/curlgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace curlgold {
namespace {

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

struct Tally {
    int taker = -1;
    int gold = 0;
    int cream = 0;
    int pts = 0;
    float redD = 99.f;
    float yelD = 99.f;
};

struct Trace {
    float x[36]{};
    float y[36]{};
    int n = 0;
    float stopY = 100.f;
    bool in = false;
};

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

void bury(Rock& r) {
    if (r.dead || r.moving) return;
    if (r.y < kHog || r.y > kBack || std::fabs(r.x) > kSide) r.dead = true;
}

void stepRock(Rock& r, float sweep, float dt) {
    float sp = std::hypot(r.vx, r.vy);
    if (sp < kStop) {
        r.vx = r.vy = 0;
        r.moving = false;
        return;
    }
    if (sp > 80.f) {
        r.vx *= 80.f / sp;
        r.vy *= 80.f / sp;
        sp = 80.f;
    }
    float sw = clampf(sweep, 0.f, 1.f);
    float fr = kA0 * (1.f - kSweepCut * sw);
    float slow = clampf((16.f - sp) / 16.f, 0.f, 1.f);
    float curlA = float(r.curl) * kCurl * (1.f - kCurlSweep * sw) * (0.30f + 0.70f * slow);
    float px = r.vy / sp;
    float py = -r.vx / sp;
    r.vx += px * curlA * dt;
    r.vy += py * curlA * dt;
    sp = std::hypot(r.vx, r.vy);
    if (sp < 1e-4f) {
        r.vx = r.vy = 0;
        r.moving = false;
        return;
    }
    float ns = sp - fr * dt;
    if (ns <= kStop) {
        r.vx = r.vy = 0;
        r.moving = false;
        return;
    }
    r.vx *= ns / sp;
    r.vy *= ns / sp;
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
    a.moving = true;
    b.moving = true;
    if (j > 0.4f) {
        if (aWas) a.curl = 0;
        if (bWas) b.curl = 0;
    } else {
        if (!aWas) a.curl = 0;
        if (!bWas) b.curl = 0;
    }
    park(a);
    park(b);
    return j > 0.3f;
}

void killIfOut(Rock& r) {
    if (r.dead || !r.moving) return;
    if (r.y > kBack || std::fabs(r.x) > kSide || r.y < kImgBot + 1.f) {
        r.dead = true;
        r.moving = false;
        r.vx = r.vy = 0;
    }
}

bool stepSheet(Sheet& sh, int sweepIx, float sweep, float dt, bool* hit) {
    for (int i = 0; i < sh.n; i++) {
        Rock& r = sh.r[i];
        if (!r.moving || r.dead) continue;
        stepRock(r, i == sweepIx ? sweep : 0.f, dt);
        killIfOut(r);
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
    for (int i = 0; i < sh.n; i++) {
        killIfOut(sh.r[i]);
        bury(sh.r[i]);
    }
    return anyMoving(sh);
}

void forceRest(Sheet& sh) {
    for (int i = 0; i < sh.n; i++) {
        Rock& r = sh.r[i];
        r.vx = r.vy = 0;
        r.moving = false;
        bury(r);
    }
}

void simulate(Sheet& sh, int ix, float sweep, Trace* tr) {
    for (int n = 0; n < kSimSteps; n++) {
        if (!anyMoving(sh)) break;
        stepSheet(sh, ix, sweep, kDt, nullptr);
        if (tr && (n % 16) == 0 && tr->n < 36 && ix >= 0 && !sh.r[ix].dead) {
            tr->x[tr->n] = sh.r[ix].x;
            tr->y[tr->n] = sh.r[ix].y;
            tr->n++;
        }
    }
    forceRest(sh);
    if (!tr || ix < 0) return;
    if (!sh.r[ix].dead) {
        tr->stopY = sh.r[ix].y;
        tr->in = true;
        if (tr->n < 36) {
            tr->x[tr->n] = sh.r[ix].x;
            tr->y[tr->n] = sh.r[ix].y;
            tr->n++;
        }
    } else if (tr->n > 0) {
        tr->stopY = tr->y[tr->n - 1];
        tr->in = true;
    }
}

int launchRock(Sheet& sh, const Shot& s, int side) {
    if (sh.n >= kRocks) return -1;
    Rock& r = sh.r[sh.n];
    r = Rock{};
    r.x = clampf(s.aim, -5.6f, 5.6f);
    r.y = kRelease;
    r.vy = speedForPower(s.power);
    r.side = side;
    r.curl = s.curl >= 0 ? 1 : -1;
    r.moving = true;
    return sh.n++;
}

void compact(Sheet& sh) {
    int w = 0;
    for (int i = 0; i < sh.n; i++)
        if (!sh.r[i].dead) sh.r[w++] = sh.r[i];
    sh.n = w;
}

float nearest(const Sheet& sh, int side) {
    float b = 99.f;
    for (int i = 0; i < sh.n; i++) {
        const Rock& r = sh.r[i];
        if (r.dead || r.moving || r.side != side) continue;
        float d = buttonDist(r.x, r.y);
        if (d <= kHouse && d < b) b = d;
    }
    return b;
}

Tally measure(const Sheet& sh) {
    Tally t;
    t.redD = nearest(sh, 0);
    t.yelD = nearest(sh, 1);
    int taker = -1;
    if (t.redD < t.yelD) taker = 0;
    else if (t.yelD < t.redD) taker = 1;
    else return t;
    float opp = taker == 0 ? t.yelD : t.redD;
    for (int i = 0; i < sh.n; i++) {
        const Rock& r = sh.r[i];
        if (r.dead || r.moving || r.side != taker) continue;
        float d = buttonDist(r.x, r.y);
        if (d > kHouse || d >= opp) continue;
        if (d <= kGold) t.gold++;
        else t.cream++;
    }
    t.taker = taker;
    t.pts = t.gold * 2 + t.cream;
    return t;
}

Sheet play(Sheet sh, const Shot& s, int side) {
    int ix = launchRock(sh, s, side);
    if (ix < 0) return sh;
    simulate(sh, ix, clampf(s.sweep, 0.f, 1.f), nullptr);
    compact(sh);
    return sh;
}

float evalFor(const Sheet& sh, int side) {
    Tally t = measure(sh);
    float mine = nearest(sh, side);
    float opp = nearest(sh, 1 - side);
    float v = clampf(opp - mine, -12.f, 12.f) * 4.f;
    v += clampf(kHouse - mine, 0.f, kHouse) * 2.f;
    if (t.taker == side) {
        v += 25.f + float(t.gold) * 80.f + float(t.cream) * 4.f;
        if (side == 0 && t.gold <= 0) v -= 30.f;
    } else if (t.taker >= 0) {
        v -= 36.f + float(t.gold) * 50.f + float(t.cream) * 5.f;
    }
    return v;
}

void weightText(float p, char* out, int n) {
    const int cells = 16;
    int fill = int(std::lround(clampf(p, 0.f, 1.f) * float(cells)));
    if (fill > cells) fill = cells;
    int sweet = int(std::lround(0.62f * float(cells - 1)));
    int i = 0;
    for (; i < cells && i < n - 1; i++) {
        if (i == sweet) out[i] = '+';
        else if (i < fill) out[i] = '#';
        else out[i] = '-';
    }
    out[i] = 0;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.A.clear();
    sys.vdp.B.clear();
    sys.vdp.HUD.clear();
    camTop_ = camFor(kTee, 0.46f);
    camSet_ = true;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::begin() {
    red_ = yel_ = 0;
    taker_ = -1;
    takerGold_ = takerCream_ = 0;
    redLie_ = yelLie_ = 99.f;
    shot_ = 0;
    over_ = false;
    won_ = false;
    sheet_ = Sheet{};
    aim_ = 0;
    curl_ = 1;
    power_ = 0.62f;
    planSweep_ = 0;
    osc_ = 0.2f;
    manual_ = false;
    fanStep_ = -1;
    ghostN_ = 0;
    ghostIn_ = false;
    for (auto& p : puffs_) p.life = 0;
    enterAim();
}

void Game::enterAim() {
    mode_ = Mode::Aim;
    picked_ = false;
    think_ = 0;
    thrown_ = -1;
    aiThrow_ = false;
    slideFrames_ = 0;
    ghostN_ = 0;
    ghostIn_ = false;
    if (!sys_) return;
    if (throwSide() == 0) sys_->setLight(180, 40, 36);
    else sys_->setLight(210, 160, 40);
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
            sys_->apu.tone(2, 0, 0);
        }
    }
    if (fanStep_ < 0 || mode_ != Mode::Result || !won_) return;
    fanT_ -= kFrameDt;
    if (fanT_ > 0.f) return;
    const float notes[] = {523.f, 659.f, 784.f, 1046.f};
    if (fanStep_ < 4) {
        blip(notes[fanStep_], 0.1f, 0.16f);
        sys_->apu.tone(1, notes[fanStep_] * 0.5f, 0.04f);
        fanStep_++;
        fanT_ = 0.16f;
    }
}

Shot Game::choose(int side) const {
    Shot best;
    best.aim = 0;
    best.power = 0.64f;
    best.curl = 1;
    best.sweep = 0;
    float bestV = -1e9f;
    auto consider = [&](Shot s) {
        s.power = clampf(s.power, 0.04f, 1.f);
        s.aim = clampf(s.aim, -5.5f, 5.5f);
        s.curl = s.curl >= 0 ? 1 : -1;
        s.sweep = clampf(s.sweep, 0.f, 1.f);
        float v = evalFor(play(sheet_, s, side), side);
        bool better = v > bestV + 0.05f;
        bool tie = std::fabs(v - bestV) <= 0.05f && s.power < best.power;
        if (better || tie) {
            bestV = v;
            best = s;
        }
    };

    if (side == 0) {
        const float powers[] = {0.42f, 0.50f, 0.56f, 0.60f, 0.63f, 0.66f, 0.70f, 0.75f, 0.82f, 0.90f, 0.97f, 1.f};
        for (int ia = 0; ia <= 20; ia++) {
            float aim = -5.f + float(ia) * 0.5f;
            for (float p : powers) {
                for (int c = -1; c <= 1; c += 2) {
                    for (int sw = 0; sw < 2; sw++) {
                        Shot s;
                        s.aim = aim;
                        s.power = p;
                        s.curl = c;
                        s.sweep = sw ? 0.55f : 0.f;
                        consider(s);
                    }
                }
            }
        }
    } else {
        const float powers[] = {0.52f, 0.62f, 0.72f, 0.86f, 1.f};
        for (int ia = 0; ia <= 8; ia++) {
            float aim = -4.8f + float(ia) * 1.2f;
            for (float p : powers) {
                for (int c = -1; c <= 1; c += 2) {
                    Shot s;
                    s.aim = aim;
                    s.power = p;
                    s.curl = c;
                    s.sweep = 0;
                    consider(s);
                }
            }
        }
    }

    for (int i = 0; i < sheet_.n; i++) {
        const Rock& o = sheet_.r[i];
        if (o.dead || o.moving || o.side == side) continue;
        for (int c = -1; c <= 1; c += 2) {
            for (float lead : {0.4f, 1.1f, 1.8f, 2.6f}) {
                for (float p : {0.8f, 0.92f, 1.f}) {
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

    if (side == 0) {
        Shot base = best;
        for (int ia = -7; ia <= 7; ia++) {
            for (int ip = -5; ip <= 5; ip++) {
                Shot s = base;
                s.aim = base.aim + float(ia) * 0.07f;
                s.power = base.power + float(ip) * 0.01f;
                consider(s);
                s.sweep = base.sweep > 0.25f ? 0.f : 0.55f;
                consider(s);
            }
        }
        Tally got = measure(play(sheet_, best, 0));
        if (!(got.taker == 0 && got.gold > 0)) {
            for (int ia = 0; ia <= 32; ia++) {
                float aim = -3.2f + float(ia) * 0.2f;
                for (int ip = 0; ip <= 16; ip++) {
                    float p = 0.50f + float(ip) * 0.02f;
                    for (int c = -1; c <= 1; c += 2) {
                        Shot s;
                        s.aim = aim;
                        s.power = p;
                        s.curl = c;
                        s.sweep = 0;
                        consider(s);
                        s.sweep = 0.6f;
                        consider(s);
                    }
                }
            }
        }
    } else {
        float dir = best.curl >= 0 ? 1.f : -1.f;
        best.aim = clampf(best.aim + 1.25f * dir, -5.5f, 5.5f);
        best.power = clampf(best.power + 0.02f, 0.05f, 1.f);
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
    Trace tr;
    simulate(sh, ix, clampf(sweep, 0.f, 1.f), &tr);
    ghostN_ = tr.n;
    for (int i = 0; i < tr.n; i++) {
        ghostX_[i] = tr.x[i];
        ghostY_[i] = tr.y[i];
    }
    ghostStopY_ = tr.stopY;
    ghostIn_ = tr.in;
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
    sys_->apu.noiseBurst(0.12f, 320.f, 0.08f);
    sys_->rumble(0.2f, 0.08f, 50);
}

void Game::finishEnd() {
    Tally t = measure(sheet_);
    taker_ = t.taker;
    takerGold_ = t.gold;
    takerCream_ = t.cream;
    redLie_ = t.redD;
    yelLie_ = t.yelD;
    red_ = taker_ == 0 ? t.pts : 0;
    yel_ = taker_ == 1 ? t.pts : 0;
    won_ = shot_ == kRocks && taker_ == 0 && takerGold_ >= 1 && red_ > yel_;
    over_ = true;
    mode_ = Mode::Result;
    snap_ = true;
    fanStep_ = won_ ? 0 : -1;
    fanT_ = 0.02f;
    if (!sys_) return;
    if (won_) {
        sys_->apu.tone(0, 523.f, 0.11f);
        sys_->apu.tone(1, 659.f, 0.08f);
        sys_->apu.tone(2, 784.f, 0.07f);
        toneT_ = 0.45f;
        sys_->rumble(0.35f, 0.7f, 160);
        sys_->setLight(255, 196, 48);
    } else {
        blip(110.f, 0.08f, 0.28f);
        sys_->setLight(150, 36, 32);
    }
}

void Game::onRest() {
    bool inGold = false;
    if (thrown_ >= 0 && thrown_ < sheet_.n && !sheet_.r[thrown_].dead) {
        if (buttonDist(sheet_.r[thrown_].x, sheet_.r[thrown_].y) <= kGold) inGold = true;
    }
    forceRest(sheet_);
    compact(sheet_);
    thrown_ = -1;
    aiThrow_ = false;
    shot_++;
    if (inGold) blip(784.f, 0.08f, 0.12f);
    if (shot_ >= kRocks) finishEnd();
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
            think_ = bot_ ? 2 : 22;
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
    aim_ = clampf(aim_ + clampf(dir, -1.f, 1.f) * 3.4f * kFrameDt, -5.4f, 5.4f);
    if (sys_->pad.pressed(gs::BTN_B)) curl_ = -curl_;
    if (sys_->pad.pressed(gs::BTN_X)) manual_ = !manual_;
    if (sys_->pad.down(gs::BTN_UP)) {
        manual_ = true;
        power_ = clampf(power_ + 0.55f * kFrameDt, 0.08f, 1.f);
    }
    if (sys_->pad.down(gs::BTN_DOWN)) {
        manual_ = true;
        power_ = clampf(power_ - 0.55f * kFrameDt, 0.08f, 1.f);
    }
    if (sys_->pad.pressed(gs::BTN_A) || sys_->pad.pressed(gs::BTN_Z)) {
        launch(side, false);
        return;
    }
    if (!manual_) {
        osc_ += kFrameDt;
        float s = 0.5f + 0.5f * std::sin(osc_ * 2.15f);
        power_ = 0.30f + 0.68f * s;
    }
    bool preview = sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_TURBO);
    predict(side, preview ? 1.f : 0.f);
}

void Game::updateSlide() {
    if (thrown_ < 0 || thrown_ >= sheet_.n) {
        onRest();
        return;
    }
    float sw = 0.f;
    if (aiThrow_) sw = planSweep_;
    else if (sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_TURBO)) sw = 1.f;
    bool hit = false;
    for (int i = 0; i < kSubs; i++) {
        if (!anyMoving(sheet_)) break;
        stepSheet(sheet_, thrown_, sw, kDt, &hit);
    }
    if (hit && hitCool_ == 0 && sys_) {
        sys_->apu.noiseBurst(0.18f, 640.f, 0.06f);
        sys_->rumble(0.4f, 0.22f, 40);
        hitCool_ = 8;
    }
    if (sw > 0.4f && thrown_ >= 0 && thrown_ < sheet_.n && sheet_.r[thrown_].moving && (slideFrames_ % 4) == 0) {
        const Rock& r = sheet_.r[thrown_];
        for (auto& p : puffs_) {
            if (p.life > 0.f) continue;
            p.x = r.x;
            p.y = r.y - 0.55f;
            p.life = 0.28f;
            break;
        }
    }
    slideFrames_++;
    if (!anyMoving(sheet_) || slideFrames_ > kMaxSlide) onRest();
}

float Game::focusY() const {
    if (mode_ == Mode::Slide && thrown_ >= 0 && thrown_ < sheet_.n && sheet_.r[thrown_].moving &&
        !sheet_.r[thrown_].dead)
        return sheet_.r[thrown_].y;
    if (mode_ == Mode::Aim && ghostIn_) return clampf(ghostStopY_, kHog, kBack);
    if (mode_ == Mode::Title || mode_ == Mode::Result) return kTee;
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
    return found ? by : kTee;
}

void Game::seekCam(float focus, float bias, bool snap) {
    float want = camFor(focus, bias);
    if (!camSet_ || snap) {
        camTop_ = want;
        camSet_ = true;
        return;
    }
    camTop_ += (want - camTop_) * 0.2f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = held_;
        draw();
        return;
    }
    clock_ += kFrameDt;
    if (hitCool_ > 0) hitCool_--;
    for (auto& p : puffs_)
        if (p.life > 0.f) p.life -= kFrameDt;
    tickAudio();

    if (!bot_ && mode_ != Mode::Title && mode_ != Mode::Result && sys.pad.pressed(gs::BTN_START)) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
        else if (sys.pad.pressed(gs::BTN_MODE)) sys.eject();
    } else if (mode_ == Mode::Aim) {
        updateAim();
    } else if (mode_ == Mode::Slide) {
        updateSlide();
    } else if (mode_ == Mode::Result) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            fanStep_ = -1;
        }
    }

    float bias = mode_ == Mode::Slide ? 0.62f : 0.46f;
    seekCam(focusY(), bias, snap_);
    snap_ = false;
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, int w, int h, int pal, bool flip, bool shadow, int fog) {
    if (!sys_ || w < 1 || h < 1 || img.w == 0 || img.h == 0) return;
    float x0 = cx - float(w) * 0.5f;
    float y0 = cy - float(h) * 0.5f;
    if (x0 >= gs::SCREEN_W || y0 >= gs::SCREEN_H || x0 + float(w) <= 0.f || y0 + float(h) <= 0.f) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.x = int16_t(std::lround(x0));
    s.y = int16_t(std::lround(y0));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(clampf(float(fog), 0.f, 16.f));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow, int fog) {
    if (h < 2.f || m.h < 1) return;
    int ih = std::max(1, int(std::lround(h)));
    int iw = std::max(1, int(std::lround(float(m.w) * (h / float(m.h)))));
    spr(m.pick(h), cx, cy, iw, ih, pal, flip, shadow, fog);
}

void Game::drawRock(float x, float y, int side, int curl, bool shadow) {
    float sx = screenX(x);
    float sy = screenY(y, camTop_);
    int d = std::max(12, int(std::lround(kStoneR * 2.f * kPx)));
    int pal = side == 0 ? PAL_RED : PAL_YEL;
    bool flip = curl < 0;
    sprM(art_.stone, sx, sy, float(d), pal, flip, false, 0);
    if (shadow) sprM(art_.stone, sx + 2.f, sy + 3.f, float(d), pal, flip, true, 0);
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
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int band = y < 40 ? 2 : (y > 180 ? 3 : 2);
        v.lineBackdrop[y] = gs::rgb4(0, 0, band);
    }

    if (mode_ == Mode::Title) {
        float rx = screenX(0.05f);
        float ry = screenY(kTee + 0.05f, camTop_);
        sprM(art_.badge2, rx - 18.f, ry - 14.f, 18.f, PAL_GOLD, false, false, 0);
        float yx = screenX(-2.55f);
        float yy = screenY(kTee - 1.35f, camTop_);
        sprM(art_.badge1, yx - 18.f, yy - 14.f, 18.f, PAL_CREAM, false, false, 0);
        drawRock(0.05f, kTee + 0.05f, 0, 1, true);
        drawRock(-2.55f, kTee - 1.35f, 1, -1, true);
    } else if (mode_ == Mode::Result && won_) {
        sprM(art_.badge2, 26.f, 112.f, 40.f, PAL_GOLD, false, false, 0);
    }

    int order[kRocks];
    int nOrder = 0;
    for (int i = 0; i < sheet_.n; i++)
        if (!sheet_.r[i].dead) order[nOrder++] = i;
    for (int a = 1; a < nOrder; a++) {
        int key = order[a];
        int b = a;
        while (b > 0 && sheet_.r[order[b - 1]].y > sheet_.r[key].y) {
            order[b] = order[b - 1];
            b--;
        }
        order[b] = key;
    }
    for (int i = 0; i < nOrder; i++) {
        const Rock& r = sheet_.r[order[i]];
        drawRock(r.x, r.y, r.side, r.curl, true);
    }

    for (const auto& p : puffs_) {
        if (p.life <= 0.f) continue;
        int fog = int((1.f - p.life / 0.28f) * 12.f);
        sprM(art_.puff, screenX(p.x), screenY(p.y, camTop_), 12.f, PAL_PUFF, false, false, fog);
    }

    bool showBroom = false;
    float bx = 0, by = 0;
    if (mode_ == Mode::Aim && ghostN_ > 0) {
        showBroom = true;
        bx = screenX(ghostX_[ghostN_ - 1]);
        by = screenY(ghostY_[ghostN_ - 1], camTop_) - 8.f;
    } else if (mode_ == Mode::Slide && thrown_ >= 0 && thrown_ < sheet_.n && sheet_.r[thrown_].moving) {
        bool sweeping = aiThrow_ ? planSweep_ > 0.2f
                                 : (sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_TURBO));
        if (sweeping) {
            showBroom = true;
            const Rock& r = sheet_.r[thrown_];
            bx = screenX(r.x) + 8.f;
            by = screenY(r.y, camTop_);
        }
    }
    if (showBroom) sprM(art_.broom, bx, by, 24.f, PAL_BROOM, false, false, 0);

    {
        float bob = std::sin(clock_ * 2.1f) * 1.1f;
        float sx = screenX(6.35f);
        float sy = screenY(kTee - 3.1f, camTop_) + bob;
        sprM(art_.skip, sx, sy, 30.f, PAL_SKIP, false, false, 0);
    }

    spr(art_.dot, screenX(0.f), screenY(kTee, camTop_), 5, 5, PAL_DOT, false, false, 0);

    if (mode_ == Mode::Aim) {
        for (int i = 0; i < ghostN_; i++)
            spr(art_.dot, screenX(ghostX_[i]), screenY(ghostY_[i], camTop_), 4, 4, PAL_DOT, false, false, 0);
    }

    gs::Sprite rink;
    rink.img = art_.rink;
    rink.x = int16_t(iceX());
    rink.y = int16_t(std::lround(screenY(kImgTop, camTop_)));
    rink.w = int16_t(art_.rink.w);
    rink.h = int16_t(art_.rink.h);
    rink.pal = PAL_ICE;
    v.sprite(rink);

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(0, "S3 CURL GOLD", PAL_GOLD);
        hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_HUD);
        hudC(26, "CREAM COUNTS ONE   HAMMER LAST", PAL_CREAM);
        hudC(27, "ARROWS AIM  X HANDLE  Z THROW", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        hudC(1, "PAUSED", PAL_WIN);
        hudC(27, "START", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Result) {
        if (won_) {
            hudC(0, "DOUBLE", PAL_WIN);
            hudC(1, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
            std::snprintf(line, sizeof line, "RED %d   YEL %d", red_, yel_);
            hudC(26, line, PAL_HUD);
            std::snprintf(line, sizeof line, "GOLD %d X2   CREAM %d X1", gold(), cream());
            hudC(27, line, PAL_GOLD);
        } else {
            hudC(0, "SHORT", PAL_ALERT);
            if (taker_ == 0) hudC(1, "CREAM DOES NOT BUY THE DOUBLE", PAL_CREAM);
            else if (taker_ == 1) hudC(1, "YELLOW IS CLOSER", PAL_GOLD);
            else hudC(1, "THE HOUSE WAS BLANK", PAL_DIM);
            std::snprintf(line, sizeof line, "RED %d   YEL %d", red_, yel_);
            hudC(27, line, PAL_HUD);
        }
        return;
    }

    hud(0, 0, "GOLD X2", PAL_GOLD);
    hud(9, 0, "CREAM X1", PAL_CREAM);
    int rk = shot_ < kRocks ? shot_ + 1 : kRocks;
    std::snprintf(line, sizeof line, "ROCK %d/%d", rk, kRocks);
    hud(30, 0, line, PAL_DIM);

    Tally live = measure(sheet_);
    const char* call = "OPEN HOUSE";
    int callPal = PAL_DIM;
    if (live.taker == 1) {
        std::snprintf(line, sizeof line, "YELLOW %d", live.pts);
        call = line;
        callPal = PAL_GOLD;
    } else if (live.taker == 0 && live.gold > 0) {
        std::snprintf(line, sizeof line, "DOUBLE %d", live.pts);
        call = line;
        callPal = PAL_WIN;
    } else if (live.taker == 0) {
        std::snprintf(line, sizeof line, "CREAM %d", live.pts);
        call = line;
        callPal = PAL_CREAM;
    }
    const char* who = throwSide() == 0 ? "YOUR STONE" : "YELLOW";
    hud(0, 1, who, throwSide() == 0 ? PAL_ALERT : PAL_GOLD);
    hud(22, 1, call, callPal);

    if (mode_ == Mode::Slide) {
        bool sweeping = aiThrow_ ? planSweep_ > 0.2f
                                 : (sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_TURBO));
        hudC(26, curl_ > 0 ? "HANDLE RIGHT" : "HANDLE LEFT", PAL_HUD);
        hudC(27, sweeping ? "SWEEPING" : "HOLD C TO SWEEP", PAL_GOLD);
        return;
    }

    hudC(26, curl_ > 0 ? "HANDLE RIGHT   X TURNS" : "HANDLE LEFT   X TURNS", PAL_HUD);
    char bar[20];
    weightText(power_, bar, int(sizeof bar));
    hud(0, 27, manual_ ? "HAND" : "AUTO", PAL_DIM);
    hud(5, 27, bar, PAL_GOLD);
    hud(24, 27, "Z THROW", PAL_HUD);
}

}  // namespace curlgold
