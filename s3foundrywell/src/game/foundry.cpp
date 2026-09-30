#include "game/foundry.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace foundrywell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kRim = 32.f;
constexpr float kSolid = 22.f;
constexpr float kReach = 50.f;
constexpr float kSpeed = 148.f;
constexpr int kRings = 3;

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [&](int s, int d) { return int(std::lround(s + (d - s) * t)); };
    return gs::rgb4(ch((a >> 8) & 15, (b >> 8) & 15), ch((a >> 4) & 15, (b >> 4) & 15), ch(a & 15, b & 15));
}

gs::FMPatch humPatch() {
    gs::FMPatch p{};
    p.alg = 4;
    p.fb = 0.25f;
    p.op[0] = {1.f, 0.7f, 0.35f, 0.9f, 0.7f, 0.45f};
    p.op[1] = {2.f, 0.18f, 0.3f, 0.6f, 0.4f, 0.4f};
    p.op[2] = {0.5f, 0.32f, 0.4f, 1.0f, 0.55f, 0.4f};
    p.op[3] = {3.f, 0.08f, 0.2f, 0.4f, 0.2f, 0.3f};
    p.vol = 0.12f;
    p.tone = 520.f;
    p.drive = 0.08f;
    return p;
}

gs::FMPatch hitPatch() {
    gs::FMPatch p{};
    p.alg = 7;
    p.fb = 0.12f;
    p.op[0] = {1.f, 1.f, 0.004f, 0.07f, 0.f, 0.05f};
    p.op[1] = {2.4f, 0.35f, 0.004f, 0.06f, 0.f, 0.04f};
    p.op[2] = {0.5f, 0.22f, 0.006f, 0.09f, 0.f, 0.06f};
    p.op[3] = {4.f, 0.12f, 0.004f, 0.05f, 0.f, 0.04f};
    p.vol = 0.2f;
    p.tone = 1400.f;
    return p;
}

float chewTime(int kind) { return kind >= 2 ? 1.55f : kind == 1 ? 1.15f : 0.95f; }
int hpFor(int kind) { return kind >= 2 ? 3 : kind == 1 ? 2 : 1; }
int pointsFor(int kind) { return kind >= 2 ? 800 : kind == 1 ? 280 : 120; }

void gate(int side, float skew, float& x, float& y) {
    switch (side) {
    case 0: x = kWellX + skew; y = 54.f; break;
    case 1: x = 300.f; y = kWellY + skew; break;
    case 2: x = kWellX + skew; y = 200.f; break;
    case 3: x = 22.f; y = kWellY + skew; break;
    case 4: x = 276.f; y = 56.f; break;
    case 5: x = 44.f; y = 56.f; break;
    case 6: x = 276.f; y = 196.f; break;
    default: x = 44.f; y = 196.f; break;
    }
    x = std::clamp(x, 28.f, 292.f);
    y = std::clamp(y, 52.f, 200.f);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Over) return 4;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) return 1;
    return 0;
}

const char* Game::waveName(int w) const {
    if (w <= 0) return "SPARKS";
    if (w == 1) return "SLAG";
    return "THE POUR";
}

float Game::depth(float y) const {
    float u = std::clamp((y - 48.f) / 156.f, 0.f, 1.f);
    return 0.84f + u * 0.24f;
}

void Game::blip(float freq, float vol) { sys_->apu.keyOn(1, freq, vol); }

void Game::ashAt(float x, float y) {
    ash_.push_back(Ash{x, y, 0.34f});
}

float Game::urgency(const Beast& b) const {
    if (b.chewing) return -1.f + (chewTime(b.kind) - b.chew) * 0.01f;
    float d = std::hypot(b.x - kWellX, b.y - kWellY);
    return std::max(0.f, d - kRim) / std::max(8.f, b.speed);
}

int Game::byId(int id) const {
    if (id < 0) return -1;
    for (int i = 0; i < int(beasts_.size()); ++i)
        if (beasts_[i].id == id && beasts_[i].live) return i;
    return -1;
}

int Game::closest() const {
    int best = -1;
    float bd = 1e9f;
    for (int i = 0; i < int(beasts_.size()); ++i) {
        if (!beasts_[i].live) continue;
        float d = std::hypot(beasts_[i].x - x_, beasts_[i].y - y_);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

void Game::bootAudio() {
    sys_->apu.setMaster(0.8f);
    sys_->apu.setEcho(0.12f, 0.18f, 0.1f);
    sys_->apu.setPatch(0, humPatch());
    sys_->apu.setPatch(1, hitPatch());
    droneHz_ = -1.f;
}

void Game::drone() {
    float f = 110.f, v = 0.045f;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) {
        f = 73.f;
        v = 0.055f;
    } else if (mode_ == Mode::Over) {
        f = 42.f;
        v = 0.06f;
    } else if (mode_ == Mode::Victory) {
        f = 164.f;
        v = 0.05f;
    }
    if (f == droneHz_) return;
    droneHz_ = f;
    sys_->apu.keyOn(0, f, v);
}

void Game::scriptWave(int wave) {
    script_.clear();
    auto add = [&](float t, int gate, float skew, int kind, float speed) {
        script_.push_back(Cue{t, gate, skew, kind, speed});
    };
    if (wave <= 0) {
        add(0.4f, 2, -40, 0, 26);
        add(2.6f, 1, 10, 0, 28);
        add(4.8f, 3, -8, 0, 26);
        add(7.0f, 0, 30, 0, 30);
        add(9.2f, 6, 0, 0, 24);
    } else if (wave == 1) {
        add(0.35f, 5, 0, 1, 24);
        add(2.7f, 1, -12, 0, 30);
        add(4.9f, 2, 20, 1, 22);
        add(7.2f, 3, 16, 0, 28);
        add(9.4f, 4, 0, 1, 22);
        add(11.6f, 7, 0, 0, 26);
    } else {
        add(0.35f, 2, 0, 0, 30);
        add(2.5f, 1, 0, 1, 24);
        add(4.8f, 3, 0, 1, 24);
        add(7.2f, 0, 0, 0, 32);
        add(9.6f, 6, 0, 2, 18);
        add(12.6f, 5, 0, 1, 22);
    }
}

void Game::dropBeast(const Cue& c) {
    Beast b;
    b.id = nextId_++;
    b.kind = c.kind;
    b.hp = hpFor(c.kind);
    b.speed = c.speed;
    b.live = true;
    gate(c.gate, c.skew, b.x, b.y);
    beasts_.push_back(b);
    ashAt(b.x, b.y - 6.f);
    if (c.kind >= 2) blip(90.f, 0.12f);
}

void Game::openWave() {
    beasts_.clear();
    cueAt_ = 0;
    tWave_ = 0;
    lock_ = -1;
    scriptWave(wave_);
    blip(wave_ == 0 ? 392.f : 494.f, 0.1f);
}

void Game::beginRun() {
    wave_ = 0;
    nextWave_ = 0;
    rings_ = kRings;
    breaches_ = 0;
    score_ = 0;
    over_ = false;
    won_ = false;
    reason_ = "UNFINISHED";
    lock_ = -1;
    swingCd_ = 0;
    swingT_ = 0;
    swingNo_ = 1;
    shake_ = 0;
    walking_ = false;
    faceLeft_ = false;
    x_ = kWellX;
    y_ = 180.f;
    beasts_.clear();
    floaters_.clear();
    ash_.clear();
    mode_ = Mode::Play;
    droneHz_ = -1.f;
    openWave();
    sys_->setLight(120, 48, 16);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "UNFINISHED";
    rings_ = kRings;
    breaches_ = 0;
    score_ = 0;
    beasts_.clear();
    floaters_.clear();
    ash_.clear();
    sys_->setLight(90, 36, 12);
}

void Game::defeat() {
    if (over_) return;
    rings_ = 0;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    reason_ = "THE WELL FELL";
    shake_ = 1.f;
    sys_->rumble(1.f, 1.f, 260);
    sys_->apu.noiseBurst(0.6f, 160.f, 0.22f);
    blip(55.f, 0.18f);
    sys_->setLight(140, 20, 8);
}

void Game::victory() {
    if (over_ || rings_ <= 0) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE WELL STANDS";
    score_ += rings_ * 400;
    shake_ = 0.1f;
    sys_->setLight(40, 90, 28);
    blip(523.f, 0.16f);
    for (int i = 0; i < 6; ++i) {
        float a = i * 1.0472f;
        ashAt(kWellX + std::cos(a) * 26.f, kWellY - 16.f + std::sin(a) * 8.f);
    }
}

void Game::fell(Beast& b) {
    int pts = pointsFor(b.kind);
    score_ += pts;
    b.live = false;
    floaters_.push_back(Floater{b.x, b.y - 28.f, 0.7f, pts});
    ashAt(b.x, b.y - 10.f);
}

void Game::crackWell(Beast& b) {
    b.live = false;
    ++breaches_;
    if (rings_ > 0) --rings_;
    shake_ = std::max(shake_, 0.85f);
    ashAt(kWellX, kWellY - 18.f);
    floaters_.push_back(Floater{kWellX, kWellY - 52.f, 0.8f, 0});
    sys_->rumble(0.8f, 1.f, 180);
    sys_->apu.noiseBurst(0.5f, 200.f, 0.16f);
    blip(64.f, 0.16f);
    if (rings_ <= 0) defeat();
}

void Game::strike(Beast& b) {
    b.hp -= 1;
    b.flash = 0.1f;
    b.chew = 0.f;
    b.chewing = false;
    float vx = b.x - kWellX, vy = b.y - kWellY;
    float d = std::hypot(vx, vy);
    if (d < 0.001f) {
        vx = 0;
        vy = 1;
        d = 1;
    }
    float push = d < kRim + 16.f ? (kRim + 20.f) : d + 12.f;
    b.x = std::clamp(kWellX + vx / d * push, 30.f, 290.f);
    b.y = std::clamp(kWellY + vy / d * push, 54.f, 198.f);
    if (b.hp <= 0) fell(b);
}

void Game::hammer() {
    if (mode_ != Mode::Play || swingCd_ > 0.f) return;
    swingCd_ = 0.14f;
    swingT_ = 0.11f;
    ++swingNo_;
    bool hit = false;
    for (Beast& b : beasts_) {
        if (!b.live || b.tagged == swingNo_) continue;
        if (std::hypot(b.x - x_, b.y - y_) > kReach) continue;
        b.tagged = swingNo_;
        hit = true;
        strike(b);
    }
    shake_ = std::max(shake_, hit ? 0.14f : 0.03f);
    sys_->rumble(hit ? 0.28f : 0.06f, hit ? 0.45f : 0.1f, hit ? 36 : 16);
    sys_->apu.noiseBurst(hit ? 0.22f : 0.05f, hit ? 1100.f : 500.f, 0.035f);
    blip(hit ? 280.f : 150.f, hit ? 0.11f : 0.035f);
}

void Game::stepSmith(float vx, float vy, float dt) {
    float len = std::hypot(vx, vy);
    if (len < 8.f) return;
    walking_ = true;
    if (vx < -4.f) faceLeft_ = true;
    else if (vx > 4.f) faceLeft_ = false;
    float mx = vx / len, my = vy / len;
    float nx = x_ + mx * len * dt;
    float ny = y_ + my * len * dt;
    float dx = nx - kWellX, dy = ny - kWellY;
    float d = std::hypot(dx, dy);
    if (d < kSolid) {
        float tx = -dy, ty = dx;
        if (tx * mx + ty * my < 0.f) {
            tx = -tx;
            ty = -ty;
        }
        float tl = std::hypot(tx, ty);
        if (tl > 0.001f) {
            nx = x_ + tx / tl * len * dt;
            ny = y_ + ty / tl * len * dt;
        }
        dx = nx - kWellX;
        dy = ny - kWellY;
        d = std::hypot(dx, dy);
        if (d < kSolid && d > 0.001f) {
            nx = kWellX + dx / d * kSolid;
            ny = kWellY + dy / d * kSolid;
        }
    }
    x_ = std::clamp(nx, 28.f, 292.f);
    y_ = std::clamp(ny, 56.f, 200.f);
}

void Game::bot(float dt) {
    int best = -1;
    float bestT = 1e9f;
    for (int i = 0; i < int(beasts_.size()); ++i) {
        if (!beasts_[i].live) continue;
        float e = urgency(beasts_[i]);
        if (e < bestT) {
            bestT = e;
            best = i;
        }
    }
    int cur = byId(lock_);
    if (cur >= 0 && best >= 0) {
        float d = std::hypot(beasts_[cur].x - x_, beasts_[cur].y - y_);
        if (d < 70.f && urgency(beasts_[cur]) < bestT + 0.45f) best = cur;
    }
    float tx = kWellX, ty = 180.f;
    if (best >= 0) {
        lock_ = beasts_[best].id;
        tx = beasts_[best].x;
        ty = beasts_[best].y;
    } else {
        lock_ = -1;
    }
    float dx = tx - x_, dy = ty - y_;
    float d = std::hypot(dx, dy);
    if (d > 6.f) stepSmith(dx / d * kSpeed, dy / d * kSpeed, dt);
    bool near = false;
    for (const Beast& b : beasts_) {
        if (!b.live) continue;
        if (std::hypot(b.x - x_, b.y - y_) <= kReach - 2.f) near = true;
    }
    if (near) hammer();
}

void Game::human(float dt) {
    const gs::Pad& p = sys_->pad;
    float ax = 0, ay = 0;
    if (p.down(gs::BTN_LEFT)) ax -= 1;
    if (p.down(gs::BTN_RIGHT)) ax += 1;
    if (p.down(gs::BTN_UP)) ay -= 1;
    if (p.down(gs::BTN_DOWN)) ay += 1;
    if (ax == 0.f && ay == 0.f) {
        ax = p.axisX;
        ay = -p.axisY;
    }
    float len = std::hypot(ax, ay);
    if (len > 1.f) {
        ax /= len;
        ay /= len;
    }
    if (len > 0.18f) stepSmith(ax * kSpeed, ay * kSpeed, dt);
    else {
        int n = closest();
        if (n >= 0) faceLeft_ = beasts_[n].x < x_;
    }
    if (p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO) ||
        p.accel > 0.45f)
        hammer();
}

void Game::coolFx(float dt) {
    for (Floater& f : floaters_) {
        f.t -= dt;
        f.y -= 14.f * dt;
    }
    floaters_.erase(std::remove_if(floaters_.begin(), floaters_.end(), [](const Floater& f) { return f.t <= 0.f; }),
                    floaters_.end());
    for (Ash& a : ash_) a.t -= dt;
    ash_.erase(std::remove_if(ash_.begin(), ash_.end(), [](const Ash& a) { return a.t <= 0.f; }), ash_.end());
    for (Beast& b : beasts_)
        if (b.flash > 0.f) b.flash -= dt;
    if (swingCd_ > 0.f) swingCd_ = std::max(0.f, swingCd_ - dt);
    if (swingT_ > 0.f) swingT_ = std::max(0.f, swingT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.7f);
}

void Game::tickPlay(float dt) {
    tWave_ += dt;
    while (cueAt_ < int(script_.size()) && script_[cueAt_].t <= tWave_) {
        dropBeast(script_[cueAt_]);
        ++cueAt_;
    }
    if (bot_) bot(dt);
    else human(dt);
    for (Beast& b : beasts_) {
        if (!b.live) continue;
        float vx = b.x - kWellX, vy = b.y - kWellY;
        float d = std::hypot(vx, vy);
        if (d < 0.001f) {
            vx = 0;
            vy = 1;
            d = 1;
        }
        if (d <= kRim) {
            b.chewing = true;
            b.x = kWellX + vx / d * kRim;
            b.y = kWellY + vy / d * kRim;
            b.chew += dt;
            b.anim += dt * 12.f;
            if (b.chew >= chewTime(b.kind)) {
                crackWell(b);
                if (over_) return;
            }
        } else {
            b.chewing = false;
            float step = b.speed * dt;
            if (d - step <= kRim) {
                b.x = kWellX + vx / d * kRim;
                b.y = kWellY + vy / d * kRim;
                b.chewing = true;
            } else {
                b.x -= vx / d * step;
                b.y -= vy / d * step;
            }
            b.anim += dt * 7.f;
        }
    }
    if (over_) return;
    beasts_.erase(std::remove_if(beasts_.begin(), beasts_.end(), [](const Beast& b) { return !b.live; }), beasts_.end());
    if (cueAt_ >= int(script_.size()) && beasts_.empty()) {
        if (rings_ <= 0) defeat();
        else if (wave_ >= 2) victory();
        else {
            nextWave_ = wave_ + 1;
            mode_ = Mode::Banner;
            bannerT_ = 1.15f;
            blip(620.f, 0.1f);
        }
    }
}

void Game::tickBanner(float dt) {
    if (bot_) bot(dt);
    else human(dt);
    bannerT_ -= dt;
    if (bannerT_ <= 0.f) {
        wave_ = nextWave_;
        openWave();
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

void Game::shade(float x, float y, float w) {
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

void Game::layFloor() {
    gs::VDP& v = sys_->vdp;
    v.B.resize(64, 32);
    v.B.clear();
    for (int y = 0; y < 28; ++y) {
        for (int x = 0; x < 40; ++x) {
            int tile = art_.soot;
            if (y < 5) tile = art_.brick;
            else if ((x + y) % 9 == 0) tile = art_.plate;
            else if (y > 18 && x % 11 == 0) tile = art_.grate;
            v.B.set(x, y, gs::entry(tile, PAL_FLOOR));
        }
    }
    v.A.clear();
    v.A.enabled = false;
    floorLaid_ = true;
}

void Game::sky() {
    uint16_t top = gs::rgb4(4, 2, 2);
    uint16_t low = gs::rgb4(10, 4, 1);
    if (mode_ == Mode::Victory) {
        top = gs::rgb4(3, 6, 4);
        low = gs::rgb4(12, 9, 3);
    } else if (mode_ == Mode::Over) {
        top = gs::rgb4(3, 1, 1);
        low = gs::rgb4(6, 2, 1);
    }
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float u = std::clamp((y - 36) / 80.f, 0.f, 1.f);
        v.lineBackdrop[y] = mixC(top, low, u);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    int glow = std::clamp(8 + int(std::lround(3.f * std::sin(t_ * 4.f))), 0, 15);
    v.setColor(PAL_FLOOR * 16 + 9, gs::rgb4(glow, glow / 2, 1));
    v.setColor(PAL_WELL * 16 + 6, gs::rgb4(1, 2, std::clamp(3 + glow / 4, 0, 15)));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    if (!floorLaid_) layFloor();
    sky();
    camX_ = 0;
    camY_ = 0;
    if (shake_ > 0.f) {
        camX_ = std::sin(t_ * 51.f) * shake_ * 5.f;
        camY_ = std::cos(t_ * 37.f) * shake_ * 3.f;
    }

    if (mode_ == Mode::Title) {
        text("S3 FOUNDRY WELL", 160, 12, 0.7f, PAL_EMBER);
        text("KEEP THE WELL STANDING", 160, 30, 0.4f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 12, 0.62f, PAL_EMBER);
        text("THREE WAVES", 160, 30, 0.46f, PAL_GOOD);
    } else if (mode_ == Mode::Over) {
        text("THE WELL FELL", 160, 12, 0.68f, PAL_ALERT);
        text("THE WATCH IS OVER", 160, 30, 0.42f, PAL_TEXT);
    } else if (mode_ == Mode::Banner) {
        char line[16];
        std::snprintf(line, sizeof line, "WAVE %d", nextWave_ + 1);
        text(line, 160, 12, 0.7f, PAL_EMBER);
        text(waveName(nextWave_), 160, 30, 0.5f, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 12, 0.8f, PAL_EMBER);
    }

    for (const Floater& f : floaters_) {
        if (f.pts <= 0) text("CRACK", f.x, f.y, 0.4f, PAL_ALERT);
        else {
            char buf[12];
            std::snprintf(buf, sizeof buf, "%d", f.pts);
            text(buf, f.x, f.y, 0.4f, PAL_EMBER);
        }
    }
    for (const Ash& a : ash_) {
        float k = 1.f - std::clamp(a.t / 0.34f, 0.f, 1.f);
        spr(art_.puff, a.x, a.y, 8.f + k * 14.f, PAL_FX, false, false);
    }

    float sx = x_, sy = y_;
    bool flip = faceLeft_;
    int frame = 0;
    if (mode_ == Mode::Title) {
        float a = t_ * 0.7f;
        sx = kWellX + std::cos(a) * 58.f;
        sy = kWellY + 28.f + std::sin(a) * 22.f;
        flip = std::sin(a) > 0.f;
        frame = int(t_ * 5.f) & 1;
    } else if (mode_ == Mode::Victory) {
        sy -= std::fabs(std::sin(t_ * 6.f)) * 5.f;
        frame = int(t_ * 7.f) & 1;
    } else if (swingT_ > 0.f) {
        frame = 2;
    } else if (walking_) {
        frame = int(t_ * 8.f) & 1;
    }
    float sh = 42.f * depth(sy);
    spr(art_.smith[frame], sx, sy, sh, PAL_SMITH, flip, true);
    shade(sx, sy, sh * 0.65f);
    if (swingT_ > 0.f) {
        float dir = flip ? -1.f : 1.f;
        spr(art_.arc, sx + dir * 16.f, sy - 20.f, 14.f, PAL_FX, flip, false);
    }

    auto drawBeast = [&](const Beast& b) {
        const gs::Mipped* m = art_.spark;
        int pal = PAL_SPARK;
        float h = 26.f;
        if (b.kind == 1) {
            m = art_.slag;
            pal = PAL_SLAG;
            h = 34.f;
        } else if (b.kind >= 2) {
            m = art_.pour;
            pal = PAL_POUR;
            h = 46.f;
        }
        if (b.flash > 0.f) pal = PAL_FX;
        h *= depth(b.y);
        spr(m[int(b.anim) & 1], b.x, b.y, h, pal, b.x < kWellX, true);
        shade(b.x, b.y, h * 0.7f);
        if (b.chewing) {
            float frac = std::clamp(b.chew / chewTime(b.kind), 0.f, 1.f);
            stamp(art_.bar, b.x, b.y - h - 4.f, std::max(4.f, 28.f * frac), 4.f, PAL_ALERT, false);
        }
    };

    if (mode_ == Mode::Title) {
        Beast demo;
        demo.kind = 0;
        demo.x = 70;
        demo.y = 168;
        demo.anim = t_ * 6.f;
        drawBeast(demo);
        demo.kind = 1;
        demo.x = 250;
        demo.y = 150;
        demo.anim = t_ * 4.f;
        drawBeast(demo);
        demo.kind = 2;
        demo.x = 200;
        demo.y = 190;
        demo.anim = t_ * 3.f;
        drawBeast(demo);
    } else {
        std::vector<int> ord;
        for (int i = 0; i < int(beasts_.size()); ++i)
            if (beasts_[i].live) ord.push_back(i);
        std::sort(ord.begin(), ord.end(), [&](int a, int b) { return beasts_[a].y < beasts_[b].y; });
        for (int i : ord) drawBeast(beasts_[i]);
    }

    float wh = 70.f * depth(kWellY);
    if (rings_ <= 0 || mode_ == Mode::Over) spr(art_.rubble, kWellX, kWellY + 8.f, 36.f, PAL_WELL, false, true);
    else {
        spr(art_.well, kWellX, kWellY, wh, PAL_WELL, false, true);
        if (rings_ <= 2) spr(art_.crack, kWellX - 6.f, kWellY - 22.f, 18.f, PAL_ALERT, false, false);
        if (rings_ <= 1) spr(art_.crack, kWellX + 8.f, kWellY - 10.f, 20.f, PAL_ALERT, true, false);
    }
    shade(kWellX, kWellY + 8.f, 48.f);

    spr(art_.stack, 36.f, 86.f, 78.f, PAL_IRON, false, true);
    shade(36.f, 86.f, 22.f);
    spr(art_.crucible, 286.f, 96.f, 40.f, PAL_IRON, false, true);
    shade(286.f, 96.f, 30.f);
    spr(art_.anvil, 64.f, 188.f, 20.f * depth(188.f), PAL_IRON, false, true);
    spr(art_.ingot, 96.f, 192.f, 12.f, PAL_IRON, false, true);
    spr(art_.ladle, 250.f, 176.f, 28.f * depth(176.f), PAL_IRON, false, true);
    float sway = std::sin(t_ * 1.6f) * 2.f;
    spr(art_.chain, 120.f, 48.f + sway, 18.f, PAL_IRON, false, false);
    spr(art_.chain, 200.f, 46.f - sway, 18.f, PAL_IRON, false, false);

    char line[40];
    if (mode_ == Mode::Title) {
        hud(8, 23, "SPARKS  SLAG  POUR", PAL_EMBER);
        hud(1, 24, "ARROWS WALK THE FLOOR", PAL_TEXT);
        hud(1, 25, "Z OR SPACE SWINGS THE HAMMER", PAL_EMBER);
        hud(1, 26, "ENTER STARTS THE WATCH", PAL_GOOD);
        hud(22, 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Banner) {
        std::snprintf(line, sizeof line, "WAVE %d/3", std::min(3, wave_ + 1));
        hud(1, 0, line, PAL_EMBER);
        std::string pips = "WELL ";
        int show = std::max(0, rings_);
        for (int i = 0; i < kRings; ++i) pips.push_back(i < show ? '#' : '-');
        hud(29, 0, pips, show > 1 ? PAL_GOOD : PAL_ALERT);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_TEXT);
        if (mode_ == Mode::Play && wave_ == 0 && tWave_ < 2.2f) hud(16, 27, "Z SWINGS", PAL_EMBER);
    } else if (mode_ == Mode::Victory) {
        hud(1, 0, "THE WELL STANDS", PAL_GOOD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_EMBER);
        hud(30, 27, "ENTER", PAL_TEXT);
    } else if (mode_ == Mode::Over) {
        hud(1, 0, "THE WELL FELL", PAL_ALERT);
        std::snprintf(line, sizeof line, "WAVE %d", wave_ + 1);
        hud(30, 0, line, PAL_TEXT);
        hud(1, 27, "ENTER TRIES AGAIN", PAL_EMBER);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootAudio();
    layFloor();
    rings_ = kRings;
    x_ = kWellX;
    y_ = 180.f;
    if (bot_) beginRun();
    else {
        mode_ = Mode::Title;
        sys.setLight(90, 36, 12);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    walking_ = false;
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
        else tickPlay(kDt);
    } else if (mode_ == Mode::Banner) {
        tickBanner(kDt);
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    }
    coolFx(kDt);
    drone();
    draw();
}

}  // namespace foundrywell
