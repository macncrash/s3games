#include "game/lot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace lotc {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 78.f;
constexpr float kSpan = 136.f;
constexpr float kZNear = 6.8f;
constexpr float kPpm = 24.f;
constexpr float kRoadHalf = 4.8f;
constexpr float kLine = 16.2f;
constexpr float kPanic = 27.f;
constexpr float kTurnZ = 26.f;
constexpr float kGap = 5.1f;
constexpr float kBoomRate = 1.4f;
constexpr float kAcross = 0.8f;
constexpr float kLeadZ = 108.f;
constexpr float kVanV = 5.2f;
constexpr float kWatch = 42.f;

float lotBend(float z) { return std::sin(z * 0.022f) * 0.85f; }

float laneLat(int slot) {
    if (slot <= 0) return 0.05f;
    return (slot & 1) ? -0.85f : 0.95f;
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
    float f = std::clamp((0.16f - t) / 0.16f, 0.f, 1.f);
    return int(f * 10.f);
}

bool Game::shopperParked() const {
    for (const Rig& r : rigs_)
        if (!r.column) return r.parked;
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
        float z = 22.f + float(i) * 16.f;
        Prop booth;
        booth.z = z;
        booth.lat = kRoadHalf + 2.8f;
        booth.h = 3.1f;
        booth.kind = 0;
        props_.push_back(booth);
        Prop lamp;
        lamp.z = z + 7.f;
        lamp.lat = -kRoadHalf - 1.8f;
        lamp.h = 4.6f;
        lamp.kind = 1;
        props_.push_back(lamp);
        Prop cone;
        cone.z = z + 3.5f;
        cone.lat = kRoadHalf + 1.15f;
        cone.h = 1.15f;
        cone.kind = 2;
        props_.push_back(cone);
        Prop tree;
        tree.z = z + 10.f;
        tree.lat = -kRoadHalf - 3.4f;
        tree.h = 3.8f;
        tree.kind = 3;
        props_.push_back(tree);
    }
}

void Game::lay() {
    rigs_.clear();
    puffs_.clear();
    Rig shop;
    shop.column = false;
    shop.z = 52.f;
    shop.lat = 0.15f;
    shop.cruise = 6.6f;
    shop.speed = shop.cruise;
    shop.haltZ = 20.f;
    rigs_.push_back(shop);
    for (int i = 0; i < kColumn; ++i) {
        Rig van;
        van.column = true;
        van.slot = i;
        van.z = kLeadZ + float(i) * 11.2f;
        van.lat = (i & 1) ? 0.35f : -0.25f;
        van.cruise = kVanV;
        van.speed = kVanV;
        van.haltZ = kLine + 2.4f + float(i) * kGap;
        rigs_.push_back(van);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    boom_ = 0.2f;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE WATCH IS OVER";
    lay();
}

void Game::begin() {
    lay();
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    boom_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
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
    sys.apu.setEcho(0.16f, 0.2f, 0.1f);
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
    hold_ = 1.6f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.22f, 220.f, 0.22f);
    sys_->rumble(0.5f, 0.16f, 160);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.4f;
    fanfare(true);
    sys_->rumble(0.22f, 0.48f, 150);
}

const char* Game::hint() const {
    if (!shopperParked()) {
        if (boom_ >= kAcross * 0.55f) return "BOOM IS EARLY";
        return "LET THE SHOPPER PARK";
    }
    if (boom_ < kAcross) {
        if (leadZ() <= kPanic + 2.f) return "TOO CLOSE";
        return "DROP THE BOOM";
    }
    return "HOLD THE BOOM";
}

void Game::stepShopper(Rig& r, bool across) {
    if (r.parked) {
        r.z = r.haltZ;
        r.lat = -kRoadHalf - 2.7f;
        r.speed = 0.f;
        r.turning = false;
        return;
    }
    if (r.turning) {
        r.lat -= 4.6f * kDt;
        r.z -= 1.9f * kDt;
        r.speed = 1.9f;
        if (r.lat <= -kRoadHalf - 1.9f) {
            r.parked = true;
            blip(440.f);
        } else if (across && r.z <= kLine + 2.6f) {
            lose("NOT THE COLUMN");
        } else if (r.z < kLine - 0.3f) {
            lose("THE SHOPPER LEFT THE LOT");
        }
        return;
    }
    if (across && r.z <= kLine + 4.4f) {
        lose("NOT THE COLUMN");
        return;
    }
    if (!across && r.z <= kTurnZ) {
        r.turning = true;
        return;
    }
    r.speed = r.cruise;
    r.z -= r.speed * kDt;
    r.lat += (0.15f - r.lat) * std::min(1.f, 2.2f * kDt);
    if (r.z < kLine) lose("THE SHOPPER LEFT THE LOT");
}

void Game::stepVan(Rig& r, bool across) {
    if (r.spooked) {
        r.lat += 4.2f * kDt;
        r.speed = std::max(r.speed, 2.4f);
        r.z -= r.speed * kDt;
        if (r.lat > kRoadHalf + 0.35f) lose("OFF THE LOT");
        else if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (r.stopped && across) {
        r.z = r.haltZ;
        r.lat = laneLat(r.slot);
        r.speed = 0.f;
        return;
    }
    if (r.stopped && !across) {
        r.stopped = false;
        r.orderly = false;
        r.speed = std::max(r.speed, 0.8f);
    }
    if (!across) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 3.2f * kDt);
        r.z -= r.speed * kDt;
        r.lat += (0.f - r.lat) * std::min(1.f, 2.8f * kDt);
        if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.1f) {
            r.spooked = true;
            blip(110.f);
            return;
        }
        r.orderly = true;
    }
    float park = laneLat(r.slot);
    float dist = r.z - r.haltZ;
    if (dist <= 0.2f) {
        if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
            return;
        }
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(88.f + float(r.slot) * 24.f);
        r.stopped = true;
        return;
    }
    if (dist > 9.f) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 1.8f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.35f));
        a = std::min(a, 22.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 3.2f * kDt);
        if (r.speed > 0.55f && r.puff <= 0.f && puffs_.size() < 12) {
            r.puff = 0.16f;
            Puff puff;
            puff.z = r.z + 0.9f;
            puff.lat = r.lat;
            puffs_.push_back(puff);
        }
    }
    if (r.speed < 0.22f && dist < 1.7f) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(88.f + float(r.slot) * 24.f);
        r.stopped = true;
    }
}

void Game::update() {
    t_ += kDt;
    float dir = 0.f;
    if (bot_) {
        dir = shopperParked() ? 1.f : -1.f;
    } else {
        const gs::Pad& pad = sys_->pad;
        bool drop = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.axisY < -0.35f;
        bool lift = pad.down(gs::BTN_UP) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.axisY > 0.35f;
        if (drop && !lift) dir = 1.f;
        else if (lift && !drop) dir = -1.f;
    }
    float prev = boom_;
    boom_ = std::clamp(boom_ + dir * kBoomRate * kDt, 0.f, 1.f);
    bool across = boom_ >= kAcross;
    if ((prev >= kAcross) != across) {
        blip(across ? 140.f : 340.f);
        sys_->rumble(across ? 0.28f : 0.08f, across ? 0.38f : 0.1f, across ? 70 : 32);
    }
    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        if (r.column) stepVan(r, across);
        else stepShopper(r, across);
    }
    if (mode_ != Mode::Play) return;
    int held = 0;
    for (const Rig& r : rigs_) {
        if (!r.column || !r.stopped || r.spooked) continue;
        if (r.z >= kLine - 0.05f && std::fabs(r.lat) <= kRoadHalf * 0.75f) ++held;
    }
    stopped_ = held;
    if (across && shopperParked() && through_ == 0 && held == kColumn) settle_ += kDt;
    else settle_ = 0.f;
    if (settle_ >= 0.45f) win();
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
            static const float good[] = {330.f, 392.f, 494.f, 659.f};
            static const float bad[] = {180.f, 150.f, 120.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Play) {
        float hum = boom_ > 0.04f && boom_ < 0.97f ? 62.f : 0.f;
        sys_->apu.tone(1, hum, hum > 0.f ? 0.028f : 0.f);
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 78.f, 0.012f);
    } else {
        sys_->apu.tone(1, 0.f, 0.f);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
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
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::layRoad(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(10, 3, 2) : gs::rgb4(9, 5, 3));
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int rr = std::clamp(int(12.f - u * 4.f), 0, 15);
            int g = std::clamp(int(6.f - u * 2.f), 0, 15);
            int b = std::clamp(int(3.f + u * 3.f), 0, 15);
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
        rd.cx = 160.f + lotBend(wz) * ppm + shx;
        rd.hw = std::max(2.f, kRoadHalf * ppm);
        rd.v = wz * 28.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.18f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
    }
}

void Game::drawBoom(float shx) {
    float left = lotBend(kLine) - kRoadHalf + 0.2f;
    float right = lotBend(kLine) + kRoadHalf - 0.15f;
    Spot L = project(left, kLine);
    Spot R = project(right, kLine);
    if (!L.ok || !R.ok) return;
    int fog = fogFor(kLine);
    float postH = std::clamp(1.5f * L.ppm, 14.f, 52.f);
    spr(art_.post, L.x + shx, L.y, postH * 0.92f, PAL_BOOM, true, fog, true);
    spr(art_.post, R.x + shx, R.y, postH, PAL_BOOM, false, fog, true);
    int n = 11;
    for (int i = 0; i < n; ++i) {
        float u = (float(i) + 0.5f) / float(n);
        float along = u * boom_;
        float x = R.x + (L.x - R.x) * along;
        float lift = (1.f - boom_) * postH * (0.15f + 0.7f * (1.f - u));
        float y = R.y - postH * 0.82f + lift * 0.15f - (1.f - boom_) * (1.f - u) * postH * 0.05f;
        y = R.y - lift - postH * 0.12f * boom_;
        int pal = (i % 3 == 0) ? PAL_ARM : PAL_BOOM;
        float box = std::clamp(L.ppm * 0.42f, 5.f, 16.f);
        sprBox(art_.arm, x + shx, y, box * 1.4f, box * 0.55f, pal, fog);
    }
    int lampPal = boom_ >= kAcross ? PAL_ALERT : PAL_LAMP;
    spr(art_.sign, R.x + shx, R.y - postH, std::clamp(L.ppm * 0.7f, 8.f, 22.f), lampPal, false, 0, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.7f) * 3.2f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) text("LOT COLUMN", 160.f + shx, 26.f, 1.1f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 30.f, 1.1f, PAL_GOOD);
    else if (mode_ == Mode::Fail)
        text(reason_, 160.f + shx, 30.f, std::strlen(reason_) > 16 ? 0.68f : 0.92f, PAL_ALERT);
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
            drawBoom(shx);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot p = project(lotBend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.puff, p.x + shx, p.y - u * 7.f, std::clamp(p.ppm * (1.1f + u), 4.f, 20.f), PAL_FX, false,
                fogFor(f.z), false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[size_t(it.id)];
            Spot p = project(lotBend(pr.z) + pr.lat, pr.z);
            if (!p.ok) continue;
            float h = std::clamp(pr.h * p.ppm, 4.f, 96.f);
            int fog = fogFor(pr.z);
            if (pr.kind == 1) spr(art_.lamp, p.x + shx, p.y, h, PAL_LAMP, false, fog, true);
            else if (pr.kind == 2) spr(art_.cone, p.x + shx, p.y, h, PAL_STRIPE, false, fog, true);
            else if (pr.kind == 3) spr(art_.tree, p.x + shx, p.y, h, PAL_TREE, pr.z > 50.f, fog, true);
            else spr(art_.booth, p.x + shx, p.y, h, PAL_BOOTH, false, fog, true);
            continue;
        }
        const Rig& r = rigs_[size_t(it.id)];
        if (r.z < kZNear + 0.15f) continue;
        Spot p = project(lotBend(r.z) + r.lat, r.z);
        if (!p.ok) continue;
        int fog = fogFor(r.z);
        float tall = r.column ? 3.4f : 1.85f;
        float h = std::clamp(tall * p.ppm, 3.f, 90.f);
        spr(art_.shadow, p.x + shx, p.y, h * 0.22f, PAL_FX, false, 0, false);
        if (r.column) spr(art_.van, p.x + shx, p.y, h, PAL_VAN, false, fog, true);
        else spr(art_.shopper, p.x + shx, p.y, h, PAL_SHOP, r.turning, fog, true);
    }

    bool across = boom_ >= kAcross;
    if (mode_ == Mode::Title) {
        hudC(22, "AT THE LOT", PAL_TEXT);
        hudC(23, "STOP THE COLUMN ON THE ROAD", PAL_GOLD);
        hudC(24, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(25, "LET THE SHOPPER INTO A STALL", PAL_TEXT);
        hudC(26, "DOWN/C DROPS THE BOOM", PAL_TEXT);
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
        hud(1, 0, across ? "BOOM DOWN" : "BOOM UP", across ? PAL_ALERT : PAL_GOOD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(28, 0, buf, PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "DROP THE BOOM") || !std::strcmp(h, "HOLD THE BOOM")) pal = PAL_GOLD;
        else if (!std::strcmp(h, "TOO CLOSE") || !std::strcmp(h, "BOOM IS EARLY")) pal = PAL_ALERT;
        hudC(1, h, pal);
        hudC(27, "DOWN DROPS  UP LIFTS", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        boom_ = 0.12f + 0.22f * (0.5f + 0.5f * std::sin(t_ * 0.7f));
        if (pad.pressed(gs::BTN_START)) begin();
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
    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 150, 60);
    else if (mode_ == Mode::Fail) sys.setLight(180, 40, 24);
    else sys.setLight(140, 70, 30);
}

}  // namespace lotc
