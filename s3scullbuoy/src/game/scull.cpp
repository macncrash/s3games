#include "game/scull.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace scull {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kCrew = 46.f;
constexpr int kGates = 5;

struct V2 {
    float x, y;
};

// Leave each buoy to port, then the same dock. Gates are the water you must cross.
constexpr V2 kBuoy[4] = {{28.f, 74.f}, {22.f, 168.f}, {-40.f, 172.f}, {-44.f, 78.f}};
constexpr V2 kGate[kGates] = {{52.f, 74.f}, {22.f, 196.f}, {-64.f, 172.f}, {-68.f, 78.f}, {0.f, 16.f}};
constexpr V2 kPath[] = {{0.f, 16.f},  {52.f, 74.f},  {40.f, 140.f}, {22.f, 196.f}, {-22.f, 196.f},
                        {-64.f, 172.f}, {-68.f, 78.f}, {-30.f, 28.f}, {0.f, 16.f}};
constexpr int kPathN = int(sizeof(kPath) / sizeof(kPath[0]));

constexpr V2 kReed[] = {{-70.f, 30.f}, {-74.f, 90.f}, {-72.f, 150.f}, {-68.f, 200.f},
                        {70.f, 24.f},  {74.f, 88.f},  {72.f, 148.f},  {68.f, 198.f},
                        {-20.f, 214.f}, {16.f, 216.f}, {48.f, 208.f}};

float wrapPi(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float pathLength() {
    float n = 0;
    for (int i = 1; i < kPathN; i++) n += std::hypot(kPath[i].x - kPath[i - 1].x, kPath[i].y - kPath[i - 1].y);
    return n;
}

}  // namespace

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    tone_ = 0.06f;
}

void Game::fanfare() { fanStep_ = 0; }

void Game::begin() {
    x_ = 0;
    y_ = 14.f;
    heading_ = 0;
    speed_ = 0;
    stroke_ = 0;
    you_ = 0;
    next_ = 0;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    foam_.clear();
    camX_ = x_;
    camY_ = y_;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    pathLen_ = pathLength();
    crewTime_ = kCrew;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    for (int y = 0; y < sys.vdp.B.h; y++)
        for (int x = 0; x < sys.vdp.B.w; x++) sys.vdp.B.set(x, y, gs::entry(art_.waterTile, PAL_WATER));
    sys.vdp.setFogColor(gs::rgb4(1, 4, 8));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.2f, 0.1f);
    rng_ = 0x5C011u;
    begin();
    if (bot_) {
        mode_ = Mode::Race;
        zoom_ = 2.75f;
    } else {
        mode_ = Mode::Title;
        zoom_ = 2.15f;
        camX_ = 8.f;
        camY_ = 36.f;
    }
}

void Game::controls(float& thrust, float& steer) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    thrust = 0;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (p.axisX) steer = std::clamp(p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_UP) || p.accel > 0.2f) thrust = 1.f;
    if (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN)) thrust = -0.35f;
}

void Game::pilot(float& thrust, float& steer) {
    const V2 g = kGate[std::clamp(next_, 0, kGates - 1)];
    float dx = g.x - x_;
    float dy = g.y - y_;
    float dist = std::hypot(dx, dy);
    float want = std::atan2(dx, dy);
    float err = wrapPi(want - heading_);
    steer = std::clamp(err * 2.6f, -1.f, 1.f);
    thrust = std::fabs(err) < 0.9f ? 1.f : 0.15f;
    if (dist < 20.f && std::fabs(err) > 0.6f) thrust = 0.1f;
}

void Game::physics(float dt, float thrust, float steer) {
    float drive = thrust;
    if (thrust > 0.f && !bot_) {
        stroke_ += dt * 3.1f;
        drive = 0.35f + 0.65f * std::max(0.f, std::sin(stroke_ * kTau));
    } else if (thrust > 0.f) {
        stroke_ += dt * 3.1f;
        drive = thrust;
    } else {
        stroke_ += dt * 0.4f;
        drive = thrust;
    }
    float yaw = (0.7f + std::min(speed_, 24.f) * 0.045f) * steer;
    heading_ = wrapPi(heading_ + yaw * dt);
    speed_ += drive * 34.f * dt;
    speed_ -= speed_ * 0.72f * dt;
    if (speed_ > 40.f) speed_ = 40.f;
    if (speed_ < -6.f) speed_ = -6.f;
    x_ += std::sin(heading_) * speed_ * dt;
    y_ += std::cos(heading_) * speed_ * dt;

    for (const V2& b : kBuoy) {
        float dx = x_ - b.x, dy = y_ - b.y;
        float d = std::hypot(dx, dy);
        if (d < 7.5f && d > 0.01f) {
            float push = (7.5f - d) / d;
            x_ += dx * push;
            y_ += dy * push;
            speed_ *= 0.45f;
        }
    }
    if (x_ < -78.f) {
        x_ = -78.f;
        speed_ *= 0.5f;
    }
    if (x_ > 78.f) {
        x_ = 78.f;
        speed_ *= 0.5f;
    }
    if (y_ < 2.f) {
        y_ = 2.f;
        speed_ *= 0.5f;
    }
    if (y_ > 220.f) {
        y_ = 220.f;
        speed_ *= 0.5f;
    }

    if (std::fabs(speed_) > 4.f) {
        rng_ = rng_ * 1664525u + 1013904223u;
        if (foam_.size() < 18 && (rng_ >> 28) < 3) {
            float bx = x_ - std::sin(heading_) * 7.f;
            float by = y_ - std::cos(heading_) * 7.f;
            foam_.push_back({bx, by, 0.7f});
        }
    }
    for (Foam& f : foam_) f.life -= dt;
    foam_.erase(std::remove_if(foam_.begin(), foam_.end(), [](const Foam& f) { return f.life <= 0.f; }), foam_.end());
}

void Game::marks(float dt) {
    (void)dt;
    if (next_ >= kGates) return;
    const V2 g = kGate[next_];
    float r = next_ == kGates - 1 ? 16.f : 14.f;
    if (std::hypot(x_ - g.x, y_ - g.y) <= r) {
        next_++;
        blip(next_ >= kGates ? 880.f : 640.f);
        if (next_ >= kGates) {
            mode_ = Mode::Win;
            won_ = true;
            over_ = true;
            fanfare();
        }
    }
}

void Game::rivalAt(float t, float& x, float& y, float& heading) const {
    float dist = std::min(t, crewTime_) / crewTime_ * pathLen_;
    float acc = 0;
    for (int i = 1; i < kPathN; i++) {
        float seg = std::hypot(kPath[i].x - kPath[i - 1].x, kPath[i].y - kPath[i - 1].y);
        if (acc + seg >= dist || i == kPathN - 1) {
            float u = seg > 0.01f ? std::clamp((dist - acc) / seg, 0.f, 1.f) : 1.f;
            x = kPath[i - 1].x + (kPath[i].x - kPath[i - 1].x) * u;
            y = kPath[i - 1].y + (kPath[i].y - kPath[i - 1].y) * u;
            heading = std::atan2(kPath[i].x - kPath[i - 1].x, kPath[i].y - kPath[i - 1].y);
            return;
        }
        acc += seg;
    }
    x = kPath[kPathN - 1].x;
    y = kPath[kPathN - 1].y;
    heading = 0;
}

void Game::audio(float dt, float thrust) {
    if (tone_ > 0.f) {
        tone_ -= dt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Race && thrust > 0.2f) {
        float phase = stroke_ - std::floor(stroke_);
        if (phase < dt * 3.1f) {
            sys_->apu.tone(1, 180.f + speed_ * 4.f, 0.04f);
        } else if (phase > 0.5f && phase < 0.5f + dt * 3.1f) {
            sys_->apu.tone(1, 0, 0);
        }
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        fanStep_++;
        if (fanStep_ % 10 == 1 && fanStep_ < 40) sys_->apu.tone(0, notes[fanStep_ / 10], 0.1f);
        if (fanStep_ > 56) {
            fanStep_ = -1;
            sys_->apu.tone(0, 0, 0);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - camX_) * zoom_ + 160.f;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal, flip);
}

int Game::shellFrame(float heading) const {
    float h = heading - std::floor(heading / kTau) * kTau;
    int face = int(h / (kTau / 8.f) + 0.5f) & 7;
    int phase = int(stroke_ * 2.f) & 1;
    return face * 2 + phase;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

static void clockText(char* out, int n, const char* label, float sec) {
    if (sec < 0.f) sec = 0.f;
    int s = int(sec);
    int cs = int(sec * 100.f) % 100;
    std::snprintf(out, size_t(n), "%s %d:%02d.%02d", label, s / 60, s % 60, cs);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    int scrollX = int(std::lround(camX_ * zoom_));
    int scrollY = int(std::lround(-camY_ * zoom_ + t_ * 6.f));
    vdp.B.scroll(scrollX, scrollY);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int band = 4 + ((y + int(t_ * 18.f)) / 28) % 3;
        vdp.lineBackdrop[y] = gs::rgb4(1, band, 8 + (y > 160 ? 1 : 0));
        vdp.road[y].on = false;
    }

    // Earlier sprites sit on top. Shell first, then the other crew, then the course.
    if (mode_ != Mode::Title) place(art_.shell[shellFrame(heading_)], x_, y_, 16.f, PAL_SHELL);
    else place(art_.shell[shellFrame(0.2f)], 6.f, 28.f + std::sin(t_ * 1.4f) * 1.5f, 22.f, PAL_SHELL);

    float rx, ry, rh;
    rivalAt(mode_ == Mode::Title ? std::fmod(t_ * 0.35f, crewTime_) : you_, rx, ry, rh);
    if (mode_ != Mode::Win) place(art_.shell[shellFrame(rh)], rx, ry, 16.f, PAL_RIVAL);

    if (mode_ == Mode::Race && next_ < kGates) {
        const V2 g = kGate[next_];
        float bob = std::sin(t_ * 3.f) * 1.2f;
        place(art_.chevron, g.x, g.y + bob, 5.f, PAL_MARK);
    }
    for (int i = 0; i < 4; i++) {
        float bob = std::sin(t_ * 2.2f + i) * 0.8f;
        bool gold = i == 1 || i == 2;
        bool next = mode_ == Mode::Race && i == next_;
        place(art_.buoy[gold ? 1 : 0], kBuoy[i].x, kBuoy[i].y + bob, next ? 12.f : 10.f, PAL_BUOY);
    }
    place(art_.dock, 0.f, 6.f, 22.f, PAL_DOCK);
    for (const Foam& f : foam_) place(art_.foam, f.x, f.y, 3.5f + f.life, PAL_FOAM);
    for (const V2& r : kReed) place(art_.reed, r.x, r.y, 9.f, PAL_SHORE);

    char line[32];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SCULL", PAL_HUD);
        hudC(6, "ROUND THE BUOYS", 2);
        hudC(8, "BACK TO THE SAME DOCK", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", 3);
        hudC(18, "A ROW    ARROWS STEER", PAL_HUD);
        hudC(21, "START", 2);
    } else if (mode_ == Mode::Pause) {
        clockText(line, 32, "YOU", you_);
        hud(1, 1, line, PAL_HUD);
        hudC(12, "PAUSED", 2);
        hudC(15, "START CONTINUES", PAL_HUD);
    } else {
        clockText(line, 32, "YOU", you_);
        hud(1, 1, line, PAL_HUD);
        clockText(line, 32, "CREW", crewTime_);
        hud(24, 1, line, 5);
        if (mode_ == Mode::Race) {
            if (next_ < 4) {
                std::snprintf(line, sizeof(line), "BUOY %d OF 4", next_ + 1);
                hudC(26, line, 2);
            } else {
                hudC(26, "HOME TO THE DOCK", 3);
            }
            hud(1, 26, "PORT", PAL_HUD);
        } else if (mode_ == Mode::Win) {
            hudC(11, "HOME", 3);
            hudC(13, "BEAT THE OTHER CREW", 2);
            if (!bot_) hudC(16, "START ROWS AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(11, "THE OTHER CREW", 4);
            hudC(13, "IS ALREADY HOME", 4);
            if (!bot_) hudC(16, "START ROWS AGAIN", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    float thrust = 0;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Race;
            zoom_ = 2.75f;
            blip(620.f);
        }
    } else if (mode_ == Mode::Race) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
        } else {
            float steer = 0;
            if (bot_) pilot(thrust, steer);
            else controls(thrust, steer);
            you_ += kDt;
            physics(kDt, thrust, steer);
            if (!over_) marks(kDt);
            if (mode_ == Mode::Race && you_ >= crewTime_) {
                mode_ = Mode::Fail;
                won_ = false;
                over_ = true;
                blip(110.f);
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Race;
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        begin();
        mode_ = Mode::Race;
        zoom_ = 2.75f;
        blip(620.f);
    }

    if (mode_ == Mode::Title) {
        camX_ = 10.f + std::sin(t_ * 0.25f) * 6.f;
        camY_ = 40.f + std::sin(t_ * 0.17f) * 4.f;
        zoom_ = 2.15f;
    } else {
        float lead = mode_ == Mode::Race ? 10.f : 0.f;
        float gx = x_ + std::sin(heading_) * lead;
        float gy = y_ + std::cos(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.5f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ = 2.75f;
    }
    audio(kDt, thrust);
    draw();
}

}  // namespace scull
