#include "game/quarry.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace qcol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 62.f;
constexpr float kSpan = 152.f;
constexpr float kZNear = 7.4f;
constexpr float kPpm = 26.f;
constexpr float kRoadHalf = 4.4f;
constexpr float kLine = 13.8f;
constexpr float kGap = 7.4f;
constexpr float kSpawn = 9.2f;
constexpr float kLeadZ = 76.f;
constexpr float kCruise = 5.4f;
constexpr float kWindowFar = 40.f;
constexpr float kWindowNear = 17.2f;
constexpr float kWatch = 24.f;
constexpr float kTruckH = 3.15f;
constexpr float kLane[3] = {-2.15f, 0.f, 2.15f};

float bend(float z) {
    float u = std::max(0.f, z - 14.f);
    return std::sin(u * 0.028f) * 1.55f;
}

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
    float fade = std::clamp((0.18f - t) / 0.18f, 0.f, 1.f);
    return int(fade * 12.f);
}

float Game::leadZ() const {
    for (const Truck& r : trucks_)
        if (r.slot == 0) return r.z;
    return 999.f;
}

int Game::leadLane() const { return 1; }

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
    const float left = -(kRoadHalf + 2.8f);
    const float right = kRoadHalf + 2.6f;
    for (int i = 0; i < 6; ++i) {
        add(18.f + float(i) * 14.f, left - 0.4f, 7.2f + float(i % 3) * 0.6f, 0);
        add(22.f + float(i) * 13.f, right + 0.5f, 6.4f + float((i + 1) % 3) * 0.7f, 0);
    }
    add(16.5f, left + 0.2f, 4.2f, 1);
    add(19.f, left - 1.2f, 2.6f, 2);
    add(28.f, right + 0.3f, 1.5f, 3);
    add(44.f, left - 0.2f, 1.4f, 3);
    add(58.f, right, 1.7f, 3);
    add(72.f, left, 1.3f, 3);
}

void Game::lay(bool scenic) {
    trucks_.clear();
    puffs_.clear();
    for (int i = 0; i < kColumn; ++i) {
        Truck r;
        r.slot = i;
        r.z = (scenic ? 48.f : kLeadZ) + float(i) * kSpawn;
        r.lat = kLane[leadLane()];
        r.speed = scenic ? 0.f : kCruise;
        trucks_.push_back(r);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    locked_ = false;
    burned_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    lane_ = 0;
    aim_ = 0;
    plant_ = 0.35f;
    hold_ = 0;
    settle_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    stopZ_ = 0;
    reason_ = "THE WATCH IS OVER";
    lay(true);
}

void Game::begin() {
    lay(false);
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    locked_ = false;
    burned_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    lane_ = 0;
    aim_ = 0;
    plant_ = 0;
    hold_ = 0;
    settle_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    stopZ_ = 0;
    puffT_ = 0;
    reason_ = "THE WATCH IS OVER";
    blip(520.f);
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
    hold_ = 1.5f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.18f, 240.f, 0.22f);
    sys_->rumble(0.45f, 0.15f, 160);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.3f;
    fanfare(true);
    sys_->rumble(0.2f, 0.45f, 140);
}

const char* Game::hint() const {
    if (burned_) return "FALSE HALT";
    if (locked_) return "HOLD THE HALT";
    float z = leadZ();
    if (z > kWindowFar) return "WAIT FOR THE CUT";
    if (aim_ != leadLane()) return "STEP INTO THE LANE";
    if (plant_ < 0.72f) return "PLANT THE BOARD";
    return "HOLD THE HALT";
}

void Game::update() {
    const gs::Pad& pad = sys_->pad;
    int want = aim_;
    bool wantPlant = plant_ > 0.5f;
    if (bot_) {
        want = leadLane();
        float z = leadZ();
        wantPlant = !locked_ && !burned_ && z <= kWindowFar && z > kWindowNear + 1.2f && std::fabs(lane_ - float(want)) < 0.2f;
        if (locked_) wantPlant = true;
    } else {
        if (pad.pressed(gs::BTN_LEFT)) want = std::max(0, aim_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) want = std::min(2, aim_ + 1);
        wantPlant = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A);
    }
    if (want != aim_) {
        aim_ = want;
        blip(340.f);
    }
    float goal = float(aim_);
    lane_ += std::clamp(goal - lane_, -6.f * kDt, 6.f * kDt);
    float plantGoal = wantPlant ? 1.f : 0.f;
    plant_ += std::clamp(plantGoal - plant_, -3.2f * kDt, 3.2f * kDt);
    bool planted = plant_ >= 0.72f && std::fabs(lane_ - float(aim_)) < 0.18f;

    float zLead = leadZ();
    if (!locked_ && planted && zLead > kWindowFar + 0.4f) burned_ = true;
    if (burned_ && !planted && zLead > kWindowFar) burned_ = false;

    if (!locked_ && !burned_ && planted && aim_ == leadLane() && zLead <= kWindowFar && zLead > kWindowNear) {
        locked_ = true;
        stopZ_ = std::max(zLead, kLine + 3.2f);
        blip(680.f);
        sys_->rumble(0.2f, 0.35f, 90);
    }

    int halted = 0;
    puffT_ += kDt;
    bool puff = puffT_ > 0.11f;
    if (puff) puffT_ = 0;
    for (Truck& r : trucks_) {
        if (r.passed) continue;
        float haltZ = locked_ ? stopZ_ + float(r.slot) * kGap : -1.f;
        if (locked_ && r.z <= haltZ + 0.08f) {
            r.z = haltZ;
            r.speed = 0;
            r.stopped = true;
            r.lat = kLane[leadLane()];
        } else {
            r.stopped = false;
            float cap = kCruise;
            if (locked_) cap = std::max(0.55f, (r.z - haltZ) * 1.15f);
            r.speed += std::clamp(cap - r.speed, -7.f * kDt, 4.f * kDt);
            r.z -= r.speed * kDt;
            r.lat = kLane[leadLane()];
            if (r.z < kLine) {
                r.passed = true;
                r.speed = 0;
                ++through_;
                lose("THE COLUMN PASSED");
            }
        }
        if (r.stopped) ++halted;
        if (puff && r.speed > 0.8f) {
            Puff f;
            f.z = r.z + 1.1f;
            f.lat = r.lat + ((r.slot & 1) ? 0.7f : -0.7f);
            f.life = 0.4f;
            puffs_.push_back(f);
        }
    }
    stopped_ = halted;

    if (mode_ == Mode::Play && !locked_ && zLead <= kWindowNear) lose("THE COLUMN PASSED");
    if (mode_ == Mode::Play && burned_ && zLead <= kWindowFar) lose("FALSE HALT");
    if (mode_ == Mode::Play && locked_ && stopped_ == kColumn) {
        settle_ += kDt;
        if (settle_ > 0.7f && planted) win();
        else if (settle_ > 0.7f && !planted) {
            locked_ = false;
            settle_ = 0;
            lose("THE COLUMN ROLLED");
        }
    } else {
        settle_ = 0;
    }
    if (mode_ == Mode::Play && t_ >= kWatch) lose("THE WATCH IS OVER");
    t_ += kDt;
}

void Game::tickPuffs() {
    for (auto it = puffs_.begin(); it != puffs_.end();) {
        it->age += kDt;
        if (it->age >= it->life) it = puffs_.erase(it);
        else ++it;
    }
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
            static const float bad[] = {196.f, 164.f, 130.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    }
    bool rolling = false;
    if (mode_ == Mode::Play) {
        for (const Truck& r : trucks_)
            if (!r.passed && r.speed > 0.5f) rolling = true;
    }
    if (rolling) sys_->apu.tone(2, 42.f, 0.03f);
    else sys_->apu.tone(2, 0.f, 0.f);
    if (plant_ > 0.2f && plant_ < 0.95f && mode_ == Mode::Play) sys_->apu.tone(1, 90.f, 0.03f);
    else sys_->apu.tone(1, 0.f, 0.f);
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
    uint16_t sky = mode_ == Mode::Fail ? gs::rgb4(8, 3, 2) : gs::rgb4(11, 7, 4);
    v.setFogColor(sky);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            int r = int(6.f + u * 6.f);
            int g = int(4.f + u * 3.f);
            int b = int(3.f + u * 1.f);
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
        rd.v = wz * 22.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(wz * 0.16f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.15f - t) / 0.15f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11.f);
        v.lineBackdrop[y] = sky;
    }
}

void Game::drawBoard(float shx) {
    float lat = kLane[std::clamp(aim_, 0, 2)];
    float standZ = 11.2f;
    Spot feet = project(bend(standZ) + lat, standZ);
    if (!feet.ok) return;
    int fog = fogFor(standZ);
    float h = std::clamp(2.15f * feet.ppm, 18.f, 120.f);
    float x = feet.x + shx;
    spr(art_.shadow, x, feet.y, h * 0.22f, PAL_DUST, false, 0, false);
    spr(art_.marshal, x, feet.y, h, PAL_COAT, aim_ > 1, fog, true);
    if (plant_ > 0.08f) {
        float bh = h * (0.35f + 0.45f * plant_);
        spr(art_.board, x + h * 0.35f, feet.y - h * 0.55f, bh, plant_ > 0.72f ? PAL_ALERT : PAL_BOARD, false, fog,
            false);
    }
    Spot line = project(bend(kLine), kLine);
    if (!line.ok) return;
    float lineW = (kRoadHalf * 2.f - 0.6f) * line.ppm;
    sprBox(art_.block, line.x + shx, line.y, lineW, std::max(2.f, line.ppm * 0.12f), PAL_AMBER, fogFor(kLine));
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot s = project(bend(pr.z) + pr.lat, pr.z);
    if (!s.ok) return;
    int fog = fogFor(pr.z);
    float h = std::clamp(pr.h * s.ppm, 6.f, 160.f);
    float x = s.x + shx;
    const gs::Mipped* m = &art_.cliff;
    int pal = PAL_ROCK;
    if (pr.kind == 1) {
        m = &art_.crusher;
        pal = PAL_CRUSH;
    } else if (pr.kind == 2) {
        m = &art_.hopper;
        pal = PAL_CRUSH;
    } else if (pr.kind == 3) {
        m = &art_.boulder;
        pal = PAL_ROCK;
    }
    spr(*m, x, s.y, h, pal, pr.lat > 0, fog, true);
}

void Game::drawTruck(const Truck& r, float shx) {
    if (r.z < kZNear + 0.2f) return;
    Spot s = project(bend(r.z) + r.lat, r.z);
    if (!s.ok) return;
    int fog = fogFor(r.z);
    float h = std::clamp(kTruckH * s.ppm, 4.f, 130.f);
    float x = s.x + shx;
    spr(art_.shadow, x, s.y, h * 0.22f, PAL_DUST, false, 0, false);
    spr(art_.dumper, x, s.y, h, PAL_DUMP, false, fog, true);
    if (r.slot == 0) {
        float bob = std::sin(t_ * 4.f) * 1.2f;
        spr(art_.flag, x, s.y - h + bob, h * 0.32f, PAL_ALERT, false, fog, true);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 64.f) * 3.6f * std::min(shake_, 1.f);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.8f);
    layRoad(shx);

    if (mode_ == Mode::Title) text("QUARRY COLUMN", 160.f + shx, 26.f, 1.f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 28.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text("WATCH OVER", 160.f + shx, 28.f, 1.05f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 28.f, 1.1f, PAL_AMBER);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < int(trucks_.size()); ++i) items.push_back({trucks_[size_t(i)].z, 0, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 2, i});
    items.push_back({11.2f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) drawBoard(shx);
        else if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot s = project(bend(f.z) + f.lat, f.z);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(s.ppm * (0.9f + u * 1.4f), 4.f, 26.f);
            spr(art_.dust, s.x + shx, s.y - u * 6.f, h, PAL_DUST, false, fogFor(f.z), false);
        } else if (it.kind == 2) drawProp(props_[size_t(it.id)], shx);
        else drawTruck(trucks_[size_t(it.id)], shx);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "STOP THE COLUMN ON THE ROAD", PAL_AMBER);
        hudC(23, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(24, "LEFT RIGHT  STEP THE CUT", PAL_TEXT);
        hudC(25, "DOWN PLANTS THE BOARD", PAL_GOOD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(24, "THE WATCH HOLDS", PAL_AMBER);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "THE WATCH IS OVER", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else {
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(1, 0, buf, PAL_TEXT);
        int left = std::max(0, int(std::ceil(kWatch - t_)));
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(30, 0, buf, left <= 6 ? PAL_ALERT : PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "PLANT THE BOARD") || !std::strcmp(h, "HOLD THE HALT")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "STEP INTO THE LANE") || !std::strcmp(h, "WAIT FOR THE CUT")) pal = PAL_AMBER;
        else pal = PAL_ALERT;
        hudC(1, h, pal);
        const char* laneName = aim_ == 0 ? "LEFT BENCH" : aim_ == 2 ? "RIGHT BENCH" : "HAUL LANE";
        hud(1, 2, laneName, aim_ == leadLane() ? PAL_GOOD : PAL_AMBER);
        hud(30, 2, plant_ >= 0.72f ? "BOARD" : "OPEN", plant_ >= 0.72f ? PAL_ALERT : PAL_TEXT);
        hudC(27, "LEFT RIGHT    DOWN BOARD", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        plant_ = 0.25f + 0.2f * std::sin(t_ * 1.4f);
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 150, 60);
    else if (mode_ == Mode::Fail) sys.setLight(180, 30, 24);
    else if (mode_ == Mode::Play && plant_ >= 0.72f) sys.setLight(170, 40, 28);
    else sys.setLight(150, 100, 40);
}

}  // namespace qcol
