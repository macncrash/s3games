#include "game/curlmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace curlmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSub = 1.f / 240.f;
constexpr int kSubs = 4;
constexpr int kCastCap = 240 * 8;

bool liePlays(float x, float y, bool dead) {
    if (dead) return false;
    if (y < kHog + kStoneR - 0.02f) return false;
    if (y + kStoneR > kBack + 0.05f) return false;
    if (std::fabs(x) + kStoneR > kSide + 0.02f) return false;
    return true;
}

bool lieMarks(float x, float y, bool dead) {
    return liePlays(x, y, dead) && std::hypot(x - kMarkX, y - kMarkY) <= kBite;
}

float speed(const Rock& r) { return std::hypot(r.vx, r.vy); }

void shove(Rock& r, float sweep, float dt) {
    float sp = speed(r);
    if (sp < kStop) {
        r.vx = r.vy = 0;
        return;
    }
    float a = kA * (1.f - kSweepDrag * sweep);
    float c = kC * (1.f - kSweepCurl * sweep) * float(r.handle);
    float ax = -a * r.vx / sp + c * r.vy / sp;
    float ay = -a * r.vy / sp - c * r.vx / sp;
    r.vx += ax * dt;
    r.vy += ay * dt;
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
            if (d >= minD || d < 1e-5f) continue;
            hit = true;
            float nx = dx / d, ny = dy / d;
            float overlap = minD - d;
            r[i].x -= nx * overlap * 0.5f;
            r[i].y -= ny * overlap * 0.5f;
            r[j].x += nx * overlap * 0.5f;
            r[j].y += ny * overlap * 0.5f;
            float rel = (r[i].vx - r[j].vx) * nx + (r[i].vy - r[j].vy) * ny;
            if (rel <= 0.f) continue;
            float jolt = (1.f + 0.86f) * rel * 0.5f;
            r[i].vx -= jolt * nx;
            r[i].vy -= jolt * ny;
            r[j].vx += jolt * nx;
            r[j].vy += jolt * ny;
        }
    }
    for (int i = 0; i < n; i++) {
        if (r[i].dead) continue;
        if (r[i].y > kBack || std::fabs(r[i].x) > kSide) {
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

}  // namespace

void Game::place() {
    for (auto& r : rock_) r = {};
    rock_[0].x = kGuardX;
    rock_[0].y = kGuardY;
    n_ = 1;
    thrown_ = 0;
    stone_ = 0;
    bitten_ = lifted_ = finished_ = won_ = over_ = onMark_ = false;
    solved_ = false;
    wasOn_ = false;
    hitSnd_ = false;
    sweepArm_ = false;
    handle_ = 1;
    aimVx_ = 0.f;
    aimVy_ = 15.4f;
    sweep_ = 0.f;
    broomX_ = kMarkX;
    broomY_ = kMarkY - 1.75f;
    lieX_ = kReleaseX;
    lieY_ = kReleaseY;
    ghostOn_ = false;
    ghostDead_ = false;
    pathN_ = 0;
    solDist_ = 99.f;
    for (auto& p : puff_) p.life = 0;
}

void Game::begin() {
    place();
    mode_ = Mode::Aim;
    blip(392.f, 0.05f);
}

void Game::toTitle() {
    place();
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(9, 12, 14));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.16f, 0.26f, 0.16f);
    sys.setLight(70, 130, 170);
    place();
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    toneT_ = 0.12f;
    chord_ = false;
}

void Game::chord() {
    if (!sys_) return;
    sys_->apu.tone(0, 523.25f, 0.07f);
    sys_->apu.tone(1, 659.25f, 0.06f);
    sys_->apu.tone(2, 783.99f, 0.055f);
    toneT_ = 0.48f;
    chord_ = true;
}

void Game::decayAudio() {
    if (!sys_) return;
    if (toneT_ > 0.f) {
        toneT_ -= kDt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
            chord_ = false;
        }
    }
    if (mode_ == Mode::Slide && sweep_ > 0.5f) sys_->apu.noise(0.045f, 2400.f, false);
    else if (!chord_) sys_->apu.noise(0.f, 0.f, false);
}

void Game::launch(float vx, float vy) {
    if (n_ >= kMaxRock || thrown_ >= kMaxThrow) return;
    Rock& r = rock_[n_++];
    r = {};
    r.x = kReleaseX;
    r.y = kReleaseY;
    r.vx = vx;
    r.vy = vy;
    r.handle = handle_;
    r.ours = true;
    thrown_++;
    sweep_ = 0.f;
    sweepArm_ = false;
    hitSnd_ = false;
    mode_ = Mode::Slide;
    blip(148.f, 0.07f);
}

void Game::fail() {
    won_ = false;
    finished_ = false;
    lifted_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    if (sys_) sys_->setLight(170, 30, 30);
    blip(92.f, 0.07f);
}

void Game::finish() {
    broomX_ = kMarkX;
    broomY_ = kMarkY;
    lifted_ = true;
    finished_ = true;
    won_ = true;
    onMark_ = true;
    over_ = true;
    mode_ = Mode::Win;
    if (sys_) {
        sys_->setLight(220, 160, 40);
        sys_->rumble(0.35f, 0.55f, 140);
    }
    chord();
}

void Game::settle() {
    int last = n_ - 1;
    if (last < 0 || !rock_[last].ours) {
        fail();
        return;
    }
    Rock& r = rock_[last];
    if (!r.dead && r.y < kHog + kStoneR) {
        r.dead = true;
        r.vx = r.vy = 0;
    }
    lieX_ = r.x;
    lieY_ = r.y;
    if (lieMarks(r.x, r.y, r.dead)) {
        bitten_ = true;
        onMark_ = true;
        stone_ = thrown_;
        broomX_ = kMarkX;
        broomY_ = kMarkY - 1.75f;
        mode_ = Mode::Open;
        if (sys_) sys_->rumble(0.2f, 0.45f, 90);
        blip(784.f, 0.07f);
        return;
    }
    if (thrown_ >= kMaxThrow) {
        fail();
        return;
    }
    solved_ = false;
    wasOn_ = false;
    mode_ = Mode::Aim;
    blip(110.f, 0.05f);
}

Game::ShotEnd Game::cast(float vx, float vy, int handle, bool record) {
    Rock tmp[kMaxRock];
    int n = n_;
    if (n > kMaxRock - 1) n = kMaxRock - 1;
    for (int i = 0; i < n; i++) tmp[i] = rock_[i];
    Rock& shot = tmp[n];
    shot = {};
    shot.x = kReleaseX;
    shot.y = kReleaseY;
    shot.vx = vx;
    shot.vy = vy;
    shot.handle = handle;
    shot.ours = true;
    int active = n;
    n++;
    if (record) pathN_ = 0;
    bool hit = false;
    int steps = 0;
    for (; steps < kCastCap; steps++) {
        if (!anyMoving(tmp, n)) break;
        hit = stepRocks(tmp, n, active, 0.f, kSub) || hit;
        if (record && pathN_ < 6 && (steps % 70) == 20) {
            pathX_[pathN_] = tmp[active].x;
            pathY_[pathN_] = tmp[active].y;
            pathN_++;
        }
    }
    ShotEnd e;
    e.x = tmp[active].x;
    e.y = tmp[active].y;
    e.dead = tmp[active].dead;
    e.hit = hit;
    if (!e.dead && e.y < kHog + kStoneR) e.dead = true;
    return e;
}

bool Game::solve(float& vx, float& vy, int& handle) {
    float best = 1e9f;
    bool any = false;
    auto take = [&](float tvx, float tvy, int th) {
        ShotEnd e = cast(tvx, tvy, th, false);
        if (e.dead || e.hit || !liePlays(e.x, e.y, false)) return;
        float d = std::hypot(e.x - kMarkX, e.y - kMarkY);
        if (d < best) {
            best = d;
            vx = tvx;
            vy = tvy;
            handle = th;
            any = true;
        }
    };
    for (int iy = 0; iy <= 40 && best > 0.08f; iy++) {
        float tvy = 15.6f + float(iy) * 0.1f;
        for (int ix = 0; ix <= 36 && best > 0.08f; ix++) {
            float tvx = -2.2f + float(ix) * 0.08f;
            take(tvx, tvy, 1);
        }
    }
    if (best > kBite) {
        for (int iy = 0; iy <= 28 && best > 0.08f; iy++) {
            float tvy = 15.6f + float(iy) * 0.14f;
            for (int ix = 0; ix <= 28 && best > 0.08f; ix++) {
                float tvx = -2.2f + float(ix) * 0.12f;
                take(tvx, tvy, -1);
            }
        }
    }
    if (any) {
        float bx = vx, by = vy;
        int bh = handle;
        for (int iy = -5; iy <= 5; iy++)
            for (int ix = -5; ix <= 5; ix++) take(bx + float(ix) * 0.02f, by + float(iy) * 0.02f, bh);
    }
    solDist_ = best;
    solVx_ = vx;
    solVy_ = vy;
    solHandle_ = handle;
    return any && best <= kBite;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    decayAudio();
    const gs::Pad& p = sys.pad;
    const bool action = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    const bool start = p.pressed(gs::BTN_START);
    const bool back = p.pressed(gs::BTN_MODE);

    if (mode_ == Mode::Title) {
        if (bot_) {
            if (clock_ > 0.45f) begin();
        } else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || action) begin();
    } else if (mode_ == Mode::Pause) {
        if (start || action) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (start || action)) begin();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && (start || back)) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            if (!solved_) {
                float vx = 0, vy = 17.f;
                int h = 1;
                solved_ = true;
                bool ok = solve(vx, vy, h);
                if (!ok) std::fprintf(stderr, "s3curlmark no draw  best %.3f\n", solDist_);
                solVx_ = vx;
                solVy_ = vy;
                solHandle_ = h;
            }
            handle_ = solHandle_;
            launch(solVx_, solVy_);
        } else {
            if (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_Z)) handle_ = -handle_;
            float mx = 0, my = 0;
            if (p.down(gs::BTN_LEFT)) mx -= 1.f;
            if (p.down(gs::BTN_RIGHT)) mx += 1.f;
            if (p.down(gs::BTN_UP)) my += 1.f;
            if (p.down(gs::BTN_DOWN)) my -= 1.f;
            if (std::fabs(p.axisX) > 0.22f) mx = p.axisX;
            if (std::fabs(p.axisY) > 0.22f) my = p.axisY;
            aimVx_ = std::clamp(aimVx_ + mx * 1.85f * kDt, -3.1f, 2.4f);
            aimVy_ = std::clamp(aimVy_ + my * 2.7f * kDt, 11.f, 21.f);
            ShotEnd e = cast(aimVx_, aimVy_, handle_, true);
            ghostX_ = e.x;
            ghostY_ = e.y;
            ghostDead_ = e.dead;
            ghostOn_ = lieMarks(e.x, e.y, e.dead);
            if (ghostOn_ && !wasOn_) blip(880.f, 0.04f);
            wasOn_ = ghostOn_;
            if (action) launch(aimVx_, aimVy_);
        }
    } else if (mode_ == Mode::Slide) {
        bool held = !bot_ && (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO));
        if (!held) sweepArm_ = true;
        sweep_ = (sweepArm_ && held) ? 1.f : 0.f;
        int active = n_ - 1;
        bool hit = false;
        for (int s = 0; s < kSubs; s++) hit = stepRocks(rock_, n_, active, sweep_, kSub) || hit;
        if (hit && !hitSnd_) {
            hitSnd_ = true;
            sys.apu.noiseBurst(0.16f, 1800.f, 12.f);
        }
        if (sweep_ > 0.5f && active >= 0 && !rock_[active].dead) {
            for (auto& pf : puff_) {
                if (pf.life > 0.f) continue;
                pf.x = rock_[active].x + (handle_ > 0 ? 0.45f : -0.45f);
                pf.y = rock_[active].y - 0.15f;
                pf.life = 0.26f;
                break;
            }
        }
        for (auto& pf : puff_)
            if (pf.life > 0.f) pf.life -= kDt;
        if (!anyMoving(rock_, n_)) settle();
    } else if (mode_ == Mode::Open) {
        if (bot_) {
            float dx = kMarkX - broomX_;
            float dy = kMarkY - broomY_;
            float d = std::hypot(dx, dy);
            if (d <= 0.7f) finish();
            else {
                float step = std::min(d, 6.5f * kDt);
                broomX_ += dx / d * step;
                broomY_ += dy / d * step;
            }
        } else {
            float mx = 0, my = 0;
            if (p.down(gs::BTN_LEFT)) mx -= 1.f;
            if (p.down(gs::BTN_RIGHT)) mx += 1.f;
            if (p.down(gs::BTN_UP)) my += 1.f;
            if (p.down(gs::BTN_DOWN)) my -= 1.f;
            if (std::fabs(p.axisX) > 0.22f) mx = p.axisX;
            if (std::fabs(p.axisY) > 0.22f) my = p.axisY;
            broomX_ = std::clamp(broomX_ + mx * 4.8f * kDt, -kSide, kSide);
            broomY_ = std::clamp(broomY_ + my * 4.8f * kDt, kHog, kBack);
            if (action) {
                if (std::hypot(broomX_ - kMarkX, broomY_ - kMarkY) <= 0.85f) finish();
                else blip(140.f, 0.04f);
            }
        }
    }

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

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int band = 2 + ((y / 14) & 1);
        v.lineBackdrop[y] = gs::rgb4(band + 1, band, 1);
    }

    const Mode view = mode_ == Mode::Pause ? held_ : mode_;
    const float stoneH = kStoneR * 2.f * kPx * 1.2f;
    const bool hack = view == Mode::Title || view == Mode::Aim;

    auto stoneAt = [&](float wx, float wy, int pal, int handle) {
        float sx = screenX(wx);
        float sy = screenY(wy);
        sprM(art_.stone, sx, sy, stoneH, pal, handle < 0);
    };
    auto shadeAt = [&](float wx, float wy) {
        float sx = screenX(wx);
        float sy = screenY(wy);
        spr(art_.shadow, sx, sy + stoneH * 0.28f, stoneH * 0.95f, stoneH * 0.36f, PAL_ICE, false, true);
    };

    if (view == Mode::Title) spr(art_.logo, 160.f, 168.f, float(art_.logo.w), float(art_.logo.h), PAL_TITLE);
    if (view == Mode::Win) spr(art_.fin, 160.f, 168.f, float(art_.fin.w), float(art_.fin.h), PAL_WIN);
    if (view == Mode::Lose) spr(art_.open, 160.f, 168.f, float(art_.open.w), float(art_.open.h), PAL_ALERT);

    if (lifted_) {
        float sx = screenX(kMarkX);
        float sy = screenY(kMarkY) - 16.f;
        spr(art_.mark, sx, sy, 13.f, 13.f, PAL_GOLD);
    }

    const bool showBroom = view == Mode::Open || view == Mode::Win || (view == Mode::Slide && sweep_ > 0.5f);
    if (showBroom) {
        float bx = view == Mode::Slide && n_ > 0 ? rock_[n_ - 1].x + (handle_ > 0 ? 0.7f : -0.7f) : broomX_;
        float by = view == Mode::Slide && n_ > 0 ? rock_[n_ - 1].y : broomY_;
        spr(art_.broom, screenX(bx), screenY(by) - 8.f, float(art_.broom.w), float(art_.broom.h), PAL_BROOM);
    }
    for (const auto& pf : puff_) {
        if (pf.life <= 0.f) continue;
        float h = 3.f + pf.life * 16.f;
        spr(art_.puff, screenX(pf.x), screenY(pf.y), h, h * 0.8f, PAL_PUFF);
    }

    if (view == Mode::Aim && !ghostDead_) stoneAt(ghostX_, ghostY_, PAL_GHOST, handle_);
    if (hack) stoneAt(kReleaseX, kReleaseY, PAL_RED, handle_);
    for (int i = n_ - 1; i >= 1; i--) {
        if (rock_[i].dead) continue;
        stoneAt(rock_[i].x, rock_[i].y, PAL_RED, rock_[i].handle);
    }
    if (!rock_[0].dead) stoneAt(rock_[0].x, rock_[0].y, PAL_YEL, 0);

    if (view == Mode::Aim) {
        for (int i = 0; i < pathN_; i++)
            spr(art_.dot, screenX(pathX_[i]), screenY(pathY_[i]), 3.f, 3.f, ghostOn_ ? PAL_GREEN : PAL_AIM);
    }

    const bool ring = view == Mode::Title || (view == Mode::Aim && ghostOn_) || view == Mode::Open;
    if (ring) {
        float rad = view == Mode::Title ? 15.f + std::sin(clock_ * 3.f) * 2.f : 13.f;
        int pal = (view == Mode::Aim && ghostOn_) || view == Mode::Open ? PAL_GREEN : PAL_GOLD;
        int dots = view == Mode::Title ? 8 : 10;
        for (int i = 0; i < dots; i++) {
            float a = clock_ * 1.3f + float(i) * 6.2831853f / float(dots);
            spr(art_.dot, screenX(kMarkX) + std::cos(a) * rad, screenY(kMarkY) + std::sin(a) * rad, 3.f, 3.f, pal);
        }
    }

    if (!lifted_) spr(art_.mark, screenX(kMarkX), screenY(kMarkY), 12.f, 12.f, PAL_GOLD);

    if (hack) shadeAt(kReleaseX, kReleaseY);
    if (view == Mode::Aim && !ghostDead_) shadeAt(ghostX_, ghostY_);
    for (int i = 0; i < n_; i++) {
        if (rock_[i].dead) continue;
        if (i == 0 || rock_[i].ours) shadeAt(rock_[i].x, rock_[i].y);
    }

    if (view == Mode::Aim) {
        float u = std::clamp((aimVy_ - 11.f) / 10.f, 0.f, 1.f);
        for (int i = 0; i < 11; i++)
            spr(art_.dot, 300.f, 46.f + float(i) * 12.f, 3.f, 3.f, PAL_DIM);
        spr(art_.dot, 300.f, 46.f + (1.f - u) * 120.f, 5.f, 5.f, ghostOn_ ? PAL_GREEN : PAL_GOLD);
    }

    gs::Sprite sheet;
    sheet.img = art_.rink;
    sheet.x = int16_t(rinkX());
    sheet.y = 0;
    sheet.w = int16_t(art_.rink.w);
    sheet.h = int16_t(art_.rink.h);
    sheet.pal = PAL_ICE;
    v.sprite(sheet);

    hud(0, 0, "CURLMARK", PAL_TITLE);
    char buf[20];
    if (view == Mode::Title) {
        hud(0, 2, "DRAW", PAL_GOLD);
        hud(0, 3, "THE GOLD", PAL_INK);
        hud(0, 4, "MARK", PAL_GOLD);
        hud(0, 6, "LINE", PAL_INK);
        hud(0, 7, "LEFT", PAL_DIM);
        hud(0, 8, "RIGHT", PAL_DIM);
        hud(0, 10, "WEIGHT", PAL_INK);
        hud(0, 11, "UP DOWN", PAL_DIM);
        hud(0, 13, "HANDLE", PAL_INK);
        hud(0, 14, "X", PAL_GOLD);
        hud(0, 16, "THROW", PAL_INK);
        hud(0, 17, "Z", PAL_GOLD);
        hud(0, 19, "SWEEP", PAL_INK);
        hud(0, 20, "HOLD Z", PAL_DIM);
        hud(0, 22, "LIFT", PAL_INK);
        hud(0, 23, "Z", PAL_GOLD);
        hud(33, 2, "GOLD", PAL_GOLD);
        hud(33, 3, "IS THE", PAL_INK);
        hud(33, 4, "MARK", PAL_GOLD);
        hud(33, 6, "FOUR", PAL_INK);
        hud(33, 7, "STONES", PAL_DIM);
        hudC(27, "LIFT THE MARK TO FINISH", PAL_TITLE);
        return;
    }

    std::snprintf(buf, sizeof buf, "%d/%d", std::min(thrown_ + (view == Mode::Aim ? 1 : 0), kMaxThrow), kMaxThrow);
    hud(0, 2, "STONE", PAL_INK);
    hud(0, 3, buf, PAL_GOLD);
    hud(0, 5, "CURL", PAL_INK);
    hud(0, 6, handle_ > 0 ? "RIGHT" : "LEFT", PAL_GOLD);
    hud(33, 2, "GUARD", PAL_INK);
    hud(33, 3, "YELLOW", PAL_GOLD);

    if (view == Mode::Aim) {
        const char* call = "LINE";
        int pal = PAL_INK;
        if (ghostDead_) {
            call = ghostY_ > kTee ? "HEAVY" : "SHORT";
            pal = PAL_ALERT;
        } else if (ghostOn_) {
            call = "MARK";
            pal = PAL_GREEN;
        } else if (std::hypot(ghostX_, ghostY_ - kTee) <= kHouse) {
            call = "HOUSE";
            pal = PAL_AIM;
        } else if (ghostY_ < kMarkY - 1.2f) {
            call = "LIGHT";
            pal = PAL_AIM;
        }
        hud(0, 8, "LIE", PAL_INK);
        hud(0, 9, call, pal);
        hudC(27, ghostOn_ ? "Z THROWS THE DRAW" : "PUT THE GHOST ON IT", ghostOn_ ? PAL_GREEN : PAL_INK);
    } else if (view == Mode::Slide) {
        hud(0, 8, "SLIDE", PAL_INK);
        hud(0, 9, sweep_ > 0.5f ? "SWEEP" : "LET IT", sweep_ > 0.5f ? PAL_GOLD : PAL_DIM);
        hudC(27, "HOLD Z TO SWEEP", PAL_INK);
    } else if (view == Mode::Open) {
        bool close = std::hypot(broomX_ - kMarkX, broomY_ - kMarkY) <= 0.85f;
        hud(0, 8, "OPEN", PAL_GREEN);
        hud(0, 9, close ? "LIFT" : "BROOM", close ? PAL_GOLD : PAL_INK);
        hudC(27, close ? "Z LIFTS THE MARK" : "BROOM TO THE MARK", close ? PAL_GREEN : PAL_GOLD);
    } else if (view == Mode::Win) {
        hud(0, 8, "DONE", PAL_GREEN);
        hudC(27, "FINISHED MARK", PAL_WIN);
    } else if (view == Mode::Lose) {
        hud(0, 8, "OPEN", PAL_ALERT);
        hudC(27, "MARK STILL OPEN", PAL_ALERT);
    }
    if (mode_ == Mode::Pause) hudC(27, "PAUSED", PAL_GOLD);
}

}  // namespace curlmark
