#include "game/cistern.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cistern {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 56.f;
constexpr float kSpan = 154.f;
constexpr float kZNear = 7.0f;
constexpr float kPpm = 26.f;
constexpr float kRoadHalf = 4.8f;
constexpr float kWetNear = 13.2f;
constexpr float kWetSpan = 9.6f;
constexpr float kLine = 15.4f;
constexpr float kPanic = 36.f;
constexpr float kGap = 3.2f;
constexpr float kSpace = 8.4f;
constexpr float kBrake = 11.5f;
constexpr float kMouth = -6.6f;
constexpr float kTruckV = 5.15f;
constexpr float kLeadZ = 104.f;
constexpr float kSluiceRate = 1.55f;
constexpr float kFlood = 0.84f;

float bend(float z) {
    float u = std::max(0.f, z - 24.f);
    return std::sin(u * 0.018f) * u * 0.028f;
}

float parkLat(int slot) { return (slot & 1) ? -0.55f : 0.62f; }

float haltFor(int slot) { return kWetNear + kWetSpan + kGap + float(slot) * kSpace; }

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

bool Game::flooded() const { return flood_ >= kFlood; }

float Game::wetEdge() const { return kWetNear + kWetSpan * std::clamp(flood_, 0.f, 1.f); }

bool Game::civsClear() const {
    for (const Rig& r : rigs_)
        if (!r.column && !r.passed) return false;
    return true;
}

float Game::leadZ() const {
    for (const Rig& r : rigs_)
        if (r.column && r.slot == 0) return r.z;
    return 999.f;
}

Game::Spot Game::project(float wx, float wz) const {
    Spot p;
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
    return int(std::clamp((0.22f - t) / 0.22f, 0.f, 1.f) * 12.f);
}

void Game::buildProps() {
    props_.clear();
    for (int i = 0; i < 7; ++i) {
        Prop reed;
        reed.z = 18.f + float(i) * 11.f;
        reed.lat = (i & 1) ? -(kRoadHalf + 2.2f) : (kRoadHalf + 2.4f);
        reed.h = 2.4f + float(i % 3) * 0.3f;
        reed.kind = 1;
        props_.push_back(reed);
    }
    for (int i = 0; i < 4; ++i) {
        Prop tree;
        tree.z = 36.f + float(i) * 18.f;
        tree.lat = kRoadHalf + 3.4f;
        tree.h = 5.2f;
        tree.kind = 0;
        props_.push_back(tree);
    }
}

void Game::lay(bool scenic) {
    rigs_.clear();
    puffs_.clear();
    auto add = [&](bool column, int slot, float z, float cruise, float lat) {
        Rig r;
        r.column = column;
        r.slot = slot;
        r.z = z;
        r.lat = lat;
        r.cruise = cruise;
        r.speed = cruise;
        r.haltZ = haltFor(column ? slot : 0);
        r.side = (slot & 1) ? 1.f : -1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(false, 0, 32.f, 11.f, 2.2f);
        for (int i = 0; i < kColumn; ++i) add(true, i, 68.f + float(i) * kSpace, kTruckV, parkLat(i) * 0.2f);
        return;
    }
    add(false, 0, 36.f, 12.2f, 2.2f);
    add(false, 1, 52.f, 11.0f, 1.9f);
    for (int i = 0; i < kColumn; ++i) add(true, i, kLeadZ + float(i) * kSpace, kTruckV, parkLat(i) * 0.12f);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    sluice_ = 0.08f;
    flood_ = 0.05f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE CISTERN IS STILL";
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
    sluice_ = 0.f;
    flood_ = 0.f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE CISTERN IS STILL";
    blip(360.f);
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
    sys_->apu.tone(2, freq, 0.05f);
    blip_ = 0.07f;
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
    hold_ = 1.6f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.16f, 640.f, 0.16f);
    sys_->rumble(0.45f, 0.2f, 160);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.4f;
    fanfare(true);
    sys_->rumble(0.2f, 0.4f, 140);
}

float Game::botDir() const {
    if (!civsClear()) return sluice_ > 0.02f ? -1.f : 0.f;
    if (leadZ() > kPanic + 8.f && flood_ < kFlood + 0.04f) return 1.f;
    if (flood_ < kFlood) return 1.f;
    return sluice_ < 0.98f ? 1.f : 0.f;
}

const char* Game::hint() const {
    if (!civsClear()) return "LET THEM PASS";
    if (!flooded()) return "OPEN THE SLUICE";
    if (stopped_ >= kColumn) return "HOLD THE FLOOD";
    return "HOLD THE WATER";
}

void Game::stepRig(Rig& r) {
    bool gate = flooded();
    float edge = wetEdge();
    if (r.passed) {
        r.z -= r.cruise * kDt;
        return;
    }
    if (r.wet) {
        r.lat += r.side * 4.2f * kDt;
        r.z -= std::max(r.speed, 1.6f) * 0.45f * kDt;
        if (std::fabs(r.lat) > kRoadHalf + 0.4f) lose(r.column ? "OFF THE ROAD" : "NOT THE COLUMN");
        else if (r.z < kLine) {
            if (r.column) ++through_;
            lose(r.column ? "THE COLUMN PASSED" : "NOT THE COLUMN");
        }
        return;
    }
    if (!r.column) {
        if (gate && r.z < edge + 7.f) {
            r.z -= r.speed * kDt;
            if (r.z <= edge + 0.4f) lose("NOT THE COLUMN");
            return;
        }
        r.speed = std::min(r.cruise, r.speed + 3.f * kDt);
        r.z -= r.speed * kDt;
        if (r.z < kLine) r.passed = true;
        return;
    }
    if (r.stopped && gate) {
        r.z = r.haltZ;
        r.speed = 0.f;
        r.lat += (parkLat(r.slot) - r.lat) * std::min(1.f, 4.f * kDt);
        return;
    }
    if (r.stopped && !gate) {
        r.stopped = false;
        r.orderly = false;
        r.speed = std::max(r.speed, 0.8f);
    }
    if (!gate) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 3.5f * kDt);
        r.z -= r.speed * kDt;
        r.lat += (parkLat(r.slot) - r.lat) * std::min(1.f, 3.f * kDt);
        if (flood_ > 0.35f && r.z < edge + 0.3f) {
            r.wet = true;
            return;
        }
        if (r.z < kLine) {
            r.passed = true;
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.4f) {
            r.wet = true;
            return;
        }
        r.orderly = true;
    }
    float dist = r.z - r.haltZ;
    float park = parkLat(r.slot);
    if (dist <= 0.2f) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
        return;
    }
    if (dist > kBrake) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.35f));
        a = std::min(a, 22.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        if (r.speed > 0.5f && r.puff <= 0.f && puffs_.size() < 14) {
            r.puff = 0.12f;
            puffs_.push_back({r.z, r.lat, 0.f, 0.4f});
        }
    }
    r.lat += (park - r.lat) * std::min(1.f, 3.2f * kDt);
    if (r.z < edge - 0.2f) {
        r.wet = true;
        return;
    }
    if (r.z <= r.haltZ || (r.speed < 0.25f && dist < 1.5f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
    }
}

void Game::update() {
    t_ += kDt;
    float dir = 0.f;
    if (bot_) {
        dir = botDir();
    } else {
        const gs::Pad& pad = sys_->pad;
        bool open = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) ||
                    pad.axisX > 0.35f;
        bool shut = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_UP) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) ||
                    pad.axisX < -0.35f;
        if (open && !shut) dir = 1.f;
        else if (shut && !open) dir = -1.f;
    }
    float before = flood_;
    sluice_ = std::clamp(sluice_ + dir * kSluiceRate * kDt, 0.f, 1.f);
    float chase = sluice_ - flood_;
    flood_ = std::clamp(flood_ + std::clamp(chase, -0.35f, 0.72f) * kDt, 0.f, 1.f);
    if ((before < kFlood) != (flood_ < kFlood)) blip(flood_ >= kFlood ? 140.f : 320.f);

    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        stepRig(r);
    }
    if (mode_ != Mode::Play) return;

    int held = 0;
    bool clear = true;
    for (const Rig& r : rigs_) {
        if (!r.column) {
            if (!r.passed) clear = false;
            continue;
        }
        if (r.stopped && !r.wet && r.z >= kLine && flooded()) ++held;
    }
    stopped_ = held;
    if (clear && flooded() && held == kColumn && through_ == 0) {
        settle_ += kDt;
        if (settle_ >= 0.45f) win();
    } else {
        settle_ = 0.f;
    }
    if (mode_ == Mode::Play && t_ > 30.f) lose("THE COLUMN DID NOT STOP");
}

void Game::tickPuffs() {
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }), puffs_.end());
    for (Rig& r : rigs_)
        if (r.puff > 0.f) r.puff -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.8f);
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Play && flood_ > 0.08f && flood_ < 0.96f) sys_->apu.tone(1, 70.f + flood_ * 50.f, 0.035f);
    else if (mode_ == Mode::Play && flooded()) sys_->apu.tone(1, 48.f, 0.02f);
    else sys_->apu.tone(1, 0.f, 0.f);
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.13f) {
            static const float good[] = {262.f, 330.f, 392.f, 523.f};
            static const float bad[] = {180.f, 140.f, 98.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(0, 98.f, 0.014f);
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        flood_ = 0.12f + 0.06f * std::sin(t_ * 1.4f);
        sluice_ = flood_;
        t_ += kDt;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else {
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
    }
    tickPuffs();
    serviceAudio();
    draw();
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
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
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
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 3, 3) : gs::rgb4(7, 9, 11));
    float edge = wetEdge();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(int(3.f + u * 4.f), 0, 15), std::clamp(int(5.f + u * 3.f), 0, 15),
                                         std::clamp(int(9.f - u * 2.f), 0, 15));
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
        rd.v = wz * 26.f;
        rd.pal = uint8_t(PAL_ROAD);
        bool wet = flood_ > 0.12f && wz >= kWetNear - 0.4f && wz <= edge + 0.6f;
        rd.style = wet ? 2 : 1;
        rd.band = (int(std::floor(wz * 0.16f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = wet ? gs::GROUND_WATER : gs::GROUND_LAND;
        int f = int(std::clamp((0.18f - t) / 0.18f, 0.f, 1.f) * 10.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 4.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(2, 4, 3);
    }
    v.roadTime = int(sys_->frame);
}

void Game::drawCistern(float shx) {
    float z = kWetNear + 4.f;
    Spot mouth = project(bend(z) + kMouth, z);
    if (!mouth.ok) return;
    int fog = fogFor(z);
    float bh = std::clamp(5.6f * mouth.ppm, 40.f, 180.f);
    spr(art_.cistern, mouth.x + shx, mouth.y, bh, PAL_STONE, false, fog, true);
    float gateH = std::clamp((0.35f + (1.f - sluice_) * 1.1f) * mouth.ppm, 8.f, 48.f);
    spr(art_.gate, mouth.x + shx + mouth.ppm * 1.15f, mouth.y - bh * 0.22f, gateH, PAL_GATE, false, fog, false);
    if (flood_ > 0.08f) {
        float sw = std::clamp(flood_ * mouth.ppm * 3.2f, 8.f, 90.f);
        float sh = std::clamp(flood_ * mouth.ppm * 0.7f, 4.f, 22.f);
        sprBox(art_.spout, mouth.x + shx + mouth.ppm * 1.8f, mouth.y - bh * 0.18f, sw, sh, PAL_WATER, fog);
    }
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot p = project(bend(pr.z) + pr.lat, pr.z);
    if (!p.ok) return;
    const gs::Mipped& m = pr.kind == 0 ? art_.tree : art_.reed;
    int pal = pr.kind == 0 ? PAL_TREE : PAL_GOOD;
    spr(m, p.x + shx, p.y, std::clamp(pr.h * p.ppm, 6.f, 130.f), pal, pr.lat < 0, fogFor(pr.z), true);
}

void Game::drawRig(const Rig& r, float shx) {
    Spot p = project(bend(r.z) + r.lat, r.z);
    if (!p.ok) return;
    float h = std::clamp((r.column ? 2.55f : 1.35f) * p.ppm, 6.f, 110.f);
    spr(r.column ? art_.truck : art_.car, p.x + shx, p.y, h, r.column ? PAL_TRUCK : PAL_CAR, false, fogFor(r.z), true);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.7f) * 3.f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) {
        text("CISTERN COLUMN", 160.f, 24.f, 0.85f, PAL_AMBER);
        text("STOP THE COLUMN", 160.f, 50.f, 0.52f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("ON THE ROAD", 160.f, 26.f, 1.0f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 160.f, 26.f, std::char_traits<char>::length(reason_) > 16 ? 0.58f : 0.8f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 26.f, 1.0f, PAL_AMBER);
    }

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < (int)rigs_.size(); ++i) items.push_back({rigs_[i].z, 0, i});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({puffs_[i].z, 1, i});
    for (int i = 0; i < (int)props_.size(); ++i) items.push_back({props_[i].z, 2, i});
    items.push_back({kWetNear + 4.f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) drawCistern(shx);
        else if (it.kind == 2) drawProp(props_[it.id], shx);
        else if (it.kind == 0) drawRig(rigs_[it.id], shx);
        else {
            const Puff& f = puffs_[it.id];
            Spot p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, p.x + shx, p.y - u * 5.f, std::clamp(p.ppm * (0.9f + u), 4.f, 20.f), PAL_FX, false, fogFor(f.z),
                false);
        }
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char line[40];
        std::snprintf(line, sizeof line, "HELD %d/%d", stopped_, kColumn);
        hud(1, 1, line, PAL_TEXT);
        hudC(25, hint(), flooded() ? PAL_GOOD : PAL_AMBER);
        hud(1, 26, "A OPEN  B SHUT", PAL_TEXT);
    } else if (mode_ == Mode::Title) {
        hudC(25, "START", PAL_AMBER);
        hudC(26, "YOU HAVE THE CISTERN", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(25, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, reason_, PAL_ALERT);
    }
}

}  // namespace cistern
