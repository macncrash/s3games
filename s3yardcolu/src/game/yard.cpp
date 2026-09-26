#include "game/yard.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace ycol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 48.f;
constexpr float kSpan = 168.f;
constexpr float kZNear = 6.8f;
constexpr float kPpm = 36.f;
constexpr float kRoadHalf = 5.15f;
constexpr float kLine = 10.4f;
constexpr float kGantryZ = 20.f;
constexpr float kYardZ = 16.6f;
constexpr float kPanic = 13.f;
constexpr float kFoul = 14.f;
constexpr float kGap = 2.45f;
constexpr float kPitch = 6.6f;
constexpr float kBrake = 13.f;
constexpr float kHookRate = 4.8f;
constexpr float kDropRate = 1.6f;
constexpr float kSet = 0.78f;
constexpr float kOnRoad = 1.6f;
constexpr float kStow = -7.2f;
constexpr float kHookMin = -8.6f;
constexpr float kHookMax = 6.4f;
constexpr float kWatch = 34.f;
constexpr float kMuleZ0 = 40.f;
constexpr float kMuleV = 7.0f;
constexpr float kLeadZ = 82.f;
constexpr float kTruckV = 5.2f;
constexpr float kSpawn = 8.2f;
constexpr float kTruckH = 3.4f;
constexpr float kMuleH = 2.55f;
constexpr float kHulkH = 1.7f;
constexpr float kTurnLat = 4.4f;
constexpr float kTurnAdvance = 1.25f;
constexpr float kBerthZ = 14.7f;
constexpr float kBerthLat = -6.9f;

float bend(float z) {
    float u = std::max(0.f, z - 12.f);
    return std::sin(u * 0.02f) * 1.35f;
}

float parkLat(int slot) {
    static const float kPark[] = {0.1f, -0.7f, 0.55f, -0.25f};
    return kPark[slot & 3];
}

float haltOf(int slot) { return kGantryZ + kGap + float(slot) * kPitch; }

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

Game::Spot Game::project(float wx, float wz) const {
    Spot s;
    if (!(wz > kZNear + 0.05f)) return s;
    float t = kZNear / wz;
    s.ppm = kPpm * t;
    s.y = kHorizon + t * kSpan;
    s.x = 160.f + wx * s.ppm;
    s.ok = true;
    return s;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float fade = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
    return int(fade * 11.f);
}

bool Game::blockOn() const { return drop_ >= kSet && std::fabs(hook_) <= kOnRoad; }

bool Game::muleClear() const {
    for (const Rig& r : rigs_)
        if (!r.column) return r.passed || r.turning || r.berthed;
    return false;
}

bool Game::muleIn() const {
    for (const Rig& r : rigs_)
        if (!r.column) return r.berthed;
    return false;
}

float Game::leadZ() const {
    for (const Rig& r : rigs_)
        if (r.column && r.slot == 0) return r.z;
    return 999.f;
}

void Game::buildProps() {
    props_.clear();
    auto add = [&](float z, float lat, float h, int kind) {
        Prop p;
        p.z = z;
        p.lat = lat;
        p.h = h;
        p.kind = kind;
        props_.push_back(p);
    };
    add(12.4f, -6.8f, 3.1f, 3);
    add(14.2f, -6.3f, 2.5f, 4);
    add(18.4f, -7.2f, 2.1f, 0);
    add(17.2f, -6.15f, 1.35f, 6);
    add(24.5f, -7.6f, 2.8f, 2);
    add(23.0f, -6.2f, 1.3f, 6);
    add(31.f, -7.1f, 1.7f, 1);
    add(38.f, -7.8f, 2.9f, 7);
    add(48.f, -7.4f, 2.2f, 0);
    add(60.f, -8.f, 2.6f, 2);
    add(15.5f, 6.6f, 3.3f, 5);
    add(27.f, 6.9f, 3.2f, 5);
    add(21.f, 6.5f, 1.25f, 6);
    add(36.f, 7.2f, 1.8f, 0);
    add(52.f, 7.6f, 2.4f, 1);
}

void Game::lay(bool scenic) {
    rigs_.clear();
    puffs_.clear();
    auto add = [&](bool column, int slot, float z, float lat, float cruise) {
        Rig r;
        r.column = column;
        r.slot = slot;
        r.z = z;
        r.lat = lat;
        r.cruise = cruise;
        r.speed = scenic ? 0.f : cruise;
        r.haltZ = column ? haltOf(slot) : kGantryZ + 2.4f;
        r.side = (slot & 1) ? 1.f : -1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(false, 0, kBerthZ, kBerthLat, kMuleV);
        rigs_.back().berthed = true;
        rigs_.back().passed = true;
        for (int i = 0; i < kColumn; ++i) add(true, i, 52.f + float(i) * 9.f, parkLat(i), kTruckV);
        return;
    }
    add(false, 0, kMuleZ0, 0.f, kMuleV);
    for (int i = 0; i < kColumn; ++i) add(true, i, kLeadZ + float(i) * kSpawn, 0.f, kTruckV);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    hook_ = -2.f;
    drop_ = 0.35f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    hookVel_ = 0;
    dropVel_ = 0;
    fanStep_ = -1;
    wasBlock_ = false;
    reason_ = "ANYTHING ELSE";
    lay(true);
}

void Game::begin() {
    lay(false);
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    hook_ = kStow;
    drop_ = 0.12f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    hookVel_ = 0;
    dropVel_ = 0;
    fanStep_ = -1;
    wasBlock_ = false;
    reason_ = "ANYTHING ELSE";
    blip(620.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.18f, 0.08f);
    if (bot_) begin();
    else bootTitle();
}

void Game::blip(float freq) {
    if (fanStep_ >= 0) return;
    sys_->apu.tone(0, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::fanfare(bool good) {
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = good;
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.7f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.2f, 240.f, 0.22f);
    sys_->rumble(0.5f, 0.16f, 180);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.45f;
    fanfare(true);
    sys_->rumble(0.22f, 0.48f, 160);
}

const char* Game::hint() const {
    if (!muleClear()) {
        if (blockOn()) return "HULK IS EARLY";
        if (drop_ >= 0.45f && std::fabs(hook_) <= kOnRoad + 0.5f) return "HOLD IT UP";
        return "LET THE MULE IN";
    }
    if (std::fabs(hook_) > kOnRoad) return drop_ >= kSet ? "OFF THE MARK" : "SWING TO THE MARK";
    if (drop_ < kSet) return leadZ() < kGantryZ + kPanic + 10.f ? "TOO CLOSE" : "SET THE HULK";
    return "HOLD THE HULK";
}

void Game::stepMule(Rig& r, bool block) {
    if (r.berthed) {
        r.z = kBerthZ;
        r.lat = kBerthLat;
        r.speed = 0.f;
        r.turning = false;
        return;
    }
    if (r.turning) {
        r.lat -= kTurnLat * kDt;
        r.z -= kTurnAdvance * kDt;
        r.speed = kTurnAdvance;
        if (r.lat <= kBerthLat) {
            r.berthed = true;
            r.z = kBerthZ;
            r.lat = kBerthLat;
            r.speed = 0.f;
            blip(480.f);
        } else if (r.z < kLine) {
            lose("MISSED THE YARD");
        }
        return;
    }
    if (!r.passed) {
        if (block) {
            float gap = r.z - (kGantryZ + 2.4f);
            if (gap < 7.5f && r.speed > 4.f) {
                lose("NOT THE COLUMN");
                return;
            }
            r.speed = std::max(0.f, r.speed - 18.f * kDt);
            if (r.speed > 0.04f) r.z -= r.speed * kDt;
            r.lat += (0.f - r.lat) * std::min(1.f, 3.f * kDt);
            if (r.speed < 0.3f) {
                r.speed = 0.f;
                lose("NOT THE COLUMN");
            }
            return;
        }
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (0.f - r.lat) * std::min(1.f, 3.f * kDt);
        if (r.z < kGantryZ) r.passed = true;
        return;
    }
    r.speed = r.cruise * 0.92f;
    r.z -= r.speed * kDt;
    if (r.z <= kYardZ) r.turning = true;
    else if (r.z < kLine) lose("MISSED THE YARD");
}

void Game::stepTruck(Rig& r, bool block) {
    if (r.passed) {
        r.z -= std::max(r.cruise, 2.f) * kDt;
        return;
    }
    if (r.spooked) {
        r.lat += r.side * 5.f * kDt;
        r.speed = std::max(r.speed, 2.5f);
        r.z -= r.speed * kDt;
        if (r.lat < -kRoadHalf - 0.05f) lose("INTO THE YARD");
        else if (r.lat > kRoadHalf - 0.05f) lose("OFF THE ROAD");
        else if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (r.stopped && block) {
        r.z = r.haltZ;
        r.lat = parkLat(r.slot);
        r.speed = 0.f;
        return;
    }
    if (r.stopped && !block) {
        r.stopped = false;
        r.orderly = false;
        r.speed = std::max(r.speed, 0.8f);
    }
    bool foul = drop_ >= kSet && std::fabs(hook_) > kOnRoad;
    if (!block && foul && r.z > kGantryZ && r.z <= kGantryZ + kFoul && r.speed > 2.2f) {
        r.spooked = true;
        r.side = hook_ >= 0.f ? -1.f : 1.f;
        blip(120.f);
        return;
    }
    if (!block) {
        r.orderly = false;
        if (r.z < kGantryZ) {
            r.lat -= 5.f * kDt;
            r.speed = std::max(r.speed, 2.8f);
            r.z -= r.speed * kDt;
            if (r.lat < -kRoadHalf - 0.05f) lose("INTO THE YARD");
            else if (r.z < kLine) {
                ++through_;
                lose("THE COLUMN PASSED");
            }
            return;
        }
        r.speed = std::min(r.cruise, r.speed + 3.4f * kDt);
        r.z -= r.speed * kDt;
        r.lat += (0.f - r.lat) * std::min(1.f, 2.6f * kDt);
        if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kGantryZ + kPanic && r.speed > 2.1f) {
            r.spooked = true;
            r.side = (r.slot & 1) ? 1.f : -1.f;
            blip(110.f);
            return;
        }
        r.orderly = true;
    }
    float park = parkLat(r.slot);
    float dist = r.z - r.haltZ;
    if (dist <= 0.2f || (dist < 1.15f && r.speed < 0.32f)) {
        if (r.haltZ < kGantryZ - 0.2f) {
            ++through_;
            lose("THE COLUMN PASSED");
            return;
        }
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(88.f + float(r.slot) * 26.f);
        r.stopped = true;
        return;
    }
    if (dist > kBrake) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 2.4f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.25f));
        a = std::min(a, 24.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 3.8f * kDt);
        if (r.speed > 0.45f && r.puff <= 0.f && puffs_.size() < 18) {
            r.puff = 0.16f;
            Puff puff;
            puff.z = r.z + 0.55f;
            puff.lat = r.lat;
            puff.age = 0.f;
            puff.life = 0.48f;
            puffs_.push_back(puff);
        }
    }
    if (r.z <= r.haltZ || (r.speed < 0.22f && dist < 1.7f)) {
        r.z = std::max(r.z, r.haltZ);
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(88.f + float(r.slot) * 26.f);
        r.stopped = true;
    }
}

void Game::update() {
    t_ += kDt;
    float hookDir = 0.f;
    float dropDir = -1.f;
    if (bot_) {
        float hookTarget = muleClear() ? 0.f : kStow;
        float dropTarget = muleClear() ? 1.f : 0.f;
        if (std::fabs(hook_ - hookTarget) <= kHookRate * kDt) hook_ = hookTarget;
        else hookDir = hook_ < hookTarget ? 1.f : -1.f;
        if (std::fabs(drop_ - dropTarget) <= kDropRate * kDt) drop_ = dropTarget;
        else dropDir = drop_ < dropTarget ? 1.f : -1.f;
    } else {
        const gs::Pad& pad = sys_->pad;
        bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
        bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
        if (left && !right) hookDir = -1.f;
        else if (right && !left) hookDir = 1.f;
        bool down = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.axisY < -0.35f;
        bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) || pad.axisY > 0.35f;
        if (down && !up) dropDir = 1.f;
    }
    float prevHook = hook_;
    float prevDrop = drop_;
    hook_ = std::clamp(hook_ + hookDir * kHookRate * kDt, kHookMin, kHookMax);
    drop_ = std::clamp(drop_ + dropDir * kDropRate * kDt, 0.f, 1.f);
    hookVel_ = (hook_ - prevHook) / kDt;
    dropVel_ = (drop_ - prevDrop) / kDt;
    bool block = blockOn();
    if (block != wasBlock_) {
        blip(block ? 160.f : 380.f);
        sys_->rumble(block ? 0.3f : 0.08f, block ? 0.45f : 0.12f, block ? 90 : 40);
        wasBlock_ = block;
    }

    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        if (r.column) stepTruck(r, block);
        else stepMule(r, block);
    }
    if (mode_ != Mode::Play) return;

    int held = 0;
    for (const Rig& r : rigs_) {
        if (!r.column || !r.stopped || r.spooked) continue;
        if (r.z >= kGantryZ - 0.05f && std::fabs(r.lat) <= kRoadHalf * 0.75f) ++held;
    }
    stopped_ = held;
    bool file = block && muleIn() && through_ == 0 && held == kColumn;
    if (file) settle_ += kDt;
    else settle_ = 0.f;
    if (settle_ >= 0.42f) win();
    else if (t_ >= kWatch) lose("THE WATCH IS OVER");
}

void Game::tickPuffs() {
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                 puffs_.end());
    for (Rig& r : rigs_)
        if (r.puff > 0.f) r.puff -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f && fanStep_ < 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.13f) {
            static const float good[] = {330.f, 415.f, 494.f, 660.f};
            static const float bad[] = {196.f, 155.f, 123.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Title && beep_ <= 0.f) {
        sys_->apu.tone(0, 98.f, 0.016f);
    }
    bool swing = std::fabs(hookVel_) > 0.2f && (mode_ == Mode::Play || mode_ == Mode::Title);
    if (swing) sys_->apu.tone(1, 52.f + std::fabs(hook_) * 4.f, 0.028f);
    else sys_->apu.tone(1, 0.f, 0.f);
    bool rolling = false;
    if (mode_ == Mode::Play) {
        for (const Rig& r : rigs_)
            if (!r.berthed && !r.stopped && r.speed > 0.45f) rolling = true;
    }
    if (rolling) sys_->apu.tone(2, 42.f, 0.024f);
    else sys_->apu.tone(2, 0.f, 0.f);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
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
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
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

void Game::layRoad(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t skyHor = mode_ == Mode::Fail ? gs::rgb4(10, 4, 2) : gs::rgb4(11, 7, 4);
    v.setFogColor(skyHor);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            int r = int(2.f + u * 9.f);
            int g = int(3.f + u * 4.f);
            int b = int(6.f - u * 2.f);
            if (mode_ == Mode::Fail) r = std::min(15, r + 3);
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
            v.lineFog[y] = 0;
            rd.on = false;
            continue;
        }
        float row = float(y) - kHorizon;
        float t = std::max(row / kSpan, 0.004f);
        float wz = kZNear / t;
        rd.on = true;
        rd.cx = 160.f + bend(wz) * (kPpm * t) + shx;
        rd.hw = std::max(2.f, kRoadHalf * kPpm * t);
        rd.v = wz * 30.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.16f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
        int f = int(fogT * 10.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 5.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(4, 4, 2);
    }
}

void Game::drawGantry(float shx) {
    float z = kGantryZ;
    float left = bend(z) - kRoadHalf - 1.15f;
    float right = bend(z) + kRoadHalf + 1.15f;
    Spot L = project(left, z);
    Spot R = project(right, z);
    if (!L.ok || !R.ok) return;
    int fog = fogFor(z);
    float postH = std::clamp(5.6f * L.ppm, 28.f, 150.f);
    float groundY = L.y;
    float beamY = groundY - postH * 0.92f;
    float span = std::fabs(R.x - L.x);
    float midX = (L.x + R.x) * 0.5f + shx;

    Spot mark = project(bend(z) + hook_, z);
    float hulkH = std::clamp(kHulkH * L.ppm, 8.f, 90.f);
    float feet = beamY + drop_ * (groundY - beamY);
    if (mark.ok) {
        float hx = mark.x + shx;
        spr(art_.hulk, hx, feet, hulkH, PAL_HULK, false, fog, true);
        float magH = hulkH * 0.55f;
        spr(art_.magnet, hx, feet - hulkH - magH * 0.15f, magH, PAL_MAGNET, false, fog, true);
        float cableTop = beamY;
        float cableBot = feet - hulkH - magH * 0.35f;
        int n = 5;
        for (int i = 0; i < n; ++i) {
            float u = (float(i) + 0.5f) / float(n);
            float y = cableTop + (cableBot - cableTop) * u;
            sprBox(art_.cable, hx, y, std::max(2.f, L.ppm * 0.12f), std::max(3.f, (cableBot - cableTop) / float(n) + 1.f),
                   PAL_GANTRY, fog);
        }
    }

    spr(art_.post, L.x + shx, groundY, postH, PAL_GANTRY, false, fog, true);
    spr(art_.post, R.x + shx, groundY, postH, PAL_GANTRY, true, fog, true);
    sprBox(art_.beam, midX, beamY, span, std::max(5.f, L.ppm * 0.28f), PAL_GANTRY, fog);
    if (mark.ok) {
        float trolley = std::clamp(mark.x, std::min(L.x, R.x), std::max(L.x, R.x));
        sprBox(art_.beam, trolley + shx, beamY - L.ppm * 0.18f, std::max(8.f, L.ppm * 0.7f), std::max(4.f, L.ppm * 0.22f),
               PAL_HAZARD, fog);
    }

    float markW = std::max(10.f, kOnRoad * 2.f * L.ppm);
    Spot roadMark = project(bend(z), z);
    if (roadMark.ok)
        sprBox(art_.mark, roadMark.x + shx, roadMark.y - 1.f, markW, std::max(3.f, L.ppm * 0.16f), PAL_AMBER, fog);

    int lampPal = blockOn() ? PAL_GOOD : (drop_ >= kSet ? PAL_ALERT : PAL_AMBER);
    float lampH = std::max(6.f, L.ppm * 0.42f);
    spr(art_.lamp, L.x + shx, beamY - 2.f, lampH, lampPal, false, 0, false);
    spr(art_.lamp, R.x + shx, beamY - 2.f, lampH * 0.9f, lampPal, false, 0, false);

    if (drop_ > 0.35f && mark.ok)
        spr(art_.shadow, mark.x + shx, groundY, hulkH * (0.35f + 0.45f * drop_), PAL_FX, false, 0, false, true);
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot s = project(bend(pr.z) + pr.lat, pr.z);
    if (!s.ok) return;
    int fog = fogFor(pr.z);
    float h = std::clamp(pr.h * s.ppm, 6.f, 150.f);
    float x = s.x + shx;
    if (pr.kind == 5) {
        spr(art_.post, x, s.y, h, PAL_YARD, false, fog, true);
        spr(art_.lamp, x, s.y - h * 0.92f, h * 0.28f, PAL_AMBER, false, fog, false);
        return;
    }
    const gs::Mipped* m = &art_.scrap;
    int pal = PAL_YARD;
    if (pr.kind == 1) m = &art_.drums;
    else if (pr.kind == 2) m = &art_.stack;
    else if (pr.kind == 3) m = &art_.shack;
    else if (pr.kind == 4) {
        m = &art_.sign;
        pal = PAL_SIGN;
    } else if (pr.kind == 6) m = &art_.fence;
    else if (pr.kind == 7) m = &art_.baler;
    spr(art_.shadow, x, s.y, h * 0.22f, PAL_FX, false, 0, false, true);
    spr(*m, x, s.y, h, pal, pr.lat > 0.f, fog, true);
}

void Game::drawRig(const Rig& r, float shx) {
    if (r.z < kZNear + 0.25f) return;
    Spot s = project(bend(r.z) + r.lat, r.z);
    if (!s.ok) return;
    int fog = fogFor(r.z);
    float worldH = r.column ? kTruckH : kMuleH;
    float h = std::clamp(worldH * s.ppm, 4.f, 130.f);
    float x = s.x + shx;
    if (r.column && r.slot == 0) {
        float bob = std::sin(t_ * 4.5f + r.z) * 1.4f;
        spr(art_.pennant, x + h * 0.08f, s.y - h + bob, h * 0.32f, PAL_ALERT, false, fog, true);
    }
    const gs::Mipped& body = r.column ? art_.truck : art_.mule;
    int pal = r.column ? PAL_TRUCK : PAL_MULE;
    bool flip = !r.column && (r.turning || r.berthed);
    spr(body, x, s.y, h, pal, flip, fog, true);
    spr(art_.shadow, x, s.y, h * 0.28f, PAL_FX, false, 0, false, true);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 68.f) * 4.f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) text("YARD COLUMN", 160.f + shx, 26.f, 1.1f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 28.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text(reason_, 160.f + shx, 28.f, 0.85f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 28.f, 1.05f, PAL_AMBER);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(48);
    for (int i = 0; i < int(rigs_.size()); ++i) items.push_back({rigs_[size_t(i)].z, 0, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 2, i});
    items.push_back({kGantryZ, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) {
            drawGantry(shx);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot s = project(bend(f.z) + f.lat, f.z);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(s.ppm * (1.1f + u * 1.4f), 4.f, 28.f);
            spr(art_.dust, s.x + shx, s.y - u * 7.f, h, PAL_FX, false, fogFor(f.z), false);
            continue;
        }
        if (it.kind == 2) {
            drawProp(props_[size_t(it.id)], shx);
            continue;
        }
        drawRig(rigs_[size_t(it.id)], shx);
    }

    float drift = std::fmod(t_ * 6.f, 420.f);
    spr(art_.cloud, drift - 60.f, 16.f, 16.f, PAL_DUSK, false, 2, false);
    spr(art_.cloud, std::fmod(drift + 220.f, 420.f) - 40.f, 28.f, 12.f, PAL_DUSK, true, 3, false);
    spr(art_.sun, 278.f + shx * 0.15f, 18.f, 18.f, PAL_DUSK, false, 0, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(21, "STOP THE COLUMN ON THE ROAD", PAL_AMBER);
        hudC(22, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(23, "LEFT RIGHT SWINGS THE HULK", PAL_TEXT);
        hudC(24, "DOWN HOLDS IT ON THE MARK", PAL_GOOD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_TEXT);
        hudC(26, "ESC TITLE", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(24, "THE YARD HOLDS", PAL_AMBER);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "ANYTHING ELSE IS A LOSS", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else {
        bool down = drop_ >= kSet;
        const char* hulk = blockOn() ? "ON THE MARK" : (down ? "OFF THE MARK" : "HULK UP");
        int hulkPal = blockOn() ? PAL_GOOD : (down ? PAL_ALERT : PAL_AMBER);
        hud(1, 0, hulk, hulkPal);
        hud(31, 0, muleIn() ? "MULE IN" : "MULE OUT", muleIn() ? PAL_GOOD : PAL_AMBER);
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(1, 1, buf, PAL_TEXT);
        int left = std::max(0, int(std::ceil(kWatch - t_)));
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(30, 1, buf, left <= 8 ? PAL_ALERT : PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "HOLD THE HULK") || !std::strcmp(h, "LET THE MULE IN")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "SET THE HULK") || !std::strcmp(h, "SWING TO THE MARK")) pal = PAL_AMBER;
        else pal = PAL_ALERT;
        hudC(2, h, pal);
        hudC(27, "LEFT RIGHT SWING    DOWN SET", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        float prevHook = hook_;
        float prevDrop = drop_;
        hook_ = -1.4f + std::sin(t_ * 0.65f) * 4.4f;
        drop_ = 0.28f + 0.18f * std::sin(t_ * 0.95f + 0.4f);
        hookVel_ = (hook_ - prevHook) / kDt;
        dropVel_ = (drop_ - prevDrop) / kDt;
        for (Rig& r : rigs_) {
            if (!r.column) continue;
            r.z -= 1.6f * kDt;
            if (r.z < 48.f) r.z += 36.f;
        }
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            hookVel_ = dropVel_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        hookVel_ = dropVel_ = 0.f;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        hookVel_ = dropVel_ = 0.f;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Fail) sys.setLight(190, 40, 24);
    else if (mode_ == Mode::Play && blockOn()) sys.setLight(40, 150, 60);
    else if (mode_ == Mode::Play && drop_ >= kSet) sys.setLight(180, 40, 28);
    else sys.setLight(170, 110, 36);
}

}  // namespace ycol
