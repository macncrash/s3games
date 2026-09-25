#include "game/flak.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace flak {
namespace {

constexpr int kCount = 6;
constexpr int kMag = 10;
constexpr float kShellSpd = 230.f;
constexpr float kHitR = 24.f;
constexpr float kGunX = 160.f;
constexpr float kGunY = 176.f;
constexpr float kPivotY = 200.f;
constexpr float kLaneL = 136.f;
constexpr float kLaneR = 196.f;
constexpr float kFuse = 0.62f;
constexpr float kCool = 0.40f;
constexpr float kDt = 1.f / 60.f;

struct Spec {
    float t, y, vx;
};

// Staggered so a gunner who leads can meet them one at a time.
const Spec kRaid[kCount] = {
    {0.70f, 36.f, 78.f}, {4.00f, 62.f, -74.f}, {7.30f, 44.f, 82.f},
    {10.6f, 76.f, -70.f}, {13.9f, 32.f, 86.f}, {17.2f, 54.f, -76.f},
};

float wrap(float x, float m) {
    x = std::fmod(x, m);
    if (x < 0) x += m;
    return x;
}

int mix(int a, int b, float u) {
    u = std::clamp(u, 0.f, 1.f);
    return int(std::lround(a + (b - a) * u));
}

int barrelIndex(float deg) {
    int best = 0;
    float bd = 1e9f;
    for (int i = 0; i < 9; i++) {
        float d = std::fabs(kBarrelDeg[i] - deg);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.tone(0, 0, 0);
    sys.apu.tone(1, 0, 0);
    sys.apu.tone(2, 0, 0);
    mode_ = Mode::Title;
    t_ = 0;
    sightX_ = 150.f;
    sightY_ = 80.f;
}

void Game::beginRaid() {
    planes_.clear();
    shells_.clear();
    bombs_.clear();
    puffs_.clear();
    brass_.clear();
    rounds_ = kMag;
    splashed_ = 0;
    hits_ = 0;
    next_ = 0;
    t_ = 0;
    endT_ = 0;
    cool_ = 0;
    flash_ = 0;
    shake_ = 0;
    won_ = false;
    over_ = false;
    fan_ = -1;
    sightX_ = 160.f;
    sightY_ = 64.f;
    mode_ = Mode::Play;
}

void Game::finish(bool win) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::End;
    won_ = win;
    endT_ = 0;
    over_ = false;
    if (win) {
        fan_ = 0;
        fanT_ = 0;
        sys_->rumble(0.25f, 0.45f, 120);
    } else {
        fan_ = -1;
        shake_ = 0.5f;
        sys_->apu.noiseBurst(0.8f, 160.f, 0.45f);
        sys_->rumble(0.9f, 1.f, 240);
    }
}

bool Game::predict(const Plane& p, float& ax, float& ay) const {
    const float rx = p.x - kGunX;
    const float ry = p.y - kGunY;
    const float vx = p.vx;
    const float a = vx * vx - kShellSpd * kShellSpd;
    const float b = 2.f * rx * vx;
    const float c = rx * rx + ry * ry;
    float t;
    if (std::fabs(a) < 1e-3f) {
        if (std::fabs(b) < 1e-3f) return false;
        t = -c / b;
    } else {
        const float d = b * b - 4.f * a * c;
        if (d < 0) return false;
        const float s = std::sqrt(d);
        const float t1 = (-b - s) / (2.f * a);
        const float t2 = (-b + s) / (2.f * a);
        t = 1e9f;
        if (t1 > 0.05f) t = t1;
        if (t2 > 0.05f && t2 < t) t = t2;
        if (t > 1e8f) return false;
    }
    if (t < 0.05f || t > 2.4f) return false;
    ax = p.x + vx * t;
    ay = p.y;
    return ay > 8.f && ay < 130.f && ax > -10.f && ax < 330.f;
}

int Game::pickPlane() const {
    int best = -1;
    float bestEta = 1e9f;
    for (int i = 0; i < int(planes_.size()); i++) {
        const Plane& p = planes_[i];
        if (p.st != St::Fly) continue;
        float ax, ay;
        if (!predict(p, ax, ay)) continue;
        float eta;
        if (p.x > kLaneL && p.x < kLaneR) eta = -1.f;
        else eta = (p.vx > 0 ? kLaneL - p.x : kLaneR - p.x) / p.vx;
        if (eta < bestEta) {
            bestEta = eta;
            best = i;
        }
    }
    return best;
}

bool Game::raidClear() const {
    if (next_ < kCount) return false;
    for (const Plane& p : planes_)
        if (p.st == St::Fly || p.st == St::Run) return false;
    return shells_.empty() && bombs_.empty();
}

void Game::burstAt(float x, float y, bool hit) {
    Puff p;
    p.x = x;
    p.y = y;
    p.t = 0;
    p.life = hit ? 0.58f : 0.42f;
    p.kind = hit ? 1 : 0;
    puffs_.push_back(p);
    if (puffs_.size() > 12) puffs_.erase(puffs_.begin());
    if (hit) {
        sys_->apu.noiseBurst(0.62f, 480.f, 0.22f);
        sys_->rumble(0.35f, 0.7f, 90);
    } else {
        sys_->apu.noiseBurst(0.30f, 980.f, 0.14f);
    }
}

void Game::dropBomb(const Plane& p) {
    bombs_.push_back({p.x, p.y, 28.f});
    sys_->apu.tone(2, 220.f, 0.05f);
    beep_ = 0.08f;
}

void Game::tryFire(bool want) {
    if (!want || cool_ > 0.f || mode_ != Mode::Play) return;
    if (rounds_ <= 0) {
        cool_ = 0.22f;
        sys_->apu.tone(1, 90.f, 0.04f);
        beep_ = 0.05f;
        return;
    }
    cool_ = kCool;
    rounds_--;
    const float dx = sightX_ - kGunX;
    const float dy = sightY_ - kGunY;
    const float dur = std::max(0.18f, std::sqrt(dx * dx + dy * dy) / kShellSpd);
    shells_.push_back({kGunX, kGunY, sightX_, sightY_, 0.f, dur});
    flash_ = 0.08f;
    brass_.push_back({168.f, 190.f, 110.f, -60.f});
    sys_->apu.noiseBurst(0.40f, 1700.f, 0.08f);
    sys_->apu.tone(1, 360.f, 0.05f);
    beep_ = 0.06f;
    sys_->rumble(0.12f, 0.35f, 40);
}

void Game::updateTitle(float dt) {
    demoX_ += demoVx_ * dt;
    if (demoX_ > 300.f) demoVx_ = -std::fabs(demoVx_);
    if (demoX_ < 24.f) demoVx_ = std::fabs(demoVx_);
    const float dx = demoX_ - sightX_;
    const float dy = demoY_ - sightY_;
    const float d = std::hypot(dx, dy);
    if (d > 0.8f) {
        const float step = std::min(d, 150.f * dt);
        sightX_ += dx / d * step;
        sightY_ += dy / d * step;
    }
}

void Game::control(float dt) {
    bool want = false;
    if (bot_) {
        const int idx = pickPlane();
        if (idx >= 0) {
            float ax, ay;
            if (predict(planes_[size_t(idx)], ax, ay)) {
                const float dx = ax - sightX_;
                const float dy = ay - sightY_;
                const float d = std::hypot(dx, dy);
                if (d < 14.f) {
                    sightX_ = ax;
                    sightY_ = ay;
                    if (shells_.empty()) want = true;
                } else {
                    const float step = std::min(d, 480.f * dt);
                    sightX_ += dx / d * step;
                    sightY_ += dy / d * step;
                }
            }
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        float x = 0, y = 0;
        if (std::fabs(pad.axisX) > 0.12f) x = pad.axisX;
        else x = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        y = float(pad.down(gs::BTN_UP)) - float(pad.down(gs::BTN_DOWN));
        const float m = std::hypot(x, y);
        if (m > 1.f) {
            x /= m;
            y /= m;
        }
        sightX_ += x * 220.f * dt;
        sightY_ -= y * 220.f * dt;
        sightX_ = std::clamp(sightX_, 14.f, 306.f);
        sightY_ = std::clamp(sightY_, 16.f, 120.f);
        want = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) || pad.accel > 0.45f;
    }
    tryFire(want);
}

void Game::updateFx(float dt) {
    if (cool_ > 0) cool_ -= dt;
    if (flash_ > 0) flash_ -= dt;
    camX_ = camY_ = 0;
    if (shake_ > 0) {
        camX_ = std::sin(shake_ * 90.f) * 5.f * std::min(1.f, shake_ * 2.f);
        camY_ = std::cos(shake_ * 70.f) * 3.f * std::min(1.f, shake_ * 2.f);
        shake_ -= dt;
    }
    for (int i = int(puffs_.size()) - 1; i >= 0; --i) {
        puffs_[size_t(i)].t += dt;
        if (puffs_[size_t(i)].t >= puffs_[size_t(i)].life) puffs_.erase(puffs_.begin() + i);
    }
    for (Brass& c : brass_) {
        if (c.vy == 0 && c.y >= 208.f) continue;
        c.vy += 320.f * dt;
        c.x += c.vx * dt;
        c.y += c.vy * dt;
        c.vx *= (1.f - 2.2f * dt);
        if (c.y >= 208.f) {
            c.y = 208.f;
            c.vy = 0;
            c.vx = 0;
        }
        c.x = std::clamp(c.x, 8.f, 312.f);
    }
}

void Game::updatePlay(float dt) {
    while (next_ < kCount && t_ >= kRaid[next_].t) {
        Plane p;
        p.vx = kRaid[next_].vx;
        p.y = kRaid[next_].y;
        p.x = p.vx > 0 ? -36.f : 356.f;
        p.fuse = -1.f;
        p.st = St::Fly;
        planes_.push_back(p);
        next_++;
    }

    for (Plane& p : planes_) {
        if (p.st == St::Dead || p.st == St::Gone) continue;
        p.x += p.vx * dt;
        if (p.st == St::Fly) {
            if (p.fuse < 0.f && p.x > kLaneL && p.x < kLaneR) p.fuse = kFuse;
            if (p.fuse >= 0.f) {
                p.fuse -= dt;
                if (p.fuse <= 0.f) {
                    dropBomb(p);
                    p.st = St::Run;
                }
            }
        }
        if (p.x < -110.f || p.x > 430.f) p.st = St::Gone;
    }

    for (int i = int(shells_.size()) - 1; i >= 0; --i) {
        Shell& s = shells_[size_t(i)];
        s.t += dt;
        if (s.t < s.dur) continue;
        bool hit = false;
        for (Plane& p : planes_) {
            if (p.st != St::Fly) continue;
            const float dx = p.x - s.tx;
            const float dy = p.y - s.ty;
            if (dx * dx + dy * dy <= kHitR * kHitR) {
                p.st = St::Dead;
                p.fuse = -1.f;
                splashed_++;
                hit = true;
            }
        }
        burstAt(s.tx, s.ty, hit);
        shells_.erase(shells_.begin() + i);
    }

    for (int i = int(bombs_.size()) - 1; i >= 0; --i) {
        Bomb& b = bombs_[size_t(i)];
        b.vy += 240.f * dt;
        b.y += b.vy * dt;
        if (b.y < 200.f) continue;
        hits_++;
        burstAt(b.x, 192.f, true);
        bombs_.erase(bombs_.begin() + i);
        finish(false);
        break;
    }

    if (mode_ != Mode::Play) return;
    control(dt);
    if (raidClear()) finish(splashed_ == kCount);
}

void Game::audio(float dt) {
    if (fan_ >= 0) {
        static const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
        static const float when[4] = {0.f, 0.13f, 0.26f, 0.42f};
        if (fan_ < 4 && fanT_ >= when[fan_]) {
            sys_->apu.tone(0, notes[fan_], 0.07f);
            beep_ = 0.12f;
            fan_++;
        }
        fanT_ += dt;
    }
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        updateTitle(kDt);
        const gs::Pad& pad = sys.pad;
        if (bot_ && t_ > 0.45f) beginRaid();
        else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) beginRaid();
    } else if (mode_ == Mode::Play) {
        t_ += kDt;
        updatePlay(kDt);
    } else {
        endT_ += kDt;
        if (endT_ > 1.0f) over_ = true;
        const gs::Pad& pad = sys.pad;
        if (!bot_ && endT_ > 0.35f && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            t_ = 0;
            over_ = false;
            won_ = false;
            planes_.clear();
            shells_.clear();
            bombs_.clear();
            puffs_.clear();
            demoX_ = 70.f;
            demoVx_ = 62.f;
            sightX_ = 150.f;
            sightY_ = 80.f;
        }
    }
    updateFx(kDt);
    audio(kDt);
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 126) {
            const float u = y / 126.f;
            const int r = mix(1, 11, u * u);
            const int g = mix(2, 8, u);
            const int b = mix(7, 11, std::sqrt(u));
            c = gs::rgb4(r, g, b);
        } else if (y < 172) {
            const float u = (y - 126) / 46.f;
            const int glint = ((y + int(t_ * 28.f)) & 7) == 0 ? 1 : 0;
            c = gs::rgb4(mix(3, 1, u), mix(8, 4, u) + glint, mix(11, 7, u));
        } else {
            c = gs::rgb4(4, 3, 2);
        }
        if (y == 126 || y == 127) c = gs::rgb4(14, 12, 10);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float h, int pal) {
    const float srcH = float(art_.glyph['A' - 32].h);
    if (srcH < 1.f || s.empty()) return;
    const float adv = h * (18.f / srcH);
    float left = x - adv * float(s.size()) * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + (float(i) + 0.5f) * adv, y, h, pal, false, 0, false, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool world) {
    if (h < 1.2f || m.h < 1) return;
    if (world) {
        cx += camX_;
        cy += camY_;
    }
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::draw() {
    backdrop();
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();

    char line[40];
    if (mode_ == Mode::Title) {
        text("S3 FLAK", 160, 20, 30, PAL_WHITE);
        std::snprintf(line, sizeof line, "%d ROUNDS   %d BOMBERS", kMag, kCount);
        text(line, 160, 46, 15, PAL_AMBER);
        if ((int(t_ * 2.f) & 1) == 0) text("PRESS START", 160, 116, 14, PAL_WHITE);
        hudC(25, "ARROWS AIM", PAL_WHITE);
        hudC(26, "Z OR C FIRES", PAL_AMBER);
        hudC(27, "ENTER STARTS", PAL_GREEN);
    } else if (mode_ == Mode::End) {
        if (won_) {
            text("DECK HELD", 160, 52, 28, PAL_GREEN);
            text("SKY CLEAR", 160, 80, 16, PAL_AMBER);
        } else {
            text("DECK LOST", 160, 52, 28, PAL_RED);
            text("A BOMB ON THE DECK", 160, 82, 14, PAL_WHITE);
        }
        std::snprintf(line, sizeof line, "SPLASH %d  SHELLS %d", splashed_, rounds_);
        hudC(26, line, PAL_WHITE);
        if (!bot_) hudC(27, "ENTER AGAIN", PAL_AMBER);
    } else {
        std::snprintf(line, sizeof line, "SPLASH %d/%d", splashed_, kCount);
        hud(1, 0, line, PAL_WHITE);
        std::snprintf(line, sizeof line, "RND %02d", rounds_);
        hud(32, 0, line, rounds_ <= 2 ? PAL_RED : PAL_AMBER);
        if (t_ < 3.2f) hudC(2, "LEAD THEM", PAL_AMBER);
        else if (rounds_ <= 0 && splashed_ < kCount) hudC(2, "MAGAZINE EMPTY", PAL_RED);
        if (hits_ > 0) hud(1, 1, "DECK HIT", PAL_RED);
    }

    // Front to back. The first sprite submitted sits on top.
    spr(art_.sight, sightX_, sightY_, 36, PAL_AMBER, false, 0, false, true);

    if (flash_ > 0) spr(art_.flash, kGunX, kGunY - 8.f, 18, PAL_BURST, false);

    for (const Shell& s : shells_) {
        const float u = std::clamp(s.t / s.dur, 0.f, 1.f);
        const float x = s.x0 + (s.tx - s.x0) * u;
        const float y = s.y0 + (s.ty - s.y0) * u;
        spr(art_.tracer, x, y, 8, PAL_BURST, false);
    }
    for (const Puff& p : puffs_) {
        const float u = std::clamp(p.t / p.life, 0.f, 1.f);
        const float h = (p.kind == 1 ? 18.f : 12.f) + u * (p.kind == 1 ? 36.f : 22.f);
        spr(art_.puff, p.x, p.y, h, PAL_BURST, false, int(u * 6));
        if (p.t < 0.1f) spr(art_.flash, p.x, p.y, 16 - p.t * 80.f, PAL_BURST, false);
    }
    for (const Bomb& b : bombs_) spr(art_.bomb, b.x, b.y, 16, PAL_PLANE, false);

    auto drawBomber = [&](float x, float y, float vx) {
        const int fr = int(t_ * 16.f) & 1;
        const int fog = int(std::clamp((y - 28.f) / 16.f, 0.f, 4.f));
        spr(art_.bomber[fr], x, y, 34, PAL_PLANE, vx < 0, fog);
    };
    if (mode_ == Mode::Title) drawBomber(demoX_, demoY_, demoVx_);
    for (const Plane& p : planes_) {
        if (p.st == St::Dead || p.st == St::Gone) continue;
        drawBomber(p.x, p.y, p.vx);
    }

    spr(art_.gate, kLaneL, 108, 36, PAL_AMBER, false, 6);
    spr(art_.gate, kLaneR, 108, 36, PAL_AMBER, false, 6);

    const float deg = std::atan2(sightX_ - kGunX, kPivotY - sightY_) * (180.f / 3.14159265f);
    spr(art_.mount, 160, 216, 54, PAL_GUN, false, 0, true);
    spr(art_.barrel[barrelIndex(deg)], 160, kPivotY, 96, PAL_GUN, false, 0, true);

    for (int i = 0; i < rounds_; i++) {
        const int col = i % 5;
        const int row = i / 5;
        spr(art_.round, 214.f + col * 11.f, 196.f + row * 13.f, 13, PAL_GUN, false, 0, true);
    }
    for (const Brass& c : brass_) spr(art_.round, c.x, c.y, 12, PAL_GUN, c.x > 200.f, 0, true);

    if (hits_ > 0) spr(art_.crack, 168, 206, 26, PAL_SHIP, false, 0, true);

    spr(art_.funnel, 52, 186, 48, PAL_SHIP, false, 0, true);
    const float smokeY = 142.f + std::sin(t_ * 1.7f) * 2.f;
    spr(art_.smoke, 56, smokeY, 16, PAL_BURST, false, 3);
    spr(art_.ring, 92, 200, 16, PAL_SHIP, false, 0, true);
    spr(art_.crate, 292, 210, 22, PAL_SHIP, false, 0, true);

    for (int i = 0; i < 3; i++) spr(art_.rail, 40.f + i * 78.f, 174, 24, PAL_SHIP, false, 0, true);
    spr(art_.bow, 160, 178, 36, PAL_SHIP, false, 0, true);
    spr(art_.deck, 80, 224, 42, PAL_SHIP, false, 0, true);
    spr(art_.deck, 240, 224, 42, PAL_SHIP, false, 0, true);

    for (int i = 0; i < 4; i++) {
        const float x = wrap(i * 86.f - t_ * 26.f, 392.f) - 36.f;
        spr(art_.wave, x, 150.f + (i & 1) * 6.f, 12, PAL_SEA, i & 1, 2);
    }
    spr(art_.freighter, wrap(30.f + t_ * 10.f, 380.f) - 20.f, 136, 14, PAL_SHIP, false, 5);
    spr(art_.freighter, wrap(210.f - t_ * 7.f, 380.f) - 20.f, 140, 16, PAL_SHIP, true, 6);

    const int flap = int(t_ * 8.f) & 1;
    spr(art_.bird[flap], wrap(t_ * 36.f, 360.f), 28, 8, PAL_SKY, false, 1);
    spr(art_.bird[flap ^ 1], wrap(40.f + t_ * 36.f, 360.f), 34, 7, PAL_SKY, false, 2);

    struct Cloud {
        float x, y, h, v, fog;
    };
    static const Cloud kClouds[] = {{18.f, 30.f, 26.f, 7.f, 2.f}, {150.f, 18.f, 34.f, 4.f, 1.f},
                                    {250.f, 42.f, 22.f, 9.f, 3.f}, {90.f, 64.f, 18.f, 12.f, 1.f}};
    for (const Cloud& c : kClouds) {
        const float x = wrap(c.x + t_ * c.v, 420.f) - 50.f;
        spr(art_.cloud, x, c.y, c.h, PAL_SKY, false, int(c.fog));
    }
    spr(art_.sun, 278, 34, 30, PAL_SKY, false, 0);
}

}  // namespace flak
