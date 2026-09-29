#include "game/purs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace cwpurs {

namespace {
constexpr float kDt = 1.f / 60.f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (seizeT_ > 0.f) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    boot();
}

void Game::boot() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "NOT THE LAST";
    stalled_ = 0;
    boiler_ = 8;
    lane_ = 0;
    wantLane_ = 0;
    t_ = 0;
    playT_ = 0;
    seizeT_ = 0;
    cool_ = 0;
    shake_ = 0;
    drive_ = false;
    for (int i = 0; i < kFleet; i++) {
        Mach& m = mach_[i];
        m = {};
        m.kind = i;
        m.home = (i % 3) - 1;
        m.lane = m.home;
        m.hp = 4;
        m.z = 18.f + float(i) * 13.f;
        m.think = float(i) * 0.35f;
        m.running = true;
    }
}

void Game::begin() {
    boot();
    mode_ = Mode::Play;
    playT_ = 0;
}

const gs::Mipped& Game::body(int kind) const {
    switch (kind) {
        case 0: return art_.pile;
        case 1: return art_.grade;
        case 2: return art_.pump;
        default: return art_.lorry;
    }
}

int Game::alive() const {
    int n = 0;
    for (const Mach& m : mach_)
        if (m.running) ++n;
    return n;
}

int Game::nearest() const {
    int best = -1;
    float z = 1.0e9f;
    for (int i = 0; i < kFleet; i++) {
        if (!mach_[i].running) continue;
        if (mach_[i].z < z) {
            z = mach_[i].z;
            best = i;
        }
    }
    return best;
}

void Game::stallRival(int i) {
    Mach& m = mach_[i];
    if (!m.running) return;
    m.running = false;
    m.hp = 0;
    ++stalled_;
    seizeT_ = 0.85f;
    shake_ = 5.f;
    if (alive() == 0 && boiler_ > 0) winRoad();
}

void Game::winRoad() {
    if (over_) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
}

void Game::loseRoad(const char* why) {
    if (over_) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    boiler_ = 0;
    reason_ = why;
}

void Game::botIntent(int& laneDir, bool& drive, bool& go) {
    go = true;
    drive = true;
    laneDir = 0;
    int n = nearest();
    if (n < 0) {
        wantLane_ = 0;
        return;
    }
    const Mach& r = mach_[n];
    if (r.boost > 0.f && r.lane == lane_) {
        wantLane_ = lane_ == 0 ? 1 : 0;
    } else if (r.boost <= 0.f && r.z < 7.f) {
        wantLane_ = r.lane;
    } else {
        wantLane_ = 0;
    }
    if (wantLane_ < lane_) laneDir = -1;
    else if (wantLane_ > lane_) laneDir = 1;
}

void Game::rivalsThink() {
    int n = nearest();
    for (Mach& m : mach_) {
        if (!m.running) continue;
        if (m.boost > 0.f) m.boost -= kDt;
        if (m.boost < 0.f) m.boost = 0.f;
        if (m.cool > 0.f) m.cool -= kDt;
    }
    if (n < 0) return;
    Mach& r = mach_[n];
    r.think += kDt;
    if (r.z < 11.f && r.boost <= 0.f && r.think > 2.6f) {
        r.think = 0.f;
        r.boost = 0.48f;
        r.lane = lane_;
    } else if (r.boost <= 0.f) {
        r.lane = r.home;
    }
}

void Game::contact() {
    if (cool_ > 0.f) return;
    for (int i = 0; i < kFleet; i++) {
        Mach& m = mach_[i];
        if (!m.running || m.lane != lane_ || m.z > 3.1f) continue;
        cool_ = 0.38f;
        m.cool = 0.38f;
        shake_ = 4.f;
        if (m.boost > 0.f && !drive_) {
            boiler_ -= 3;
        } else if (m.boost > 0.f) {
            boiler_ -= 2;
            m.hp -= 1;
        } else if (drive_) {
            m.hp -= 2;
        } else {
            m.hp -= 1;
            boiler_ -= 1;
        }
        if (m.hp <= 0) stallRival(i);
        if (boiler_ <= 0) loseRoad("YOUR MACHINE STALLED");
        return;
    }
}

void Game::physics(bool drive) {
    drive_ = drive;
    float playerV = drive ? 20.f : 9.f;
    for (Mach& m : mach_) {
        if (!m.running) continue;
        float rivalV = 11.f + (m.boost > 0.f ? 14.f : 0.f);
        m.z += (rivalV - playerV) * kDt;
        if (m.z < 2.3f) m.z = 2.3f;
        if (m.z > 90.f) m.z = 90.f;
    }
    contact();
}

void Game::stepPlay(int laneDir, bool drive) {
    playT_ += kDt;
    if (seizeT_ > 0.f) seizeT_ -= kDt;
    if (cool_ > 0.f) cool_ -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 8.f);
    rivalsThink();
    if (laneDir < 0) lane_ = std::max(-1, lane_ - 1);
    else if (laneDir > 0) lane_ = std::min(1, lane_ + 1);
    physics(drive);
    if (!over_ && alive() == 0 && boiler_ > 0) winRoad();
    if (!over_ && playT_ > 42.f) loseRoad("THE CAUSEWAY RAN OUT");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    int laneDir = 0;
    bool drive = false;
    bool go = false;
    if (mode_ == Mode::Title) {
        if (bot_) go = t_ > 0.2f;
        else {
            const gs::Pad& p = sys.pad;
            go = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
        }
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        if (bot_) botIntent(laneDir, drive, go);
        else {
            const gs::Pad& p = sys.pad;
            if (p.pressed(gs::BTN_LEFT)) laneDir = -1;
            if (p.pressed(gs::BTN_RIGHT)) laneDir = 1;
            drive = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.accel > 0.4f;
        }
        if (!over_) stepPlay(laneDir, drive);
    }
    serviceAudio();
    draw();
}

void Game::serviceAudio() {
    if (!sys_) return;
    if (mode_ != Mode::Play) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    float f = drive_ ? 78.f : 52.f;
    sys_->apu.tone(0, f, 0.05f);
    sys_->apu.tone(1, shake_ > 0.f ? 140.f : 0.f, shake_ > 0.f ? 0.04f : 0.f);
}

void Game::place(float z, int lane, float& x, float& y, float& h) const {
    float depth = 1.f / (1.f + z * 0.075f);
    h = 14.f + depth * 46.f;
    y = float(kHorizon) + depth * 118.f - h * 0.15f;
    float spread = 18.f + depth * 78.f;
    x = 160.f + float(lane) * spread;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.f || m.h < 1) return;
    float w = h * (float(m.w) / float(m.h));
    gs::Sprite s;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, 0);
    }
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    float sway = std::sin(t_ * 0.25f) * 10.f;
    float scroll = playT_ * (drive_ ? 90.f : 40.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < kHorizon) {
            float sky = float(y) / float(kHorizon);
            v.lineBackdrop[y] = gs::rgb4(2 + int(sky * 4), 3 + int(sky * 3), 7 + int((1.f - sky) * 4));
            v.lineFog[y] = uint8_t(6 - int(sky * 4));
            v.road[y].on = false;
            continue;
        }
        float depth = float(y - kHorizon) / float(gs::SCREEN_H - kHorizon);
        gs::RoadLine& rl = v.road[y];
        rl = {};
        rl.on = true;
        rl.cx = 160.f + sway * (1.f - depth);
        rl.hw = 10.f + depth * depth * 150.f;
        rl.v = 2400.f / (depth + 0.15f) - scroll;
        rl.pal = PAL_ROAD;
        rl.band = (int(std::floor(rl.v / 80.f)) & 1) ? 1 : 0;
        rl.style = gs::ROAD_ROCKY;
        rl.left = gs::GROUND_WATER;
        rl.right = gs::GROUND_WATER;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 6);
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - depth) * 9.f), 0, 9));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    road();

    float bob = std::sin(t_ * 2.1f) * 2.f;
    spr(art_.boat, 40.f, 120.f + bob, 16.f, PAL_BOAT, false, 3);
    spr(art_.boat, 278.f, 136.f - bob, 20.f, PAL_BOAT, true, 2);
    spr(art_.lamp, 160.f, 18.f, 22.f, PAL_LAMP, false, 6);

    for (int i = 3; i >= 0; --i) {
        float z = 8.f + float(i) * 7.f;
        float x, y, h;
        place(z, -1, x, y, h);
        spr(art_.post, x - 36.f, y, h * 0.85f, PAL_STONE, false, int(i * 2));
        place(z, 1, x, y, h);
        spr(art_.post, x + 36.f, y, h * 0.85f, PAL_STONE, false, int(i * 2));
    }

    for (int i = kFleet - 1; i >= 0; --i) {
        const Mach& m = mach_[i];
        float x, y, h;
        float z = m.running ? m.z : std::min(m.z, 6.f);
        place(z, m.lane, x, y, h);
        int pal = m.running ? (PAL_PILE + m.kind) : PAL_DEAD;
        int fog = int(std::clamp(z * 0.18f, 0.f, 10.f));
        spr(body(m.kind), x, y, h, pal, m.lane < 0, fog);
        if (!m.running) spr(art_.splash, x, y + h * 0.45f, h * 0.45f, PAL_FX, false, fog);
    }

    float px, py, ph;
    place(0.4f, lane_, px, py, ph);
    px += std::sin(t_ * 40.f) * shake_ * 0.4f;
    spr(art_.you, px, py + 8.f, ph + 10.f, boiler_ > 0 ? PAL_YOU : PAL_DEAD, false, 0);

    char buf[64];
    if (mode_ == Mode::Title) {
        text("CAUSEWAY", 160.f, 40.f, 1.35f, PAL_TITLE);
        text("LAST MACHINE", 160.f, 64.f, 0.85f, PAL_HUD);
        text("PRESS START", 160.f, 168.f, 0.7f, int(t_ * 2.f) % 2 ? PAL_TITLE : PAL_HUD);
        text("ARROWS LANE   C DRIVE", 160.f, 190.f, 0.5f, PAL_HUD);
    } else if (mode_ == Mode::Play) {
        std::snprintf(buf, sizeof(buf), "BOILER %d", std::max(0, boiler_));
        text(buf, 70.f, 8.f, 0.55f, boiler_ > 2 ? PAL_HUD : PAL_ALERT);
        std::snprintf(buf, sizeof(buf), "RUNNING %d", alive() + (boiler_ > 0 ? 1 : 0));
        text(buf, 250.f, 8.f, 0.55f, PAL_HUD);
        if (seizeT_ > 0.4f) text("STALLED", 160.f, 48.f, 0.8f, PAL_ALERT);
    } else if (mode_ == Mode::Victory) {
        text("LAST MACHINE", 160.f, 64.f, 1.0f, PAL_GOOD);
        text("STILL RUNNING", 160.f, 92.f, 0.8f, PAL_TITLE);
    } else {
        text("LOSS", 160.f, 70.f, 1.1f, PAL_ALERT);
        text(reason_, 160.f, 98.f, 0.5f, PAL_HUD);
    }
}

}  // namespace cwpurs
