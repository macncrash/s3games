#include "game/foundry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace foundry {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 58.f;
constexpr float kSpan = 154.f;
constexpr float kZNear = 7.2f;
constexpr float kPpm = 22.f;
constexpr float kRoadHalf = 5.1f;
constexpr float kLine = 16.2f;
constexpr float kPanic = 26.5f;
constexpr float kGap = 4.4f;
constexpr float kSpace = 8.6f;
constexpr float kBrakeDist = 12.5f;
constexpr float kRoll = 3.5f;
constexpr float kBay = -7.5f;
constexpr float kCover = 0.15f;
constexpr float kReach = 1.5f;
constexpr float kLeadZ = 108.f;
constexpr float kCartV = 5.9f;
constexpr float kDecoyLat = 3.4f;

float bend(float z) {
    float u = std::max(0.f, z - 24.f);
    return std::sin(u * 0.031f) * u * 0.035f;
}

float tallOf(int kind) {
    if (kind == 0) return 2.05f;
    if (kind == 1) return 2.7f;
    return 3.05f;
}

float parkLat(int slot) {
    if (slot <= 0) return 0.f;
    return (slot & 1) ? -0.8f : 0.8f;
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
    return int(f * 12.f);
}

bool Game::covers(float lat) const { return std::fabs(gate_ - lat) <= kReach && gate_ > kBay + 2.2f; }

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
    for (int i = 0; i < 6; ++i) {
        Prop stack;
        stack.z = 20.f + float(i) * 13.f;
        stack.lat = -kRoadHalf - 2.0f;
        stack.kind = 0;
        stack.h = 5.6f;
        props_.push_back(stack);
        Prop pot;
        pot.z = 28.f + float(i) * 14.f;
        pot.lat = kRoadHalf + 2.2f;
        pot.kind = 1;
        pot.h = 3.4f;
        props_.push_back(pot);
        Prop bars;
        bars.z = 34.f + float(i) * 11.f;
        bars.lat = kRoadHalf + 3.6f;
        bars.kind = 2;
        bars.h = 1.6f;
        props_.push_back(bars);
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
        add(0, false, 0, 30.f, 12.f, kDecoyLat);
        add(1, false, 1, 50.f, 10.f, kDecoyLat);
        for (int i = 0; i < kColumn; ++i) add(2, true, i, 66.f + float(i) * kSpace, kCartV, parkLat(i) * 0.2f);
        return;
    }
    add(0, false, 0, 38.f, 12.2f, kDecoyLat);
    add(1, false, 1, 56.f, 10.6f, kDecoyLat);
    for (int i = 0; i < kColumn; ++i) add(2, true, i, kLeadZ + float(i) * kSpace, kCartV, parkLat(i) * 0.12f);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    gate_ = kBay + 0.8f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE FOUNDRY IS COLD";
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
    gate_ = kBay;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE FOUNDRY IS COLD";
    blip(480.f);
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
    sys_->rumble(0.5f, 0.2f, 180);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.6f;
    fanfare(true);
    sys_->rumble(0.25f, 0.5f, 140);
}

float Game::botDir() const {
    if (!decoysClear()) return gate_ > kBay + 0.05f ? -1.f : 0.f;
    if (gate_ < kCover - 0.08f) return 1.f;
    if (gate_ > kCover + 0.18f) return -1.f;
    return 0.f;
}

const char* Game::hint() const {
    if (!decoysClear()) return "LET THE YARD PASS";
    bool out = covers(0.f);
    if (stopped_ >= kColumn && out) return "HOLD THE GATE";
    if (!out && leadZ() < kPanic + 8.f) return "TOO CLOSE";
    if (!out) return "ROLL THE GATE";
    return "HOLD THE GATE";
}

void Game::stepRig(Rig& r) {
    bool hit = covers(r.lat);
    if (r.passed) {
        r.z -= r.cruise * kDt;
        r.lat = r.column ? parkLat(r.slot) : kDecoyLat;
        return;
    }
    if (r.spooked) {
        r.lat += r.side * 6.0f * kDt;
        r.z -= std::max(r.speed, 2.f) * kDt;
        if (std::fabs(r.lat) > kRoadHalf - 0.1f) lose("OFF THE ROAD");
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
        r.speed = std::max(r.speed, 0.6f);
    }
    if (!hit) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 4.0f * kDt);
        r.z -= r.speed * kDt;
        float lane = r.column ? parkLat(r.slot) : kDecoyLat;
        r.lat += (lane - r.lat) * std::min(1.f, 3.f * kDt);
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
        if (r.z <= kPanic && r.speed > 2.2f) {
            r.spooked = true;
            r.side = (r.slot & 1) ? 1.f : -1.f;
            return;
        }
        r.orderly = true;
    }
    float dist = r.z - r.haltZ;
    float park = parkLat(r.slot);
    if (dist <= 0.2f || (dist < 1.1f && r.speed < 0.3f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
        if (std::fabs(r.lat) > kRoadHalf * 0.72f) lose("OFF THE ROAD");
        return;
    }
    if (dist > kBrakeDist) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 2.4f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.25f));
        a = std::min(a, 26.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 4.f * kDt);
        if (r.speed > 0.5f && r.puff <= 0.f && puffs_.size() < 16) {
            r.puff = 0.12f;
            puffs_.push_back({r.z, r.lat, 0.f, 0.45f});
        }
    }
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
        bool out = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.axisX > 0.35f;
        bool in = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.axisX < -0.35f;
        if (out && !in) dir = 1.f;
        else if (in && !out) dir = -1.f;
    }
    float before = gate_;
    gate_ = std::clamp(gate_ + dir * kRoll * kDt, kBay, kRoadHalf - 0.45f);
    roll_ = std::fabs(gate_ - before) > 0.0001f ? 1.f : 0.f;
    bool was = covers(0.f);
    bool now = covers(0.f);
    if (was != now) blip(now ? 160.f : 420.f);

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
        if (r.stopped && !r.spooked && r.z >= kLine - 0.05f && covers(r.lat)) ++held;
    }
    stopped_ = held;
    if (clear && covers(0.f) && held == kColumn && through_ == 0) {
        settle_ += kDt;
        if (settle_ >= 0.5f) win();
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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (roll_ > 0.f && (mode_ == Mode::Play || mode_ == Mode::Title))
        sys_->apu.tone(1, 62.f + (gate_ - kBay) * 9.f, 0.03f);
    else
        sys_->apu.tone(1, 0.f, 0.f);
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.13f) {
            static const float good[] = {330.f, 415.f, 523.f, 659.f};
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
    } else if (covers(0.f)) {
        sys_->apu.tone(0, 55.f, 0.02f);
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 2, 1) : gs::rgb4(7, 3, 1));
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(int(2.f + (1.f - u) * 4.f), 0, 15), std::clamp(int(1.f + u * 2.f), 0, 15),
                                         std::clamp(int(2.f + u), 0, 15));
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
        rd.v = wz * 28.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.18f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.18f - t) / 0.18f, 0.f, 1.f);
        int f = int(fogT * 11.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 5.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(3, 1, 1);
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

void Game::drawBay(float shx) {
    Spot mouth = project(bend(kLine) + kBay, kLine);
    Spot tip = project(bend(kLine) + gate_, kLine);
    if (!mouth.ok) return;
    int fog = fogFor(kLine);
    float ch = std::clamp(1.4f * mouth.ppm, 14.f, 52.f);
    spr(art_.chimney, mouth.x + shx, mouth.y, ch * 1.15f, PAL_CHIMNEY, false, fog, true, false);
    float span = tip.ok ? tip.x - mouth.x : mouth.ppm * 2.f;
    int n = std::clamp(int(std::fabs(span) / 18.f) + 1, 1, 8);
    for (int i = 0; i < n; ++i) {
        float u = (float(i) + 0.5f) / float(n);
        float x = mouth.x + span * u;
        spr(art_.ingot, x + shx, mouth.y, ch * 0.45f, PAL_INGOT, false, fog, true, false);
    }
    if (tip.ok) spr(art_.gate, tip.x + shx, tip.y + 2.f, ch * 0.7f, PAL_GATE, gate_ > kBay + 1.f, fog, true, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.7f) * 3.2f * std::min(shake_, 1.f);
    road(shx);

    if (mode_ == Mode::Title) text("FOUNDRY COLUMN", 160.f + shx, 26.f, 1.0f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 30.f, 1.1f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text(reason_, 160.f + shx, 30.f, std::strlen(reason_) > 16 ? 0.7f : 0.95f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 30.f, 1.15f, PAL_GOLD);

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
            drawBay(shx);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[it.id];
            Spot p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.soot, p.x + shx, p.y - u * 8.f, std::clamp(p.ppm * (1.1f + u), 4.f, 24.f), PAL_FX, false, fogFor(f.z),
                false, false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[it.id];
            Spot p = project(bend(pr.z) + pr.lat, pr.z);
            if (!p.ok) continue;
            float h = std::clamp(pr.h * p.ppm, 4.f, 120.f);
            int fog = fogFor(pr.z);
            if (pr.kind == 0) spr(art_.stack, p.x + shx, p.y, h, PAL_STACK, false, fog, true, false);
            else if (pr.kind == 1) spr(art_.crucible, p.x + shx, p.y, h, PAL_CRUCIBLE, false, fog, true, false);
            else spr(art_.ingot, p.x + shx, p.y, h, PAL_INGOT, false, fog, true, false);
            continue;
        }
        const Rig& r = rigs_[it.id];
        if (r.z < kZNear + 0.2f) continue;
        Spot p = project(bend(r.z) + r.lat, r.z);
        if (!p.ok) continue;
        const gs::Mipped* body = &art_.slag;
        int pal = PAL_SLAG;
        if (r.kind == 0) {
            body = &art_.coke;
            pal = PAL_COKE;
        } else if (r.kind == 1) {
            body = &art_.ladle;
            pal = PAL_LADLE;
        }
        float h = std::clamp(tallOf(r.kind) * p.ppm, 3.f, 96.f);
        spr(art_.shadow, p.x + shx, p.y, h * 0.28f, PAL_FX, false, 0, false, true);
        spr(*body, p.x + shx, p.y, h, pal, false, fogFor(r.z), true, false);
    }

    spr(art_.plume, 70.f + std::sin(t_ * 0.35f) * 8.f + shx, 16.f, 14.f, PAL_FX, false, 0, false, false);
    spr(art_.plume, 230.f + std::sin(t_ * 0.22f) * 6.f + shx, 12.f, 11.f, PAL_GLOW, true, 0, false, false);

    bool out = covers(0.f);
    if (mode_ == Mode::Title) {
        hudC(23, "AT THE FOUNDRY", PAL_TEXT);
        hudC(24, "STOP THE COLUMN ON THE ROAD", PAL_GOLD);
        hudC(25, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(26, "RIGHT ROLLS THE GATE OUT", PAL_TEXT);
        hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(26, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(1, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(27, "START", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(1, reason_, PAL_ALERT);
        hudC(2, "THE WATCH IS OVER", PAL_TEXT);
        hudC(27, "START RETRIES", PAL_TEXT);
    } else {
        hud(1, 0, out ? "GATE OUT" : "IN THE BAY", out ? PAL_ALERT : PAL_GOOD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(28, 0, buf, PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "LET THE YARD PASS")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "ROLL THE GATE")) pal = PAL_GOLD;
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
        gate_ = kBay + 1.4f + 0.6f * std::sin(t_ * 0.7f);
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
    if (won_) sys.setLight(40, 160, 60);
    else if (mode_ == Mode::Fail) sys.setLight(200, 40, 20);
    else if (mode_ == Mode::Play && covers(0.f)) sys.setLight(200, 70, 20);
    else if (mode_ == Mode::Play) sys.setLight(40, 120, 50);
    else sys.setLight(180, 90, 30);
}

}  // namespace foundry
