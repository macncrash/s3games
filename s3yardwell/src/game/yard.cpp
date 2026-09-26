#include "game/yard.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace yardwell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStop = 30.f;
constexpr float kSolid = 20.f;
constexpr float kReach = 46.f;
constexpr float kSpeed = 142.f;
constexpr int kCourses = 3;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int c0, int c1) { return int(std::lround(c0 + (c1 - c0) * t)); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

gs::FMPatch dronePatch() {
    gs::FMPatch p{};
    p.alg = 4;
    p.fb = 0.2f;
    p.op[0] = {1.f, 0.62f, 0.4f, 1.0f, 0.75f, 0.5f};
    p.op[1] = {2.f, 0.2f, 0.3f, 0.7f, 0.45f, 0.4f};
    p.op[2] = {0.5f, 0.38f, 0.35f, 1.1f, 0.6f, 0.45f};
    p.op[3] = {3.f, 0.1f, 0.2f, 0.5f, 0.28f, 0.35f};
    p.vol = 0.13f;
    p.tone = 640.f;
    p.drive = 0.04f;
    return p;
}

gs::FMPatch hitPatch() {
    gs::FMPatch p{};
    p.alg = 7;
    p.fb = 0.16f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.08f, 0.f, 0.05f};
    p.op[1] = {2.2f, 0.4f, 0.004f, 0.07f, 0.f, 0.05f};
    p.op[2] = {0.5f, 0.28f, 0.006f, 0.1f, 0.f, 0.07f};
    p.op[3] = {3.5f, 0.16f, 0.005f, 0.06f, 0.f, 0.05f};
    p.vol = 0.2f;
    p.tone = 1600.f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p{};
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.02f, 0.16f, 0.5f, 0.12f};
    p.op[1] = {2.f, 0.32f, 0.02f, 0.18f, 0.32f, 0.12f};
    p.op[2] = {3.f, 0.16f, 0.03f, 0.2f, 0.22f, 0.14f};
    p.op[3] = {1.f, 0.24f, 0.02f, 0.18f, 0.35f, 0.12f};
    p.vol = 0.18f;
    return p;
}

float bashLimit(int kind) {
    if (kind >= 2) return 1.45f;
    if (kind == 1) return 1.05f;
    return 0.9f;
}

int hpFor(int kind) {
    if (kind >= 2) return 4;
    if (kind == 1) return 2;
    return 1;
}

int pointsFor(int kind) {
    if (kind >= 2) return 700;
    if (kind == 1) return 250;
    return 100;
}

void gatePoint(int side, float along, float& x, float& y) {
    // 0 N, 1 E, 2 S, 3 W, 4 NE, 5 NW, 6 SE, 7 SW
    switch (side) {
    case 0: x = kWellX + along; y = 56.f; break;
    case 1: x = 292.f; y = kWellY + along; break;
    case 2: x = kWellX + along; y = 196.f; break;
    case 3: x = 30.f; y = kWellY + along; break;
    case 4: x = 268.f; y = 58.f; break;
    case 5: x = 52.f; y = 58.f; break;
    case 6: x = 268.f; y = 192.f; break;
    case 7: x = 52.f; y = 192.f; break;
    default: x = kWellX; y = 56.f; break;
    }
    x = std::clamp(x, 36.f, 284.f);
    y = std::clamp(y, 58.f, 194.f);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) return 1;
    return 0;
}

const char* Game::waveName(int w) const {
    if (w <= 0) return "MOLES";
    if (w == 1) return "GOATS";
    return "THE BULL";
}

float Game::zoom(float y) const {
    float u = std::clamp((y - 52.f) / 150.f, 0.f, 1.f);
    return 0.86f + u * 0.22f;
}

void Game::blip(float freq, float vol) { sys_->apu.keyOn(1, freq, vol); }

void Game::puffAt(float x, float y) {
    Puff u;
    u.x = x;
    u.y = y;
    u.t = 0.32f;
    puffs_.push_back(u);
}

float Game::eta(const Pest& p) const {
    if (p.bashing) return -1.f + (bashLimit(p.kind) - p.bash) * 0.01f;
    float dw = std::hypot(p.x - kWellX, p.y - kWellY);
    return std::max(0.f, dw - kStop) / std::max(8.f, p.speed);
}

int Game::findId(int id) const {
    if (id < 0) return -1;
    for (int i = 0; i < int(pests_.size()); ++i)
        if (pests_[i].id == id && pests_[i].alive) return i;
    return -1;
}

int Game::nearest() const {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < int(pests_.size()); ++i) {
        if (!pests_[i].alive) continue;
        float d = std::hypot(pests_[i].x - x_, pests_[i].y - y_);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::bootAudio() {
    sys_->apu.setMaster(0.82f);
    sys_->apu.setEcho(0.14f, 0.22f, 0.12f);
    sys_->apu.setPatch(0, dronePatch());
    sys_->apu.setPatch(1, hitPatch());
    sys_->apu.setPatch(2, hornPatch());
    humFreq_ = -1.f;
}

void Game::humForMode() {
    float f = 146.f, v = 0.04f;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) {
        f = 92.f;
        v = 0.05f;
    } else if (mode_ == Mode::Over) {
        f = 49.f;
        v = 0.06f;
    } else if (mode_ == Mode::Victory) {
        f = 196.f;
        v = 0.05f;
    }
    if (f == humFreq_) return;
    humFreq_ = f;
    sys_->apu.keyOn(0, f, v);
}

void Game::buildScript(int wave) {
    script_.clear();
    auto add = [&](float t, int side, float along, int kind, float speed) {
        Spawn s;
        s.t = t;
        s.side = side;
        s.along = along;
        s.kind = kind;
        s.speed = speed;
        script_.push_back(s);
    };
    if (wave <= 0) {
        add(0.35f, 1, 0, 0, 26);
        add(1.20f, 5, 0, 0, 24);
        add(3.15f, 3, 0, 0, 26);
        add(5.05f, 4, 0, 0, 24);
        add(6.95f, 7, 0, 0, 24);
        add(8.85f, 0, 0, 0, 16);
        add(10.70f, 6, 0, 0, 26);
    } else if (wave == 1) {
        add(0.45f, 1, 8, 1, 24);
        add(2.55f, 5, 0, 1, 22);
        add(4.65f, 2, 0, 1, 15);
        add(6.85f, 3, -6, 1, 24);
        add(9.00f, 4, 0, 1, 22);
        add(11.15f, 6, 0, 1, 24);
    } else {
        add(0.40f, 3, 0, 0, 28);
        add(2.20f, 1, 0, 0, 28);
        add(4.10f, 4, 0, 1, 22);
        add(6.20f, 7, 0, 1, 22);
        add(8.40f, 2, 0, 2, 13);
        add(11.40f, 5, 0, 0, 26);
        add(13.20f, 6, 0, 0, 26);
    }
}

void Game::spawnOne(const Spawn& s) {
    Pest p;
    p.id = nextId_++;
    p.kind = s.kind;
    p.hp = hpFor(s.kind);
    p.speed = s.speed;
    p.alive = true;
    gatePoint(s.side, s.along, p.x, p.y);
    pests_.push_back(p);
    puffAt(p.x, p.y - 8.f);
    if (s.kind >= 2) blip(98.f, 0.1f);
}

void Game::startWave() {
    pests_.clear();
    spawnAt_ = 0;
    tWave_ = 0;
    focus_ = -1;
    buildScript(wave_);
    blip(wave_ == 0 ? 440.f : 523.f, 0.1f);
}

void Game::beginRun() {
    wave_ = 0;
    nextWave_ = 0;
    courses_ = kCourses;
    breaches_ = 0;
    score_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "UNFINISHED";
    focus_ = -1;
    swingCd_ = 0;
    swingT_ = 0;
    swingSerial_ = 1;
    shake_ = 0;
    endT_ = 0;
    fanStep_ = -1;
    fanT_ = 0;
    faceLeft_ = false;
    moving_ = false;
    x_ = kWellX;
    y_ = 176.f;
    pests_.clear();
    pops_.clear();
    puffs_.clear();
    mode_ = Mode::Play;
    humFreq_ = -1.f;
    startWave();
    sys_->setLight(90, 64, 24);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "UNFINISHED";
    courses_ = kCourses;
    breaches_ = 0;
    score_ = 0;
    pests_.clear();
    pops_.clear();
    puffs_.clear();
    fanStep_ = -1;
    sys_->setLight(48, 72, 28);
}

void Game::lose() {
    if (over_) return;
    courses_ = 0;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    reason_ = "THE WELL FELL";
    shake_ = 1.f;
    fanStep_ = -1;
    sys_->rumble(1.f, 1.f, 280);
    sys_->apu.noiseBurst(0.62f, 180.f, 0.24f);
    blip(62.f, 0.18f);
    sys_->setLight(110, 24, 16);
}

void Game::win() {
    if (over_ || courses_ <= 0) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE WELL STANDS";
    score_ += courses_ * 300;
    fanStep_ = 0;
    fanT_ = 0;
    shake_ = 0.12f;
    sys_->setLight(40, 100, 36);
    blip(523.f, 0.16f);
    for (int i = 0; i < 6; ++i) {
        float a = i * 1.0472f;
        puffAt(kWellX + std::cos(a) * 28.f, kWellY - 18.f + std::sin(a) * 10.f);
    }
}

void Game::kill(Pest& p) {
    int pts = pointsFor(p.kind);
    score_ += pts;
    p.alive = false;
    Pop pop;
    pop.x = p.x;
    pop.y = p.y - 30.f;
    pop.t = 0.75f;
    pop.pts = pts;
    pops_.push_back(pop);
    puffAt(p.x, p.y - 12.f);
}

void Game::breach(Pest& p) {
    p.alive = false;
    ++breaches_;
    if (courses_ > 0) --courses_;
    shake_ = std::max(shake_, 0.8f);
    puffAt(kWellX, kWellY - 20.f);
    Pop pop;
    pop.x = kWellX;
    pop.y = kWellY - 56.f;
    pop.t = 0.85f;
    pop.pts = 0;
    pops_.push_back(pop);
    sys_->rumble(0.85f, 1.f, 200);
    sys_->apu.noiseBurst(0.55f, 220.f, 0.18f);
    blip(70.f, 0.16f);
    if (courses_ <= 0) lose();
}

void Game::hurt(Pest& p) {
    p.hp -= 1;
    p.flash = 0.1f;
    p.bash = 0.f;
    p.bashing = false;
    float vx = p.x - kWellX, vy = p.y - kWellY;
    float d = std::hypot(vx, vy);
    if (d < 0.001f) {
        vx = 0;
        vy = 1;
        d = 1;
    }
    if (d < kStop + 18.f) {
        p.x = kWellX + vx / d * (kStop + 18.f);
        p.y = kWellY + vy / d * (kStop + 18.f);
    } else {
        p.x += vx / d * 14.f;
        p.y += vy / d * 14.f;
    }
    p.x = std::clamp(p.x, 36.f, 284.f);
    p.y = std::clamp(p.y, 58.f, 194.f);
    float ox = p.x - kWellX, oy = p.y - kWellY;
    float od = std::hypot(ox, oy);
    if (od < kStop) {
        if (od < 0.001f) {
            ox = 0;
            oy = 1;
            od = 1;
        }
        p.x = kWellX + ox / od * (kStop + 8.f);
        p.y = kWellY + oy / od * (kStop + 8.f);
    }
    if (p.hp <= 0) kill(p);
}

void Game::swing() {
    if (mode_ != Mode::Play || swingCd_ > 0.f) return;
    swingCd_ = 0.15f;
    swingT_ = 0.12f;
    ++swingSerial_;
    bool hit = false;
    for (Pest& p : pests_) {
        if (!p.alive || p.hitSerial == swingSerial_) continue;
        if (std::hypot(p.x - x_, p.y - y_) > kReach) continue;
        p.hitSerial = swingSerial_;
        hit = true;
        hurt(p);
    }
    shake_ = std::max(shake_, hit ? 0.16f : 0.04f);
    sys_->rumble(hit ? 0.3f : 0.08f, hit ? 0.5f : 0.12f, hit ? 40 : 18);
    sys_->apu.noiseBurst(hit ? 0.26f : 0.07f, hit ? 1300.f : 640.f, 0.04f);
    blip(hit ? 310.f : 170.f, hit ? 0.12f : 0.04f);
}

void Game::nudge(float vx, float vy, float dt) {
    float len = std::hypot(vx, vy);
    if (len < 8.f) return;
    moving_ = true;
    if (vx < -4.f) faceLeft_ = true;
    else if (vx > 4.f) faceLeft_ = false;
    float mx = vx / len, my = vy / len;
    float ox = x_, oy = y_;
    float nx = ox + mx * len * dt;
    float ny = oy + my * len * dt;
    auto shove = [](float& x, float& y) {
        float dx = x - kWellX, dy = y - kWellY;
        float d = std::hypot(dx, dy);
        if (d >= kSolid) return;
        if (d < 0.001f) {
            x = kWellX;
            y = kWellY + kSolid;
            return;
        }
        x = kWellX + dx / d * kSolid;
        y = kWellY + dy / d * kSolid;
    };
    float dx = nx - kWellX, dy = ny - kWellY;
    if (std::hypot(dx, dy) < kSolid) {
        float t1x = -dy, t1y = dx;
        float t2x = dy, t2y = -dx;
        float l1 = std::hypot(t1x, t1y);
        float l2 = std::hypot(t2x, t2y);
        if (l1 > 0.001f) {
            t1x /= l1;
            t1y /= l1;
        }
        if (l2 > 0.001f) {
            t2x /= l2;
            t2y /= l2;
        }
        float s1 = t1x * mx + t1y * my;
        float s2 = t2x * mx + t2y * my;
        float tx = s1 >= s2 ? t1x : t2x;
        float ty = s1 >= s2 ? t1y : t2y;
        nx = ox + tx * len * dt;
        ny = oy + ty * len * dt;
        shove(nx, ny);
    }
    nx = std::clamp(nx, 36.f, 284.f);
    ny = std::clamp(ny, 60.f, 190.f);
    shove(nx, ny);
    x_ = nx;
    y_ = ny;
}

void Game::botAct(float dt) {
    int best = -1;
    float bestT = 1e9f;
    for (int i = 0; i < int(pests_.size()); ++i) {
        if (!pests_[i].alive) continue;
        float e = eta(pests_[i]);
        if (e < bestT) {
            bestT = e;
            best = i;
        }
    }
    int cur = findId(focus_);
    if (cur >= 0 && best >= 0) {
        float d = std::hypot(pests_[cur].x - x_, pests_[cur].y - y_);
        if (d < 64.f && eta(pests_[cur]) < bestT + 0.5f) best = cur;
    }
    float tx = kWellX, ty = 176.f;
    if (best >= 0) {
        focus_ = pests_[best].id;
        tx = pests_[best].x;
        ty = pests_[best].y;
    } else {
        focus_ = -1;
    }
    float dx = tx - x_, dy = ty - y_;
    float d = std::hypot(dx, dy);
    if (d > 4.f) nudge(dx / d * kSpeed, dy / d * kSpeed, dt);
    if (best >= 0 && std::hypot(pests_[best].x - x_, pests_[best].y - y_) <= kReach + 4.f)
        faceLeft_ = pests_[best].x < x_;
    bool near = false;
    for (const Pest& p : pests_) {
        if (!p.alive) continue;
        if (std::hypot(p.x - x_, p.y - y_) <= kReach) near = true;
    }
    if (near && mode_ == Mode::Play) swing();
}

void Game::human(float dt) {
    const gs::Pad& p = sys_->pad;
    float ax = 0, ay = 0;
    if (p.down(gs::BTN_LEFT)) ax -= 1;
    if (p.down(gs::BTN_RIGHT)) ax += 1;
    if (p.down(gs::BTN_UP)) ay -= 1;
    if (p.down(gs::BTN_DOWN)) ay += 1;
    bool digital = ax != 0.f || ay != 0.f;
    if (!digital) {
        ax = p.axisX;
        ay = -p.axisY;
        float len = std::hypot(ax, ay);
        if (len < 0.18f) {
            ax = 0;
            ay = 0;
        } else if (len > 1.f) {
            ax /= len;
            ay /= len;
        }
    } else {
        float len = std::hypot(ax, ay);
        ax /= len;
        ay /= len;
    }
    if (ax != 0.f || ay != 0.f) nudge(ax * kSpeed, ay * kSpeed, dt);
    else {
        int n = nearest();
        if (n >= 0) faceLeft_ = pests_[n].x + 1.f < x_;
    }
    bool attack = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) ||
                  p.down(gs::BTN_TURBO) || p.accel > 0.45f;
    if (attack) swing();
}

void Game::fadeFx(float dt) {
    for (Pop& p : pops_) {
        p.t -= dt;
        p.y -= 16.f * dt;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0.f; }), pops_.end());
    for (Puff& u : puffs_) u.t -= dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& u) { return u.t <= 0.f; }), puffs_.end());
    for (Pest& p : pests_)
        if (p.flash > 0.f) p.flash -= dt;
    if (swingCd_ > 0.f) swingCd_ = std::max(0.f, swingCd_ - dt);
    if (swingT_ > 0.f) swingT_ = std::max(0.f, swingT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.8f);
}

void Game::updatePlay(float dt) {
    tWave_ += dt;
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= tWave_) {
        spawnOne(script_[spawnAt_]);
        ++spawnAt_;
    }
    if (bot_) botAct(dt);
    else human(dt);
    for (Pest& p : pests_) {
        if (!p.alive) continue;
        float vx = p.x - kWellX, vy = p.y - kWellY;
        float d = std::hypot(vx, vy);
        if (d < 0.001f) {
            vx = 0;
            vy = 1;
            d = 1;
        }
        if (d <= kStop) {
            p.bashing = true;
            p.x = kWellX + vx / d * kStop;
            p.y = kWellY + vy / d * kStop;
            p.bash += dt;
            p.anim += dt * 14.f;
            if (p.bash >= bashLimit(p.kind)) {
                breach(p);
                if (over_) return;
            }
        } else {
            p.bashing = false;
            float step = p.speed * dt;
            if (d - step <= kStop) {
                p.x = kWellX + vx / d * kStop;
                p.y = kWellY + vy / d * kStop;
                p.bashing = true;
            } else {
                p.x -= vx / d * step;
                p.y -= vy / d * step;
            }
            p.anim += dt * 8.f;
        }
    }
    if (over_) return;
    pests_.erase(std::remove_if(pests_.begin(), pests_.end(), [](const Pest& p) { return !p.alive; }), pests_.end());
    if (spawnAt_ >= int(script_.size()) && pests_.empty()) {
        if (courses_ <= 0) lose();
        else if (wave_ >= 2) win();
        else {
            nextWave_ = wave_ + 1;
            mode_ = Mode::Banner;
            bannerT_ = 1.25f;
            blip(660.f, 0.1f);
        }
    }
}

void Game::updateBanner(float dt) {
    if (bot_) botAct(dt);
    else human(dt);
    bannerT_ -= dt;
    if (bannerT_ <= 0.f) {
        wave_ = nextWave_;
        startWave();
        mode_ = Mode::Play;
    }
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (w < 1.f || h < 1.f || m.h < 1 || m.w < 1) return;
    gs::Sprite s;
    int sw = std::clamp(int(std::lround(w)), 1, 2000);
    int sh = std::clamp(int(std::lround(h)), 1, 2000);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(int(std::lround(cx + camX_ - sw * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(cy + camY_ - sh * 0.5f)), -2000, 2000));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal & 15);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.5f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (feet) cy -= h * 0.5f;
    stamp(m, cx, cy, w, h, pal, flip);
}

void Game::shadowAt(float x, float y, float w) {
    if (w < 3.f || art_.shadow.h < 1) return;
    gs::Sprite s;
    int sw = std::clamp(int(std::lround(w)), 1, 400);
    s.w = int16_t(sw);
    s.h = 8;
    s.x = int16_t(std::clamp(int(std::lround(x + camX_ - sw * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(y + camY_ - 4.f)), -2000, 2000));
    s.img = art_.shadow.pick(8);
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 15.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27 || art_.panel <= 0) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39) continue;
        if (c < 32 || c >= 128 || c == ' ') sys_->vdp.HUD.set(x, row, gs::entry(art_.panel, pal));
        else sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::backdrop() {
    uint16_t top = gs::rgb4(5, 9, 14);
    uint16_t hor = gs::rgb4(14, 11, 7);
    if (mode_ == Mode::Victory) {
        top = gs::rgb4(7, 11, 15);
        hor = gs::rgb4(15, 13, 8);
    } else if (mode_ == Mode::Over) {
        top = gs::rgb4(5, 2, 3);
        hor = gs::rgb4(9, 4, 3);
    }
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float u = std::clamp(y / 44.f, 0.f, 1.f);
        u = u * u * (3.f - 2.f * u);
        v.lineBackdrop[y] = lerpC(top, hor, u);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    int water = std::clamp(7 + int(std::lround(2.f * std::sin(t_ * 2.4f))), 0, 15);
    v.setColor(PAL_WELL * 16 + 7, gs::rgb4(2, 7, water));
    int hi = std::clamp(12 + int(std::lround(1.5f * std::sin(t_ * 3.1f))), 0, 15);
    v.setColor(PAL_WELL * 16 + 8, gs::rgb4(7, hi, 15));
    int sun = std::clamp(11 + int(std::lround(1.5f * std::sin(t_ * 1.3f))), 0, 15);
    v.setColor(PAL_SKY * 16 + 3, gs::rgb4(15, sun, 3));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    camX_ = 0;
    camY_ = 0;
    if (shake_ > 0.f) {
        camX_ = std::sin(t_ * 47.f) * shake_ * 5.f;
        camY_ = std::cos(t_ * 39.f) * shake_ * 3.f;
    }

    if (mode_ == Mode::Title) {
        text("S3 YARD WELL", 160, 13, 0.78f, PAL_GOLD);
        text("THE WELL MUST STAND", 160, 30, 0.44f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 14, 0.64f, PAL_GOLD);
        text("THREE WAVES", 160, 32, 0.46f, PAL_GOOD);
    } else if (mode_ == Mode::Over) {
        text("THE WELL FELL", 160, 14, 0.7f, PAL_ALERT);
        text("THE YARD IS LOST", 160, 32, 0.44f, PAL_TEXT);
    } else if (mode_ == Mode::Banner) {
        char line[16];
        std::snprintf(line, sizeof line, "WAVE %d", nextWave_ + 1);
        text(line, 160, 14, 0.7f, PAL_GOLD);
        text(waveName(nextWave_), 160, 32, 0.5f, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 14, 0.8f, PAL_GOLD);
    }

    for (const Pop& p : pops_) {
        if (p.pts <= 0) text("CRACK", p.x, p.y, 0.4f, PAL_ALERT);
        else {
            char buf[12];
            std::snprintf(buf, sizeof buf, "%d", p.pts);
            text(buf, p.x, p.y, 0.4f, PAL_GOLD);
        }
    }
    for (const Puff& u : puffs_) {
        float k = 1.f - std::clamp(u.t / 0.32f, 0.f, 1.f);
        spr(art_.puff, u.x, u.y, 8.f + k * 16.f, PAL_FX, false, false);
    }

    float kx = x_, ky = y_;
    bool flip = faceLeft_;
    int kframe = 0;
    if (mode_ == Mode::Title) {
        float a = t_ * 0.85f;
        kx = kWellX + std::cos(a) * 54.f;
        ky = kWellY + std::sin(a) * 34.f;
        flip = std::sin(a) > 0.f;
        kframe = int(t_ * 6.f) & 1;
    } else if (mode_ == Mode::Victory) {
        ky -= std::fabs(std::sin(t_ * 7.f)) * 6.f;
        kframe = int(t_ * 8.f) & 1;
    } else if (swingT_ > 0.f) {
        kframe = 2;
    } else if (moving_) {
        kframe = int(t_ * 8.f) & 1;
    }
    float kh = 40.f * zoom(ky);
    if (swingT_ > 0.f) {
        float dir = flip ? -1.f : 1.f;
        spr(art_.whoosh, kx + dir * 18.f, ky - 18.f, 16.f, PAL_FX, flip, false);
    }
    spr(art_.keeper[kframe], kx, ky, kh, PAL_KEEPER, flip, true);
    shadowAt(kx, ky, kh * 0.7f);

    auto drawPest = [&](int kind, int frame, float x, float y, float flash, bool left) {
        const gs::Mipped* m = art_.mole;
        int pal = PAL_MOLE;
        float h = 28.f;
        if (kind == 1) {
            m = art_.goat;
            pal = PAL_GOAT;
            h = 36.f;
        } else if (kind >= 2) {
            m = art_.bull;
            pal = PAL_BULL;
            h = 50.f;
        }
        if (flash > 0.f) pal = PAL_FX;
        h *= zoom(y);
        spr(m[frame & 1], x, y, h, pal, left, true);
        shadowAt(x, y, h * 0.7f);
        return h;
    };

    if (mode_ == Mode::Title) {
        float mx = kWellX + std::sin(t_ * 0.8f) * 30.f;
        drawPest(0, int(t_ * 6.f) & 1, mx, 186.f, 0, std::cos(t_ * 0.8f) > 0.f);
        drawPest(1, int(t_ * 4.f) & 1, 72.f, 162.f, 0, false);
        drawPest(2, int(t_ * 3.f) & 1, 248.f, 160.f, 0, true);
    } else {
        std::vector<int> ord;
        for (int i = 0; i < int(pests_.size()); ++i)
            if (pests_[i].alive) ord.push_back(i);
        std::sort(ord.begin(), ord.end(), [&](int a, int b) { return pests_[a].y > pests_[b].y; });
        for (int i : ord) {
            const Pest& p = pests_[i];
            bool left = p.x < kWellX;
            float h = drawPest(p.kind, int(p.anim) & 1, p.x, p.y, p.flash, left);
            if (p.bashing) {
                float frac = std::clamp(p.bash / bashLimit(p.kind), 0.f, 1.f);
                float bw = std::max(3.f, 26.f * frac);
                stamp(art_.bar, p.x - 13.f + bw * 0.5f, p.y - h - 4.f, bw, 4.f, PAL_ALERT, false);
            }
        }
    }

    float wellH = 76.f * zoom(kWellY);
    if (courses_ <= 0 || mode_ == Mode::Over) {
        spr(art_.rubble, kWellX, kWellY, 48.f * zoom(kWellY), PAL_WELL, false, true);
    } else {
        spr(art_.well, kWellX, kWellY, wellH, PAL_WELL, false, true);
        if (courses_ <= 2) spr(art_.crack, kWellX - 8.f, kWellY - 28.f, 28.f, PAL_ALERT, false, false);
        if (courses_ <= 1) spr(art_.crack, kWellX + 10.f, kWellY - 16.f, 32.f, PAL_ALERT, true, false);
    }
    shadowAt(kWellX, kWellY, 54.f);

    float zTree = zoom(102.f);
    spr(art_.tree, 262.f, 102.f, 62.f * zTree, PAL_PROP, false, true);
    shadowAt(262.f, 102.f, 36.f);
    spr(art_.shed, 64.f, 118.f, 44.f * zoom(118.f), PAL_PROP, false, true);
    shadowAt(64.f, 118.f, 34.f);
    spr(art_.rope, 90.f, 66.f, 16.f, PAL_WELL, false, false);
    float sway = std::sin(t_ * 2.2f) * 2.f;
    spr(art_.shirt, 72.f, 80.f + sway, 16.f, PAL_PROP, false, true);
    spr(art_.shirt, 108.f, 82.f - sway, 16.f, PAL_PROP, true, true);
    spr(art_.can, 108.f, 128.f, 18.f * zoom(128.f), PAL_PROP, false, true);
    spr(art_.barrow, 270.f, 184.f, 22.f * zoom(184.f), PAL_PROP, false, true);
    spr(art_.bush, 44.f, 170.f, 18.f * zoom(170.f), PAL_PROP, false, true);
    spr(art_.bush, 214.f, 176.f, 16.f * zoom(176.f), PAL_PROP, true, true);

    float drift = std::sin(t_ * 0.25f) * 8.f;
    spr(art_.cloud, 46.f + drift, 16.f, 18.f, PAL_SKY, false, false);
    spr(art_.cloud, 168.f - drift, 11.f, 24.f, PAL_SKY, false, false);
    spr(art_.cloud, 248.f + drift * 0.5f, 18.f, 16.f, PAL_SKY, false, false);
    spr(art_.sun, 296.f, 14.f, 22.f, PAL_SKY, false, false);
    float bx = 36.f + std::fmod(t_ * 22.f, 250.f);
    spr(art_.bird[int(t_ * 8.f) & 1], bx, 18.f + std::sin(t_ * 1.7f) * 3.f, 12.f, PAL_SKY, false, false);

    char line[40];
    if (mode_ == Mode::Title) {
        hud(6, 23, "MOLES   GOATS   THE BULL", PAL_GOLD);
        hud(1, 24, "ARROWS MOVE IN THE YARD", PAL_TEXT);
        hud(1, 25, "Z OR SPACE SWINGS THE RAKE", PAL_GOLD);
        hud(1, 26, "ENTER KEEPS THE WATCH", PAL_GOOD);
        hud(22, 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Banner) {
        std::snprintf(line, sizeof line, "WAVE %d/3", std::min(3, wave_ + 1));
        hud(1, 0, line, PAL_GOLD);
        std::string pips = "WELL ";
        int show = std::max(0, courses_);
        for (int i = 0; i < kCourses; ++i) pips.push_back(i < show ? '#' : '-');
        hud(29, 0, pips, show > 1 ? PAL_GOOD : PAL_ALERT);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_TEXT);
        if (mode_ == Mode::Play && wave_ == 0 && tWave_ < 2.4f) hud(16, 27, "Z SWINGS", PAL_GOLD);
        else if (mode_ == Mode::Play && tWave_ < 1.6f) hud(28, 27, waveName(wave_), PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        hud(1, 0, "THE WELL STANDS", PAL_GOOD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_GOLD);
        hud(30, 27, "ENTER", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hud(1, 0, "THE WELL FELL", PAL_ALERT);
        std::snprintf(line, sizeof line, "WAVE %d", wave_ + 1);
        hud(30, 0, line, PAL_TEXT);
        hud(1, 27, "ENTER TRIES AGAIN", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootAudio();
    courses_ = kCourses;
    x_ = kWellX;
    y_ = 176.f;
    faceLeft_ = false;
    if (bot_) beginRun();
    else {
        mode_ = Mode::Title;
        sys.setLight(48, 72, 28);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    moving_ = false;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && pad.pressed(gs::BTN_START)) beginRun();
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
        endT_ += kDt;
        if (mode_ == Mode::Victory && fanStep_ >= 0) {
            fanT_ += kDt;
            if (fanT_ > 0.15f) {
                static const float notes[] = {392.f, 494.f, 587.f, 784.f, 659.f, 784.f};
                if (fanStep_ < 6) sys.apu.keyOn(2, notes[fanStep_], 0.16f);
                else sys.apu.keyOff(2);
                ++fanStep_;
                fanT_ = 0;
                if (fanStep_ > 8) fanStep_ = -1;
            }
        }
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            if (mode_ == Mode::Over) beginRun();
            else toTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            toTitle();
        }
    }
    humForMode();
    fadeFx(kDt);
    draw();
}

}  // namespace yardwell
