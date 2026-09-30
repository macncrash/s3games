#include "keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace keel {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kTide = 52.f;
constexpr float kStartX = -6.f;
constexpr float kStartY = -30.f;
constexpr float kStartH = 1.15f;
constexpr float kBow = 2.7f;
constexpr float kStern = 2.15f;
constexpr float kBeam = 0.9f;
constexpr float kPlayZoom = 7.4f;
constexpr float kTitleZoom = 4.2f;
constexpr float kTitleCamX = 0.f;
constexpr float kTitleCamY = 2.f;

// Finger slip opens south. Inner water is x -2.5..2.5, y 0..15.2.
constexpr float kInL = -2.55f, kInR = 2.55f;
constexpr float kMouth = 0.f, kHead = 15.2f;
constexpr float kBerthX0 = -1.15f, kBerthX1 = 1.15f;
constexpr float kBerthY0 = 7.2f, kBerthY1 = 12.4f;
constexpr float kParkX = 0.f, kParkY = 9.6f;

struct Box {
    float x0, y0, x1, y1;
};

// Pilings and quay. The channel between the fingers is open water.
constexpr Box kSolid[] = {
    {-6.4f, 0.f, -2.7f, 17.4f},
    {2.7f, 0.f, 6.4f, 17.4f},
    {-6.4f, 15.4f, 6.4f, 18.6f},
    {-22.f, -6.f, -16.5f, 8.f},
    {16.f, -4.f, 24.f, 12.f},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

bool inside(const Box& b, float x, float y) {
    return x >= b.x0 && x <= b.x1 && y >= b.y0 && y <= b.y1;
}

bool solidAt(float x, float y) {
    for (const Box& b : kSolid)
        if (inside(b, x, y)) return true;
    return false;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    int r = int(ar + (br - ar) * t + 0.5f);
    int g = int(ag + (bg - ag) * t + 0.5f);
    int bl = int(ab + (bb - ab) * t + 0.5f);
    return gs::rgb4(r, g, bl);
}

}  // namespace

float Game::tideLeft() const { return std::max(0.f, kTide - raceTime_); }

float Game::tideU() const {
    if (mode_ == Mode::Title) return 0.08f;
    return std::clamp(raceTime_ / kTide, 0.f, 1.f);
}

float Game::ebb() const {
    float u = std::clamp(raceTime_ / kTide, 0.f, 1.f);
    return 0.25f + 1.7f * u * u;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inBerth_ && std::fabs(speed_) < 0.7f) return 3;
    if (inSlip_) return 2;
    return 1;
}

int Game::hullFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

const char* Game::hint() const {
    if (inBerth_ && std::fabs(speed_) > 0.4f) return "EASE OFF AND HOLD";
    if (inBerth_) return "HOLD THE BERTH";
    if (inSlip_) return "BOW TO THE HEAD OF THE SLIP";
    if (tideU() > 0.5f) return "THE TIDE IS RUNNING WEST";
    return "BERTH BEFORE THE TIDE TURNS";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    hold_ = 0.f;
    wakeT_ = 0.f;
    tickT_ = 0.f;
    wakeCursor_ = 0;
    inSlip_ = false;
    inBerth_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Wake& w : wakes_) w = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    blip(440.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.07f);
    if (bot_) {
        begin();
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::human(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.1f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.15f);
    else {
        float decay = std::fabs(speed_) < 0.35f ? 2.8f : 0.5f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& steer, float& throttle) {
    float flow = ebb();
    float vx, vy;
    const float doorX = 0.1f;
    const float doorY = -6.6f;
    bool atDoor = std::fabs(x_ - doorX) < 1.1f && y_ > doorY - 1.f && y_ < -4.8f;
    if (!atDoor && y_ < -4.8f) {
        float fair = y_ > -16.f && x_ > -9.f ? flow * 0.15f : flow;
        vx = std::clamp((doorX - x_) * 0.9f, -2.6f, 2.6f) + fair;
        vy = std::clamp((doorY - y_) * 0.65f, -1.6f, 3.8f);
    } else if (y_ < 0.4f) {
        float xerr = doorX - x_;
        if (std::fabs(xerr) > 0.45f) {
            vx = std::clamp(xerr * 1.6f, -1.2f, 1.2f);
            vy = 0.f;
        } else {
            vx = xerr * 0.35f;
            vy = 1.3f;
        }
    } else if (y_ < kParkY - 0.25f) {
        vx = std::clamp((0.02f - x_) * 1.1f, -0.7f, 0.7f) + (y_ > 0.4f ? flow * 0.1f : flow * 0.35f);
        float crawl = y_ < 2.f ? 1.35f : 0.85f;
        vy = std::clamp((kParkY - y_) * 0.4f, 0.15f, crawl);
    } else {
        vx = std::clamp((0.02f - x_) * 0.8f, -0.35f, 0.35f);
        vy = std::clamp((kParkY - y_) * 1.4f, -0.7f, 0.35f);
    }
    if (y_ > kHead - 3.f) vy = std::min(vy, -0.35f);
    float want = std::atan2(vy, vx);
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.26f, -1.f, 1.f);
    float along = std::hypot(vx, vy);
    if (std::fabs(err) > 0.75f) along = 0.f;
    else if (std::fabs(err) > 0.4f) along *= 0.35f;
    float sp = speed_;
    throttle = std::clamp((along - sp) * 0.7f, -1.f, 1.f);
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    chime(5);
    sys_->rumble(0.22f, 0.4f, 140);
    sys_->setLight(40, 160, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.36f, 90.f, 0.38f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.35f;
    sys_->rumble(0.45f, 0.16f, 150);
    sys_->setLight(160, 30, 24);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    if (raceTime_ >= kTide) {
        fail("tide turned");
        return;
    }

    float rate = 1.7f + std::min(std::fabs(speed_), 6.f) * 0.04f;
    heading_ = wrap(heading_ + steer * rate * dt);

    inSlip_ = x_ > kInL + 0.15f && x_ < kInR - 0.15f && y_ > kMouth + 0.4f && y_ < kHead - 0.3f;
    float cap = inSlip_ ? 2.8f : 6.2f;
    float target = throttle >= 0.f ? throttle * cap : throttle * 2.4f;
    float k = throttle < -0.02f && speed_ > 0.f ? 4.8f : 1.8f;
    speed_ += (target - speed_) * (1.f - std::exp(-k * dt));
    if (std::fabs(throttle) < 0.04f) speed_ *= std::exp(-2.4f * dt);
    speed_ = std::clamp(speed_, -2.2f, 7.2f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;

    float flow = ebb();
    bool sheltered = y_ > 0.2f && x_ > kInL && x_ < kInR;
    bool basin = y_ > -16.f && x_ > -9.f && x_ < 9.f;
    float push = sheltered ? flow * 0.05f : basin ? flow * 0.15f : flow;
    x_ -= push * dt;

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float pts[5][2];
    sample(kBow, 0.f, pts[0][0], pts[0][1]);
    sample(-kStern, 0.f, pts[1][0], pts[1][1]);
    sample(0.3f, -kBeam, pts[2][0], pts[2][1]);
    sample(0.3f, kBeam, pts[3][0], pts[3][1]);
    sample(-1.2f, 0.f, pts[4][0], pts[4][1]);

    for (int i = 0; i < 5; i++) {
        if (solidAt(pts[i][0], pts[i][1])) {
            fail(y_ > -2.f ? "scraped the piling" : "hit the breakwater");
            return;
        }
    }
    if (x_ < -36.f || x_ > 36.f || y_ < -48.f || y_ > 28.f) {
        fail("lost the harbor");
        return;
    }

    float herr = std::fabs(wrap(heading_ - kPi * 0.5f));
    inBerth_ = x_ > kBerthX0 && x_ < kBerthX1 && y_ > kBerthY0 && y_ < kBerthY1 && herr < 0.42f;
    if (inBerth_ && std::fabs(speed_) < 0.42f) hold_ += dt;
    else hold_ = std::max(0.f, hold_ - dt * 1.5f);
    if (hold_ > 0.7f) succeed();

    if (std::fabs(speed_) > 0.4f) {
        wakeT_ -= dt;
        if (wakeT_ <= 0.f) {
            wakeT_ = 0.08f;
            Wake& w = wakes_[wakeCursor_++ % 10];
            w.x = x_ - c * 2.2f;
            w.y = y_ - s * 2.2f;
            w.life = 1.f;
        }
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= dt * 0.7f;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.f;
}

void Game::audio(float dt) {
    t_ += dt;
    float water = mode_ == Mode::Run ? 0.012f + std::fabs(speed_) * 0.004f + tideU() * 0.01f : 0.008f;
    sys_->apu.noise(water, 1800.f + tideU() * 900.f, false);
    if (mode_ == Mode::Run && std::fabs(throttle_) > 0.05f) {
        float vol = 0.02f + std::fabs(throttle_) * 0.03f;
        sys_->apu.tone(2, 55.f + std::max(0.f, throttle_) * 36.f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && tideLeft() < 14.f && tideLeft() > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(tideLeft() < 5.f ? 760.f : 420.f);
            tickT_ = tideLeft() < 5.f ? 0.28f : 0.6f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f && chimeStep_ < chimeN_) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.18f;
            chimeStep_++;
            chimeT_ = 0.14f;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Run && !bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
    else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A))) startRun();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) showTitle();
    }

    if (mode_ == Mode::Run) {
        float steer = 0.f, throttle = 0.f;
        if (bot_) pilot(steer, throttle);
        else human(steer, throttle);
        physics(kDt, steer, throttle);
        if (mode_ == Mode::Run) {
            if (tideLeft() < 12.f) sys.setLight(170, 80, 30);
            else sys.setLight(30, 110, 90);
        }
    }
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 3.2f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        gx = kParkX;
        gy = (kMouth + kHead) * 0.5f;
        gz = 5.2f;
    }
    float k = 1.f - std::exp(-kDt * (mode_ == Mode::Run ? 3.2f : 2.f));
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (gz - zoom_) * k;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char ch = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || ch < 32 || ch > 127 || ch == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[ch - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite spt;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    spt.w = int16_t(sw);
    spt.h = int16_t(sh);
    spt.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    spt.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    spt.img = m.pick(float(sh));
    spt.pal = uint8_t(pal);
    spt.shadow = shadow;
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    spr(m, sx, sy, std::max(2.f, worldH * zoom_), pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 24.f);
    float tide = tideU();
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float u = std::clamp((wy + 20.f) / 70.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(2, 8, 11), gs::rgb4(1, 3, 7), 1.f - u);
        float shim = 0.5f + 0.5f * std::sin(wy * 0.45f + t_ * 1.6f + camX_ * 0.05f);
        if (shim > 0.93f) water = lerpC(water, gs::rgb4(10, 13, 12), 0.35f);
        water = lerpC(water, gs::rgb4(6, 5, 3), tide * 0.45f);
        v.lineBackdrop[y] = water;
        v.lineFog[y] = uint8_t(std::clamp(int((std::fabs(wy - camY_) - 22.f) * 0.08f), 0, 7));
        gs::RoadLine& r = v.road[y];
        r = {};
        if (wy > kMouth && wy < kHead) {
            float sx0 = 160.f + (kInL - camX_) * zoom;
            float sx1 = 160.f + (kInR - camX_) * zoom;
            r.on = true;
            r.cx = (sx0 + sx1) * 0.5f;
            r.hw = std::max(2.f, std::fabs(sx1 - sx0) * 0.5f);
            r.v = wy * 6.f + t_ * 4.f;
            r.pal = uint8_t(PAL_SLIP);
            r.band = (int(std::floor(wy * 0.35f)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        } else if (wy > 18.6f && wy < 28.f) {
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom;
            r.hw = 40.f * zoom;
            r.v = wy * 3.f;
            r.pal = uint8_t(PAL_SHORE);
            r.band = (int(std::floor(wy * 0.25f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = 0;
            r.right = 0;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 22.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 28.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.lost;
        if (std::strcmp(why_, "tide turned") == 0) msg = &art_.tide;
        else if (std::strcmp(why_, "scraped the piling") == 0 || std::strcmp(why_, "hit the breakwater") == 0)
            msg = &art_.scraped;
        banner(*msg, 160.f, 22.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.berthed, 160.f, 18.f, PAL_WIN);
        banner(art_.held, 160.f, 46.f, PAL_WIN);
    }

    for (float py = 1.1f; py < 15.2f; py += 2.15f) {
        place(art_.plank, -4.55f, py, 2.05f, PAL_PIER);
        place(art_.plank, 4.55f, py, 2.05f, PAL_PIER);
    }
    for (float px = -5.2f; px <= 5.2f; px += 2.1f) place(art_.plank, px, 16.6f, 1.5f, PAL_PIER);
    for (float py = -4.f; py < 7.f; py += 2.2f) place(art_.plank, -19.2f, py, 2.0f, PAL_PIER);
    place(art_.lamp, -2.9f, 16.2f, 3.4f, PAL_LAMP);
    place(art_.lamp, 2.9f, 16.2f, 3.4f, PAL_LAMP);
    place(art_.piling, -2.85f, 0.35f, 1.5f, PAL_PIER);
    place(art_.piling, 2.85f, 0.35f, 1.5f, PAL_PIER);
    place(art_.buoy, -3.6f, -1.6f, 2.2f, PAL_ALERT);
    place(art_.buoy, 3.6f, -1.6f, 2.2f, PAL_MARK);
    place(art_.dot, kBerthX0, kBerthY0, 0.55f, PAL_WIN);
    place(art_.dot, kBerthX1, kBerthY0, 0.55f, PAL_WIN);
    place(art_.dot, kBerthX0, kBerthY1, 0.55f, PAL_WIN);
    place(art_.dot, kBerthX1, kBerthY1, 0.55f, PAL_WIN);

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        place(art_.foam, w.x, w.y, 0.7f + (1.f - w.life) * 1.1f, PAL_HULL);
    }

    float bob = (inSlip_ ? 0.08f : 0.18f) * std::sin(t_ * 2.1f);
    float bsx = 160.f + (x_ - camX_) * zoom;
    float bsy = 112.f - (y_ - camY_) * zoom + bob * zoom * 0.15f;
    float hullH = 6.4f * zoom;
    if (mode_ == Mode::Title) hullH = std::max(hullH, 28.f);
    const gs::Mipped& hull = art_.hull[hullFrame()];
    spr(hull, bsx + 2.f, bsy + 3.f, hullH, PAL_HULL, true);
    spr(hull, bsx, bsy, hullH, PAL_HULL, false);

    if (mode_ == Mode::Run) {
        float psx = 160.f + (kParkX - camX_) * zoom;
        float psy = 112.f - (kParkY - camY_) * zoom;
        if (psx < 14.f || psx > 306.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float kk = 1.f;
            if (std::fabs(dx) > 1.f) kk = std::min(kk, 130.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) kk = std::min(kk, 80.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * kk, 112.f + dy * kk, 12.f, PAL_MARK, false);
        }
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(22, "BERTH IN THE SLIP", PAL_HUD);
        hudC(23, "BEFORE THE TIDE TURNS", PAL_HUD);
        hudC(25, "ARROWS STEER   UP AHEAD   DOWN ASTERN", PAL_HUD);
        hudC(26, "START TO CAST OFF", PAL_HUD);
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        std::snprintf(line, sizeof line, "TIDE %4.1f", tideLeft());
        hud(1, 1, line, tideLeft() < 12.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "SPD %4.1f", speed_);
        hud(30, 1, line, PAL_HUD);
        hudC(26, hint(), PAL_HUD);
    } else if (mode_ == Mode::Win) {
        std::snprintf(line, sizeof line, "MADE IT WITH %4.1f S OF TIDE", tideLeft());
        hudC(25, line, PAL_WIN);
        hudC(26, "START FOR THE HARBOR", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, why_, PAL_ALERT);
        hudC(26, "START TO TRY THE SLIP AGAIN", PAL_HUD);
    }
}

}  // namespace keel
