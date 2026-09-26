#include "game/purs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace ridgepurs {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 64.f;
constexpr float kSpan = 156.f;
constexpr float kZNear = 5.2f;
constexpr float kPpm = 46.f;
constexpr float kRoadHalf = 2.72f;
constexpr float kLane = 1.12f;
constexpr float kPlayerZ = 6.15f;
constexpr float kShotZ = 6.75f;
constexpr float kHitZ = 6.15f;
constexpr float kContact = 7.2f;
constexpr float kShotV = 32.f;
constexpr float kBoltV = 5.4f;
constexpr float kSteer = 5.4f;
constexpr float kFireCd = 0.16f;
constexpr float kBand = 0.76f;
constexpr float kAim = 0.46f;
constexpr float kWeave = 0.1f;
constexpr float kWatch = 48.f;
constexpr float kLip = 1.92f;
constexpr float kSlipDie = 0.68f;
constexpr float kBotLimit = 1.32f;
constexpr int kHull = 8;

float laneLat(int lane) { return float(std::clamp(lane, -1, 1)) * kLane; }

const char* kindName(int kind) {
    if (kind == 0) return "CRAWLER";
    if (kind == 1) return "DRAY";
    return "HAULER";
}

int kindPal(int kind) {
    if (kind == 0) return PAL_CRAWL;
    if (kind == 1) return PAL_DRAY;
    return PAL_HAUL;
}

float bodyH(int kind) {
    if (kind == 0) return 1.55f;
    if (kind == 1) return 2.35f;
    return 2.05f;
}

float closingOf(int kind) {
    if (kind == 0) return 0.145f;
    if (kind == 1) return 0.112f;
    return 0.09f;
}

float windOf(int kind) {
    if (kind == 0) return 0.78f;
    if (kind == 1) return 0.98f;
    return 1.16f;
}

float coolOf(int kind) {
    if (kind == 0) return 1.05f;
    if (kind == 1) return 1.28f;
    return 1.5f;
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

int Game::quarry() const {
    int best = -1;
    for (int i = 0; i < int(machines_.size()); i++) {
        const Machine& m = machines_[size_t(i)];
        if (!m.running) continue;
        if (best < 0 || m.z < machines_[size_t(best)].z) best = i;
    }
    return best;
}

bool Game::laneHot(float lat, float reach) const {
    for (const Shot& s : shots_) {
        if (s.from < 0) continue;
        if (s.z <= kHitZ) continue;
        if (s.z > kPlayerZ + reach) continue;
        if (std::fabs(s.lat - lat) < kBand) return true;
    }
    return false;
}

float Game::bendAt(float z) const { return std::sin(z * 0.05f + travel_ * 0.035f) * 0.62f; }

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
    arm_ = 0.55f;
    bannerT_ = 0;
    banner_ = "";
    blip_ = 0;
    slip_ = 0;
    pull_ = 0;
    fanStep_ = -1;
    over_ = false;
    won_ = false;
    reason_ = "NOT THE LAST";
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
    add(0, -1, 9.6f, 2, 0.3f, 1.7f);
    add(1, 1, 12.4f, 3, 1.4f, 2.2f);
    add(2, 0, 15.6f, 4, 2.5f, 2.6f);

    auto prop = [&](int kind, float lat, float z) { props_.push_back(Prop{lat, z, kind}); };
    prop(0, -4.3f, 8.4f);
    prop(0, 4.5f, 11.2f);
    prop(0, -4.6f, 14.6f);
    prop(0, 4.2f, 17.8f);
    prop(1, -3.55f, 9.1f);
    prop(1, 3.6f, 12.0f);
    prop(1, -3.7f, 15.2f);
    prop(1, 3.45f, 18.4f);
}

void Game::bootTitle() {
    resetWorld();
    mode_ = Mode::Title;
}

void Game::begin() {
    resetWorld();
    mode_ = Mode::Play;
    if (sys_) sys_->setLight(70, 90, 140);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    buildArt(sys.vdp, art_);
    if (bot_) begin();
    else bootTitle();
}

void Game::launchPlayer() {
    if (fireCd_ > 0.f) return;
    int n = 0;
    for (const Shot& s : shots_)
        if (s.from < 0) n++;
    if (n >= 3) return;
    Shot s;
    s.lat = px_;
    s.z = s.prev = kShotZ;
    s.from = -1;
    shots_.push_back(s);
    fireCd_ = kFireCd;
    blip_ = 0.05f;
    sys_->apu.tone(2, 880.f, 0.05f);
}

void Game::launchBolt(int index) {
    Machine& m = machines_[size_t(index)];
    for (const Shot& s : shots_)
        if (s.from == index) return;
    Shot s;
    s.lat = m.lat;
    s.z = s.prev = m.z - 0.2f;
    s.from = index;
    shots_.push_back(s);
    m.wind = 0;
    m.cool = coolOf(m.kind);
    sys_->apu.tone(2, 160.f, 0.05f);
}

void Game::hurtPlayer(int dmg) {
    if (hurtT_ > 0.f || hull_ <= 0 || dmg <= 0) return;
    hull_ = std::max(0, hull_ - dmg);
    hurtT_ = 0.48f;
    shake_ = 1.f;
    sys_->rumble(0.65f, 0.85f, 120);
    sys_->apu.noiseBurst(0.42f, 240.f, 0.12f);
    sys_->apu.tone(0, 86.f, 0.07f);
}

void Game::stopMachine(Machine& m) {
    if (!m.running) return;
    m.running = false;
    m.hp = 0;
    m.wind = 0;
    m.flash = 0.18f;
    banner_ = kindName(m.kind);
    bannerT_ = 1.2f;
    shake_ = std::max(shake_, 0.4f);
    sys_->apu.noiseBurst(0.38f, 140.f, 0.16f);
    sys_->apu.tone(0, 128.f, 0.06f);
    for (int i = 0; i < 4; i++) {
        Puff p;
        p.lat = m.lat + float(i - 1) * 0.16f;
        p.z = m.z - 0.15f;
        p.life = 0.5f + float(i) * 0.06f;
        p.kind = i == 0 ? 1 : 0;
        puffs_.push_back(p);
    }
}

void Game::winRidge() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "LAST MACHINE STILL RUNNING";
    fanGood_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    shake_ = 0;
    sys_->rumble(0.25f, 0.45f, 180);
    sys_->setLight(40, 170, 70);
}

void Game::loseRidge(const char* why) {
    if (mode_ == Mode::Fail || mode_ == Mode::Victory) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    fanGood_ = false;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(180, 30, 20);
}

void Game::botPlan(float& goal, bool& fire) {
    int qi = quarry();
    if (qi < 0) {
        goal = 0.f;
        fire = false;
        return;
    }
    const Machine& q = machines_[size_t(qi)];
    bool urgent = watch_ > 20.f;
    float reach = urgent ? 2.6f : 8.f;
    float prefer = q.lat;
    const float lanes[3] = {-kLane, 0.f, kLane};
    goal = prefer;
    bool threatened = laneHot(px_, reach) || laneHot(prefer, reach);
    if (threatened) {
        float best = 0.f;
        float score = 1e9f;
        bool found = false;
        for (float x : lanes) {
            if (laneHot(x, reach)) continue;
            float sc = std::fabs(x) * 1.5f + std::fabs(x - prefer);
            if (!found || sc < score) {
                found = true;
                score = sc;
                best = x;
            }
        }
        goal = found ? best : 0.f;
    }
    goal = std::clamp(goal, -kBotLimit, kBotLimit);
    float aim = urgent ? 0.64f : kAim;
    fire = !laneHot(px_, reach) && std::fabs(px_ - q.lat) < aim;
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
    if (arm_ > 0.f) arm_ = std::max(0.f, arm_ - dt);

    float drive = 4.f;
    if (mode_ == Mode::Play) drive = 14.f + pull_ * 8.f;
    else if (mode_ == Mode::Pause) drive = 0.f;
    scroll_ += drive * dt * 3.f;
    travel_ += drive * dt;

    bool live = mode_ == Mode::Title || mode_ == Mode::Play || mode_ == Mode::Victory || mode_ == Mode::Fail;
    if (live) {
        for (Machine& m : machines_) {
            if (!m.running) {
                float side = m.lat < -0.04f ? -1.f : 1.f;
                if (std::fabs(m.lat) <= 0.04f) side = (m.kind & 1) ? 1.f : -1.f;
                float shoulder = side * (kRoadHalf + 0.85f);
                float d = shoulder - m.lat;
                float step = 1.35f * dt;
                if (std::fabs(d) <= step) m.lat = shoulder;
                else m.lat += std::copysign(step, d);
                m.wreck += dt;
                if (puffs_.size() < 20 && int(m.wreck * 8.f) % 3 == 0) {
                    Puff p;
                    p.lat = m.lat;
                    p.z = m.z;
                    p.life = 0.42f;
                    puffs_.push_back(p);
                }
                continue;
            }
            if (mode_ == Mode::Play) {
                m.z = std::max(kContact - 0.2f, m.z - closingOf(m.kind) * (pull_ > 0.5f ? 0.38f : 1.f) * dt);
                m.hold -= dt;
                if (m.hold <= 0.f) {
                    m.hold = 2.1f + float(m.kind) * 0.35f;
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
            float bob = std::sin(t_ * 0.9f + m.phase) * kWeave;
            float want = laneLat(m.lane) + bob;
            float d = want - m.lat;
            float step = 0.9f * dt;
            if (std::fabs(d) <= step) m.lat = want;
            else m.lat += std::copysign(step, d);
            if (m.flash > 0.f) m.flash = std::max(0.f, m.flash - dt);
            if (puffs_.size() < 18 && int((t_ + m.phase) * 9.f) % 6 == 0) {
                Puff p;
                p.lat = m.lat;
                p.z = m.z - 0.35f;
                p.life = 0.26f;
                puffs_.push_back(p);
            }
        }
    }

    for (Prop& pr : props_) {
        if (mode_ == Mode::Pause) continue;
        pr.z -= (mode_ == Mode::Play ? 2.4f : 0.7f) * dt;
        if (pr.z < 5.4f) pr.z += 12.5f;
    }

    for (Puff& p : puffs_) p.age += dt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }), puffs_.end());

    if (mode_ != Mode::Play) {
        pull_ = 0;
        serviceAudio();
        return;
    }

    watch_ += dt;

    float goal = 0.f;
    bool wantFire = false;
    pull_ = 0.f;
    if (bot_) {
        botPlan(goal, wantFire);
    } else {
        const gs::Pad& pad = sys_->pad;
        float digital = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        float axis = std::fabs(pad.axisX) > 0.18f ? pad.axisX : digital;
        goal = px_ + std::clamp(axis, -1.f, 1.f) * kSteer * 2.2f;
        wantFire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_TURBO) ||
                   pad.accel > 0.45f;
        if (pad.down(gs::BTN_UP) || pad.axisY > 0.45f) pull_ = 1.f;
    }
    {
        float limit = bot_ ? kBotLimit : 2.5f;
        float want = std::clamp(goal, -limit, limit);
        float d = want - px_;
        float step = kSteer * dt;
        if (std::fabs(d) <= step) px_ = want;
        else px_ += std::copysign(step, d);
    }

    if (wantFire) launchPlayer();

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
                if ((s.prev - 0.2f) <= m.z && (s.z + 0.35f) >= m.z) {
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
                sys_->apu.noiseBurst(0.16f, 860.f, 0.04f);
                if (m.hp <= 0) stopMachine(m);
            } else if (s.z < 19.f) {
                keep.push_back(s);
            }
        } else {
            s.z -= kBoltV * dt;
            bool crossed = s.prev > kHitZ && s.z <= kHitZ;
            if (crossed && std::fabs(s.lat - px_) < kBand) hurtPlayer(1);
            if (s.z > kHitZ - 0.15f) keep.push_back(s);
        }
    }
    shots_.swap(keep);

    if (arm_ <= 0.f && grace_ <= 0.f) {
        for (int i = 0; i < int(machines_.size()); i++) {
            Machine& m = machines_[size_t(i)];
            if (!m.running) continue;
            if (m.cool > 0.f) m.cool = std::max(0.f, m.cool - dt);
            bool aligned = std::fabs(px_ - m.lat) < kBand && m.cool <= 0.f;
            if (aligned) m.wind += dt;
            else m.wind = std::max(0.f, m.wind - dt * 0.4f);
            if (m.wind >= windOf(m.kind)) launchBolt(i);
            if (m.z <= kContact) {
                float side = px_ >= m.lat ? 1.f : -1.f;
                px_ = std::clamp(px_ + side * 0.32f, -2.5f, 2.5f);
                m.z += 2.1f;
                m.cool = std::max(m.cool, 0.8f);
                hurtPlayer(2);
                shake_ = std::max(shake_, 0.8f);
            }
        }
    }

    if (std::fabs(px_) > kLip) {
        float over = std::fabs(px_) - kLip;
        float rate = 1.15f + over * 2.6f;
        if (pull_ > 0.5f) rate *= 1.4f;
        slip_ += dt * rate;
    } else {
        slip_ = std::max(0.f, slip_ - dt * 1.75f);
    }

    if (hull_ <= 0) loseRidge("YOUR MACHINE STOPPED");
    else if (slip_ >= kSlipDie) loseRidge("OFF THE RIDGE");
    else if (rivalsRunning() == 0) {
        grace_ += dt;
        if (grace_ > 0.45f) winRidge();
    } else if (watch_ >= kWatch) {
        loseRidge("NOT THE LAST");
    }

    if (mode_ == Mode::Play) {
        if (hull_ <= 2 || slip_ > 0.35f) sys_->setLight(200, 40, 30);
        else if (laneHot(px_, 4.f) || slip_ > 0.08f) sys_->setLight(200, 130, 30);
        else sys_->setLight(60, 100, 160);
    }

    serviceAudio();
}

void Game::serviceAudio() {
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.14f) {
            static const float good[] = {349.f, 440.f, 523.f, 698.f};
            static const float bad[] = {196.f, 155.f, 123.f};
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
        int running = rivalsRunning() + (mode_ == Mode::Title ? 1 : (hull_ > 0 ? 1 : 0));
        float wob = std::sin(t_ * 34.f) * 3.f;
        float base = 46.f + float(running) * 4.f + pull_ * 10.f;
        sys_->apu.tone(1, base + wob, mode_ == Mode::Play ? 0.035f : 0.02f);
    } else {
        sys_->apu.tone(1, 0.f, 0.f);
    }
    if (mode_ == Mode::Play && (laneHot(px_, 3.2f) || slip_ > 0.2f) && blip_ <= 0.f) sys_->apu.tone(2, 140.f, 0.028f);
    else if (blip_ <= 0.f && mode_ != Mode::Play) sys_->apu.tone(2, 0.f, 0.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START)) begin();
        else update(kDt);
    } else if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else {
            sys.apu.tone(0, 0.f, 0.f);
            sys.apu.tone(1, 0.f, 0.f);
            sys.apu.tone(2, 0.f, 0.f);
        }
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
    p.x = 160.f + (bendAt(z) + lat) * p.ppm;
    p.ok = true;
    return p;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float f = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
    return int(f * 11.f);
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

void Game::layRoad(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t fog = gs::rgb4(6, 5, 5);
    if (mode_ == Mode::Fail) fog = gs::rgb4(8, 2, 2);
    else if (mode_ == Mode::Victory) fog = gs::rgb4(8, 7, 4);
    v.setFogColor(fog);
    int hor = int(kHorizon);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < hor) {
            v.road[y].on = false;
            float u = float(y) / float(hor);
            int r = std::clamp(int(2 + u * u * 11.f), 0, 15);
            int g = std::clamp(int(3 + u * 4.f), 0, 15);
            int b = std::clamp(int(7 - u * 4.f), 0, 15);
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
        rd.cx = 160.f + bendAt(wz) * ppm + shx;
        rd.hw = std::max(2.f, kRoadHalf * ppm);
        rd.v = wz * 7.f - scroll_;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(wz * 0.35f + travel_ * 0.08f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_DROP;
        rd.right = gs::GROUND_DROP;
        float fogT = std::clamp((0.18f - t) / 0.18f, 0.f, 1.f);
        int f = int(fogT * 10.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + 3);
        v.lineFog[y] = uint8_t(f);
        int abyss = std::clamp(2 - int((float(y) - kHorizon) / 40.f), 0, 2);
        v.lineBackdrop[y] = gs::rgb4(abyss, abyss, abyss + 1);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 90.f) * 5.f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) {
        text("RIDGE PURSUIT", 160.f + shx, 28.f, 0.92f, PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        text("LAST MACHINE", 160.f + shx, 26.f, 0.9f, PAL_GOOD);
        text("STILL RUNNING", 160.f + shx, 48.f, 0.9f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text("RIDGE LOST", 160.f + shx, 30.f, 1.0f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f + shx, 32.f, 1.05f, PAL_GOLD);
    }

    struct Item {
        float z;
        int ord;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(48);
    items.push_back({kPlayerZ, 0, 0, 0});
    for (int i = 0; i < int(machines_.size()); i++) items.push_back({machines_[size_t(i)].z, 3, 1, i});
    for (int i = 0; i < int(shots_.size()); i++) items.push_back({shots_[size_t(i)].z, 1, 2, i});
    for (int i = 0; i < int(puffs_.size()); i++) items.push_back({puffs_[size_t(i)].z, 2, 3, i});
    for (int i = 0; i < int(props_.size()); i++) items.push_back({props_[size_t(i)].z, 4, 4, i});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.ord < b.ord;
    });

    auto machineSprite = [&](int kind, bool you, float lat, float z, bool running, float flash, float phase, float wind) {
        Proj p = project(lat, z);
        if (!p.ok) return;
        int fog = you ? 0 : fogFor(z);
        const gs::Mipped* body = &art_.you;
        int pal = PAL_YOU;
        float wh = 1.82f;
        if (!you) {
            wh = bodyH(kind);
            pal = kindPal(kind);
            if (kind == 0) body = &art_.crawler;
            else if (kind == 1) body = &art_.dray;
            else body = &art_.hauler;
        }
        float h = std::clamp(wh * p.ppm, 8.f, you ? 86.f : 78.f);
        bool blink = you && hurtT_ > 0.f && (int(t_ * 24.f) & 1);
        if (!blink) {
            spr(art_.shadow, p.x + shx, p.y, h * 0.22f, 0, false, 0, false, true);
            spr(*body, p.x + shx, p.y, h, pal, false, fog, true, false);
        }
        if (running && !blink && (you || (int(t_ * 10.f + phase) & 1))) {
            float yf = you ? 0.62f : 0.58f;
            spr(art_.spark, p.x + shx - h * 0.22f, p.y - h * yf, h * 0.1f, PAL_BOLT, false, fog, false, false);
            spr(art_.spark, p.x + shx + h * 0.22f, p.y - h * yf, h * 0.1f, PAL_BOLT, false, fog, false, false);
        }
        if (!you && running && wind > windOf(kind) * 0.55f && (int(t_ * 16.f) & 1))
            spr(art_.flame[int(t_ * 12.f) & 1], p.x + shx, p.y - h * 0.7f, h * 0.22f, PAL_FX, false, fog, false, false);
        if (!running) {
            int flick = int(t_ * 12.f + phase) & 1;
            spr(art_.flame[flick], p.x + shx, p.y - h * 0.5f, h * 0.36f, PAL_FX, false, fog, false, false);
        }
        if (flash > 0.f) spr(art_.spark, p.x + shx, p.y - h * 0.45f, h * 0.28f, PAL_BOLT, false, 0, false, false);
    };

    for (const Item& it : items) {
        if (it.kind == 0) {
            machineSprite(0, true, px_, kPlayerZ, hull_ > 0 && mode_ != Mode::Fail, hurtT_, 0.f, 0.f);
            continue;
        }
        if (it.kind == 1) {
            const Machine& m = machines_[size_t(it.id)];
            machineSprite(m.kind, false, m.lat, m.z, m.running, m.flash, m.phase, m.wind);
            continue;
        }
        if (it.kind == 2) {
            const Shot& s = shots_[size_t(it.id)];
            Proj p = project(s.lat, std::max(s.z, kZNear + 0.05f));
            if (!p.ok) continue;
            float h = std::clamp(0.62f * p.ppm, 4.f, 18.f);
            if (s.from < 0) spr(art_.shot, p.x + shx, p.y - h, h, PAL_BOLT, false, fogFor(s.z), false, false);
            else spr(art_.bolt, p.x + shx, p.y - h * 0.4f, h, PAL_ALERT, false, fogFor(s.z), false, false);
            continue;
        }
        if (it.kind == 3) {
            const Puff& f = puffs_[size_t(it.id)];
            Proj p = project(f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(p.ppm * (0.5f + u * 0.7f), 3.f, 24.f);
            spr(f.kind == 1 ? art_.spark : art_.puff, p.x + shx, p.y - u * 8.f, h, f.kind == 1 ? PAL_BOLT : PAL_FX, false,
                fogFor(f.z), false, false);
            continue;
        }
        const Prop& pr = props_[size_t(it.id)];
        Proj p = project(pr.lat, pr.z);
        if (!p.ok) continue;
        int fog = fogFor(pr.z);
        if (pr.kind == 0) {
            spr(art_.cairn, p.x + shx, p.y, std::clamp(2.4f * p.ppm, 8.f, 70.f), PAL_STONE, pr.lat > 0, fog, true, false);
        } else {
            float h = std::clamp(2.6f * p.ppm, 8.f, 72.f);
            spr(art_.post, p.x + shx, p.y, h, PAL_STONE, pr.lat < 0, fog, true, false);
        }
    }

    float sunX = 168.f + std::sin(t_ * 0.08f) * 4.f + shx * 0.2f;
    spr(art_.sun, sunX, kHorizon - 10.f, 18.f, PAL_GOLD, false, 1, false, false);
    spr(art_.moon, 46.f + shx * 0.15f, 18.f, 14.f, PAL_PEAK, false, 2, false, false);
    float drift = std::fmod(t_ * 8.f, 360.f);
    spr(art_.cloud, drift - 20.f, 22.f, 16.f, PAL_INK, false, 3, false, false);
    spr(art_.cloud, std::fmod(drift + 170.f, 360.f) - 10.f, 36.f, 12.f, PAL_INK, true, 4, false, false);
    spr(art_.peakL, 46.f + shx * 0.25f, kHorizon + 8.f, 70.f, PAL_PEAK, false, 2, true, false);
    spr(art_.peakR, 274.f + shx * 0.25f, kHorizon + 12.f, 60.f, PAL_PEAK, false, 3, true, false);

    if (mode_ == Mode::Title) {
        hudC(20, "YOU HAVE THE RIDGE", PAL_INK);
        hudC(21, "BE THE LAST MACHINE STILL RUNNING", PAL_GOLD);
        hudC(22, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(24, "ARROWS STEER   UP PULLS   C FIRES", PAL_INK);
        if ((sys_->frame / 30) % 2 == 0) hudC(26, "ENTER STARTS", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(26, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Victory) {
        hudC(1, "LAST MACHINE STILL RUNNING", PAL_GOOD);
        hudC(2, "THE RIDGE IS YOURS", PAL_GOLD);
        if ((sys_->frame / 30) % 2 == 0) hudC(26, "ENTER", PAL_INK);
    } else if (mode_ == Mode::Fail) {
        hudC(1, "THE RIDGE IS LOST", PAL_ALERT);
        hudC(2, reason_, PAL_INK);
        hudC(26, "ENTER RETRIES", PAL_GOLD);
    } else {
        std::string pips = "HULL ";
        for (int i = 0; i < kHull; i++) pips += (i < hull_) ? '#' : '-';
        hud(1, 0, pips, hull_ <= 2 ? PAL_ALERT : PAL_INK);
        char buf[48];
        int running = rivalsRunning() + (hull_ > 0 ? 1 : 0);
        std::snprintf(buf, sizeof buf, "RUN %d", running);
        hud(16, 0, buf, running == 1 ? PAL_GOOD : PAL_GOLD);
        int left = std::max(0, int(std::ceil(kWatch - watch_)));
        std::snprintf(buf, sizeof buf, "RIDGE %d:%02d", left / 60, left % 60);
        hud(28, 0, buf, left <= 12 ? PAL_ALERT : PAL_INK);
        std::string row;
        for (const Machine& m : machines_) {
            if (!row.empty()) row += "  ";
            row += kindName(m.kind);
            row += " ";
            row += m.running ? std::to_string(m.hp) : "--";
        }
        hudC(1, row, PAL_INK);
        if (bannerT_ > 0.f) hudC(2, std::string(banner_) + " STOPPED", PAL_GOOD);
        const char* hint = "HOLD THE CREST";
        int hpal = PAL_INK;
        if (slip_ > 0.05f) {
            hint = "BACK FROM THE LIP";
            hpal = PAL_ALERT;
        } else if (laneHot(px_, 4.f)) {
            hint = "OFF THEIR LINE";
            hpal = PAL_ALERT;
        } else if (int qi = quarry(); qi >= 0 && std::fabs(px_ - machines_[size_t(qi)].lat) < kAim) {
            hint = "FIRE";
            hpal = PAL_GOLD;
        }
        hudC(26, hint, hpal);
        hudC(27, "ARROWS STEER   UP PULLS   C FIRES", PAL_INK);
    }
}

}  // namespace ridgepurs
