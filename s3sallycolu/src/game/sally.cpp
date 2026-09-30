#include "game/sally.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace sally {
namespace {

constexpr int kWagons = 4;
constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 52.f;
constexpr float kSpan = 170.f;
constexpr float kZNear = 6.0f;
constexpr float kPpm = 28.f;
constexpr float kRoadHalf = 4.5f;
constexpr float kBandNear = 16.f;
constexpr float kBandFar = 27.5f;
constexpr float kThrough = 12.2f;
constexpr float kHorn = 14.f;
constexpr float kSpace = 24.f;
constexpr float kCruise = 6.2f;
constexpr float kLeadZ = 90.f;
constexpr float kFlagV = 8.4f;
constexpr float kReach = 1.35f;
constexpr float kFlagZ = 21.5f;

const float kLane[kWagons] = {-1.7f, 1.55f, 0.15f, -1.35f};

float bend(float z) {
    float u = std::max(0.f, z - 24.f);
    return u * u * 0.00055f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

Game::Spot Game::project(float wx, float wz) const {
    Spot p;
    if (!(wz > kZNear + 0.04f)) return p;
    float t = kZNear / wz;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * kSpan;
    p.x = 160.f + wx * p.ppm;
    p.ok = true;
    return p;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float f = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
    return int(f * 11.f);
}

bool Game::covers(float lat) const { return std::fabs(flag_ - lat) <= kReach; }

int Game::lead() const {
    int best = -1;
    float z = 1e9f;
    for (int i = 0; i < (int)wagons_.size(); ++i) {
        if (wagons_[i].stopped) continue;
        if (wagons_[i].z < z) {
            z = wagons_[i].z;
            best = i;
        }
    }
    return best;
}

float Game::botDir() const {
    int i = lead();
    if (i < 0) return 0.f;
    float d = wagons_[i].lat - flag_;
    if (std::fabs(d) < 0.08f) return 0.f;
    return d > 0.f ? 1.f : -1.f;
}

bool Game::botHalt() const {
    int i = lead();
    if (i < 0 || horn_ > 0.f) return false;
    const Wagon& w = wagons_[i];
    return w.z >= kBandNear && w.z <= kBandFar && covers(w.lat);
}

void Game::lay(bool parked) {
    wagons_.clear();
    puffs_.clear();
    for (int i = 0; i < kWagons; ++i) {
        Wagon w;
        w.lane = kLane[i];
        w.lat = parked ? w.lane : w.lane * 0.25f;
        w.speed = parked ? 0.f : kCruise;
        w.stopped = parked;
        w.z = parked ? (kBandNear + 1.6f + float(i) * 2.5f) : (kLeadZ + float(i) * kSpace);
        wagons_.push_back(w);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = kWagons;
    through_ = 0;
    t_ = 0;
    flag_ = 0;
    settle_ = 0;
    hold_ = 0;
    horn_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE WATCH IS OPEN";
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
    flag_ = 0;
    settle_ = 0;
    hold_ = 0;
    horn_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    reason_ = "THE WATCH IS OPEN";
    blip(440.f);
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
    sys_->apu.tone(2, freq, 0.06f);
    blip_ = 0.08f;
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.4f;
    shake_ = 0.8f;
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = false;
    sys_->apu.noiseBurst(0.3f, 180.f, 0.2f);
}

void Game::winWatch() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.5f;
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = true;
    stopped_ = kWagons;
}

void Game::halt() {
    int i = lead();
    if (i < 0 || horn_ > 0.f) return;
    horn_ = 0.28f;
    Wagon& w = wagons_[i];
    if (!covers(w.lat)) {
        blip(180.f);
        return;
    }
    if (w.z >= kBandNear && w.z <= kBandFar) {
        w.stopped = true;
        w.speed = 0;
        w.lat = w.lane;
        ++stopped_;
        blip(520.f);
        sys_->apu.noiseBurst(0.12f, 90.f, 0.08f);
        return;
    }
    if (w.z > kBandFar && w.z <= kBandFar + kHorn) {
        w.stopped = true;
        w.speed = 0;
        lose("HALTED OFF THE ROAD");
        return;
    }
    if (w.z < kBandNear && w.z >= kThrough) {
        w.stopped = true;
        w.speed = 0;
        lose("HALTED IN THE MOUTH");
        return;
    }
    blip(220.f);
}

void Game::update() {
    t_ += kDt;
    if (horn_ > 0.f) horn_ -= kDt;
    float dir = 0.f;
    bool sound = false;
    if (bot_) {
        dir = botDir();
        sound = botHalt();
    } else {
        const gs::Pad& pad = sys_->pad;
        bool right = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.axisX > 0.35f;
        bool left = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.axisX < -0.35f;
        if (right && !left) dir = 1.f;
        else if (left && !right) dir = -1.f;
        sound = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Y);
    }
    flag_ = std::clamp(flag_ + dir * kFlagV * kDt, -kRoadHalf + 0.35f, kRoadHalf - 0.35f);

    for (Wagon& w : wagons_) {
        if (w.stopped || mode_ != Mode::Play) continue;
        w.z -= w.speed * kDt;
        w.lat += (w.lane - w.lat) * std::min(1.f, 1.8f * kDt);
        if (w.puff <= 0.f && puffs_.size() < 16) {
            w.puff = 0.14f;
            puffs_.push_back({w.z, w.lat, 0.f, 0.4f});
        }
        if (w.z < kThrough) {
            ++through_;
            lose("THE COLUMN TOOK THE SALLY");
            return;
        }
    }
    if (mode_ != Mode::Play) return;
    if (sound) halt();
    if (mode_ != Mode::Play) return;

    bool all = wagons_.size() == size_t(kWagons);
    for (const Wagon& w : wagons_)
        if (!w.stopped || w.z < kBandNear - 0.05f || w.z > kBandFar + 0.05f) all = false;
    if (all && through_ == 0 && stopped_ == kWagons) {
        settle_ += kDt;
        if (settle_ >= 0.4f) winWatch();
    } else {
        settle_ = 0.f;
    }
    if (mode_ == Mode::Play && t_ > 36.f) lose("THE COLUMN DID NOT STOP");
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.14f) {
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
    } else if (mode_ == Mode::Play) {
        sys_->apu.tone(0, 55.f, 0.02f);
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(mode_ == Mode::Fail ? gs::rgb4(8, 3, 3) : gs::rgb4(5, 6, 10));
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y < int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(std::clamp(int(2.f + (1.f - u) * 2.f), 0, 15),
                                         std::clamp(int(2.f + u * 3.f), 0, 15),
                                         std::clamp(int(6.f + (1.f - u) * 6.f), 0, 15));
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
        bool band = wz >= kBandNear && wz <= kBandFar;
        rd.pal = uint8_t(band ? PAL_BAND : PAL_ROAD);
        rd.style = 1;
        rd.band = band ? 1 : ((int(std::floor(wz * 0.18f)) & 1) ? 1 : 0);
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.15f - t) / 0.15f, 0.f, 1.f);
        int f = int(fogT * 9.f);
        if (mode_ == Mode::Fail) f = std::min(16, f + 3);
        v.lineFog[y] = uint8_t(f);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (!(h > 1.4f) || m.h < 1) return;
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 8.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 8.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.9f) * 3.f * std::min(shake_, 1.f);
    road(shx);

    if (mode_ == Mode::Title) text("SALLY COLUMN", 160.f + shx, 22.f, 1.15f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 24.f, 1.2f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text(reason_, 160.f + shx, 24.f, std::strlen(reason_) > 18 ? 0.65f : 0.9f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 24.f, 1.1f, PAL_GOLD);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < (int)wagons_.size(); ++i) items.push_back({wagons_[i].z, 0, i});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({puffs_[i].z, 1, i});
    items.push_back({kFlagZ, 2, 0});
    items.push_back({11.5f, 3, 0});
    items.push_back({kBandNear, 4, 0});
    items.push_back({kBandFar, 4, 1});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z > b.z; });

    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        const Item& item = *it;
        if (item.kind == 1) {
            const Puff& f = puffs_[item.id];
            Spot p = project(bend(f.z) + f.lat, f.z);
            if (!p.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, p.x + shx, p.y - u * 6.f, std::clamp(p.ppm * (0.9f + u), 4.f, 22.f), PAL_DUST, false, fogFor(f.z),
                false);
        } else if (item.kind == 0) {
            const Wagon& w = wagons_[item.id];
            Spot p = project(bend(w.z) + w.lat, w.z);
            if (!p.ok) continue;
            float h = std::clamp(2.35f * p.ppm, 6.f, 150.f);
            spr(art_.wagon, p.x + shx, p.y, h, PAL_WAGON, w.lane < 0.f, fogFor(w.z), true);
        } else if (item.kind == 4) {
            float z = item.id ? kBandFar : kBandNear;
            for (int side = -1; side <= 1; side += 2) {
                Spot p = project(bend(z) + float(side) * (kRoadHalf + 0.35f), z);
                if (!p.ok) continue;
                spr(art_.post, p.x + shx, p.y, std::clamp(1.6f * p.ppm, 6.f, 70.f), PAL_GOLD, false, fogFor(z), true);
            }
        } else if (item.kind == 2) {
            Spot p = project(bend(kFlagZ) + flag_, kFlagZ);
            if (!p.ok) continue;
            float h = std::clamp(2.8f * p.ppm, 10.f, 120.f);
            spr(art_.flag, p.x + shx, p.y, h, PAL_FLAG, flag_ > 0.15f, fogFor(kFlagZ), true);
        } else if (item.kind == 3) {
            float z = 11.2f;
            for (int side = -1; side <= 1; side += 2) {
                Spot p = project(bend(z) + float(side) * (kRoadHalf + 1.35f), z);
                if (!p.ok) continue;
                float h = std::clamp(6.4f * p.ppm, 20.f, 200.f);
                spr(art_.jamb, p.x + shx, p.y, h, PAL_STONE, side > 0, fogFor(z), true);
                Spot t = project(bend(z) + float(side) * (kRoadHalf + 0.15f), z + 0.4f);
                if (t.ok) spr(art_.torch, t.x + shx, t.y - h * 0.45f, h * 0.22f, PAL_TORCH, false, 0, false);
            }
        }
    }

    if (mode_ == Mode::Title) {
        hudC(22, "AT THE SALLY", PAL_HUD);
        hudC(23, "STOP THE COLUMN ON THE ROAD", PAL_GOLD);
        hudC(24, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(26, "LEFT RIGHT MOVES THE STANDARD", PAL_HUD);
        hudC(27, "A HALTS    START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(26, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(1, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(27, "START", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(1, reason_, PAL_ALERT);
        hudC(2, "THE WATCH IS OVER", PAL_HUD);
        hudC(27, "START RETRIES", PAL_HUD);
    } else {
        char buf[24];
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kWagons);
        hud(1, 0, buf, PAL_HUD);
        int i = lead();
        const char* hint = "WAIT FOR THE ROAD";
        int pal = PAL_HUD;
        if (i >= 0) {
            float z = wagons_[i].z;
            if (z >= kBandNear && z <= kBandFar) {
                hint = covers(wagons_[i].lat) ? "HALT" : "COVER THE WAGON";
                pal = PAL_GOLD;
            } else if (z < kBandNear + 4.f) {
                hint = "TOO CLOSE";
                pal = PAL_ALERT;
            }
        }
        hudC(1, hint, pal);
        hudC(27, "A HALTS ON THE OCHRE ROAD", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        flag_ = 1.15f * std::sin(t_ * 0.7f);
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else {
        hold_ -= kDt;
        if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }), puffs_.end());
    for (Wagon& w : wagons_)
        if (w.puff > 0.f) w.puff -= kDt;
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 40, 30);
    else if (mode_ == Mode::Play) sys.setLight(170, 110, 40);
    else sys.setLight(90, 80, 140);
}

}  // namespace sally
