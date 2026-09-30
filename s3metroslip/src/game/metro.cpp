#include "metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace metroslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;

constexpr float kStartY = 28.f;
constexpr float kEndY = 348.f;
constexpr float kSlipY0 = 286.f;
constexpr float kSlipY1 = 336.f;
constexpr float kBerthY0 = 300.f;
constexpr float kBerthY1 = 324.f;
constexpr float kBerthOff0 = 14.f;
constexpr float kBerthOff1 = 26.f;
constexpr float kHalf = 20.f;
constexpr float kLimit = 16.f;
constexpr float kSlipOff = 32.f;
constexpr float kTide = 48.f;
constexpr float kStop = 0.32f;
constexpr float kSettle = 0.5f;
constexpr float kPlayZoom = 1.7f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::tideLeft() const {
    int s = int(std::ceil(kTide - raceTime_));
    return std::max(0, s);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inSlip_ && speed_ <= kStop) return 3;
    if (inMouth_) return 2;
    return 1;
}

float Game::laneX(float y) const { return 16.f * std::sin((y - kStartY) * 0.016f); }

bool Game::mouthOpen() const { return y_ >= kSlipY0 && y_ <= kSlipY1; }

void Game::pose() { x_ = laneX(y_) + off_; }

void Game::begin() {
    y_ = kStartY;
    off_ = 0.f;
    speed_ = 0.f;
    throttle_ = 0.f;
    steer_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    washT_ = 0.f;
    inSlip_ = false;
    inMouth_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    pose();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = 0.72f;
    camX_ = laneX(300.f) + 12.f;
    camY_ = 312.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(5, 8, 11));
    sys.apu.setMaster(0.76f);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
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

void Game::human(float& throttle, float& steer) {
    const gs::Pad& p = sys_->pad;
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.28f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.4f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.2f);
    else {
        float decay = std::fabs(speed_) < 0.5f ? 3.2f : 0.7f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    float want = 0.f;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.25f) want -= 1.f;
    if (p.down(gs::BTN_RIGHT) || p.axisX > 0.25f) want += 1.f;
    steer_ += (want - steer_) * (1.f - std::exp(-6.f * kDt));
    throttle = throttle_;
    steer = steer_;
}

void Game::pilot(float& throttle, float& steer) {
    float wantOff = 0.f;
    float wantSpd = 13.5f;
    if (y_ < 230.f) {
        wantOff = 0.f;
        wantSpd = 13.5f;
    } else if (y_ < kSlipY0) {
        wantOff = 11.f;
        wantSpd = std::clamp((kSlipY0 + 6.f - y_) * 0.32f, 2.2f, 8.f);
    } else if (off_ < kBerthOff0 + 1.f) {
        wantOff = 22.f;
        wantSpd = y_ < 308.f ? 2.0f : 0.45f;
    } else {
        wantOff = 20.f;
        wantSpd = std::clamp((312.f - y_) * 0.6f, 0.f, 2.2f);
    }
    if (wantSpd <= 0.05f) throttle = speed_ > 0.16f ? -1.f : -0.4f;
    else if (speed_ > wantSpd + 0.3f) throttle = -1.f;
    else if (speed_ < wantSpd - 0.35f) throttle = 0.92f;
    else throttle = 0.1f;
    steer = std::clamp((wantOff - off_) * 0.35f, -1.f, 1.f);
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    std::printf("S3 METRO SLIP  PASS  berthed in the slip  %ds before the tide turned\n", tideLeft());
    std::fflush(stdout);
    chime(4);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.4f, 80.f, 0.38f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics(float dt, float throttle, float steer) {
    raceTime_ += dt;
    float tideU = raceTime_ / kTide;
    float flow = std::cos(std::min(tideU, 1.f) * kPi) * 2.6f;

    float cap = 15.5f;
    float target = std::max(0.f, throttle) * cap;
    speed_ += (target - speed_) * (1.f - std::exp(-1.35f * dt));
    if (throttle < -0.02f && speed_ > 0.f) speed_ = std::max(0.f, speed_ - (-throttle) * 7.2f * dt);

    const bool sheltered = y_ >= kSlipY0 - 6.f && y_ <= kSlipY1 && off_ > 8.f;
    if (!sheltered) speed_ += flow * dt;
    if (tideU > 1.f) speed_ = std::max(0.f, speed_ - 3.5f * dt);
    speed_ = std::clamp(speed_, 0.f, 20.f);

    y_ += speed_ * dt;
    if (y_ < kStartY - 8.f) y_ = kStartY - 8.f;
    off_ += steer * 12.f * dt;
    if (!(y_ >= kSlipY0 && y_ <= kSlipY1)) off_ += -off_ * 0.2f * dt;
    pose();

    inMouth_ = mouthOpen() && off_ > 6.f;
    inSlip_ = y_ >= kBerthY0 && y_ <= kBerthY1 && off_ >= kBerthOff0 && off_ <= kBerthOff1;

    if (y_ > kEndY) {
        fail("missed the end");
        return;
    }
    float limR = mouthOpen() ? kSlipOff : kLimit;
    if (off_ < -kLimit || off_ > limR) {
        fail(y_ > kEndY - 18.f ? "missed the end" : "left the metro");
        return;
    }
    if (raceTime_ >= kTide) {
        fail("the tide turned");
        return;
    }

    if (inSlip_ && speed_ <= kStop) {
        settle_ += dt;
        speed_ = 0.f;
        if (settle_ >= kSettle) {
            succeed();
            return;
        }
    } else {
        settle_ = 0.f;
    }

    washT_ -= dt;
    if (mode_ == Mode::Run && speed_ > 3.f && washT_ <= 0.f) {
        washT_ = 0.12f;
        sys_->apu.noiseBurst(0.08f, 380.f, 0.04f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 4);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float wash = mode_ == Mode::Run ? 0.008f + speed_ * 0.0006f : 0.004f;
    sys_->apu.noise(wash, inSlip_ ? 160.f : 250.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || speed_ > 2.f)) {
        float wob = 0.7f + 0.3f * std::sin(t_ * (8.f + std::max(0.f, throttle_) * 10.f));
        float vol = (0.012f + std::max(0.f, throttle_) * 0.026f) * wob;
        sys_->apu.tone(2, 48.f + std::max(0.f, throttle_) * 16.f + speed_ * 0.25f, vol);
    } else if (tone0_ <= 0.f) {
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
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 3)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(320.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float thr = 0.f, st = 0.f;
            if (bot_) pilot(thr, st);
            else human(thr, st);
            throttle_ = thr;
            steer_ = st;
            physics(kDt, throttle_, steer_);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = laneX(300.f) + 12.f;
        camY_ = 312.f;
        zoom_ = 0.72f;
        return;
    }
    float gy = y_ + (mode_ == Mode::Run ? 18.f : 0.f);
    float gx = laneX(gy) + off_ * 0.55f;
    float k = 1.f - std::exp(-kDt * 4.2f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
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
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    spr(m, sx, sy, worldH * zoom_, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 16.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        float u = std::clamp((wy - kStartY) / 340.f, 0.f, 1.f);
        v.lineBackdrop[y] = lerpC(gs::rgb4(4, 6, 5), gs::rgb4(2, 3, 4), u);
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        bool water = wy >= 8.f && wy <= kEndY + 2.f;
        if (water) {
            bool slip = wy >= kSlipY0 && wy <= kSlipY1;
            bool berth = wy >= kBerthY0 && wy <= kBerthY1;
            float left = laneX(wy) - kHalf;
            float right = laneX(wy) + kHalf + (slip ? (kSlipOff - kLimit + 6.f) : 0.f);
            r.on = true;
            r.cx = 160.f + ((left + right) * 0.5f - camX_) * zoom_;
            r.hw = std::max(8.f, (right - left) * 0.5f * zoom_);
            r.v = wy * 20.f;
            r.pal = uint8_t(berth ? PAL_SLIP : PAL_WATER);
            r.band = (int(std::floor(wy * 0.12f)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = 0;
            r.right = 0;
        } else {
            r.on = false;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 20.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        banner(std::strcmp(why_, "the tide turned") == 0 ? art_.tide : art_.missed, 160.f, 78.f, PAL_ALERT);
        banner(art_.legFail, 160.f, 108.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.berthed, 160.f, 74.f, PAL_WIN);
        banner(art_.inSlip, 160.f, 108.f, PAL_WIN);
    }

    const float berthX = laneX(312.f) + 20.f;
    const float berthY = 312.f;
    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (berthX - camX_) * zoom_;
        float psy = 112.f - (berthY - camY_) * zoom_;
        if (psx < 16.f || psx > 304.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 142.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 90.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false);
        }
    }

    for (float sy = 16.f; sy < kEndY; sy += 18.f) {
        bool slipRow = sy >= kSlipY0 && sy <= kSlipY1;
        place(art_.pile, laneX(sy) - kHalf - 2.f, sy, 8.f, PAL_PILE);
        if (!slipRow) place(art_.pile, laneX(sy) + kHalf + 2.f, sy, 8.f, PAL_PILE);
    }
    for (float sy = kSlipY0; sy <= kSlipY1; sy += 12.f)
        place(art_.pile, laneX(sy) + kSlipOff + 4.f, sy, 8.f, PAL_PILE);
    place(art_.fender, laneX(312.f) + 18.f, 304.f, 12.f, PAL_WOOD);
    place(art_.fender, laneX(312.f) + 30.f, 320.f, 12.f, PAL_WOOD);
    place(art_.flag, laneX(kSlipY0) + kHalf + 4.f, kSlipY0, 11.f, PAL_MARK);
    place(art_.flag, laneX(kEndY) - 4.f, kEndY - 2.f, 12.f, PAL_MARK);
    place(art_.lamp, laneX(312.f) + 22.f, 312.f, 10.f, PAL_LAMP);
    for (int i = 0; i < 4; i++)
        place(art_.dash, laneX(kEndY - 2.f) - 12.f + i * 8.f, kEndY - 1.f, 2.2f, PAL_MARK);

    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_;
    float hullH = 28.f * zoom_;
    if (mode_ == Mode::Title) hullH = std::max(hullH, 18.f);
    spr(art_.hull, bsx + 2.f, bsy + 4.f, hullH, PAL_HULL, true);
    spr(art_.hull, bsx, bsy, hullH, PAL_HULL, false);
    if (mode_ == Mode::Run && speed_ > 1.4f) place(art_.wake, x_, y_ - 10.f, 5.f, PAL_WAKE);

    int flap = int(t_ * 3.2f) & 1;
    place(art_.gull[flap], laneX(160.f) + 24.f, 168.f + std::sin(t_ * 0.4f) * 4.f, 5.f, PAL_GULL);
    place(art_.gull[1 - flap], laneX(80.f) - 18.f, 90.f, 5.f, PAL_GULL);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "BERTH IN THE SLIP", PAL_WIN);
        hudC(23, "BEFORE THE TIDE TURNS", PAL_BANNER);
        hudC(24, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "UP GO   DOWN BRAKE   LEFT RIGHT", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 METRO SLIP", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "TIDE %02d", tideLeft());
    hud(30, 0, buf, tideLeft() < 8 ? PAL_ALERT : PAL_TIDE);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "%dS BEFORE THE TIDE", tideLeft());
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    const char* hint = "STAY IN THE METRO";
    if (inSlip_) hint = speed_ > 1.f ? "BRAKE IN THE SLIP" : "HOLD THE BERTH";
    else if (inMouth_) hint = "THE SLIP IS TO STARBOARD";
    else if (y_ > 220.f) hint = "THE SLIP IS AHEAD";
    hud(1, 1, hint, inSlip_ ? PAL_WIN : PAL_BANNER);
    std::snprintf(buf, sizeof buf, "SPD %02d", int(std::lround(speed_)));
    hud(1, 2, buf, PAL_HUD);
    if (inSlip_) {
        int n = std::clamp(int(settle_ / kSettle * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "BERTH %.*s", n, "******");
        hud(1, 24, buf, PAL_WIN);
    } else {
        int dist = std::max(0, int(std::lround(kEndY - y_)));
        std::snprintf(buf, sizeof buf, "END %d", dist);
        hud(1, 24, buf, PAL_MARK);
    }
    hud(1, 27, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
}

}  // namespace metroslip
