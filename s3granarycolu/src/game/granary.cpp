#include "game/granary.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace granary {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 58.f;
constexpr float kSpan = 154.f;
constexpr float kZNear = 7.2f;
constexpr float kPpm = 24.f;
constexpr float kRoadHalf = 5.0f;
constexpr float kLine = 16.0f;
constexpr float kPanic = 28.f;
constexpr float kGap = 4.8f;
constexpr float kSpace = 9.0f;
constexpr float kBrakeDist = 14.f;
constexpr float kSwing = 4.1f;
constexpr float kYard = -8.4f;
constexpr float kAcross = 0.15f;
constexpr float kReach = 1.7f;
constexpr float kLeadZ = 118.f;
constexpr float kWagonV = 5.8f;
constexpr float kLocalLat = 3.2f;

float bend(float z) {
    float u = std::max(0.f, z - 22.f);
    return std::sin(u * 0.021f) * u * 0.032f;
}

float tallOf(int kind) {
    if (kind == 0) return 1.7f;
    if (kind == 1) return 1.55f;
    return 2.6f;
}

float parkLat(int slot) {
    if (slot <= 0) return 0.f;
    return (slot & 1) ? -0.7f : 0.7f;
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
    float f = std::clamp((0.24f - t) / 0.24f, 0.f, 1.f);
    return int(f * 11.f);
}

bool Game::blocks(float lat) const { return std::fabs(chute_ - lat) <= kReach && chute_ > kYard + 2.6f; }

bool Game::localsClear() const {
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
    Prop barn;
    barn.z = kLine + 0.4f;
    barn.lat = kYard - 1.1f;
    barn.kind = 0;
    barn.h = 7.2f;
    props_.push_back(barn);
    Prop silo;
    silo.z = kLine + 6.5f;
    silo.lat = kYard - 0.2f;
    silo.kind = 1;
    silo.h = 8.4f;
    props_.push_back(silo);
    for (int i = 0; i < 8; ++i) {
        Prop wheat;
        wheat.z = 20.f + float(i) * 10.5f;
        wheat.lat = kRoadHalf + 2.1f + float(i % 3) * 0.35f;
        wheat.kind = 2;
        wheat.h = 2.8f;
        props_.push_back(wheat);
        Prop left;
        left.z = 24.f + float(i) * 9.5f;
        left.lat = -kRoadHalf - 2.6f;
        left.kind = 2;
        left.h = 2.4f;
        props_.push_back(left);
    }
}

void Game::lay(bool scenic) {
    rigs_.clear();
    puffs_.clear();
    auto add = [&](int kind, bool column, int slot, float z, float cruise, float lat) {
        Rig r;
        r.kind = kind;
        r.column = column;
        r.slot = slot;
        r.z = z;
        r.lat = lat;
        r.cruise = cruise;
        r.speed = cruise;
        r.haltZ = kLine + kGap + (column ? float(slot) * kSpace : 0.f);
        r.side = (slot & 1) ? 1.f : -1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(0, false, 0, 30.f, 11.5f, kLocalLat);
        add(1, false, 1, 46.f, 13.f, -kLocalLat * 0.4f);
        for (int i = 0; i < kColumn; ++i) add(2, true, i, 64.f + float(i) * kSpace, kWagonV, parkLat(i) * 0.2f);
        return;
    }
    add(0, false, 0, 38.f, 12.2f, kLocalLat);
    add(1, false, 1, 54.f, 14.0f, -kLocalLat * 0.35f);
    for (int i = 0; i < kColumn; ++i) add(2, true, i, kLeadZ + float(i) * kSpace, kWagonV, parkLat(i) * 0.12f);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    chute_ = kYard + 1.2f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE GRANARY IS OPEN";
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
    chute_ = kYard;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE GRANARY IS OPEN";
    blip(240.f);
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
    hold_ = 2.0f;
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
    hold_ = 1.6f;
    fanfare(true);
    sys_->rumble(0.2f, 0.45f, 140);
}

float Game::botDir() const {
    if (!localsClear()) return chute_ > kYard + 0.08f ? -1.f : 0.f;
    if (chute_ < kAcross - 0.1f) return 1.f;
    if (chute_ > kAcross + 0.25f) return -1.f;
    return 0.f;
}

const char* Game::hint() const {
    if (!localsClear()) return "LET THE LOCALS PASS";
    bool out = blocks(0.f);
    if (stopped_ >= kColumn && out) return "HOLD THE CHUTE";
    if (!out && leadZ() < kPanic + 10.f) return "TOO CLOSE";
    if (!out) return "SWING THE CHUTE";
    return "HOLD THE CHUTE";
}

void Game::stepRig(Rig& r) {
    bool hit = blocks(r.lat);
    if (r.passed) {
        r.z -= r.cruise * kDt;
        r.lat = r.column ? parkLat(r.slot) : (r.kind == 1 ? -kLocalLat * 0.35f : kLocalLat);
        return;
    }
    if (r.spooked) {
        r.lat += r.side * 5.6f * kDt;
        r.z -= std::max(r.speed, 2.f) * kDt;
        if (std::fabs(r.lat) > kRoadHalf - 0.15f) lose("OFF THE ROAD");
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
    if (r.stopped && hit) {
        r.z = r.haltZ;
        r.lat = r.column ? parkLat(r.slot) : r.lat;
        r.speed = 0.f;
        return;
    }
    if (r.stopped && !hit) {
        r.stopped = false;
        r.orderly = false;
        r.speed = std::max(r.speed, 0.55f);
    }
    if (!hit) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 3.8f * kDt);
        r.z -= r.speed * kDt;
        float lane = r.column ? parkLat(r.slot) : (r.kind == 1 ? -kLocalLat * 0.35f : kLocalLat);
        r.lat += (lane - r.lat) * std::min(1.f, 2.6f * kDt);
        if (r.z < kLine) {
            r.passed = true;
            if (r.column) {
                ++through_;
                lose("THE COLUMN PASSED");
            }
        }
        return;
    }
    if (!r.column) {
        lose("ANYTHING ELSE");
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.0f) {
            r.spooked = true;
            r.side = (r.slot & 1) ? 1.f : -1.f;
            return;
        }
        r.orderly = true;
    }
    float dist = r.z - r.haltZ;
    float park = parkLat(r.slot);
    if (dist <= 0.2f || (dist < 1.1f && r.speed < 0.28f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
        if (std::fabs(r.lat) > kRoadHalf * 0.75f) lose("OFF THE ROAD");
        return;
    }
    if (dist > kBrakeDist) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 2.2f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.3f));
        a = std::min(a, 24.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 3.5f * kDt);
        if (r.speed > 0.45f && r.puff <= 0.f && puffs_.size() < 16) {
            r.puff = 0.14f;
            puffs_.push_back({r.z, r.lat, 0.f, 0.45f});
        }
    }
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
        bool out = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.axisX > 0.35f;
        bool in = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.axisX < -0.35f;
        if (out && !in) dir = 1.f;
        else if (in && !out) dir = -1.f;
    }
    float before = chute_;
    chute_ = std::clamp(chute_ + dir * kSwing * kDt, kYard, kRoadHalf - 0.55f);
    roll_ = std::fabs(chute_ - before) > 0.0001f ? 1.f : 0.f;
    bool was = blocks(0.f);
    bool now = blocks(0.f);
    if (was != now) blip(now ? 160.f : 380.f);

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
        if (r.stopped && !r.spooked && r.z >= kLine - 0.05f && blocks(r.lat)) ++held;
    }
    stopped_ = held;
    if (clear && blocks(0.f) && held == kColumn && through_ == 0) {
        settle_ += kDt;
        if (settle_ >= 0.5f) win();
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
    if (roll_ > 0.f && (mode_ == Mode::Play || mode_ == Mode::Title))
        sys_->apu.tone(1, 62.f + (chute_ - kYard) * 6.f, 0.028f);
    else
        sys_->apu.tone(1, 0.f, 0.f);
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.14f) {
            static const float good[] = {330.f, 440.f, 554.f, 659.f};
            static const float bad[] = {196.f, 147.f, 110.f};
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

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 3, 2) : gs::rgb4(12, 10, 5));
    v.roadTime = int(sys_->frame);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(int(6.f + u * 6.f), 0, 15), std::clamp(int(8.f + u * 4.f), 0, 15),
                                         std::clamp(int(12.f - u * 4.f), 0, 15));
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
        rd.v = wz * 26.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_RUTS;
        rd.band = (int(std::floor(wz * 0.18f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.18f - t) / 0.18f, 0.f, 1.f);
        int f = int(fogT * 9.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 4.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(5, 6, 2);
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
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
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw;
    }
}

void Game::drawGate(float shx) {
    Spot mouth = project(bend(kLine) + kYard, kLine);
    Spot tip = project(bend(kLine) + chute_, kLine);
    if (!mouth.ok) return;
    int fog = fogFor(kLine);
    float ch = std::clamp(1.15f * mouth.ppm, 10.f, 42.f);
    float span = tip.ok ? tip.x - mouth.x : mouth.ppm * 2.f;
    int n = std::clamp(int(std::fabs(span) / 18.f) + 1, 1, 7);
    for (int i = 0; i < n; ++i) {
        float u = (float(i) + 0.5f) / float(n);
        float x = mouth.x + span * u;
        spr(art_.sack, x + shx, mouth.y, ch, PAL_SACK, (i & 1) != 0, fog, true, false);
    }
    if (tip.ok) spr(art_.chute, tip.x + shx, tip.y - ch * 0.15f, ch * 0.45f, PAL_CHUTE, chute_ > kYard + 1.f, fog, true, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.6f) * 3.f * std::min(shake_, 1.f);
    road(shx);

    if (mode_ == Mode::Title) text("GRANARY COLUMN", 160.f + shx, 26.f, 0.95f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 30.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text(reason_, 160.f + shx, 30.f, std::strlen(reason_) > 16 ? 0.68f : 0.9f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 30.f, 1.1f, PAL_GOLD);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < (int)rigs_.size(); ++i) items.push_back({rigs_[i].z, 0, i});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({puffs_[i].z, 1, i});
    for (int i = 0; i < (int)props_.size(); ++i) items.push_back({props_[i].z, 2, i});
    items.push_back({kLine - 0.15f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 3) {
            drawGate(shx);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[it.id];
            Spot p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, p.x + shx, p.y - u * 6.f, std::clamp(p.ppm * (1.1f + u), 4.f, 24.f), PAL_FX, false, fogFor(f.z),
                false, false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[it.id];
            Spot p = project(bend(pr.z) + pr.lat, pr.z);
            if (!p.ok) continue;
            float h = std::clamp(pr.h * p.ppm, 4.f, 130.f);
            int fog = fogFor(pr.z);
            if (pr.kind == 0) spr(art_.barn, p.x + shx, p.y, h, PAL_BARN, false, fog, true, false);
            else if (pr.kind == 1) spr(art_.silo, p.x + shx, p.y, h, PAL_SILO, false, fog, true, false);
            else spr(art_.wheat, p.x + shx, p.y, h, PAL_WHEAT, (it.id & 1) != 0, fog, true, false);
            continue;
        }
        const Rig& r = rigs_[it.id];
        if (r.z < kZNear + 0.2f) continue;
        Spot p = project(bend(r.z) + r.lat, r.z);
        if (!p.ok) continue;
        const gs::Mipped* body = &art_.wagon;
        int pal = PAL_WAGON;
        if (r.kind == 0) {
            body = &art_.cart;
            pal = PAL_CART;
        } else if (r.kind == 1) {
            body = &art_.bike;
            pal = PAL_BIKE;
        }
        float h = std::clamp(tallOf(r.kind) * p.ppm, 3.f, 96.f);
        spr(art_.shadow, p.x + shx, p.y, h * 0.28f, PAL_FX, false, 0, false, true);
        spr(*body, p.x + shx, p.y, h, pal, r.kind == 1 && r.lat < 0.f, fogFor(r.z), true, false);
    }

    bool out = blocks(0.f);
    if (mode_ == Mode::Title) {
        hudC(23, "YOU HAVE THE GRANARY", PAL_TEXT);
        hudC(24, "STOP THE COLUMN ON THE ROAD", PAL_GOLD);
        hudC(25, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(26, "RIGHT SWINGS THE CHUTE OUT", PAL_TEXT);
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
        hud(1, 0, out ? "CHUTE OUT" : "IN THE YARD", out ? PAL_ALERT : PAL_GOOD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(27, 0, buf, PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "LET THE LOCALS PASS")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "SWING THE CHUTE")) pal = PAL_GOLD;
        else if (!std::strcmp(h, "TOO CLOSE")) pal = PAL_ALERT;
        hudC(1, h, pal);
        hudC(27, "RIGHT OUT   LEFT IN", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        chute_ = kYard + 1.6f + 1.1f * std::sin(t_ * 0.7f);
        roll_ = 0.35f;
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            roll_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        roll_ = 0.f;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        roll_ = 0.f;
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(50, 160, 40);
    else if (mode_ == Mode::Fail) sys.setLight(190, 40, 24);
    else if (mode_ == Mode::Play && blocks(0.f)) sys.setLight(170, 90, 20);
    else if (mode_ == Mode::Play) sys.setLight(40, 130, 40);
    else sys.setLight(160, 120, 40);
}

}  // namespace granary
