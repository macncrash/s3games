#include "game/column.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rcol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 58.f;
constexpr float kZScale = 280.f;
constexpr float kZLine = 2.7f;
constexpr float kOnRoad = 0.64f;
constexpr float kMove = 1.15f;
constexpr float kSoon = 25.f;
constexpr float kWarn = 11.f;
constexpr float kPanic = 8.4f;
constexpr float kBotHi = 18.f;
constexpr float kBotLo = 12.f;
constexpr float kGap = 3.8f;
constexpr float kSpace = 7.6f;
constexpr float kBrakeDist = 12.f;
constexpr float kScoutZ = 20.f;
constexpr float kScoutV = 5.8f;
constexpr float kLeadZ = 46.f;
constexpr float kTruckV = 3.25f;
constexpr float kCrest = 34.f;

float parkLat(int slot) {
    static const float kPark[] = {0.f, -0.16f, 0.15f, -0.06f};
    return kPark[slot & 3];
}

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

float Game::bendAt(float row) const { return std::sin(row * 0.016f + 0.55f) * (5.f + row * 0.03f); }

float Game::halfAt(float row) const { return 26.f + row * 0.56f; }

int Game::horizon() const { return std::clamp(int(std::lround(kHorizon + shy_)), 40, 90); }

bool Game::onRoad() const { return std::fabs(u_) <= kOnRoad; }

bool Game::scoutPassed() const {
    for (const Rig& r : rigs_)
        if (!r.column && (r.passed || r.z < kZLine)) return true;
    return false;
}

float Game::leadZ() const {
    for (const Rig& r : rigs_)
        if (r.column && r.slot == 0) return r.z;
    return 999.f;
}

Game::Spot Game::spot(float u, float z, float base) const {
    Spot s;
    if (!(z > 0.85f)) return s;
    float row = kZScale / z;
    float feet = kZScale / kZLine;
    float t = std::clamp(row / feet, 0.02f, 1.35f);
    s.h = std::max(6.f, base * std::pow(t, 0.70f));
    float along = std::clamp((z - kZLine) / kCrest, 0.f, 1.f);
    float lift = std::pow(along, 1.22f) * s.h * 0.68f;
    s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
    s.y = float(hor_) + row - lift;
    s.fog = int(std::clamp(along * 8.f, 0.f, 8.f));
    s.ok = true;
    return s;
}

void Game::buildProps() {
    props_.clear();
    auto add = [&](float z, float u, float h, int kind) {
        Prop p;
        p.z = z;
        p.u = u;
        p.h = h;
        p.kind = kind;
        props_.push_back(p);
    };
    add(3.2f, -1.02f, 70.f, 0);
    add(3.2f, 1.02f, 70.f, 0);
    add(5.4f, -1.22f, 48.f, 1);
    add(7.6f, 1.24f, 42.f, 1);
    add(11.2f, -1.28f, 36.f, 1);
    add(14.8f, 1.22f, 34.f, 1);
    add(9.4f, -1.18f, 86.f, 2);
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
        r.speed = scenic ? 0.f : cruise;
        r.haltZ = kZLine + kGap + (column ? float(slot) * kSpace : 0.f);
        r.side = (slot & 1) ? -1.f : 1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(false, 0, 11.f, kScoutV, 0.42f);
        for (int i = 0; i < kColumn; ++i) add(true, i, 18.f + float(i) * kSpace, kTruckV, parkLat(i));
        return;
    }
    add(false, 0, kScoutZ, kScoutV, 0.42f);
    for (int i = 0; i < kColumn; ++i) add(true, i, kLeadZ + float(i) * kSpace, kTruckV, 0.f);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    committed_ = false;
    chain_ = false;
    planted_ = false;
    chimed_ = false;
    stopped_ = 0;
    through_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    t_ = 0;
    u_ = 0;
    vu_ = 0;
    fanStep_ = -1;
    reason_ = "THE ROAD IS OPEN";
    lay(true);
}

void Game::begin() {
    lay(false);
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    committed_ = false;
    chain_ = false;
    planted_ = false;
    chimed_ = false;
    stopped_ = 0;
    through_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    t_ = 0;
    u_ = 0;
    vu_ = 0;
    fanStep_ = -1;
    reason_ = "THE ROAD IS OPEN";
    blip(620.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.18f, 0.22f, 0.12f);
    if (bot_) begin();
    else bootTitle();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
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
    sys_->apu.noiseBurst(0.22f, 240.f, 0.22f);
    sys_->rumble(0.55f, 0.2f, 180);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.35f;
    fanfare(true);
    sys_->rumble(0.28f, 0.5f, 160);
}

void Game::leave(bool spook) {
    for (Rig& r : rigs_) {
        if (!r.column || r.passed) continue;
        r.leaving = true;
        r.spook = spook;
        r.orderly = false;
        r.stopped = false;
        r.side = (r.slot & 1) ? -1.f : 1.f;
    }
}

void Game::decide() {
    if (!scoutPassed()) {
        lose("NOT THE COLUMN");
        return;
    }
    float z = leadZ();
    if (z > kSoon) leave(false);
    else if (z <= kPanic) leave(true);
    else {
        committed_ = true;
        for (Rig& r : rigs_)
            if (r.column && !r.passed) r.orderly = true;
        blip(170.f);
        sys_->rumble(0.2f, 0.35f, 80);
    }
}

const char* Game::hint() const {
    if (!onRoad()) return "GET ON THE ROAD";
    if (!scoutPassed()) return "LET THE SCOUT PASS";
    if (committed_ && chain_) return "HOLD THE CHAIN";
    if (committed_) return "BAR THE ROAD";
    float z = leadZ();
    if (z > kSoon) return "LET THEM CLOSE";
    if (z <= kWarn) return "TOO CLOSE";
    return "BAR THE ROAD";
}

void Game::stepRig(Rig& r) {
    if (r.passed) {
        r.z -= std::max(r.cruise, 2.f) * kDt;
        return;
    }
    if (r.leaving) {
        r.lat += r.side * (r.spook ? 2.8f : 2.15f) * kDt;
        r.z -= std::max(r.speed, 2.4f) * kDt;
        if (std::fabs(r.lat) > 1.05f) lose("OFF THE ROAD");
        else if (r.z < kZLine) {
            if (r.column) {
                ++through_;
                lose("THE COLUMN PASSED");
            } else {
                r.passed = true;
            }
        }
        return;
    }
    if (r.stopped && chain_) {
        r.z = r.haltZ;
        r.lat = r.column ? parkLat(r.slot) : r.lat;
        r.speed = 0.f;
        return;
    }
    if (r.stopped && !chain_) {
        r.stopped = false;
        r.orderly = false;
        committed_ = false;
        r.speed = std::max(r.speed, 0.7f);
    }
    if (!chain_ || !r.orderly) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 3.2f * kDt);
        r.z -= r.speed * kDt;
        float home = r.column ? 0.f : 0.42f;
        r.lat += (home - r.lat) * std::min(1.f, 3.f * kDt);
        if (r.z < kZLine) {
            if (r.column) {
                ++through_;
                lose("THE COLUMN PASSED");
            } else {
                r.passed = true;
            }
        }
        return;
    }
    float dist = r.z - r.haltZ;
    float park = parkLat(r.slot);
    if (dist <= 0.2f || (dist < 1.1f && r.speed < 0.3f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(90.f + float(r.slot) * 18.f);
        r.stopped = true;
        return;
    }
    if (dist > kBrakeDist) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 2.2f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.25f));
        a = std::min(a, 28.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 3.5f * kDt);
        if (r.speed > 0.45f && r.puff <= 0.f && puffs_.size() < 16) {
            r.puff = 0.16f;
            Puff p;
            p.z = r.z;
            p.lat = r.lat;
            p.age = 0.f;
            p.life = 0.48f;
            puffs_.push_back(p);
        }
    }
    if (r.z <= r.haltZ || (r.speed < 0.25f && dist < 1.8f)) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(90.f + float(r.slot) * 18.f);
        r.stopped = true;
    } else if (r.z < kZLine) {
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
    bool want = false;
    if (bot_) {
        bool window = scoutPassed() && !committed_ && leadZ() <= kBotHi && leadZ() >= kBotLo;
        want = committed_ || window;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (std::fabs(pad.axisX) > 0.18f) dir = std::clamp(pad.axisX, -1.f, 1.f);
        else dir = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        want = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO) ||
               pad.axisY < -0.45f;
    }
    if (want) dir = 0.f;
    float prev = u_;
    u_ = std::clamp(u_ + dir * kMove * kDt, -0.90f, 0.90f);
    vu_ = (u_ - prev) / kDt;
    planted_ = want;
    bool chain = want && onRoad();
    if (chain && !chain_) decide();
    chain_ = chain;
    if (mode_ != Mode::Play) return;

    if (!chimed_ && scoutPassed() && !committed_ && leadZ() <= kSoon && leadZ() > kPanic) {
        chimed_ = true;
        blip(720.f);
    }

    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        stepRig(r);
        if (r.puff > 0.f) r.puff -= kDt;
    }
    if (mode_ != Mode::Play) return;

    int held = 0;
    for (const Rig& r : rigs_) {
        if (!r.column || r.leaving || !r.stopped) continue;
        if (r.z >= kZLine - 0.05f && std::fabs(r.lat) <= 0.7f) ++held;
    }
    stopped_ = held;
    bool file = chain_ && onRoad() && scoutPassed() && through_ == 0 && held == kColumn;
    if (file) settle_ += kDt;
    else settle_ = 0.f;
    if (settle_ >= 0.28f) win();
    else if (t_ > 24.f) lose("THE COLUMN DID NOT STOP");
}

void Game::tickPuffs() {
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                 puffs_.end());
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    bool moving = false;
    if (mode_ == Mode::Play) {
        for (const Rig& r : rigs_)
            if (!r.passed && r.speed > 0.35f) moving = true;
    }
    if (moving) sys_->apu.tone(2, 50.f, 0.03f);
    else sys_->apu.tone(2, 0.f, 0.f);

    if (fanStep_ < 0) return;
    fanT_ += kDt;
    if (fanT_ < 0.12f) return;
    static const float good[] = {392.f, 523.3f, 659.3f, 784.f};
    static const float bad[] = {196.f, 155.6f, 123.5f};
    const float* notes = fanGood_ ? good : bad;
    int n = fanGood_ ? 4 : 3;
    if (fanStep_ < n) sys_->apu.tone(1, notes[fanStep_], 0.08f);
    else sys_->apu.tone(1, 0.f, 0.f);
    ++fanStep_;
    fanT_ = 0.f;
    if (fanStep_ > n + 3) fanStep_ = -1;
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

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 4, 8);
    uint16_t skyHor = mode_ == Mode::Fail ? gs::rgb4(10, 4, 3) : gs::rgb4(13, 8, 5);
    v.setFogColor(skyHor);
    hor_ = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor_) {
            float t = float(y) / float(std::max(hor_, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, t * t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor_);
        float z = kZScale / std::max(row, 0.5f);
        r.on = true;
        r.cx = 160.f + bendAt(row) + shx_;
        r.hw = halfAt(row);
        r.v = z * 72.f;
        r.pal = uint8_t(PAL_FIELD);
        r.style = gs::ROAD_ROCKY;
        r.band = (int(std::floor(z * 0.45f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(11.f - row * 0.10f), 0, 10));
        float dropT = std::clamp(row / 110.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(gs::rgb4(3, 2, 2), gs::rgb4(0, 0, 1), dropT);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 71.f) * 4.6f * shake_;
        shy_ = std::cos(t_ * 53.f) * 2.6f * shake_;
    }
    layRoad();

    if (mode_ == Mode::Title) text("ONE RIDGE", 160.f + shx_, 34.f, 1.15f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx_, 36.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail)
        text(reason_, 160.f + shx_, 34.f, std::strlen(reason_) > 16 ? 0.72f : 0.95f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx_, 36.f, 1.1f, PAL_AMBER);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(40);
    for (int i = 0; i < int(rigs_.size()); ++i)
        if (rigs_[size_t(i)].z > 1.15f) items.push_back({rigs_[size_t(i)].z, 0, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 2, i});
    items.push_back({kZLine + 0.12f, 3, 0});
    items.push_back({kZLine - 0.35f, 4, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 4) {
            bool walk = std::fabs(vu_) > 0.15f && !planted_;
            int fr = walk && (int(t_ * 8.f) & 1) ? 1 : 0;
            const gs::Mipped& body = planted_ ? art_.brace : art_.warden[fr];
            Spot s = spot(u_, kZLine, planted_ ? 92.f : 88.f);
            if (!s.ok) continue;
            spr(art_.shadow, s.x, s.y, s.h * 0.42f, PAL_FX, false, 0, false);
            spr(body, s.x, s.y, s.h, PAL_YOU, vu_ < -0.2f, 0, true);
            continue;
        }
        if (it.kind == 3) {
            if (!planted_) continue;
            Spot feet = spot(u_, kZLine, 90.f);
            if (!feet.ok) continue;
            float row = kZScale / kZLine;
            if (chain_) {
                float w = halfAt(row) * 1.72f;
                sprBox(art_.chain, 160.f + bendAt(row) + shx_, feet.y - feet.h * 0.56f, w, 11.f, PAL_CHAIN, 0);
            } else {
                sprBox(art_.chain, feet.x, feet.y - feet.h * 0.5f, 42.f, 10.f, PAL_CHAIN, 0);
            }
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot s = spot(f.lat, f.z, 28.f);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, s.x, s.y - u * 8.f, s.h * (0.7f + u), PAL_FX, false, s.fog, false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[size_t(it.id)];
            Spot s = spot(pr.u, pr.z, pr.h);
            if (!s.ok) continue;
            if (pr.kind == 2) {
                spr(art_.banner, s.x, s.y, s.h, PAL_BANNER, false, s.fog, true);
            } else if (pr.kind == 1) {
                spr(art_.cairn, s.x, s.y, s.h, PAL_STONE, pr.u > 0, s.fog, true);
            } else {
                spr(art_.post, s.x, s.y, s.h, PAL_STONE, pr.u > 0, s.fog, true);
                int lampPal = PAL_GOOD;
                bool blink = (int(t_ * 4.f) & 1) == 0;
                if (mode_ == Mode::Play || mode_ == Mode::Pause) {
                    if (chain_) lampPal = PAL_ALERT;
                    else if (!scoutPassed()) lampPal = PAL_GOOD;
                    else if (committed_ || leadZ() <= kWarn) lampPal = PAL_ALERT;
                    else if (leadZ() <= kSoon) lampPal = blink ? PAL_AMBER : PAL_HUD;
                    else lampPal = PAL_AMBER;
                } else if (mode_ == Mode::Victory) {
                    lampPal = PAL_GOOD;
                } else if (mode_ == Mode::Fail) {
                    lampPal = PAL_ALERT;
                } else {
                    lampPal = chain_ ? PAL_ALERT : PAL_AMBER;
                }
                spr(art_.lamp, s.x, s.y - s.h * 0.92f, std::max(8.f, s.h * 0.18f), lampPal, false, 0, false);
            }
            continue;
        }
        const Rig& r = rigs_[size_t(it.id)];
        Spot s = spot(r.lat, r.z, r.column ? 78.f : 56.f);
        if (!s.ok) continue;
        spr(art_.shadow, s.x, s.y, s.h * 0.34f, PAL_FX, false, s.fog, false);
        if (r.column) {
            spr(art_.truck, s.x, s.y, s.h, PAL_TRUCK, false, s.fog, true);
            if (r.slot == 0) {
                float bob = std::sin(t_ * 5.f + r.z) * 1.4f;
                spr(art_.pennant, s.x + s.h * 0.08f, s.y - s.h * 0.92f + bob, s.h * 0.34f, PAL_BANNER, false, s.fog,
                    false);
            }
        } else {
            spr(art_.scout, s.x, s.y, s.h, PAL_SCOUT, false, s.fog, true);
        }
    }

    spr(art_.peak[0], 48.f + shx_ * 0.25f, float(hor_) + 2.f, 52.f, PAL_MOUNT, false, 2, true);
    spr(art_.peak[1], 268.f + shx_ * 0.25f, float(hor_) + 4.f, 44.f, PAL_MOUNT, false, 3, true);
    float drift = std::fmod(t_ * 6.f, 380.f);
    spr(art_.cloud, drift - 40.f, 20.f, 16.f, PAL_FX, false, 2, false);
    spr(art_.cloud, std::fmod(drift + 190.f, 380.f) - 30.f, 30.f, 12.f, PAL_FX, true, 3, false);
    spr(art_.sun, 236.f, 18.f, 22.f, PAL_FX, false, 0, false);

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(23, "STOP THE COLUMN ON THE ROAD", PAL_AMBER);
        hudC(24, "BAR THE ROAD WHEN THEY CLOSE", PAL_GOOD);
        hudC(25, "ARROWS STEP   DOWN OR C BARS", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(24, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(25, "THEN IT IS DONE", PAL_AMBER);
        hudC(27, "START", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(24, reason_, PAL_ALERT);
        hudC(25, "THE COLUMN IS NOT ON THE ROAD", PAL_HUD);
        hudC(27, "START RETRIES", PAL_HUD);
    } else {
        hud(1, 0, onRoad() ? "ON ROAD" : "SHOULDER", onRoad() ? PAL_GOOD : PAL_AMBER);
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_HUD);
        hud(1, 1, chain_ ? "CHAIN ACROSS" : "CHAIN UP", chain_ ? PAL_ALERT : PAL_HUD);
        const char* h = hint();
        int pal = PAL_HUD;
        if (!std::strcmp(h, "LET THE SCOUT PASS")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "BAR THE ROAD") || !std::strcmp(h, "GET ON THE ROAD")) pal = PAL_AMBER;
        else if (!std::strcmp(h, "TOO CLOSE") || !std::strcmp(h, "HOLD THE CHAIN")) pal = PAL_ALERT;
        hudC(2, h, pal);
        hudC(27, "ARROWS STEP   DOWN OR C BARS", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        t_ += kDt;
        float prev = u_;
        u_ = std::sin(t_ * 0.55f) * 0.20f;
        vu_ = (u_ - prev) / kDt;
        planted_ = std::sin(t_ * 1.25f) > 0.2f;
        chain_ = planted_ && onRoad();
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            vu_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        vu_ = 0.f;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        vu_ *= 0.9f;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }

    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 150, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 36, 28);
    else if (mode_ == Mode::Play && chain_) sys.setLight(170, 48, 28);
    else if (mode_ == Mode::Play) sys.setLight(150, 110, 48);
    else sys.setLight(160, 100, 50);
}

}  // namespace rcol
