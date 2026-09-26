#include "game/gate.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace colu {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 5;
constexpr float kHorizon = 66.f;
constexpr float kSpan = 148.f;
constexpr float kZNear = 8.f;
constexpr float kPpm = 21.f;
constexpr float kRoadHalf = 5.5f;
constexpr float kLine = 16.f;
constexpr float kPanic = 26.5f;
constexpr float kGap = 4.6f;
constexpr float kSpace = 9.2f;
constexpr float kBrakeDist = 13.5f;
constexpr float kArmRate = 1.2f;
constexpr float kShut = 0.18f;
constexpr float kLeadZ = 108.f;
constexpr float kTruckV = 6.25f;

float bend(float z) {
    float u = std::max(0.f, z - 30.f);
    return std::sin(u * 0.031f) * u * 0.055f;
}

float tallOf(int kind) {
    if (kind == 0) return 2.35f;
    if (kind == 1) return 3.05f;
    return 3.05f;
}

// Parked file: the lead sits in the lane, the rest step aside just enough to read.
float parkLat(int slot) {
    if (slot <= 0) return 0.f;
    return (slot & 1) ? -1.25f : 1.25f;
}

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

Game::Proj Game::project(float wx, float wz) const {
    Proj p;
    if (!(wz > kZNear + 0.05f)) return p;
    float t = kZNear / wz;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * kSpan;
    p.x = 160.f + wx * p.ppm;
    p.ok = true;
    return p;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float f = std::clamp((0.20f - t) / 0.20f, 0.f, 1.f);
    return int(f * 12.f);
}

bool Game::decoysClear() const {
    for (const Rig& r : rigs_)
        if (!r.column && !r.passed) return false;
    return true;
}

float Game::leadZ() const {
    for (const Rig& r : rigs_)
        if (r.column && r.slot == 0) return r.z;
    return 999.f;
}

void Game::buildProps() {
    props_.clear();
    for (int i = 0; i < 8; ++i) {
        float z = 24.f + float(i) * 13.5f;
        float side = (i & 1) ? 1.f : -1.f;
        float lat = side * (kRoadHalf + 2.35f + float(i % 3) * 0.4f);
        Prop p;
        p.z = z;
        p.lat = lat;
        p.kind = (i % 3 == 1) ? 1 : 0;
        p.h = (p.kind == 1) ? 7.1f : 5.5f;
        props_.push_back(p);
    }
    Prop sign;
    sign.z = kLine + 7.f;
    sign.lat = kRoadHalf + 1.55f;
    sign.kind = 2;
    sign.h = 2.5f;
    props_.push_back(sign);
    Prop booth;
    booth.z = kLine - 2.2f;
    booth.lat = -kRoadHalf - 3.55f;
    booth.kind = 3;
    booth.h = 4.7f;
    props_.push_back(booth);
}

void Game::lay(bool scenic) {
    rigs_.clear();
    puffs_.clear();
    auto add = [&](int kind, bool column, int slot, float z, float cruise) {
        Rig r;
        r.kind = kind;
        r.column = column;
        r.slot = slot;
        r.z = z;
        r.cruise = cruise;
        r.speed = cruise;
        r.haltZ = kLine + kGap + (column ? float(slot) * kSpace : 0.f);
        r.side = (slot & 1) ? 1.f : -1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(0, false, 0, 30.f, 13.4f);
        add(1, false, 1, 46.f, 11.6f);
        for (int i = 0; i < kColumn; ++i) add(2, true, i, 64.f + float(i) * kSpace, kTruckV);
        return;
    }
    add(0, false, 0, 44.f, 13.4f);
    add(1, false, 1, 70.f, 11.6f);
    for (int i = 0; i < kColumn; ++i) add(2, true, i, kLeadZ + float(i) * kSpace, kTruckV);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    arm_ = 0.35f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE GATE IS OPEN";
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
    arm_ = 1.f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE GATE IS OPEN";
    blip(660.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    buildProps();
    bootTitle();
    if (bot_) begin();
}

void Game::blip(float freq) {
    sys_->apu.tone(2, freq, 0.055f);
    blip_ = 0.07f;
}

void Game::latchSound(bool shut) {
    blip(shut ? 150.f : 420.f);
    sys_->rumble(shut ? 0.25f : 0.08f, shut ? 0.45f : 0.12f, shut ? 90 : 40);
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
    hold_ = 2.1f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.18f, 900.f, 0.18f);
    sys_->rumble(0.55f, 0.2f, 200);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.8f;
    fanfare(true);
    sys_->rumble(0.3f, 0.55f, 160);
}

float Game::botDir() const {
    if (!decoysClear()) return 1.f;
    return -1.f;
}

const char* Game::hint() const {
    if (!decoysClear()) return "LET THEM PASS";
    bool shut = arm_ < kShut;
    if (stopped_ >= kColumn && shut) return "HOLD THE GATE";
    if (!shut && leadZ() < kPanic + 9.f) return "TOO CLOSE";
    if (!shut) return "SHUT THE GATE";
    return "HOLD THE GATE";
}

void Game::stepRig(Rig& r, bool shut) {
    if (r.passed) {
        r.z -= r.cruise * kDt;
        return;
    }
    if (r.spooked) {
        r.lat += r.side * 6.5f * kDt;
        r.z -= std::max(r.speed, 2.2f) * kDt;
        if (std::fabs(r.lat) > kRoadHalf - 0.05f) lose("OFF THE ROAD");
        else if (r.z < kLine) {
            if (r.column) {
                ++through_;
                lose("THE COLUMN PASSED");
            } else {
                r.passed = true;
            }
        }
        return;
    }
    if (r.stopped && shut) {
        r.z = r.haltZ;
        r.lat = r.column ? parkLat(r.slot) : 0.f;
        r.speed = 0.f;
        return;
    }
    if (r.stopped && !shut) {
        r.stopped = false;
        r.orderly = false;
        r.speed = std::max(r.speed, 0.55f);
    }
    if (!shut) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 4.4f * kDt);
        r.z -= r.speed * kDt;
        r.lat = 0.22f * std::sin(t_ * 0.85f + float(r.slot) * 1.4f);
        if (r.z < kLine) {
            r.passed = true;
            if (r.column) {
                ++through_;
                lose("THE COLUMN PASSED");
            }
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.1f) {
            r.spooked = true;
            r.side = (r.slot & 1) ? 1.f : -1.f;
            return;
        }
        r.orderly = true;
    }
    float dist = r.z - r.haltZ;
    float park = r.column ? parkLat(r.slot) : 0.f;
    if (dist <= 0.15f || (dist < 1.05f && r.speed < 0.28f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
        if (!r.column) lose("NOT THE COLUMN");
        else if (std::fabs(r.lat) > kRoadHalf * 0.7f) lose("OFF THE ROAD");
        return;
    }
    if (dist > kBrakeDist) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 2.5f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.2f));
        a = std::min(a, 30.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 4.f * kDt);
        if (r.speed > 0.45f && r.puff <= 0.f && puffs_.size() < 20) {
            r.puff = 0.14f;
            puffs_.push_back({r.z, r.lat, 0.f, 0.55f});
        }
    }
    if (r.z <= r.haltZ || (r.speed < 0.2f && dist < 1.6f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
        if (!r.column) lose("NOT THE COLUMN");
    } else if (r.z < kLine) {
        if (r.column) {
            ++through_;
            lose("THE COLUMN PASSED");
        } else {
            r.passed = true;
        }
    }
}

void Game::update() {
    t_ += kDt;
    float dir = 0.f;
    if (bot_) {
        dir = botDir();
    } else {
        const gs::Pad& pad = sys_->pad;
        bool close = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.axisY < -0.35f;
        bool open = pad.down(gs::BTN_UP) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.axisY > 0.35f;
        if (close && !open) dir = -1.f;
        else if (open && !close) dir = 1.f;
    }
    float before = arm_;
    arm_ = std::clamp(arm_ + dir * kArmRate * kDt, 0.f, 1.f);
    motor_ = std::fabs(arm_ - before) > 0.0001f ? 1.f : 0.f;
    bool wasShut = before < kShut;
    bool shut = arm_ < kShut;
    if (wasShut != shut) latchSound(shut);

    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        stepRig(r, shut);
    }
    if (mode_ != Mode::Play) return;

    bool clear = true;
    int held = 0;
    for (const Rig& r : rigs_) {
        if (!r.column) {
            if (!r.passed) clear = false;
            continue;
        }
        if (r.stopped && !r.spooked && r.z >= kLine - 0.02f && std::fabs(r.lat) <= kRoadHalf * 0.7f) ++held;
    }
    stopped_ = held;
    if (clear && shut && held == kColumn && through_ == 0) {
        settle_ += kDt;
        if (settle_ >= 0.40f) win();
    } else {
        settle_ = 0.f;
    }
    if (mode_ == Mode::Play && t_ > 25.f) lose("THE COLUMN DID NOT STOP");
}

void Game::tickPuffs() {
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                 puffs_.end());
    for (Rig& r : rigs_)
        if (r.puff > 0.f) r.puff -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.8f);
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (motor_ > 0.f && (mode_ == Mode::Play || mode_ == Mode::Title))
        sys_->apu.tone(1, 46.f + (1.f - arm_) * 28.f, 0.03f);
    else
        sys_->apu.tone(1, 0.f, 0.f);
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.13f) {
            static const float good[] = {392.f, 494.f, 587.f, 784.f};
            static const float bad[] = {220.f, 174.f, 130.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(0, 110.f, 0.02f);
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t fog = mode_ == Mode::Fail ? gs::rgb4(10, 4, 3) : gs::rgb4(12, 11, 8);
    v.setFogColor(fog);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int r = std::clamp(int(2.f + u * 10.f), 0, 15);
            int g = std::clamp(int(4.f + u * 7.f), 0, 15);
            int b = std::clamp(int(11.f - u * 3.f), 0, 15);
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
        rd.v = wz * 32.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.22f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.18f - t) / 0.18f, 0.f, 1.f);
        int f = int(fogT * 11.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 6.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(3, 4, 2);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) {
            width += 10.f * scale;
            continue;
        }
        width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
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

void Game::drawGate(float shx) {
    float left = bend(kLine) - kRoadHalf + 0.2f;
    float right = bend(kLine) + kRoadHalf - 0.2f;
    Proj L = project(left, kLine);
    Proj R = project(right, kLine);
    if (!L.ok || !R.ok) return;
    int fog = fogFor(kLine);
    float postH = std::clamp(1.05f * L.ppm, 10.f, 36.f);
    float pivotX = L.x;
    float pivotY = L.y - postH;
    float tipShutX = R.x;
    float armLen = std::fabs(tipShutX - pivotX);
    float tipX = tipShutX + (pivotX - tipShutX) * arm_;
    float tipY = pivotY - armLen * arm_;
    float dx = tipX - pivotX;
    float dy = tipY - pivotY;
    bool horiz = std::fabs(dx) >= std::fabs(dy);
    float thick = std::clamp(L.ppm * 0.42f, 5.f, 14.f);
    int n = 12;
    int lampPal = arm_ < kShut ? PAL_ALERT : (arm_ > 0.82f ? PAL_GOOD : PAL_GOLD);
    spr(art_.lamp, pivotX + shx, pivotY, thick * 1.6f, lampPal, false, 0, false, false);
    spr(art_.post, L.x + shx, L.y, postH, PAL_POST, false, fog, true, false);
    spr(art_.post, R.x + shx, R.y, postH * 0.92f, PAL_POST, true, fog, true, false);
    for (int i = 0; i < n; ++i) {
        float t = (float(i) + 0.5f) / float(n);
        float x = pivotX + dx * t;
        float y = pivotY + dy * t;
        int pal = (i & 1) ? PAL_WHITE : PAL_RED;
        if (horiz)
            sprBox(art_.stripe, x + shx, y, std::fabs(dx) / float(n) + 2.f, thick, pal, fog, false);
        else
            sprBox(art_.stripe, x + shx, y, thick, std::fabs(dy) / float(n) + 2.f, pal, fog, false);
    }
    float lineW = (kRoadHalf * 2.f - 0.6f) * L.ppm;
    sprBox(art_.stripe, (L.x + R.x) * 0.5f + shx, L.y - 1.f, lineW, std::max(2.f, L.ppm * 0.16f), PAL_WHITE, fog, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.6f) * 3.4f * std::min(shake_, 1.f);
    road(shx);

    if (mode_ == Mode::Title) text("GATE COLUMN", 160.f + shx, 30.f, 1.15f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 34.f, 1.15f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text(reason_, 160.f + shx, 34.f, std::strlen(reason_) > 16 ? 0.72f : 0.95f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 34.f, 1.2f, PAL_GOLD);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(48);
    for (int i = 0; i < (int)rigs_.size(); ++i) items.push_back({rigs_[i].z, 0, i});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({puffs_[i].z, 1, i});
    for (int i = 0; i < (int)props_.size(); ++i) items.push_back({props_[i].z, 2, i});
    items.push_back({kLine - 0.35f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 3) {
            drawGate(shx);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[it.id];
            Proj p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(p.ppm * (1.4f + u * 1.6f), 4.f, 28.f);
            spr(art_.dust, p.x + shx, p.y - u * 10.f, h, PAL_FX, false, fogFor(f.z), false, false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[it.id];
            Proj p = project(bend(pr.z) + pr.lat, pr.z);
            if (!p.ok) continue;
            int fog = fogFor(pr.z);
            float h = std::clamp(pr.h * p.ppm, 4.f, 96.f);
            if (pr.kind == 3)
                spr(art_.booth, p.x + shx, p.y, h, PAL_BOOTH, false, fog, true, false);
            else if (pr.kind == 2)
                spr(art_.sign, p.x + shx, p.y, h, PAL_FLAG, false, fog, true, false);
            else {
                const gs::Mipped& m = pr.kind == 1 ? art_.tree[1] : art_.tree[0];
                spr(m, p.x + shx, p.y, h, PAL_TREE, pr.lat > 0, fog, true, false);
            }
            continue;
        }
        const Rig& r = rigs_[it.id];
        if (r.z < kZNear + 0.2f) continue;
        Proj p = project(bend(r.z) + r.lat, r.z);
        if (!p.ok) continue;
        int fog = fogFor(r.z);
        const gs::Mipped* body = &art_.truck;
        int pal = PAL_TRUCK;
        if (r.kind == 0) {
            body = &art_.car;
            pal = PAL_CAR;
        } else if (r.kind == 1) {
            body = &art_.van;
            pal = PAL_VAN;
        }
        float h = std::clamp(tallOf(r.kind) * p.ppm, 3.f, 86.f);
        spr(art_.shadow, p.x + shx, p.y, h * 0.28f, PAL_FX, false, 0, false, true);
        spr(*body, p.x + shx, p.y, h, pal, false, fog, true, false);
        if (r.column && r.slot == 0) {
            float bob = std::sin(t_ * 5.5f) * 1.6f;
            spr(art_.pennant, p.x + shx + h * 0.12f, p.y - h + bob, h * 0.32f, PAL_FLAG, false, fog, true, false);
        }
    }

    float anim = t_;
    spr(art_.cloud, 70.f + std::sin(anim * 0.22f) * 8.f + shx, 22.f, 14.f, PAL_FX, false, 0, false, false);
    spr(art_.cloud, 196.f + std::sin(anim * 0.17f + 1.f) * 10.f + shx, 16.f, 12.f, PAL_FX, true, 0, false, false);
    spr(art_.sun, 286.f + shx, 20.f, 14.f, PAL_FX, false, 0, false, false);

    bool shut = arm_ < kShut;
    if (mode_ == Mode::Title) {
        hudC(23, "YOU HAVE THE GATE", PAL_TEXT);
        hudC(24, "STOP THE COLUMN ON THE ROAD", PAL_GOLD);
        hudC(25, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(26, "DOWN/C SHUTS  UP/X OPENS", PAL_TEXT);
        hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(26, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(1, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(27, "START", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(1, reason_, PAL_ALERT);
        hudC(2, "ANYTHING ELSE IS A LOSS", PAL_TEXT);
        hudC(27, "START RETRIES", PAL_TEXT);
    } else {
        hud(1, 0, shut ? "GATE SHUT" : "GATE OPEN", shut ? PAL_ALERT : PAL_GOOD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(28, 0, buf, PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "LET THEM PASS")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "SHUT THE GATE")) pal = PAL_GOLD;
        else if (!std::strcmp(h, "TOO CLOSE")) pal = PAL_ALERT;
        hudC(1, h, pal);
        hudC(27, "DOWN/C SHUTS  UP/X OPENS", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        arm_ = 0.18f + 0.34f * (0.5f + 0.5f * std::sin(t_ * 0.7f));
        motor_ = 1.f;
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            motor_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        motor_ = 0.f;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        motor_ = 0.f;
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(200, 40, 30);
    else if (mode_ == Mode::Play && arm_ < kShut) sys.setLight(180, 40, 30);
    else if (mode_ == Mode::Play) sys.setLight(40, 150, 70);
    else sys.setLight(180, 140, 60);
}

}  // namespace colu
