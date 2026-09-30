#include "metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace metrograss {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kTau = 6.2831853f;

constexpr float kStartY = 32.f;
constexpr float kGrassY0 = 206.f;
constexpr float kGrassY1 = 332.f;
constexpr float kEndY0 = 262.f;
constexpr float kEndY1 = 298.f;
constexpr float kEndMid = (kEndY0 + kEndY1) * 0.5f;
constexpr float kGrassHalf = 40.f;
constexpr float kWaterHalf = 22.f;
constexpr float kWaterLimit = 20.f;
constexpr float kGrassLimit = 36.f;

constexpr float kStop = 0.28f;
constexpr float kSettle = 0.45f;
constexpr float kShort = 2.4f;
constexpr float kTitleZoom = 0.72f;
constexpr float kTitleCamX = 6.f;
constexpr float kTitleCamY = 250.f;
constexpr float kPlayZoom = 1.62f;

const float kTuft[][2] = {
    {-34.f, 220.f}, {36.f, 234.f}, {-38.f, 252.f}, {34.f, 274.f}, {-30.f, 292.f},
    {32.f, 310.f},  {-36.f, 322.f}, {28.f, 244.f}, {-24.f, 212.f}, {26.f, 328.f},
};
const float kPostY[] = {58.f, 108.f, 158.f, 198.f};

float laneX(float y) { return 26.f * std::sin((y - kStartY) * 0.019f); }

float laneHead(float y) {
    float dx = 26.f * 0.019f * std::cos((y - kStartY) * 0.019f);
    return std::atan2(1.f, dx);
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_) return 3;
    if (onGrass_) return 2;
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
    if (inEnd_) return std::fabs(speed_) > 1.f ? "BRAKE TO A FULL STOP" : "HOLD THE FULL STOP";
    if (onGrass_) return "THE END IS THE PALE BAND";
    if (y_ > 150.f) return "THE BANK IS AHEAD";
    return "STAY IN THE METRO";
}

void Game::pose() {
    x_ = laneX(y_) + off_;
    heading_ = laneHead(y_) + steer_ * 0.35f;
}

void Game::begin() {
    y_ = kStartY;
    off_ = 0.f;
    speed_ = 0.f;
    throttle_ = 0.f;
    steer_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    washT_ = 0.f;
    onGrass_ = false;
    inEnd_ = false;
    landed_ = false;
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
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.76f);
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
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.2f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.15f);
    else {
        float decay = std::fabs(speed_) < 0.45f ? 3.4f : 0.75f;
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
    float dist = kEndMid - y_;
    float want = 14.5f;
    if (y_ > 160.f) want = 10.5f;
    if (onGrass_) want = std::clamp(dist * 0.26f, 0.f, 7.2f);
    if (y_ >= kEndY0 - 8.f) want = std::clamp(dist * 0.15f, 0.f, 2.6f);
    if (inEnd_) want = 0.f;
    if (speed_ > want + 0.32f) throttle = -1.f;
    else if (speed_ < want - 0.4f) throttle = 0.85f;
    else if (inEnd_ && speed_ > 0.15f) throttle = -0.6f;
    else throttle = 0.f;
    steer = std::clamp(-off_ * 0.22f, -1.f, 1.f);
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    std::printf("S3 METRO GRASS  PASS  landed on the grass and came to a full stop  (%.1f s)\n", raceTime_);
    std::fflush(stdout);
    chime(5);
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
    const bool grassNow = y_ >= kGrassY0 && y_ <= kGrassY1;
    const bool climb = y_ >= 168.f && y_ < kGrassY0;

    if (grassNow && throttle < -0.02f) {
        float decel = (-throttle) * 4.8f;
        if (speed_ > 0.02f) speed_ = std::max(0.f, speed_ - decel * dt);
        else speed_ = 0.f;
    } else {
        float cap = grassNow ? 9.2f : 17.5f;
        if (!grassNow && throttle < 0.f) cap = 7.5f;
        float target = std::max(0.f, throttle) * cap;
        if (!grassNow && throttle < 0.f) target = std::max(0.f, throttle * cap);
        float ak = grassNow ? 1.5f : 1.1f;
        speed_ += (target - speed_) * (1.f - std::exp(-ak * dt));
        if (grassNow && throttle < 0.08f && speed_ > 0.f) speed_ = std::max(0.f, speed_ - 1.7f * dt);
        if (!grassNow && throttle < -0.02f && speed_ > 0.f) speed_ = std::max(0.f, speed_ - (-throttle) * 2.8f * dt);
    }
    if (climb && throttle > -0.35f) speed_ += 5.6f * dt;
    speed_ = std::clamp(speed_, 0.f, 24.f);

    y_ += speed_ * dt;
    if (y_ < kStartY - 6.f) y_ = kStartY - 6.f;
    off_ += steer * 8.5f * dt;
    if (!grassNow) off_ += -off_ * 0.35f * dt;
    pose();

    onGrass_ = y_ >= kGrassY0 && y_ <= kGrassY1;
    inEnd_ = onGrass_ && y_ >= kEndY0 && y_ <= kEndY1 && std::fabs(off_) <= kGrassLimit;
    if (onGrass_ && !landed_) {
        landed_ = true;
        blip(420.f);
        sys_->rumble(0.35f, 0.18f, 100);
    }

    float limit = onGrass_ ? kGrassLimit : kWaterLimit;
    if (std::fabs(off_) > limit) {
        fail(onGrass_ ? "left the grass" : "left the metro");
        return;
    }
    if (y_ > kGrassY1 + 2.f) {
        fail("missed the end");
        return;
    }
    if (raceTime_ > 72.f) {
        fail("timed out");
        return;
    }

    if (inEnd_ && speed_ <= kStop) {
        settle_ += dt;
        speed_ = 0.f;
        if (settle_ >= kSettle) {
            succeed();
            return;
        }
    } else {
        settle_ = 0.f;
    }

    if (mode_ != Mode::Run) return;
    if (onGrass_ && !inEnd_ && speed_ <= kStop) {
        short_ += dt;
        if (short_ >= kShort) {
            fail("missed the end");
            return;
        }
    } else {
        short_ = 0.f;
    }

    washT_ -= dt;
    if (mode_ == Mode::Run && !onGrass_ && speed_ > 3.f && washT_ <= 0.f) {
        washT_ = std::clamp(0.18f - speed_ * 0.004f, 0.06f, 0.18f);
        sys_->apu.noiseBurst(0.1f, 420.f, 0.04f);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float wash = mode_ == Mode::Run ? 0.008f + speed_ * 0.0006f : 0.004f;
    sys_->apu.noise(onGrass_ ? wash * 0.35f : wash, onGrass_ ? 180.f : 260.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || speed_ > 2.f)) {
        float wob = 0.7f + 0.3f * std::sin(t_ * (8.f + std::max(0.f, throttle_) * 12.f));
        float base = onGrass_ ? 38.f : 52.f;
        float vol = (0.01f + std::max(0.f, throttle_) * 0.028f) * wob;
        sys_->apu.tone(2, base + std::max(0.f, throttle_) * 18.f + speed_ * 0.3f, vol);
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
            static const float notes[] = {349.f, 440.f, 523.f, 698.f, 880.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
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
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 20.f : 0.f;
    float gy = y_ + lead;
    float gx = laneX(gy) + off_ * 0.4f;
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

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.roadTime = int(t_ * 18.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        float u = std::clamp((wy - 10.f) / 360.f, 0.f, 1.f);
        uint16_t sky = lerpC(gs::rgb4(5, 8, 12), gs::rgb4(3, 5, 7), u);
        if (wy >= kGrassY0) sky = lerpC(gs::rgb4(5, 7, 4), gs::rgb4(3, 4, 2), u);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        bool grass = wy >= kGrassY0 && wy <= kGrassY1;
        bool water = wy >= 8.f && wy < kGrassY0;
        if (grass || water) {
            bool endBand = grass && wy >= kEndY0 && wy <= kEndY1;
            r.on = true;
            r.cx = 160.f + (laneX(wy) - camX_) * zoom_;
            r.hw = std::max(6.f, (grass ? kGrassHalf : kWaterHalf) * zoom_);
            r.v = wy * (water ? 22.f : 16.f);
            r.pal = uint8_t(water ? PAL_WATER : (endBand ? PAL_ENDF : PAL_FIELD));
            r.band = (int(std::floor(wy * 0.15f)) & 1) ? 1 : 0;
            r.style = water ? 2 : 0;
            r.left = 0;
            r.right = 0;
        } else {
            r.on = false;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 18.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 96.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        banner(art_.missed, 160.f, 78.f, PAL_ALERT);
        banner(art_.legFail, 160.f, 108.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.fullStop, 160.f, 74.f, PAL_WIN);
        banner(art_.onGrass, 160.f, 108.f, PAL_WIN);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (laneX(kEndMid) - camX_) * zoom_;
        float psy = 112.f - (kEndMid - camY_) * zoom_;
        if (psx < 16.f || psx > 304.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 142.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 90.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false);
        }
    }

    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_;
    float hullH = 26.f * zoom_;
    if (mode_ == Mode::Title) hullH = std::max(hullH, 16.f);
    const gs::Mipped& body = art_.hull[hullFrame()];
    spr(body, bsx + 3.f, bsy + 5.f, hullH, PAL_HULL, true);
    spr(body, bsx, bsy, hullH, PAL_HULL, false);

    const float markPx = mode_ == Mode::Title ? 5.f : 0.f;
    for (float sy = 20.f; sy < kGrassY0; sy += 14.f) {
        float sx = laneX(sy);
        place(art_.post, sx - 16.f, sy, 9.f, PAL_POST, 0.f);
        place(art_.post, sx + 16.f, sy, 9.f, PAL_POST, 0.f);
    }
    for (float ly : kPostY) place(art_.lamp, laneX(ly) - 20.f, ly, 11.f, PAL_LAMP, markPx * 0.4f);
    for (const float* p : kTuft) place(art_.tuft, laneX(p[1]) + p[0], p[1], 8.f, PAL_TUFT, markPx * 0.4f);
    place(art_.halt, laneX(kEndMid) - 28.f, kEndMid + 2.f, 18.f, PAL_WOOD, markPx);
    place(art_.flag, laneX(kEndY0) - 18.f, kEndY0, 12.f, PAL_MARK, markPx);
    place(art_.flag, laneX(kEndY1) + 18.f, kEndY1, 12.f, PAL_MARK, markPx);

    auto dashes = [&](float y, int n) {
        float x0 = laneX(y) - 14.f;
        float x1 = laneX(y) + 14.f;
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dash, x0 + (x1 - x0) * u, y, 2.f, PAL_MARK, 0.f);
        }
    };
    dashes(kEndY0, 5);
    dashes(kEndY1, 5);
    if ((onGrass_ || mode_ == Mode::Title) && mode_ != Mode::Win)
        place(art_.ring, laneX(kEndMid), kEndMid, 24.f, PAL_MARK, mode_ == Mode::Title ? 7.f : 0.f);

    if (!onGrass_ && speed_ > 1.5f && mode_ == Mode::Run) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.wake, x_ - c * 8.f, y_ - 6.f * std::fabs(s) - 4.f, 5.f, PAL_WAKE, 2.f);
    }

    int flap = int(t_ * 3.5f) & 1;
    place(art_.gull[flap], laneX(240.f) + std::sin(t_ * 0.45f) * 18.f, 246.f, 6.f, PAL_GULL, markPx);
    place(art_.gull[1 - flap], laneX(90.f) + 22.f, 96.f + std::sin(t_ * 0.35f) * 5.f, 5.f, PAL_GULL, markPx);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        auto chart = [&](float wy, int pal, float h) {
            spr(art_.dot, 286.f + laneX(wy) * 0.3f, 58.f - (wy - 170.f) * 0.15f, h, pal, false);
        };
        chart(kGrassY0, PAL_WIN, 3.f);
        chart(kGrassY1, PAL_WIN, 3.f);
        chart(kEndMid, PAL_MARK, 5.f);
        chart(y_, PAL_ALERT, 5.f);
        spr(art_.panel, 286.f, 56.f, 64.f, PAL_MAP, false);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(23, "LAND ON THE GRASS", PAL_WIN);
        hudC(24, "COME TO A FULL STOP", PAL_BANNER);
        hudC(25, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "UP GO   DOWN BRAKE   LEFT RIGHT", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 METRO GRASS", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(33, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(18, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    hud(1, 1, hint(), inEnd_ ? PAL_WIN : PAL_BANNER);
    int sp = int(std::lround(speed_));
    std::snprintf(buf, sizeof buf, "SPD %02d  %s", sp, onGrass_ ? "GRASS" : "METRO");
    hud(1, 2, buf, onGrass_ ? PAL_WIN : PAL_HUD);
    if (inEnd_) {
        int n = std::clamp(int(settle_ / kSettle * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "HOLD %.*s", n, "******");
        hud(1, 24, buf, PAL_WIN);
    } else if (onGrass_) {
        int dist = std::max(0, int(std::lround(kEndY0 - y_)));
        std::snprintf(buf, sizeof buf, "END %d", dist);
        hud(1, 24, buf, PAL_MARK);
    } else {
        hud(1, 24, "LEG 1  —  THE GRASS", PAL_WIN);
    }
    hud(1, 27, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
}

}  // namespace metrograss
