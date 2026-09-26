#include "game/purs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace purs {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 68.f;
constexpr float kSpan = 152.f;
constexpr float kZNear = 5.6f;
constexpr float kPpm = 32.f;
constexpr float kRoadHalf = 4.0f;
constexpr float kLane = 1.64f;
constexpr float kGateZ = 16.6f;
constexpr float kPlayerZ = 6.35f;
constexpr float kShotZ = 7.15f;
constexpr float kHitZ = 6.2f;
constexpr float kShotV = 28.f;
constexpr float kBoltV = 5.2f;
constexpr float kSteer = 4.0f;
constexpr float kFireCd = 0.20f;
constexpr float kWind = 0.78f;
constexpr float kCool = 1.15f;
constexpr float kBand = 0.52f;
constexpr float kAim = 0.34f;
constexpr float kWeave = 0.16f;
constexpr float kWatch = 64.f;
constexpr int kHull = 6;

float bend(float z) { return std::sin(z * 0.055f) * 0.42f; }

float laneLat(int lane) { return float(std::clamp(lane, -1, 1)) * kLane; }

const char* kindName(int kind) {
    if (kind == 0) return "SCOUT";
    if (kind == 1) return "WAGON";
    return "HEAVY";
}

int lampPal(int kind) {
    if (kind == 0) return PAL_ALERT;
    if (kind == 1) return PAL_GOLD;
    return PAL_INK;
}

float bodyH(int kind) {
    if (kind == 0) return 1.95f;
    if (kind == 1) return 2.45f;
    return 2.35f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (bannerT_ > 0.f && (mode_ == Mode::Play || mode_ == Mode::Pause)) return 2;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::stoppedCount() const {
    int n = 0;
    for (const Machine& m : machines_)
        if (!m.running) n++;
    return n;
}

int Game::rivalsRunning() const {
    int n = 0;
    for (const Machine& m : machines_)
        if (m.running) n++;
    return n;
}

const Game::Machine* Game::quarry() const {
    const Machine* best = nullptr;
    for (const Machine& m : machines_) {
        if (!m.running) continue;
        if (!best || m.z < best->z) best = &m;
    }
    return best;
}

bool Game::laneHot(float lat) const {
    for (const Shot& s : shots_) {
        if (s.from < 0) continue;
        if (s.z <= kHitZ) continue;
        if (std::fabs(s.lat - lat) < kBand) return true;
    }
    return false;
}

void Game::resetWorld() {
    hull_ = kHull;
    t_ = 0;
    watch_ = 0;
    px_ = 0;
    fireCd_ = 0;
    hurtT_ = 0;
    shake_ = 0;
    scroll_ = 0;
    travel_ = 0;
    grace_ = 0;
    bannerT_ = 0;
    banner_ = "";
    blip_ = 0;
    fanStep_ = -1;
    over_ = false;
    won_ = false;
    reason_ = "WATCH OVER";
    shots_.clear();
    puffs_.clear();
    machines_.clear();
    props_.clear();

    auto add = [&](int kind, int lane, float z, int hp, float phase, float hold) {
        Machine m;
        m.kind = kind;
        m.lane = lane;
        m.dir = lane >= 0 ? -1 : 1;
        m.hp = hp;
        m.maxHp = hp;
        m.lat = laneLat(lane);
        m.z = z;
        m.hold = hold;
        m.phase = phase;
        m.running = true;
        machines_.push_back(m);
    };
    add(0, -1, 8.15f, 3, 0.4f, 22.f);
    add(1, 1, 10.7f, 4, 1.7f, 24.f);
    add(2, 0, 12.4f, 8, 2.6f, 26.f);

    auto prop = [&](int kind, float lat, float z) {
        props_.push_back(Prop{lat, z, kind});
    };
    prop(0, -6.1f, 11.2f);
    prop(0, 6.3f, 13.4f);
    prop(0, -6.4f, 16.8f);
    prop(0, 5.9f, 18.6f);
    prop(1, -4.55f, 9.2f);
    prop(1, 4.65f, 11.6f);
    prop(1, -4.7f, 14.4f);
    prop(1, 4.5f, 17.0f);
}

void Game::bootTitle() {
    resetWorld();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetWorld();
    mode_ = Mode::Play;
    if (sys_) sys_->setLight(40, 70, 120);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    if (bot_) begin();
    else bootTitle();
}

void Game::launchPlayer() {
    if (fireCd_ > 0.f) return;
    int n = 0;
    for (const Shot& s : shots_)
        if (s.from < 0) n++;
    if (n >= 4) return;
    Shot s;
    s.lat = px_;
    s.z = s.prev = kShotZ;
    s.from = -1;
    shots_.push_back(s);
    fireCd_ = kFireCd;
    blip_ = 0.05f;
    sys_->apu.tone(2, 920.f, 0.05f);
}

void Game::launchBolt(int index) {
    Machine& m = machines_[size_t(index)];
    for (const Shot& s : shots_)
        if (s.from == index) return;
    Shot s;
    s.lat = m.lat;
    s.z = s.prev = m.z - 0.15f;
    s.from = index;
    shots_.push_back(s);
    m.wind = 0;
    m.cool = kCool;
    sys_->apu.tone(2, 180.f, 0.05f);
}

void Game::hurtPlayer() {
    if (hurtT_ > 0.f || hull_ <= 0) return;
    hull_--;
    hurtT_ = 0.45f;
    shake_ = 1.f;
    sys_->rumble(0.7f, 0.9f, 140);
    sys_->apu.noiseBurst(0.45f, 280.f, 0.12f);
    sys_->apu.tone(0, 90.f, 0.07f);
}

void Game::stopMachine(Machine& m) {
    if (!m.running) return;
    m.running = false;
    m.hp = 0;
    m.wind = 0;
    m.flash = 0.2f;
    banner_ = kindName(m.kind);
    bannerT_ = 1.15f;
    shake_ = std::max(shake_, 0.45f);
    sys_->apu.noiseBurst(0.4f, 160.f, 0.16f);
    sys_->apu.tone(0, 140.f, 0.06f);
    for (int i = 0; i < 4; i++) {
        Puff p;
        p.lat = m.lat + float(i - 1) * 0.18f;
        p.z = m.z - 0.2f;
        p.life = 0.55f + float(i) * 0.08f;
        p.kind = i == 0 ? 1 : 0;
        puffs_.push_back(p);
    }
}

void Game::winWatch() {
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    shake_ = 0;
    sys_->rumble(0.3f, 0.5f, 180);
    sys_->setLight(40, 180, 70);
}

void Game::loseWatch(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Victory) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    hull_ = std::max(0, hull_);
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(180, 30, 20);
}

void Game::botGoal(float& goal, bool& fire) {
    const Machine* q = quarry();
    goal = q ? q->lat : 0.f;
    fire = false;
    bool blocked = laneHot(px_) || (q && laneHot(q->lat));
    if (blocked) {
        float prefer = q ? q->lat : 0.f;
        float best = laneLat(-1);
        float bestD = 1e9f;
        bool found = false;
        for (int lane = -1; lane <= 1; lane++) {
            float x = laneLat(lane);
            if (laneHot(x)) continue;
            float d = std::fabs(x - prefer);
            if (!found || d < bestD) {
                found = true;
                bestD = d;
                best = x;
            }
        }
        goal = found ? best : laneLat(px_ >= 0.f ? -1 : 1);
    }
    if (q && !blocked && std::fabs(px_ - q->lat) < kAim) fire = true;
}

void Game::update(float dt) {
    t_ += dt;
    if (fireCd_ > 0.f) fireCd_ = std::max(0.f, fireCd_ - dt);
    if (hurtT_ > 0.f) hurtT_ = std::max(0.f, hurtT_ - dt);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt * 1.6f);
    if (bannerT_ > 0.f) bannerT_ = std::max(0.f, bannerT_ - dt);
    if (blip_ > 0.f) {
        blip_ -= dt;
        if (blip_ <= 0.f && fanStep_ < 0) sys_->apu.tone(2, 0.f, 0.f);
    }

    float drive = mode_ == Mode::Play ? 16.f : 5.f;
    if (mode_ == Mode::Pause) drive = 0.f;
    scroll_ += drive * dt * 3.f;
    travel_ += drive * dt;

    if (mode_ == Mode::Title || mode_ == Mode::Play) {
        for (Machine& m : machines_) {
            if (!m.running) {
                float side = m.lat < -0.05f ? -1.f : 1.f;
                if (std::fabs(m.lat) <= 0.05f) side = (m.kind & 1) ? 1.f : -1.f;
                float shoulder = side * (kRoadHalf + 0.7f);
                float d = shoulder - m.lat;
                float step = 1.4f * dt;
                if (std::fabs(d) <= step) m.lat = shoulder;
                else m.lat += std::copysign(step, d);
                m.wreck += dt;
                if (int(m.wreck * 8.f) % 3 == 0 && puffs_.size() < 16) {
                    Puff p;
                    p.lat = m.lat;
                    p.z = m.z - 0.3f;
                    p.life = 0.45f;
                    puffs_.push_back(p);
                }
                continue;
            }
            if (mode_ == Mode::Play) {
                m.hold -= dt;
                if (m.hold <= 0.f) {
                    m.hold = 6.5f;
                    m.lane += m.dir;
                    if (m.lane > 1) {
                        m.lane = 1;
                        m.dir = -1;
                    } else if (m.lane < -1) {
                        m.lane = -1;
                        m.dir = 1;
                    }
                }
            }
            float bob = std::sin(t_ * 0.85f + m.phase) * (mode_ == Mode::Title ? 0.28f : kWeave);
            float want = laneLat(m.lane) + bob;
            float d = want - m.lat;
            float step = 1.15f * dt;
            if (std::fabs(d) <= step) m.lat = want;
            else m.lat += std::copysign(step, d);
            if (m.flash > 0.f) m.flash = std::max(0.f, m.flash - dt);
            if (int((t_ + m.phase) * 10.f) % 5 == 0 && puffs_.size() < 14) {
                Puff p;
                p.lat = m.lat;
                p.z = m.z - 0.45f;
                p.life = 0.28f;
                puffs_.push_back(p);
            }
        }
    }

    for (Puff& p : puffs_) p.age += dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }), puffs_.end());

    if (mode_ != Mode::Play) {
        serviceAudio();
        return;
    }

    watch_ += dt;

    float goal = 0;
    bool wantFire = false;
    if (bot_) {
        if (grace_ <= 0.f) botGoal(goal, wantFire);
    } else {
        const gs::Pad& pad = sys_->pad;
        float digital = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        float axis = std::fabs(pad.axisX) > 0.18f ? pad.axisX : digital;
        goal = px_ + std::clamp(axis, -1.f, 1.f) * kSteer * 2.f;
        wantFire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_TURBO) || pad.accel > 0.45f;
    }
    {
        float d = std::clamp(goal, -1.95f, 1.95f) - px_;
        float step = kSteer * dt;
        if (std::fabs(d) <= step) px_ = std::clamp(goal, -1.95f, 1.95f);
        else px_ += std::copysign(step, d);
    }

    if (wantFire && grace_ <= 0.f) launchPlayer();

    std::vector<Shot> keep;
    keep.reserve(shots_.size());
    for (Shot s : shots_) {
        s.prev = s.z;
        if (s.from < 0) {
            s.z += kShotV * dt;
            int hit = -1;
            for (int i = 0; i < int(machines_.size()); i++) {
                Machine& m = machines_[size_t(i)];
                if (!m.running) continue;
                if (std::fabs(m.lat - s.lat) > kBand) continue;
                if (s.prev - 0.05f <= m.z && s.z + 0.2f >= m.z) {
                    if (hit < 0 || m.z < machines_[size_t(hit)].z) hit = i;
                }
            }
            if (hit >= 0) {
                Machine& m = machines_[size_t(hit)];
                m.hp--;
                m.flash = 0.1f;
                Puff p;
                p.lat = m.lat;
                p.z = m.z;
                p.life = 0.22f;
                p.kind = 1;
                puffs_.push_back(p);
                sys_->apu.noiseBurst(0.18f, 900.f, 0.04f);
                if (m.hp <= 0) stopMachine(m);
            } else if (s.z < kGateZ - 0.3f) {
                keep.push_back(s);
            }
        } else {
            s.z -= kBoltV * dt;
            bool crossed = s.prev > kHitZ && s.z <= kHitZ;
            if (crossed && std::fabs(s.lat - px_) < kBand) hurtPlayer();
            if (s.z > kHitZ - 0.05f) keep.push_back(s);
        }
    }
    shots_.swap(keep);

    if (grace_ <= 0.f) {
        for (int i = 0; i < int(machines_.size()); i++) {
            Machine& m = machines_[size_t(i)];
            if (!m.running) continue;
            if (m.cool > 0.f) m.cool = std::max(0.f, m.cool - dt);
            bool aligned = std::fabs(px_ - m.lat) < kBand && m.cool <= 0.f;
            if (aligned) m.wind += dt;
            else m.wind = std::max(0.f, m.wind - dt * 0.45f);
            if (m.wind >= kWind) launchBolt(i);
        }
    }

    if (hull_ <= 0) loseWatch("YOUR MACHINE STOPPED");
    else if (rivalsRunning() == 0) {
        grace_ += dt;
        if (grace_ > 0.5f) winWatch();
    } else if (watch_ >= kWatch) {
        loseWatch("THE WATCH RAN OUT");
    }

    if (mode_ == Mode::Play) {
        if (hull_ <= 2) sys_->setLight(200, 40, 30);
        else if (laneHot(px_)) sys_->setLight(200, 140, 30);
        else sys_->setLight(50, 90, 150);
    }

    serviceAudio();
}

void Game::serviceAudio() {
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.14f) {
            static const float good[] = {392.f, 523.f, 659.f, 784.f};
            static const float bad[] = {220.f, 174.f, 130.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
        sys_->apu.tone(1, 0.f, 0.f);
        return;
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title) {
        float wob = std::sin(t_ * 36.f) * 4.f;
        sys_->apu.tone(1, 52.f + wob, 0.03f);
    } else {
        sys_->apu.tone(1, 0.f, 0.f);
    }
    if (mode_ == Mode::Play && laneHot(px_) && blip_ <= 0.f) sys_->apu.tone(2, 160.f, 0.03f);
    else if (blip_ <= 0.f && mode_ != Mode::Play) sys_->apu.tone(2, 0.f, 0.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START)) begin();
        else update(kDt);
    } else if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(kDt);
    } else {
        update(kDt);
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
    }
    draw();
}

Game::Proj Game::project(float lat, float z) const {
    Proj p;
    if (!(z > kZNear)) return p;
    float t = kZNear / z;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * kSpan;
    p.x = 160.f + (bend(z) + lat) * p.ppm;
    p.ok = true;
    return p;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float f = std::clamp((0.18f - t) / 0.18f, 0.f, 1.f);
    return int(f * 12.f);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.2f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 24 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) {
            width += 10.f * scale;
            continue;
        }
        width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw;
    }
}

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t fog = mode_ == Mode::Fail ? gs::rgb4(8, 2, 2) : gs::rgb4(3, 2, 4);
    v.setFogColor(fog);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int r = std::clamp(int(1 + u * u * 9.f), 0, 15);
            int g = std::clamp(int(1 + u * 3.f), 0, 15);
            int b = std::clamp(int(6 - u * 3.f), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) - kHorizon) / kSpan;
        if (t < 0.004f) t = 0.004f;
        float wz = kZNear / t;
        float ppm = kPpm * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bend(wz) * ppm + shx;
        rd.hw = std::max(2.f, kRoadHalf * ppm);
        rd.v = wz * 6.f - scroll_;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor((wz - travel_) * 0.22f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.16f - t) / 0.16f, 0.f, 1.f);
        int f = int(fogT * 10.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + 3);
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(2, 2, 2);
    }
}

void Game::drawGate(float shx) {
    Proj L = project(-(kRoadHalf + 0.15f), kGateZ);
    Proj R = project(kRoadHalf + 0.15f, kGateZ);
    if (!L.ok || !R.ok) return;
    int fog = fogFor(kGateZ);
    float th = std::clamp(8.6f * L.ppm, 28.f, 120.f);
    spr(art_.tower, L.x + shx, L.y, th, PAL_STONE, false, fog, true, false);
    spr(art_.tower, R.x + shx, R.y, th, PAL_STONE, true, fog, true, false);
    float midX = (L.x + R.x) * 0.5f + shx;
    float gap = std::fabs(R.x - L.x);
    float grateH = std::clamp(3.1f * L.ppm, 12.f, 64.f);
    float beamH = std::clamp(th * 0.13f, 6.f, 16.f);
    float beamW = gap * 1.08f;
    {
        gs::Sprite s;
        s.w = int16_t(std::clamp(int(std::lround(beamW)), 1, 400));
        s.h = int16_t(std::clamp(int(std::lround(beamH)), 1, 80));
        s.x = int16_t(std::lround(midX - s.w * 0.5f));
        s.y = int16_t(std::lround(L.y - th * 0.82f));
        s.img = art_.beam.pick(std::max(beamW, beamH));
        s.pal = PAL_STONE;
        s.fog = uint8_t(fog);
        sys_->vdp.sprite(s);
    }
    {
        gs::Sprite s;
        s.w = int16_t(std::clamp(int(std::lround(gap * 0.94f)), 1, 400));
        s.h = int16_t(std::clamp(int(std::lround(grateH)), 1, 160));
        s.x = int16_t(std::lround(midX - s.w * 0.5f));
        s.y = int16_t(std::lround(L.y - grateH * 0.92f));
        s.img = art_.grate.pick(std::max(gap, grateH));
        s.pal = PAL_STONE;
        s.fog = uint8_t(fog);
        sys_->vdp.sprite(s);
    }
    float banH = std::clamp(th * 0.22f, 8.f, 28.f);
    spr(art_.banner, midX, L.y - th * 0.62f, banH, PAL_STONE, false, fog, false, false);
    int flick = int(t_ * 12.f) & 1;
    float flameH = std::clamp(th * 0.16f, 6.f, 20.f);
    spr(art_.flame[flick], L.x + shx, L.y - th * 0.92f, flameH, PAL_FX, false, 0, false, false);
    spr(art_.flame[flick ^ 1], R.x + shx, R.y - th * 0.9f, flameH * 0.9f, PAL_FX, true, 0, false, false);
}

void Game::drawMachine(const Machine& m, float shx, bool you) {
    float z = you ? kPlayerZ : m.z;
    float lat = you ? px_ : m.lat;
    Proj p = project(lat, z);
    if (!p.ok) return;
    int fog = you ? 0 : fogFor(z);
    const gs::Mipped* body = &art_.heavy;
    int pal = PAL_HEAVY;
    float wh = 2.35f;
    if (you) {
        body = &art_.you;
        pal = PAL_YOU;
        wh = 1.9f;
    } else if (m.kind == 0) {
        body = &art_.scout;
        pal = PAL_SCOUT;
        wh = bodyH(0);
    } else if (m.kind == 1) {
        body = &art_.wagon;
        pal = PAL_WAGON;
        wh = bodyH(1);
    }
    float h = std::clamp(wh * p.ppm, 8.f, you ? 78.f : 70.f);
    float w = h * float(body->w) / float(body->h);
    spr(art_.shadow, p.x + shx, p.y, h * 0.28f, PAL_FX, false, 0, false, true);
    bool blink = you && hurtT_ > 0.f && (int(t_ * 24.f) & 1);
    if (!blink) spr(*body, p.x + shx, p.y, h, pal, false, fog, true, false);

    bool lampsOn = you ? mode_ != Mode::Fail : m.running;
    if (!you && m.running && m.wind > 0.35f && (int(t_ * 18.f) & 1)) lampsOn = false;
    if (you && mode_ == Mode::Play && (int(t_ * 8.f) & 1)) {
        spr(art_.lamp, p.x + shx, p.y - h * 0.86f, h * 0.12f, PAL_SHOT, false, 0, false, false);
    }
    if (lampsOn && !blink) {
        int lp = you ? PAL_SHOT : lampPal(m.kind);
        float yf = you ? 0.62f : (m.kind == 1 ? 0.70f : 0.58f);
        float xf = m.kind == 2 && !you ? 0.30f : 0.24f;
        spr(art_.lamp, p.x + shx - w * xf, p.y - h * (1.f - yf), h * 0.12f, lp, false, fog, false, false);
        spr(art_.lamp, p.x + shx + w * xf, p.y - h * (1.f - yf), h * 0.12f, lp, false, fog, false, false);
    }
    if (!you && !m.running) {
        int flick = int(t_ * 14.f + m.phase) & 1;
        spr(art_.flame[flick], p.x + shx, p.y - h * 0.55f, h * 0.38f, PAL_FX, false, fog, false, false);
    }
    if (m.flash > 0.f || (you && hurtT_ > 0.25f))
        spr(art_.spark, p.x + shx, p.y - h * 0.45f, h * 0.28f, PAL_BOLT, false, fog, false, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 90.f) * 5.f * std::min(shake_, 1.f);
    road(shx);

    if (mode_ == Mode::Title) {
        text("GATE PURSUIT", 160.f + shx, 36.f, 1.05f, PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        text("LAST MACHINE", 160.f + shx, 28.f, 0.95f, PAL_GOOD);
        text("STILL RUNNING", 160.f + shx, 52.f, 0.95f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text("WATCH OVER", 160.f + shx, 34.f, 1.05f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f + shx, 34.f, 1.1f, PAL_GOLD);
    }

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.push_back({kPlayerZ, 0, 0});
    for (int i = 0; i < int(machines_.size()); i++) items.push_back({machines_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(shots_.size()); i++) items.push_back({shots_[size_t(i)].z, 2, i});
    for (int i = 0; i < int(puffs_.size()); i++) items.push_back({puffs_[size_t(i)].z, 3, i});
    for (int i = 0; i < int(props_.size()); i++) items.push_back({props_[size_t(i)].z, 4, i});
    items.push_back({kGateZ, 5, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 0) {
            Machine ghost;
            drawMachine(ghost, shx, true);
            continue;
        }
        if (it.kind == 1) {
            drawMachine(machines_[size_t(it.id)], shx, false);
            continue;
        }
        if (it.kind == 2) {
            const Shot& s = shots_[size_t(it.id)];
            Proj p = project(s.lat, s.z);
            if (!p.ok) continue;
            float h = std::clamp(0.7f * p.ppm, 4.f, 18.f);
            if (s.from < 0) spr(art_.shot, p.x + shx, p.y - h, h, PAL_SHOT, false, fogFor(s.z), false, false);
            else spr(art_.bolt, p.x + shx, p.y - h * 0.4f, h, PAL_BOLT, false, fogFor(s.z), false, false);
            continue;
        }
        if (it.kind == 3) {
            const Puff& f = puffs_[size_t(it.id)];
            Proj p = project(f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(p.ppm * (0.55f + u * 0.8f), 3.f, 22.f);
            const gs::Mipped& img = f.kind == 1 ? art_.spark : art_.puff;
            int pal = f.kind == 1 ? PAL_BOLT : PAL_FX;
            spr(img, p.x + shx, p.y - u * 8.f, h, pal, false, fogFor(f.z), false, false);
            continue;
        }
        if (it.kind == 4) {
            const Prop& pr = props_[size_t(it.id)];
            Proj p = project(pr.lat, pr.z);
            if (!p.ok) continue;
            int fog = fogFor(pr.z);
            if (pr.kind == 0) {
                float h = std::clamp(3.3f * p.ppm, 8.f, 80.f);
                spr(art_.tree, p.x + shx, p.y, h, PAL_TREE, pr.lat > 0, fog, true, false);
            } else {
                float h = std::clamp(2.3f * p.ppm, 8.f, 64.f);
                spr(art_.post, p.x + shx, p.y, h, PAL_STONE, false, fog, true, false);
                spr(art_.lamp, p.x + shx, p.y - h * 0.92f, h * 0.16f, PAL_GOLD, false, fog, false, false);
            }
            continue;
        }
        drawGate(shx);
    }

    float moonX = 36.f + std::sin(t_ * 0.15f) * 2.f + shx;
    spr(art_.moon, moonX, 22.f, 22.f, PAL_NIGHT, false, 0, false, false);
    static const float kStar[][2] = {{18, 12}, {58, 18}, {96, 10}, {124, 20}, {250, 14}, {286, 22}, {304, 9}, {210, 12}};
    for (int i = 0; i < 8; i++) {
        if (((int(t_ * 3.f) + i * 3) % 17) == 0) continue;
        spr(art_.star, kStar[i][0] + shx, kStar[i][1], 4.f, PAL_NIGHT, false, 0, false, false);
    }

    if (mode_ == Mode::Title) {
        hudC(22, "AT THE GATE", PAL_INK);
        hudC(23, "BE THE LAST MACHINE STILL RUNNING", PAL_GOLD);
        hudC(24, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(25, "ARROWS STEER    C FIRES", PAL_INK);
        if ((sys_->frame / 30) % 2 == 0) hudC(26, "ENTER STARTS", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(26, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Victory) {
        hudC(1, "LAST MACHINE STILL RUNNING", PAL_GOOD);
        hudC(2, "THE WATCH IS YOURS", PAL_GOLD);
        if ((sys_->frame / 30) % 2 == 0) hudC(26, "ENTER", PAL_INK);
    } else if (mode_ == Mode::Fail) {
        hudC(1, "WATCH OVER", PAL_ALERT);
        hudC(2, reason_, PAL_INK);
        hudC(26, "ENTER RETRIES", PAL_GOLD);
    } else {
        std::string pips = "HULL ";
        for (int i = 0; i < kHull; i++) pips += (i < hull_) ? '#' : '-';
        hud(1, 0, pips, hull_ <= 2 ? PAL_ALERT : PAL_INK);
        char buf[48];
        std::snprintf(buf, sizeof buf, "RIVALS %d", rivalsRunning());
        hud(16, 0, buf, PAL_GOLD);
        int left = std::max(0, int(std::ceil(kWatch - watch_)));
        std::snprintf(buf, sizeof buf, "WATCH %d:%02d", left / 60, left % 60);
        hud(28, 0, buf, left <= 15 ? PAL_ALERT : PAL_INK);
        std::string row;
        for (const Machine& m : machines_) {
            if (!row.empty()) row += "  ";
            row += kindName(m.kind);
            row += " ";
            if (m.running) row += std::to_string(m.hp);
            else row += "--";
        }
        hudC(1, row, PAL_INK);
        if (bannerT_ > 0.f) {
            std::string b = std::string(banner_) + " STOPPED";
            hudC(2, b, PAL_GOOD);
        }
        const char* hint = "PURSUE THEIR LANE";
        int hpal = PAL_INK;
        if (laneHot(px_)) {
            hint = "OFF THE LANE";
            hpal = PAL_ALERT;
        } else if (const Machine* q = quarry()) {
            if (std::fabs(px_ - q->lat) < kAim) {
                hint = "FIRE";
                hpal = PAL_GOLD;
            }
        }
        hudC(26, hint, hpal);
        hudC(27, "ARROWS STEER   C FIRES", PAL_INK);
    }
}

}  // namespace purs
