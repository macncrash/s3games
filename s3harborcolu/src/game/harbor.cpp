#include "game/harbor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace hcol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 74.f;
constexpr float kSpan = 142.f;
constexpr float kZNear = 7.2f;
constexpr float kPpm = 22.f;
constexpr float kRoadHalf = 5.1f;
constexpr float kLine = 14.6f;
constexpr float kPanic = 25.f;
constexpr float kTurnZ = 23.f;
constexpr float kGap = 4.7f;
constexpr float kChainRate = 1.25f;
constexpr float kAcross = 0.78f;
constexpr float kLeadZ = 102.f;
constexpr float kLorryV = 5.55f;
constexpr float kWatch = 38.f;

float quayBend(float z) { return std::sin(z * 0.018f) * 1.15f; }

float parkLat(int slot) {
    if (slot <= 0) return 0.15f;
    return (slot & 1) ? -1.05f : 1.15f;
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
    float f = std::clamp((0.18f - t) / 0.18f, 0.f, 1.f);
    return int(f * 11.f);
}

bool Game::tenderClear() const {
    for (const Rig& r : rigs_)
        if (!r.column) return r.berthed;
    return true;
}

float Game::leadZ() const {
    for (const Rig& r : rigs_)
        if (r.column && r.slot == 0) return r.z;
    return 999.f;
}

void Game::buildProps() {
    props_.clear();
    for (int i = 0; i < 7; ++i) {
        float z = 20.f + float(i) * 14.f;
        Prop shed;
        shed.z = z;
        shed.lat = kRoadHalf + 2.4f;
        shed.h = 3.4f;
        shed.kind = 0;
        props_.push_back(shed);
        Prop crane;
        crane.z = z + 6.f;
        crane.lat = kRoadHalf + 3.6f;
        crane.h = 5.2f;
        crane.kind = 1;
        props_.push_back(crane);
        Prop buoy;
        buoy.z = z + 3.f;
        buoy.lat = -kRoadHalf - 2.6f;
        buoy.h = 1.6f;
        buoy.kind = 2;
        props_.push_back(buoy);
    }
}

void Game::lay() {
    rigs_.clear();
    puffs_.clear();
    Rig tender;
    tender.column = false;
    tender.z = 48.f;
    tender.lat = 0.2f;
    tender.cruise = 7.1f;
    tender.speed = tender.cruise;
    tender.haltZ = 18.f;
    rigs_.push_back(tender);
    for (int i = 0; i < kColumn; ++i) {
        Rig lorry;
        lorry.column = true;
        lorry.slot = i;
        lorry.z = kLeadZ + float(i) * 10.4f;
        lorry.lat = (i & 1) ? 0.4f : -0.3f;
        lorry.cruise = kLorryV;
        lorry.speed = kLorryV;
        lorry.haltZ = kLine + 2.15f + float(i) * kGap;
        rigs_.push_back(lorry);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    chain_ = 0.25f;
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
    chain_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE WATCH IS OVER";
    blip(540.f);
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
    sys_->apu.noiseBurst(0.22f, 240.f, 0.22f);
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
    if (!tenderClear()) {
        if (chain_ >= kAcross * 0.55f) return "CHAIN IS EARLY";
        return "LET THE TENDER SLIP";
    }
    if (chain_ < kAcross) {
        if (leadZ() <= kPanic + 2.f) return "TOO CLOSE";
        return "DROP THE CHAIN";
    }
    return "HOLD THE CHAIN";
}

void Game::stepTender(Rig& r, bool across) {
    if (r.berthed) {
        r.z = r.haltZ;
        r.lat = -kRoadHalf - 2.5f;
        r.speed = 0.f;
        r.turning = false;
        return;
    }
    if (r.turning) {
        r.lat -= 4.8f * kDt;
        r.z -= 2.05f * kDt;
        r.speed = 2.05f;
        if (r.lat <= -kRoadHalf - 1.7f) {
            r.berthed = true;
            blip(460.f);
        } else if (across && r.z <= kLine + 2.4f) {
            lose("NOT THE COLUMN");
        } else if (r.z < kLine - 0.4f) {
            lose("THE TENDER RAN THE ROAD");
        }
        return;
    }
    if (across && r.z <= kLine + 4.2f) {
        lose("NOT THE COLUMN");
        return;
    }
    if (!across && r.z <= kTurnZ) {
        r.turning = true;
        return;
    }
    r.speed = r.cruise;
    r.z -= r.speed * kDt;
    r.lat += (0.2f - r.lat) * std::min(1.f, 2.4f * kDt);
    if (r.z < kLine) lose("THE TENDER RAN THE ROAD");
}

void Game::stepLorry(Rig& r, bool across) {
    if (r.spooked) {
        r.lat -= 4.5f * kDt;
        r.speed = std::max(r.speed, 2.5f);
        r.z -= r.speed * kDt;
        if (r.lat < -kRoadHalf - 0.25f) lose("OFF THE QUAY");
        else if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (r.stopped && across) {
        r.z = r.haltZ;
        r.lat = parkLat(r.slot);
        r.speed = 0.f;
        return;
    }
    if (r.stopped && !across) {
        r.stopped = false;
        r.orderly = false;
        r.speed = std::max(r.speed, 0.85f);
    }
    if (!across) {
        r.orderly = false;
        r.speed = std::min(r.cruise, r.speed + 3.4f * kDt);
        r.z -= r.speed * kDt;
        r.lat += (0.f - r.lat) * std::min(1.f, 3.f * kDt);
        if (r.z < kLine) {
            ++through_;
            lose("THE COLUMN PASSED");
        }
        return;
    }
    if (!r.orderly) {
        if (r.z <= kPanic && r.speed > 2.05f) {
            r.spooked = true;
            blip(120.f);
            return;
        }
        r.orderly = true;
    }
    float park = parkLat(r.slot);
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
        if (!r.stopped) blip(90.f + float(r.slot) * 22.f);
        r.stopped = true;
        return;
    }
    if (dist > 8.5f) {
        r.speed = r.cruise;
        r.z -= r.speed * kDt;
        r.lat += (park - r.lat) * std::min(1.f, 2.f * kDt);
    } else {
        float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.3f));
        a = std::min(a, 24.f);
        r.z -= r.speed * kDt;
        r.speed = std::max(0.f, r.speed - a * kDt);
        r.lat += (park - r.lat) * std::min(1.f, 3.6f * kDt);
        if (r.speed > 0.5f && r.puff <= 0.f && puffs_.size() < 14) {
            r.puff = 0.14f;
            Puff puff;
            puff.z = r.z + 0.8f;
            puff.lat = r.lat;
            puffs_.push_back(puff);
        }
    }
    if (r.speed < 0.2f && dist < 1.6f) {
        r.z = r.haltZ;
        r.lat = park;
        r.speed = 0.f;
        if (!r.stopped) blip(90.f + float(r.slot) * 22.f);
        r.stopped = true;
    }
}

void Game::update() {
    t_ += kDt;
    float dir = 0.f;
    if (bot_) {
        dir = tenderClear() ? 1.f : -1.f;
    } else {
        const gs::Pad& pad = sys_->pad;
        bool drop = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.axisY < -0.35f;
        bool lift = pad.down(gs::BTN_UP) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.axisY > 0.35f;
        if (drop && !lift) dir = 1.f;
        else if (lift && !drop) dir = -1.f;
    }
    float prev = chain_;
    chain_ = std::clamp(chain_ + dir * kChainRate * kDt, 0.f, 1.f);
    bool across = chain_ >= kAcross;
    if ((prev >= kAcross) != across) {
        blip(across ? 130.f : 360.f);
        sys_->rumble(across ? 0.3f : 0.08f, across ? 0.4f : 0.1f, across ? 80 : 36);
    }
    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        if (r.column) stepLorry(r, across);
        else stepTender(r, across);
    }
    if (mode_ != Mode::Play) return;
    int held = 0;
    for (const Rig& r : rigs_) {
        if (!r.column || !r.stopped || r.spooked) continue;
        if (r.z >= kLine - 0.05f && std::fabs(r.lat) <= kRoadHalf * 0.72f) ++held;
    }
    stopped_ = held;
    if (across && tenderClear() && through_ == 0 && held == kColumn) settle_ += kDt;
    else settle_ = 0.f;
    if (settle_ >= 0.4f) win();
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
            static const float good[] = {349.f, 440.f, 523.f, 698.f};
            static const float bad[] = {196.f, 164.f, 130.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Play) {
        float hum = chain_ > 0.05f && chain_ < 0.98f ? 70.f : 0.f;
        sys_->apu.tone(1, hum, hum > 0.f ? 0.03f : 0.f);
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 90.f, 0.015f);
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
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(9, 4, 3) : gs::rgb4(6, 8, 11));
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int rr = std::clamp(int(2.f + u * 6.f), 0, 15);
            int g = std::clamp(int(4.f + u * 6.f), 0, 15);
            int b = std::clamp(int(10.f - u * 2.f), 0, 15);
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
        rd.cx = 160.f + quayBend(wz) * ppm + shx;
        rd.hw = std::max(2.f, kRoadHalf * ppm);
        rd.v = wz * 30.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.2f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_LAND;
    }
}

void Game::drawChain(float shx) {
    float left = quayBend(kLine) - kRoadHalf + 0.35f;
    float right = quayBend(kLine) + kRoadHalf - 0.35f;
    Spot L = project(left, kLine);
    Spot R = project(right, kLine);
    if (!L.ok || !R.ok) return;
    int fog = fogFor(kLine);
    float postH = std::clamp(1.35f * L.ppm, 12.f, 48.f);
    spr(art_.post, L.x + shx, L.y, postH, PAL_CHAIN, false, fog, true);
    spr(art_.post, R.x + shx, R.y, postH * 0.95f, PAL_CHAIN, true, fog, true);
    float rise = (1.f - chain_) * postH * 0.82f;
    int n = 14;
    for (int i = 0; i < n; ++i) {
        float u = (float(i) + 0.5f) / float(n);
        float sag = std::sin(u * 3.14159f) * (6.f + chain_ * 10.f);
        float x = L.x + (R.x - L.x) * u;
        float y = L.y - rise + sag * (0.35f + 0.65f * (1.f - chain_));
        int pal = (i & 1) ? PAL_LINK : PAL_CHAIN;
        float box = std::clamp(L.ppm * 0.28f, 4.f, 12.f);
        sprBox(art_.link, x + shx, y - postH * 0.15f, box, box * 0.7f, pal, fog);
    }
    int lampPal = chain_ >= kAcross ? PAL_ALERT : PAL_LAMP;
    spr(art_.lamp, L.x + shx, L.y - postH, std::clamp(L.ppm * 0.45f, 6.f, 16.f), lampPal, false, 0, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.7f) * 3.2f * std::min(shake_, 1.f);
    layRoad(shx);

    if (mode_ == Mode::Title) text("HARBOR COLUMN", 160.f + shx, 28.f, 1.05f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 32.f, 1.1f, PAL_GOOD);
    else if (mode_ == Mode::Fail)
        text(reason_, 160.f + shx, 32.f, std::strlen(reason_) > 16 ? 0.7f : 0.95f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 32.f, 1.15f, PAL_GOLD);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < (int)rigs_.size(); ++i) items.push_back({rigs_[i].z, 0, i});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({puffs_[i].z, 1, i});
    for (int i = 0; i < (int)props_.size(); ++i) items.push_back({props_[i].z, 2, i});
    items.push_back({kLine - 0.2f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 3) {
            drawChain(shx);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot p = project(quayBend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.puff, p.x + shx, p.y - u * 8.f, std::clamp(p.ppm * (1.2f + u), 4.f, 22.f), PAL_FX, false,
                fogFor(f.z), false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[size_t(it.id)];
            Spot p = project(quayBend(pr.z) + pr.lat, pr.z);
            if (!p.ok) continue;
            float h = std::clamp(pr.h * p.ppm, 4.f, 90.f);
            int fog = fogFor(pr.z);
            if (pr.kind == 1) spr(art_.crane, p.x + shx, p.y, h, PAL_CRANE, false, fog, true);
            else if (pr.kind == 2) {
                float bob = std::sin(t_ * 1.6f + pr.z) * 2.f;
                spr(art_.buoy, p.x + shx, p.y + bob, h, PAL_BUOY, false, fog, true);
            } else spr(art_.shed, p.x + shx, p.y, h, PAL_SHED, pr.z > 40.f, fog, true);
            continue;
        }
        const Rig& r = rigs_[size_t(it.id)];
        if (r.z < kZNear + 0.15f) continue;
        Spot p = project(quayBend(r.z) + r.lat, r.z);
        if (!p.ok) continue;
        int fog = fogFor(r.z);
        float tall = r.column ? 3.15f : 2.15f;
        float h = std::clamp(tall * p.ppm, 3.f, 84.f);
        spr(art_.shadow, p.x + shx, p.y, h * 0.26f, PAL_FX, false, 0, false);
        if (r.column) spr(art_.lorry, p.x + shx, p.y, h, PAL_LORRY, false, fog, true);
        else spr(art_.tender, p.x + shx, p.y, h, PAL_TENDER, r.turning, fog, true);
    }

    float gullX = 48.f + std::fmod(t_ * 18.f, 240.f);
    spr(art_.gull, gullX + shx, 36.f + std::sin(t_ * 3.f) * 4.f, 10.f, PAL_GULL, false, 0, false);
    spr(art_.cloud, 70.f + std::sin(t_ * 0.2f) * 6.f + shx, 18.f, 12.f, PAL_FX, false, 0, false);
    spr(art_.cloud, 210.f + std::sin(t_ * 0.15f) * 8.f + shx, 14.f, 10.f, PAL_FX, true, 0, false);

    bool across = chain_ >= kAcross;
    if (mode_ == Mode::Title) {
        hudC(22, "AT THE HARBOR", PAL_TEXT);
        hudC(23, "STOP THE COLUMN ON THE ROAD", PAL_GOLD);
        hudC(24, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(25, "LET THE TENDER ONTO THE SLIP", PAL_TEXT);
        hudC(26, "DOWN/C DROPS THE CHAIN", PAL_TEXT);
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
        hud(1, 0, across ? "CHAIN ACROSS" : "CHAIN UP", across ? PAL_ALERT : PAL_GOOD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(28, 0, buf, PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "DROP THE CHAIN") || !std::strcmp(h, "HOLD THE CHAIN")) pal = PAL_GOLD;
        else if (!std::strcmp(h, "TOO CLOSE") || !std::strcmp(h, "CHAIN IS EARLY")) pal = PAL_ALERT;
        hudC(1, h, pal);
        hudC(27, "DOWN DROPS  UP LIFTS", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        chain_ = 0.15f + 0.25f * (0.5f + 0.5f * std::sin(t_ * 0.8f));
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
    if (won_) sys.setLight(40, 160, 80);
    else if (mode_ == Mode::Fail) sys.setLight(190, 36, 28);
    else sys.setLight(30, 70, 110);
}

}  // namespace hcol
