#include "game/baron.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace baron {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FOCAL = 240.0f;
constexpr float HORIZON = 98.0f;
constexpr float GROUND_Y = -3.2f;
constexpr int WAVES = 5;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch rotaryPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.9f;
    p.op[0] = {0.5f, 0.85f, 0.02f, 0.4f, 1.0f, 0.3f};
    p.op[1] = {1.0f, 1.0f, 0.02f, 0.35f, 1.0f, 0.25f};
    p.op[2] = {1.5f, 0.45f, 0.03f, 0.4f, 0.7f, 0.25f, 5};
    p.op[3] = {2.0f, 0.28f, 0.02f, 0.3f, 0.5f, 0.2f};
    p.vol = 0.14f;
    p.drive = 0.4f;
    p.tone = 1600;
    return p;
}

gs::FMPatch brassPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.35f;
    p.op[0] = {1, 1, 0.01f, 0.18f, 0.7f, 0.12f};
    p.op[1] = {2, 0.6f, 0.01f, 0.2f, 0.5f, 0.12f};
    p.op[2] = {3, 0.35f, 0.02f, 0.25f, 0.4f, 0.15f};
    p.op[3] = {1, 0.4f, 0.01f, 0.2f, 0.6f, 0.12f};
    p.vol = 0.22f;
    p.drive = 0.15f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (over_) return 4;
    if (mode_ == Mode::Clear) return 2;
    if ((mode_ == Mode::Fly || mode_ == Mode::Dead) && aceUp_) return 3;
    if (mode_ == Mode::Fly || mode_ == Mode::Dead) return 1;
    return 0;
}

const char* Game::waveName() const {
    static const char* n[WAVES] = {"DAWN PATROL", "BALLOON LINE", "THE RAID", "CHECK SIX", "THE CIRCUS"};
    return practice_ ? "PRACTICE" : n[std::clamp(wave_, 0, WAVES - 1)];
}

const char* Game::waveOrder() const {
    static const char* n[WAVES] = {"SIX SCOUTS, HEAD ON", "BALLOONS, THEN THE ESCORT", "STOP THE TWO SEATERS",
                                   "THEY SHOOT BACK", "A RED TRIPLANE"};
    return practice_ ? "ENDLESS SCOUTS" : n[std::clamp(wave_, 0, WAVES - 1)];
}

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

void Game::blip(bool high) {
    sys_->apu.tone(0, high ? 880.0f : 520.0f, 0.07f);
    beep_ = 0.05f;
}

void Game::explode() { sys_->apu.noiseBurst(0.55f, 900.0f, 0.28f); }

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
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

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::bootSortie(bool practice) {
    practice_ = practice;
    wave_ = 0;
    score_ = 0;
    lives_ = 4;
    hull_ = 100;
    over_ = false;
    won_ = false;
    px_ = py_ = vx_ = vy_ = 0;
    shots_.clear();
    pops_.clear();
    prepareWave();
    mode_ = Mode::Brief;
    t_ = 0;
    if (!engineOn_) {
        sys_->apu.setPatch(0, rotaryPatch());
        sys_->apu.setPatch(1, brassPatch());
        sys_->apu.keyOn(0, 100, 0.14f);
        engineOn_ = true;
    }
}

void Game::prepareWave() {
    enemies_.clear();
    shots_.clear();
    spawns_.clear();
    aceUp_ = false;
    skyFor(practice_ ? 1 : wave_, skyTop_, skyHor_, skyFog_);
    if (sys_) sys_->vdp.setFogColor(skyFog_);
    if (practice_) {
        practiceSpawn_ = 0.6f;
        return;
    }
    auto add = [&](float when, Kind k, float x, float y) { spawns_.push_back({when, k, x, y}); };
    if (wave_ == 0) {
        for (int i = 0; i < 6; i++) add(0.35f + i * 1.05f, Kind::Scout, (i % 2 ? -2.5f : 2.5f), (i % 3) * 0.65f - 0.55f);
    } else if (wave_ == 1) {
        for (int i = 0; i < 3; i++) add(0.15f + i * 0.35f, Kind::Balloon, -2.3f + i * 2.3f, 1.7f);
        for (int i = 0; i < 4; i++) add(1.4f + i * 1.05f, Kind::Scout, (i % 2 ? -1.6f : 1.9f), i % 2 ? 0.5f : -0.4f);
    } else if (wave_ == 2) {
        for (int i = 0; i < 3; i++) add(0.25f + i * 1.45f, Kind::Bomber, -1.7f + i * 1.7f, -0.65f);
        for (int i = 0; i < 3; i++) add(0.7f + i * 1.25f, Kind::Scout, (i - 1) * 1.9f, 1.05f);
    } else if (wave_ == 3) {
        for (int i = 0; i < 8; i++)
            add(0.25f + i * 0.82f, Kind::Scout, (i % 2 ? -3.1f : 3.1f), ((i % 4) - 1.5f) * 0.5f);
    } else {
        add(0.35f, Kind::Ace, 0.2f, 0.35f);
        for (int i = 0; i < 4; i++) add(1.0f + i * 1.15f, Kind::Scout, (i % 2 ? -2.8f : 2.8f), i % 2 ? 1.15f : -0.35f);
    }
}

void Game::addKill(int pts, float sx, float sy) {
    if (t_ - lastKill_ < 1.15f) pts = pts * 3 / 2;
    lastKill_ = t_;
    score_ += pts;
    pops_.push_back({sx, sy, 0.7f, pts});
    if (pops_.size() > 5) pops_.erase(pops_.begin());
}

void Game::hurt(int dmg) {
    if (mode_ != Mode::Fly || invuln_ > 0 || roll_ > 0) return;
    hull_ -= dmg;
    shake_ = 0.28f;
    sys_->rumble(0.55f, 0.9f, 160);
    sys_->apu.noiseBurst(0.4f, 1400.0f, 0.12f);
    if (hull_ > 0) return;
    hull_ = 0;
    lives_--;
    mode_ = Mode::Dead;
    t_ = 0;
    explode();
    shots_.erase(std::remove_if(shots_.begin(), shots_.end(), [](const Shot& s) { return s.hostile; }), shots_.end());
}

void Game::stick(float& sx, float& sy, bool& fire, bool& roll) {
    sx = sy = 0;
    fire = roll = false;
    if (bot_) {
        const Enemy* best = nullptr;
        const Enemy* close = nullptr;
        auto rank = [](Kind k) { return k == Kind::Ace ? 0 : k == Kind::Bomber ? 1 : k == Kind::Balloon ? 2 : 3; };
        for (const Enemy& e : enemies_) {
            if (e.z < 8.0f && std::hypot(e.x - px_, e.y - py_) < 1.6f && (!close || e.z < close->z)) close = &e;
            if (e.z < 7.0f || e.z > 42.0f) continue;
            if (!best || rank(e.kind) < rank(best->kind) || (rank(e.kind) == rank(best->kind) && e.z < best->z)) best = &e;
        }
        const Shot* danger = nullptr;
        for (const Shot& s : shots_) {
            if (!s.hostile || s.z > 15.0f) continue;
            if (std::hypot(s.x - px_, s.y - py_) > 1.05f) continue;
            if (!danger || s.z < danger->z) danger = &s;
        }
        if (danger || close) {
            float ax = danger ? danger->x : close->x;
            float ay = danger ? danger->y : close->y;
            sx = px_ >= ax ? 1.0f : -1.0f;
            sy = py_ >= ay ? 0.8f : -0.8f;
            roll = rollCd_ <= 0;
            return;
        }
        if (best) {
            sx = std::clamp((best->x - px_) * 1.7f, -1.0f, 1.0f);
            sy = std::clamp((best->y - py_) * 1.7f, -1.0f, 1.0f);
            fire = std::fabs(best->x - px_) < 0.7f && std::fabs(best->y - py_) < 0.7f;
        } else {
            sx = std::clamp(-px_ * 0.9f, -1.0f, 1.0f);
            sy = std::clamp(-py_ * 0.9f, -1.0f, 1.0f);
        }
        return;
    }
    const gs::Pad& pad = sys_->pad;
    sx = std::fabs(pad.axisX) > 0.08f ? pad.axisX : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
    sy = std::fabs(pad.axisY) > 0.08f ? pad.axisY : float(pad.down(gs::BTN_UP)) - float(pad.down(gs::BTN_DOWN));
    fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.45f;
    roll = pad.pressed(gs::BTN_TURBO);
}

void Game::update(float dt) {
    float sx, sy;
    bool fire = false, doRoll = false;
    if (mode_ == Mode::Fly) stick(sx, sy, fire, doRoll);
    else sx = sy = 0;

    if (mode_ == Mode::Fly) {
        vx_ += sx * 11.0f * dt;
        vy_ += sy * 9.0f * dt;
    }
    vx_ -= vx_ * 2.6f * dt;
    vy_ -= vy_ * 2.6f * dt;
    px_ = std::clamp(px_ + vx_ * dt, -4.4f, 4.4f);
    py_ = std::clamp(py_ + vy_ * dt, -1.35f, 2.15f);
    scroll_ += 78.0f * dt;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - dt);
    if (invuln_ > 0) invuln_ = std::max(0.0f, invuln_ - dt);
    if (rollCd_ > 0) rollCd_ = std::max(0.0f, rollCd_ - dt);
    if (doRoll && rollCd_ <= 0 && mode_ == Mode::Fly) {
        roll_ = 0.48f;
        rollCd_ = 1.45f;
        sys_->apu.noiseBurst(0.2f, 2200.0f, 0.1f);
    }
    if (roll_ > 0) roll_ = std::max(0.0f, roll_ - dt);

    for (Prop& p : props_) {
        p.z -= 16.0f * dt;
        if (p.z < 1.4f) {
            p.z += 46.0f + rnd() * 8.0f;
            p.x = (rnd() < 0.5f ? -1.0f : 1.0f) * (6.5f + rnd() * 5.0f);
            p.kind = int(rnd() * 4.0f) % 4;
        }
    }

    if (mode_ != Mode::Fly) return;

    fireCd_ = std::max(0.0f, fireCd_ - dt);
    if (fire && fireCd_ <= 0) {
        fireCd_ = 0.11f;
        shots_.push_back({px_ - 0.22f, py_, 1.8f, 0, 0, false});
        shots_.push_back({px_ + 0.22f, py_, 1.8f, 0, 0, false});
        sys_->apu.noiseBurst(0.18f, 4800.0f, 0.045f);
    }

    if (practice_) {
        practiceSpawn_ -= dt;
        if (practiceSpawn_ <= 0 && enemies_.size() < 6) {
            spawns_.push_back({0, Kind::Scout, (rnd() * 2.0f - 1.0f) * 3.2f, (rnd() * 2.0f - 1.0f) * 1.1f});
            practiceSpawn_ = 0.95f + rnd() * 0.45f;
        }
    }

    for (Spawn& s : spawns_) s.t -= dt;
    while (!spawns_.empty() && spawns_.front().t <= 0 && enemies_.size() < 8) {
        const Spawn s = spawns_.front();
        spawns_.erase(spawns_.begin());
        Enemy e{};
        e.kind = s.kind;
        e.x = s.x;
        e.y = s.y;
        e.home = s.x;
        e.oy = s.y;
        e.age = rnd() * 2.0f;
        e.shoot = 0.8f + rnd() * 0.6f;
        e.z = 34.0f;
        if (s.kind == Kind::Scout) {
            e.hp = 3;
            e.speed = 15;
            e.radius = 1.05f;
            e.height = 2.35f;
            e.amp = 1.2f;
            e.freq = 1.45f;
            e.homing = 0.16f;
            e.points = 100;
        } else if (s.kind == Kind::Balloon) {
            e.hp = 4;
            e.speed = 6.5f;
            e.radius = 1.7f;
            e.height = 4.8f;
            e.amp = 0.25f;
            e.freq = 0.45f;
            e.homing = 0;
            e.points = 150;
            e.z = 26;
        } else if (s.kind == Kind::Bomber) {
            e.hp = 5;
            e.speed = 11;
            e.radius = 1.35f;
            e.height = 3.4f;
            e.amp = 0.4f;
            e.freq = 0.7f;
            e.homing = 0.04f;
            e.points = 250;
            e.leak = true;
            e.z = 38;
        } else {
            e.hp = 12;
            e.speed = 14.5f;
            e.radius = 1.15f;
            e.height = 2.9f;
            e.amp = 1.65f;
            e.freq = 1.1f;
            e.homing = 0.4f;
            e.points = 1000;
            e.z = 30;
            aceUp_ = true;
        }
        enemies_.push_back(e);
    }

    for (Enemy& e : enemies_) {
        e.age += dt;
        float tx = e.home + std::sin(e.age * e.freq) * e.amp;
        float ty = e.oy + std::sin(e.age * e.freq * 0.8f + 1.4f) * e.amp * 0.40f;
        e.x += ((tx - e.x) * 2.3f + (px_ - e.x) * e.homing) * dt;
        e.y += (ty - e.y) * 2.0f * dt;
        e.x = std::clamp(e.x, -4.5f, 4.5f);
        e.y = std::clamp(e.y, -1.3f, 2.3f);
        e.z -= e.speed * dt;
        e.flash = std::max(0.0f, e.flash - dt);
        e.shoot -= dt;
        bool armed = e.kind != Kind::Balloon && e.z < 30.0f && e.z > 7.0f && std::fabs(e.x - px_) < (e.kind == Kind::Ace ? 2.6f : 1.9f);
        float gap = e.kind == Kind::Ace ? 0.85f : e.kind == Kind::Bomber ? 1.6f : 1.35f;
        if (armed && e.shoot <= 0) {
            e.shoot = gap;
            float aimX = px_ + (rnd() - 0.5f) * 1.7f;
            float aimY = py_ + (rnd() - 0.5f) * 1.2f;
            float eta = std::max(0.2f, e.z / 52.0f);
            Shot s;
            s.x = e.x;
            s.y = e.y;
            s.z = e.z;
            s.vx = (aimX - e.x) / eta;
            s.vy = (aimY - e.y) / eta;
            s.hostile = true;
            shots_.push_back(s);
        }
    }

    for (Shot& s : shots_) {
        if (s.hostile) {
            s.z -= 52.0f * dt;
            s.x += s.vx * dt;
            s.y += s.vy * dt;
        } else {
            s.z += 92.0f * dt;
        }
    }

    for (Shot& s : shots_) {
        if (s.hostile || s.z < 0) continue;
        const float prev = s.z - 92.0f * dt;
        for (Enemy& e : enemies_) {
            if (e.hp <= 0) continue;
            const float z0 = std::min(prev, s.z) - 0.6f, z1 = std::max(prev, s.z) + 0.6f;
            if (e.z >= z0 && e.z <= z1 && std::hypot(s.x - e.x, s.y - e.y) < e.radius) {
                e.hp -= 1;
                e.flash = 0.1f;
                s.z = -1;
                if (e.hp <= 0) {
                    float sxp = 160 + (e.x - px_) * FOCAL / std::max(1.0f, e.z);
                    float syp = HORIZON - (e.y - py_) * FOCAL / std::max(1.0f, e.z);
                    addKill(e.points, sxp, syp);
                    if (e.kind == Kind::Ace) aceUp_ = false;
                    explode();
                    e.z = -1;
                }
                break;
            }
        }
    }

    for (Enemy& e : enemies_) {
        if (e.z < 0) continue;
        if (e.hp <= 0) continue;
        if (e.z > 0.4f && e.z < 1.8f && std::hypot(e.x - px_, e.y - py_) < 0.7f) {
            hurt(e.kind == Kind::Balloon ? 28 : 40);
            explode();
            e.z = -1;
        } else if (e.z <= 1.0f) {
            // They come around. A two-seater that reaches the lines does it only once.
            if (e.leak && !e.leaked) {
                e.leaked = true;
                hurt(26);
            }
            e.z = 30.0f + rnd() * 4.0f;
            e.home = std::clamp((rnd() - 0.5f) * 6.0f, -3.4f, 3.4f);
            e.oy = std::clamp((rnd() - 0.5f) * 2.2f, -1.1f, 1.9f);
            e.x = e.home;
            e.y = e.oy;
            e.shoot = 0.4f;
        }
    }
    for (Shot& s : shots_) {
        if (s.hostile && s.z < 1.25f && s.z > 0 && std::hypot(s.x - px_, s.y - py_) < 0.78f) {
            hurt(20);
            s.z = -1;
        }
    }

    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return e.z < 0 || e.hp <= 0; }), enemies_.end());
    shots_.erase(std::remove_if(shots_.begin(), shots_.end(),
                                 [](const Shot& s) { return s.z < 0 || s.z > 70; }),
                 shots_.end());

    if (mode_ == Mode::Fly && !practice_ && spawns_.empty() && enemies_.empty()) {
        t_ = 0;
        if (wave_ >= WAVES - 1) {
            mode_ = Mode::Victory;
            won_ = true;
            over_ = true;
            fanfare();
            if (bot_) std::printf("circus breaks  score %d  lives %d\n", score_, lives_);
        } else {
            mode_ = Mode::Clear;
            fanfare();
            if (bot_) std::printf("wave %d clear  score %d  lives %d\n", wave_ + 1, score_, lives_);
        }
    }
}

void Game::banners() {
    char buf[48];
    if (mode_ == Mode::Title) text("S3 BARON", 160, 72, 1.45f, PAL_RED);
    else if (mode_ == Mode::Menu) text("S3 BARON", 160, 52, 1.1f, PAL_RED);
    else if (mode_ == Mode::Help) text("HOW TO FLY", 160, 36, 1.0f, PAL_AMBER);
    else if (mode_ == Mode::Brief) text(waveName(), 160, 64, 1.05f, PAL_AMBER);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 72, 1.3f, PAL_HUD);
    else if (mode_ == Mode::Clear) {
        std::snprintf(buf, sizeof buf, "WAVE %d CLEAR", wave_ + 1);
        text(buf, 160, 46, 0.95f, PAL_GREEN);
    } else if (mode_ == Mode::Dead) text(lives_ > 0 ? "HIT" : "DOWN", 160, 48, 1.3f, PAL_RED);
    else if (mode_ == Mode::Over) text("SHOT DOWN", 160, 46, 1.1f, PAL_RED);
    else if (mode_ == Mode::Victory) text("CIRCUS BREAKS", 160, 42, 0.9f, PAL_AMBER);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    banners();
    v.setFogColor(skyFog_);
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = (rnd() - 0.5f) * 10.0f * shake_ * 4;
        shy = (rnd() - 0.5f) * 6.0f * shake_ * 4;
    }
    const int horizon = std::clamp(int(std::lround(HORIZON + shy)), 64, 140);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < horizon) {
            v.lineBackdrop[y] = lerpC(skyTop_, skyHor_, y / float(horizon));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - horizon) + 1.0f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = 280;
        r.cx = 160 - px_ * (16.0f + row * 0.28f) + shx;
        r.v = scroll_ + 4200.0f / row;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(r.v / 36.0f)) & 1) ? 1 : 0;
        r.style = 0;
        r.left = r.right = 0;
        v.lineFog[y] = uint8_t(std::clamp(int(13 - row / 7.0f), 0, 13));
        v.lineBackdrop[y] = skyHor_;
    }

    // Far sprites first in the list so nearer ones (drawn earlier) cover them.
    // We draw near-to-far by pushing the near sprites first.
    auto project = [&](float x, float y, float z, float& sx, float& sy, int& fog) {
        float zz = std::max(0.8f, z);
        float s = FOCAL / zz;
        sx = 160 + (x - px_) * s + shx;
        sy = HORIZON - (y - py_) * s + shy;
        fog = std::clamp(int((zz - 16.0f) / 2.6f), 0, 13);
    };

    // Clouds sit behind everything, so they are pushed last.
    struct Cloud {
        float x, y, h;
    };
    Cloud clouds[5];
    for (int i = 0; i < 5; i++) {
        float span = 420.0f;
        float x = std::fmod(30.0f + i * 86.0f - t_ * (8.0f + i * 2.0f) - px_ * 18.0f, span);
        if (x < 0) x += span;
        clouds[i] = {x - 40.0f, 16.0f + (i % 3) * 18.0f, 16.0f + (i % 3) * 7.0f};
    }

    // Title kite: a red triplane crossing far ahead.
    bool showAce = mode_ == Mode::Title || mode_ == Mode::Menu || mode_ == Mode::Help;
    float aceX = 0, aceY = 0, aceZ = 1;
    if (showAce) {
        // High in the sky, clear of the title lettering.
        aceZ = 27.0f;
        aceX = std::sin(t_ * 0.4f) * 1.4f;
        aceY = 7.2f;
    }

    // Player, sight, flashes, popups — on top.
    if (mode_ == Mode::Fly || mode_ == Mode::Dead || mode_ == Mode::Pause || mode_ == Mode::Clear || showAce ||
        mode_ == Mode::Brief || mode_ == Mode::Over || mode_ == Mode::Victory) {
        bool blink = invuln_ > 0 && (int(t_ * 18) & 1);
        if (!blink && mode_ != Mode::Title && mode_ != Mode::Menu && mode_ != Mode::Help) {
            float bank = std::clamp(vx_ * 0.24f, -1.0f, 1.0f);
            if (roll_ > 0) bank = std::sin((0.48f - roll_) * 28.0f);
            if (mode_ == Mode::Dead) bank = std::sin(t_ * 22.0f);
            int frame = std::fabs(bank) > 0.62f ? 2 : std::fabs(bank) > 0.28f ? 1 : 0;
            float ph = (mode_ == Mode::Dead) ? 56.0f - t_ * 18.0f : 54.0f;
            if (roll_ > 0) ph *= 0.72f + 0.28f * std::fabs(std::cos((0.48f - roll_) * 28.0f));
            spr(art_.rear[frame], 160 + shx, 180 + shy, std::max(16.0f, ph), PAL_PLAYER, bank < 0);
            if (fireCd_ > 0.08f && mode_ == Mode::Fly) {
                spr(art_.flash, 132 + shx, 168 + shy, 12, PAL_FX, false);
                spr(art_.flash, 188 + shx, 168 + shy, 12, PAL_FX, false);
            }
        }
        if (mode_ == Mode::Fly || mode_ == Mode::Pause || mode_ == Mode::Dead)
            spr(art_.sight, 160 + shx, HORIZON + shy, 36, PAL_FX, false);
    }
    for (const Popup& p : pops_) text("+" + std::to_string(p.pts), p.x, p.y, 0.7f, PAL_AMBER);

    // Bullets and enemies, near first.
    std::vector<int> order(enemies_.size());
    for (int i = 0; i < int(enemies_.size()); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) { return enemies_[a].z < enemies_[b].z; });

    for (const Shot& s : shots_) {
        if (s.z < 1.2f) continue;
        float sx, sy;
        int fog;
        project(s.x, s.y, s.z, sx, sy, fog);
        float h = s.hostile ? std::max(6.0f, 90.0f / s.z) : std::max(5.0f, 70.0f / s.z);
        spr(s.hostile ? art_.tracer : art_.bullet, sx, sy, h, PAL_FX, false, fog);
    }
    for (int idx : order) {
        const Enemy& e = enemies_[idx];
        if (e.z < 0.8f) continue;
        float sx, sy;
        int fog;
        project(e.x, e.y, e.z, sx, sy, fog);
        float h = e.height * FOCAL / e.z;
        if (e.flash > 0) spr(art_.puff, sx, sy, h * 0.8f, PAL_FX, false, fog);
        if (e.kind == Kind::Balloon) {
            spr(art_.balloon, sx, sy, h, PAL_ENEMY, false, fog);
        } else {
            float b = std::sin(e.age * e.freq);
            int frame = std::fabs(b) > 0.66f ? 2 : std::fabs(b) > 0.3f ? 1 : 0;
            int pal = e.kind == Kind::Ace ? PAL_ACE : PAL_ENEMY;
            spr(art_.front[frame], sx, sy, h, pal, b < 0, fog);
        }
    }
    if (showAce) {
        float sx, sy;
        int fog;
        project(aceX, aceY, aceZ, sx, sy, fog);
        float b = std::cos(t_ * 0.45f);
        int frame = std::fabs(b) > 0.66f ? 2 : std::fabs(b) > 0.3f ? 1 : 0;
        spr(art_.front[frame], sx, sy, 2.9f * FOCAL / aceZ, PAL_ACE, b < 0, fog);
    }

    for (const Prop& p : props_) {
        if (p.z < 1.2f) continue;
        float s = FOCAL / p.z;
        float sx = 160 + (p.x - px_) * s + shx;
        float sy = HORIZON - (GROUND_Y - py_) * s + shy;
        float h = (p.kind == 2 ? 1.1f : 3.4f) * s;
        int fog = std::clamp(int((p.z - 14.0f) / 2.8f), 0, 14);
        spr(art_.prop[p.kind], sx, sy, h, PAL_PROP, p.x < 0, fog, true);
    }
    for (const Cloud& c : clouds) spr(art_.cloud, c.x, c.y, c.h, PAL_FX, false, 3);

    char buf[48];
    if (mode_ == Mode::Fly || mode_ == Mode::Pause || mode_ == Mode::Dead || mode_ == Mode::Clear) {
        std::snprintf(buf, sizeof buf, "WAVE %d", practice_ ? 0 : wave_ + 1);
        if (!practice_) hud(1, 1, buf, PAL_AMBER);
        else hud(1, 1, "PRACTICE", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "X%d", std::max(0, lives_));
        hud(1, 26, buf, lives_ > 1 ? PAL_HUD : PAL_RED);
        hud(4, 26, "HULL", PAL_HUD);
        int pips = std::clamp(hull_ / 10, 0, 10);
        for (int i = 0; i < 10; i++) hud(9 + i, 26, i < pips ? "=" : "-", i < pips ? PAL_GREEN : PAL_HUD);
        if (rollCd_ <= 0) hud(30, 26, "ROLL", PAL_GREEN);
        if (mode_ == Mode::Pause) {
            hudC(14, "START  RESUME", PAL_HUD);
            hudC(16, "ESC    MENU", PAL_HUD);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(12, "WESTERN FRONT", PAL_AMBER);
        if (int(t_ * 2) % 2 == 0) hudC(17, "PRESS START", PAL_HUD);
        hudC(21, "ARROWS FLY    C FIRE    SPACE ROLL", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Menu) {
        const char* items[] = {"SORTIE", "PRACTICE", "HOW TO FLY"};
        for (int i = 0; i < 3; i++) {
            hud(12, 12 + i * 2, i == menu_ ? ">" : " ", PAL_AMBER);
            hud(14, 12 + i * 2, items[i], i == menu_ ? PAL_AMBER : PAL_HUD);
        }
        hudC(22, "START TO FLY", PAL_HUD);
    } else if (mode_ == Mode::Help) {
        const char* lines[] = {"ARROWS OR STICK MOVE THE SCOUT", "C OR A FIRES THE VICKERS", "SPACE BARREL ROLLS THROUGH FIRE",
                               "PUT THE SIGHT ON A KITE", "BALLOONS ARE SLOW AND FAT", "TWO SEATERS MUST NOT PASS",
                               "THE LAST ONE IS PAINTED RED"};
        for (int i = 0; i < 7; i++) hudC(7 + i * 2, lines[i], PAL_HUD);
        hudC(23, "START OR ESC", PAL_AMBER);
    } else if (mode_ == Mode::Brief) {
        hudC(12, waveOrder(), PAL_HUD);
        std::snprintf(buf, sizeof buf, "SORTIE %d OF %d", wave_ + 1, WAVES);
        if (!practice_) hudC(15, buf, PAL_HUD);
        if (int(t_ * 2) % 2 == 0) hudC(19, "START", PAL_AMBER);
    } else if (mode_ == Mode::Clear) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(14, buf, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(12, buf, PAL_HUD);
        hudC(16, waveName(), PAL_AMBER);
        hudC(20, "START", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        const char* rank = score_ >= 3800 && lives_ >= 2 ? "CIRCUS BREAKER" : score_ >= 2500 ? "ACE" : "FLIGHT LEAD";
        hudC(11, rank, PAL_GREEN);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(14, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "KITES LEFT %d", std::max(0, lives_));
        hudC(16, buf, PAL_HUD);
        hudC(20, "START", PAL_HUD);
    }

    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (engineOn_) {
        float burble = 1.0f + 0.055f * std::sin(t_ * 46.0f) * std::sin(t_ * 17.0f);
        float busy = (mode_ == Mode::Fly || mode_ == Mode::Dead) ? 1.0f : 0.72f;
        sys_->apu.setFreq(0, 102.0f * burble * busy);
        sys_->apu.setVol(0, mode_ == Mode::Fly ? 0.15f : 0.07f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {523.0f, 659.0f, 784.0f, 1046.0f};
        fanT_ += DT;
        if (fanT_ > 0.13f) {
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.2f);
            else sys_->apu.keyOff(1);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 6) fanStep_ = -1;
        }
    }
    for (Popup& p : pops_) p.t -= DT;
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Popup& p) { return p.t <= 0; }), pops_.end());
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    skyFor(0, skyTop_, skyHor_, skyFog_);
    sys.vdp.setFogColor(skyFog_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.18f, 0.25f, 0.18f);
    props_.clear();
    for (int i = 0; i < 8; i++) {
        Prop p;
        p.z = 8.0f + i * 6.0f;
        p.x = (i % 2 ? -1.0f : 1.0f) * (7.0f + (i % 3) * 1.6f);
        p.kind = i % 4;
        props_.push_back(p);
    }
    if (bot_) bootSortie(false);
    else {
        mode_ = Mode::Title;
        sys.apu.setPatch(0, rotaryPatch());
        sys.apu.setPatch(1, brassPatch());
        sys.apu.keyOn(0, 80, 0.07f);
        engineOn_ = true;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    t_ += dt;
    const gs::Pad& pad = sys.pad;

    if (!bot_ && mode_ == Mode::Title) {
        px_ = std::sin(t_ * 0.35f) * 0.7f;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Menu;
            menu_ = 0;
            t_ = 0;
            blip(true);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (!bot_ && mode_ == Mode::Menu) {
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_DOWN)) {
            menu_ = (menu_ + (pad.pressed(gs::BTN_DOWN) ? 1 : 2)) % 3;
            blip(false);
        }
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            if (menu_ == 2) {
                mode_ = Mode::Help;
                t_ = 0;
            } else {
                bootSortie(menu_ == 1);
            }
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Help) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Brief) {
        if (bot_ && t_ > 0.28f) {
            mode_ = Mode::Fly;
            t_ = 0;
        } else if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Fly;
            t_ = 0;
            blip(true);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Menu;
        }
    } else if (mode_ == Mode::Fly) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Fly;
            mode_ = Mode::Pause;
        } else {
            update(dt);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Fly;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Clear) {
        if ((bot_ && t_ > 0.35f) || pad.pressed(gs::BTN_START) || t_ > 2.4f) {
            wave_++;
            prepareWave();
            mode_ = Mode::Brief;
            t_ = 0;
            px_ = py_ = vx_ = vy_ = 0;
        }
    } else if (mode_ == Mode::Dead) {
        update(dt);  // world keeps moving; combat is gated on Mode::Fly
        if ((bot_ && t_ > 0.45f) || t_ > 1.35f) {
            if (lives_ <= 0) {
                mode_ = Mode::Over;
                over_ = true;
                t_ = 0;
                if (bot_) std::printf("shot down  score %d  wave %d\n", score_, wave_ + 1);
            } else {
                mode_ = Mode::Fly;
                hull_ = 100;
                invuln_ = 2.1f;
                px_ = py_ = vx_ = vy_ = 0;
                t_ = 0;
            }
        }
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) mode_ = Mode::Menu;
    }

    // Title and menus drift so the field is alive.
    if (mode_ == Mode::Title || mode_ == Mode::Menu || mode_ == Mode::Help || mode_ == Mode::Brief) {
        scroll_ += 48.0f * dt;
        for (Prop& p : props_) {
            p.z -= 10.0f * dt;
            if (p.z < 1.4f) p.z += 48.0f;
        }
    }

    draw();
}

}  // namespace baron
