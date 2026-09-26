#include "game/depot.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace dcol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 58.f;
constexpr float kSpan = 156.f;
constexpr float kZNear = 8.2f;
constexpr float kPpm = 27.f;
constexpr float kRoadHalf = 5.1f;
constexpr float kLine = 15.6f;
constexpr float kGap = 3.8f;
constexpr float kHalt = 8.0f;
constexpr float kSpawn = 8.6f;
constexpr float kBrake = 13.0f;
constexpr float kPanic = 23.0f;
constexpr float kYardZ = 27.2f;
constexpr float kTurnZ = 28.4f;
constexpr float kBayOpen = 0.68f;
constexpr float kBayShut = 0.45f;
constexpr float kBoomDown = 0.74f;
constexpr float kBayRate = 1.0f;
constexpr float kBoomRate = 1.7f;
constexpr float kWatch = 36.f;
constexpr float kTankerZ = 40.f;
constexpr float kTankerV = 6.4f;
constexpr float kLeadZ = 102.f;
constexpr float kTruckV = 4.9f;
constexpr float kTurnRate = 3.4f;
constexpr float kTruckH = 3.25f;

float bend(float z) {
    float u = std::max(0.f, z - 16.f);
    return std::sin(u * 0.021f) * 1.2f + u * 0.011f;
}

float parkLat(int slot) {
    static const float kPark[] = {0.05f, -0.32f, 0.28f, -0.1f};
    return kPark[slot & 3];
}

float haltOf(int slot) { return kLine + kGap + float(slot) * kHalt; }

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
    if (!(wz > kZNear + 0.08f)) return s;
    float t = kZNear / wz;
    s.ppm = kPpm * t;
    s.y = kHorizon + t * kSpan;
    s.x = 160.f + wx * s.ppm;
    s.ok = true;
    return s;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float fade = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
    return int(fade * 11.f);
}

bool Game::tankerIn() const {
    for (const Rig& r : rigs_)
        if (!r.column && r.berthed) return true;
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
    const float left = -(kRoadHalf + 3.6f);
    const float right = kRoadHalf + 2.5f;
    add(24.2f, left - 0.8f, 6.4f, 5);
    add(21.4f, left + 0.4f, 1.1f, 9);
    add(36.f, left, 3.6f, 0);
    add(50.f, left + 0.4f, 2.3f, 1);
    add(64.f, left - 0.2f, 3.4f, 0);
    add(78.f, left + 0.3f, 2.2f, 2);
    add(94.f, left - 0.6f, 6.8f, 4);
    add(19.5f, right, 3.4f, 3);
    add(18.2f, right - 0.15f, 2.7f, 8);
    add(32.f, right + 0.4f, 2.8f, 6);
    add(46.f, right + 0.2f, 3.1f, 7);
    add(70.f, right + 0.5f, 2.4f, 1);
}

void Game::lay(bool scenic) {
    rigs_.clear();
    puffs_.clear();
    auto add = [&](int kind, bool column, int slot, float z, float lat, float cruise) {
        Rig r;
        r.kind = kind;
        r.column = column;
        r.slot = slot;
        r.z = z;
        r.lat = lat;
        r.cruise = cruise;
        r.speed = scenic ? 0.f : cruise;
        r.haltZ = column ? haltOf(slot) : kLine + 4.f;
        r.side = (slot & 1) ? 1.f : -1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(0, false, 0, 24.f, -3.4f, kTankerV);
        for (int i = 0; i < kColumn; ++i) add(1, true, i, 42.f + float(i) * kSpawn, parkLat(i), kTruckV);
        return;
    }
    add(0, false, 0, kTankerZ, 0.f, kTankerV);
    for (int i = 0; i < kColumn; ++i) add(1, true, i, kLeadZ + float(i) * kSpawn, 0.f, kTruckV);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    bay_ = 0.55f;
    boom_ = 0.2f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE WATCH IS OVER";
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
    bay_ = 0;
    boom_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    bayVel_ = 0;
    boomVel_ = 0;
    reason_ = "THE WATCH IS OVER";
    blip(620.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.2f, 0.1f);
    if (bot_) begin();
    else bootTitle();
}

void Game::blip(float freq) {
    if (fanStep_ >= 0) return;
    sys_->apu.tone(0, freq, 0.055f);
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
    sys_->apu.noiseBurst(0.2f, 280.f, 0.2f);
    sys_->rumble(0.5f, 0.18f, 180);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.45f;
    fanfare(true);
    sys_->rumble(0.25f, 0.5f, 160);
}

const char* Game::hint() const {
    if (!tankerIn()) {
        if (boom_ >= kBoomDown * 0.55f) return "BOOM IS EARLY";
        if (bay_ < kBayOpen) return "OPEN THE BAY";
        return "HOLD THE BAY";
    }
    if (bay_ > kBayShut) return "SHUT THE BAY";
    if (boom_ < kBoomDown) {
        if (leadZ() <= kPanic + 2.f) return "TOO CLOSE";
        return "DROP THE BOOM";
    }
    return "HOLD THE BOOM";
}

void Game::stepTanker(Rig& r, bool boomDown, bool bayOpen) {
    if (r.berthed) {
        r.z = 22.4f;
        r.lat = -kRoadHalf - 2.15f;
        r.speed = 0.f;
        r.turning = false;
        return;
    }
    if (r.lat < -kRoadHalf * 0.82f) {
        r.lat -= kTurnRate * kDt;
        r.z -= 2.05f * kDt;
        r.speed = 2.05f;
        if (r.lat <= -kRoadHalf - 1.55f) {
            r.berthed = true;
            blip(520.f);
        } else if (r.z < kLine) {
            lose("MISSED THE BAY");
        }
        return;
    }
    if (boomDown) {
        r.turning = false;
        r.speed = std::max(0.f, r.speed - 16.f * kDt);
        r.z -= r.speed * kDt;
        r.lat += (0.f - r.lat) * std::min(1.f, 3.f * kDt);
        if (r.speed < 0.45f || r.z <= kLine + 2.4f) lose("NOT THE COLUMN");
        return;
    }
    if (r.turning && !bayOpen) r.turning = false;
    if (bayOpen && (r.turning || r.z <= kTurnZ)) {
        r.turning = true;
        r.lat -= kTurnRate * kDt;
        r.z -= 2.15f * kDt;
        r.speed = 2.15f;
        if (r.z < kLine) lose("MISSED THE BAY");
        return;
    }
    r.speed = r.cruise;
    r.z -= r.speed * kDt;
    r.lat += (0.f - r.lat) * std::min(1.f, 2.5f * kDt);
    if (r.z < kLine) lose("MISSED THE BAY");
}

void Game::stepTruck(Rig& r, bool boomDown, bool bayLeak) {
    if (r.passed) {
        r.z -= std::max(r.cruise, 2.f) * kDt;
        return;
    }
    if (r.spooked) {
        r.lat += r.side * 4.4f * kDt;
        r.speed = std::max(r.speed, 2.6f);
        r.z -= r.speed * kDt;
        if (std::fabs(r.lat) > kRoadHalf - 0.1f) lose("OFF THE ROAD");
        else if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (bayLeak && !r.stopped && r.z < kYardZ) {
        r.orderly = false;
        r.lat -= 3.35f * kDt;
        r.speed = std::max(r.speed, 2.5f);
        r.z -= r.speed * kDt;
        if (r.lat < -kRoadHalf - 0.3f) lose("INTO THE YARD");
        else if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (r.stopped && boomDown) {
        r.z = r.haltZ;
        r.lat = parkLat(r.slot);
        r.speed = 0.f;
        return;
    }
    if (r.stopped && !boomDown) {
        r.stopped = false;
        r.orderly = false;
        r.speed = std::max(r.speed, 0.9f);
    }
    if (!boomDown) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 3.5f * kDt);
        r.z -= r.speed * kDt;
        r.lat += (0.f - r.lat) * std::min(1.f, 3.f * kDt);
        if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.05f) {
            r.spooked = true;
            r.side = (r.slot & 1) ? 1.f : -1.f;
            blip(130.f);
            return;
        }
        r.orderly = true;
    }
    if (r.z < kLine) {
        ++through_;
        lose("THE COLUMN PASSED");
        return;
    }
    float park = parkLat(r.slot);
    float dist = r.z - r.haltZ;
    if (dist <= 0.22f || (dist < 1.2f && r.speed < 0.32f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(86.f + float(r.slot) * 24.f);
        r.stopped = true;
        return;
    }
    if (dist > kBrake) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 2.2f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.25f));
        a = std::min(a, 26.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 3.8f * kDt);
        if (r.speed > 0.45f && r.puff <= 0.f && puffs_.size() < 16) {
            r.puff = 0.15f;
            Puff puff;
            puff.z = r.z + 0.7f;
            puff.lat = r.lat;
            puff.age = 0.f;
            puff.life = 0.5f;
            puffs_.push_back(puff);
        }
    }
    if (r.z <= r.haltZ || (r.speed < 0.22f && dist < 1.8f)) {
        if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
            return;
        }
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(86.f + float(r.slot) * 24.f);
        r.stopped = true;
    }
}

void Game::update() {
    t_ += kDt;
    float bayDir = -1.f;
    float boomDir = -1.f;
    if (bot_) {
        bool in = tankerIn();
        bayDir = in ? -1.f : 1.f;
        boomDir = in ? 1.f : -1.f;
    } else {
        const gs::Pad& pad = sys_->pad;
        bool open = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
        bool shut = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
        if (open && !shut) bayDir = 1.f;
        else bayDir = shut ? -1.f : -0.7f;
        bool drop = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.axisY < -0.35f;
        bool lift = pad.down(gs::BTN_UP) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.axisY > 0.35f;
        boomDir = (drop && !lift) ? 1.f : -1.f;
    }
    float prevBay = bay_;
    float prevBoom = boom_;
    bay_ = std::clamp(bay_ + bayDir * kBayRate * kDt, 0.f, 1.f);
    boom_ = std::clamp(boom_ + boomDir * kBoomRate * kDt, 0.f, 1.f);
    bayVel_ = (bay_ - prevBay) / kDt;
    boomVel_ = (boom_ - prevBoom) / kDt;
    bool boomDown = boom_ >= kBoomDown;
    bool bayOpen = bay_ >= kBayOpen;
    bool bayLeak = bay_ > kBayShut;
    if ((prevBoom >= kBoomDown) != boomDown) {
        blip(boomDown ? 140.f : 380.f);
        sys_->rumble(boomDown ? 0.28f : 0.08f, boomDown ? 0.42f : 0.12f, boomDown ? 90 : 40);
    }

    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        if (r.column) stepTruck(r, boomDown, bayLeak);
        else stepTanker(r, boomDown, bayOpen);
    }
    if (mode_ != Mode::Play) return;

    int held = 0;
    for (const Rig& r : rigs_) {
        if (!r.column || !r.stopped || r.spooked) continue;
        if (r.z >= kLine - 0.05f && std::fabs(r.lat) <= kRoadHalf * 0.75f) ++held;
    }
    stopped_ = held;
    bool file = boomDown && tankerIn() && through_ == 0 && held == kColumn;
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
            static const float good[] = {392.f, 494.f, 587.f, 784.f};
            static const float bad[] = {220.f, 174.f, 146.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Title && beep_ <= 0.f) {
        sys_->apu.tone(0, 110.f, 0.018f);
    }
    bool slide = std::fabs(bayVel_) > 0.15f && (mode_ == Mode::Play || mode_ == Mode::Title);
    if (slide) sys_->apu.tone(1, 48.f + bay_ * 36.f, 0.03f);
    else sys_->apu.tone(1, 0.f, 0.f);
    bool rolling = false;
    if (mode_ == Mode::Play) {
        for (const Rig& r : rigs_)
            if (!r.berthed && !r.passed && r.speed > 0.4f) rolling = true;
    }
    if (rolling) sys_->apu.tone(2, 46.f, 0.026f);
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
    uint16_t skyTop = gs::rgb4(1, 2, 5);
    uint16_t skyHor = mode_ == Mode::Fail ? gs::rgb4(10, 3, 2) : gs::rgb4(12, 7, 3);
    v.setFogColor(skyHor);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            int r = int(1.f + u * 11.f);
            int g = int(2.f + u * 5.f);
            int b = int(5.f + u * (mode_ == Mode::Fail ? -2.f : -1.f));
            if (mode_ == Mode::Fail) r = std::min(15, r + 2);
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
        rd.v = wz * 28.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.18f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.16f - t) / 0.16f, 0.f, 1.f);
        int f = int(fogT * 10.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 5.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = skyTop;
    }
}

void Game::drawBoom(float shx) {
    float z = kLine + 0.15f;
    float left = bend(z) - kRoadHalf - 0.15f;
    float right = bend(z) + kRoadHalf + 0.15f;
    Spot L = project(left, z);
    Spot R = project(right, z);
    if (!L.ok || !R.ok) return;
    int fog = fogFor(z);
    float postH = std::clamp(3.7f * L.ppm, 22.f, 130.f);
    spr(art_.post, L.x + shx, L.y, postH, PAL_DEPOT, false, fog, true);
    spr(art_.post, R.x + shx, R.y, postH, PAL_DEPOT, true, fog, true);
    float midX = (L.x + R.x) * 0.5f + shx;
    float span = std::fabs(R.x - L.x);
    float topY = L.y - postH * 0.9f;
    sprBox(art_.block, midX, topY, span, std::max(4.f, L.ppm * 0.28f), PAL_DEPOT, fog);
    float drop = 0.14f + 0.72f * boom_;
    float barY = L.y - postH * (1.f - drop);
    float thick = std::clamp(L.ppm * 0.42f, 6.f, 16.f);
    int n = 10;
    for (int i = 0; i < n; ++i) {
        float u = (float(i) + 0.5f) / float(n);
        float x = L.x + (R.x - L.x) * u + shx;
        sprBox(art_.stripe, x, barY, span / float(n) + 2.f, thick, PAL_BOOM, fog);
    }
    int lampPal = boom_ >= kBoomDown ? PAL_ALERT : PAL_GOOD;
    spr(art_.lamp, L.x + shx, topY, thick * 1.7f, lampPal, false, 0, false);
    spr(art_.lamp, R.x + shx, topY - 1.f, thick * 1.35f, bay_ >= kBayOpen ? PAL_GOOD : PAL_ALERT, false, 0, false);
    Spot line = project(bend(kLine), kLine);
    if (!line.ok) return;
    float lineW = (kRoadHalf * 2.f - 0.8f) * line.ppm;
    sprBox(art_.block, line.x + shx, line.y, lineW, std::max(2.f, line.ppm * 0.14f), PAL_AMBER, fogFor(kLine));
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot s = project(bend(pr.z) + pr.lat, pr.z);
    if (!s.ok) return;
    int fog = fogFor(pr.z);
    float h = std::clamp(pr.h * s.ppm, 6.f, 150.f);
    float x = s.x + shx;
    if (pr.kind == 5) {
        spr(art_.warehouse, x, s.y, h, PAL_DEPOT, false, fog, true);
        float doorH = h * 0.58f;
        float doorW = doorH * float(art_.door.w) / float(std::max(1, art_.door.h));
        float slide = bay_ * doorW * 0.92f;
        sprBox(art_.door, x - h * 0.16f - slide, s.y - h * 0.22f, doorW, doorH, PAL_DEPOT, fog);
        return;
    }
    if (pr.kind == 9) {
        float w = h * 4.2f;
        sprBox(art_.apron, x, s.y, w, h, PAL_DEPOT, fog);
        return;
    }
    const gs::Mipped* m = &art_.tank;
    int pal = PAL_YARD;
    bool feet = true;
    if (pr.kind == 1) m = &art_.drums;
    else if (pr.kind == 2) m = &art_.crates;
    else if (pr.kind == 3) m = &art_.shack;
    else if (pr.kind == 4) m = &art_.crane;
    else if (pr.kind == 6) {
        m = &art_.sign;
        pal = PAL_SIGN;
    } else if (pr.kind == 7) {
        spr(art_.post, x, s.y, h, PAL_DEPOT, false, fog, true);
        spr(art_.lamp, x, s.y - h * 0.92f, h * 0.28f, PAL_AMBER, false, fog, false);
        return;
    } else if (pr.kind == 8) {
        float bob = std::sin(t_ * 2.2f) * 1.4f;
        spr(art_.shadow, x, s.y, h * 0.28f, PAL_FX, false, 0, false);
        spr(art_.watch, x, s.y - bob, h, PAL_COAT, false, fog, true);
        return;
    } else {
        pal = PAL_YARD;
    }
    spr(art_.shadow, x, s.y, h * 0.22f, PAL_FX, false, 0, false);
    spr(*m, x, s.y, h, pal, pr.lat > 0, fog, feet);
}

void Game::drawRig(const Rig& r, float shx) {
    if (r.z < kZNear + 0.3f) return;
    Spot s = project(bend(r.z) + r.lat, r.z);
    if (!s.ok) return;
    int fog = fogFor(r.z);
    float worldH = r.column ? kTruckH : 3.05f;
    float h = std::clamp(worldH * s.ppm, 4.f, 120.f);
    const gs::Mipped& body = r.column ? art_.truck : art_.tanker;
    int pal = r.column ? PAL_TRUCK : PAL_TANKER;
    float x = s.x + shx;
    if (r.column && r.slot == 0) {
        float bob = std::sin(t_ * 5.f + r.z) * 1.5f;
        spr(art_.pennant, x + h * 0.05f, s.y - h + bob, h * 0.34f, PAL_ALERT, false, fog, true);
    }
    spr(body, x, s.y, h, pal, false, fog, true);
    spr(art_.shadow, x, s.y, h * 0.26f, PAL_FX, false, 0, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 70.f) * 4.2f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) text("DEPOT COLUMN", 160.f + shx, 28.f, 1.f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 30.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text("WATCH OVER", 160.f + shx, 30.f, 1.05f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 30.f, 1.1f, PAL_AMBER);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(40);
    for (int i = 0; i < int(rigs_.size()); ++i) items.push_back({rigs_[size_t(i)].z, 0, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 2, i});
    items.push_back({kLine + 0.15f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) {
            drawBoom(shx);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot s = project(bend(f.z) + f.lat, f.z);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(s.ppm * (1.2f + u * 1.5f), 4.f, 28.f);
            spr(art_.dust, s.x + shx, s.y - u * 8.f, h, PAL_FX, false, fogFor(f.z), false);
            continue;
        }
        if (it.kind == 2) {
            drawProp(props_[size_t(it.id)], shx);
            continue;
        }
        drawRig(rigs_[size_t(it.id)], shx);
    }

    float drift = std::fmod(t_ * 7.f, 400.f);
    spr(art_.cloud, drift - 50.f, 18.f, 16.f, PAL_NIGHT, false, 2, false);
    spr(art_.cloud, std::fmod(drift + 210.f, 400.f) - 40.f, 30.f, 12.f, PAL_NIGHT, true, 3, false);
    spr(art_.sun, 274.f + shx * 0.2f, 22.f, 18.f, PAL_FX, false, 0, false);

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(22, "STOP THE COLUMN ON THE ROAD", PAL_AMBER);
        hudC(23, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(24, "LEFT OPENS THE BAY", PAL_GOOD);
        hudC(25, "DOWN DROPS THE BOOM", PAL_TEXT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_TEXT);
        hudC(26, "ESC TITLE", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(24, "THE WATCH HOLDS", PAL_AMBER);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "THE WATCH IS OVER", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else {
        bool bayOpen = bay_ >= kBayOpen;
        bool boomDown = boom_ >= kBoomDown;
        hud(1, 0, bayOpen ? "BAY OPEN" : "BAY SHUT", bayOpen ? PAL_GOOD : PAL_AMBER);
        hud(30, 0, boomDown ? "BOOM" : "ROAD", boomDown ? PAL_ALERT : PAL_GOOD);
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(1, 1, buf, PAL_TEXT);
        int left = std::max(0, int(std::ceil(kWatch - t_)));
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(30, 1, buf, left <= 8 ? PAL_ALERT : PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "OPEN THE BAY") || !std::strcmp(h, "HOLD THE BAY")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "DROP THE BOOM") || !std::strcmp(h, "SHUT THE BAY")) pal = PAL_AMBER;
        else if (!std::strcmp(h, "TOO CLOSE") || !std::strcmp(h, "BOOM IS EARLY")) pal = PAL_ALERT;
        hudC(2, h, pal);
        hudC(27, "LEFT BAY    DOWN BOOM", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        bay_ = 0.58f + 0.34f * std::sin(t_ * 0.65f);
        boom_ = 0.22f + 0.16f * std::sin(t_ * 0.9f + 0.6f);
        bayVel_ = std::cos(t_ * 0.65f) * 0.22f;
        boomVel_ = 0.f;
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            bayVel_ = boomVel_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        bayVel_ = boomVel_ = 0.f;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        bayVel_ = boomVel_ = 0.f;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Fail) sys.setLight(190, 36, 28);
    else if (mode_ == Mode::Play && boom_ >= kBoomDown) sys.setLight(180, 40, 30);
    else if (mode_ == Mode::Play && bay_ >= kBayOpen) sys.setLight(40, 150, 70);
    else sys.setLight(160, 110, 40);
}

}  // namespace dcol
