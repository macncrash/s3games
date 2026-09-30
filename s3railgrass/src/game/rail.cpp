#include "rail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace railgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kTau = 6.2831853f;

constexpr float kStartY = 36.f;
constexpr float kGrassY0 = 214.f;
constexpr float kGrassY1 = 338.f;
constexpr float kEndY0 = 268.f;
constexpr float kEndY1 = 304.f;
constexpr float kEndMid = (kEndY0 + kEndY1) * 0.5f;
constexpr float kGrassHalf = 38.f;

constexpr float kStop = 0.30f;
constexpr float kSettle = 0.48f;
constexpr float kShort = 2.6f;
constexpr float kTitleZoom = 0.78f;
constexpr float kTitleCamX = 8.f;
constexpr float kTitleCamY = 168.f;
constexpr float kPlayZoom = 1.72f;

const float kTuft[][2] = {
    {-30.f, 230.f}, {34.f, 238.f}, {-36.f, 258.f}, {32.f, 276.f}, {-28.f, 292.f},
    {30.f, 312.f},  {-32.f, 326.f}, {28.f, 248.f}, {-22.f, 220.f}, {24.f, 332.f},
};
const float kLampY[] = {70.f, 130.f, 190.f, 250.f, 320.f};

float railX(float y) { return 28.f * std::sin((y - kStartY) * 0.0205f); }

float railHead(float y) {
    float dx = 28.f * 0.0205f * std::cos((y - kStartY) * 0.0205f);
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

int Game::carFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

const char* Game::hint() const {
    if (inEnd_) return std::fabs(speed_) > 1.f ? "BRAKE, THEN LET GO" : "HOLD THE FULL STOP";
    if (onGrass_) return "THE END IS THE PALE BAND";
    if (y_ > 150.f) return "THE GRADE RUNS YOU ON";
    return "STAY IN THE RAIL";
}

void Game::pose() {
    x_ = railX(y_);
    heading_ = railHead(y_);
}

void Game::begin() {
    y_ = kStartY;
    speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    clackT_ = 0.f;
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
    blip(520.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
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

void Game::human(float& throttle) {
    const gs::Pad& p = sys_->pad;
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.28f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_TURBO) ||
                      p.axisY < -0.28f || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.1f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.2f);
    else {
        float decay = std::fabs(speed_) < 0.45f ? 3.2f : 0.7f;
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * decay);
        else throttle_ = std::min(0.f, throttle_ + kDt * decay);
    }
    throttle = throttle_;
}

void Game::pilot(float& throttle) {
    float dist = kEndMid - y_;
    float want = 15.5f;
    if (y_ > 168.f) want = 11.f;
    if (onGrass_) want = std::clamp(dist * 0.28f, 0.f, 7.5f);
    if (y_ >= kEndY0 - 6.f) want = std::clamp(dist * 0.16f, 0.f, 3.2f);
    if (inEnd_) want = 0.f;
    if (speed_ > want + 0.35f) throttle = -1.f;
    else if (speed_ < want - 0.45f) throttle = 0.9f;
    else if (inEnd_ && speed_ > 0.2f) throttle = -0.55f;
    else throttle = 0.f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    std::printf("S3 RAIL GRASS  PASS  landed on the grass and came to a full stop  (%.1f s)\n", raceTime_);
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
    sys_->apu.noiseBurst(0.42f, 90.f, 0.4f);
    sys_->apu.tone(0, 74.f, 0.06f);
    tone0_ = 0.4f;
}

void Game::physics(float dt, float throttle) {
    raceTime_ += dt;
    const bool grassNow = y_ >= kGrassY0 && y_ <= kGrassY1;
    const bool grade = y_ >= 156.f && y_ < kGrassY0;

    if (grassNow && throttle < -0.02f) {
        float decel = (-throttle) * 4.6f;
        if (speed_ > 0.02f) speed_ = std::max(0.f, speed_ - decel * dt);
        else speed_ = 0.f;
    } else {
        float cap = grassNow ? 9.5f : 18.5f;
        if (!grassNow && throttle < 0.f) cap = 8.f;
        float target = std::max(0.f, throttle) * cap;
        if (!grassNow && throttle < 0.f) target = std::max(0.f, throttle * cap);
        float ak = grassNow ? 1.4f : 1.15f;
        speed_ += (target - speed_) * (1.f - std::exp(-ak * dt));
        if (grassNow && throttle < 0.08f && speed_ > 0.f) speed_ = std::max(0.f, speed_ - 1.6f * dt);
        if (!grassNow && throttle < -0.02f && speed_ > 0.f) speed_ = std::max(0.f, speed_ - (-throttle) * 3.1f * dt);
    }
    if (grade && throttle > -0.4f) speed_ += 7.4f * dt;
    speed_ = std::clamp(speed_, 0.f, 26.f);

    y_ += speed_ * dt;
    if (y_ < kStartY - 4.f) y_ = kStartY - 4.f;
    pose();

    onGrass_ = y_ >= kGrassY0 && y_ <= kGrassY1;
    inEnd_ = onGrass_ && y_ >= kEndY0 && y_ <= kEndY1;
    if (onGrass_ && !landed_) {
        landed_ = true;
        blip(440.f);
        sys_->rumble(0.4f, 0.2f, 110);
    }

    if (y_ > kGrassY1 + 3.f) {
        fail("missed the end");
        return;
    }
    if (raceTime_ > 70.f) {
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

    clackT_ -= dt;
    if (mode_ == Mode::Run && speed_ > 4.f && clackT_ <= 0.f) {
        clackT_ = std::clamp(0.22f - speed_ * 0.006f, 0.07f, 0.22f);
        sys_->apu.noiseBurst(0.12f, 900.f, 0.04f);
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
    float roll = mode_ == Mode::Run ? 0.01f + speed_ * 0.0007f : 0.006f;
    sys_->apu.noise(onGrass_ ? roll * 0.4f : roll, onGrass_ ? 220.f : 480.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || speed_ > 2.f)) {
        float wob = 0.65f + 0.35f * std::sin(t_ * (10.f + std::max(0.f, throttle_) * 16.f));
        float base = onGrass_ ? 42.f : 58.f;
        float vol = (0.012f + std::max(0.f, throttle_) * 0.03f) * wob;
        sys_->apu.tone(2, base + std::max(0.f, throttle_) * 22.f + speed_ * 0.35f, vol);
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
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
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
            blip(340.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float thr = 0.f;
            if (bot_) pilot(thr);
            else human(thr);
            throttle_ = thr;
            physics(kDt, throttle_);
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
    float lead = mode_ == Mode::Run ? 22.f : 0.f;
    float gy = y_ + lead;
    float gx = railX(gy);
    float k = 1.f - std::exp(-kDt * 4.4f);
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
    v.roadTime = int(t_ * 20.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.2f);
        float u = std::clamp((wy - 20.f) / 360.f, 0.f, 1.f);
        uint16_t land = lerpC(gs::rgb4(6, 8, 4), gs::rgb4(4, 5, 3), u);
        if (wy < kGrassY0) land = lerpC(gs::rgb4(7, 6, 4), gs::rgb4(4, 4, 3), u);
        v.lineBackdrop[y] = land;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        if (wy >= kGrassY0 && wy <= kGrassY1) {
            bool endBand = wy >= kEndY0 && wy <= kEndY1;
            r.on = true;
            r.cx = 160.f + (railX(wy) - camX_) * zoom_;
            r.hw = std::max(4.f, kGrassHalf * zoom_);
            r.v = wy * 18.f;
            r.pal = uint8_t(endBand ? PAL_ENDF : PAL_FIELD);
            r.band = (int(std::floor(wy * 0.16f)) & 1) ? 1 : 0;
            r.style = 0;
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
        float psx = 160.f + (railX(kEndMid) - camX_) * zoom_;
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
    float carH = 22.f * zoom_;
    if (mode_ == Mode::Title) carH = std::max(carH, 18.f);
    const gs::Mipped& body = art_.car[carFrame()];
    spr(body, bsx + 3.f, bsy + 4.f, carH, PAL_CAR, true);
    spr(body, bsx, bsy, carH, PAL_CAR, false);
    if (mode_ == Mode::Run && speed_ > 8.f && (int(t_ * 18.f) & 1)) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.spark, x_ - c * 6.f, y_ + 7.f * s, 3.2f, PAL_SPARK, 2.f);
    }

    const float markPx = mode_ == Mode::Title ? 6.f : 0.f;
    for (float sy = 24.f; sy <= 352.f; sy += 10.f) {
        float sx = railX(sy);
        float h = railHead(sy);
        float ox = std::cos(h);
        float oy = -std::sin(h);
        place(art_.sleeper, sx, sy, 3.2f, PAL_RAIL, 0.f);
        place(art_.dash, sx - ox * 5.5f, sy - oy * 5.5f, 1.5f, PAL_RAIL, 0.f);
        place(art_.dash, sx + ox * 5.5f, sy + oy * 5.5f, 1.5f, PAL_RAIL, 0.f);
    }
    for (const float* p : kTuft) place(art_.tuft, railX(p[1]) + p[0], p[1], 8.f, PAL_TUFT, markPx * 0.5f);
    for (float ly : kLampY) place(art_.lamp, railX(ly) - 16.f, ly, 10.f, PAL_LAMP, markPx * 0.5f);
    place(art_.signal, railX(kGrassY0) - 18.f, kGrassY0 - 4.f, 14.f, PAL_MARK, markPx);
    place(art_.signal, railX(kEndY0) + 16.f, kEndY0 - 2.f, 14.f, PAL_MARK, markPx);
    place(art_.halt, railX(kEndMid) - 26.f, kEndMid + 4.f, 20.f, PAL_WOOD, markPx);
    place(art_.flag, railX(kEndY0) - 14.f, kEndY0, 12.f, PAL_MARK, markPx);
    place(art_.flag, railX(kEndY1) + 14.f, kEndY1, 12.f, PAL_MARK, markPx);

    auto dashes = [&](float y, int n) {
        float x0 = railX(y) - 12.f;
        float x1 = railX(y) + 12.f;
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dash, x0 + (x1 - x0) * u, y, 2.2f, PAL_MARK, 0.f);
        }
    };
    dashes(kEndY0, 5);
    dashes(kEndY1, 5);
    if ((onGrass_ || mode_ == Mode::Title) && mode_ != Mode::Win)
        place(art_.ring, railX(kEndMid), kEndMid, 26.f, PAL_MARK, mode_ == Mode::Title ? 8.f : 0.f);

    int flap = int(t_ * 4.f) & 1;
    place(art_.bird[flap], railX(250.f) + std::sin(t_ * 0.5f) * 20.f, 250.f, 6.f, PAL_BIRD, markPx);
    place(art_.bird[1 - flap], railX(100.f) + 24.f, 100.f + std::sin(t_ * 0.3f) * 6.f, 5.f, PAL_BIRD, markPx);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        auto chart = [&](float wy, int pal, float h) {
            spr(art_.dot, 286.f + railX(wy) * 0.35f, 58.f - (wy - 180.f) * 0.16f, h, pal, false);
        };
        chart(kGrassY0, PAL_WIN, 3.f);
        chart(kGrassY1, PAL_WIN, 3.f);
        chart(kEndMid, PAL_MARK, 5.f);
        chart(y_, PAL_ALERT, 5.f);
        spr(art_.panel, 286.f, 56.f, 68.f, PAL_MAP, false);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(23, "LAND ON THE GRASS", PAL_WIN);
        hudC(24, "COME TO A FULL STOP", PAL_BANNER);
        hudC(25, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        else hudC(27, "UP THROTTLE   DOWN BRAKE   IN THE RAIL", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 RAIL GRASS", PAL_BANNER);
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
    std::snprintf(buf, sizeof buf, "SPD %02d  %s", sp, onGrass_ ? "GRASS" : "RAIL");
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

}  // namespace railgrass
