#include "combine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "../version.h"

namespace combine {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kHorizon = 46.f;
constexpr float kFocal = 980.f;
constexpr float kHeaderLead = 14.f;
constexpr float kHeaderHW = 1.14f;
constexpr float kLength = 400.f;
constexpr float kGrace = 16.f;
constexpr float kFull = 88.f;
constexpr float kHold = 0.94f;
constexpr float kBandLo = 7.3f;
constexpr float kBandHi = 9.7f;
constexpr float kCruise = 8.45f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float pinch(float s, float c, float rad) {
    float x = std::fabs(s - c);
    if (x >= rad) return 0.f;
    float n = 1.f - x / rad;
    return n * n;
}

float swathCenter(float s) {
    float ease = std::clamp(s / 36.f, 0.f, 1.f);
    ease *= ease;
    float wave = 0.85f * std::sin(s * 0.016f) + 0.42f * std::sin(s * 0.038f + 0.7f) + 0.22f * std::sin(s * 0.11f + 1.2f);
    float jt = std::clamp((s - 160.f) / 28.f, 0.f, 1.f);
    jt = jt * jt * (3.f - 2.f * jt);
    return ease * wave + 0.38f * jt;
}

float swathHalf(float s) {
    float w = 1.72f;
    w -= 0.22f * pinch(s, 108.f, 30.f);
    w -= 0.30f * pinch(s, 210.f, 34.f);
    w -= 0.24f * pinch(s, 312.f, 28.f);
    if (s < 36.f) w += (36.f - std::max(s, 0.f)) / 36.f * 0.5f;
    if (s > kLength - 48.f) w += std::clamp((s - (kLength - 48.f)) / 48.f, 0.f, 1.f) * 0.65f;
    return w;
}

bool inSwath(float s, float x) {
    float c = swathCenter(s);
    float h = swathHalf(s);
    return x >= c - h && x <= c + h;
}

float coverageAt(float s, float x) {
    float left = x - kHeaderHW;
    float right = x + kHeaderHW;
    float wL = swathCenter(s) - swathHalf(s);
    float wR = swathCenter(s) + swathHalf(s);
    float overlap = std::max(0.f, std::min(right, wR) - std::max(left, wL));
    return overlap / (kHeaderHW * 2.f);
}

float speedMatch(float sp) {
    if (sp >= kBandLo && sp <= kBandHi) return 1.f;
    if (sp < kBandLo) return std::clamp((sp - 4.5f) / (kBandLo - 4.5f), 0.f, 1.f);
    return std::clamp(1.f - (sp - kBandHi) / 4.2f, 0.f, 1.f);
}

gs::FMPatch diesel() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.62f;
    p.op[0] = {0.5f, 0.9f, 0.04f, 0.45f, 0.85f, 0.35f};
    p.op[1] = {1.0f, 1.0f, 0.03f, 0.5f, 0.75f, 0.3f};
    p.op[2] = {2.0f, 0.32f, 0.02f, 0.4f, 0.45f, 0.22f};
    p.op[3] = {0.5f, 0.4f, 0.02f, 0.35f, 0.6f, 0.28f};
    p.vol = 0.16f;
    p.drive = 0.5f;
    p.tone = 680;
    p.vibRate = 5.5f;
    p.vibDepth = 0.01f;
    return p;
}

gs::FMPatch horn() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.28f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.7f, 0.12f};
    p.op[1] = {2, 0.55f, 0.01f, 0.2f, 0.5f, 0.12f};
    p.op[2] = {3, 0.3f, 0.02f, 0.22f, 0.4f, 0.14f};
    p.op[3] = {1, 0.35f, 0.01f, 0.18f, 0.6f, 0.12f};
    p.vol = 0.2f;
    p.drive = 0.12f;
    return p;
}

}  // namespace

float Game::rnd() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0f / 16777216.0f);
}

float Game::botSteer() const {
    float err = swathCenter(s_ + 6.5f) - x_;
    float slope = (swathCenter(s_ + 5.5f) - swathCenter(s_ + 1.5f)) / 4.f;
    float wantV = slope * speed_;
    return std::clamp(err * 2.6f + (wantV - latV_) * 0.85f, -1.f, 1.f);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    t_ = 0;
    over_ = false;
    won_ = false;
    header_ = 100;
    supply_ = 1;
    view_ = 40.f;
    x_ = swathCenter(view_);
    fanStep_ = -1;
    shake_ = 0;
}

void Game::beginDrive() {
    mode_ = Mode::Drive;
    t_ = 0;
    s_ = 0;
    x_ = swathCenter(0);
    latV_ = 0;
    cruise_ = kCruise;
    speed_ = kCruise;
    header_ = 100;
    supply_ = 1;
    view_ = 0;
    over_ = false;
    won_ = false;
    shake_ = 0;
    fanStep_ = -1;
    beep_ = 0;
    for (auto& m : motes_) m.life = 0;
}

void Game::win() {
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->rumble(0.25f, 0.5f, 160);
    sys_->setLight(30, 180, 50);
}

void Game::lose() {
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    shake_ = 0.5f;
    fanStep_ = -1;
    sys_->apu.noiseBurst(0.55f, 160.f, 0.45f);
    sys_->rumble(0.85f, 0.35f, 240);
    sys_->setLight(210, 28, 16);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(13, 10, 6));
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.14f, 0.25f, 0.12f);
    sys.apu.setPatch(0, diesel());
    sys.apu.setPatch(1, horn());
    sys.apu.keyOn(0, 36.f, 0.1f);
    sys.apu.tone(0, 0, 0);

    props_.clear();
    for (int i = 1; i < 20; i++) {
        float ps = 18.f + i * 20.f;
        if (ps > kLength - 8.f) break;
        float side = (i & 1) ? 1.f : -1.f;
        float wob = float((i * 17) % 5) * 0.08f;
        int kind = (i % 6 == 0) ? 1 : 0;
        float px = swathCenter(ps) + side * (swathHalf(ps) + 1.55f + wob);
        props_.push_back({ps, px, kind});
    }
    props_.push_back({kLength + 16.f, swathCenter(kLength) + 3.4f, 2});
    props_.push_back({kLength + 18.f, swathCenter(kLength) - 3.8f, 0});

    if (bot_) beginDrive();
    else toTitle();
}

void Game::updateDrive(float dt) {
    const gs::Pad& pad = sys_->pad;
    float steer;
    if (bot_) {
        steer = botSteer();
    } else {
        float axis = pad.axisX;
        float digital = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        steer = std::fabs(axis) > 0.12f ? axis : digital;
        float nudge = 0;
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Y) || pad.accel > 0.25f) nudge += 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) || pad.brake > 0.25f) nudge -= 1.f;
        cruise_ = std::clamp(cruise_ + nudge * 3.2f * dt, 5.2f, 13.5f);
    }
    latV_ += steer * 8.f * dt;
    latV_ *= std::exp(-2.4f * dt);
    latV_ = std::clamp(latV_, -2.8f, 2.8f);
    x_ += latV_ * dt;
    speed_ += (cruise_ - speed_) * std::min(1.f, dt * 2.4f);
    if (speed_ < 0.f) speed_ = 0.f;
    s_ += speed_ * dt;
    view_ = s_;

    supply_ = coverageAt(s_, x_) * speedMatch(speed_);
    if (s_ < kGrace) {
        header_ = 100.f;
    } else if (supply_ >= kHold) {
        header_ = std::min(100.f, header_ + 24.f * dt);
    } else {
        float d = kHold - supply_;
        header_ -= (3.5f + d * 14.f) * dt;
    }
    header_ = std::clamp(header_, 0.f, 100.f);

    if (s_ >= kGrace && header_ < kFull) lose();
    else if (s_ >= kLength) win();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    if (beep_ > 0) beep_ -= DT;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        view_ = 40.f;
        x_ = swathCenter(view_);
        header_ = 100;
        supply_ = 1;
        if (pad.pressed(gs::BTN_START)) beginDrive();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Drive) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            toTitle();
        } else {
            updateDrive(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Drive;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else {
        if (!bot_ && pad.pressed(gs::BTN_START)) beginDrive();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) toTitle();
    }

    for (auto& m : motes_) {
        if (m.life <= 0) continue;
        m.life -= DT;
        m.vy += 36.f * DT;
        m.x += m.vx * DT;
        m.y += m.vy * DT;
    }

    if (fanStep_ >= 0) {
        fanT_ -= DT;
        if (fanT_ <= 0) {
            static const float notes[] = {392.f, 523.25f, 659.25f, 784.f, 1046.5f};
            if (fanStep_ >= 5) {
                fanStep_ = -1;
                sys.apu.keyOff(1);
            } else {
                sys.apu.keyOn(1, notes[fanStep_], 0.2f);
                fanT_ = 0.13f;
                fanStep_++;
            }
        }
    }

    sound();
    draw();
}

void Game::sound() {
    float hz = 32.f + speed_ * 2.4f;
    float vol = 0.1f;
    if (mode_ == Mode::Drive || mode_ == Mode::Pause) {
        hz = 34.f + speed_ * 3.1f;
        vol = mode_ == Mode::Pause ? 0.08f : 0.16f;
    } else if (mode_ == Mode::Lost) {
        hz = 28.f;
        vol = 0.06f;
    } else if (mode_ == Mode::Won) {
        vol = 0.11f;
    }
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, vol);
    float hiss = 0.f;
    if (mode_ == Mode::Drive) hiss = 0.025f + supply_ * 0.07f;
    else if (mode_ == Mode::Title) hiss = 0.03f;
    sys_->apu.noise(hiss, 700.f + speed_ * 40.f, false);
    if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    if (mode_ == Mode::Drive && header_ < 94.f && header_ >= kFull && beep_ <= 0) {
        sys_->apu.tone(0, 860.f, 0.05f);
        beep_ = 0.32f;
        sys_->setLight(220, 150, 24);
    } else if (mode_ == Mode::Drive && header_ >= 94.f) {
        sys_->setLight(24, 150, 40);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + (shadow ? 0.f : shakePx_)));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 120 || s.x + s.w < -120) return;
    if (s.y > gs::SCREEN_H + 40 || s.y + s.h < -30) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    shakePx_ = shake_ > 0 ? std::sin(t_ * 92.f) * shake_ * 16.f : 0.f;

    const float ppmC = kFocal / kHeaderLead;
    const float cutterY = kHorizon + ppmC;
    const float headerPx = kHeaderHW * 2.f * ppmC;
    const uint16_t skyTop = gs::rgb4(4, 8, 13);
    const uint16_t skyMid = gs::rgb4(11, 13, 15);
    const uint16_t skyHor = gs::rgb4(15, 11, 7);
    const uint16_t soil = gs::rgb4(8, 7, 4);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            v.lineBackdrop[y] = u < 0.62f ? lerpC(skyTop, skyMid, u / 0.62f) : lerpC(skyMid, skyHor, (u - 0.62f) / 0.38f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float dy = float(y) - kHorizon;
        float z = kFocal / dy;
        float ws = view_ - kHeaderLead + z;
        float half = swathHalf(ws);
        float along = ws - view_;
        if (along < 0.f) {
            float u = std::clamp(1.f + along / 3.4f, 0.f, 1.f);
            half *= u * u;
        }
        float lat = swathCenter(ws) - x_;
        gs::RoadLine line;
        line.on = true;
        line.cx = 160.f + lat * dy + shakePx_;
        line.hw = std::max(0.4f, half * dy);
        line.v = ws * 22.f;
        line.pal = PAL_FIELD;
        line.band = (int(std::floor(ws / 1.7f)) & 1) ? 1 : 0;
        line.style = 0;
        line.left = 0;
        line.right = 0;
        v.road[y] = line;
        float fogT = std::clamp(1.f - dy / 78.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * fogT * 12.f);
        v.lineBackdrop[y] = soil;
    }

    if ((mode_ == Mode::Drive && supply_ > 0.7f) || mode_ == Mode::Title) {
        if (rnd() < 0.55f) {
            for (auto& m : motes_) {
                if (m.life > 0) continue;
                float u = rnd() * 2.f - 1.f;
                m.x = 160.f + u * headerPx * 0.42f;
                m.y = cutterY - 8.f;
                m.vx = u * 14.f;
                m.vy = -18.f - rnd() * 10.f;
                m.life = 0.45f + rnd() * 0.25f;
                break;
            }
        }
    }
    for (const auto& m : motes_) {
        if (m.life <= 0) continue;
        spr(art_.chaff, m.x, m.y, 7.f, PAL_WHEAT, false);
    }

    for (int i = 0; i < 7; i++) {
        float u = (i - 3) / 3.f;
        float wx = x_ + u * kHeaderHW * 0.92f;
        bool on = inSwath(view_, wx);
        float sx = 160.f + u * kHeaderHW * 0.92f * ppmC;
        spr(on ? art_.lampOn : art_.lampOff, sx, cutterY - 20.f, 12.f, PAL_LAMP, false);
    }

    float phase = std::fmod((mode_ == Mode::Title ? t_ * 0.55f : s_ * 0.42f), 1.f);
    if (phase < 0) phase += 1.f;
    for (int i = 0; i < 8; i++) {
        float u = std::fmod(i / 8.f + phase, 1.f);
        float sx = 160.f - headerPx * 0.46f + u * headerPx * 0.92f;
        spr(art_.bat, sx, cutterY - 18.f, 11.f, PAL_RIG, false);
    }

    float goldW = headerPx * 0.8f * (header_ / 100.f);
    float goldH = goldW * float(art_.fill.h) / float(art_.fill.w);
    spr(art_.fill, 160.f, cutterY - 8.f, std::max(2.f, goldH), PAL_WHEAT, false);

    float headerH = headerPx * float(art_.header.h) / float(art_.header.w);
    spr(art_.header, 160.f, cutterY, headerH, PAL_RIG, false, 0, true);

    float bodyH = 112.f;
    spr(art_.body, 160.f, cutterY - 6.f + bodyH * 0.5f, bodyH, PAL_RIG, false);
    spr(art_.shadow, 160.f, 206.f, 26.f, PAL_RIG, false, 0, false, true);

    for (int row = 0; row < 2; row++) {
        float z = kHeaderLead + 0.85f + row * 1.45f;
        float ws = view_ - kHeaderLead + z;
        float ppm = kFocal / z;
        float ey = kHorizon + ppm;
        float half = swathHalf(ws);
        float center = swathCenter(ws);
        int fog = int(std::clamp((z - 22.f) * 0.18f, 0.f, 8.f));
        for (int i = -3; i <= 3; i++) {
            float wx = center + i * (half / 3.4f);
            if (!inSwath(ws, wx)) continue;
            float sx = 160.f + (wx - x_) * ppm;
            float sh = row == 0 ? 26.f : 18.f;
            bool flip = ((i + row) & 1) != 0;
            spr(art_.ear, sx, ey, sh, PAL_WHEAT, flip, fog);
        }
    }

    std::vector<const Prop*> vis;
    vis.reserve(props_.size());
    for (const auto& p : props_) {
        float z = p.s - (view_ - kHeaderLead);
        if (z > 36.f && z < 150.f) vis.push_back(&p);
    }
    std::sort(vis.begin(), vis.end(), [&](const Prop* a, const Prop* b) {
        return a->s < b->s;
    });
    for (const Prop* p : vis) {
        float z = p->s - (view_ - kHeaderLead);
        float ppm = kFocal / z;
        float gy = kHorizon + ppm;
        float sx = 160.f + (p->x - x_) * ppm;
        int fog = int(std::clamp((z - 36.f) * 0.1f, 0.f, 12.f));
        if (p->kind == 2) {
            spr(art_.leg, sx, gy, std::min(96.f, 7.5f * ppm), PAL_YARD, false, fog, true);
        } else if (p->kind == 1) {
            spr(art_.bale, sx, gy, std::min(20.f, 1.15f * ppm), PAL_WHEAT, false, fog, true);
        } else {
            spr(art_.tree, sx, gy, std::min(64.f, 5.2f * ppm), PAL_TREE, false, fog, true);
        }
    }

    float sunX = 286.f + std::sin(t_ * 0.15f) * 2.f;
    spr(art_.sun, sunX, 22.f, 30.f, PAL_SUN, false);
    spr(art_.cloud, std::fmod(30.f + t_ * 3.f, 380.f) - 20.f, 18.f, 22.f, PAL_CLOUD, false);
    spr(art_.cloud, std::fmod(210.f + t_ * 2.2f, 420.f) - 40.f, 30.f, 16.f, PAL_CLOUD, true);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(1, "S3 COMBINE", PAL_AMBER);
        hudC(3, "ONE PASS", PAL_AMBER);
        hudC(4, "THE HEADER STAYS FULL", PAL_HUD);
        hudC(5, "KEEP EVERY LAMP GREEN", PAL_GREEN);
        hudC(25, "ARROWS STEER   C FAST   X SLOW", PAL_HUD);
        if (int(t_ * 2.f) % 2 == 0) hudC(26, "PRESS START", PAL_GREEN);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Drive || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "%d M", int(s_));
        hud(1, 1, "S3 COMBINE", PAL_AMBER);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "HEADER %3.0f  %s", header_, header_ >= 94.f ? "FULL" : "HOLD");
        hud(1, 2, buf, header_ >= 94.f ? PAL_GREEN : PAL_AMBER);
        int kmh = int(std::lround(speed_ * 3.6f));
        const char* band = "IN BAND";
        int spal = PAL_GREEN;
        if (speed_ < kBandLo) {
            band = "TOO SLOW";
            spal = PAL_RED;
        } else if (speed_ > kBandHi) {
            band = "TOO FAST";
            spal = PAL_RED;
        }
        std::snprintf(buf, sizeof buf, "SPEED %d KM/H  %s", kmh, band);
        hudC(26, buf, spal);
        if (s_ > kLength - 48.f && mode_ == Mode::Drive) hudC(24, "HEADLAND", PAL_AMBER);
        if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Won) {
        hudC(1, "THE HEADER STAYED FULL", PAL_GREEN);
        hudC(3, "ONE PASS", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "%d M", int(kLength));
        hudC(4, buf, PAL_HUD);
        hudC(26, "START TO RUN IT AGAIN", PAL_HUD);
    } else {
        hudC(1, "THE HEADER RAN THIN", PAL_RED);
        std::snprintf(buf, sizeof buf, "%d M", int(s_));
        hudC(3, buf, PAL_HUD);
        hudC(4, "ONE PASS", PAL_AMBER);
        hudC(26, "START TO TRY AGAIN", PAL_HUD);
    }
}

}  // namespace combine
