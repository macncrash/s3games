#include "game/depot.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace depotwell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kCoupler = 16.f;
constexpr float kLaneV = 4.6f;
constexpr float kAccel = 260.f;
constexpr float kMaxV = 110.f;
constexpr float kBrake = 460.f;
constexpr float kDrag = 160.f;
constexpr float kMinX = 48.f;
constexpr float kMaxX = 220.f;
constexpr float kTankHold = 0.36f;
constexpr int kCourses = 3;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int c0, int c1) { return int(c0 + (c1 - c0) * t + 0.5f); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.22f;
    p.op[0] = {1.f, 0.6f, 0.5f, 1.2f, 0.75f, 0.5f};
    p.op[1] = {2.f, 0.22f, 0.4f, 0.9f, 0.45f, 0.4f};
    p.op[2] = {0.5f, 0.4f, 0.45f, 1.1f, 0.6f, 0.45f};
    p.op[3] = {3.f, 0.1f, 0.3f, 0.7f, 0.25f, 0.35f};
    p.vol = 0.1f;
    p.tone = 480.f;
    p.drive = 0.04f;
    return p;
}

gs::FMPatch clankPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.16f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.08f, 0.f, 0.05f};
    p.op[1] = {2.2f, 0.4f, 0.004f, 0.07f, 0.f, 0.05f};
    p.op[2] = {0.5f, 0.28f, 0.006f, 0.1f, 0.f, 0.07f};
    p.op[3] = {3.5f, 0.16f, 0.005f, 0.06f, 0.f, 0.04f};
    p.vol = 0.2f;
    p.tone = 1600.f;
    return p;
}

gs::FMPatch whistlePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.02f, 0.16f, 0.5f, 0.12f};
    p.op[1] = {2.f, 0.32f, 0.02f, 0.18f, 0.32f, 0.12f};
    p.op[2] = {3.f, 0.16f, 0.03f, 0.2f, 0.22f, 0.14f};
    p.op[3] = {1.f, 0.24f, 0.02f, 0.18f, 0.36f, 0.12f};
    p.vol = 0.16f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) return 1;
    return 0;
}

const char* Game::waveName(int w) const {
    if (w <= 0) return "BOXES";
    if (w == 1) return "BARRELS";
    return "THE TANK";
}

const char* Game::sidingName(int lane) const {
    if (lane <= 0) return "NORTH";
    if (lane == 1) return "MIDDLE";
    return "SOUTH";
}

float Game::halfOf(int kind) const { return kind == 2 ? 22.f : 18.f; }

float Game::bodyH(int kind) const { return kind == 2 ? 22.f : 24.f; }

void Game::blip(float freq, float vol) { sys_->apu.keyOn(1, freq, vol); }

void Game::buildScript(int wave) {
    script_.clear();
    auto add = [&](float t, int lane, int kind, float speed) {
        Spawn s;
        s.t = t;
        s.lane = lane;
        s.kind = kind;
        s.speed = speed;
        script_.push_back(s);
    };
    if (wave <= 0) {
        const int lanes[] = {1, 0, 2, 1, 0};
        for (int i = 0; i < 5; ++i) add(0.45f + i * 1.9f, lanes[i], 0, 44.f);
    } else if (wave == 1) {
        const int lanes[] = {0, 2, 1, 0, 2, 1};
        const int kinds[] = {1, 1, 0, 1, 1, 0};
        const float speeds[] = {50.f, 48.f, 52.f, 50.f, 47.f, 51.f};
        for (int i = 0; i < 6; ++i) add(0.40f + i * 1.8f, lanes[i], kinds[i], speeds[i]);
    } else {
        add(0.40f, 1, 0, 58.f);
        add(2.15f, 0, 0, 56.f);
        add(3.90f, 2, 1, 60.f);
        add(5.70f, 1, 0, 54.f);
        add(8.10f, 0, 2, 32.f);
        add(12.30f, 2, 0, 52.f);
        add(14.15f, 1, 1, 55.f);
    }
}

void Game::startWave() {
    wagons_.clear();
    pops_.clear();
    puffs_.clear();
    spawnAt_ = 0;
    tWave_ = 0.f;
    buildScript(wave_);
    if (!script_.empty()) laneTarget_ = script_[0].lane;
}

void Game::begin() {
    wave_ = 0;
    nextWave_ = 0;
    courses_ = kCourses;
    breaches_ = 0;
    held_ = 0;
    score_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "UNFINISHED";
    fanStep_ = -1;
    fanT_ = 0.f;
    shake_ = 0.f;
    speed_ = 0.f;
    x_ = kMarkX + kCoupler;
    mode_ = Mode::Play;
    startWave();
    lanePos_ = float(laneTarget_);
    sys_->setLight(70, 42, 16);
    blip(392.f, 0.1f);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    wagons_.clear();
    pops_.clear();
    reason_ = "UNFINISHED";
    sys_->setLight(50, 32, 14);
}

void Game::lose() {
    if (over_) return;
    courses_ = 0;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    reason_ = "THE WATCH IS OVER";
    shake_ = 0.9f;
    sys_->rumble(0.9f, 1.f, 240);
    sys_->apu.noiseBurst(0.55f, 240.f, 0.22f);
    blip(70.f, 0.16f);
    sys_->setLight(90, 16, 12);
}

void Game::win() {
    if (over_ || courses_ <= 0) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE WELL STANDS";
    score_ += courses_ * 400;
    fanStep_ = 0;
    fanT_ = 0.f;
    shake_ = 0.12f;
    sys_->setLight(40, 90, 28);
    blip(523.f, 0.16f);
}

void Game::couple(Wagon& w) {
    if (!w.alive) return;
    w.alive = false;
    w.pinned = false;
    ++held_;
    int pts = w.kind == 2 ? 500 : (w.kind == 1 ? 150 : 100);
    score_ += pts;
    Pop p;
    p.x = w.nose - 8.f;
    p.y = laneFoot(w.lane) - 36.f;
    p.t = 0.7f;
    p.pts = pts;
    pops_.push_back(p);
    Puff u;
    u.x = x_ - 12.f;
    u.y = laneFoot(w.lane) - 22.f;
    u.t = 0.28f;
    puffs_.push_back(u);
    shake_ = std::max(shake_, w.kind == 2 ? 0.22f : 0.08f);
    sys_->rumble(0.22f, 0.4f, 40);
    sys_->apu.noiseBurst(0.32f, w.kind == 2 ? 520.f : 1500.f, 0.05f);
    blip(w.kind == 2 ? 196.f : 480.f, 0.12f);
}

void Game::hitWell(Wagon& w) {
    if (!w.alive) return;
    w.alive = false;
    w.pinned = false;
    ++breaches_;
    --courses_;
    shake_ = 0.85f;
    Puff u;
    u.x = kWellX - 20.f;
    u.y = laneFoot(w.lane) - 16.f;
    u.t = 0.35f;
    puffs_.push_back(u);
    sys_->rumble(0.85f, 1.f, 160);
    sys_->apu.noiseBurst(0.5f, 320.f, 0.16f);
    blip(84.f, 0.14f);
    if (courses_ <= 0) {
        courses_ = 0;
        lose();
    }
}

int Game::urgent() const {
    int best = -1;
    float bestT = 1e9f;
    for (int i = 0; i < int(wagons_.size()); ++i) {
        const Wagon& w = wagons_[i];
        if (!w.alive) continue;
        if (w.pinned) return i;
        float tti = (kImpactX - w.nose) / std::max(8.f, w.speed);
        if (tti < bestT) {
            bestT = tti;
            best = i;
        }
    }
    return best;
}

void Game::bot(float& steer, bool& brake) {
    int idx = urgent();
    if (idx >= 0 && wagons_[idx].pinned) {
        laneTarget_ = wagons_[idx].lane;
        steer = 0.f;
        brake = true;
        speed_ = 0.f;
        return;
    }
    int lane = 1;
    float want = kMarkX + kCoupler;
    if (idx >= 0) {
        const Wagon& w = wagons_[idx];
        lane = w.lane;
        if (w.nose > kMarkX + 10.f && std::fabs(lanePos_ - float(w.lane)) < 0.35f) {
            float plant = std::min(kImpactX - 26.f, w.nose + 10.f);
            want = plant + kCoupler;
        }
    } else if (spawnAt_ < int(script_.size())) {
        lane = script_[spawnAt_].lane;
    }
    laneTarget_ = lane;
    float dx = want - x_;
    if (std::fabs(dx) < 0.8f) {
        steer = 0.f;
        brake = true;
        speed_ = 0.f;
        x_ = want;
    } else {
        steer = dx > 0.f ? 1.f : -1.f;
        brake = (speed_ * dx < 0.f) || (std::fabs(dx) < 14.f && std::fabs(speed_) > 24.f);
    }
}

void Game::human(float& steer, bool& brake) {
    const gs::Pad& p = sys_->pad;
    bool up = p.axisY > 0.55f;
    bool dn = p.axisY < -0.55f;
    if (p.pressed(gs::BTN_UP) || (up && !upLatch_)) laneTarget_ = std::max(0, laneTarget_ - 1);
    if (p.pressed(gs::BTN_DOWN) || (dn && !dnLatch_)) laneTarget_ = std::min(2, laneTarget_ + 1);
    upLatch_ = up;
    dnLatch_ = dn;
    steer = 0.f;
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.25f) steer += 1.f;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.25f) steer -= 1.f;
    steer = std::clamp(steer, -1.f, 1.f);
    brake = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) ||
            p.down(gs::BTN_TURBO) || p.brake > 0.35f;
}

void Game::moveShunter(float steer, bool brake, float dt) {
    braking_ = brake;
    float dl = float(laneTarget_) - lanePos_;
    float step = kLaneV * dt;
    if (std::fabs(dl) <= step) lanePos_ = float(laneTarget_);
    else lanePos_ += std::copysign(step, dl);

    if (brake) {
        float d = kBrake * dt;
        if (std::fabs(speed_) <= d) speed_ = 0.f;
        else speed_ -= std::copysign(d, speed_);
    } else if (steer != 0.f) {
        speed_ += steer * kAccel * dt;
    } else {
        float d = kDrag * dt;
        if (std::fabs(speed_) <= d) speed_ = 0.f;
        else speed_ -= std::copysign(d, speed_);
    }
    speed_ = std::clamp(speed_, -kMaxV, kMaxV);
    x_ += speed_ * dt;
    x_ = std::clamp(x_, kMinX, kMaxX);
}

void Game::updateWagons(float dt) {
    float coupler = x_ - kCoupler;
    for (Wagon& w : wagons_) {
        if (!w.alive) continue;
        w.anim += dt * 8.f;
        if (w.flash > 0.f) w.flash -= dt;
        bool aligned = std::fabs(lanePos_ - float(w.lane)) < 0.28f;
        bool touch = aligned && w.nose >= coupler - 6.f && w.nose <= coupler + 22.f;
        if (w.pinned) {
            if (!braking_) w.pinned = false;
            else {
                w.hold += dt;
                w.flash = 0.06f;
                if (w.kind != 2 || w.hold >= kTankHold) couple(w);
                continue;
            }
        }
        if (touch && braking_) {
            w.pinned = true;
            w.flash = 0.08f;
            if (w.kind != 2) couple(w);
            else {
                w.hold += dt;
                if (w.hold >= kTankHold) couple(w);
            }
            continue;
        }
        if (touch && !braking_) {
            x_ = std::min(kMaxX, x_ + w.speed * dt);
            speed_ = std::max(speed_, w.speed * 0.85f);
        }
        w.nose += w.speed * dt;
        if (w.alive && w.nose >= kImpactX) hitWell(w);
    }
    wagons_.erase(std::remove_if(wagons_.begin(), wagons_.end(), [](const Wagon& w) { return !w.alive; }),
                  wagons_.end());
}

void Game::updatePlay(float dt) {
    if (over_) return;
    tWave_ += dt;
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= tWave_) {
        const Spawn& s = script_[spawnAt_];
        Wagon w;
        w.lane = s.lane;
        w.kind = s.kind;
        w.nose = kSpawnX;
        w.speed = s.speed;
        w.alive = true;
        wagons_.push_back(w);
        blip(300.f + float(s.lane) * 90.f, 0.07f);
        ++spawnAt_;
    }
    float steer = 0.f;
    bool brake = false;
    if (bot_) bot(steer, brake);
    else human(steer, brake);
    moveShunter(steer, brake, dt);
    updateWagons(dt);
    if (over_) return;
    if (spawnAt_ >= int(script_.size())) {
        bool any = false;
        for (const Wagon& w : wagons_)
            if (w.alive) any = true;
        if (!any) {
            if (courses_ <= 0) lose();
            else if (wave_ >= 2) win();
            else {
                nextWave_ = wave_ + 1;
                mode_ = Mode::Banner;
                bannerT_ = 1.2f;
                buildScript(nextWave_);
                spawnAt_ = 0;
                wagons_.clear();
                blip(660.f, 0.12f);
            }
        }
    }
}

void Game::updateBanner(float dt) {
    float steer = 0.f;
    bool brake = false;
    if (bot_) bot(steer, brake);
    else human(steer, brake);
    moveShunter(steer, brake, dt);
    bannerT_ -= dt;
    if (bannerT_ <= 0.f) {
        wave_ = nextWave_;
        startWave();
        mode_ = Mode::Play;
        blip(440.f, 0.1f);
    }
}

void Game::fadeFx(float dt) {
    for (Pop& p : pops_) {
        p.t -= dt;
        p.y -= 14.f * dt;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0.f; }), pops_.end());
    for (Puff& u : puffs_) u.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& u) { return u.t <= 0.f; }), puffs_.end());
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.6f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    stamp(m, cx, cy, w, h, pal, flip);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + camX_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy + camY_ - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal & 15);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::shadowAt(float x, float y, float w) {
    if (art_.shadow.h < 1 || w < 2.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = 6;
    s.x = int16_t(std::lround(x + camX_ - s.w * 0.5f));
    s.y = int16_t(std::lround(y + camY_ - 3.f));
    s.img = art_.shadow.pick(6);
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::backdrop() {
    uint16_t top = gs::rgb4(1, 1, 5);
    uint16_t hor = gs::rgb4(10, 5, 3);
    if (mode_ == Mode::Victory) {
        top = gs::rgb4(2, 4, 8);
        hor = gs::rgb4(12, 9, 4);
    } else if (mode_ == Mode::Over) {
        top = gs::rgb4(4, 1, 2);
        hor = gs::rgb4(8, 3, 2);
    }
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float u = y < 70 ? y / 70.f : 1.f;
        v.lineBackdrop[y] = lerpC(top, hor, u);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    int lamp = 10 + int(3.f * std::fabs(std::sin(t_ * 7.f)));
    v.setColor(PAL_YARD * 16 + 10, gs::rgb4(15, std::min(15, lamp), 3));
    int water = 7 + int(2.f * std::sin(t_ * 3.f));
    v.setColor(PAL_STONE * 16 + 6, gs::rgb4(3, std::clamp(water, 0, 15), 12));
    int hot = 11 + int(3.f * std::fabs(std::sin(t_ * 6.f)));
    v.setColor(PAL_ENGINE * 16 + 6, gs::rgb4(15, std::min(15, hot), 4));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    backdrop();
    camX_ = 0.f;
    camY_ = 0.f;
    if (shake_ > 0.f) {
        camX_ = std::sin(t_ * 47.f) * shake_ * 5.f;
        camY_ = std::cos(t_ * 39.f) * shake_ * 3.f;
    }

    if (mode_ == Mode::Title) {
        text("S3 DEPOT WELL", 160, 14, 0.78f, PAL_GOLD);
        text("THREE WAVES", 160, 34, 0.46f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 14, 0.62f, PAL_GOLD);
        text("THE WATCH HELD", 160, 34, 0.46f, PAL_GOOD);
    } else if (mode_ == Mode::Over) {
        text("THE WATCH IS OVER", 160, 14, 0.58f, PAL_ALERT);
        text("THE WELL FELL", 160, 34, 0.46f, PAL_TEXT);
    } else if (mode_ == Mode::Banner) {
        char line[24];
        std::snprintf(line, sizeof line, "WAVE %d", nextWave_ + 1);
        text(line, 160, 14, 0.7f, PAL_GOLD);
        text(waveName(nextWave_), 160, 34, 0.5f, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 16, 0.8f, PAL_GOLD);
    }

    for (const Pop& p : pops_) {
        char buf[12];
        std::snprintf(buf, sizeof buf, "%d", p.pts);
        text(buf, p.x, p.y, 0.38f, PAL_GOLD);
    }
    for (const Puff& u : puffs_) {
        float k = std::clamp(1.f - u.t / 0.35f, 0.f, 1.f);
        spr(art_.puff, u.x, u.y - k * 8.f, 8.f + k * 14.f, PAL_FX, false);
    }

    float sy = laneFoot(0) + lanePos_ * (laneFoot(1) - laneFoot(0));
    int frame = int(t_ * 8.f) & 1;
    if (std::fabs(speed_) > 4.f) frame = int(t_ * 12.f) & 1;
    if (mode_ != Mode::Title) {
        spr(art_.shunter[frame], x_, sy - 16.f, 30.f, PAL_ENGINE, false);
        if (braking_ || std::fabs(speed_) > 8.f)
            spr(art_.puff, x_ + 6.f, sy - 32.f, 8.f + 4.f * std::fabs(std::sin(t_ * 14.f)), PAL_FX, false);
    }

    auto drawWagon = [&](int kind, int fr, int lane, float nose, float flash) {
        const gs::Mipped* m = art_.box;
        int pal = PAL_BOX;
        if (kind == 1) {
            m = art_.flat;
            pal = PAL_FLAT;
        } else if (kind == 2) {
            m = art_.tank;
            pal = PAL_TANK;
        }
        if (flash > 0.f) pal = PAL_FX;
        float hw = halfOf(kind);
        float h = bodyH(kind);
        float y = laneFoot(lane) - h * 0.5f;
        stamp(m[fr & 1], nose - hw, y, hw * 2.f, h, pal, false);
    };

    if (mode_ == Mode::Title) {
        float roll = std::fmod(t_ * 28.f, 220.f) - 24.f;
        drawWagon(0, frame, 0, roll, 0.f);
        drawWagon(1, frame, 2, 70.f + std::sin(t_ * 0.8f) * 24.f, 0.f);
        spr(art_.shunter[frame], kMarkX + kCoupler, laneFoot(1) - 16.f, 30.f, PAL_ENGINE, false);
    } else {
        for (const Wagon& w : wagons_) {
            if (!w.alive) continue;
            drawWagon(w.kind, int(w.anim) & 1, w.lane, w.nose, w.flash);
        }
    }

    int sig = 1;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) {
        int idx = urgent();
        if (idx >= 0) sig = wagons_[idx].lane;
        else if (spawnAt_ < int(script_.size())) sig = script_[spawnAt_].lane;
        else sig = laneTarget_;
    }
    if ((int(t_ * 6.f) & 1) == 0) spr(art_.lamp, kMarkX, laneFoot(sig) - 22.f, 8.f, PAL_FX, false);

    for (int i = 0; i < 3; ++i) spr(art_.buffer, 228.f, laneFoot(i) - 8.f, 16.f, PAL_BOX, false);

    float swing = std::sin(t_ * 1.6f) * 7.f;
    if (courses_ <= 0 || mode_ == Mode::Over) {
        spr(art_.rubble, kWellX - 8.f, kWellFoot - 18.f, 44.f, PAL_STONE, false);
    } else {
        spr(art_.well, kWellX, kWellFoot - 66.f, 132.f, PAL_STONE, false);
        int cracks = kCourses - courses_;
        for (int i = 0; i < cracks; ++i) {
            float ox = i == 0 ? -8.f : (i == 1 ? 12.f : 0.f);
            float oy = i == 0 ? -40.f : (i == 1 ? -18.f : -58.f);
            spr(art_.crack, kWellX + ox, kWellFoot + oy, 28.f, PAL_STONE, i == 1);
        }
        if (courses_ == kCourses) spr(art_.bucket, kWellX + swing, kWellFoot - 78.f, 12.f, PAL_STONE, false);
        else spr(art_.bucket, kWellX + 24.f, kWellFoot - 14.f, 12.f, PAL_STONE, false);
        for (float y = 78.f; y < 108.f; y += 7.f) spr(art_.chain, kWellX - 8.f, y, 8.f, PAL_STONE, false);
    }

    if ((int(t_ * 2.f) & 3) != 0) spr(art_.star, 250, 12, 6, PAL_SKY, false);
    if ((int(t_ * 2.f + 1.f) & 3) != 0) spr(art_.star, 300, 20, 5, PAL_SKY, false);
    spr(art_.star, 220, 16, 5, PAL_SKY, false);
    spr(art_.moon, 292, 16, 16, PAL_SKY, false);
    text("DEPOT", 78, 62, 0.42f, PAL_GOLD);

    if (mode_ != Mode::Title) {
        shadowAt(x_, sy + 2.f, 28.f);
        for (const Wagon& w : wagons_)
            if (w.alive) shadowAt(w.nose - halfOf(w.kind), laneFoot(w.lane) + 2.f, halfOf(w.kind) * 1.6f);
    } else {
        shadowAt(kMarkX + kCoupler, laneFoot(1) + 2.f, 28.f);
    }
    shadowAt(kWellX, kWellFoot - 2.f, 56.f);

    char line[48];
    if (mode_ == Mode::Title) {
        hud(1, 24, "UP DOWN CHANGES SIDING", PAL_TEXT);
        hud(1, 25, "LEFT RIGHT ROLLS THE SHUNTER", PAL_TEXT);
        hud(1, 26, "Z COUPLES AT THE MARK", PAL_GOLD);
        hud(1, 27, "ENTER TAKES THE WATCH", PAL_GOOD);
        hud(28, 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Banner) {
        int show = mode_ == Mode::Banner ? nextWave_ : wave_;
        std::snprintf(line, sizeof line, "WAVE %d/3", std::min(3, show + 1));
        hud(1, 0, line, PAL_GOLD);
        std::string pips = "WELL ";
        int left = std::max(0, courses_);
        for (int i = 0; i < kCourses; ++i) pips.push_back(i < left ? '#' : '-');
        hud(30, 0, pips, left > 1 ? PAL_GOOD : PAL_ALERT);
        std::snprintf(line, sizeof line, "NEXT %s", sidingName(sig));
        hud(1, 26, line, PAL_GOLD);
        std::snprintf(line, sizeof line, "HELD %d", held_);
        hud(28, 26, line, PAL_TEXT);
        if (mode_ == Mode::Play && tWave_ < 1.5f) hud(16, 1, waveName(wave_), PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hud(1, 0, "THE WELL STANDS", PAL_GOOD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 26, line, PAL_GOLD);
        hud(30, 26, "ENTER", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hud(1, 0, "THE WATCH IS OVER", PAL_ALERT);
        std::snprintf(line, sizeof line, "WAVE %d", wave_ + 1);
        hud(30, 0, line, PAL_TEXT);
        hud(1, 26, "ENTER TRIES AGAIN", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.84f);
    sys.apu.setEcho(0.16f, 0.24f, 0.14f);
    sys.apu.setPatch(0, dronePatch());
    sys.apu.setPatch(1, clankPatch());
    sys.apu.setPatch(2, whistlePatch());
    sys.apu.keyOn(0, 72.f, 0.035f);
    droneWatch_ = false;
    if (bot_) begin();
    else {
        mode_ = Mode::Title;
        sys.setLight(50, 32, 14);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    bool watch = mode_ == Mode::Play || mode_ == Mode::Banner;
    if (watch != droneWatch_) {
        droneWatch_ = watch;
        sys.apu.setFreq(0, watch ? 46.f : 72.f);
        sys.apu.setVol(0, watch ? 0.05f : 0.032f);
    }

    if (mode_ == Mode::Title) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updatePlay(kDt);
    } else if (mode_ == Mode::Banner) {
        updateBanner(kDt);
    } else {
        if (mode_ == Mode::Victory) {
            fanT_ += kDt;
            if (fanStep_ >= 0 && fanT_ > 0.16f) {
                static const float notes[] = {262.f, 330.f, 392.f, 523.f, 392.f, 523.f};
                if (fanStep_ < 6) sys.apu.keyOn(2, notes[fanStep_], 0.15f);
                else sys.apu.keyOff(2);
                ++fanStep_;
                fanT_ = 0.f;
                if (fanStep_ > 8) fanStep_ = -1;
            }
        }
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            if (mode_ == Mode::Over) begin();
            else toTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) toTitle();
    }
    fadeFx(kDt);
    draw();
}

}  // namespace depotwell
