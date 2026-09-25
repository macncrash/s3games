#include "game/trap.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace trap {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float G = 9.81f;
constexpr float DEG = 0.017453292f;
constexpr float FOCAL = 780.0f;
constexpr float THRUST = 16.0f;

float kt(float mps) { return mps * 1.943844f; }
float feet(float m) { return m * 3.28084f; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch jetPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.55f;
    p.op[0] = {1, 0.7f, 0.02f, 0.3f, 1, 0.2f};
    p.op[1] = {2, 0.9f, 0.01f, 0.25f, 0.8f, 0.2f};
    p.op[2] = {3, 0.35f, 0.02f, 0.3f, 0.5f, 0.2f, 6};
    p.op[3] = {1, 0.4f, 0.02f, 0.4f, 0.6f, 0.25f};
    p.vol = 0.12f;
    p.drive = 0.2f;
    p.tone = 2400;
    return p;
}

gs::FMPatch brassPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.3f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.7f, 0.12f};
    p.op[1] = {2, 0.55f, 0.01f, 0.18f, 0.5f, 0.12f};
    p.op[2] = {3, 0.3f, 0.02f, 0.2f, 0.4f, 0.12f};
    p.op[3] = {1, 0.35f, 0.01f, 0.18f, 0.5f, 0.12f};
    p.vol = 0.2f;
    return p;
}

const char* gradeName(Grade g) {
    switch (g) {
        case Grade::Under: return "_OK_";
        case Grade::Ok: return "OK";
        case Grade::Fair: return "FAIR";
        case Grade::Bolter: return "BOLTER";
        case Grade::Waveoff: return "WAVE OFF";
        case Grade::Ramp: return "RAMP STRIKE";
        case Grade::Edge: return "OFF THE EDGE";
        case Grade::Crash: return "CRASH";
        default: return "";
    }
}

}  // namespace

int Game::marker() const {
    if (over_) return 4;
    if (mode_ == Mode::Grade) return 3;
    if (mode_ == Mode::Fly && z_ < 280.f) return 2;
    if (mode_ == Mode::Fly) return 1;
    return 0;
}

bool Game::passGrade(Grade g) const { return g == Grade::Under || g == Grade::Ok || g == Grade::Fair; }

void Game::say(const char* line) {
    if (lsoT_ > 0.4f && lso_ == line) return;
    lso_ = line;
    lsoT_ = 1.3f;
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 90 || s.x + s.w < -90 || s.y > gs::SCREEN_H + 50 || s.y + s.h < -50) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::loadProgress() {
    unlocked_ = 1;
    if (!sys_ || sys_->headless) return;
    std::string s = sys_->loadBlob("trap.txt");
    if (s.rfind("mask=", 0) == 0) unlocked_ = std::max(1, std::atoi(s.c_str() + 5)) | 1;
}

void Game::saveProgress() {
    if (!sys_ || sys_->headless || bot_) return;
    sys_->saveBlob("trap.txt", "mask=" + std::to_string(unlocked_) + "\n");
}

void Game::finish(Grade g, int wire) {
    if (mode_ != Mode::Fly) return;
    grade_ = g;
    wire_ = wire;
    mode_ = Mode::Grade;
    t_ = 0;
    bool ok = passGrade(g);
    if (ok) {
        passes_++;
        if (site_ == 0) boatOk_ = true;
        if (site_ + 1 < NUM_SITES) unlocked_ |= 1 << (site_ + 1);
        saveProgress();
        sys_->rumble(ok ? 0.4f : 0.2f, 0.7f, 280);
        sys_->apu.noiseBurst(0.45f, g == Grade::Under || g == Grade::Ok ? 700.0f : 400.0f, 0.25f);
        sys_->apu.keyOn(1, 523, 0.18f);
    } else {
        sys_->rumble(0.9f, 0.9f, 400);
        sys_->apu.noiseBurst(0.6f, 180.0f, 0.4f);
    }
    if (bot_) {
        const char* wireS = wire ? (wire == 1 ? "1" : wire == 2 ? "2" : wire == 3 ? "3" : "4") : "-";
        std::printf("%-14s %-12s wire %s  x %5.1f  ias %3.0f  aoa %4.1f  sink %4.0f\n", site().name, gradeName(g), wireS, x_,
                    kt(vz_), (pitch_ - std::atan2(vy_, std::max(5.0f, vz_))) / DEG, feet(vy_) * 60.0f);
    }
}

void Game::resetPass() {
    const Site& s = site();
    float slope = std::tan(s.slopeDeg * DEG);
    float closing = std::max(12.0f, s.vRef - s.padSpeed);
    z_ = s.startZ;
    x_ = s.startX;
    y_ = z_ * slope + s.startHigh;
    vz_ = s.vRef;
    vy_ = -closing * slope;
    vx_ = 0;
    pitch_ = s.aoaRef * DEG + std::atan2(vy_, vz_);
    roll_ = 0;
    throttle_ = 0.70f;
    brake_ = 0;
    prevAoa_ = s.aoaRef * DEG;
    waved_ = false;
    bolter_ = false;
    onDeck_ = false;
    grade_ = Grade::None;
    wire_ = 0;
    flyT_ = 0;
    log_ = 1.5f;
    lso_.clear();
    lsoT_ = 0;
    shake_ = 0;
    rng_ = 0xA31u * uint32_t(site_ + 1);
    applyTheme(sys_->vdp, s);
    mode_ = Mode::Fly;
}

void Game::step(float dt) {
    const Site& s = site();
    const float slope = std::tan(s.slopeDeg * DEG);
    const float aoaRef = s.aoaRef * DEG;
    float gamma = std::atan2(vy_, std::max(8.0f, vz_));
    float aoa = pitch_ - gamma;

    float sx = 0, sy = 0;
    bool wantBrake = false;
    if (bot_) {
        float yIdeal = std::max(0.0f, z_) * slope;
        // The ledge starts almost on the aimpoint. A path a few meters high
        // still meets dirt; the raw slope meets the cliff when the ball dips.
        if (s.theme == CANYON) yIdeal += 4.0f;
        float closing = std::max(10.0f, vz_ - s.padSpeed);
        float vyIdeal = -closing * slope;
        float sway = std::sin(flyT_ * 6.2831853f / std::max(0.5f, s.period)) * s.swayAmp;
        if (onDeck_ && !s.wires) {
            throttle_ = std::max(0.0f, throttle_ - dt * 1.4f);
            wantBrake = true;
            sy = std::clamp(-pitch_ * 2.0f, -1.0f, 1.0f);
            sx = std::clamp(-(x_ - sway) * 0.08f - vx_ * 0.2f, -1.0f, 1.0f);
        } else if (waved_ || bolter_) {
            throttle_ = 1.0f;
            sy = 0.85f;
            sx = std::clamp(-(x_ - sway) * 0.05f, -1.0f, 1.0f);
        } else {
            float aoaErr = aoa - aoaRef;
            float dAoa = (aoa - prevAoa_) / dt;
            sy = std::clamp(-aoaErr * 6.5f - dAoa * 0.12f, -1.0f, 1.0f);
            float yErr = y_ - yIdeal;
            float target = 0.70f - yErr * 0.018f - (vy_ - vyIdeal) * 0.10f;
            // A hole in close is a ramp strike. A float in close misses the wires.
            if (z_ < 450.0f && yErr < -0.6f) target += (-yErr) * 0.06f;
            if (s.theme != CANYON && z_ < 220.0f && yErr > 1.2f) target -= (yErr - 1.2f) * 0.03f;
            throttle_ += (std::clamp(target, 0.12f, 1.0f) - throttle_) * std::min(1.0f, dt * 4.0f);
            float line = s.halfW < 14.0f ? 0.08f : 0.045f;
            sx = std::clamp(-(x_ - sway) * line - vx_ * 0.24f, -1.0f, 1.0f);
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        sx = std::fabs(pad.axisX) > 0.08f ? pad.axisX : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        sy = std::fabs(pad.axisY) > 0.08f ? pad.axisY : float(pad.down(gs::BTN_UP)) - float(pad.down(gs::BTN_DOWN));
        if (pad.accel > 0.03f || pad.brake > 0.03f) {
            throttle_ += (pad.accel - throttle_) * std::min(1.0f, dt * 5.0f);
            wantBrake = pad.brake > 0.28f || pad.down(gs::BTN_TURBO);
        } else {
            if (pad.down(gs::BTN_C)) throttle_ += 0.48f * dt;
            if (pad.down(gs::BTN_B)) throttle_ -= 0.48f * dt;
            wantBrake = pad.down(gs::BTN_TURBO);
        }
    }
    throttle_ = std::clamp(throttle_, 0.0f, 1.0f);
    brake_ = wantBrake ? 1.0f : std::max(0.0f, brake_ - dt * 3.0f);
    prevAoa_ = aoa;

    pitch_ += sy * 0.62f * dt;
    roll_ += sx * 1.05f * dt;
    roll_ += -roll_ * 1.45f * dt;
    pitch_ = std::clamp(pitch_, -0.28f, 0.48f);
    roll_ = std::clamp(roll_, -0.75f, 0.75f);

    float gust = (std::sin(flyT_ * 1.7f) + 0.45f * std::sin(flyT_ * 3.1f)) * s.gust;
    if (!onDeck_) {
        float q = (vz_ * vz_) / (s.vRef * s.vRef);
        float cd = 0.22f + 2.4f * aoa * aoa + brake_ * 0.5f;
        float cdRef = 0.22f + 2.4f * aoaRef * aoaRef;
        float drag = THRUST * 0.70f * q * (cd / std::max(0.05f, cdRef));
        float lift = G * q * (aoa / std::max(0.05f, aoaRef));
        vz_ += (throttle_ * THRUST - drag) * dt;
        vy_ += (lift * std::cos(roll_) - G + gust) * dt;
        vx_ += lift * std::sin(roll_) * dt;
        vx_ += (s.wind - vx_) * 0.32f * dt;
        vz_ = std::clamp(vz_, 25.0f, s.vRef * 1.45f);
    } else {
        float decel = (7.0f + 16.0f * brake_ + 6.0f * (1.0f - throttle_)) * s.friction;
        float rel = vz_ - s.padSpeed;
        if (s.wires) rel *= std::exp(-2.8f * dt);
        else if (rel > 0) rel = std::max(0.0f, rel - decel * dt);
        else rel = std::min(0.0f, rel + decel * dt);
        vz_ = s.padSpeed + rel;
        vy_ = 0;
        vx_ += sx * 6.0f * dt;
        vx_ *= (1.0f - 1.8f * dt);
        float heave = std::sin(flyT_ * 6.2831853f / std::max(0.5f, s.period)) * s.pitchAmp;
        y_ = heave + 0.45f;
    }

    x_ += vx_ * dt;
    if (!onDeck_) y_ += vy_ * dt;
    float closing = vz_ - s.padSpeed;
    if (!(onDeck_ && s.wires)) z_ -= closing * dt;

    float sway = std::sin(flyT_ * 6.2831853f / std::max(0.5f, s.period) * 0.85f) * s.swayAmp;
    float heaveNow = std::sin(flyT_ * 6.2831853f / std::max(0.5f, s.period)) * s.pitchAmp *
                     (0.3f + 0.7f * std::clamp(z_ / std::max(1.0f, s.deckAft), 0.0f, 1.0f));
    float agl = y_ - heaveNow;
    float yIdeal = std::max(0.0f, z_) * slope;
    float aoaDeg = aoa / DEG;

    if (!onDeck_ && mode_ == Mode::Fly) {
        if (z_ < 900.0f && z_ > s.deckAft && lsoT_ <= 0) {
            if (agl < yIdeal - 9.0f) say("POWER");
            else if (agl > yIdeal + 12.0f) say("EASY WITH IT");
            else if (x_ - sway > 7.0f) say("COME LEFT");
            else if (x_ - sway < -7.0f) say("RIGHT FOR LINEUP");
            else if (aoaDeg > s.aoaRef + 1.6f) say("YOU'RE SLOW");
            else if (aoaDeg < s.aoaRef - 1.6f) say("YOU'RE FAST");
        }
        if (!waved_ && z_ < 260.0f && z_ > s.deckAft && (agl < yIdeal - 18.0f || std::fabs(x_ - sway) > s.halfW + 12.0f)) {
            waved_ = true;
            say("WAVE OFF");
            sys_->apu.tone(0, 180, 0.12f);
        }
        if (waved_ && agl > yIdeal + 22.0f && vy_ > 1.2f) {
            finish(Grade::Waveoff, 0);
            return;
        }
        if (bolter_ && y_ > 18.0f && vy_ > 1.0f) {
            finish(Grade::Bolter, 0);
            return;
        }
        if (z_ < -s.deckFwd - 40.0f) {
            finish(y_ > 12.0f && vy_ > -1.0f ? Grade::Bolter : Grade::Crash, 0);
            return;
        }
        if (agl < 0.75f && vy_ < -0.25f && z_ < s.deckAft + 40.0f) {
            if (bot_) std::printf("  contact z %5.0f  x %5.1f  vy %5.2f  aoa %4.1f\n", z_, x_ - sway, vy_, aoaDeg);
            if (z_ > s.deckAft + 1.5f) {
                finish(Grade::Ramp, 0);
                return;
            }
            if (std::fabs(x_ - sway) > s.halfW + 2.0f) {
                finish(Grade::Crash, 0);
                return;
            }
            if (vy_ < -8.6f) {
                finish(Grade::Crash, 0);
                return;
            }
            onDeck_ = true;
            shake_ = 0.35f;
            int caught = 0;
            if (s.wires) {
                float wires[4];
                int n = s.wireCount >= 4 ? 4 : 1;
                if (n == 4) {
                    wires[0] = 24;
                    wires[1] = 12;
                    wires[2] = 0;
                    wires[3] = -12;
                } else {
                    wires[0] = 0;
                    n = 1;
                }
                float best = 8.2f;
                for (int i = 0; i < n; i++) {
                    float d = std::fabs(z_ - wires[i]);
                    if (d < best) {
                        best = d;
                        caught = i + 1;
                    }
                }
                bool hook = aoaDeg > s.aoaRef - 3.5f && vz_ < s.vRef * 1.32f && std::fabs(x_ - sway) < s.halfW - 0.4f;
                if (caught && hook) {
                    Grade g = Grade::Fair;
                    bool centered = std::fabs(x_ - sway) < 3.0f;
                    bool onSpeed = std::fabs(aoaDeg - s.aoaRef) < 1.25f;
                    bool sinkOk = vy_ < -2.0f && vy_ > -6.4f;
                    if ((n == 1 || caught == 3) && centered && onSpeed && sinkOk) g = Grade::Under;
                    else if (centered && std::fabs(aoaDeg - s.aoaRef) < 2.6f && vy_ > -7.2f) g = Grade::Ok;
                    finish(g, n == 1 ? 3 : caught);
                    onDeck_ = true;
                    return;
                }
                say("BOLTER");
                bolter_ = true;
                onDeck_ = false;
                y_ = heaveNow + 1.4f;
                if (vy_ < 0) vy_ = 0.2f;
                throttle_ = 1.0f;
            }
        }
    } else if (onDeck_ && mode_ == Mode::Fly) {
        if (!s.wires) {
            float rel = std::fabs(vz_ - s.padSpeed);
            if (rel < 9.0f && z_ > -s.deckFwd) {
                float remain = z_ + s.deckFwd;
                float len = s.deckFwd + s.deckAft;
                Grade g = (remain > len * 0.2f && std::fabs(x_ - sway) < s.halfW * 0.55f) ? Grade::Ok : Grade::Fair;
                finish(g, 0);
                return;
            }
            if (z_ < -s.deckFwd) {
                finish(Grade::Edge, 0);
                return;
            }
        } else if (z_ < -s.deckFwd - 10.0f) {
            finish(Grade::Bolter, 0);
            return;
        }
    }

    if (flyT_ > 110.0f && mode_ == Mode::Fly) finish(Grade::Crash, 0);

    log_ -= dt;
    if (bot_ && log_ <= 0 && mode_ == Mode::Fly) {
        log_ = 5.0f;
        std::printf("%-14s z %5.0f  alt %4.0f  ideal %4.0f  ias %3.0f  aoa %4.1f  x %5.1f  thr %2.0f\n", s.name, z_, feet(agl),
                    feet(yIdeal), kt(vz_), aoaDeg, x_ - sway, throttle_ * 100.0f);
    }
}

void Game::banners() {
    if (mode_ == Mode::Title) text("S3 TRAP", 160, 58, 1.35f, PAL_AMBER);
    else if (mode_ == Mode::Brief) text(site().name, 160, 48, 0.9f, PAL_AMBER);
    else if (mode_ == Mode::Grade || mode_ == Mode::Victory) {
        char buf[48];
        if (mode_ == Mode::Victory) text("WINGS", 160, 46, 1.3f, PAL_GREEN);
        else {
            std::snprintf(buf, sizeof buf, "%s", gradeName(grade_));
            text(buf, 160, 46, 1.05f, passGrade(grade_) ? PAL_GREEN : PAL_RED);
            if (wire_ && passGrade(grade_)) {
                std::snprintf(buf, sizeof buf, "%d WIRE", wire_);
                text(buf, 160, 78, 0.8f, PAL_AMBER);
            }
        }
    } else if (mode_ == Mode::Pause) text("PAUSE", 160, 52, 1.2f, PAL_HUD);
    else if (mode_ == Mode::Help) text("THE BALL", 160, 34, 1.0f, PAL_AMBER);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    const Site& s = site();
    Sky sky = themeSky(s.theme);
    v.clearSprites();
    v.HUD.clear();
    banners();
    v.setFogColor(sky.fog);

    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(flyT_ * 90.0f) * 7.0f * shake_;
        shy = std::cos(flyT_ * 70.0f) * 4.0f * shake_;
        shake_ = std::max(0.0f, shake_ - DT);
    }
    float H0 = 104.0f + pitch_ * 57.3f * 2.3f + shy;
    H0 = std::clamp(H0, 34.0f, 168.0f);
    float camY = std::max(y_, 5.5f);
    float rollShift = std::tan(roll_) * 0.42f;

    for (int row = 0; row < gs::SCREEN_H; row++) {
        if (row < H0) {
            v.lineBackdrop[row] = lerpC(sky.top, sky.horizon, row / H0);
            v.lineFog[row] = uint8_t(row > H0 - 18 ? (18 - (H0 - row)) / 2 : 0);
            v.road[row].on = false;
            continue;
        }
        float scan = float(row) - H0 + 0.5f;
        float ahead = camY * FOCAL / scan;
        float deckZ = z_ - ahead;
        float sway = std::sin(flyT_ * 6.2831853f / std::max(0.5f, s.period) * 0.85f) * s.swayAmp;
        bool on = deckZ < s.deckAft && deckZ > -s.deckFwd;
        gs::RoadLine& r = v.road[row];
        r.on = true;
        r.cx = 160.0f - (x_ - sway) * FOCAL / ahead + scan * rollShift + shx;
        r.v = ahead + flyT_ * std::max(10.0f, vz_) * 0.15f;
        r.band = (int(std::floor(ahead / 22.0f)) & 1) ? 1 : 0;
        r.left = r.right = s.sea && !on ? 1 : 0;
        if (on) {
            r.hw = std::max(2.0f, s.halfW * FOCAL / ahead);
            r.style = s.hardDeck ? 1 : 0;
            r.pal = PAL_DECK;
            if (!s.sea) r.left = r.right = 0;
            else r.left = r.right = 1;
        } else if (s.sea) {
            r.hw = 420;
            r.style = 2;
            r.pal = PAL_DECK;
            r.left = r.right = 1;
        } else {
            r.hw = 420;
            r.style = 0;
            r.pal = PAL_SIDE;
        }
        int fog = std::clamp(int(16.0f - scan / 8.0f), 0, 14);
        v.lineFog[row] = uint8_t(fog);
        v.lineBackdrop[row] = sky.horizon;
    }

    auto project = [&](float wx, float wy, float ahead, float& sx, float& sy, float& sc, int& fog) {
        sc = FOCAL / std::max(2.0f, ahead);
        sy = H0 - (wy - camY) * sc + shy;
        sx = 160.0f + (wx - x_) * sc + (sy - H0) * rollShift + shx;
        fog = std::clamp(int((ahead - 60.0f) / 80.0f), 0, 14);
    };

    auto scenery = [&](const gs::Mipped& m, float wx, float pz, float worldH, int pal) {
        float ahead = z_ - pz;
        if (ahead < 3.0f || ahead > 1400.0f) return;
        float sx, sy, sc;
        int fog;
        project(wx, 0, ahead, sx, sy, sc, fog);
        spr(m, sx, sy, worldH * sc, pal, wx < 0, fog, true);
    };

    float sway = std::sin(flyT_ * 6.2831853f / std::max(0.5f, s.period) * 0.85f) * s.swayAmp;
    if (s.theme == SEA || s.theme == NIGHT) {
        scenery(art_.island, sway + s.halfW + 18.0f, -30.0f, 34.0f, PAL_SEA);
        scenery(art_.mast, sway - s.halfW + 3.0f, s.deckAft * 0.55f, 8.0f, PAL_SEA);
    } else if (s.theme == RIG) {
        scenery(art_.derrick, sway + s.halfW + 6.0f, -4.0f, 40.0f, PAL_SEA);
        scenery(art_.mast, sway - s.halfW + 1.5f, 6.0f, 7.0f, PAL_SEA);
    } else if (s.theme == CANYON || s.theme == ASH) {
        for (int i = 0; i < 5; i++) {
            scenery(art_.wall, -s.halfW - 8.0f, 40.0f + i * 70.0f, 30.0f, PAL_CITY);
            scenery(s.theme == ASH ? art_.spire : art_.wall, s.halfW + 8.0f, 10.0f + i * 70.0f, 28.0f, PAL_CITY);
        }
    } else if (s.theme == CITY) {
        for (int i = 0; i < 6; i++) {
            scenery(art_.building, -28.0f - (i % 2) * 6.0f, -40.0f + i * 55.0f, 22.0f + (i % 3) * 6.0f, PAL_CITY);
            scenery(art_.building, 26.0f, i * 55.0f, 18.0f + (i % 2) * 8.0f, PAL_CITY);
        }
    } else if (s.theme == ICE) {
        for (int i = 0; i < 4; i++) scenery(art_.ice, -30.0f + i * 18.0f, 30.0f + i * 80.0f, 6.0f, PAL_FX);
    } else if (s.theme == HIGHWAY && s.padSpeed > 1.0f) {
        for (int i = -2; i <= 3; i++) scenery(art_.boxcar, sway, i * 36.0f, 5.5f, PAL_CITY);
    } else {
        scenery(art_.bridge, 0, -s.deckFwd + 6.0f, 8.0f, PAL_SEA);
        for (int i = 0; i < 5; i++) scenery(art_.pole, s.halfW + 4.0f, i * 50.0f, 7.0f, PAL_SEA);
    }

    if (s.wires) {
        float wires[4] = {24, 12, 0, -12};
        int n = s.wireCount >= 4 ? 4 : 1;
        if (n == 1) wires[0] = 0;
        for (int i = 0; i < n; i++) {
            float ahead = z_ - wires[i];
            if (ahead < 4 || ahead > 500) continue;
            float sx, sy, sc;
            int fog;
            project(sway, 0.2f, ahead, sx, sy, sc, fog);
            spr(art_.wire, sx, sy, std::max(3.0f, s.halfW * 1.6f * sc * 0.15f), PAL_AMBER, false, fog);
        }
    }

    // The jet sits in the chase view. Bank frames plus a mirror for the other wing down.
    float bank = std::clamp(roll_ * 1.4f, -1.0f, 1.0f);
    int frame = std::fabs(bank) > 0.62f ? 2 : std::fabs(bank) > 0.28f ? 1 : 0;
    float jy = 178.0f + shy - pitch_ * 20.0f;
    spr(art_.jet[frame], 160 + shx, jy, 62, PAL_JET, bank < 0);
    if (throttle_ > 0.35f) {
        float fh = 10.0f + throttle_ * 16.0f;
        spr(art_.flame, 148 + shx, jy + 24, fh, PAL_FX, false);
        spr(art_.flame, 172 + shx, jy + 24, fh, PAL_FX, false);
    }

    if (mode_ == Mode::Fly || mode_ == Mode::Pause || mode_ == Mode::Grade) {
        float aoaDeg = (pitch_ - std::atan2(vy_, std::max(8.0f, vz_))) / DEG;
        int idx = aoaDeg > s.aoaRef + 1.05f ? -1 : aoaDeg < s.aoaRef - 1.05f ? 1 : 0;
        spr(art_.chev, 28, 78, idx == -1 ? 18 : 12, PAL_GREEN, false, idx == -1 ? 0 : 11);
        spr(art_.donut, 28, 98, idx == 0 ? 16 : 11, PAL_AMBER, false, idx == 0 ? 0 : 11);
        spr(art_.tri, 28, 118, idx == 1 ? 16 : 11, PAL_RED, false, idx == 1 ? 0 : 11);
    }

    char buf[64];
    if (mode_ == Mode::Fly || mode_ == Mode::Pause) {
        float aoaDeg = (pitch_ - std::atan2(vy_, std::max(8.0f, vz_))) / DEG;
        float agl = y_;
        std::snprintf(buf, sizeof buf, "IAS %3.0f", kt(vz_));
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "ALT %4.0f", std::max(0.0f, feet(agl)));
        hud(1, 2, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "VS %+5.0f", feet(vy_) * 60.0f);
        hud(1, 3, buf, vy_ < -7.5f ? PAL_RED : PAL_HUD);
        std::snprintf(buf, sizeof buf, "AOA %4.1f", aoaDeg);
        hud(30, 1, buf, std::fabs(aoaDeg - s.aoaRef) < 1.1f ? PAL_AMBER : PAL_HUD);
        std::snprintf(buf, sizeof buf, "RNG %.2f", std::max(0.0f, z_) / 1852.0f);
        hud(30, 2, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "PWR %2.0f", throttle_ * 100.0f);
        hud(30, 3, buf, PAL_HUD);
        if (!lso_.empty() && lsoT_ > 0) hudC(6, lso_, lso_ == "WAVE OFF" ? PAL_RED : PAL_AMBER);
        float ballErr = (y_ - std::max(0.0f, z_) * std::tan(s.slopeDeg * DEG)) / std::max(6.0f, z_ * 0.02f);
        int cell = std::clamp(int(std::lround(ballErr * 2.4f)), -3, 3);
        spr(art_.wire, 18, 148, 6, PAL_FX, false);
        spr(art_.ball, 18, 148 - cell * 6.0f, 8, PAL_AMBER, false);
        hud(0, 20, "BALL", PAL_HUD);
    }

    if (mode_ == Mode::Title) {
        hudC(10, "CALL THE BALL", PAL_HUD);
        if (int(t_ * 2) % 2 == 0) hudC(13, "PRESS START", PAL_AMBER);
        hudC(16, "STICK FLY   C POWER   X IDLE", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Menu) {
        const char* items[] = {"TOUR", "PRACTICE", "THE BALL"};
        for (int i = 0; i < 3; i++) {
            hud(13, 12 + i * 2, i == menu_ ? ">" : " ", PAL_AMBER);
            hud(15, 12 + i * 2, items[i], i == menu_ ? PAL_AMBER : PAL_HUD);
        }
    } else if (mode_ == Mode::Pick) {
        hudC(1, "PRACTICE", PAL_AMBER);
        for (int i = 0; i < NUM_SITES; i++) {
            bool open = unlocked_ & (1 << i);
            hud(4, 4 + i * 2, i == pick_ ? ">" : " ", PAL_AMBER);
            hud(6, 4 + i * 2, siteDef(i).name, !open ? PAL_RED : i == pick_ ? PAL_AMBER : PAL_HUD);
        }
    } else if (mode_ == Mode::Help) {
        const char* lines[] = {"PITCH HOLDS THE DONUT", "POWER HOLDS THE BALL", "ARROWS PITCH AND ROLL", "C POWER UP   X POWER DOWN",
                               "SPACE IS THE SPEEDBRAKE", "WIRES RUN 1 AFT TO 4 FWD", "3 WIRE ON SPEED IS THE GOAL"};
        for (int i = 0; i < 7; i++) hudC(6 + i * 2, lines[i], PAL_HUD);
        hudC(22, "START OR ESC", PAL_AMBER);
    } else if (mode_ == Mode::Brief) {
        hudC(10, site().place, PAL_HUD);
        hudC(13, site().blurb, PAL_AMBER);
        hudC(18, "ROGER BALL", PAL_HUD);
        if (int(t_ * 2) % 2 == 0) hudC(21, "START", PAL_AMBER);
    } else if (mode_ == Mode::Grade) {
        hudC(14, site().place, PAL_HUD);
        if (passGrade(grade_)) hudC(17, tour_ && site_ + 1 < NUM_SITES ? "START  NEXT" : "START", PAL_GREEN);
        else hudC(17, "START  RETRY", PAL_RED);
        hudC(19, "ESC  MENU", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "START  RESUME", PAL_HUD);
        hudC(14, "ESC    MENU", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        std::snprintf(buf, sizeof buf, "%d TRAPS", passes_);
        hudC(12, buf, PAL_HUD);
        hudC(16, "START", PAL_AMBER);
    }

    if (lsoT_ > 0) lsoT_ -= DT;
    if (mode_ == Mode::Fly || mode_ == Mode::Title || mode_ == Mode::Menu || mode_ == Mode::Brief) {
        float hz = 70.0f + throttle_ * 180.0f + vz_ * 0.4f;
        sys_->apu.setFreq(0, hz);
        sys_->apu.setVol(0, mode_ == Mode::Fly ? 0.11f + throttle_ * 0.06f : 0.05f);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.22f, 0.14f);
    sys.apu.setPatch(0, jetPatch());
    sys.apu.setPatch(1, brassPatch());
    sys.apu.keyOn(0, 140, 0.08f);
    loadProgress();
    if (bot_) {
        unlocked_ = (1 << NUM_SITES) - 1;
        site_ = 0;
        tour_ = true;
        passes_ = 0;
        boatOk_ = false;
        over_ = false;
        applyTheme(sys.vdp, site());
        mode_ = Mode::Brief;
        t_ = 0;
    } else {
        site_ = 0;
        applyTheme(sys.vdp, site());
        z_ = 780;
        y_ = z_ * std::tan(3.5f * DEG) + 10;
        x_ = 12;
        pitch_ = 0.08f;
        vz_ = 72;
        throttle_ = 0.7f;
        mode_ = Mode::Title;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        z_ = 820 + std::sin(t_ * 0.25f) * 30.0f;
        y_ = z_ * std::tan(3.5f * DEG) + 8;
        x_ = 14 + std::sin(t_ * 0.4f) * 4;
        pitch_ = 0.08f + std::sin(t_ * 0.3f) * 0.01f;
        flyT_ = t_;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Menu;
            menu_ = 0;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Menu && !bot_) {
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_DOWN)) menu_ = (menu_ + (pad.pressed(gs::BTN_DOWN) ? 1 : 2)) % 3;
        if (pad.pressed(gs::BTN_START)) {
            if (menu_ == 2) mode_ = Mode::Help;
            else if (menu_ == 1) {
                mode_ = Mode::Pick;
                pick_ = 0;
            } else {
                tour_ = true;
                site_ = 0;
                passes_ = 0;
                mode_ = Mode::Brief;
                t_ = 0;
                applyTheme(sys.vdp, site());
            }
        } else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Pick && !bot_) {
        if (pad.pressed(gs::BTN_UP)) pick_ = (pick_ + NUM_SITES - 1) % NUM_SITES;
        if (pad.pressed(gs::BTN_DOWN)) pick_ = (pick_ + 1) % NUM_SITES;
        if (pad.pressed(gs::BTN_START) && (unlocked_ & (1 << pick_))) {
            tour_ = false;
            site_ = pick_;
            mode_ = Mode::Brief;
            t_ = 0;
            applyTheme(sys.vdp, site());
        } else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Help) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Brief) {
        flyT_ = t_;
        z_ = std::min(site().startZ, 900.0f);
        y_ = z_ * std::tan(site().slopeDeg * DEG) + site().startHigh;
        if ((bot_ && t_ > 0.25f) || pad.pressed(gs::BTN_START)) {
            resetPass();
            say("CALL THE BALL");
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Fly) {
        flyT_ += DT;
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else step(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Fly;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Grade) {
        bool next = (bot_ && t_ > 0.45f) || pad.pressed(gs::BTN_START);
        if (next) {
            if (passGrade(grade_) && tour_ && site_ + 1 < NUM_SITES) {
                site_++;
                mode_ = Mode::Brief;
                t_ = 0;
                applyTheme(sys.vdp, site());
            } else if (passGrade(grade_) && tour_ && site_ + 1 >= NUM_SITES) {
                mode_ = Mode::Victory;
                over_ = true;
            } else if (!passGrade(grade_) && bot_) {
                if (site_ + 1 < NUM_SITES) {
                    site_++;
                    mode_ = Mode::Brief;
                    t_ = 0;
                    applyTheme(sys.vdp, site());
                } else {
                    over_ = true;
                    mode_ = Mode::Victory;
                }
            } else if (!passGrade(grade_)) resetPass();
            else mode_ = Mode::Menu;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Victory) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) mode_ = Mode::Menu;
    }

    if (bot_ && mode_ == Mode::Victory) over_ = true;
    draw();
}

}  // namespace trap
