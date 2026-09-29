#include "pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr double kFinish = 420.0;
constexpr double kClock = 40.0;
constexpr int kHorizon = 78;
constexpr float kCamZ = 2.35f;

struct Rock {
    double s;
    float lat;
};

constexpr Rock kRocks[] = {
    {70, -0.55f}, {118, 0.52f}, {168, -0.42f}, {214, 0.58f}, {262, -0.50f}, {308, 0.36f}, {352, -0.46f}, {388, 0.40f},
};
constexpr int kRockN = int(sizeof(kRocks) / sizeof(kRocks[0]));

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    int r = ar + int((br - ar) * t);
    int g = ag + int((bg - ag) * t);
    int bl = ab + int((bb - ab) * t);
    return gs::rgb4(r, g, bl);
}

}  // namespace

float Game::clockLeft() const { return float(std::max(0.0, kClock - time_)); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (s_ > 340.0) return 3;
    if (s_ > 150.0) return 2;
    return 1;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    s_ = 18;
    lat_ = 0;
    v_ = 0;
    time_ = 0;
    shake_ = 0;
    chime_ = -1;
}

void Game::startRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    why_ = "";
    s_ = 0;
    lat_ = 0;
    v_ = 0;
    time_ = 0;
    shake_ = 0;
    chime_ = -1;
    chimeT_ = 0;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    v_ = 0;
    shake_ = 1.f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    v_ = 0;
    chime_ = 0;
    chimeT_ = 0;
}

void Game::pilot(float& pedal, float& brake, float& steer) {
    pedal = brake = steer = 0;
    if (bot_) {
        float desired = 0.f;
        const Rock* near = nullptr;
        double nearDz = 1e9;
        for (int i = 0; i < kRockN; i++) {
            double dz = kRocks[i].s - s_;
            if (dz < 4.0 || dz > 34.0) continue;
            if (dz < nearDz) {
                nearDz = dz;
                near = &kRocks[i];
            }
        }
        if (near) desired = near->lat > 0.f ? -0.50f : 0.50f;
        float gust = 0.f;
        double left = kClock - time_;
        if (left < 16.0) gust = std::sin(float(t_) * 2.2f) * float(1.0 - left / 16.0) * 0.22f;
        steer = clampf((desired - lat_ - gust * 0.35f) * 3.4f, -1.f, 1.f);
        if (std::fabs(lat_) > 0.72f) steer = clampf(steer - lat_ * 1.4f, -1.f, 1.f);
        pedal = v_ < 15.5f ? 1.f : 0.35f;
        if (near && nearDz < 9.0 && std::fabs(lat_ - near->lat) < 0.28f) brake = 0.6f;
        return;
    }
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_UP) || pad.accel > 0.2f) pedal = pad.accel > 0.2f ? pad.accel : 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) brake = pad.brake > 0.2f ? pad.brake : 1.f;
    if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
    steer = clampf(steer, -1.f, 1.f);
}

void Game::stepRun(float pedal, float brake, float steer) {
    time_ += kDt;
    float accel = pedal * 9.2f - brake * 16.f - 1.15f;
    v_ = clampf(v_ + accel * kDt, 0.f, 16.8f);
    double left = kClock - time_;
    wind_ = 0.f;
    if (left < 16.0) wind_ = std::sin(float(t_) * 2.2f) * float(1.0 - left / 16.0) * 0.55f;
    float rate = (0.55f + v_ / 16.8f) * 1.45f;
    lat_ = clampf(lat_ + (steer * rate + wind_) * kDt, -1.35f, 1.35f);
    s_ += double(v_) * kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.4f);

    if (lat_ > 1.02f) {
        fail("went over the edge");
        return;
    }
    if (lat_ < -1.02f) {
        fail("buried in the snow wall");
        return;
    }
    for (int i = 0; i < kRockN; i++) {
        double dz = s_ - kRocks[i].s;
        if (std::fabs(dz) < 3.4 && std::fabs(lat_ - kRocks[i].lat) < 0.26f) {
            v_ = std::min(v_, 2.4f);
            lat_ += (lat_ >= kRocks[i].lat ? 0.18f : -0.18f);
            shake_ = 1.f;
            time_ = std::min(kClock - 0.05, time_ + 1.35);
            sys_->apu.noiseBurst(0.35f, 1400.f, 0.12f);
            break;
        }
    }
    if (time_ >= kClock && s_ < kFinish) {
        fail("the storm closed the pass");
        return;
    }
    if (s_ >= kFinish) win();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, int fog) {
    if (ht < 1.5f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 400L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 400L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -400L, 800L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h)), -400L, 800L));
    if (s.x > gs::SCREEN_W + 4 || s.x + s.w < -4 || s.y > gs::SCREEN_H + 4 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.roadTime++;

    const bool title = mode_ == Mode::Title;
    double showS = title ? 40.0 + t_ * 4.0 : s_;
    float showLat = title ? std::sin(float(t_) * 0.6f) * 0.2f : lat_;
    float storm = 0.f;
    if (!title) storm = clampf(1.f - clockLeft() / float(kClock), 0.f, 1.f);
    if (mode_ == Mode::Fail && why_ && std::strstr(why_, "storm")) storm = 1.f;

    uint16_t zen = lerpC(gs::rgb4(3, 5, 10), gs::rgb4(5, 6, 8), storm);
    uint16_t hor = lerpC(gs::rgb4(13, 10, 7), gs::rgb4(9, 9, 11), storm);
    if (mode_ == Mode::Win) hor = gs::rgb4(8, 12, 8);
    vdp.setFogColor(lerpC(gs::rgb4(12, 11, 10), gs::rgb4(10, 11, 13), storm));

    float shakeX = shake_ > 0.f ? std::sin(float(t_) * 48.f) * shake_ * 5.f : 0.f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float skyT = y / float(kHorizon);
        vdp.lineBackdrop[y] = y < kHorizon ? lerpC(zen, hor, skyT) : hor;
        vdp.lineFog[y] = 0;
        gs::RoadLine& r = vdp.road[y];
        if (y <= kHorizon) {
            r.on = false;
            continue;
        }
        float t = (y - kHorizon) / float(gs::SCREEN_H - 1 - kHorizon);
        t = std::max(t, 0.02f);
        float z = kCamZ / t;
        float bend = std::sin(float(showS + z) * 0.018f) * 42.f * (1.f - t * 0.4f);
        float pinch = 1.f;
        double wz = showS + z;
        if (wz > 190.0 && wz < 250.0) pinch = 0.72f;
        if (wz > 330.0) pinch = 0.84f;
        bool snow = wz > 250.0;
        r.on = true;
        r.cx = 160.f + bend + shakeX - showLat * (90.f / std::max(z, 2.f));
        r.hw = (280.f / z) * pinch;
        r.v = float(wz) * 46.f;
        r.pal = snow ? PAL_SNOW : PAL_ROAD;
        r.band = (int(std::floor(wz)) & 1) ? 1 : 0;
        r.style = snow ? gs::ROAD_SNOW : gs::ROAD_ROCKY;
        r.left = gs::GROUND_SNOWWALL;
        r.right = (wz > 120.0) ? gs::GROUND_DROP : gs::GROUND_LAND;
        float fog = 0.f;
        if (z > 22.f) fog = std::min(8.f, (z - 22.f) * 0.28f);
        fog += storm * 7.f * (1.f - t);
        vdp.lineFog[y] = uint8_t(std::min(16.f, fog));
    }

    auto project = [&](double zs, double lat, float& sx, float& sy, float& fog, float& scale) {
        float z = float(std::max(zs, 1.7));
        float t = kCamZ / z;
        int y = kHorizon + int(t * float(gs::SCREEN_H - 1 - kHorizon));
        y = std::clamp(y, 0, gs::SCREEN_H - 1);
        sx = vdp.road[y].cx + float(lat) * vdp.road[y].hw;
        sy = float(y);
        fog = z > 20.f ? std::min(12.f, (z - 20.f) * 0.35f) : 0.f;
        fog = std::min(16.f, fog + storm * 4.f);
        scale = 20.f / std::max(z, 2.f);
    };

    struct Spr {
        float z, sx, sy, h, fog;
        int kind;
    };
    Spr list[28];
    int n = 0;
    auto push = [&](float z, double lat, float h, int kind) {
        if (n >= 28 || z < 1.6f || z > 64.f) return;
        float sx, sy, fog, scale;
        project(z, lat, sx, sy, fog, scale);
        list[n++] = {z, sx, sy, h * scale, fog, kind};
    };

    for (int i = 0; i < kRockN; i++) push(float(kRocks[i].s - showS), kRocks[i].lat, 26.f, 1);
    for (int i = 0; i < 16; i++) {
        double ps = std::floor(showS / 28.0) * 28.0 + i * 28.0;
        push(float(ps - showS), -1.45, (i % 3 == 0) ? 40.f : 32.f, 2);
    }
    push(float(kFinish - showS), 0.f, 48.f, 3);

    std::sort(list, list + n, [](const Spr& a, const Spr& b) { return a.z > b.z; });

    float psx, psy, pfog, pscale;
    project(2.6, showLat, psx, psy, pfog, pscale);
    spr(art_.cab, psx, psy + 8.f, 58.f, PAL_CAB, 0);
    spr(art_.puller, psx - 6.f, psy - 6.f, 34.f, PAL_CAB, 0);
    if (title) spr(art_.banner, 160.f, 52.f, 28.f, PAL_STORM, 0);

    for (int i = 0; i < n; i++) {
        const Spr& p = list[i];
        if (p.kind == 1) spr(art_.rock, p.sx, p.sy, p.h, PAL_ROCK, int(p.fog));
        else if (p.kind == 2) spr(art_.pine, p.sx, p.sy, p.h, PAL_PINE, int(p.fog));
        else spr(art_.gate, p.sx, p.sy, p.h, PAL_GATE, int(p.fog));
    }

    char buf[48];
    if (title) {
        hudC(16, "CLEAR THE PASS", PAL_HUD);
        hudC(18, "BEFORE THE STORM CLOCK", PAL_STORM);
        hudC(22, "A START    ARROWS STEER", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(22, "THE PASS IS CLOSED", PAL_STORM);
        hudC(24, why_, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(22, "PASS CLEAR", PAL_HUD);
        hudC(24, "AHEAD OF THE STORM", PAL_STORM);
    } else {
        hudC(24, "KEEP TO THE ROAD", PAL_HUD);
    }
    hud(1, 0, "S3 RICKSHAW PASS", PAL_HUD);
    if (!title) {
        std::snprintf(buf, sizeof buf, "%d M", int(std::max(0.0, std::min(kFinish, s_))));
        hud(1, 26, buf, PAL_HUD);
        int left = int(std::ceil(std::max(0.0, kClock - time_)));
        std::snprintf(buf, sizeof buf, "STORM %d", left);
        hud(28, 26, buf, left < 10 ? PAL_STORM : PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.silence();
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
    showTitle();
    if (bot_) startRun();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float pedal, brake, steer;
            pilot(pedal, brake, steer);
            stepRun(pedal, brake, steer);
            if (v_ > 3.f) {
                float tick = std::fmod(float(s_), 6.f);
                if (tick < v_ * kDt) sys.apu.tone(0, 90.f + v_ * 6.f, 0.03f);
            }
            if (wind_ != 0.f) sys.apu.noise(0.04f + std::fabs(wind_) * 0.05f, 900.f, false);
            else sys.apu.noise(0, 0, false);
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        showTitle();
    }

    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        int step = int(chimeT_ / 0.14f);
        if (step != chime_ && step < 4) {
            chime_ = step;
            sys.apu.tone(1, notes[step], 0.18f);
        }
        if (step >= 6) {
            sys.apu.tone(1, 0, 0);
            chime_ = -1;
        }
    }
    draw();
}

}  // namespace pass
