#include "game/seven.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace curlseven {
namespace {

constexpr float kDt = 1.f / 180.f;
constexpr float kFrameDt = 1.f / 60.f;
constexpr int kSubs = 3;
constexpr float kDrag = 18.f;
constexpr float kCurl = 1.75f;
constexpr float kStop = 0.3f;
constexpr float kBounce = 0.8f;
constexpr float kClear = kStoneR * 2.f + 0.22f;

struct Aim {
    float aim = 0;
    float weight = 0.58f;
    int handle = 1;
    float err = 99.f;
};

struct Trail {
    float x[16]{};
    float y[16]{};
    int n = 0;
};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

float speedFor(float w) {
    w = clampf(w, 0.f, 1.f);
    float d0 = (kHog - kRelease) - 1.35f;
    float d1 = (kBack - kRelease) + 2.4f;
    float d = d0 + (d1 - d0) * w;
    if (d < 0.4f) d = 0.4f;
    return std::sqrt(2.f * kDrag * d);
}

bool bitesXY(float x, float y) { return buttonDist(x, y) <= kBite + 0.02f; }

bool legal(float x, float y) {
    if (std::fabs(x) > kSide - kStoneR - 0.08f) return false;
    if (y > kBack - kStoneR - 0.08f) return false;
    if (y < kHog + 0.2f) return false;
    return true;
}

bool clearOf(const Rock* rs, int n, float x, float y) {
    float need = kClear * kClear;
    for (int i = 0; i < n; i++) {
        if (rs[i].gone) continue;
        float dx = rs[i].x - x;
        float dy = rs[i].y - y;
        if (dx * dx + dy * dy < need) return false;
    }
    return true;
}

bool inPlay(const Rock& r) {
    if (r.gone || r.moving) return false;
    if (r.y < kHog) return false;
    if (r.y > kBack - kStoneR) return false;
    if (std::fabs(r.x) > kSide) return false;
    return true;
}

bool bitesRock(const Rock& r) { return inPlay(r) && bitesXY(r.x, r.y); }

void markOut(Rock& r) {
    if (r.gone) return;
    if (std::fabs(r.x) > kSide || r.y > kBack - kStoneR) {
        r.gone = true;
        r.moving = false;
        r.vx = r.vy = 0;
        return;
    }
    if (!r.moving && r.y < kHog) {
        r.gone = true;
        r.vx = r.vy = 0;
    }
}

void launchRock(Rock& r, float aim, float weight, int handle, int side) {
    float sp = speedFor(weight);
    r.x = 0;
    r.y = kRelease;
    r.vx = std::sin(aim) * sp;
    r.vy = std::cos(aim) * sp;
    r.side = side;
    r.handle = handle >= 0 ? 1 : -1;
    r.gone = false;
    r.moving = true;
}

void integrate(Rock& r, float sweep, float dt) {
    if (!r.moving || r.gone) return;
    float sp = std::hypot(r.vx, r.vy);
    if (sp < kStop) {
        r.vx = r.vy = 0;
        r.moving = false;
        markOut(r);
        return;
    }
    float drag = kDrag * (1.f - 0.32f * sweep);
    if (drag < 6.f) drag = 6.f;
    float slow = 1.f - std::min(sp / 16.f, 1.f);
    float curl = float(r.handle) * kCurl * (0.45f + 0.55f * slow) * (1.f - 0.55f * sweep);
    float inv = 1.f / sp;
    float ax = -drag * r.vx * inv + curl * (-r.vy * inv);
    float ay = -drag * r.vy * inv + curl * (r.vx * inv);
    r.vx += ax * dt;
    r.vy += ay * dt;
    r.x += r.vx * dt;
    r.y += r.vy * dt;
    markOut(r);
}

void collide(Rock& a, Rock& b, bool* hit) {
    if (a.gone || b.gone) return;
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float d2 = dx * dx + dy * dy;
    float minD = kStoneR * 2.f;
    if (d2 >= minD * minD || d2 < 1e-8f) return;
    float d = std::sqrt(d2);
    float nx = dx / d;
    float ny = dy / d;
    float pen = minD - d;
    float wa = (a.moving || !b.moving) ? 0.5f : 0.15f;
    float wb = (b.moving || !a.moving) ? 0.5f : 0.15f;
    float s = wa + wb;
    a.x -= nx * pen * (wa / s);
    a.y -= ny * pen * (wa / s);
    b.x += nx * pen * (wb / s);
    b.y += ny * pen * (wb / s);
    float rv = (a.vx - b.vx) * nx + (a.vy - b.vy) * ny;
    if (rv < 0.f) {
        float j = (1.f + kBounce) * 0.5f * rv;
        a.vx -= j * nx;
        a.vy -= j * ny;
        b.vx += j * nx;
        b.vy += j * ny;
        if (hit && std::fabs(j) > 0.35f) *hit = true;
    }
    if (std::hypot(a.vx, a.vy) > kStop) a.moving = true;
    else {
        a.vx = a.vy = 0;
        a.moving = false;
    }
    if (std::hypot(b.vx, b.vy) > kStop) b.moving = true;
    else {
        b.vx = b.vy = 0;
        b.moving = false;
    }
    markOut(a);
    markOut(b);
}

void stepAll(Rock* rs, int n, int thrown, float sweep, float dt, bool* hit) {
    for (int i = 0; i < n; i++)
        if (rs[i].moving) integrate(rs[i], i == thrown ? sweep : 0.f, dt);
    for (int iter = 0; iter < 3; iter++)
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++) collide(rs[i], rs[j], hit);
}

bool anyMoving(const Rock* rs, int n) {
    for (int i = 0; i < n; i++)
        if (rs[i].moving && !rs[i].gone) return true;
    return false;
}

void restAll(Rock* rs, int n, int thrown, float sweep, Trail* tr, bool* hit) {
    for (int s = 0; s < 420; s++) {
        if (!anyMoving(rs, n)) break;
        stepAll(rs, n, thrown, sweep, kDt, hit);
        if (tr && thrown >= 0 && (s % 22) == 0 && tr->n < 16) {
            tr->x[tr->n] = rs[thrown].x;
            tr->y[tr->n] = rs[thrown].y;
            tr->n++;
        }
    }
    for (int i = 0; i < n; i++) {
        if (rs[i].gone) continue;
        if (rs[i].moving) {
            rs[i].moving = false;
            rs[i].vx = rs[i].vy = 0;
        }
        markOut(rs[i]);
    }
    if (tr && thrown >= 0 && thrown < n && tr->n < 16) {
        tr->x[tr->n] = rs[thrown].x;
        tr->y[tr->n] = rs[thrown].y;
        tr->n++;
    }
}

float bestOf(const Rock* rs, int n, int side) {
    float b = 99.f;
    for (int i = 0; i < n; i++) {
        if (!bitesRock(rs[i]) || rs[i].side != side) continue;
        float d = buttonDist(rs[i].x, rs[i].y);
        if (d < b) b = d;
    }
    return b;
}

void scoreOf(const Rock* rs, int n, int& you, int& them) {
    float yb = bestOf(rs, n, 0);
    float tb = bestOf(rs, n, 1);
    you = them = 0;
    if (yb + 0.05f < tb) {
        for (int i = 0; i < n; i++) {
            if (!bitesRock(rs[i]) || rs[i].side != 0) continue;
            if (buttonDist(rs[i].x, rs[i].y) < tb - 0.02f) you++;
        }
    } else if (tb + 0.05f < yb) {
        for (int i = 0; i < n; i++) {
            if (!bitesRock(rs[i]) || rs[i].side != 1) continue;
            if (buttonDist(rs[i].x, rs[i].y) < yb - 0.02f) them++;
        }
    }
}

int counters(const Rock* rs, int n, int side) {
    float other = bestOf(rs, n, 1 - side);
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (!bitesRock(rs[i]) || rs[i].side != side) continue;
        if (buttonDist(rs[i].x, rs[i].y) < other - 0.02f) c++;
    }
    return c;
}

int pockets(const Rock* base, int n, int kind, int salt, float* xs, float* ys, int cap) {
    int w = 0;
    auto add = [&](float x, float y, bool house) {
        if (w >= cap) return;
        if (!legal(x, y) || !clearOf(base, n, x, y)) return;
        if (house && !bitesXY(x, y)) return;
        if (!house && bitesXY(x, y)) return;
        xs[w] = x;
        ys[w] = y;
        w++;
    };
    if (kind == 2) {
        float s = (salt & 1) ? -1.f : 1.f;
        add(s * 2.25f, kTee - kHouse - kStoneR - 0.95f, false);
        add(-s * 2.25f, kTee - kHouse - kStoneR - 1.25f, false);
        add(0.f, kTee - kHouse - kStoneR - 1.5f, false);
        return w;
    }
    if (kind == 1) {
        const float rads[] = {3.9f, 3.5f, 4.2f};
        const float angs[] = {1.15f, -1.15f, 0.85f, -0.85f, 1.4f, -1.4f};
        int shift = salt % 6;
        for (float rad : rads) {
            for (int i = 0; i < 6 && w < cap; i++) {
                float a = angs[(i + shift) % 6];
                float x = std::sin(a) * rad;
                float y = kTee - std::cos(a) * rad;
                if (std::fabs(x) < 1.7f) continue;
                add(x, y, true);
            }
            if (w >= 3) break;
        }
        return w;
    }
    add(0.f, kTee, true);
    const float rads[] = {0.9f, 1.4f, 1.9f};
    int shift = salt & 7;
    for (float rad : rads) {
        for (int i = 0; i < 8 && w < cap; i++) {
            float a = float((i + shift) % 8) * 0.78539816f;
            add(std::cos(a) * rad, kTee + std::sin(a) * rad, true);
        }
        if (w >= 3) break;
    }
    return w;
}

bool bumped(const Rock* before, const Rock* after, int n) {
    for (int i = 0; i < n; i++) {
        if (before[i].gone) continue;
        if (after[i].gone) return true;
        float dx = after[i].x - before[i].x;
        float dy = after[i].y - before[i].y;
        if (dx * dx + dy * dy > 0.18f * 0.18f) return true;
    }
    return false;
}

Aim tryAim(const Rock* base, int n, float aim, float weight, int handle, float tx, float ty) {
    Rock tmp[4]{};
    for (int i = 0; i < n; i++) tmp[i] = base[i];
    launchRock(tmp[n], aim, weight, handle, 0);
    restAll(tmp, n + 1, n, 0.f, nullptr, nullptr);
    Aim a;
    a.aim = aim;
    a.weight = weight;
    a.handle = handle;
    if (tmp[n].gone) a.err = 24.f + std::fabs(tmp[n].y - ty) + std::fabs(tmp[n].x - tx);
    else a.err = std::hypot(tmp[n].x - tx, tmp[n].y - ty);
    if (bumped(base, tmp, n)) a.err += 8.f;
    return a;
}

Aim searchAt(const Rock* base, int n, float tx, float ty) {
    Aim best;
    auto keep = [&](const Aim& a) {
        if (a.err < best.err) best = a;
    };
    for (int hi = 0; hi < 2; hi++) {
        int handle = hi == 0 ? 1 : -1;
        for (int wi = 0; wi < 12; wi++) {
            float w = 0.30f + float(wi) * (0.52f / 11.f);
            for (int ai = -10; ai <= 10; ai++) {
                float aim = float(ai) * 0.048f;
                keep(tryAim(base, n, aim, w, handle, tx, ty));
            }
        }
    }
    float a0 = best.aim;
    float w0 = best.weight;
    int h0 = best.handle;
    for (int wi = -5; wi <= 5; wi++) {
        for (int ai = -5; ai <= 5; ai++) {
            float aim = clampf(a0 + float(ai) * 0.012f, -0.7f, 0.7f);
            float w = clampf(w0 + float(wi) * 0.014f, 0.05f, 0.96f);
            keep(tryAim(base, n, aim, w, h0, tx, ty));
        }
    }
    return best;
}

Aim deliverAim(const Rock* base, int n, int kind, int salt) {
    float xs[6]{};
    float ys[6]{};
    int c = pockets(base, n, kind, salt, xs, ys, 6);
    if (c <= 0) {
        xs[0] = 0;
        ys[0] = kTee;
        c = 1;
    }
    Aim best;
    for (int i = 0; i < c; i++) {
        Aim a = searchAt(base, n, xs[i], ys[i]);
        if (a.err < best.err) best = a;
        if (best.err < 0.35f) break;
    }
    if (kind != 0 && best.err > 0.75f) {
        best.aim = (salt & 1) ? -0.66f : 0.66f;
        best.weight = 0.5f;
        best.handle = 1;
        best.err = 0.75f;
    }
    return best;
}

Aim smartAim(const Rock* base, int n) {
    Aim best;
    best.err = -1e9f;
    for (int hi = 0; hi < 2; hi++) {
        int handle = hi == 0 ? 1 : -1;
        for (int wi = 0; wi < 8; wi++) {
            float w = 0.34f + float(wi) * 0.07f;
            for (int ai = -7; ai <= 7; ai++) {
                float aim = float(ai) * 0.07f;
                Rock tmp[4]{};
                for (int i = 0; i < n; i++) tmp[i] = base[i];
                launchRock(tmp[n], aim, w, handle, 1);
                restAll(tmp, n + 1, n, 0.f, nullptr, nullptr);
                int you = 0, them = 0;
                scoreOf(tmp, n + 1, you, them);
                float tb = bestOf(tmp, n + 1, 1);
                float value = float(them) * 50.f - float(you) * 40.f;
                if (tb < 90.f) value += (6.f - tb) * 4.f;
                if (tmp[n].gone) value -= 3.f;
                if (value > best.err) {
                    best.aim = aim;
                    best.weight = w;
                    best.handle = handle;
                    best.err = value;
                }
            }
        }
    }
    return best;
}

bool placedRules() {
    Rock rs[4]{};
    rs[0].x = 0.1f;
    rs[0].y = kTee;
    rs[0].side = 0;
    rs[1].x = -1.15f;
    rs[1].y = kTee + 0.15f;
    rs[1].side = 0;
    rs[2].x = 3.55f;
    rs[2].y = kTee - 0.9f;
    rs[2].side = 1;
    int you = 0, them = 0;
    scoreOf(rs, 3, you, them);
    if (you != 2 || them != 0) return false;
    rs[1].x = 0.f;
    rs[1].y = kTee - kHouse - kStoneR - 0.9f;
    scoreOf(rs, 3, you, them);
    if (you != 1 || them != 0) return false;
    rs[0].gone = true;
    scoreOf(rs, 3, you, them);
    if (you != 0 || them != 1) return false;
    return true;
}

bool scriptEnd() {
    Rock rs[4]{};
    int n = 0;
    const int side[4] = {1, 0, 1, 0};
    const int kind[4] = {1, 0, 1, 0};
    for (int t = 0; t < 4; t++) {
        Aim a = deliverAim(rs, n, kind[t], t);
        launchRock(rs[n], a.aim, a.weight, a.handle, side[t]);
        restAll(rs, n + 1, n, 0.f, nullptr, nullptr);
        n++;
    }
    int you = 0, them = 0;
    scoreOf(rs, n, you, them);
    if (you >= 1 && them == 0) return true;
    std::fprintf(stderr, "s3curlseven script %d-%d\n", you, them);
    for (int i = 0; i < n; i++) {
        std::fprintf(stderr, "  rock %d side %d gone %d x %.2f y %.2f d %.2f\n", i, rs[i].side, rs[i].gone ? 1 : 0,
                     rs[i].x, rs[i].y, buttonDist(rs[i].x, rs[i].y));
    }
    return false;
}

bool travelRules() {
    Rock light{};
    launchRock(light, 0.f, 0.f, 1, 0);
    restAll(&light, 1, 0, 0.f, nullptr, nullptr);
    if (!light.gone) return false;
    Rock heavy{};
    launchRock(heavy, 0.f, 1.f, 1, 0);
    restAll(&heavy, 1, 0, 0.f, nullptr, nullptr);
    if (!heavy.gone) return false;
    return true;
}

}  // namespace

bool Game::yourTurn() const {
    bool second = (n_ & 1) == 1;
    return youHammer_ ? second : !second;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.A.scroll(0, 0);
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 11));
    sys.apu.setMaster(0.7f);
    rules_ = placedRules() && travelRules() && scriptEnd();
    if (!rules_) std::fprintf(stderr, "s3curlseven rules failed\n");
    mode_ = Mode::Title;
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    ends_ = 0;
    end_ = 1;
    n_ = 0;
    thrown_ = -1;
    took_ = 0;
    youHammer_ = true;
    over_ = false;
    won_ = false;
    picked_ = false;
    aiThrow_ = false;
    aim_ = humanAim_;
    weight_ = humanWeight_;
    handle_ = humanHandle_;
    for (auto& p : puff_) p.life = 0;
    mode_ = Mode::Aim;
}

void Game::toTitle() {
    n_ = 0;
    thrown_ = -1;
    you_ = 0;
    them_ = 0;
    ends_ = 0;
    end_ = 1;
    over_ = false;
    won_ = false;
    picked_ = false;
    mode_ = Mode::Title;
}

void Game::planBot(int side) {
    int kind = 1;
    if (side == 0) {
        int have = counters(rock_, n_, 0);
        kind = (you_ + have >= kRace) ? 2 : 0;
    }
    Aim a = deliverAim(rock_, n_, kind, n_ + end_ * 3);
    aim_ = a.aim;
    weight_ = a.weight;
    handle_ = a.handle;
}

void Game::planSmart() {
    Aim a = smartAim(rock_, n_);
    float jig = ((n_ + end_) & 1) ? 0.02f : -0.018f;
    aim_ = clampf(a.aim + jig, -0.7f, 0.7f);
    weight_ = clampf(a.weight + 0.02f, 0.08f, 0.94f);
    handle_ = a.handle;
}

void Game::predict() {
    if (n_ >= 4) return;
    Rock tmp[4]{};
    for (int i = 0; i < n_; i++) tmp[i] = rock_[i];
    int side = yourTurn() ? 0 : 1;
    launchRock(tmp[n_], aim_, weight_, handle_, side);
    Trail tr;
    restAll(tmp, n_ + 1, n_, 0.f, &tr, nullptr);
    ghostX_ = tmp[n_].x;
    ghostY_ = tmp[n_].y;
    ghostGone_ = tmp[n_].gone;
    pathN_ = tr.n;
    for (int i = 0; i < tr.n; i++) {
        pathX_[i] = tr.x[i];
        pathY_[i] = tr.y[i];
    }
}

void Game::launch() {
    if (n_ >= 4 || !rules_) return;
    int side = yourTurn() ? 0 : 1;
    aiThrow_ = bot_ || side == 1;
    launchRock(rock_[n_], aim_, weight_, handle_, side);
    thrown_ = n_;
    n_++;
    slideFrames_ = 0;
    pathN_ = 0;
    mode_ = Mode::Slide;
    if (!sys_) return;
    sys_->apu.noiseBurst(0.12f, 240.f, 0.08f);
    sys_->rumble(0.2f, 0.06f, 50);
}

void Game::updateAim() {
    if (!rules_) return;
    bool yours = yourTurn();
    bool ai = bot_ || !yours;
    if (ai) {
        if (!picked_) {
            if (bot_ || yours) planBot(yours ? 0 : 1);
            else planSmart();
            picked_ = true;
            think_ = bot_ ? 4 : 26;
        } else if (--think_ <= 0) {
            launch();
            return;
        }
        predict();
        return;
    }
    if (!picked_) {
        aim_ = humanAim_;
        weight_ = humanWeight_;
        handle_ = humanHandle_;
        picked_ = true;
    }
    const gs::Pad& p = sys_->pad;
    float dir = clampf(p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    dir = clampf(dir, -1.f, 1.f);
    float lift = clampf(p.axisY, -1.f, 1.f);
    if (p.down(gs::BTN_UP)) lift += 1.f;
    if (p.down(gs::BTN_DOWN)) lift -= 1.f;
    lift = clampf(lift, -1.f, 1.f);
    aim_ = clampf(aim_ + dir * 1.15f * kFrameDt, -0.62f, 0.62f);
    weight_ = clampf(weight_ + lift * 0.7f * kFrameDt, 0.04f, 0.96f);
    if (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X)) handle_ = -handle_;
    humanAim_ = aim_;
    humanWeight_ = weight_;
    humanHandle_ = handle_;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z)) {
        launch();
        return;
    }
    predict();
}

void Game::updateSlide() {
    if (thrown_ < 0 || thrown_ >= n_) {
        onRest();
        return;
    }
    float sw = 0.f;
    bool ours = rock_[thrown_].side == 0;
    if (!aiThrow_ && ours && sys_ &&
        (sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_TURBO)))
        sw = 1.f;
    bool hit = false;
    for (int i = 0; i < kSubs; i++) stepAll(rock_, n_, thrown_, sw, kDt, &hit);
    if (hit && hitCool_ == 0 && sys_) {
        sys_->apu.noiseBurst(0.18f, 640.f, 0.06f);
        sys_->rumble(0.4f, 0.2f, 40);
        hitCool_ = 8;
    }
    if (sw > 0.5f && rock_[thrown_].moving && (slideFrames_ % 4) == 0) {
        for (auto& p : puff_) {
            if (p.life > 0.f) continue;
            p.x = rock_[thrown_].x;
            p.y = rock_[thrown_].y - 0.55f;
            p.life = 0.28f;
            break;
        }
        if (sys_ && (slideFrames_ % 8) == 0) sys_->apu.noiseBurst(0.04f, 1600.f, 0.03f);
    }
    for (auto& p : puff_)
        if (p.life > 0.f) p.life -= kFrameDt;
    slideFrames_++;
    if (!anyMoving(rock_, n_) || slideFrames_ > 60 * 4) {
        if (anyMoving(rock_, n_)) {
            for (int i = 0; i < n_; i++) {
                rock_[i].moving = false;
                rock_[i].vx = rock_[i].vy = 0;
                markOut(rock_[i]);
            }
        }
        onRest();
    }
}

void Game::onRest() {
    thrown_ = -1;
    aiThrow_ = false;
    if (n_ >= 4) scoreEnd();
    else {
        picked_ = false;
        mode_ = Mode::Aim;
    }
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    blip(523.f, 659.f, 784.f, 0.45f);
    if (sys_) {
        sys_->setLight(40, 180, 70);
        sys_->rumble(0.3f, 0.7f, 160);
    }
}

void Game::lose() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    blip(130.f, 0.f, 0.f, 0.35f);
    if (sys_) sys_->setLight(160, 30, 30);
}

void Game::scoreEnd() {
    int addY = 0, addT = 0;
    scoreOf(rock_, n_, addY, addT);
    you_ += addY;
    them_ += addT;
    took_ = addY > 0 ? addY : (addT > 0 ? -addT : 0);
    ends_++;
    if (addY > 0) youHammer_ = false;
    else if (addT > 0) youHammer_ = true;
    if (addY > 0) blip(523.f, 659.f, 0.f, 0.18f);
    else if (addT > 0) blip(196.f, 0.f, 0.f, 0.2f);
    else blip(330.f, 0.f, 0.f, 0.12f);
    if (you_ >= kRace && you_ > them_) {
        win();
        return;
    }
    if (them_ >= kRace && them_ >= you_) {
        lose();
        return;
    }
    if (ends_ >= kMaxEnds) {
        lose();
        return;
    }
    hold_ = 0;
    mode_ = Mode::Between;
}

void Game::nextEnd() {
    n_ = 0;
    thrown_ = -1;
    picked_ = false;
    end_++;
    for (auto& p : puff_) p.life = 0;
    mode_ = Mode::Aim;
}

void Game::blip(float a, float b, float c, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, a > 0.f ? 0.07f : 0.f);
    sys_->apu.tone(1, b, b > 0.f ? 0.06f : 0.f);
    sys_->apu.tone(2, c, c > 0.f ? 0.05f : 0.f);
    toneT_ = hold;
}

void Game::decay() {
    if (!sys_ || toneT_ <= 0.f) return;
    toneT_ -= kFrameDt;
    if (toneT_ <= 0.f) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

bool Game::audit() { return rules_; }

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - float(s.w) * 0.5f));
    s.y = int16_t(std::lround(cy - float(s.h) * 0.5f));
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

void Game::hudR(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    int col = 40 - n;
    if (col < 29) col = 29;
    hud(col, row, s, pal);
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::lamps(float x, int score, int pal) {
    for (int i = 0; i < kRace; i++) {
        bool on = score > i;
        float s = 8.f;
        if (on && mode_ == Mode::Win && i == kRace - 1) s = 9.f + std::sin(clock_ * 6.f) * 1.4f;
        spr(art_.pip, x + float(i) * 11.f, 16.f, s, s, on ? pal : PAL_DIM);
    }
}

void Game::stoneAt(float wx, float wy, int pal, int handle) {
    sprM(art_.stone, screenX(wx), screenY(wy), 16.f, pal, handle > 0);
}

void Game::shadeAt(float wx, float wy) {
    spr(art_.shadow, screenX(wx), screenY(wy) + 7.f, 14.f, 6.f, PAL_ICE, false, true);
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
        int band = 3 + y / 112;
        v.lineBackdrop[y] = gs::rgb4(1, 1, band);
    }

    const Mode view = mode_ == Mode::Pause ? held_ : mode_;
    const bool poster = view == Mode::Title || view == Mode::Win || view == Mode::Lose;

    if (view == Mode::Title) {
        float bob = std::sin(clock_ * 2.2f) * 2.f;
        spr(art_.curl, 44.f, 52.f, float(art_.curl.w), float(art_.curl.h), PAL_TITLE);
        spr(art_.big, 44.f, 126.f + bob, float(art_.big.w), float(art_.big.h), PAL_GOLD);
        spr(art_.skip, screenX(-2.55f), screenY(kRelease) - 14.f, float(art_.skip.w), float(art_.skip.h), PAL_SKIP);
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 1.4f + float(i) * 0.78539816f;
            spr(art_.dot, screenX(0.f) + std::cos(a) * 20.f, screenY(kTee) + std::sin(a) * 20.f, 3.f, 3.f, PAL_GOLD);
        }
    } else if (view == Mode::Win) {
        spr(art_.seven, 44.f, 52.f, float(art_.seven.w), float(art_.seven.h), PAL_WIN);
        spr(art_.big, 44.f, 126.f, float(art_.big.w), float(art_.big.h), PAL_GOLD);
    } else if (view == Mode::Lose) {
        spr(art_.shorty, 44.f, 70.f, float(art_.shorty.w), float(art_.shorty.h), PAL_ALERT);
    }

    lamps(6.f, you_, PAL_RED);
    lamps(236.f, them_, PAL_YEL);

    for (auto& p : puff_) {
        if (p.life <= 0.f) continue;
        float h = 4.f + p.life * 18.f;
        spr(art_.puff, screenX(p.x), screenY(p.y), h, h * 0.8f, PAL_PUFF);
    }
    if (view == Mode::Slide && thrown_ >= 0 && thrown_ < n_ && rock_[thrown_].moving) {
        const Rock& r = rock_[thrown_];
        bool sweep = false;
        if (!aiThrow_ && r.side == 0 && sys_->pad.down(gs::BTN_C)) sweep = true;
        if (sweep) spr(art_.broom, screenX(r.x + 0.7f), screenY(r.y) - 8.f, float(art_.broom.w), float(art_.broom.h), PAL_BROOM);
    }

    auto drawRock = [&](const Rock& r) {
        if (r.gone) return;
        int pal = r.side == 0 ? PAL_RED : PAL_YEL;
        stoneAt(r.x, r.y, pal, r.handle);
    };
    if (view == Mode::Slide && thrown_ >= 0 && thrown_ < n_) drawRock(rock_[thrown_]);
    for (int i = n_ - 1; i >= 0; i--) {
        if (view == Mode::Slide && i == thrown_) continue;
        drawRock(rock_[i]);
    }
    if (view == Mode::Title) {
        stoneAt(0.12f, kTee, PAL_RED, 1);
        stoneAt(-1.05f, kTee + 0.4f, PAL_RED, -1);
        stoneAt(3.45f, kTee - 0.7f, PAL_YEL, 1);
        stoneAt(0.f, kRelease, PAL_RED, 1);
    }
    if (view == Mode::Aim && !ghostGone_) stoneAt(ghostX_, ghostY_, PAL_GHOST, handle_);
    if (view == Mode::Aim) {
        int pal = PAL_AIM;
        if (!ghostGone_ && bitesXY(ghostX_, ghostY_) && buttonDist(ghostX_, ghostY_) < 1.f) pal = PAL_GREEN;
        for (int i = 0; i < pathN_; i++) spr(art_.dot, screenX(pathX_[i]), screenY(pathY_[i]), 3.f, 3.f, pal);
    }

    if (view == Mode::Slide && thrown_ >= 0 && thrown_ < n_) shadeAt(rock_[thrown_].x, rock_[thrown_].y);
    for (int i = 0; i < n_; i++) {
        if (rock_[i].gone) continue;
        if (view == Mode::Slide && i == thrown_) continue;
        shadeAt(rock_[i].x, rock_[i].y);
    }
    if (view == Mode::Title) {
        shadeAt(0.12f, kTee);
        shadeAt(-1.05f, kTee + 0.4f);
        shadeAt(3.45f, kTee - 0.7f);
        shadeAt(0.f, kRelease);
    }
    if (view == Mode::Aim && !ghostGone_) shadeAt(ghostX_, ghostY_);

    if (view == Mode::Aim || view == Mode::Slide) {
        int dots = 10;
        for (int i = 0; i < dots; i++) spr(art_.dot, 242.f, 168.f - float(i) * 9.f, 3.f, 3.f, PAL_DIM);
        int lit = int(std::lround(clampf(weight_, 0.f, 1.f) * float(dots - 1)));
        spr(art_.dot, 242.f, 168.f - float(lit) * 9.f, 5.f, 5.f, PAL_GOLD);
    }

    char buf[24];
    std::snprintf(buf, sizeof buf, "YOU %d", you_);
    hud(0, 0, buf, PAL_INK);
    std::snprintf(buf, sizeof buf, "THEM %d", them_);
    hudR(0, buf, PAL_INK);

    if (!rules_) {
        hudC(27, "RULES FAILED", PAL_ALERT);
        return;
    }

    if (view == Mode::Title) {
        hudR(4, "LINE", PAL_INK);
        hudR(5, "ARROWS", PAL_GOLD);
        hudR(7, "WEIGHT", PAL_INK);
        hudR(8, "UP DOWN", PAL_GOLD);
        hudR(10, "HANDLE", PAL_INK);
        hudR(11, "X", PAL_GOLD);
        hudR(13, "THROW", PAL_INK);
        hudR(14, "Z", PAL_GOLD);
        hudR(16, "SWEEP", PAL_INK);
        hudR(17, "HOLD C", PAL_GOLD);
        hudR(19, "TWO", PAL_INK);
        hudR(20, "ROCKS", PAL_INK);
        hudR(22, "HOUSE", PAL_GOLD);
        hudR(23, "COUNTS", PAL_INK);
        hudC(27, "FIRST TO SEVEN", PAL_TITLE);
        return;
    }

    if (!poster) {
        hud(0, 4, "CURL", PAL_INK);
        hud(0, 5, handle_ > 0 ? "LEFT" : "RIGHT", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "W %d", int(std::lround(weight_ * 100.f)));
        hud(0, 7, buf, PAL_INK);
        bool yoursNow = yourTurn();
        if (view == Mode::Slide && thrown_ >= 0 && thrown_ < n_) yoursNow = rock_[thrown_].side == 0;
        hud(0, 9, view == Mode::Slide ? "ROCK" : "THROW", PAL_INK);
        hud(0, 10, yoursNow ? "YOU" : "THEM", yoursNow ? PAL_GREEN : PAL_ALERT);
        std::snprintf(buf, sizeof buf, "END %d", end_);
        hud(0, 12, buf, PAL_TITLE);
        std::snprintf(buf, sizeof buf, "ROCK %d", std::min(n_ + (view == Mode::Aim ? 1 : 0), 4));
        hud(0, 13, buf, PAL_INK);

        hudR(4, "FIRST", PAL_TITLE);
        hudR(5, "TO", PAL_TITLE);
        hudR(6, "SEVEN", PAL_TITLE);
        const char* call = "LINE";
        int pal = PAL_INK;
        if (view == Mode::Aim) {
            if (ghostGone_ && ghostY_ < kHog) {
                call = "SHORT";
                pal = PAL_ALERT;
            } else if (ghostGone_ && std::fabs(ghostX_) > kSide) {
                call = "WIDE";
                pal = PAL_ALERT;
            } else if (ghostGone_) {
                call = "HEAVY";
                pal = PAL_ALERT;
            } else if (bitesXY(ghostX_, ghostY_) && buttonDist(ghostX_, ghostY_) < 0.75f) {
                call = "BUTTON";
                pal = PAL_GREEN;
            } else if (bitesXY(ghostX_, ghostY_)) {
                call = "HOUSE";
                pal = PAL_GOLD;
            } else if (ghostY_ >= kHog) {
                call = "GUARD";
                pal = PAL_AIM;
            } else {
                call = "SHORT";
                pal = PAL_ALERT;
            }
        } else if (view == Mode::Slide) {
            call = "SLIDE";
            pal = PAL_GOLD;
        } else if (view == Mode::Between) {
            if (took_ > 0) {
                std::snprintf(buf, sizeof buf, "TAKE %d", took_);
                call = buf;
                pal = PAL_GREEN;
            } else if (took_ < 0) {
                std::snprintf(buf, sizeof buf, "THEM %d", -took_);
                call = buf;
                pal = PAL_ALERT;
            } else {
                call = "BLANK";
                pal = PAL_INK;
            }
        }
        hudR(8, "LIE", PAL_INK);
        hudR(9, call, pal);
    }

    if (mode_ == Mode::Pause) hudC(27, "PAUSED", PAL_GOLD);
    else if (view == Mode::Win) hudC(27, "FIRST TO SEVEN", PAL_WIN);
    else if (view == Mode::Lose) hudC(27, "THEY GOT THERE", PAL_ALERT);
    else if (view == Mode::Aim) hudC(27, yourTurn() && !bot_ ? "Z THROWS" : "WATCH THE LINE", yourTurn() ? PAL_GREEN : PAL_INK);
    else if (view == Mode::Slide) hudC(27, !aiThrow_ ? "HOLD C TO SWEEP" : "SLIDING", PAL_INK);
    else if (view == Mode::Between) {
        if (took_ > 0) std::snprintf(buf, sizeof buf, "YOU TAKE %d", took_);
        else if (took_ < 0) std::snprintf(buf, sizeof buf, "THEM TAKE %d", -took_);
        else std::snprintf(buf, sizeof buf, "BLANK END");
        hudC(27, buf, took_ > 0 ? PAL_GREEN : (took_ < 0 ? PAL_ALERT : PAL_INK));
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kFrameDt;
    if (hitCool_ > 0) hitCool_--;
    decay();
    const gs::Pad& p = sys.pad;

    if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = held_;
        else if (p.pressed(gs::BTN_MODE)) toTitle();
    } else if (mode_ == Mode::Title) {
        if (bot_ && !rules_) {
            over_ = true;
            won_ = false;
        } else if (bot_ && clock_ > 0.15f) begin();
        else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) begin();
        else if (p.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
        else if (!bot_ && p.pressed(gs::BTN_MODE)) toTitle();
    } else if (!bot_ && p.pressed(gs::BTN_START)) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) updateAim();
    else if (mode_ == Mode::Slide) updateSlide();
    else if (mode_ == Mode::Between) {
        bool skip = !bot_ && (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_START));
        if (++hold_ > (bot_ ? 12 : 70) || skip) nextEnd();
    }
    draw();
}

}  // namespace curlseven
