#include "game/convoy.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>

namespace convoy {

namespace {

constexpr float DT = 1.f / 60.f;
constexpr float HORIZON = 92.f;
constexpr float FOCAL = 268.f;
constexpr float CAM_H = 2.0f;
constexpr float ASPHALT = 3.7f;
constexpr float LANE = 2.0f;
constexpr float TW = 0.95f;
constexpr float TRUCK_V = 30.f;
constexpr float TRUCK_LAT = 8.5f;
constexpr float LOOK = 155.f;
constexpr float JEEP_LAT = 14.f;
constexpr float CAM_BACK = 5.3f;
constexpr float SHOT_V = 125.f;
constexpr float FIRE_CD = 0.09f;
constexpr float HIT_X = 2.5f;
constexpr float HIT_Z = 2.3f;
constexpr float RAIDER_V = 15.f;
constexpr float RAIDER_LAT = 3.8f;
constexpr float WAKE = 64.f;
constexpr float DEPOT = 1420.f;
constexpr float GAP = 11.5f;
constexpr float TRUCK_W = 1.90f;
constexpr float WRECK_W = 3.80f;
constexpr float MINE_W = 2.60f;

float bend(float z) { return std::sin(z * 0.008f) * 9.f + std::sin(z * 0.02f + 0.7f) * 2.5f; }

uint16_t lerp4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int x, int y) { return int(std::lround(x + (y - x) * t)); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

int16_t n16(float v) { return int16_t(std::lround(std::clamp(v, -1800.f, 1800.f))); }

bool laneHits(float lane, float ox, float hw) { return std::fabs(lane - ox) < TW + hw; }

}  // namespace

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Drive || mode_ == Mode::Pause) return 1;
    return 2;
}

void Game::resetCourse() {
    static constexpr float O[NOBS][4] = {
        {200.f, -1.65f, 1.90f, 0.f},
        {470.f, 1.65f, 1.90f, 0.f},
        {760.f, -2.00f, 1.30f, 1.f},
        {1060.f, 1.65f, 1.90f, 0.f},
    };
    static constexpr float R[NRAID][2] = {{330.f, 6.3f}, {610.f, -6.3f}, {910.f, 6.3f}, {1220.f, -6.3f}};
    for (int i = 0; i < NOBS; i++) {
        obs_[i] = {O[i][0], O[i][1], O[i][2], int(O[i][3]), false};
    }
    for (int i = 0; i < NRAID; i++) raid_[i] = {R[i][1], R[i][0], R[i][1], 2, true, false};
    shots_.clear();
    booms_.clear();
    puffs_.clear();
    hull_ = 100;
    score_ = 0;
    ticks_ = 0;
    order_ = -1;
    truckX_ = -LANE;
    truckZ_ = 0.f;
    jeepX_ = -LANE;
    jeepZ_ = -GAP;
    jeepV_ = TRUCK_V;
    cliff_ = 1.0e9f;
    shake_ = 0.f;
    fireCd_ = 0.f;
    fanT_ = 0.f;
    fanStep_ = -1;
    lostSnd_ = false;
    won_ = false;
    over_ = false;
}

void Game::toTitle() {
    resetCourse();
    mode_ = Mode::Title;
    menuWait_ = 0;
    attract_ = 0.f;
}

void Game::begin() {
    resetCourse();
    mode_ = Mode::Drive;
    blip(false);
    sys_->setLight(40, 180, 60);
}

void Game::win() {
    if (mode_ != Mode::Drive) return;
    if (hull_ < 1) hull_ = 1;
    score_ += hull_;
    truckZ_ = DEPOT;
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    fanT_ = 0.f;
    fanStep_ = -1;
    sys_->setLight(40, 200, 70);
    sys_->rumble(0.35f, 0.2f, 220);
}

void Game::lose() {
    if (mode_ != Mode::Drive) return;
    hull_ = 0;
    won_ = false;
    over_ = true;
    mode_ = Mode::Dead;
    cliff_ = 58.f;
    if (!lostSnd_) {
        lostSnd_ = true;
        sys_->apu.keyOff(0);
        sys_->apu.noiseBurst(0.55f, 320.f, 0.45f);
        sys_->apu.keyOn(2, 68.f, 0.28f);
        sys_->setLight(210, 30, 20);
    }
}

void Game::blip(bool high) {
    blipHi_ = high;
    blipT_ = 0.1f;
}

void Game::gun() {
    sys_->apu.noiseBurst(0.3f, 8600.f, 0.04f);
    sys_->apu.keyOn(1, 170.f, 0.16f);
}

void Game::killRaider(int i) {
    if (!raid_[i].live) return;
    raid_[i].live = false;
    score_ += 100;
    booms_.push_back({raid_[i].x, raid_[i].z, 0.f});
    sys_->apu.noiseBurst(0.42f, 2400.f, 0.12f);
    sys_->rumble(0.15f, 0.45f, 80);
}

void Game::hurt(int dmg, float x, float z, const char* why) {
    (void)why;
    if (mode_ != Mode::Drive) return;
    hull_ -= dmg;
    if (hull_ < 0) hull_ = 0;
    shake_ = 0.45f;
    booms_.push_back({x, z, 0.f});
    sys_->rumble(0.8f, 0.35f, 180);
    sys_->apu.noiseBurst(0.5f, 1600.f, 0.16f);
    if (hull_ <= 0) lose();
}

void Game::human(float& steer, float& gas, bool& fire) {
    const gs::Pad& p = sys_->pad;
    steer = p.axisX;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    steer = std::clamp(steer, -1.f, 1.f);
    gas = 0.f;
    if (p.down(gs::BTN_UP)) gas += 1.f;
    if (p.down(gs::BTN_DOWN)) gas -= 1.f;
    gas = std::clamp(gas, -1.f, 1.f);
    fire = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
    int next = order_;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_X)) next = -1;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_Y)) next = 1;
    if (next != order_) {
        order_ = next;
        blip(next > 0);
    }
}

void Game::pilot(float& steer, float& gas, bool& fire) {
    int want = order_;
    bool badL = false, badR = false;
    for (const Obs& o : obs_) {
        if (o.passed) continue;
        if (o.z <= truckZ_ + 1.f || o.z > truckZ_ + LOOK) continue;
        if (laneHits(-LANE, o.x, o.hw)) badL = true;
        if (laneHits(LANE, o.x, o.hw)) badR = true;
    }
    bool bad = want < 0 ? badL : badR;
    bool other = want < 0 ? badR : badL;
    if (bad && !other) want = -want;
    if (want != order_) {
        order_ = want;
        blip(want > 0);
    }

    int threat = -1;
    float best = 1.0e9f;
    for (int i = 0; i < NRAID; i++) {
        if (!raid_[i].live) continue;
        float dz = raid_[i].z - truckZ_;
        if (dz < -2.f || dz > 78.f) continue;
        float score = std::fabs(dz);
        if (score < best) {
            best = score;
            threat = i;
        }
    }
    float aim = truckX_;
    float gap = truckZ_ - jeepZ_;
    gas = std::clamp((gap - GAP) * 0.45f, -1.f, 1.f);
    fire = false;
    if (threat >= 0) {
        aim = raid_[threat].x;
        float dx = std::fabs(jeepX_ - aim);
        float rdz = raid_[threat].z - jeepZ_;
        if (dx < 1.2f && rdz > 2.f && rdz < 72.f) fire = true;
        if (raid_[threat].z - truckZ_ < 18.f) gas = 1.f;
    }
    steer = std::clamp((aim - jeepX_) * 0.95f, -1.f, 1.f);
}

void Game::physics(float steer, float gas, bool fire) {
    ticks_++;
    float prev = truckZ_;
    float dest = float(order_) * LANE;
    float dx = dest - truckX_;
    float step = TRUCK_LAT * DT;
    if (std::fabs(dx) <= step) truckX_ = dest;
    else truckX_ += std::copysign(step, dx);
    truckZ_ += TRUCK_V * DT;

    jeepX_ = std::clamp(jeepX_ + steer * JEEP_LAT * DT, -7.6f, 7.6f);
    jeepV_ = TRUCK_V + gas * 14.f;
    jeepZ_ += jeepV_ * DT;
    if (jeepZ_ > truckZ_ - 1.7f) jeepZ_ = truckZ_ - 1.7f;
    if (jeepZ_ < truckZ_ - 34.f) jeepZ_ = truckZ_ - 34.f;

    for (int i = 0; i < NRAID; i++) {
        Raider& r = raid_[i];
        if (!r.live) continue;
        if (!r.woke && truckZ_ > r.z - WAKE) {
            r.woke = true;
            blip(true);
        }
        if (r.woke) {
            r.z += RAIDER_V * DT;
            float tx = truckX_ + std::sin(r.z * 0.18f) * 0.65f;
            float d = tx - r.x;
            float st = RAIDER_LAT * DT;
            if (std::fabs(d) <= st) r.x = tx;
            else r.x += std::copysign(st, d);
        }
    }

    if (fire && fireCd_ <= 0.f && shots_.size() < 10) {
        shots_.push_back({jeepX_, jeepZ_ + 1.2f});
        fireCd_ = FIRE_CD;
        gun();
    }
    if (fireCd_ > 0.f) fireCd_ -= DT;

    size_t nw = 0;
    for (size_t i = 0; i < shots_.size(); i++) {
        Shot s = shots_[i];
        s.z += SHOT_V * DT;
        bool gone = s.z > truckZ_ + 82.f;
        if (!gone) {
            for (int r = 0; r < NRAID; r++) {
                if (!raid_[r].live) continue;
                if (std::fabs(s.x - raid_[r].x) < HIT_X && std::fabs(s.z - raid_[r].z) < HIT_Z) {
                    raid_[r].hp--;
                    gone = true;
                    if (raid_[r].hp <= 0) killRaider(r);
                    else booms_.push_back({raid_[r].x, raid_[r].z, 0.f});
                    break;
                }
            }
        }
        if (!gone) shots_[nw++] = s;
    }
    shots_.resize(nw);

    for (int i = 0; i < NRAID; i++) {
        Raider& r = raid_[i];
        if (!r.live) continue;
        if (std::fabs(jeepX_ - r.x) < 1.85f && std::fabs(jeepZ_ - r.z) < 2.7f) {
            killRaider(i);
            continue;
        }
        if (std::fabs(truckX_ - r.x) < 1.65f && std::fabs(truckZ_ - r.z) < 2.8f) {
            float push = truckX_ >= r.x ? 0.4f : -0.4f;
            truckX_ = std::clamp(truckX_ + push, -2.6f, 2.6f);
            hurt(26, r.x, r.z, "raider");
            r.live = false;
        } else if (r.z < truckZ_ - 18.f) {
            r.live = false;
        }
    }

    if (mode_ == Mode::Drive) {
        for (Obs& o : obs_) {
            if (o.passed) continue;
            if (prev < o.z && truckZ_ >= o.z) {
                o.passed = true;
                if (std::fabs(truckX_ - o.x) < TW + o.hw) hurt(o.kind ? 22 : 34, o.x, o.z, o.kind ? "mine" : "wreck");
            }
        }
    }
    truckX_ = std::clamp(truckX_, -2.6f, 2.6f);
    if (mode_ == Mode::Drive && truckZ_ >= DEPOT) win();

    if ((frame_ % 4) == 0 && puffs_.size() < 12) {
        float s = (frame_ & 1) ? 0.42f : -0.42f;
        puffs_.push_back({jeepX_ + s, jeepZ_ - 0.15f, 0.f});
        puffs_.push_back({truckX_ - s, truckZ_ - 1.8f, 0.f});
    }
    size_t pw = 0;
    for (size_t i = 0; i < puffs_.size(); i++) {
        puffs_[i].t += DT;
        if (puffs_[i].t < 0.55f) puffs_[pw++] = puffs_[i];
    }
    puffs_.resize(pw);
    size_t bw = 0;
    for (size_t i = 0; i < booms_.size(); i++) {
        booms_[i].t += DT;
        if (booms_[i].t < 0.45f) booms_[bw++] = booms_[i];
    }
    booms_.resize(bw);
}

void Game::audio() {
    float vol = 0.07f;
    float spd = 16.f;
    if (mode_ == Mode::Drive) {
        vol = 0.16f;
        spd = jeepV_;
        if (hull_ > 66) sys_->setLight(40, 180, 60);
        else if (hull_ > 33) sys_->setLight(220, 140, 30);
        else sys_->setLight(210, 40, 24);
    } else if (mode_ == Mode::Pause) {
        vol = 0.04f;
    } else if (mode_ == Mode::Win) {
        vol = 0.09f;
        spd = 20.f;
        fanT_ += DT;
        int step = int(fanT_ / 0.16f);
        if (step != fanStep_ && step >= 0 && step < 8) {
            fanStep_ = step;
            static const float notes[8] = {392.f, 523.f, 659.f, 784.f, 659.f, 784.f, 1046.f, 784.f};
            sys_->apu.keyOn(2, notes[step], 0.22f);
        }
    } else if (mode_ == Mode::Dead) {
        vol = 0.f;
    }
    if (engineOn_ && mode_ != Mode::Dead) {
        sys_->apu.setFreq(0, 34.f + spd * 0.8f);
        sys_->apu.setVol(0, vol);
    }
    if (blipT_ > 0.f) {
        blipT_ -= DT;
        sys_->apu.tone(0, blipHi_ ? 720.f : 400.f, blipT_ > 0.f ? 0.07f : 0.f);
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::screen(const gs::Mipped& m, float x, float y, float w, float h, int pal, bool flip) {
    if (w < 1.f || h < 1.f || m.h <= 0) return;
    gs::Sprite s;
    s.x = n16(x);
    s.y = n16(y);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::center(const gs::Mipped& m, float y, int pal) {
    screen(m, (gs::SCREEN_W - m.w) * 0.5f, y, float(m.w), float(m.h), pal, false);
}

float Game::text(const char* s, float x, float y, int pal, float scale) {
    while (*s) {
        unsigned char uc = static_cast<unsigned char>(*s++);
        if (uc < 32 || uc > 127) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.font[uc - 32];
        float w = g.w * scale;
        float h = g.h * scale;
        if (uc != ' ') screen(g, x, y, w, h, pal, false);
        x += w - scale;
    }
    return x;
}

float Game::textWidth(const char* s, float scale) const {
    float x = 0.f;
    while (*s) {
        unsigned char uc = static_cast<unsigned char>(*s++);
        if (uc < 32 || uc > 127) {
            x += 10.f * scale;
            continue;
        }
        x += art_.font[uc - 32].w * scale - scale;
    }
    return x;
}

void Game::stamp(const gs::Mipped& m, float lat, float z, float worldW, int pal, bool flip, bool shadow, float bias) {
    float dz = z - camZ_;
    if (dz < 0.75f || dz > 210.f || m.w <= 0 || m.h <= 0 || worldW < 0.15f) return;
    float worldH = worldW * float(m.h) / float(m.w);
    float sc = FOCAL / dz;
    float h = worldH * sc;
    if (h < 1.4f || h > 460.f) return;
    float w = h * float(m.w) / float(m.h);
    float dx = bend(z) + lat - bend(camZ_) - camLat_;
    float sx = 160.f + dx * sc;
    float sy = HORIZON + CAM_H * sc;
    gs::Sprite s;
    s.x = n16(sx - w * 0.5f);
    s.y = n16(sy - h);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    s.fog = uint8_t(std::clamp(int((dz - 24.f) / 12.f), 0, 12));
    q_.push_back({dz + bias, s});
}

void Game::flush() {
    std::sort(q_.begin(), q_.end(), [](const Q& a, const Q& b) { return a.key < b.key; });
    for (const Q& s : q_) sys_->vdp.sprite(s.sp);
    q_.clear();
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    const bool title = mode_ == Mode::Title;
    float jolt = shake_ > 0.f ? std::sin(frame_ * 1.7f) * shake_ * 0.18f : 0.f;
    float truckX, truckZ, jeepX, jeepZ;
    if (title) {
        camZ_ = attract_;
        camLat_ = 0.f;
        truckX = 0.85f;
        truckZ = attract_ + 15.f;
        jeepX = -0.45f;
        jeepZ = attract_ + 5.4f;
    } else {
        camZ_ = jeepZ_ - CAM_BACK;
        camLat_ = jeepX_ + jolt;
        truckX = truckX_;
        truckZ = truckZ_;
        jeepX = jeepX_;
        jeepZ = jeepZ_;
    }

    const uint16_t skyTop = gs::rgb4(3, 5, 11);
    const uint16_t skyHor = gs::rgb4(14, 10, 6);
    const int horizon = int(HORIZON);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= horizon) {
            float u = float(y) / HORIZON;
            v.lineBackdrop[y] = lerp4(skyTop, skyHor, u * u);
            v.lineFog[y] = u > 0.8f ? uint8_t((u - 0.8f) / 0.2f * 6.f) : 0;
            v.road[y].on = false;
            continue;
        }
        float zRel = CAM_H * FOCAL / (float(y) - HORIZON);
        float z = camZ_ + zRel;
        if (zRel > cliff_) {
            v.road[y].on = false;
            float rim = zRel - cliff_;
            v.lineBackdrop[y] = rim < 1.3f ? gs::rgb4(10, 4, 2) : gs::rgb4(3, 1, 1);
            v.lineFog[y] = 0;
            continue;
        }
        float dxm = bend(z) - bend(camZ_) - camLat_;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.f + dxm * FOCAL / zRel;
        r.hw = ASPHALT * FOCAL / zRel;
        r.v = z * 24.f;
        r.pal = PAL_ROAD;
        r.band = 0;
        r.style = 1;
        r.left = 0;
        r.right = 0;
        v.lineFog[y] = uint8_t(std::clamp(int((zRel - 18.f) / 11.f), 0, 13));
        v.lineBackdrop[y] = skyHor;
    }
    int pan = int(std::lround(bend(camZ_) * -1.6f));
    int cloud = int(frame_ * 0.35f) + pan / 3;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.B.hscroll[y] = int16_t(pan);
        v.A.hscroll[y] = int16_t(cloud);
    }

    if (title) {
        center(art_.title, 4, PAL_AMBER);
        center(art_.sub1, 38, PAL_WHITE);
        center(art_.sub2, 56, PAL_RED);
        center(art_.help1, 168, PAL_WHITE);
        center(art_.help2, 186, PAL_WHITE);
        if ((frame_ / 24) & 1) center(art_.roll, 206, PAL_AMBER);
    } else {
        screen(art_.quad, 8, 5, 66, 9, PAL_DIM, false);
        int fw = int(62.f * std::max(0, hull_) / 100.f);
        bool blink = hull_ < 34 && (frame_ & 8);
        if (fw > 0 && !blink) {
            int pal = hull_ < 34 ? PAL_RED : hull_ < 67 ? PAL_AMBER : PAL_GREEN;
            screen(art_.quad, 10, 7, float(fw), 5, pal, false);
        }
        char buf[48];
        std::snprintf(buf, sizeof buf, "HULL %d", hull_);
        text(buf, 80, 3, PAL_WHITE, 1.f);
        float miles = std::max(0.f, DEPOT - truckZ_) / 260.f;
        std::snprintf(buf, sizeof buf, "%.1f MI", miles);
        text(buf, gs::SCREEN_W - textWidth(buf, 1.f) - 6.f, 3, PAL_AMBER, 1.f);
        text(order_ < 0 ? "LEFT LANE" : "RIGHT LANE", 8, 18, PAL_AMBER, 1.f);
        if (mode_ == Mode::Drive && ticks_ < 280) text("Z LEFT   X RIGHT   C FIRE", 8, 34, PAL_WHITE, 1.f);
        bool hot = false;
        if (mode_ == Mode::Drive) {
            for (const Raider& r : raid_) {
                if (!r.live) continue;
                float dz = r.z - jeepZ_;
                if (dz > 2.f && dz < 70.f && std::fabs(r.x - jeepX_) < 1.2f) hot = true;
            }
        }
        screen(art_.reticle, 152, HORIZON + 12, 16, 16, hot ? PAL_RED : PAL_WHITE, false);
        if (mode_ == Mode::Pause) center(art_.paused, 76, PAL_AMBER);
        if (mode_ == Mode::Win) {
            center(art_.arrived, 62, PAL_GREEN);
            std::snprintf(buf, sizeof buf, "SCORE %d", score_);
            text(buf, (gs::SCREEN_W - textWidth(buf, 1.f)) * 0.5f, 98, PAL_WHITE, 1.f);
        }
        if (mode_ == Mode::Dead) center(art_.ends, 62, PAL_RED);
    }

    float z0 = std::floor((camZ_ - 2.f) / 26.f) * 26.f;
    for (int i = 0; i < 10; i++) {
        float z = z0 + i * 26.f;
        int id = int(std::lround(z / 26.f));
        if (id < 0) continue;
        float side = (id & 1) ? 1.f : -1.f;
        if (id % 6 == 0) {
            if (z - camZ_ > 22.f) stamp(art_.butte, side * (11.5f + float(id % 3)), z, 10.f, PAL_PROP, side < 0, false, 0.4f);
        } else {
            float mag = 6.1f + float((id * 17) % 5) * 0.5f;
            stamp(art_.cactus, side * mag, z, 1.25f, PAL_PROP, side > 0, false, 0.f);
        }
    }
    for (const Obs& o : obs_) {
        if (title && o.z < attract_) continue;
        stamp(o.kind ? art_.mine : art_.wreck, o.x, o.z, o.kind ? MINE_W : WRECK_W, PAL_HAZ, false, false, 0.2f);
    }
    if (!title) {
        for (const Raider& r : raid_) {
            if (!r.live) continue;
            stamp(art_.shadow, r.x, r.z, 1.35f, PAL_WHITE, false, true, 0.3f);
            stamp(art_.bike, r.x, r.z, 1.2f, PAL_RAID, r.side > 0, false, 0.f);
        }
    } else {
        for (const Raider& r : raid_) {
            if (r.z < attract_) continue;
            stamp(art_.shadow, r.x, r.z, 1.35f, PAL_WHITE, false, true, 0.3f);
            stamp(art_.bike, r.side, r.z, 1.2f, PAL_RAID, r.side > 0, false, 0.f);
        }
    }
    stamp(art_.arch, 0.f, DEPOT, 8.4f, PAL_PROP, false, false, 1.2f);

    const bool dead = mode_ == Mode::Dead;
    float bodyW = dead ? 2.6f : TRUCK_W;
    const gs::Mipped& body = dead ? art_.wreck : art_.truck;
    stamp(art_.shadow, truckX, truckZ, bodyW, PAL_WHITE, false, true, 0.3f);
    stamp(body, truckX, truckZ, bodyW, dead ? PAL_HAZ : PAL_TRUCK, false, false, 0.f);
    stamp(art_.shadow, jeepX, jeepZ, 1.75f, PAL_WHITE, false, true, 0.3f);
    stamp(art_.jeep, jeepX, jeepZ, 1.65f, PAL_JEEP, false, false, -0.02f);
    for (const Shot& s : shots_) stamp(art_.shot, s.x, s.z, 0.4f, PAL_FX, false, false, -0.08f);
    for (const Boom& b : booms_) {
        float u = b.t / 0.45f;
        int fr = u < 0.34f ? 0 : u < 0.67f ? 1 : 2;
        stamp(art_.boom[fr], b.x, b.z, 1.6f + u * 1.8f, PAL_FX, false, false, -0.12f);
    }
    for (const Puff& p : puffs_) {
        float u = std::clamp(p.t / 0.55f, 0.f, 1.f);
        stamp(art_.puff, p.x, p.z, 1.05f * (1.f - 0.45f * u), PAL_FX, false, false, 0.05f);
    }
    if (mode_ == Mode::Drive || mode_ == Mode::Pause) {
        float dz = truckZ - camZ_;
        if (dz > 0.8f && dz < 80.f) {
            float sc = FOCAL / dz;
            float worldH = TRUCK_W * float(art_.truck.h) / float(std::max(1, art_.truck.w));
            float h = worldH * sc;
            float sx = 160.f + (bend(truckZ) + truckX - bend(camZ_) - camLat_) * sc;
            float sy = HORIZON + CAM_H * sc;
            screen(art_.arrow, sx - 8.f, sy - h - 14.f, 16, 12, PAL_AMBER, order_ > 0);
        }
    }
    flush();
}

void Game::titleFrame() {
    attract_ += 12.f * DT;
    bool go = false;
    if (bot_) go = ++menuWait_ >= 45;
    else go = sys_->pad.pressed(gs::BTN_START);
    if (!bot_ && sys_->pad.pressed(gs::BTN_MODE)) sys_->eject();
    if (go) begin();
    audio();
    draw();
}

void Game::driveFrame() {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        audio();
        draw();
        return;
    }
    float steer = 0.f, gas = 0.f;
    bool fire = false;
    if (bot_) pilot(steer, gas, fire);
    else human(steer, gas, fire);
    physics(steer, gas, fire);
    audio();
    draw();
}

void Game::pauseFrame() {
    if (sys_->pad.pressed(gs::BTN_START)) mode_ = Mode::Drive;
    audio();
    draw();
}

void Game::endFrame() {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        toTitle();
        audio();
        draw();
        return;
    }
    if (mode_ == Mode::Dead) cliff_ = std::max(2.4f, cliff_ - 26.f * DT);
    size_t bw = 0;
    for (size_t i = 0; i < booms_.size(); i++) {
        booms_[i].t += DT;
        if (booms_[i].t < 0.45f) booms_[bw++] = booms_[i];
    }
    booms_.resize(bw);
    audio();
    draw();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    gs::FMPatch eng;
    eng.alg = 7;
    eng.fb = 0.4f;
    eng.drive = 0.3f;
    eng.tone = 880.f;
    eng.vol = 0.1f;
    eng.glide = 0.002f;
    eng.op[0] = {1.f, 0.85f, 0.02f, 0.45f, 0.75f, 0.3f, 0.f};
    eng.op[1] = {2.f, 0.32f, 0.02f, 0.5f, 0.55f, 0.3f, 4.f};
    eng.op[2] = {0.5f, 0.4f, 0.03f, 0.55f, 0.5f, 0.28f, 0.f};
    eng.op[3] = {3.02f, 0.18f, 0.02f, 0.4f, 0.4f, 0.25f, 0.f};
    sys.apu.setPatch(0, eng);
    gs::FMPatch click;
    click.alg = 7;
    click.vol = 0.2f;
    click.drive = 0.45f;
    click.op[0] = {1.f, 1.f, 0.001f, 0.05f, 0.f, 0.04f, 0.f};
    click.op[1] = {2.4f, 0.35f, 0.001f, 0.04f, 0.f, 0.04f, 0.f};
    click.op[2] = {0.5f, 0.25f, 0.001f, 0.06f, 0.f, 0.04f, 0.f};
    click.op[3] = {4.f, 0.15f, 0.001f, 0.04f, 0.f, 0.03f, 0.f};
    sys.apu.setPatch(1, click);
    gs::FMPatch bell;
    bell.alg = 4;
    bell.fb = 0.18f;
    bell.vol = 0.2f;
    bell.op[0] = {2.f, 0.5f, 0.001f, 0.32f, 0.08f, 0.4f, 0.f};
    bell.op[1] = {1.f, 1.f, 0.001f, 0.65f, 0.f, 0.55f, 0.f};
    bell.op[2] = {4.f, 0.32f, 0.001f, 0.22f, 0.f, 0.3f, 0.f};
    bell.op[3] = {1.f, 0.75f, 0.001f, 0.8f, 0.f, 0.65f, 0.f};
    sys.apu.setPatch(2, bell);
    sys.apu.setMaster(0.72f);
    sys.apu.keyOn(0, 46.f, 0.07f);
    engineOn_ = true;
    sys.vdp.setFogColor(gs::rgb4(13, 10, 6));
    q_.reserve(180);
    shots_.reserve(12);
    toTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    frame_++;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT);
    switch (mode_) {
        case Mode::Title: titleFrame(); break;
        case Mode::Drive: driveFrame(); break;
        case Mode::Pause: pauseFrame(); break;
        case Mode::Win:
        case Mode::Dead: endFrame(); break;
    }
}

}  // namespace convoy
