#include "game/fort.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace fort {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float SIEGE = 30.0f;
constexpr int GATE_MAX = 150;
constexpr float GATE_Z = 1.32f;
constexpr float SPAWN_Z = 14.2f;
constexpr float BOLT_VZ = 12.0f;
constexpr float MOVE = 2.35f;
constexpr float FIRE_CD = 0.15f;
constexpr float BRACE_CD = 1.55f;
constexpr int HORIZON = 74;
constexpr float FOCAL = 150.0f;
constexpr float ROAD_K = 1.05f;
constexpr float TAU = 6.2831853f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    gate_ = GATE_MAX;
    score_ = 0;
    siege_ = SIEGE;
    attempt_ = 0;
    foes_.clear();
    bolts_.clear();
    puffs_.clear();
    pops_.clear();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (won_) return 2;
    if (mode_ == Mode::Lost || (mode_ == Mode::Over && !won_)) return 3;
    return 1;
}

uint32_t Game::rnd() {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return rng_;
}

float Game::frnd() { return float(rnd() & 0xffff) / 65535.0f; }

void Game::quiet() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noise(0, 0, false);
    drone_ = false;
    fanStep_ = -1;
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = std::max(beep_, 0.05f);
}

void Game::puffAt(float x, float z, float h, bool spark) {
    if (puffs_.size() > 24) return;
    Puff p;
    p.x = x;
    p.z = z;
    p.t = spark ? 0.18f : 0.32f;
    p.h = h;
    p.spark = spark;
    puffs_.push_back(p);
}

void Game::popAt(float sx, float sy, int pts, bool bad) {
    if (pops_.size() > 6) pops_.erase(pops_.begin());
    Pop p;
    p.x = sx;
    p.y = sy;
    p.t = 0.7f;
    p.pts = pts;
    p.bad = bad;
    pops_.push_back(p);
}

void Game::spawn(Kind kind, float x, float z, float speedMul) {
    for (const Foe& e : foes_) {
        if (e.alive && std::fabs(e.lane - x) < 0.14f && std::fabs(e.z - z) < 0.85f) z += 1.15f;
    }
    z = clampf(z, GATE_Z + 0.4f, SPAWN_Z);
    Foe f;
    f.kind = kind;
    f.lane = x;
    f.x = x;
    f.z = z;
    f.phase = frnd() * TAU;
    f.freq = 1.15f + frnd() * 1.05f;
    f.age = frnd() * 0.5f;
    f.alive = true;
    if (kind == Kind::Raider) {
        f.speed = 2.2f * speedMul;
        f.radius = 0.185f;
        f.weave = 0.04f;
        f.hp = 1;
        f.dmg = 12;
        f.score = 100;
    } else if (kind == Kind::Shield) {
        f.speed = 1.42f * speedMul;
        f.radius = 0.21f;
        f.weave = 0.028f;
        f.hp = 2;
        f.dmg = 16;
        f.score = 250;
    } else {
        f.speed = 0.98f * speedMul;
        f.radius = 0.27f;
        f.weave = 0.016f;
        f.hp = 3;
        f.dmg = 26;
        f.score = 400;
    }
    foes_.push_back(f);
}

void Game::spawnRush(int which) {
    blip(196, 0.05f);
    sys_->apu.tone(2, 294, 0.04f);
    beep_ = std::max(beep_, 0.1f);
    if (which == 0) {
        spawn(Kind::Raider, -0.62f, 5.6f, 1.30f);
        spawn(Kind::Raider, 0.64f, 5.75f, 1.26f);
    } else if (which == 1) {
        spawn(Kind::Raider, -0.66f, 5.2f, 1.34f);
        spawn(Kind::Raider, 0.06f, 5.5f, 1.2f);
        spawn(Kind::Raider, 0.60f, 5.0f, 1.32f);
    } else if (which == 2) {
        spawn(Kind::Ram, 0.0f, 7.5f, 1.0f);
        spawn(Kind::Raider, -0.64f, 4.6f, 1.4f);
        spawn(Kind::Raider, 0.66f, 4.75f, 1.36f);
    } else {
        spawn(Kind::Shield, -0.34f, 5.6f, 1.12f);
        spawn(Kind::Raider, 0.62f, 4.4f, 1.42f);
        spawn(Kind::Raider, -0.68f, 4.9f, 1.28f);
    }
}

void Game::newGame() {
    mode_ = Mode::Siege;
    over_ = false;
    won_ = false;
    siege_ = SIEGE;
    gate_ = GATE_MAX;
    score_ = 0;
    leaks_ = 0;
    kills_ = 0;
    playerX_ = 0;
    fireCd_ = 0.18f;
    braceCd_ = 0.4f;
    spawnCd_ = 0.45f;
    aim_ = 0;
    braceT_ = 0;
    shake_ = 0;
    flash_ = 0;
    banner_ = 0;
    lastSec_ = 99;
    lastLane_ = -1;
    fanStep_ = -1;
    rushMask_ = 0;
    foes_.clear();
    bolts_.clear();
    puffs_.clear();
    pops_.clear();
    rng_ = bot_ ? 0xF047u : (0xC0FFu ^ uint32_t(attempt_) * 0x85EBCA6Bu);
    attempt_++;
    drone_ = false;
    spawn(Kind::Raider, 0.04f, 9.4f, 1.0f);
}

int Game::inbound(int index) const {
    const Foe& e = foes_[size_t(index)];
    int n = 0;
    for (const Bolt& b : bolts_) {
        if (!b.alive || b.z > e.z + 0.3f) continue;
        if (std::fabs(b.x - e.x) <= e.radius + 0.04f) n++;
    }
    return n;
}

int Game::pickFocus() const {
    int best = -1;
    float bestS = 1.0e9f;
    for (int i = 0; i < int(foes_.size()); i++) {
        const Foe& e = foes_[size_t(i)];
        if (!e.alive) continue;
        int need = e.hp - inbound(i);
        float eta = (e.z - GATE_Z) / std::max(0.25f, e.speed);
        if (need <= 0 && eta > 0.6f) continue;
        float s = eta - float(e.dmg) * 0.01f;
        if (need > 0) s -= 0.4f;
        if (e.z < 2.4f) s -= 1.6f;
        if (s < bestS) {
            bestS = s;
            best = i;
        }
    }
    return best;
}

void Game::botAim(float& slide, bool& fire, bool& braceDown) {
    fire = false;
    braceDown = false;
    slide = 0;
    int i = pickFocus();
    if (i < 0) {
        float d = -playerX_;
        if (std::fabs(d) > 0.03f) slide = d > 0 ? 1.0f : -1.0f;
        return;
    }
    const Foe& f = foes_[size_t(i)];
    float d = f.x - playerX_;
    if (std::fabs(d) <= MOVE * DT + 0.001f) {
        playerX_ = f.x;
        slide = 0;
    } else {
        slide = d > 0 ? 1.0f : -1.0f;
    }
    bool lined = std::fabs(playerX_ - f.x) < 0.05f;
    int need = f.hp - inbound(i);
    if (lined && need > 0) fire = true;
    if (f.z < 2.2f && braceCd_ <= 0.0f && std::fabs(playerX_ - f.x) < 0.58f) braceDown = true;
}

void Game::loose() {
    int live = 0;
    for (const Bolt& b : bolts_)
        if (b.alive) live++;
    if (live > 12) return;
    Bolt b;
    b.x = playerX_;
    b.z = GATE_Z + 0.06f;
    b.alive = true;
    bolts_.push_back(b);
    fireCd_ = FIRE_CD;
    aim_ = 0.12f;
    blip(680, 0.05f);
    sys_->apu.noiseBurst(0.05f, 4600, 0.03f);
}

void Game::braceGate() {
    braceCd_ = BRACE_CD;
    braceT_ = 0.34f;
    flash_ = std::max(flash_, 0.1f);
    sys_->rumble(0.35f, 0.75f, 70);
    sys_->apu.noiseBurst(0.32f, 520, 0.1f);
    blip(140, 0.07f);
    for (Foe& e : foes_) {
        if (!e.alive || e.z > 2.45f) continue;
        if (std::fabs(e.x - playerX_) > 0.58f) continue;
        e.z += 1.9f;
        e.flash = 0.16f;
        e.hp -= 1;
        puffAt(e.x, e.z, 16, true);
        if (e.hp <= 0) {
            e.alive = false;
            score_ += e.score;
            kills_++;
            float sx, sy;
            project(e.x, e.z, sx, sy);
            popAt(sx, sy - 16, e.score, false);
        }
    }
}

void Game::beginWin() {
    if (siege_ < 0) siege_ = 0;
    won_ = true;
    mode_ = Mode::Won;
    banner_ = 0;
    score_ += gate_ * 15;
    sys_->apu.tone(1, 0, 0);
    drone_ = false;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.tone(0, 392, 0.07f);
    sys_->setLight(255, 210, 80);
    sys_->rumble(0.25f, 0.45f, 140);
}

void Game::beginLoss() {
    gate_ = 0;
    won_ = false;
    mode_ = Mode::Lost;
    banner_ = 0;
    sys_->apu.tone(1, 0, 0);
    drone_ = false;
    fanStep_ = -1;
    sys_->apu.noiseBurst(0.55f, 100, 0.4f);
    sys_->apu.tone(0, 78, 0.08f);
    beep_ = 0.45f;
    shake_ = 0.65f;
    flash_ = 0.35f;
    sys_->setLight(180, 16, 12);
    sys_->rumble(0.85f, 0.3f, 220);
}

void Game::fanfare() {
    static const float notes[] = {392, 523, 659, 784, 1046};
    static const float hold[] = {0.11f, 0.11f, 0.11f, 0.14f, 0.42f};
    if (fanStep_ < 0 || fanStep_ >= 5) return;
    fanT_ += DT;
    if (fanT_ < hold[fanStep_]) return;
    fanStep_++;
    fanT_ = 0;
    if (fanStep_ < 5) sys_->apu.tone(0, notes[fanStep_], 0.07f);
    else {
        sys_->apu.tone(0, 0, 0);
        fanStep_ = -1;
    }
}

void Game::update(float dt) {
    if (!drone_) {
        sys_->apu.tone(1, 49, 0.028f);
        drone_ = true;
    }
    for (Foe& e : foes_) {
        if (!e.alive) continue;
        e.age += dt;
        e.z -= e.speed * dt;
        e.x = e.lane + std::sin(e.age * e.freq + e.phase) * e.weave;
        if (e.flash > 0) e.flash -= dt;
    }
    for (Bolt& b : bolts_) {
        if (!b.alive) continue;
        int near = -1;
        float best = 0.22f;
        for (int i = 0; i < int(foes_.size()); i++) {
            const Foe& e = foes_[size_t(i)];
            if (!e.alive || e.z + 0.05f < b.z) continue;
            float d = std::fabs(e.x - b.x);
            if (d < best) {
                best = d;
                near = i;
            }
        }
        if (near >= 0) {
            float d = foes_[size_t(near)].x - b.x;
            b.x += clampf(d, -1.0f, 1.0f) * 0.7f * dt;
        }
        float nz = b.z + BOLT_VZ * dt;
        int hit = -1;
        float hz = 1.0e9f;
        for (int i = 0; i < int(foes_.size()); i++) {
            Foe& e = foes_[size_t(i)];
            if (!e.alive) continue;
            if (b.z <= e.z + 0.2f && nz >= e.z - 0.05f && std::fabs(b.x - e.x) <= e.radius) {
                if (e.z < hz) {
                    hz = e.z;
                    hit = i;
                }
            }
        }
        b.z = nz;
        if (hit >= 0) {
            b.alive = false;
            Foe& e = foes_[size_t(hit)];
            e.hp--;
            e.flash = 0.12f;
            if (e.hp <= 0) {
                e.alive = false;
                score_ += e.score;
                kills_++;
                float sx, sy;
                project(e.x, e.z, sx, sy);
                popAt(sx, sy - 18, e.score, false);
                puffAt(e.x, e.z, 14, true);
                blip(230.0f + float(e.score) * 0.03f, 0.05f);
            } else {
                puffAt(e.x, e.z, 10, true);
                blip(160, 0.04f);
            }
        } else if (b.z > SPAWN_Z + 1.4f) {
            b.alive = false;
        }
    }
    for (Foe& e : foes_) {
        if (!e.alive || e.z > GATE_Z) continue;
        e.alive = false;
        leaks_++;
        gate_ -= e.dmg;
        shake_ = std::min(0.8f, shake_ + 0.3f);
        flash_ = std::max(flash_, 0.18f);
        float sx, sy;
        project(e.x, GATE_Z, sx, sy);
        popAt(sx, sy - 22, e.dmg, true);
        puffAt(e.x, GATE_Z, 18, false);
        sys_->apu.noiseBurst(0.42f, 160, 0.14f);
        blip(64, 0.06f);
        sys_->rumble(0.7f, 0.35f, 80);
    }
    if (gate_ < 0) gate_ = 0;
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.alive; }), foes_.end());
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return !b.alive; }), bolts_.end());
    if (gate_ <= 0) {
        beginLoss();
        return;
    }
    siege_ -= dt;
    int sec = int(std::ceil(siege_ - 1e-4f));
    if (sec != lastSec_) {
        lastSec_ = sec;
        if (sec >= 1 && sec <= 6) blip(sec <= 3 ? 960.0f : 700.0f, 0.04f);
    }
    if (siege_ <= 0) {
        beginWin();
        return;
    }
    float elapsed = SIEGE - siege_;
    const float rushAt[] = {9.5f, 15.5f, 21.0f, 26.2f};
    for (int i = 0; i < 4; i++) {
        if ((rushMask_ & (1 << i)) == 0 && elapsed >= rushAt[i]) {
            rushMask_ |= 1 << i;
            spawnRush(i);
        }
    }
    spawnCd_ -= dt;
    if (spawnCd_ <= 0 && foes_.size() < 16) {
        const float lanes[] = {-0.60f, -0.34f, -0.08f, 0.16f, 0.42f, 0.64f};
        int lane = int(rnd() % 6);
        if (lane == lastLane_) lane = (lane + 1 + int(rnd() % 5)) % 6;
        lastLane_ = lane;
        float r = frnd();
        Kind k = Kind::Raider;
        if (elapsed < 8) k = r < 0.8f ? Kind::Raider : Kind::Shield;
        else if (elapsed < 18) k = r < 0.55f ? Kind::Raider : (r < 0.86f ? Kind::Shield : Kind::Ram);
        else k = r < 0.36f ? Kind::Raider : (r < 0.7f ? Kind::Shield : Kind::Ram);
        spawn(k, lanes[lane], SPAWN_Z, 0.94f + frnd() * 0.12f);
        float u = clampf(elapsed / SIEGE, 0.0f, 1.0f);
        spawnCd_ = 1.32f - u * 0.42f;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0 && fanStep_ < 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 0.8f);
    if (flash_ > 0) flash_ = std::max(0.0f, flash_ - DT);
    if (aim_ > 0) aim_ -= DT;
    if (braceT_ > 0) braceT_ -= DT;
    for (Puff& p : puffs_) p.t -= DT;
    for (Pop& p : pops_) {
        p.t -= DT;
        p.y -= 18.0f * DT;
    }
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());

    if (mode_ == Mode::Siege) {
        if (fireCd_ > 0) fireCd_ -= DT;
        if (braceCd_ > 0) braceCd_ -= DT;
    }

    const gs::Pad& pad = sys.pad;
    bool fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO);
    bool braceDown = pad.down(gs::BTN_B);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
    float slide = 0;
    if (pad.down(gs::BTN_LEFT)) slide -= 1;
    if (pad.down(gs::BTN_RIGHT)) slide += 1;
    if (std::fabs(pad.axisX) > 0.18f) slide = pad.axisX;

    if (bot_) {
        fire = false;
        braceDown = false;
        start = false;
        back = false;
        tap = false;
        slide = 0;
        if (mode_ == Mode::Title && age_ > 0.45f) start = true;
        else if (mode_ == Mode::Siege) botAim(slide, fire, braceDown);
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) {
            mode_ = Mode::Title;
            quiet();
        }
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start || tap) newGame();
    } else if (mode_ == Mode::Over) {
        if (start || tap) newGame();
        else if (back) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            quiet();
        }
    } else if (mode_ == Mode::Siege) {
        if (start && !bot_) {
            held_ = Mode::Siege;
            mode_ = Mode::Pause;
        } else {
            playerX_ = clampf(playerX_ + slide * MOVE * DT, -0.76f, 0.76f);
            if (fire && fireCd_ <= 0) loose();
            if (braceDown && braceCd_ <= 0) braceGate();
            update(DT);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        banner_ += DT;
        if (banner_ > 1.25f) {
            over_ = true;
            mode_ = Mode::Over;
        }
    }
    if (fanStep_ >= 0) fanfare();
    draw();
}

void Game::project(float x, float z, float& sx, float& sy) const {
    z = std::max(0.4f, z);
    float row = FOCAL / z;
    sy = float(HORIZON) + row;
    sx = 160.0f + x * row * ROAD_K;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.0f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + shx_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy + shy_ - (feet ? float(s.h) : s.h * 0.5f)));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 24 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (w < 1.0f || h < 1.0f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + shx_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy + shy_ - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 24 || s.y + s.h < -48) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float total = 0;
    for (unsigned char c : s) {
        if (c < 32 || c >= 128) continue;
        total += float(art_.glyph[c - 32].w) * scale;
    }
    float cursor = x - total * 0.5f;
    for (unsigned char c : s) {
        if (c < 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float w = float(g.w) * scale;
        if (c > 32) spr(g, cursor + w * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        cursor += w;
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

void Game::drawFoe(const Foe& f) {
    float sx, sy;
    project(f.x, f.z, sx, sy);
    float h = clampf(102.0f / f.z, 8.0f, 90.0f);
    if (f.flash > 0) h *= 1.07f;
    int frame = int(f.age * 8.0f) & 1;
    int fog = std::clamp(int((f.z - 2.0f) * 0.85f), 0, 12);
    bool flip = std::sin(f.age * f.freq + f.phase) > 0;
    stamp(art_.shadow, sx, sy, h * 0.8f, 8, PAL_DUST, true);
    if (f.kind == Kind::Raider) spr(art_.raider[frame], sx, sy, h, PAL_RAIDER, flip, fog, true);
    else if (f.kind == Kind::Shield) spr(art_.shield[frame], sx, sy, h * 1.05f, PAL_SHIELD, flip, fog, true);
    else spr(art_.ram[frame], sx, sy, h * 0.95f, PAL_RAM, false, fog, true);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(age_ * 92.0f) * 5.0f * shake_;
        shy_ = std::cos(age_ * 74.0f) * 3.0f * shake_;
    }
    bool hold = won_;
    bool fell = !won_ && (mode_ == Mode::Lost || mode_ == Mode::Over);
    if (hold) v.setFogColor(gs::rgb4(4, 3, 5));
    else if (fell) v.setFogColor(gs::rgb4(5, 1, 2));
    else v.setFogColor(gs::rgb4(2, 2, 4));

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = float(y) / float(HORIZON);
            int r = 1 + int(u * u * 8.0f);
            int g = 1 + int(u * 3.0f);
            int b = 6 - int(u * 4.0f);
            if (b < 1) b = 1;
            if (flash_ > 0) r = std::min(15, r + int(flash_ * 8.0f));
            if (hold) {
                r = std::min(15, r + 1);
                g = std::min(15, g + 2);
            }
            if (fell) r = std::min(15, r + 3);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - HORIZON);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.0f + shx_;
        rd.hw = depth * ROAD_K;
        rd.v = 2200.0f - depth * 5.0f + age_ * 55.0f;
        rd.pal = PAL_ROAD;
        rd.band = (int(depth) / 5) & 1;
        rd.style = gs::ROAD_MUD;
        rd.left = rd.right = gs::GROUND_LAND;
        int fog = std::clamp(int((98.0f - depth) / 8.0f), 0, 12);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(1, 2, 1);
    }

    if (mode_ == Mode::Title) {
        text("S3 FORT", 168, 14, 1.15f, PAL_GOLD);
        text("HOLD THE GATE", 168, 40, 0.7f, PAL_HUD);
        text("UNTIL THE CLOCK DIES", 168, 58, 0.58f, PAL_GOLD);
    } else if (hold) {
        text("THE CLOCK DIES", 160, 30, 1.05f, PAL_GOLD);
        text("THE GATE HOLDS", 160, 58, 0.72f, PAL_HUD);
    } else if (fell) {
        text("THE GATE FALLS", 160, 30, 1.0f, PAL_ALERT);
        text("THE CLOCK STILL BURNS", 160, 58, 0.58f, PAL_DIM);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 36, 1.2f, PAL_GOLD);
    } else {
        float show = std::max(0.0f, siege_);
        int tenths = int(std::ceil(show * 10.0f - 1.0e-4f));
        if (tenths < 0) tenths = 0;
        int sec = tenths / 10;
        int frac = tenths % 10;
        char buf[16];
        std::snprintf(buf, sizeof buf, "%d.%d", sec, frac);
        text("CLOCK", 168, 10, 0.5f, PAL_DIM);
        float sc = 1.2f;
        if (sec <= 6) sc += 0.07f * std::sin(age_ * 12.0f);
        text(buf, 168, 32, sc, sec <= 6 ? PAL_ALERT : PAL_GOLD);
    }

    for (const Pop& p : pops_) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "%s%d", p.bad ? "-" : "+", p.pts);
        text(buf, p.x, p.y, 0.52f, p.bad ? PAL_ALERT : PAL_GOLD);
    }

    float px, py;
    project(playerX_, GATE_Z, px, py);
    float feet = 212;
    stamp(art_.shadow, px, feet - 2, 40, 10, PAL_DUST, true);
    if (braceT_ > 0.02f) spr(art_.brace, px, feet - 34, 26, PAL_FIRE, false, 0, false);
    int pose = braceT_ > 0 ? 2 : (aim_ > 0 ? 1 : 0);
    spr(art_.warden[pose], px, feet, 58, PAL_WARDEN, false, 0, true);

    for (const Bolt& b : bolts_) {
        if (!b.alive) continue;
        float sx, sy;
        project(b.x, b.z, sx, sy);
        spr(art_.bolt, sx, sy, clampf(24.0f / b.z, 4.0f, 16.0f), PAL_BOLT, false, 0, false);
    }
    for (const Puff& p : puffs_) {
        float sx, sy;
        project(p.x, p.z, sx, sy);
        float k = clampf(p.t * 4.0f, 0.3f, 1.0f);
        if (p.spark) spr(art_.spark, sx, sy - 8, p.h * k, PAL_DUST, false, 0, false);
        else spr(art_.puff, sx, sy - 6, p.h * (1.3f - k), PAL_DUST, false, 0, false);
    }

    float gx, gy;
    project(0, GATE_Z, gx, gy);
    stamp(art_.wall, 160, gy - 2, 308, 32, PAL_GATE, false);

    float lx, ly, rx, ry;
    project(-1.08f, GATE_Z, lx, ly);
    project(1.08f, GATE_Z, rx, ry);
    int torch = int(age_ * 12.0f) & 1;
    float sway = std::sin(age_ * 2.1f) * 5.0f;
    spr(art_.banner, lx + sway, ly - 118, 40, PAL_BANNER, false, 0, true);
    spr(art_.banner, rx - sway, ly - 118, 40, PAL_BANNER, true, 0, true);
    spr(art_.torch[torch], lx + 12, ly - 36, 24, PAL_FIRE, false, 0, true);
    spr(art_.torch[torch], rx - 12, ly - 36, 24, PAL_FIRE, true, 0, true);

    if (mode_ == Mode::Title) {
        Foe posed[4];
        const Kind kinds[] = {Kind::Raider, Kind::Shield, Kind::Ram, Kind::Raider};
        const float lanes[] = {0.22f, -0.38f, 0.02f, -0.55f};
        const float zs[] = {3.3f, 4.7f, 6.6f, 8.4f};
        for (int i = 0; i < 4; i++) {
            Foe& f = posed[i];
            f = Foe{};
            f.kind = kinds[i];
            f.lane = lanes[i];
            f.z = zs[i];
            f.age = age_ * (0.8f + 0.1f * float(i));
            f.freq = 1.7f;
            f.phase = lanes[i] * 4.0f;
            f.weave = 0.03f;
            f.x = f.lane + std::sin(f.age * f.freq + f.phase) * f.weave;
            f.alive = true;
            drawFoe(f);
        }
    } else {
        std::vector<int> order;
        order.reserve(foes_.size());
        for (int i = 0; i < int(foes_.size()); i++)
            if (foes_[size_t(i)].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[size_t(a)].z < foes_[size_t(b)].z; });
        for (int i : order) drawFoe(foes_[size_t(i)]);
    }

    spr(fell ? art_.doorBroke : art_.door, gx, gy + 6, 108, PAL_GATE, false, 0, true);
    spr(art_.tower, lx, ly + 6, 150, PAL_GATE, false, 0, true);
    spr(art_.tower, rx, ry + 6, 150, PAL_GATE, true, 0, true);

    float shown = mode_ == Mode::Title ? std::fmod(age_ * 0.15f, 1.0f) : clampf((SIEGE - siege_) / SIEGE, 0.0f, 0.999f);
    int q = std::clamp(int(shown * 4.0f), 0, 3);
    spr(art_.clock[q], 32, 22, mode_ == Mode::Title ? 36.0f : 26.0f, PAL_GOLD, false, 0, false);
    spr(art_.moon, 288, 18, 28, PAL_MOON, false, 0, false);
    const float stars[][2] = {{18, 8}, {54, 16}, {96, 6}, {214, 8}, {246, 20}, {308, 10}, {150, 6}};
    for (const auto& s : stars) spr(art_.star, s[0], s[1], 6, PAL_MOON, false, 0, false);

    char buf[32];
    std::snprintf(buf, sizeof buf, "SCORE %d", score_);
    hud(1, 0, "S3 FORT", PAL_HUD);
    hud(40 - int(std::strlen(buf)), 0, buf, PAL_HUD);
    hud(1, 1, "GATE", PAL_HUD);
    int filled = gate_ <= 0 ? 0 : (gate_ * 14 + GATE_MAX - 1) / GATE_MAX;
    if (filled > 14) filled = 14;
    int pip = art_.pipOk;
    if (gate_ * 3 < GATE_MAX) pip = art_.pipLow;
    else if (gate_ * 2 < GATE_MAX) pip = art_.pipMid;
    for (int i = 0; i < 14; i++) {
        int tile = i < filled ? pip : art_.pipOff;
        v.HUD.set(6 + i, 1, gs::entry(tile, PAL_HUD));
    }
    if (mode_ != Mode::Title) hud(30, 1, braceCd_ <= 0.0f ? "BRACE" : ".....", braceCd_ <= 0.0f ? PAL_GOLD : PAL_DIM);
    if (mode_ == Mode::Title) {
        hudC(26, "ARROWS MOVE  C FIRES  X BRACES", PAL_HUD);
        hudC(27, "START STANDS THE WALL", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(27, "START RESUMES", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(27, won_ ? "THE CLOCK DIED" : "START TRIES THE GATE", won_ ? PAL_GOLD : PAL_ALERT);
    }
}

}  // namespace fort
