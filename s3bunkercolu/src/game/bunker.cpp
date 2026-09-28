#include "game/bunker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace bcol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 58.f;
constexpr float kSpan = 152.f;
constexpr float kZNear = 7.2f;
constexpr float kPpm = 24.f;
constexpr float kRoadHalf = 5.0f;
constexpr float kBarZ = 18.5f;
constexpr float kLine = 16.2f;
constexpr float kPanic = 30.f;
constexpr float kGap = 3.8f;
constexpr float kSpace = 8.2f;
constexpr float kBrake = 12.f;
constexpr float kMouth = -6.4f;
constexpr float kTruckV = 5.35f;
constexpr float kLeadZ = 108.f;
constexpr float kArmRate = 1.45f;
constexpr float kAcross = 0.82f;

float bend(float z) {
    float u = std::max(0.f, z - 22.f);
    return std::sin(u * 0.022f) * u * 0.035f;
}

float parkLat(int slot) { return (slot & 1) ? -0.7f : 0.55f; }

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

bool Game::across() const { return arm_ >= kAcross; }

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
    for (int i = 0; i < 6; ++i) {
        Prop tree;
        tree.z = 28.f + float(i) * 14.f;
        tree.lat = kRoadHalf + 2.6f + float(i % 2) * 0.4f;
        tree.h = 5.5f;
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
        r.haltZ = kBarZ + kGap + (column ? float(slot) * kSpace : 0.f);
        r.side = (slot & 1) ? 1.f : -1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(false, 0, 34.f, 12.f, 2.4f);
        for (int i = 0; i < kColumn; ++i) add(true, i, 70.f + float(i) * kSpace, kTruckV, parkLat(i) * 0.2f);
        return;
    }
    add(false, 0, 38.f, 12.4f, 2.35f);
    add(false, 1, 54.f, 11.2f, 2.1f);
    for (int i = 0; i < kColumn; ++i) add(true, i, kLeadZ + float(i) * kSpace, kTruckV, parkLat(i) * 0.15f);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    arm_ = 0.15f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE BUNKER IS QUIET";
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
    arm_ = 0.f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE BUNKER IS QUIET";
    blip(440.f);
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
    sys_->apu.noiseBurst(0.16f, 700.f, 0.16f);
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
    sys_->rumble(0.2f, 0.45f, 140);
}

float Game::botDir() const {
    if (!civsClear()) return arm_ > 0.02f ? -1.f : 0.f;
    if (arm_ < kAcross + 0.06f) return 1.f;
    return 0.f;
}

const char* Game::hint() const {
    if (!civsClear()) return "LET THEM PASS";
    if (!across()) return "DROP THE BAR";
    if (stopped_ >= kColumn) return "HOLD THE BUNKER";
    return "HOLD THE BAR";
}

void Game::stepRig(Rig& r) {
    bool gate = across();
    if (r.passed) {
        r.z -= r.cruise * kDt;
        return;
    }
    if (r.spooked) {
        r.lat += r.side * 5.8f * kDt;
        r.z -= std::max(r.speed, 2.f) * kDt;
        if (std::fabs(r.lat) > kRoadHalf - 0.05f) lose("OFF THE ROAD");
        else if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.column) {
        if (gate && r.z < kBarZ + 8.f) {
            r.z -= r.speed * kDt;
            if (r.z <= kBarZ + 0.35f) lose("NOT THE COLUMN");
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
        r.speed = std::max(r.speed, 0.7f);
    }
    if (!gate) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 4.f * kDt);
        r.z -= r.speed * kDt;
        float lane = parkLat(r.slot);
        r.lat += (lane - r.lat) * std::min(1.f, 3.f * kDt);
        if (r.z < kLine) {
            r.passed = true;
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.3f) {
            r.spooked = true;
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
        if (std::fabs(r.lat) > kRoadHalf * 0.75f) lose("OFF THE ROAD");
        return;
    }
    if (dist > kBrake) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.3f));
        a = std::min(a, 26.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        if (r.speed > 0.45f && r.puff <= 0.f && puffs_.size() < 16) {
            r.puff = 0.11f;
            puffs_.push_back({r.z, r.lat, 0.f, 0.45f});
        }
    }
    r.lat += (park - r.lat) * std::min(1.f, 3.5f * kDt);
    if (r.z <= r.haltZ || (r.speed < 0.2f && dist < 1.4f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
    } else if (r.z < kLine) {
        ++through_;
        lose("THE COLUMN PASSED");
    }
}

void Game::update() {
    t_ += kDt;
    float dir = 0.f;
    if (bot_) {
        dir = botDir();
    } else {
        const gs::Pad& pad = sys_->pad;
        bool out = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) ||
                   pad.axisX > 0.35f;
        bool in = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_UP) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) ||
                  pad.axisX < -0.35f;
        if (out && !in) dir = 1.f;
        else if (in && !out) dir = -1.f;
    }
    float before = arm_;
    arm_ = std::clamp(arm_ + dir * kArmRate * kDt, 0.f, 1.f);
    if ((before < kAcross) != (arm_ < kAcross)) blip(arm_ >= kAcross ? 160.f : 380.f);

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
        if (r.stopped && !r.spooked && r.z >= kLine && across()) ++held;
    }
    stopped_ = held;
    if (clear && across() && held == kColumn && through_ == 0) {
        settle_ += kDt;
        if (settle_ >= 0.4f) win();
    } else {
        settle_ = 0.f;
    }
    if (mode_ == Mode::Play && t_ > 32.f) lose("THE COLUMN DID NOT STOP");
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
    if (mode_ == Mode::Play && arm_ > 0.02f && arm_ < 0.98f) sys_->apu.tone(1, 90.f + arm_ * 40.f, 0.03f);
    else sys_->apu.tone(1, 0.f, 0.f);
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.13f) {
            static const float good[] = {330.f, 415.f, 523.f, 659.f};
            static const float bad[] = {196.f, 155.f, 110.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(0, 82.f, 0.016f);
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        arm_ = 0.12f + 0.08f * std::sin(t_);
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
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 3, 2) : gs::rgb4(8, 9, 10));
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(int(2.f + u * 6.f), 0, 15), std::clamp(int(3.f + u * 5.f), 0, 15),
                                         std::clamp(int(8.f - u * 2.f), 0, 15));
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
        int f = int(std::clamp((0.18f - t) / 0.18f, 0.f, 1.f) * 10.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 4.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(3, 4, 2);
    }
}

void Game::drawBunker(float shx) {
    float z = kBarZ;
    float mouthX = bend(z) + kMouth;
    float tipLat = kMouth + (kRoadHalf * 0.95f - kMouth) * arm_;
    Spot mouth = project(mouthX, z);
    Spot tip = project(bend(z) + tipLat, z);
    if (!mouth.ok) return;
    int fog = fogFor(z);
    float bh = std::clamp(4.8f * mouth.ppm, 36.f, 170.f);
    spr(art_.bunker, mouth.x + shx - mouth.ppm * 0.4f, mouth.y, bh, PAL_BUNKER, false, fog, true);
    float slitH = std::clamp(0.45f * mouth.ppm, 4.f, 18.f);
    spr(art_.slit, mouth.x + shx, mouth.y - bh * 0.62f, slitH, PAL_LAMP, false, fog, false);
    spr(art_.lamp, mouth.x + shx - mouth.ppm * 0.15f, mouth.y - bh * 0.92f, slitH * 1.3f, PAL_LAMP, false, fog, false);
    if (tip.ok && arm_ > 0.04f) {
        float span = tip.x - mouth.x;
        int n = std::clamp(int(std::fabs(span) / 14.f) + 1, 1, 14);
        float thick = std::max(4.f, mouth.ppm * 0.22f);
        for (int i = 0; i < n; ++i) {
            float u = (float(i) + 0.5f) / float(n);
            float x = mouth.x + span * u;
            float y = mouth.y - bh * 0.42f;
            sprBox(art_.bar, x + shx, y, std::max(6.f, std::fabs(span) / float(n) + 2.f), thick,
                   (i & 1) ? PAL_BAR : PAL_AMBER, fog);
        }
        spr(art_.stripe, tip.x + shx, mouth.y - bh * 0.42f, thick * 1.4f, PAL_ALERT, false, fog, false);
    }
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot p = project(bend(pr.z) + pr.lat, pr.z);
    if (!p.ok) return;
    spr(art_.tree, p.x + shx, p.y, std::clamp(pr.h * p.ppm, 8.f, 140.f), PAL_TREE, false, fogFor(pr.z), true);
}

void Game::drawRig(const Rig& r, float shx) {
    Spot p = project(bend(r.z) + r.lat, r.z);
    if (!p.ok) return;
    float h = std::clamp((r.column ? 2.7f : 1.5f) * p.ppm, 6.f, 120.f);
    spr(r.column ? art_.truck : art_.car, p.x + shx, p.y, h, r.column ? PAL_TRUCK : PAL_CAR, false, fogFor(r.z), true);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.8f) * 3.f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) {
        text("BUNKER COLUMN", 160.f, 26.f, 0.95f, PAL_AMBER);
        text("STOP THE COLUMN", 160.f, 52.f, 0.55f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("ON THE ROAD", 160.f, 28.f, 1.0f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 160.f, 28.f, std::char_traits<char>::length(reason_) > 16 ? 0.62f : 0.85f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 28.f, 1.0f, PAL_AMBER);
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
    items.push_back({kBarZ, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) drawBunker(shx);
        else if (it.kind == 2) drawProp(props_[it.id], shx);
        else if (it.kind == 0) drawRig(rigs_[it.id], shx);
        else {
            const Puff& f = puffs_[it.id];
            Spot p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, p.x + shx, p.y - u * 6.f, std::clamp(p.ppm * (1.1f + u), 4.f, 22.f), PAL_FX, false, fogFor(f.z),
                false);
        }
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char line[40];
        std::snprintf(line, sizeof line, "HELD %d/%d", stopped_, kColumn);
        hud(1, 1, line, PAL_TEXT);
        hudC(25, hint(), across() ? PAL_GOOD : PAL_AMBER);
        hud(1, 26, "A DROP  B LIFT", PAL_TEXT);
    } else if (mode_ == Mode::Title) {
        hudC(25, "START", PAL_AMBER);
        hudC(26, "ONE BUNKER", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(25, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, reason_, PAL_ALERT);
    }
}

}  // namespace bcol
