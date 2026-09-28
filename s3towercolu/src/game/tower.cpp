#include "game/tower.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace twc {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 72.f;
constexpr float kSpan = 128.f;
constexpr float kZNear = 6.f;
constexpr float kPpm = 19.f;
constexpr float kRoadHalf = 4.6f;
constexpr float kLine = 13.f;
constexpr float kPanic = 26.f;
constexpr float kGap = 6.f;
constexpr float kSpace = 8.f;
constexpr float kBrakeDist = 12.f;
constexpr float kAimCatch = 0.55f;
constexpr float kLeadZ = 98.f;
constexpr float kTruckV = 5.7f;
constexpr float kChainRate = 1.55f;

float bend(float z) {
    float u = std::max(0.f, z - 24.f);
    return std::sin(u * 0.028f) * u * 0.04f;
}

float tallOf(int kind) { return kind == 0 ? 1.7f : 2.7f; }

float parkLat(int slot) {
    if (slot <= 0) return 0.f;
    return (slot & 1) ? -0.85f : 0.85f;
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
    float f = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
    return int(f * 11.f);
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
        add(0, false, 0, 28.f, 0.f);
        add(0, false, 1, 48.f, 0.f);
        for (int i = 0; i < kColumn; ++i) add(1, true, i, 68.f + float(i) * kSpace, 0.f);
        return;
    }
    add(0, false, 0, 34.f, 13.2f);
    add(0, false, 1, 54.f, 11.0f);
    for (int i = 0; i < kColumn; ++i) add(1, true, i, kLeadZ + float(i) * kSpace, kTruckV);
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    shut_ = false;
    dropping_ = false;
    judged_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    aim_ = 0;
    chain_ = 0.55f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE COLUMN IS MOVING";
    props_.clear();
    for (int i = 0; i < 7; ++i) {
        Prop p;
        p.z = 22.f + float(i) * 14.f;
        p.lat = ((i & 1) ? 1.f : -1.f) * (kRoadHalf + 2.4f + float(i % 3) * 0.35f);
        p.h = 5.2f + float(i % 2);
        props_.push_back(p);
    }
    lay(true);
}

void Game::begin() {
    lay(false);
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    shut_ = false;
    dropping_ = false;
    judged_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    aim_ = 0;
    chain_ = 1.f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE COLUMN IS MOVING";
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
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
    sys_->rumble(0.5f, 0.2f, 180);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.5f;
    fanfare(true);
    sys_->rumble(0.25f, 0.5f, 140);
}

void Game::resolveChain() {
    if (judged_ || chain_ > 0.02f) return;
    judged_ = true;
    dropping_ = false;
    chain_ = 0.f;
    if (!decoysClear()) {
        lose("NOT THE COLUMN");
        return;
    }
    if (std::fabs(aim_) > kAimCatch) {
        lose("THE CHAIN MISSED");
        return;
    }
    float lead = leadZ();
    if (lead <= kPanic) {
        lose("TOO LATE");
        return;
    }
    shut_ = true;
    blip(140.f);
    sys_->rumble(0.3f, 0.45f, 90);
}

const char* Game::hint() const {
    if (dropping_) return "CHAIN FALLING";
    if (shut_) return "HOLD THE CHAIN";
    if (!decoysClear()) return "LET THEM PASS";
    if (leadZ() < kPanic + 6.f) return "TOO CLOSE";
    if (std::fabs(aim_) > kAimCatch) return "AIM THE ROAD";
    return "DROP THE CHAIN";
}

void Game::stepRig(Rig& r) {
    if (r.passed || r.cruise <= 0.f) return;
    if (r.spooked) {
        r.lat += r.side * 6.2f * kDt;
        r.z -= std::max(r.speed, 2.f) * kDt;
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
    if (r.stopped && shut_) {
        r.z = r.haltZ;
        r.lat = r.column ? parkLat(r.slot) : 0.f;
        r.speed = 0.f;
        return;
    }
    if (!shut_) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 3.6f * kDt);
        r.z -= r.speed * kDt;
        r.lat = 0.16f * std::sin(t_ * 0.9f + float(r.slot));
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
        if (r.z <= kPanic && r.speed > 2.0f) {
            r.spooked = true;
            return;
        }
        r.orderly = true;
    }
    float dist = r.z - r.haltZ;
    float park = r.column ? parkLat(r.slot) : 0.f;
    if (dist <= 0.2f) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
        if (!r.column) lose("NOT THE COLUMN");
        else if (std::fabs(r.lat) > kRoadHalf * 0.72f) lose("OFF THE ROAD");
        return;
    }
    if (dist > kBrakeDist) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 3.f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.25f));
        a = std::min(a, 28.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 4.f * kDt);
        if (r.speed > 0.4f && r.puff <= 0.f && puffs_.size() < 16) {
            r.puff = 0.12f;
            puffs_.push_back({r.z, r.lat, 0.f, 0.5f});
        }
    }
    if (r.z <= r.haltZ) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        r.stopped = true;
        if (!r.column) lose("NOT THE COLUMN");
    }
}

void Game::update() {
    t_ += kDt;
    if (!shut_ && !judged_) {
        float dir = 0.f;
        if (bot_) {
            aim_ = 0.f;
            if (!dropping_ && decoysClear() && leadZ() < 74.f && leadZ() > 40.f) dropping_ = true;
        } else {
            const gs::Pad& pad = sys_->pad;
            if (pad.down(gs::BTN_LEFT) || pad.axisX < -0.3f) dir -= 1.f;
            if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.3f) dir += 1.f;
            aim_ = std::clamp(aim_ + dir * 1.7f * kDt, -2.2f, 2.2f);
            bool drop = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_DOWN);
            if (drop && !dropping_) {
                dropping_ = true;
                blip(300.f);
            }
        }
        if (dropping_) chain_ = std::max(0.f, chain_ - kChainRate * kDt);
        resolveChain();
    }

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
        if (r.stopped && !r.spooked && r.z >= kLine - 0.02f && std::fabs(r.lat) <= kRoadHalf * 0.75f) ++held;
    }
    stopped_ = held;
    if (clear && shut_ && held == kColumn && through_ == 0) {
        settle_ += kDt;
        if (settle_ >= 0.35f) win();
    } else {
        settle_ = 0.f;
    }
    if (mode_ == Mode::Play && t_ > 26.f) lose("THE COLUMN DID NOT STOP");
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.13f) {
            static const float good[] = {330.f, 440.f, 554.f, 659.f};
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
        sys_->apu.tone(0, 98.f, 0.018f);
    } else if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0.f, 0.f);
    } else if (dropping_) {
        sys_->apu.tone(1, 70.f, 0.03f);
    } else {
        sys_->apu.tone(1, 0.f, 0.f);
    }
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                 puffs_.end());
    for (Rig& r : rigs_)
        if (r.puff > 0.f) r.puff -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
}

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 3, 3) : gs::rgb4(4, 5, 7));
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int rr = std::clamp(int(1.f + u * 4.f), 0, 15);
            int g = std::clamp(int(2.f + u * 4.f), 0, 15);
            int b = std::clamp(int(6.f + u * 4.f), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(rr, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float tt = (float(y) - kHorizon) / kSpan;
        if (tt < 0.004f) tt = 0.004f;
        float wz = kZNear / tt;
        float ppm = kPpm * tt;
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
        float fogT = std::clamp((0.16f - tt) / 0.16f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 10.f);
        v.lineBackdrop[y] = gs::rgb4(2, 3, 2);
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

void Game::drawChain(float shx) {
    Proj road = project(bend(kLine) + aim_ * 0.15f, kLine + 1.5f);
    float winX = 160.f + shx + aim_ * 18.f;
    float winY = 168.f;
    float drop = 1.f - chain_;
    float tipX = road.ok ? road.x + shx : winX;
    float tipY = road.ok ? road.y : 150.f;
    float x = winX + (tipX - winX) * drop;
    float y = winY + (tipY - winY) * drop * 0.72f;
    int n = 8;
    int pal = shut_ ? PAL_ALERT : PAL_GOLD;
    for (int i = 0; i < n; ++i) {
        float u = float(i) / float(n - 1);
        spr(art_.link, winX + (x - winX) * u, winY + (y - winY) * u, 7.f, (i & 1) ? pal : PAL_WHITE, false, 0, false,
            false);
    }
    spr(art_.lamp, winX, winY - 6.f, shut_ ? 16.f : 12.f, shut_ ? PAL_ALERT : PAL_LAMP, false, 0, false, false);
    if (shut_ && road.ok) {
        float span = kRoadHalf * 1.7f * road.ppm;
        sprBox(art_.link, road.x + shx, road.y, span, std::max(4.f, road.ppm * 0.35f), PAL_ALERT, fogFor(kLine));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = shake_ > 0.f ? std::sin(float(sys_->frame) * 1.7f) * 3.f * std::min(shake_, 1.f) : 0.f;
    road(shx);

    if (mode_ == Mode::Title) text("TOWER COLUMN", 160.f + shx, 22.f, 1.1f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 22.f, 1.15f, PAL_GOOD);
    else if (mode_ == Mode::Fail)
        text(reason_, 160.f + shx, 22.f, std::strlen(reason_) > 16 ? 0.7f : 0.95f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 22.f, 1.2f, PAL_GOLD);

    spr(art_.tower, 160.f + shx, 224.f, 132.f, PAL_STONE, false, 0, true, false);
    drawChain(shx);
    spr(art_.cloud, 54.f + std::sin(t_ * 0.2f) * 6.f, 18.f, 12.f, PAL_FX, false, 0, false, false);
    spr(art_.cloud, 230.f + std::sin(t_ * 0.15f) * 8.f, 14.f, 10.f, PAL_FX, true, 0, false, false);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < (int)rigs_.size(); ++i) items.push_back({rigs_[i].z, 0, i});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({puffs_[i].z, 1, i});
    for (int i = 0; i < (int)props_.size(); ++i) items.push_back({props_[i].z, 2, i});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 1) {
            const Puff& f = puffs_[it.id];
            Proj p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, p.x + shx, p.y - u * 8.f, std::clamp(p.ppm * (1.2f + u), 4.f, 24.f), PAL_FX, false,
                fogFor(f.z), false, false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[it.id];
            Proj p = project(bend(pr.z) + pr.lat, pr.z);
            if (!p.ok) continue;
            spr(art_.tree, p.x + shx, p.y, std::clamp(pr.h * p.ppm, 4.f, 80.f), PAL_TREE, pr.lat > 0, fogFor(pr.z),
                true, false);
            continue;
        }
        const Rig& r = rigs_[it.id];
        if (r.z < kZNear + 0.2f) continue;
        Proj p = project(bend(r.z) + r.lat, r.z);
        if (!p.ok) continue;
        const gs::Mipped* body = r.kind == 0 ? &art_.car : &art_.truck;
        int pal = r.kind == 0 ? PAL_CAR : PAL_TRUCK;
        float h = std::clamp(tallOf(r.kind) * p.ppm, 3.f, 78.f);
        spr(art_.shadow, p.x + shx, p.y, h * 0.22f, PAL_FX, false, 0, false, true);
        spr(*body, p.x + shx, p.y, h, pal, false, fogFor(r.z), true, false);
        if (r.column && r.slot == 0)
            spr(art_.pennant, p.x + shx, p.y - h, h * 0.28f, PAL_FLAG, false, fogFor(r.z), true, false);
    }

    if (mode_ == Mode::Title) {
        hudC(23, "YOU HAVE THE TOWER", PAL_TEXT);
        hudC(24, "STOP THE COLUMN ON THE ROAD", PAL_GOLD);
        hudC(25, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(26, "LEFT RIGHT AIM   A DROPS THE CHAIN", PAL_TEXT);
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
        hud(1, 0, shut_ ? "CHAIN DOWN" : "CHAIN UP", shut_ ? PAL_ALERT : PAL_GOOD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(28, 0, buf, PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "LET THEM PASS")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "DROP THE CHAIN")) pal = PAL_GOLD;
        else if (!std::strcmp(h, "TOO CLOSE") || !std::strcmp(h, "AIM THE ROAD")) pal = PAL_ALERT;
        hudC(1, h, pal);
        hudC(27, "LEFT RIGHT AIM   A DROPS", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        chain_ = 0.45f + 0.2f * std::sin(t_ * 1.1f);
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
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
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Fail) sys.setLight(190, 40, 30);
    else if (shut_) sys.setLight(170, 50, 30);
    else sys.setLight(160, 130, 50);
}

}  // namespace twc
