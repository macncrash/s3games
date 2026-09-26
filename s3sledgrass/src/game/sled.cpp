#include "game/sled.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace sledgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;

constexpr float kBowY0 = 30.f;
constexpr float kBowLen = 166.f;
constexpr float kBowAmp = 28.f;
constexpr float kStartY = 44.f;
constexpr float kSnowHalf = 21.f;

constexpr float kGrassY0 = 214.f;
constexpr float kGrassY1 = 336.f;
constexpr float kGrassX = 34.f;
constexpr float kEndY0 = 250.f;
constexpr float kEndY1 = 312.f;
constexpr float kEndMid = (kEndY0 + kEndY1) * 0.5f;
constexpr float kFenceY = 326.f;

constexpr float kStop = 0.48f;
constexpr float kSettle = 0.42f;
constexpr float kShort = 3.2f;
constexpr float kLimit = 96.f;

constexpr float kPlayZoom = 1.6f;
constexpr float kTitleZoom = 0.84f;
constexpr float kTitleCamX = 14.f;
constexpr float kTitleCamY = 128.f;
constexpr float kTeamH = 46.f;

struct Spot {
    float x, y;
};

Spot gPine[24];
int gPineN = 0;
Spot gTuft[12];
int gTuftN = 0;
Spot gDrift[8];
int gDriftN = 0;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float trailX(float y) {
    float u = std::clamp((y - kBowY0) / kBowLen, 0.f, 1.f);
    float hump = 0.5f * (1.f - std::cos(kTau * u));
    return hump * kBowAmp;
}

float trailSlope(float y) {
    float u = std::clamp((y - kBowY0) / kBowLen, 0.f, 1.f);
    if (u <= 0.f || u >= 1.f) return 0.f;
    float dhdu = kPi * std::sin(kTau * u);
    return dhdu * kBowAmp / kBowLen;
}

bool grassAt(float x, float y) {
    return std::fabs(x) <= kGrassX && y >= kGrassY0 && y <= kGrassY1;
}

bool pocketAt(float x, float y) { return std::fabs(x) <= kGrassX && y >= kEndY0 && y <= kEndY1; }

void addPine(float x, float y) {
    if (gPineN < 24) gPine[gPineN++] = {x, y};
}

void bakeWorld() {
    if (gPineN) return;
    for (int i = 0; i < 8; i++) {
        float y = 58.f + float(i) * 16.f;
        float c = trailX(y);
        addPine(c - 46.f, y);
        addPine(c + 46.f, y + 4.f);
    }
    addPine(-52.f, 228.f);
    addPine(52.f, 244.f);
    addPine(-50.f, 292.f);
    addPine(54.f, 318.f);

    const float tuftY[6] = {222.f, 238.f, 258.f, 278.f, 298.f, 318.f};
    for (int i = 0; i < 6; i++) {
        float s = (i & 1) ? 1.f : -1.f;
        if (gTuftN < 12) gTuft[gTuftN++] = {s * 26.f, tuftY[i]};
        if (gTuftN < 12) gTuft[gTuftN++] = {-s * 20.f, tuftY[i] + 7.f};
    }
    const float driftY[4] = {72.f, 108.f, 146.f, 184.f};
    for (float y : driftY) {
        if (gDriftN < 8) gDrift[gDriftN++] = {trailX(y) - 32.f, y};
        if (gDriftN < 8) gDrift[gDriftN++] = {trailX(y) + 32.f, y + 5.f};
    }
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

const char* orderName(float throttle) {
    if (throttle > 0.55f) return "MUSH";
    if (throttle > 0.08f) return "EASY";
    if (throttle < -0.45f) return "WHOA";
    if (throttle < -0.05f) return "DRAG";
    return "COAST";
}

bool tracing() {
    static int on = -1;
    if (on < 0) on = std::getenv("SLEDGRASS_TRACE") != nullptr;
    return on != 0;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inEnd_) return 3;
    if (onGrass_) return 2;
    return 1;
}

int Game::teamFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    return i;
}

const char* Game::hint() const {
    if (inEnd_) return std::fabs(speed_) > 1.f ? "WHOA TO A FULL STOP" : "HOLD THE FULL STOP";
    if (onGrass_) return "THE END IS THE PALE BAND";
    if (y_ > kGrassY0 - 55.f) return "LAND ON THE GRASS";
    return "FOLLOW THE SNOW";
}

void Game::begin() {
    float slope = trailSlope(kStartY);
    x_ = trailX(kStartY);
    y_ = kStartY;
    heading_ = std::atan2(1.f, slope);
    vx_ = vy_ = pace_ = speed_ = 0.f;
    throttle_ = 0.f;
    raceTime_ = 0.f;
    settle_ = 0.f;
    short_ = 0.f;
    puffT_ = 0.f;
    stuckT_ = 0.f;
    stuckX_ = x_;
    stuckY_ = y_;
    puffCursor_ = 0;
    onGrass_ = false;
    inEnd_ = false;
    landed_ = false;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    chimeStep_ = 0;
    why_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
    for (Puff& p : puffs_) p = {};
    for (int i = 0; i < 14; i++) {
        flakes_[i].x = float((i * 47) % 320);
        flakes_[i].y = float((i * 29) % 224);
        flakes_[i].v = 16.f + float(i % 5) * 7.f;
        flakes_[i].w = 2.f + float(i % 3);
    }
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
    blip(620.f);
    sys_->setLight(150, 180, 200);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    bakeWorld();
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(10, 12, 14));
    sys.apu.setMaster(0.74f);
    sys.apu.setEcho(0.16f, 0.20f, 0.08f);
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
    if (std::fabs(p.axisX) > 0.15f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) ||
                    p.axisY > 0.28f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.axisY < -0.28f || p.brake > 0.2f;
    float target = 0.f;
    if (stop) target = -1.f;
    else if (go) target = 1.f;
    float rate = stop ? 8.f : 5.f;
    throttle_ += (target - throttle_) * (1.f - std::exp(-rate * kDt));
    throttle = throttle_;
    if (p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Z)) yip();
}

void Game::pilot(float& steer, float& throttle) {
    auto seek = [&](float tx, float ty, float want) {
        float dx = tx - x_, dy = ty - y_;
        float err = std::hypot(dx, dy) < 3.f ? wrap(kNorth - heading_) : wrap(std::atan2(dy, dx) - heading_);
        steer = std::clamp(err / 0.42f, -1.f, 1.f);
        float sp = std::fabs(pace_);
        if (sp > want + 1.3f) throttle = -0.75f;
        else if (sp < want - 0.7f) throttle = 1.f;
        else throttle = 0.62f;
        if (std::fabs(err) > 1.0f) throttle = std::min(throttle, 0.25f);
    };

    if (y_ < kGrassY0 - 1.f) {
        float ty = std::min(y_ + 22.f, kGrassY0 + 8.f);
        float tx = ty < kGrassY0 ? trailX(ty) : 0.f;
        float want = y_ > kGrassY0 - 48.f ? 7.2f : 11.5f;
        seek(tx, ty, want);
        float latErr = x_ - trailX(y_);
        steer = std::clamp(steer + latErr / 20.f, -1.f, 1.f);
        return;
    }
    if (y_ < kEndY0) {
        seek(0.f, kEndMid, 5.6f);
        steer = std::clamp(steer + x_ / 22.f, -1.f, 1.f);
        if (pace_ < 4.2f) throttle = 1.f;
        else if (pace_ > 7.2f) throttle = -0.35f;
        else if (throttle < 0.4f) throttle = 0.7f;
        return;
    }
    if (y_ > kEndY1) {
        float aim = std::atan2(1.f, std::clamp(-x_ * 0.05f, -0.3f, 0.3f));
        steer = std::clamp(wrap(aim - heading_) / 0.3f, -1.f, 1.f);
        if (pace_ > -1.5f) throttle = -1.f;
        else throttle = -0.4f;
        return;
    }
    if (std::fabs(pace_) > 0.8f) {
        float aim = std::atan2(1.f, std::clamp(-x_ * 0.045f, -0.35f, 0.35f));
        steer = std::clamp(wrap(aim - heading_) / 0.32f, -1.f, 1.f);
    } else {
        steer = 0.f;
    }
    if (pace_ > 0.36f) throttle = -1.f;
    else if (pace_ < -0.22f) throttle = 0.45f;
    else throttle = 0.f;
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = vy_ = pace_ = speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    sys_->setLight(40, 180, 70);
    sys_->rumble(0.25f, 0.55f, 160);
    chime(5);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    vx_ = vy_ = pace_ = speed_ = 0.f;
    throttle_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.4f, 90.f, 0.38f);
    sys_->apu.tone(0, 74.f, 0.06f);
    tone0_ = 0.4f;
    sys_->setLight(180, 40, 20);
}

void Game::physics(float dt, float steer, float throttle) {
    raceTime_ += dt;
    throttle_ = throttle;
    const bool grass = grassAt(x_, y_);
    const bool past = y_ > kEndY1 && y_ < kFenceY + 8.f && std::fabs(x_) <= kGrassX + 4.f;

    float c = std::cos(heading_), s = std::sin(heading_);
    float fwd = vx_ * c + vy_ * s;
    float lat = -vx_ * s + vy_ * c;

    if (throttle > 0.02f) {
        float cap = grass ? 7.4f : 14.2f;
        float target = throttle * cap;
        float ak = grass ? 2.8f : 3.3f;
        fwd += (target - fwd) * (1.f - std::exp(-ak * dt));
    } else if (throttle < -0.02f) {
        if (fwd > 0.05f) {
            float brake = grass ? 12.f : 4.2f;
            fwd = std::max(0.f, fwd - (-throttle) * brake * dt);
        } else if (past) {
            fwd = std::max(-2.3f, fwd - (-throttle) * 1.8f * dt);
        } else {
            fwd = std::min(fwd, 0.f);
        }
    }

    float lin = grass ? 0.40f : 0.10f;
    float coul = grass ? 0.85f : 0.05f;
    fwd *= std::exp(-lin * dt);
    if (fwd > 0.f) fwd = std::max(0.f, fwd - coul * dt);
    else if (fwd < 0.f) fwd = std::min(0.f, fwd + coul * dt);

    if (!grass && y_ < kGrassY0 + 2.f) {
        float off = std::fabs(x_ - trailX(y_)) - kSnowHalf;
        if (off > 2.f && fwd > 0.f) fwd = std::max(0.f, fwd - std::min(off, 24.f) * 0.85f * dt);
    }
    if (!grass) lat += steer * fwd * 0.40f * dt;

    float grip = grass ? 7.f : 1.8f;
    lat *= std::exp(-grip * dt);
    fwd = std::clamp(fwd, -3.f, 16.f);
    lat = std::clamp(lat, -8.f, 8.f);

    float sp = std::fabs(fwd);
    float yaw = steer * (0.55f + std::min(sp, 14.f) * 0.06f);
    if (sp < 0.8f) yaw *= 0.35f;
    heading_ = wrap(heading_ + yaw * dt);
    c = std::cos(heading_);
    s = std::sin(heading_);
    vx_ = c * fwd - s * lat;
    vy_ = s * fwd + c * lat;
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    pace_ = fwd;

    auto bump = [&](float rx, float ry, float rad) {
        float dx = x_ - rx, dy = y_ - ry;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.01f) {
            x_ = rx + dx / d * rad;
            y_ = ry + dy / d * rad;
            vx_ *= 0.45f;
            vy_ *= 0.45f;
            pace_ *= 0.45f;
            if (thumpT_ <= 0.f) {
                sys_->apu.noiseBurst(0.26f, 180.f, 0.12f);
                thumpT_ = 0.28f;
            }
        }
    };
    for (int i = 0; i < gPineN; i++) bump(gPine[i].x, gPine[i].y, 6.2f);

    if (y_ > kFenceY - 2.f && y_ < kFenceY + 6.f && std::fabs(x_) <= kGrassX + 1.f && vy_ > 0.f) {
        y_ = kFenceY - 2.f;
        float f = vx_ * c + vy_ * s;
        float l = -vx_ * s + vy_ * c;
        if (f > 0.f) f = 0.f;
        vx_ = c * f - s * l;
        vy_ = s * f + c * l;
        pace_ = f;
        if (thumpT_ <= 0.f) {
            sys_->apu.noiseBurst(0.36f, 140.f, 0.18f);
            thumpT_ = 0.32f;
            sys_->rumble(0.45f, 0.2f, 80);
        }
    }
    if (x_ < -120.f) {
        x_ = -120.f;
        vx_ *= 0.2f;
    } else if (x_ > 130.f) {
        x_ = 130.f;
        vx_ *= 0.2f;
    }
    if (y_ < 16.f) {
        y_ = 16.f;
        if (vy_ < 0.f) vy_ = 0.f;
    }

    onGrass_ = grassAt(x_, y_);
    inEnd_ = pocketAt(x_, y_);
    speed_ = std::hypot(vx_, vy_);
    if (onGrass_ && !landed_) {
        landed_ = true;
        blip(480.f);
        sys_->rumble(0.4f, 0.2f, 100);
        sys_->setLight(30, 140, 50);
    }

    if (y_ > kGrassY1 + 1.f) {
        fail("missed the end");
        return;
    }
    if (y_ > kGrassY0 + 4.f && y_ < kGrassY1 + 8.f && std::fabs(x_) > kGrassX + 5.f) {
        fail("off the grass");
        return;
    }
    if (raceTime_ > kLimit) {
        fail("timed out");
        return;
    }

    if (inEnd_ && speed_ <= kStop && throttle_ <= 0.25f) {
        settle_ += dt;
        vx_ *= std::exp(-6.f * dt);
        vy_ *= std::exp(-6.f * dt);
        pace_ *= std::exp(-6.f * dt);
        speed_ = std::hypot(vx_, vy_);
        if (settle_ >= kSettle) {
            succeed();
            return;
        }
    } else if (!(inEnd_ && speed_ <= kStop)) {
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

    puffT_ -= dt;
    float kick = std::fabs(pace_);
    if (puffT_ <= 0.f && kick > 3.5f) {
        puffT_ = grass ? 0.09f : 0.06f;
        Puff w;
        w.x = x_ - c * 10.f;
        w.y = y_ - s * 10.f;
        w.life = 1.f;
        w.grass = grass ? 1 : 0;
        puffs_[puffCursor_] = w;
        puffCursor_ = (puffCursor_ + 1) % 16;
    }
    for (Puff& w : puffs_)
        if (w.life > 0.f) w.life -= dt;

    if (bot_ && !inEnd_ && y_ < kEndY0) {
        stuckT_ += dt;
        if (stuckT_ > 1.5f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0.f;
            if (moved < 3.f) {
                float ty = y_ + 24.f;
                float tx = ty < kGrassY0 ? trailX(ty) : 0.f;
                heading_ = std::atan2(ty - y_, tx - x_);
                float hc = std::cos(heading_), hs = std::sin(heading_);
                vx_ = hc * 7.f;
                vy_ = hs * 7.f;
                pace_ = 7.f;
            }
        }
    }

    if (tracing() && sys_->frame % 30 == 0) {
        std::fprintf(stderr, "t %.1f x %.1f y %.1f hdg %.2f pace %.2f thr %.2f g %d e %d\n", raceTime_, x_, y_,
                     heading_, pace_, throttle_, onGrass_ ? 1 : 0, inEnd_ ? 1 : 0);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.09f;
}

void Game::yip() {
    sys_->apu.noiseBurst(0.18f, 900.f, 0.05f);
    sys_->apu.tone(1, 680.f, 0.04f);
    tone1_ = 0.08f;
    yipT_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    float hiss = mode_ == Mode::Run ? 0.012f + std::fabs(pace_) * 0.0016f : 0.006f;
    sys_->apu.noise(hiss, onGrass_ ? 240.f : 720.f, false);
    if (mode_ == Mode::Run && (throttle_ > 0.05f || std::fabs(pace_) > 2.f)) {
        float wob = 0.65f + 0.35f * std::sin(t_ * (9.f + std::max(0.f, throttle_) * 14.f));
        float base = onGrass_ ? 48.f : 62.f;
        float vol = (0.012f + std::max(0.f, throttle_) * 0.03f) * wob;
        sys_->apu.tone(2, base + std::max(0.f, throttle_) * 36.f + std::fabs(pace_) * 1.4f, vol);
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
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (yipT_ > 0.f) yipT_ -= dt;
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {330.f, 392.f, 494.f, 587.f, 784.f};
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
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) ||
            pad.pressed(gs::BTN_TURBO))
            startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(340.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            physics(kDt, steer, thr);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        if (sys.hasHome()) sys.eject();
        else showTitle();
    }
    flurry(kDt);
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
    float lead = mode_ == Mode::Run ? 16.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.4f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
}

void Game::flurry(float dt) {
    for (Flake& f : flakes_) {
        f.y += f.v * dt;
        f.x += std::sin(t_ * 0.8f + f.y * 0.02f) * 10.f * dt;
        if (f.y > 230.f) f.y = -4.f;
        if (f.x > 328.f) f.x = -4.f;
        if (f.x < -8.f) f.x = 324.f;
    }
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
    v.roadTime = int(t_ * 24.f);
    const float zoom = std::max(zoom_, 0.25f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / zoom;
        uint16_t sky = gs::rgb4(9, 12, 14);
        if (wy >= kGrassY0 && wy <= kGrassY1) sky = gs::rgb4(1, 5, 2);
        else if (wy > kGrassY1) sky = gs::rgb4(5, 6, 3);
        else {
            float u = std::clamp((wy - 10.f) / 220.f, 0.f, 1.f);
            sky = lerpC(gs::rgb4(12, 14, 15), gs::rgb4(7, 10, 13), u);
        }
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        gs::RoadLine& r = v.road[y];
        r = gs::RoadLine{};
        if (wy >= kGrassY0 && wy <= kGrassY1) {
            bool band = wy >= kEndY0 && wy <= kEndY1;
            r.on = true;
            r.cx = 160.f + (0.f - camX_) * zoom_;
            r.hw = std::max(2.f, kGrassX * zoom_);
            r.v = wy * 18.f;
            r.pal = uint8_t(band ? PAL_END : PAL_FIELD);
            r.band = (int(std::floor(wy * 0.16f)) & 1) ? 1 : 0;
            r.style = 0;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        } else if (wy < kGrassY0 && wy > 8.f) {
            float cx = trailX(wy);
            r.on = true;
            r.cx = 160.f + (cx - camX_) * zoom_;
            r.hw = std::max(2.f, kSnowHalf * zoom_);
            r.v = wy * 16.f;
            r.pal = PAL_SNOW;
            r.band = (int(std::floor(wy * 0.12f)) & 1) ? 1 : 0;
            r.style = gs::ROAD_SNOW;
            r.left = gs::GROUND_SNOWWALL;
            r.right = gs::GROUND_SNOWWALL;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 16.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 78.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.missed;
        if (std::strcmp(why_, "off the grass") == 0) msg = &art_.offGrass;
        else if (std::strcmp(why_, "timed out") == 0) msg = &art_.timed;
        banner(*msg, 160.f, 36.f, PAL_ALERT);
        banner(art_.legFail, 160.f, 64.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.fullStop, 160.f, 34.f, PAL_WIN);
        banner(art_.onGrass, 160.f, 62.f, PAL_WIN);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        float psx = 160.f + (0.f - camX_) * zoom_;
        float psy = 112.f - (kEndMid - camY_) * zoom_;
        if (psx < 16.f || psx > 304.f || psy < 16.f || psy > 208.f) {
            float dx = psx - 160.f, dy = psy - 112.f;
            float k = 1.f;
            if (std::fabs(dx) > 1.f) k = std::min(k, 142.f / std::fabs(dx));
            if (std::fabs(dy) > 1.f) k = std::min(k, 90.f / std::fabs(dy));
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 11.f, PAL_MARK, false);
        }
        auto chart = [&](float wx, float wy, int pal, float h) {
            spr(art_.dot, 286.f + wx * 0.52f, 118.f - (wy - 20.f) * 0.24f, h, pal, false);
        };
        chart(-kGrassX, kGrassY0, PAL_WIN, 3.f);
        chart(kGrassX, kGrassY0, PAL_WIN, 3.f);
        chart(-kGrassX, kGrassY1, PAL_WIN, 3.f);
        chart(kGrassX, kGrassY1, PAL_WIN, 3.f);
        chart(0.f, kEndY0, PAL_MARK, 3.f);
        chart(0.f, kEndY1, PAL_MARK, 3.f);
        chart(x_, y_, PAL_ALERT, 5.f);
        spr(art_.panel, 286.f, 78.f, 86.f, PAL_MAP, false);
    }

    float bob = onGrass_ ? 0.f : std::sin(t_ * 2.4f) * 0.6f;
    float tsx = 160.f + (x_ - camX_) * zoom_;
    float tsy = 112.f - (y_ - camY_) * zoom_ + bob;
    float teamH = kTeamH * zoom_;
    if (mode_ == Mode::Title) teamH = std::max(teamH, 26.f);
    const gs::Mipped& team = art_.team[teamFrame()];
    spr(team, tsx + 3.f, tsy + 3.f, teamH, PAL_TEAM, true);
    spr(team, tsx, tsy, teamH, PAL_TEAM, false);

    for (const Puff& w : puffs_) {
        if (w.life <= 0.f) continue;
        float h = (2.2f + (1.f - w.life) * 3.5f) * (zoom_ / kPlayZoom);
        float sx = 160.f + (w.x - camX_) * zoom_;
        float sy = 112.f - (w.y - camY_) * zoom_;
        spr(art_.spray, sx, sy, std::max(2.f, h), w.grass ? PAL_TUFT : PAL_SPRAY, false);
    }

    const float markPx = mode_ == Mode::Title ? 7.f : 0.f;
    for (int i = 0; i < gDriftN; i++) place(art_.drift, gDrift[i].x, gDrift[i].y, 8.f, PAL_DRIFT, markPx * 0.4f);
    for (int i = 0; i < gTuftN; i++) place(art_.tuft, gTuft[i].x, gTuft[i].y, 7.f, PAL_TUFT, markPx * 0.5f);
    for (int i = 0; i < gPineN; i++) place(art_.pine, gPine[i].x, gPine[i].y, 18.f, PAL_PINE, markPx);
    place(art_.cabin, -62.f, 50.f, 22.f, PAL_WOOD, markPx);
    place(art_.cabin, 58.f, 196.f, 16.f, PAL_WOOD, markPx);
    place(art_.spray, -58.f, 64.f + std::sin(t_ * 1.3f) * 1.5f, 4.f, PAL_SPRAY, 0.f);

    place(art_.flag, -kGrassX + 3.f, kEndY0, 12.f, PAL_MARK, markPx);
    place(art_.flag, kGrassX - 3.f, kEndY0, 12.f, PAL_MARK, markPx);
    place(art_.flag, -kGrassX + 3.f, kEndY1, 12.f, PAL_MARK, markPx);
    place(art_.flag, kGrassX - 3.f, kEndY1, 12.f, PAL_MARK, markPx);
    for (int i = -2; i <= 2; i++) {
        place(art_.post, float(i) * 14.f, kFenceY, 8.f, PAL_WOOD, markPx * 0.6f);
        if (i < 2) place(art_.rail, float(i) * 14.f + 7.f, kFenceY - 1.f, 3.2f, PAL_WOOD, 0.f);
    }

    auto dashes = [&](float x0, float y0, float x1, float y1, int n) {
        for (int i = 0; i < n; i++) {
            float u = n == 1 ? 0.5f : float(i) / float(n - 1);
            place(art_.dot, x0 + (x1 - x0) * u, y0 + (y1 - y0) * u, 1.8f, PAL_MARK, 0.f);
        }
    };
    dashes(-kGrassX, kEndY0, kGrassX, kEndY0, 8);
    dashes(-kGrassX, kEndY1, kGrassX, kEndY1, 8);
    if (mode_ != Mode::Win) place(art_.ring, 0.f, kEndMid, 18.f, PAL_MARK, mode_ == Mode::Title ? 8.f : 0.f);
    if (zoom_ > 1.05f || mode_ == Mode::Title) place(art_.endWord, 0.f, kEndMid + 8.f, 8.f, PAL_MARK, markPx);

    int flap = int(t_ * 3.f) & 1;
    place(art_.raven[flap], -20.f + std::sin(t_ * 0.35f) * 18.f, 250.f + std::cos(t_ * 0.22f) * 8.f, 6.f, PAL_RAVEN,
          markPx);
    place(art_.raven[1 - flap], 36.f + std::cos(t_ * 0.28f) * 14.f, 120.f, 5.f, PAL_RAVEN, markPx);

    for (const Flake& f : flakes_) spr(art_.dot, f.x, f.y, f.w, PAL_DRIFT, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "LAND ON THE GRASS", PAL_WIN);
        hudC(23, "COME TO A FULL STOP", PAL_BANNER);
        hudC(24, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "UP MUSH   DOWN WHOA   ARROWS STEER", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 SLED GRASS", PAL_BANNER);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hud(33, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(16, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(14, buf, PAL_HUD);
        if (!bot_) hudC(16, "START RUNS THE LEG AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(14, why_, PAL_ALERT);
        if (!bot_) hudC(16, "START TRIES THE LEG AGAIN", PAL_HUD);
        return;
    }
    hud(1, 1, hint(), inEnd_ ? PAL_WIN : PAL_BANNER);
    int sp = int(std::lround(std::fabs(speed_)));
    const char* surf = inEnd_ ? "END" : onGrass_ ? "GRASS" : "SNOW";
    std::snprintf(buf, sizeof buf, "PACE %02d  %s  %s", sp, orderName(throttle_), surf);
    hud(1, 2, buf, onGrass_ ? PAL_WIN : PAL_HUD);
    if (inEnd_) {
        int n = std::clamp(int(settle_ / kSettle * 6.f), 0, 6);
        std::snprintf(buf, sizeof buf, "HOLD %.*s", n, "******");
        hud(1, 24, buf, PAL_WIN);
    } else if (onGrass_) {
        int dist = std::max(0, int(std::lround(kEndY0 - y_)));
        if (y_ > kEndY1) hud(1, 24, "BACK INTO THE BAND", PAL_ALERT);
        else {
            std::snprintf(buf, sizeof buf, "END %d", dist);
            hud(1, 24, buf, PAL_MARK);
        }
    } else {
        hud(1, 24, "LEG 1  -  THE GRASS", PAL_WIN);
    }
    hud(1, 27, "MISS THE END AND THE LEG FAILS", PAL_ALERT);
}

}  // namespace sledgrass
