#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace millc {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 56.f;
constexpr float kSpan = 156.f;
constexpr float kZNear = 6.8f;
constexpr float kPpm = 26.f;
constexpr float kRoadHalf = 4.8f;
constexpr float kMillZ = 19.2f;
constexpr float kLine = 16.4f;
constexpr float kPanic = 31.f;
constexpr float kGap = 3.6f;
constexpr float kSpace = 8.0f;
constexpr float kBrake = 11.5f;
constexpr float kMouth = -6.2f;
constexpr float kWagonV = 5.2f;
constexpr float kLeadZ = 114.f;
constexpr float kCrank = 1.55f;
constexpr float kAcross = 0.78f;

float bend(float z) {
    float u = std::max(0.f, z - 18.f);
    return std::sin(u * 0.018f) * u * 0.028f;
}

float parkLat(int slot) { return (slot & 1) ? -0.55f : 0.62f; }

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

bool Game::across() const { return boom_ >= kAcross; }

bool Game::cartsClear() const {
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
    return int(std::clamp((0.24f - t) / 0.24f, 0.f, 1.f) * 12.f);
}

void Game::buildProps() {
    props_.clear();
    for (int i = 0; i < 7; ++i) {
        Prop tree;
        tree.z = 26.f + float(i) * 13.f;
        tree.lat = (i & 1 ? 1.f : -1.f) * (kRoadHalf + 2.4f + float(i % 3) * 0.35f);
        tree.h = 4.6f + float(i % 2) * 0.8f;
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
        r.haltZ = kMillZ + kGap + (column ? float(slot) * kSpace : 0.f);
        r.side = (slot & 1) ? 1.f : -1.f;
        rigs_.push_back(r);
    };
    if (scenic) {
        add(false, 0, 36.f, 11.5f, 2.2f);
        for (int i = 0; i < kColumn; ++i) add(true, i, 72.f + float(i) * kSpace, kWagonV, parkLat(i) * 0.2f);
        return;
    }
    add(false, 0, 40.f, 12.2f, 2.15f);
    add(false, 1, 56.f, 11.0f, 1.9f);
    for (int i = 0; i < kColumn; ++i) add(true, i, kLeadZ + float(i) * kSpace, kWagonV, parkLat(i) * 0.12f);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    boom_ = 0.18f;
    sail_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE MILL IS STILL";
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
    boom_ = 0.f;
    sail_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE MILL IS STILL";
    blip(392.f);
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
    hold_ = 1.5f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.15f, 640.f, 0.15f);
    sys_->rumble(0.4f, 0.18f, 150);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.3f;
    fanfare(true);
    sys_->rumble(0.18f, 0.4f, 130);
}

float Game::botDir() const {
    if (!cartsClear()) return boom_ > 0.02f ? -1.f : 0.f;
    if (leadZ() > 48.f && boom_ > 0.04f) return -1.f;
    if (boom_ < kAcross + 0.04f) return 1.f;
    return 0.f;
}

const char* Game::hint() const {
    if (!cartsClear()) return "LET THE CARTS PASS";
    if (!across()) return "DROP THE BOOM";
    if (stopped_ >= kColumn) return "HOLD THE MILL";
    return "HOLD THE BOOM";
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
        if (gate && r.z < kMillZ + 7.5f) {
            r.z -= r.speed * kDt;
            if (r.z <= kMillZ + 0.4f) lose("NOT THE COLUMN");
            return;
        }
        r.speed = std::min(r.cruise, r.speed + 2.8f * kDt);
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
        float lane = parkLat(r.slot);
        r.lat += (lane - r.lat) * std::min(1.f, 2.8f * kDt);
        if (r.z < kLine) {
            r.passed = true;
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.2f) {
            r.spooked = true;
            return;
        }
        r.orderly = true;
    }
    float dist = r.z - r.haltZ;
    float park = parkLat(r.slot);
    if (dist <= 0.25f) {
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
            puffs_.push_back({r.z, r.lat, 0.f, 0.42f});
        }
    }
    r.lat += (park - r.lat) * std::min(1.f, 3.2f * kDt);
    if (r.z <= r.haltZ || (r.speed < 0.18f && dist < 1.5f)) {
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
        bool down = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C) ||
                    pad.axisX > 0.35f;
        bool up = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_UP) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) ||
                  pad.axisX < -0.35f;
        if (down && !up) dir = 1.f;
        else if (up && !down) dir = -1.f;
    }
    float before = boom_;
    boom_ = std::clamp(boom_ + dir * kCrank * kDt, 0.f, 1.f);
    sail_ += kDt * (0.7f + std::fabs(dir) * 2.4f + (across() ? 0.4f : 0.f));
    if ((before < kAcross) != (boom_ < kAcross)) blip(boom_ >= kAcross ? 148.f : 330.f);

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
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, 70.f + 18.f * std::sin(sail_ * 2.f), 0.02f);
    else sys_->apu.tone(1, 0.f, 0.f);
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.14f) {
            static const float good[] = {294.f, 370.f, 440.f, 587.f};
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
        sys_->apu.tone(0, 98.f, 0.014f);
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        boom_ = 0.16f + 0.06f * std::sin(t_ * 0.7f);
        sail_ += kDt * 0.55f;
        t_ += kDt;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else {
        sail_ += kDt * (won_ ? 1.1f : 0.15f);
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
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 3, 2) : gs::rgb4(9, 10, 8));
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(int(3.f + u * 5.f), 0, 15), std::clamp(int(4.f + u * 5.f), 0, 15),
                                         std::clamp(int(9.f - u * 3.f), 0, 15));
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
        int f = int(std::clamp((0.2f - t) / 0.2f, 0.f, 1.f) * 10.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + int(shake_ * 4.f));
        v.lineFog[y] = uint8_t(f);
        v.lineBackdrop[y] = gs::rgb4(3, 5, 2);
    }
}

void Game::drawMill(float shx) {
    float z = kMillZ;
    float mouthX = bend(z) + kMouth;
    float tipLat = kMouth + (kRoadHalf * 0.92f - kMouth) * boom_;
    Spot mouth = project(mouthX, z);
    Spot tip = project(bend(z) + tipLat, z);
    if (!mouth.ok) return;
    int fog = fogFor(z);
    float bh = std::clamp(5.4f * mouth.ppm, 40.f, 190.f);
    float mx = mouth.x + shx;
    spr(art_.mill, mx, mouth.y, bh, PAL_MILL, false, fog, true);
    const gs::Mipped& sail = (int(sail_ * 2.f) & 1) ? art_.sailCross : art_.sailPlus;
    float sh = std::clamp(bh * 0.55f, 16.f, 110.f);
    spr(sail, mx, mouth.y - bh * 0.78f, sh, PAL_SAIL, false, fog, false);
    spr(art_.cap, mx, mouth.y - bh * 0.96f, sh * 0.28f, PAL_SAIL, false, fog, false);
    if (tip.ok && boom_ > 0.03f) {
        float span = tip.x - mouth.x;
        int n = std::clamp(int(std::fabs(span) / 16.f) + 1, 1, 12);
        float thick = std::max(4.f, mouth.ppm * 0.2f);
        float y = mouth.y - bh * 0.38f;
        for (int i = 0; i < n; ++i) {
            float u = (float(i) + 0.5f) / float(n);
            sprBox(art_.timber, mouth.x + span * u + shx, y, std::max(6.f, std::fabs(span) / float(n) + 2.f), thick,
                   PAL_TIMBER, fog);
        }
        spr(art_.stripe, tip.x + shx, y, thick * 1.6f, PAL_ALERT, false, fog, false);
    }
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot p = project(bend(pr.z) + pr.lat, pr.z);
    if (!p.ok) return;
    spr(art_.tree, p.x + shx, p.y, std::clamp(pr.h * p.ppm, 8.f, 130.f), PAL_TREE, pr.lat < 0, fogFor(pr.z), true);
}

void Game::drawRig(const Rig& r, float shx) {
    Spot p = project(bend(r.z) + r.lat, r.z);
    if (!p.ok) return;
    float h = std::clamp((r.column ? 2.55f : 1.7f) * p.ppm, 6.f, 120.f);
    spr(r.column ? art_.lorry : art_.cart, p.x + shx, p.y, h, r.column ? PAL_LORRY : PAL_CART, false, fogFor(r.z), true);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.7f) * 3.f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) {
        text("MILL COLUMN", 160.f, 24.f, 0.95f, PAL_WHEAT);
        text("STOP THE COLUMN", 160.f, 50.f, 0.52f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("ON THE ROAD", 160.f, 26.f, 0.95f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 160.f, 26.f, std::char_traits<char>::length(reason_) > 16 ? 0.58f : 0.82f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 26.f, 1.0f, PAL_WHEAT);
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
    items.push_back({kMillZ - 0.2f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) drawMill(shx);
        else if (it.kind == 2) drawProp(props_[it.id], shx);
        else if (it.kind == 0) drawRig(rigs_[it.id], shx);
        else {
            const Puff& f = puffs_[it.id];
            Spot p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, p.x + shx, p.y - u * 5.f, std::clamp(p.ppm * (1.0f + u), 4.f, 20.f), PAL_DUST, false, fogFor(f.z),
                false);
        }
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char line[40];
        std::snprintf(line, sizeof line, "HELD %d/%d", stopped_, kColumn);
        hud(1, 1, line, PAL_TEXT);
        hudC(25, hint(), across() ? PAL_GOOD : PAL_WHEAT);
        hud(1, 26, "A DROP  B LIFT", PAL_TEXT);
    } else if (mode_ == Mode::Title) {
        hudC(25, "START", PAL_WHEAT);
        hudC(26, "ONE MILL", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(25, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, reason_, PAL_ALERT);
    }
}

}  // namespace millc
