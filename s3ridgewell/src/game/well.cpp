#include "game/well.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rwell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kStones = 4;
constexpr float kHorizon = 72.f;
constexpr float kZScale = 280.f;
constexpr float kPikeZ = 2.5f;
constexpr float kWellZ = 2.65f;
constexpr float kWellU = -1.42f;
constexpr float kSpawn = 30.f;
constexpr float kStop = 6.8f;
constexpr float kBite = 8.6f;
constexpr float kPast = 5.7f;
constexpr float kCover = 0.30f;
constexpr float kLaneV = 3.2f;
constexpr float kLaneGap = 0.66f;
constexpr float kCrest = 32.f;
constexpr float kThrustCd = 0.24f;

float laneU(int lane) { return (lane - 1) * kLaneGap; }

float bashLimit(int kind) {
    if (kind == 2) return 2.15f;
    if (kind == 1) return 1.05f;
    return 0.85f;
}

int pointsFor(int kind) {
    if (kind == 2) return 600;
    if (kind == 1) return 160;
    return 100;
}

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Fail) return 4;
    if (mode_ == Mode::Play || mode_ == Mode::Banner || mode_ == Mode::Pause) return 1;
    return 0;
}

const char* Game::waveName(int w) const {
    if (w <= 0) return "BUCKETS";
    if (w == 1) return "CLUBS";
    return "THE RAM";
}

const char* Game::hint() const {
    int tgt = bestFoe();
    if (tgt < 0) return "HOLD THE RIDGE";
    const Foe& f = foes_[size_t(tgt)];
    bool here = coveredLane() == f.lane;
    if (f.z <= kBite && !here) {
        if (f.lane <= 0) return "STEP LEFT";
        if (f.lane >= 2) return "STEP RIGHT";
        return "STEP TO THE CROWN";
    }
    if (f.bash > 0.2f) return "THRUST";
    if (f.z <= kBite) return "THRUST";
    if (f.lane <= 0) return "LEFT TRACK";
    if (f.lane >= 2) return "RIGHT TRACK";
    return "THE CROWN";
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.08f;
}

float Game::bendAt(float row) const { return std::sin(row * 0.012f + 0.4f) * (6.f + row * 0.018f); }

float Game::halfAt(float row) const { return 20.f + row * 0.68f; }

int Game::horizon() const { return std::clamp(int(std::lround(kHorizon + shy_)), 48, 100); }

Game::Spot Game::spot(float u, float z, float base) const {
    Spot s;
    if (!(z > 0.9f)) return s;
    float row = kZScale / z;
    float feet = kZScale / kPikeZ;
    float t = std::clamp(row / feet, 0.02f, 1.55f);
    s.h = std::max(4.f, base * std::pow(t, 0.72f));
    float along = std::clamp((z - kPikeZ) / kCrest, 0.f, 1.f);
    float lift = std::pow(along, 1.15f) * s.h * 0.42f;
    s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
    s.y = float(hor_) + row - lift + shy_;
    s.fog = int(std::clamp(along * 9.f, 0.f, 9.f));
    s.ok = true;
    return s;
}

void Game::buildProps() {
    props_.clear();
    auto add = [&](float z, float u, float h, int kind) {
        Prop p;
        p.z = z;
        p.u = u;
        p.h = h;
        p.kind = kind;
        props_.push_back(p);
    };
    add(4.4f, -1.25f, 78.f, 0);
    add(5.2f, 1.22f, 70.f, 0);
    add(8.4f, -1.32f, 46.f, 1);
    add(11.5f, 1.28f, 40.f, 1);
    add(15.5f, -1.3f, 36.f, 1);
    add(19.5f, 1.26f, 32.f, 1);
    add(3.6f, -1.2f, 64.f, 2);
    for (int lane = 0; lane < 3; ++lane) add(7.15f, laneU(lane), 26.f, 3);
}

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
    // Arrival gaps stay above a full lane-cross so the watch can meet each one.
    if (wave <= 0) {
        const int lanes[] = {1, 0, 2, 1, 0, 2};
        for (int i = 0; i < 6; ++i) add(0.70f + float(i) * 1.55f, lanes[i], 0, 5.2f);
    } else if (wave == 1) {
        const int lanes[] = {0, 2, 1, 2, 0, 1, 0};
        for (int i = 0; i < 7; ++i) add(0.60f + float(i) * 1.50f, lanes[i], 1, 4.25f);
    } else {
        add(0.55f, 1, 0, 6.3f);
        add(2.20f, 0, 0, 6.3f);
        add(3.85f, 2, 0, 6.3f);
        add(5.50f, 1, 0, 6.3f);
        add(6.40f, 0, 2, 2.65f);
        add(12.60f, 2, 0, 6.3f);
        add(14.30f, 1, 0, 6.3f);
        add(16.00f, 0, 0, 6.3f);
        add(17.80f, 2, 1, 4.4f);
    }
}

void Game::startWave() {
    foes_.clear();
    puffs_.clear();
    spawnAt_ = 0;
    tWave_ = 0;
    thrustCd_ = 0;
    buildScript(wave_);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    bracing_ = false;
    wave_ = 0;
    nextWave_ = 0;
    stones_ = kStones;
    through_ = 0;
    score_ = 0;
    target_ = 1;
    holdDir_ = 0;
    fanStep_ = -1;
    t_ = 0;
    tWave_ = 0;
    u_ = 0;
    bannerT_ = 0;
    thrustCd_ = 0;
    thrustT_ = 0;
    shake_ = 0;
    beep_ = 0;
    fanT_ = 0;
    holdT_ = 0;
    reason_ = "UNFINISHED";
    foes_.clear();
    puffs_.clear();
    script_.clear();
    spawnAt_ = 0;
}

void Game::beginRun() {
    wave_ = 0;
    nextWave_ = 0;
    stones_ = kStones;
    through_ = 0;
    score_ = 0;
    target_ = 1;
    u_ = laneU(1);
    holdDir_ = 0;
    holdT_ = 0;
    over_ = false;
    won_ = false;
    bracing_ = false;
    reason_ = "UNFINISHED";
    fanStep_ = -1;
    fanT_ = 0;
    shake_ = 0;
    thrustT_ = 0;
    t_ = 0;
    mode_ = Mode::Play;
    startWave();
    blip(440.f);
    sys_->setLight(80, 50, 20);
}

void Game::lose() {
    if (over_) return;
    stones_ = 0;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    reason_ = "THE WELL FELL";
    shake_ = 0.9f;
    sys_->rumble(0.9f, 1.f, 240);
    sys_->apu.noiseBurst(0.55f, 240.f, 0.22f);
    blip(64.f);
    sys_->setLight(120, 20, 16);
}

void Game::win() {
    if (over_ || stones_ <= 0) return;
    score_ += stones_ * 200;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE WELL STANDS";
    fanStep_ = 0;
    fanT_ = 0;
    shake_ = 0.12f;
    sys_->setLight(40, 120, 50);
    blip(523.f);
}

void Game::kill(Foe& f) {
    score_ += pointsFor(f.kind);
    f.alive = false;
    Puff u;
    u.z = f.z;
    u.u = laneU(f.lane);
    u.age = 0;
    u.life = 0.36f;
    puffs_.push_back(u);
}

void Game::breach(Foe& f) {
    if (!f.alive || over_) return;
    f.alive = false;
    ++through_;
    --stones_;
    shake_ = std::max(shake_, 0.62f);
    Puff u;
    u.z = kWellZ;
    u.u = kWellU;
    u.life = 0.45f;
    puffs_.push_back(u);
    sys_->rumble(0.8f, 1.f, 160);
    sys_->apu.noiseBurst(0.48f, 320.f, 0.16f);
    blip(74.f);
    if (stones_ <= 0) {
        stones_ = 0;
        lose();
    }
}

void Game::thrust(int lane) {
    if (thrustCd_ > 0.f || lane < 0) return;
    int hit = -1;
    float nearest = 1e9f;
    for (int i = 0; i < int(foes_.size()); ++i) {
        Foe& f = foes_[size_t(i)];
        if (!f.alive || f.lane != lane) continue;
        if (f.z > kBite || f.z < kPast - 0.05f) continue;
        if (f.z < nearest) {
            nearest = f.z;
            hit = i;
        }
    }
    if (hit < 0) return;
    Foe& f = foes_[size_t(hit)];
    f.hp -= 1;
    f.flash = 0.1f;
    thrustCd_ = kThrustCd;
    thrustT_ = 0.12f;
    shake_ = std::max(shake_, f.hp <= 0 ? 0.16f : 0.08f);
    sys_->rumble(0.22f, 0.45f, 40);
    sys_->apu.noiseBurst(0.32f, f.hp <= 0 ? 1600.f : 860.f, 0.05f);
    blip(f.hp <= 0 ? 520.f : 190.f);
    if (f.hp <= 0) kill(f);
}

int Game::bestFoe() const {
    bool hot = false;
    for (const Foe& f : foes_)
        if (f.alive && f.z <= kBite + 0.35f) hot = true;
    int best = -1;
    float bestU = 1e9f;
    for (int i = 0; i < int(foes_.size()); ++i) {
        const Foe& f = foes_[size_t(i)];
        if (!f.alive) continue;
        if (hot && f.z > kBite + 0.35f) continue;
        float u = (f.z <= kStop + 0.08f) ? (-1000.f - f.bash) : (f.z - kStop) / std::max(0.2f, f.speed);
        u += std::fabs(laneU(f.lane) - u_) * 0.01f;
        if (u < bestU) {
            bestU = u;
            best = i;
        }
    }
    return best;
}

int Game::coveredLane() const {
    int best = -1;
    float bestD = kCover;
    for (int i = 0; i < 3; ++i) {
        float d = std::fabs(u_ - laneU(i));
        if (d <= bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

void Game::aim(float dt) {
    if (bot_) {
        int dest = target_;
        int tgt = bestFoe();
        if (tgt >= 0) dest = foes_[size_t(tgt)].lane;
        else if (spawnAt_ < int(script_.size())) dest = script_[size_t(spawnAt_)].lane;
        else if (!script_.empty()) dest = script_[0].lane;
        target_ = std::clamp(dest, 0, 2);
        bracing_ = mode_ == Mode::Play;
    } else if (mode_ == Mode::Play || mode_ == Mode::Banner) {
        const gs::Pad& pad = sys_->pad;
        int dir = 0;
        if (pad.down(gs::BTN_LEFT) || pad.axisX < -0.45f) dir -= 1;
        if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.45f) dir += 1;
        if (dir != 0) {
            if (dir != holdDir_) {
                holdDir_ = dir;
                holdT_ = 0.f;
                target_ = std::clamp(target_ + dir, 0, 2);
            } else {
                holdT_ += dt;
                if (holdT_ > 0.18f) {
                    target_ = std::clamp(target_ + dir, 0, 2);
                    holdT_ = 0.06f;
                }
            }
        } else {
            holdDir_ = 0;
        }
        bracing_ = mode_ == Mode::Play && (pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) ||
                                            pad.down(gs::BTN_Z) || pad.down(gs::BTN_TURBO) || pad.accel > 0.45f);
    } else {
        bracing_ = false;
    }
    float goal = laneU(target_);
    float d = goal - u_;
    float step = kLaneV * dt;
    if (std::fabs(d) <= step) u_ = goal;
    else u_ += std::copysign(step, d);
}

void Game::fadeFx(float dt) {
    for (Puff& u : puffs_) u.age += dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& u) { return u.age >= u.life; }),
                 puffs_.end());
    if (thrustCd_ > 0.f) thrustCd_ = std::max(0.f, thrustCd_ - dt);
    if (thrustT_ > 0.f) thrustT_ = std::max(0.f, thrustT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.4f);
}

void Game::updatePlay(float dt) {
    t_ += dt;
    tWave_ += dt;
    while (spawnAt_ < int(script_.size()) && script_[size_t(spawnAt_)].t <= tWave_) {
        const Spawn& s = script_[size_t(spawnAt_)];
        Foe f;
        f.lane = s.lane;
        f.kind = s.kind;
        f.hp = s.kind == 2 ? 2 : 1;
        f.z = kSpawn;
        f.speed = s.speed;
        f.alive = true;
        foes_.push_back(f);
        ++spawnAt_;
    }
    aim(dt);
    int lane = coveredLane();
    for (Foe& f : foes_) {
        if (!f.alive) continue;
        if (f.flash > 0.f) f.flash -= dt;
        bool cover = lane == f.lane;
        if (cover && f.z <= kStop && f.z >= kPast) {
            f.z = kStop;
            f.bash += dt;
        } else {
            f.z -= f.speed * dt;
            if (!cover) f.bash = 0.f;
        }
        f.anim += dt * (f.kind == 2 ? 4.f : 8.f);
    }
    if (bracing_) thrust(lane);
    if (!over_) {
        for (Foe& f : foes_) {
            if (!f.alive) continue;
            bool cover = coveredLane() == f.lane;
            if (cover && f.z <= kStop + 0.02f && f.bash >= bashLimit(f.kind)) breach(f);
            else if (f.z < kPast) breach(f);
            if (over_) break;
        }
    }
    if (over_) return;
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.alive; }), foes_.end());
    if (spawnAt_ < int(script_.size())) return;
    for (const Foe& f : foes_)
        if (f.alive) return;
    if (stones_ <= 0) lose();
    else if (wave_ >= 2) win();
    else {
        nextWave_ = wave_ + 1;
        mode_ = Mode::Banner;
        bannerT_ = 1.35f;
        buildScript(nextWave_);
        spawnAt_ = 0;
        foes_.clear();
        blip(660.f);
    }
}

void Game::updateBanner(float dt) {
    t_ += dt;
    aim(dt);
    bannerT_ -= dt;
    if (bannerT_ <= 0.f) {
        wave_ = nextWave_;
        startWave();
        mode_ = Mode::Play;
        blip(392.f);
    }
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    bool watch = mode_ == Mode::Play;
    sys_->apu.noise(watch ? 0.016f : 0.006f, 620.f, false);
    if (fanStep_ >= 0 && mode_ == Mode::Victory) {
        fanT_ += kDt;
        if (fanT_ >= 0.14f) {
            static const float notes[] = {392.f, 523.3f, 659.3f, 784.f};
            if (fanStep_ < 4) sys_->apu.tone(1, notes[fanStep_], 0.07f);
            else sys_->apu.tone(1, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > 7) fanStep_ = -1;
        }
    } else if (watch) {
        sys_->apu.tone(2, 49.f, 0.018f);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (!(h > 1.5f) || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 4, 8);
    uint16_t skyHor = gs::rgb4(13, 9, 6);
    if (mode_ == Mode::Fail) {
        skyTop = gs::rgb4(5, 1, 2);
        skyHor = gs::rgb4(10, 4, 3);
    } else if (mode_ == Mode::Victory) {
        skyTop = gs::rgb4(3, 5, 9);
        skyHor = gs::rgb4(15, 12, 7);
    }
    v.setFogColor(skyHor);
    hor_ = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor_) {
            float t = float(y) / float(std::max(hor_, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor_);
        float z = kZScale / std::max(row, 0.5f);
        r.on = true;
        r.cx = 160.f + bendAt(row) + shx_;
        r.hw = halfAt(row);
        r.v = z * 68.f;
        r.pal = uint8_t(PAL_FIELD);
        r.style = gs::ROAD_ROCKY;
        r.band = (int(std::floor(z * 0.35f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(10.f - row * 0.085f), 0, 10));
        float dropT = std::clamp(row / 120.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), dropT);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 71.f) * 4.2f * shake_;
        shy_ = std::cos(t_ * 53.f) * 2.4f * shake_;
    }
    layRoad();
    int glint = 9 + int(std::sin(t_ * 3.1f) * 4.f);
    v.setColor(PAL_STONE * 16 + 7, gs::rgb4(4, std::clamp(glint, 6, 15), 14));
    int lamp = (int(t_ * 4.f) & 1) ? 15 : 9;
    v.setColor(PAL_WOOD * 16 + 1, gs::rgb4(15, lamp, 4));

    if (mode_ == Mode::Title) text("ONE RIDGE", 160.f + shx_, 28.f, 1.05f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("THE WELL STANDS", 160.f + shx_, 26.f, 0.78f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text("THE WELL FELL", 160.f + shx_, 26.f, 0.9f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx_, 28.f, 1.0f, PAL_AMBER);
    else if (mode_ == Mode::Banner) {
        char line[24];
        std::snprintf(line, sizeof line, "WAVE %d", nextWave_ + 1);
        text(line, 160.f + shx_, 24.f, 0.9f, PAL_AMBER);
    }

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(48);
    items.push_back({kPikeZ, 3, 0});
    items.push_back({kWellZ, 4, 0});
    if (mode_ != Mode::Title) {
        for (int i = 0; i < int(foes_.size()); ++i)
            if (foes_[size_t(i)].alive) items.push_back({foes_[size_t(i)].z, 0, i});
    } else {
        items.push_back({8.6f, 5, 0});
        items.push_back({12.2f, 5, 1});
        items.push_back({16.8f, 5, 2});
    }
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 2, i});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    auto drawFoe = [&](int kind, int frame, int lane, float z, float flash, int hp) {
        Spot s = spot(laneU(lane), z, kind == 2 ? 72.f : (kind == 1 ? 86.f : 80.f));
        if (!s.ok) return;
        const gs::Mipped* m = art_.runner;
        int pal = PAL_FOE;
        if (kind == 1) m = art_.club;
        else if (kind == 2) {
            m = art_.ram;
            pal = PAL_RAM;
            frame = hp < 2 ? 1 : frame;
        }
        if (flash > 0.f) pal = PAL_FX;
        spr(art_.shadow, s.x, s.y, s.h * 0.28f, PAL_FX, false, s.fog, false);
        spr(m[frame & 1], s.x, s.y, s.h, pal, false, s.fog, true);
    };

    for (const Item& it : items) {
        if (it.kind == 3) {
            bool thrust = bracing_ || thrustT_ > 0.f;
            int fr = thrust ? 1 : 0;
            Spot s = spot(u_, kPikeZ, 70.f);
            if (!s.ok) continue;
            spr(art_.shadow, s.x, s.y, s.h * 0.32f, PAL_FX, false, 0, false);
            spr(art_.warden[fr], s.x, s.y, s.h, PAL_YOU, false, 0, true);
            if (thrustT_ > 0.03f) {
                Spot q = spot(u_, kPikeZ + 0.55f, 28.f);
                if (q.ok) spr(art_.shock, q.x, q.y - q.h, q.h, PAL_FX, false, 0, false);
            }
            continue;
        }
        if (it.kind == 4) {
            float stand = stones_ <= 0 ? 0.7f : (0.62f + 0.38f * float(stones_) / float(kStones));
            Spot s = spot(kWellU, kWellZ, 118.f * (mode_ == Mode::Fail ? 0.72f : stand));
            if (!s.ok) continue;
            spr(art_.shadow, s.x, s.y, s.h * 0.3f, PAL_FX, false, 0, false);
            if (stones_ <= 0 || mode_ == Mode::Fail) {
                spr(art_.rubble, s.x, s.y, s.h * 0.55f, PAL_STONE, false, 0, true);
            } else {
                spr(art_.well, s.x, s.y, s.h, PAL_STONE, false, 0, true);
                int cracks = kStones - stones_;
                for (int c = 0; c < cracks; ++c) {
                    float ox = (c == 1 ? 10.f : (c == 2 ? -12.f : 2.f));
                    float oy = (c == 0 ? 0.35f : (c == 1 ? 0.55f : 0.2f));
                    spr(art_.crack, s.x + ox, s.y - s.h * oy, s.h * 0.28f, PAL_STONE, c == 2, 0, false);
                }
                if (stones_ > 1) {
                    float swing = std::sin(t_ * 1.6f) * 5.f;
                    spr(art_.bucket, s.x + swing, s.y - s.h * 0.78f, s.h * 0.16f, PAL_STONE, false, 0, false);
                }
            }
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot s = spot(f.u, f.z, 30.f);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, s.x, s.y - u * 10.f, s.h * (0.6f + u), PAL_FX, false, s.fog, false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[size_t(it.id)];
            Spot s = spot(pr.u, pr.z, pr.h);
            if (!s.ok) continue;
            if (pr.kind == 3) {
                spr(art_.stake, s.x, s.y, s.h, PAL_WOOD, false, s.fog, true);
            } else if (pr.kind == 1) {
                spr(art_.cairn, s.x, s.y, s.h, PAL_STONE, pr.u > 0, s.fog, true);
            } else if (pr.kind == 2) {
                float flutter = std::sin(t_ * 3.f + pr.z) * 4.f;
                spr(art_.pennant, s.x + flutter, s.y, s.h, PAL_BANNER, flutter > 0, s.fog, true);
            } else {
                spr(art_.post, s.x, s.y, s.h, PAL_WOOD, pr.u > 0, s.fog, true);
            }
            continue;
        }
        if (it.kind == 5) {
            static const int dk[] = {0, 1, 2};
            static const int dl[] = {0, 2, 1};
            int kind = dk[it.id];
            drawFoe(kind, int(t_ * 6.f) & 1, dl[it.id], it.z, 0.f, 2);
            continue;
        }
        const Foe& f = foes_[size_t(it.id)];
        drawFoe(f.kind, int(f.anim) & 1, f.lane, f.z, f.flash, f.hp);
    }

    spr(art_.peak[0], 46.f + shx_ * 0.2f, float(hor_) + 4.f, 54.f, PAL_MOUNT, false, 3, true);
    spr(art_.peak[1], 262.f + shx_ * 0.2f, float(hor_) + 6.f, 42.f, PAL_MOUNT, false, 4, true);
    float drift = std::fmod(t_ * 5.f, 400.f);
    spr(art_.cloud, drift - 50.f, 22.f, 16.f, PAL_FX, false, 2, false);
    spr(art_.cloud, std::fmod(drift + 210.f, 400.f) - 30.f, 34.f, 12.f, PAL_FX, true, 3, false);
    spr(art_.sun, 248.f, 20.f, 20.f, PAL_FX, false, 0, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "KEEP THE WELL STANDING", PAL_AMBER);
        hudC(23, "THREE WAVES", PAL_GOOD);
        hudC(24, "ARROWS STEP   Z OR C THRUSTS", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THREE WAVES", PAL_GOOD);
        hudC(24, "THEN IT IS DONE", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(25, buf, PAL_HUD);
        hudC(27, "START", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(24, "THE RIDGE LOST THE WELL", PAL_ALERT);
        hudC(27, "START RETRIES", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "WAVE %d/3", std::min(3, (mode_ == Mode::Banner ? nextWave_ : wave_) + 1));
        hud(1, 0, buf, PAL_AMBER);
        std::string pips = "WELL ";
        int show = std::max(0, stones_);
        for (int i = 0; i < kStones; ++i) pips.push_back(i < show ? '#' : '-');
        hud(40 - int(pips.size()) - 1, 0, pips, show > 1 ? PAL_GOOD : PAL_ALERT);
        if (mode_ == Mode::Banner) hudC(2, waveName(nextWave_), PAL_AMBER);
        else if (tWave_ < 1.7f) hudC(2, waveName(wave_), PAL_AMBER);
        else hudC(2, hint(), bracing_ ? PAL_GOOD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 26, buf, PAL_HUD);
        hudC(27, "ARROWS STEP   Z OR C THRUSTS", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.84f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    if (bot_) beginRun();
    else {
        bootTitle();
        sys.setLight(40, 50, 80);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        u_ = std::sin(t_ * 0.7f) * 0.36f;
        target_ = 1;
        bracing_ = std::sin(t_ * 1.3f) > 0.1f;
        if (!bot_ && pad.pressed(gs::BTN_START)) beginRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
        else updatePlay(kDt);
    } else if (mode_ == Mode::Banner) {
        if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
        else updateBanner(kDt);
    } else {
        t_ += kDt;
        if (!bot_ && pad.pressed(gs::BTN_START)) beginRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    fadeFx(kDt);
    serviceAudio();
    draw();
}

}  // namespace rwell
