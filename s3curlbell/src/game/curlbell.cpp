#include "game/curlbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace curlbell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSub = 1.f / 120.f;
constexpr int kFrameSubs = 2;
constexpr int kCastCap = 120 * 5;
constexpr float kDrag = 8.8f;
constexpr float kCurl = 1.55f;
constexpr float kStop = 0.16f;
constexpr float kSweepDrag = 0.20f;
constexpr float kSweepCurl = 0.48f;
constexpr float kRestitution = 0.84f;
constexpr float kVxLo = -3.6f;
constexpr float kVxHi = 2.0f;
constexpr float kVyLo = 10.5f;
constexpr float kVyHi = 21.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float speed(const Rock& r) { return std::hypot(r.vx, r.vy); }

bool outWide(float x, float y) { return y > kBack || std::fabs(x) > kSide; }

bool plays(float x, float y, bool dead) {
    if (dead) return false;
    if (y < kHog + kStoneR) return false;
    if (outWide(x, y)) return false;
    return true;
}

bool rings(float x, float y, bool dead) {
    return plays(x, y, dead) && std::hypot(x, y - kTee) <= kBellR;
}

void atRest(Rock& r) {
    if (!r.dead && r.y < kHog + kStoneR) {
        r.dead = true;
        r.vx = r.vy = 0;
    }
}

void shove(Rock& r, float sweep, float dt) {
    float sp = speed(r);
    if (r.dead || sp < kStop) {
        r.vx = r.vy = 0;
        return;
    }
    float drag = kDrag * (1.f - kSweepDrag * sweep);
    if (drag < 1.f) drag = 1.f;
    float curl = kCurl * (1.f - kSweepCurl * sweep) * float(r.handle);
    float inv = 1.f / sp;
    float ax = -drag * r.vx * inv + curl * r.vy * inv;
    float ay = -drag * r.vy * inv - curl * r.vx * inv;
    float nx = r.vx + ax * dt;
    float ny = r.vy + ay * dt;
    if (nx * r.vx + ny * r.vy <= 0.f || std::hypot(nx, ny) < kStop) {
        float t = sp / drag;
        if (t > dt) t = dt;
        r.x += r.vx * t;
        r.y += r.vy * t;
        r.vx = r.vy = 0;
        return;
    }
    r.vx = nx;
    r.vy = ny;
    r.x += r.vx * dt;
    r.y += r.vy * dt;
}

bool stepRocks(Rock* r, int n, int active, float sweep, float dt) {
    for (int i = 0; i < n; i++) {
        if (r[i].dead) continue;
        shove(r[i], i == active ? sweep : 0.f, dt);
    }
    bool hit = false;
    const float minD = kStoneR * 2.f;
    for (int i = 0; i < n; i++) {
        if (r[i].dead) continue;
        for (int j = i + 1; j < n; j++) {
            if (r[j].dead) continue;
            float dx = r[j].x - r[i].x;
            float dy = r[j].y - r[i].y;
            float d = std::hypot(dx, dy);
            if (d >= minD || d < 1e-4f) continue;
            if (speed(r[i]) < kStop && speed(r[j]) < kStop) continue;
            hit = true;
            float nx = dx / d;
            float ny = dy / d;
            float overlap = minD - d;
            r[i].x -= nx * overlap * 0.5f;
            r[i].y -= ny * overlap * 0.5f;
            r[j].x += nx * overlap * 0.5f;
            r[j].y += ny * overlap * 0.5f;
            float rel = (r[i].vx - r[j].vx) * nx + (r[i].vy - r[j].vy) * ny;
            if (rel <= 0.f) continue;
            float jolt = (1.f + kRestitution) * rel * 0.5f;
            r[i].vx -= jolt * nx;
            r[i].vy -= jolt * ny;
            r[j].vx += jolt * nx;
            r[j].vy += jolt * ny;
        }
    }
    for (int i = 0; i < n; i++) {
        if (r[i].dead) continue;
        if (outWide(r[i].x, r[i].y)) {
            r[i].dead = true;
            r[i].vx = r[i].vy = 0;
        }
    }
    return hit;
}

bool anyMoving(const Rock* r, int n) {
    for (int i = 0; i < n; i++) {
        if (r[i].dead) continue;
        if (speed(r[i]) >= kStop) return true;
    }
    return false;
}

const char* whyName(Why w) {
    switch (w) {
    case Why::Hog: return "HOGGED";
    case Why::Burn: return "BURNED";
    case Why::Wide: return "WIDE";
    case Why::Light: return "LIGHT";
    case Why::Heavy: return "HEAVY";
    case Why::Miss: return "MISSED";
    default: return "DIED";
    }
}

}  // namespace

bool Game::onBell() const {
    if (!rung_) return false;
    for (int i = n_ - 1; i >= 0; --i) {
        if (!rock_[i].ours || rock_[i].dead) continue;
        if (rings(rock_[i].x, rock_[i].y, false)) return true;
    }
    return false;
}

void Game::place() {
    for (auto& r : rock_) r = {};
    rock_[0].x = kGuardX;
    rock_[0].y = kGuardY;
    rock_[0].handle = 0;
    n_ = 1;
    thrown_ = 0;
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    onBell_ = false;
    why_ = Why::None;
    sweep_ = 0;
    sweepArm_ = false;
    hitSnd_ = false;
    wasBell_ = false;
    bellAmp_ = 0.22f;
    lieX_ = 0;
    lieY_ = kHack;
    for (auto& p : puff_) p.life = 0;
}

void Game::toTitle() {
    place();
    mode_ = Mode::Title;
    clock_ = 0;
    aimT_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(8, 11, 13));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.18f, 0.26f, 0.18f);
    sys.setLight(150, 190, 210);
    place();
    solve();
    if (!solved_) std::fprintf(stderr, "s3curlbell no draw  best %.3f\n", solDist_);
    toTitle();
}

void Game::blip(float a, float b, float hold) {
    if (!sys_) return;
    sys_->apu.tone(0, a, 0.08f);
    if (b > 0.f) sys_->apu.tone(1, b, 0.06f);
    else sys_->apu.tone(1, 0, 0);
    toneT_ = hold;
}

void Game::decayAudio() {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming) {
        bellTick_ -= kDt;
        if (bellTick_ <= 0.f && bellAmp_ > 0.4f) {
            float vol = 0.05f + 0.09f * bellAmp_;
            sys_->apu.tone(0, 830.f, vol);
            sys_->apu.tone(1, 1245.f, vol * 0.7f);
            toneT_ = 0.16f;
            bellTick_ = 0.22f;
        }
    }
    if (toneT_ > 0.f) {
        toneT_ -= kDt;
        if (toneT_ <= 0.f && !chiming) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
    if (mode_ == Mode::Slide) sys_->apu.noise(sweep_ > 0.5f ? 0.05f : 0.016f, sweep_ > 0.5f ? 2300.f : 700.f, false);
    else sys_->apu.noise(0.f, 1000.f, false);
}

Game::ShotEnd Game::cast(float vx, float vy, int handle, bool record) {
    Rock tmp[kMaxRock];
    int n = n_;
    if (n > kMaxRock - 1) n = kMaxRock - 1;
    for (int i = 0; i < n; i++) tmp[i] = rock_[i];
    int active = n;
    Rock& shot = tmp[n];
    shot = {};
    shot.x = 0.f;
    shot.y = kHack;
    shot.vx = vx;
    shot.vy = vy;
    shot.handle = handle;
    shot.ours = true;
    n++;
    if (record) pathN_ = 0;
    bool hit = false;
    int steps = 0;
    for (; steps < kCastCap; steps++) {
        if (!anyMoving(tmp, n)) break;
        hit = stepRocks(tmp, n, active, 0.f, kSub) || hit;
        if (record && pathN_ < 8 && (steps % 22) == 8) {
            pathX_[pathN_] = tmp[active].x;
            pathY_[pathN_] = tmp[active].y;
            pathN_++;
        }
    }
    atRest(tmp[active]);
    ShotEnd e;
    e.x = tmp[active].x;
    e.y = tmp[active].y;
    e.dead = tmp[active].dead || anyMoving(tmp, n);
    e.hit = hit;
    e.bell = rings(e.x, e.y, e.dead);
    return e;
}

void Game::solve() {
    float bestScore = 1e9f;
    float bestD = 1e9f;
    float bestVx = 0.f;
    float bestVy = 15.f;
    int bestH = 1;
    bool found = false;
    auto take = [&](float vx, float vy, int h) {
        ShotEnd e = cast(vx, vy, h, false);
        if (!plays(e.x, e.y, e.dead)) return;
        float d = std::hypot(e.x, e.y - kTee);
        float score = d + (h == 0 ? 2.f : 0.f) + (e.hit ? 5.f : 0.f);
        if (d <= kBellR && score < bestScore) {
            bestScore = score;
            bestD = d;
            bestVx = vx;
            bestVy = vy;
            bestH = h;
            found = true;
        } else if (!found && d < bestD) {
            bestD = d;
            bestVx = vx;
            bestVy = vy;
            bestH = h;
        }
    };
    for (int h : {1, -1, 0}) {
        for (float vy = 11.f; vy <= 20.f; vy += 0.12f) {
            for (float vx = -3.2f; vx <= 1.6f; vx += 0.10f) take(vx, vy, h);
            if (found && bestD < 0.12f) break;
        }
        if (found && bestD < 0.12f) break;
    }
    if (found || bestD < 2.f) {
        float bx = bestVx;
        float by = bestVy;
        int bh = bestH;
        for (float dvy = -0.16f; dvy <= 0.161f; dvy += 0.02f)
            for (float dvx = -0.16f; dvx <= 0.161f; dvx += 0.02f) take(bx + dvx, by + dvy, bh);
    }
    solVx_ = bestVx;
    solVy_ = bestVy;
    solHandle_ = bestH;
    solDist_ = bestD;
    ShotEnd check = cast(bestVx, bestVy, bestH, true);
    solved_ = check.bell && !check.hit;
    if (!solved_) solved_ = check.bell;
}

void Game::newGame() {
    place();
    beginAim();
}

void Game::beginAim() {
    if (thrown_ > 0 || !solved_) solve();
    handle_ = solHandle_;
    if (bot_) {
        aimVx_ = solVx_;
        aimVy_ = solVy_;
    } else {
        aimVx_ = clampf(solVx_ + 0.75f, kVxLo, kVxHi);
        aimVy_ = clampf(solVy_ - 1.15f, kVyLo, kVyHi);
    }
    sweep_ = 0.f;
    sweepArm_ = false;
    hitSnd_ = false;
    aimT_ = 0.f;
    slideT_ = 0.f;
    wasBell_ = false;
    mode_ = Mode::Aim;
}

void Game::launch() {
    if (n_ >= kMaxRock || thrown_ >= kMaxThrow || mode_ != Mode::Aim) return;
    Rock& r = rock_[n_++];
    r = {};
    r.x = 0.f;
    r.y = kHack;
    r.vx = aimVx_;
    r.vy = aimVy_;
    r.handle = handle_;
    r.ours = true;
    thrown_++;
    sweep_ = 0.f;
    slideT_ = 0.f;
    hitSnd_ = false;
    mode_ = Mode::Slide;
    blip(150.f, 0.f, 0.08f);
    if (sys_) sys_->rumble(0.12f, 0.28f, 70);
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Slide) return;
    rung_ = true;
    won_ = true;
    onBell_ = true;
    tryNo_ = dead_ + 1;
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ringT_ = 0.f;
    mode_ = Mode::Ring;
    if (!sys_) return;
    sys_->rumble(0.4f, 0.85f, 180);
    sys_->setLight(255, 196, 48);
    blip(830.f, 1245.f, 0.28f);
}

void Game::dieTry(Why why) {
    if (rung_ || mode_ != Mode::Slide) return;
    why_ = why;
    dead_++;
    if (!sys_) {
        mode_ = dead_ >= 3 ? Mode::Over : Mode::Dead;
        if (dead_ >= 3) {
            won_ = false;
            over_ = true;
        } else deadT_ = 0.f;
        return;
    }
    sys_->apu.noise(0.f, 1000.f, false);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        blip(90.f, 64.f, 0.45f);
        sys_->setLight(150, 28, 28);
    } else {
        mode_ = Mode::Dead;
        deadT_ = 0.f;
        blip(140.f, 0.f, 0.24f);
        sys_->setLight(120, 48, 28);
    }
}

void Game::settle() {
    int last = n_ - 1;
    if (last < 0 || !rock_[last].ours) {
        dieTry(Why::Miss);
        return;
    }
    Rock& r = rock_[last];
    atRest(r);
    lieX_ = r.x;
    lieY_ = r.y;
    if (rings(r.x, r.y, r.dead)) {
        ring();
        return;
    }
    Why w = Why::Miss;
    if (!plays(r.x, r.y, r.dead)) {
        if (r.y < kHog + kStoneR) w = Why::Hog;
        else if (std::fabs(r.x) > kSide) w = Why::Wide;
        else w = Why::Burn;
    } else if (r.y < kTee - 0.7f) w = Why::Light;
    else if (r.y > kTee + 0.7f) w = Why::Heavy;
    dieTry(w);
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

    const Mode view = mode_ == Mode::Pause ? held_ : mode_;
    const float stoneH = kStoneR * 2.f * kPx * 1.28f;
    const bool showHack = view == Mode::Title || view == Mode::Aim;
    const bool leaving = view == Mode::Leave || (view == Mode::Over && won_);

    auto shadowAt = [&](float wx, float wy) {
        spr(art_.shadow, screenX(wx) + 2.f, screenY(wy) + 3.f, stoneH * 1.15f, stoneH * 0.42f, PAL_INK, false, true);
    };
    auto stoneAt = [&](float wx, float wy, int pal, int handle) {
        sprM(art_.stone, screenX(wx), screenY(wy), stoneH, pal, handle < 0);
    };

    float skipX = 1.3f;
    float skipY = kHack;
    if (view == Mode::Slide || view == Mode::Dead || view == Mode::Ring || leaving) {
        float sx = lieX_;
        float sy = lieY_;
        if (view == Mode::Slide && n_ > 0) {
            sx = rock_[n_ - 1].x;
            sy = rock_[n_ - 1].y;
        }
        skipX = sx + 1.35f;
        skipY = sy - 0.95f;
        if (skipY < kHack) skipY = kHack;
    }
    float walk = leaving ? leaveT_ * 110.f : 0.f;
    float bob = std::sin(clock_ * (view == Mode::Slide ? 14.f : 3.f)) * 1.2f;

    float amp = rung_ ? bellAmp_ : 0.22f;
    float swing = std::sin(bellPh_) * 8.f * amp;
    float bx = screenX(0.f);
    float by = screenY(kTee) - 16.f;

    if (rung_ && bellAmp_ > 0.45f) {
        for (int i = 0; i < 8; i++) {
            float a = bellPh_ * 1.3f + float(i) * 0.785f;
            float rad = 10.f + (1.f - bellAmp_) * 22.f;
            spr(art_.dot, bx + std::cos(a) * rad, by + 8.f + std::sin(a) * rad * 0.55f, 4.f, 4.f, PAL_WIN);
        }
    }
    if (view == Mode::Aim) {
        for (int i = 0; i < pathN_; i++)
            spr(art_.dot, screenX(pathX_[i]), screenY(pathY_[i]), 3.f, 3.f, ghostBell_ ? PAL_GREEN : PAL_AIM);
        float u = clampf((aimVy_ - kVyLo) / (kVyHi - kVyLo), 0.f, 1.f);
        for (int i = 0; i < 12; i++) spr(art_.dot, 308.f, 48.f + float(i) * 10.f, 3.f, 3.f, PAL_DIM);
        spr(art_.dot, 308.f, 48.f + (1.f - u) * 110.f, 5.f, 5.f, ghostBell_ ? PAL_GREEN : PAL_TITLE);
    }
    if (view == Mode::Title) {
        for (int i = 0; i < 8; i++) {
            float a = clock_ * 1.4f + float(i) * 0.785f;
            spr(art_.dot, screenX(0.f) + std::cos(a) * 16.f, screenY(kTee) + std::sin(a) * 16.f, 3.f, 3.f, PAL_TITLE);
        }
    }

    bool showBroom = view == Mode::Slide && sweep_ > 0.5f && n_ > 0;
    float broomX = showBroom ? rock_[n_ - 1].x + 0.55f : skipX + 0.15f;
    float broomY = showBroom ? rock_[n_ - 1].y : skipY + 0.15f;
    spr(art_.broom, screenX(broomX) + walk, screenY(broomY) - 8.f + bob, float(art_.broom.w), float(art_.broom.h),
        PAL_BROOM, handle_ < 0, false);
    for (const auto& pf : puff_) {
        if (pf.life <= 0.f) continue;
        float h = 4.f + pf.life * 14.f;
        spr(art_.puff, screenX(pf.x), screenY(pf.y), h, h * 0.8f, PAL_PUFF);
    }
    spr(art_.skip, screenX(skipX) + walk, screenY(skipY) + bob, float(art_.skip.w) + 8.f, float(art_.skip.h) + 8.f,
        PAL_SKIP, skipX > 0.f, false);

    spr(art_.clapper, bx + swing * 1.6f, by + 6.f, 5.f, 9.f, PAL_BELL);
    spr(art_.bell, bx + swing, by, 22.f, 26.f, PAL_BELL);

    if (view == Mode::Aim && !ghostDead_) stoneAt(ghostX_, ghostY_, PAL_GHOST, handle_);
    if (showHack) stoneAt(0.f, kHack, PAL_RED, handle_);
    for (int i = n_ - 1; i >= 0; --i) {
        if (rock_[i].dead) continue;
        int pal = rock_[i].ours ? PAL_RED : PAL_YEL;
        stoneAt(rock_[i].x, rock_[i].y, pal, rock_[i].handle);
    }
    if (view == Mode::Dead && n_ > 0 && rock_[n_ - 1].dead)
        stoneAt(lieX_, lieY_, PAL_GHOST, rock_[n_ - 1].handle);

    if (showHack) shadowAt(0.f, kHack);
    if (view == Mode::Aim && !ghostDead_) shadowAt(ghostX_, ghostY_);
    for (int i = 0; i < n_; i++) {
        if (!rock_[i].dead) shadowAt(rock_[i].x, rock_[i].y);
    }
    shadowAt(0.f, kTee);

    char buf[16];
    if (view == Mode::Title) {
        hudC(0, "CURLBELL", PAL_TITLE);
        hud(1, 4, "RING", PAL_WIN);
        hud(1, 5, "THE", PAL_INK);
        hud(1, 6, "BELL", PAL_WIN);
        hud(1, 8, "LINE", PAL_INK);
        hud(1, 9, "L R", PAL_DIM);
        hud(1, 11, "WEIGHT", PAL_INK);
        hud(1, 12, "U D", PAL_DIM);
        hud(1, 14, "HANDLE", PAL_INK);
        hud(1, 15, "X", PAL_TITLE);
        hud(1, 17, "THROW", PAL_INK);
        hud(1, 18, "Z", PAL_TITLE);
        hud(1, 20, "SWEEP", PAL_INK);
        hud(1, 21, "HOLD Z", PAL_DIM);
        hud(31, 4, "3 TRIES", PAL_TITLE);
        hud(31, 6, "TRY 3", PAL_ALERT);
        hud(31, 7, "DIES", PAL_ALERT);
        hud(31, 9, "THEN", PAL_INK);
        hud(31, 10, "LEAVE", PAL_GREEN);
        if ((int(clock_ * 2.f) & 1) == 0) hud(1, 26, "START", PAL_WIN);
        return;
    }

    std::snprintf(buf, sizeof buf, "TRY %d", std::min(dead_ + 1, 3));
    hud(1, 2, buf, dead_ == 2 ? PAL_ALERT : PAL_TITLE);
    std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
    hud(1, 3, buf, PAL_INK);
    hud(1, 5, "CURL", PAL_INK);
    hud(1, 6, handle_ > 0 ? "RIGHT" : (handle_ < 0 ? "LEFT" : "NONE"), handle_ == 0 ? PAL_DIM : PAL_TITLE);

    if (view == Mode::Aim) {
        const char* call = "LINE";
        int pal = PAL_INK;
        if (ghostDead_) {
            call = ghostY_ > kTee ? "BURNED" : (std::fabs(ghostX_) > kSide ? "WIDE" : "HOGGED");
            pal = PAL_ALERT;
        } else if (ghostBell_) {
            call = "BELL";
            pal = PAL_GREEN;
        } else if (std::hypot(ghostX_, ghostY_ - kTee) <= kHouse) {
            call = ghostY_ < kTee ? "LIGHT" : "HEAVY";
            pal = PAL_AIM;
        } else if (ghostY_ < kHog + kStoneR) {
            call = "HOGGED";
            pal = PAL_ALERT;
        }
        hud(1, 8, "LIE", PAL_INK);
        hud(1, 9, call, pal);
        hud(31, 4, "Z THROW", PAL_TITLE);
        hud(31, 6, "HOLD Z", PAL_DIM);
        hud(31, 7, "SWEEPS", PAL_INK);
    } else if (view == Mode::Slide) {
        hud(1, 8, "SLIDE", PAL_TITLE);
        hud(1, 9, sweep_ > 0.5f ? "SWEEP" : "LET IT", sweep_ > 0.5f ? PAL_WIN : PAL_DIM);
        hud(31, 4, "HOLD Z", PAL_DIM);
        hud(31, 5, "SWEEP", PAL_INK);
    } else if (view == Mode::Dead) {
        hud(1, 8, "DIED", PAL_ALERT);
        hud(1, 9, whyName(why_), PAL_ALERT);
        hud(31, 4, dead_ == 1 ? "2 LEFT" : "1 LEFT", PAL_TITLE);
    } else if (view == Mode::Ring) {
        hudC(0, "BELL", PAL_WIN);
        hud(1, 8, "BELL", PAL_WIN);
    } else if (view == Mode::Leave) {
        hudC(0, "LEAVE", PAL_GREEN);
        hud(1, 8, "LEAVE", PAL_GREEN);
        hud(1, 9, "IT RANG", PAL_WIN);
    } else if (view == Mode::Over && won_) {
        hudC(0, "LEFT", PAL_WIN);
        hud(1, 8, "BELL", PAL_WIN);
        hud(1, 9, "RANG", PAL_GREEN);
        if (!bot_) hud(31, 24, "START", PAL_TITLE);
    } else if (view == Mode::Over) {
        hudC(0, "DEAD", PAL_ALERT);
        hud(1, 8, "TRY 3", PAL_ALERT);
        hud(1, 9, "DIED", PAL_ALERT);
        hud(31, 4, "SILENT", PAL_INK);
        if (!bot_) hud(31, 24, "START", PAL_TITLE);
    }
    if (mode_ == Mode::Pause) hudC(0, "PAUSED", PAL_TITLE);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 13.f : 2.2f);
    if (rung_) bellAmp_ = std::max(0.35f, bellAmp_ - kDt * 0.22f);
    decayAudio();

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool back = p.pressed(gs::BTN_MODE);
    bool fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
    }

    Mode before = mode_;
    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        if (bot_ && clock_ > 0.45f) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || fire) newGame();
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || fire)) newGame();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && (mode_ == Mode::Aim || mode_ == Mode::Slide) && (start || back)) {
        held_ = mode_;
        mode_ = Mode::Pause;
    }

    if (mode_ == before) {
        if (mode_ == Mode::Aim) {
            aimT_ += kDt;
            if (!bot_) {
                if (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X)) handle_ = handle_ > 0 ? -1 : 1;
                float mx = 0.f, my = 0.f;
                if (p.down(gs::BTN_LEFT)) mx -= 1.f;
                if (p.down(gs::BTN_RIGHT)) mx += 1.f;
                if (p.down(gs::BTN_UP)) my += 1.f;
                if (p.down(gs::BTN_DOWN)) my -= 1.f;
                if (std::fabs(p.axisX) > 0.22f) mx = p.axisX;
                if (std::fabs(p.axisY) > 0.22f) my = p.axisY;
                aimVx_ = clampf(aimVx_ + mx * 1.7f * kDt, kVxLo, kVxHi);
                aimVy_ = clampf(aimVy_ + my * 2.5f * kDt, kVyLo, kVyHi);
            }
            ShotEnd e = cast(aimVx_, aimVy_, handle_, true);
            ghostX_ = e.x;
            ghostY_ = e.y;
            ghostDead_ = e.dead || !plays(e.x, e.y, e.dead);
            ghostBell_ = e.bell;
            if (ghostBell_ && !wasBell_) blip(880.f, 0.f, 0.05f);
            wasBell_ = ghostBell_;
            if (bot_) {
                if (aimT_ > 0.32f && solved_) launch();
            } else if (fire) launch();
        } else if (mode_ == Mode::Slide) {
            slideT_ += kDt;
            bool held = !bot_ && (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO));
            sweep_ = (slideT_ > 0.12f && held) ? 1.f : 0.f;
            int active = n_ - 1;
            bool hit = false;
            for (int s = 0; s < kFrameSubs; s++) {
                if (!anyMoving(rock_, n_)) break;
                hit = stepRocks(rock_, n_, active, sweep_, kSub) || hit;
            }
            if (hit && !hitSnd_) {
                hitSnd_ = true;
                sys.apu.noiseBurst(0.16f, 1700.f, 0.07f);
            }
            if (sweep_ > 0.5f && active >= 0 && !rock_[active].dead) {
                for (auto& pf : puff_) {
                    if (pf.life > 0.f) continue;
                    pf.x = rock_[active].x + (handle_ > 0 ? 0.4f : -0.4f);
                    pf.y = rock_[active].y - 0.1f;
                    pf.life = 0.28f;
                    break;
                }
            }
            for (auto& pf : puff_)
                if (pf.life > 0.f) pf.life -= kDt;
            if (!anyMoving(rock_, n_)) settle();
        } else if (mode_ == Mode::Dead) {
            deadT_ += kDt;
            if (deadT_ > 0.8f) beginAim();
        } else if (mode_ == Mode::Ring) {
            ringT_ += kDt;
            if (ringT_ > 0.7f) {
                mode_ = Mode::Leave;
                leaveT_ = 0.f;
            }
        } else if (mode_ == Mode::Leave) {
            leaveT_ += kDt;
            if (leaveT_ > 1.05f) {
                mode_ = Mode::Over;
                won_ = true;
                over_ = true;
            }
        }
    }

    draw();
}

}  // namespace curlbell
