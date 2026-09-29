#include "game/palisade.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace pcol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 62.f;
constexpr float kSpan = 148.f;
constexpr float kZNear = 6.8f;
constexpr float kPpm = 26.f;
constexpr float kRoadHalf = 4.6f;
constexpr float kBarZ = 21.f;
constexpr float kLine = 14.8f;
constexpr float kPanic = 28.f;
constexpr float kGap = 4.4f;
constexpr float kSpace = 7.6f;
constexpr float kBrake = 11.f;
constexpr float kPost = 5.4f;
constexpr float kWagonV = 5.1f;
constexpr float kLeadZ = 96.f;
constexpr float kArmRate = 1.55f;
constexpr float kAcross = 0.84f;

float bend(float z) {
    float u = std::max(0.f, z - 18.f);
    return std::sin(u * 0.018f) * u * 0.028f;
}

float parkLat(int slot) { return (slot & 1) ? -0.55f : 0.4f; }

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

bool Game::across() const { return arm_ >= kAcross; }

bool Game::localsClear() const {
    for (const Rig& r : rigs_)
        if (!r.column && !r.passed) return false;
    return true;
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
    return int(std::clamp((0.24f - t) / 0.24f, 0.f, 1.f) * 12.f);
}

void Game::buildProps() {
    props_.clear();
    for (int i = 0; i < 7; ++i) {
        Prop tree;
        tree.z = 30.f + float(i) * 12.f;
        tree.lat = (i & 1 ? 1.f : -1.f) * (kRoadHalf + 2.4f + float(i % 3) * 0.35f);
        tree.h = 4.2f + float(i % 3) * 0.6f;
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
        add(false, 0, 32.f, 10.f, 1.8f);
        for (int i = 0; i < kColumn; ++i) add(true, i, 64.f + float(i) * kSpace, kWagonV, parkLat(i) * 0.2f);
        return;
    }
    add(false, 0, 34.f, 11.6f, 1.9f);
    add(false, 1, 48.f, 10.4f, 1.5f);
    for (int i = 0; i < kColumn; ++i) add(true, i, kLeadZ + float(i) * kSpace, kWagonV, parkLat(i) * 0.12f);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    arm_ = 0.22f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE WATCH IS QUIET";
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
    reason_ = "THE WATCH IS QUIET";
    blip(420.f);
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
    sys_->apu.noiseBurst(0.16f, 620.f, 0.16f);
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
    if (!localsClear()) return arm_ > 0.02f ? -1.f : 0.f;
    if (arm_ < kAcross + 0.08f) return 1.f;
    return 0.f;
}

const char* Game::hint() const {
    if (!localsClear()) return "LET THE CARTS PASS";
    if (!across()) return "DROP THE TIMBER";
    if (stopped_ >= kColumn) return "HOLD THE PALISADE";
    return "HOLD THE TIMBER";
}

void Game::stepRig(Rig& r) {
    bool gate = across();
    if (r.passed) {
        r.z -= r.cruise * kDt;
        return;
    }
    if (r.spooked) {
        r.lat += r.side * 5.4f * kDt;
        r.z -= std::max(r.speed, 2.f) * kDt;
        if (std::fabs(r.lat) > kRoadHalf - 0.08f) lose("OFF THE ROAD");
        else if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.column) {
        if (gate && r.z < kBarZ + 7.5f) {
            r.z -= r.speed * kDt;
            if (r.z <= kBarZ + 0.4f) lose("NOT THE COLUMN");
            return;
        }
        r.speed = std::min(r.cruise, r.speed + 2.6f * kDt);
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
        r.speed = std::min(r.cruise, r.speed + 3.6f * kDt);
        r.z -= r.speed * kDt;
        r.lat += (parkLat(r.slot) - r.lat) * std::min(1.f, 2.8f * kDt);
        if (r.z < kLine) {
            r.passed = true;
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.4f) {
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
        return;
    }
    if (dist > kBrake) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.35f));
        a = std::min(a, 24.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        if (r.speed > 0.4f && r.puff <= 0.f && puffs_.size() < 14) {
            r.puff = 0.12f;
            puffs_.push_back({r.z, r.lat, 0.f, 0.4f});
        }
    }
    r.lat += (park - r.lat) * std::min(1.f, 3.2f * kDt);
    if (r.z <= r.haltZ || (r.speed < 0.22f && dist < 1.5f)) {
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
        bool drop = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) ||
                    pad.axisX > 0.35f;
        bool lift = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_UP) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) ||
                    pad.axisX < -0.35f;
        if (drop && !lift) dir = 1.f;
        else if (lift && !drop) dir = -1.f;
    }
    float before = arm_;
    arm_ = std::clamp(arm_ + dir * kArmRate * kDt, 0.f, 1.f);
    if ((before < kAcross) != (arm_ < kAcross)) blip(arm_ >= kAcross ? 150.f : 360.f);

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
        if (settle_ >= 0.45f) win();
    } else {
        settle_ = 0.f;
    }
    if (mode_ == Mode::Play && t_ > 36.f) lose("THE COLUMN DID NOT STOP");
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
    if (mode_ == Mode::Play && arm_ > 0.02f && arm_ < 0.98f) sys_->apu.tone(1, 70.f + arm_ * 50.f, 0.03f);
    else sys_->apu.tone(1, 0.f, 0.f);
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.13f) {
            static const float good[] = {294.f, 370.f, 440.f, 587.f};
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
        sys_->apu.tone(0, 74.f, 0.014f);
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        arm_ = 0.18f + 0.06f * std::sin(t_ * 1.3f);
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
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 3, 2) : gs::rgb4(9, 8, 5));
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(int(6.f + u * 6.f), 0, 15), std::clamp(int(4.f + u * 4.f), 0, 15),
                                         std::clamp(int(3.f + u * 2.f), 0, 15));
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
        rd.style = 0;
        rd.band = (int(std::floor(wz * 0.16f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        int f = int(std::clamp((0.2f - t) / 0.2f, 0.f, 1.f) * 9.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 4.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(3, 4, 2);
    }
}

void Game::drawGate(float shx) {
    float z = kBarZ;
    int fog = fogFor(z);
    Spot left = project(bend(z) - kPost, z);
    Spot right = project(bend(z) + kPost, z);
    if (!left.ok || !right.ok) return;
    float sh = std::clamp(5.6f * left.ppm, 28.f, 160.f);
    spr(art_.stake, left.x + shx, left.y, sh, PAL_WOOD, false, fog, true);
    spr(art_.stake, right.x + shx, right.y, sh, PAL_WOOD, true, fog, true);
    float sentryH = std::clamp(1.6f * right.ppm, 10.f, 48.f);
    spr(art_.sentry, right.x + shx + right.ppm * 0.35f, right.y - sh * 0.42f, sentryH, PAL_SENTRY, false, fog, true);

    float reach = (kPost * 2.f - 0.6f) * arm_;
    Spot tip = project(bend(z) + kPost - reach, z);
    if (tip.ok && arm_ > 0.03f) {
        float span = right.x - tip.x;
        int n = std::clamp(int(std::fabs(span) / 12.f) + 1, 1, 16);
        float thick = std::max(4.f, right.ppm * 0.28f);
        float y = right.y - sh * 0.55f;
        for (int i = 0; i < n; ++i) {
            float u = (float(i) + 0.5f) / float(n);
            sprBox(art_.boom, tip.x + span * u + shx, y, std::max(6.f, std::fabs(span) / float(n) + 2.f), thick,
                   (i & 1) ? PAL_BOOM : PAL_WOOD, fog);
        }
        spr(art_.tip, tip.x + shx, y, thick * 1.5f, PAL_ALERT, false, fog, false);
    }
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot p = project(bend(pr.z) + pr.lat, pr.z);
    if (!p.ok) return;
    spr(art_.tree, p.x + shx, p.y, std::clamp(pr.h * p.ppm, 8.f, 130.f), PAL_TREE, false, fogFor(pr.z), true);
}

void Game::drawRig(const Rig& r, float shx) {
    Spot p = project(bend(r.z) + r.lat, r.z);
    if (!p.ok) return;
    float h = std::clamp((r.column ? 2.5f : 1.35f) * p.ppm, 6.f, 110.f);
    spr(r.column ? art_.wagon : art_.cart, p.x + shx, p.y, h, r.column ? PAL_WAGON : PAL_CART, false, fogFor(r.z), true);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.7f) * 3.f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) {
        text("PALISADE COLUMN", 160.f, 24.f, 0.78f, PAL_AMBER);
        text("STOP THE COLUMN", 160.f, 50.f, 0.5f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("ON THE ROAD", 160.f, 26.f, 1.0f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 160.f, 26.f, std::char_traits<char>::length(reason_) > 16 ? 0.58f : 0.82f, PAL_ALERT);
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
    items.push_back({kBarZ, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) drawGate(shx);
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
        hudC(25, hint(), across() ? PAL_GOOD : PAL_AMBER);
        hud(1, 26, "A DROP  B LIFT", PAL_TEXT);
    } else if (mode_ == Mode::Title) {
        hudC(25, "START", PAL_AMBER);
        hudC(26, "ONE PALISADE", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(25, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, reason_, PAL_ALERT);
    }
}

}  // namespace pcol
