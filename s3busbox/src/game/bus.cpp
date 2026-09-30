#include "bus.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace busbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStop = 0.16f;
constexpr float kHoldNeed = 0.55f;
constexpr float kOutNeed = 0.7f;
constexpr float kGo = 2.35f;
constexpr float kBrake = 5.6f;
constexpr float kBack = 1.7f;
constexpr float kDrag = 0.32f;
constexpr float kRoll = 0.14f;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
float signf(float v) { return v < 0.f ? -1.f : v > 0.f ? 1.f : 0.f; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (bodyInside()) return 2;
    return 1;
}

bool Game::bodyInside() const {
    return (x_ - kRear) >= kBoxL && (x_ + kFront) <= kBoxR;
}

bool Game::nosePastEnd() const { return (x_ + kFront) >= kEnd; }

void Game::begin() {
    x_ = 18.f;
    vel_ = 0.35f;
    hold_ = 0;
    outT_ = 0;
    race_ = 0;
    odo_ = 0;
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = chimeStep_ = dustCursor_ = 0;
    tone0_ = chimeT_ = thumpT_ = 0;
    dustT_ = shake_ = goIn_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : dust_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 42.f;
    x_ = 30.f;
    vel_ = 0;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    blip(220.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(5, 6, 8));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.05f, 0.08f, 0.03f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    tone0_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = std::max(1, std::min(notes, 6));
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::controls(float& go, float& brake, float& back) {
    const gs::Pad& p = sys_->pad;
    go = brake = back = 0;
    bool accel = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_UP) || p.down(gs::BTN_C);
    bool stop = p.down(gs::BTN_LEFT) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_A) || p.down(gs::BTN_B);
    if (p.accel > 0.12f) accel = true;
    if (p.brake > 0.12f) stop = true;
    if (p.axisX > 0.25f) accel = true;
    if (p.axisX < -0.25f) stop = true;
    if (accel && !stop) go = 1.f;
    else if (stop && !accel) {
        if (vel_ > 0.15f) brake = 1.f;
        else back = 1.f;
    }
    if (p.accel > 0.12f && go > 0.f) go = clampf(p.accel, 0.f, 1.f);
}

void Game::pilot(float& go, float& brake, float& back) {
    go = brake = back = 0;
    float dx = kGoal - x_;
    float room = (kEnd - 0.45f) - (x_ + kFront);
    float stopDist = (vel_ * vel_) / (2.f * 4.4f);
    if (vel_ > 0.05f && stopDist > room) {
        brake = 1.f;
        phase_ = 1;
        return;
    }
    if (!bodyInside()) {
        phase_ = x_ + kFront > kBoxL ? 1 : 0;
        if (dx > 0.4f) {
            float want = clampf(dx * 0.42f, 1.1f, 6.4f);
            if (dx < 12.f) want = clampf(dx * 0.38f, 0.55f, 3.2f);
            float err = want - vel_;
            if (err > 0.2f) go = clampf(err * 0.45f, 0.2f, 1.f);
            else if (err < -0.12f) brake = clampf(-err * 0.55f, 0.15f, 1.f);
        } else if (vel_ > 0.08f) {
            brake = 1.f;
        } else {
            back = 0.7f;
        }
    } else {
        phase_ = 2;
        float want = clampf(dx * 1.4f, -1.1f, 1.1f);
        if (std::fabs(dx) < 0.12f) want = 0.f;
        float err = want - vel_;
        if (err > 0.08f) go = clampf(err * 0.8f, 0.f, 0.7f);
        else if (err < -0.08f) {
            if (vel_ > 0.04f) brake = clampf(-err, 0.f, 1.f);
            else back = clampf(-err * 0.45f, 0.f, 0.55f);
        }
    }
}

void Game::physics(float go, float brake, float back) {
    float a = go * kGo - back * kBack;
    if (brake > 0.f) {
        if (std::fabs(vel_) < 0.07f) vel_ = 0.f;
        else a -= brake * kBrake * signf(vel_);
    }
    a -= vel_ * kDrag;
    if (std::fabs(vel_) > 0.02f) a -= kRoll * signf(vel_);
    vel_ += a * kDt;
    vel_ = clampf(vel_, -1.8f, 7.6f);
    x_ += vel_ * kDt;
    odo_ += std::fabs(vel_) * kDt;
    if (!std::isfinite(x_) || !std::isfinite(vel_)) {
        fail("lost the street");
        return;
    }
    if (nosePastEnd()) {
        fail("missed the end");
        return;
    }
    if (x_ - kRear < kWest) {
        x_ += kWest - (x_ - kRear);
        vel_ *= -0.08f;
        if (thumpT_ <= 0.f) {
            thumpT_ = 0.18f;
            shake_ = std::max(shake_, 0.4f);
            sys_->apu.noiseBurst(0.14f, 180.f, 0.08f);
        }
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(why_, sizeof why_, "stopped inside the box");
    banner_ = &art_.stopped;
    chime(4);
    sys_->rumble(0.26f, 0.08f, 140);
    sys_->setLight(40, 160, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    bool missed = why && std::strcmp(why, "missed the end") == 0;
    banner_ = missed ? &art_.missed : &art_.outside;
    shake_ = 0.75f;
    sys_->rumble(0.5f, 0.2f, 160);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.3f, 120.f, 0.24f);
    sys_->apu.tone(0, 64.f, 0.05f);
    tone0_ = 0.3f;
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    bool in = bodyInside();
    if (in && !announced_) {
        announced_ = true;
        blip(392.f);
    }
    if (in && std::fabs(vel_) <= kStop) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else if (!in && std::fabs(vel_) <= kStop && race_ > 1.1f) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            if (x_ + kFront < kBoxL) fail("stopped short of the box");
            else if (x_ - kRear > kBoxR) fail("stopped past the box");
            else fail("stopped with the bus sticking out");
        }
    } else {
        hold_ = 0;
        outT_ = 0;
    }
}

void Game::audio() {
    float roll = mode_ == Mode::Run ? 0.012f + std::fabs(vel_) * 0.004f : 0.005f;
    sys_->apu.noise(roll, 280.f + std::fabs(vel_) * 28.f, false);
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            sys_->apu.tone(1, notes[chimeStep_ % 4], 0.06f);
            chimeStep_++;
            chimeT_ = 0.16f;
            if (chimeStep_ >= chimeN_) {
                chimeN_ = 0;
                sys_->apu.tone(1, 0, 0);
            }
        }
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (mode_ == Mode::Run && std::fabs(vel_) > 0.4f) {
        float rate = 90.f + std::fabs(vel_) * 18.f;
        sys_->apu.tone(2, rate, 0.012f);
    } else {
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_A)) startRun();
        else if (p.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (p.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C))) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
        } else {
            float go = 0, brake = 0, back = 0;
            if (bot_) pilot(go, brake, back);
            else controls(go, brake, back);
            goIn_ = go;
            physics(go, brake, back);
            race_ += kDt;
            if (mode_ == Mode::Run) judge();
            if (go > 0.4f && std::fabs(vel_) > 0.8f) {
                dustT_ -= kDt;
                if (dustT_ <= 0.f) {
                    dustT_ = 0.08f;
                    Puff& d = dust_[dustCursor_++ % 8];
                    d.x = x_ - kRear;
                    d.life = 0.7f;
                    d.rise = 0;
                }
            }
        }
    }

    for (Puff& d : dust_) {
        if (d.life > 0.f) {
            d.life -= kDt * 0.65f;
            d.rise += kDt * 5.f;
        }
    }

    if (mode_ == Mode::Win || hold_ > 0.05f) sys.setLight(40, 160, 60);
    else if (mode_ == Mode::Fail) sys.setLight(170, 40, 28);
    else if (mode_ == Mode::Run && x_ + kFront > kEnd - 4.f) sys.setLight(170, 70, 24);
    else if (mode_ == Mode::Run && bodyInside()) sys.setLight(40, 130, 70);
    else sys.setLight(30, 70, 120);

    float want = mode_ == Mode::Title ? 48.f : x_;
    camX_ += (want - camX_) * (1.f - std::exp(-kDt * 3.0f));
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 42.f) * shake_ * 0.45f;
        shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    }
    audio();
    draw();
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

void Game::drawHud() {
    char buf[80];
    if (mode_ == Mode::Title) {
        hudC(21, "STOP THE BUS INSIDE THE BOX", PAL_HUD);
        hudC(22, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(23, "RIGHT DRIVES    LEFT BRAKES", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "RETURN", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 BUS BOX", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "LEG %04.0f", odo_);
    hud(30, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "STOPPED IN THE BOX", PAL_WIN);
        hudC(16, "THE LEG IS IN", PAL_HUD);
        if (!bot_) hudC(18, "RETURN RUNS AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(15, why_[0] ? why_ : "FAILED THE LEG", PAL_ALERT);
        if (!bot_) hudC(17, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %+05.1f", vel_);
    hud(1, 1, buf, std::fabs(vel_) <= kStop && !bodyInside() ? PAL_ALERT : PAL_HUD);
    float nose = kEnd - (x_ + kFront);
    std::snprintf(buf, sizeof buf, "END %04.1f", std::max(0.f, nose));
    hud(29, 1, buf, nose < 6.f ? PAL_ALERT : PAL_HUD);
    const char* line = "THE BOX IS THE END";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        int n = std::max(1, std::min(5, int(hold_ / kHoldNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "HOLD %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (bodyInside()) {
        line = "WHOLE BUS IN  STOP";
        pal = PAL_WIN;
    } else if (x_ + kFront > kBoxL && x_ - kRear < kBoxR) {
        line = "THE BUS STICKS OUT";
        pal = PAL_ALERT;
    } else if (x_ - kRear > kBoxR) {
        line = "BACK UP BEFORE THE END";
        pal = PAL_BANNER;
    } else if (nose < 8.f) {
        line = "THE END OF THE LEG";
        pal = PAL_ALERT;
    }
    hudC(26, line, pal);
    hud(1, 27, "RIGHT DRIVE   LEFT BRAKE", PAL_HUD);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -24 || cy + h * 0.5f < -24 || cx - w * 0.5f > gs::SCREEN_W + 24 ||
        cy - h * 0.5f > gs::SCREEN_H + 24)
        return;
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

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

float Game::sx(float wx) const { return 160.f + (wx - camX_) * kPpm; }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / 150.f;
        uint16_t sky = lerpC(gs::rgb4(3, 5, 10), gs::rgb4(11, 10, 8), clampf(u, 0.f, 1.f));
        if (y > 156) sky = lerpC(gs::rgb4(3, 4, 3), gs::rgb4(2, 3, 2), clampf((y - 156.f) / 40.f, 0.f, 1.f));
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 78.f, 30.f, float(art_.title.h) * 1.15f, PAL_BANNER);
        spr(art_.boxWord, 230.f, 30.f, float(art_.boxWord.h) * 1.15f, PAL_BANNER);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 34.f, float(banner_->h), pal);
    }

    float bob = std::sin(odo_ * 4.2f) * (mode_ == Mode::Run ? 0.8f : 0.2f);
    float busH = 58.f;
    float by = kRoadY - 36.f + bob;
    spr(art_.bus, sx(x_) + 2.f, by + 5.f, busH, PAL_BUS, true);
    int spin = int(odo_ * 2.2f) & 1;
    spr(art_.wheel[spin], sx(x_ - 3.4f), kRoadY - 8.f, 18.f, PAL_GLASS);
    spr(art_.wheel[spin], sx(x_ + 3.6f), kRoadY - 8.f, 18.f, PAL_GLASS);
    spr(art_.bus, sx(x_), by, busH, PAL_BUS);

    for (const Puff& p : dust_)
        if (p.life > 0.05f) spr(art_.dust, sx(p.x), kRoadY - 8.f - p.rise, 6.f + p.life * 7.f, PAL_DUST);

    int markPal = (hold_ > 0.02f || mode_ == Mode::Win) ? PAL_WIN : PAL_BOX;
    float boxCx = (kBoxL + kBoxR) * 0.5f;
    sprBox(art_.stripe, sx(boxCx), kRoadY - 4.f, (kBoxR - kBoxL) * kPpm, 16.f, markPal);
    for (int i = 0; i < 4; i++) {
        float hx = kBoxL + (kBoxR - kBoxL) * (i + 0.5f) / 4.f;
        spr(art_.hatch, sx(hx), kRoadY - 4.f, 9.f, markPal);
    }
    spr(art_.post, sx(kBoxL), kRoadY - 34.f, 56.f, PAL_STOP);
    spr(art_.post, sx(kBoxR), kRoadY - 34.f, 56.f, PAL_STOP);
    spr(art_.shelter, sx((kBoxL + kBoxR) * 0.5f), kRoadY - 58.f, 48.f, PAL_TOWN);
    bool wave = (int(t_ * 2.f) & 1) != 0;
    spr(art_.rider, sx(kBoxL + 2.2f) + (wave ? 1.f : 0.f), kRoadY - 30.f, 28.f, PAL_FOLK);
    spr(art_.rider, sx(kBoxR - 2.4f), kRoadY - 28.f, 26.f, PAL_FOLK);
    spr(art_.lamp, sx(kBoxL - 4.f), kRoadY - 46.f, 42.f, PAL_NIGHT);
    spr(art_.barrier, sx(kEnd), kRoadY - 28.f, 46.f, PAL_END);
    spr(art_.barrier, sx(kEnd + 1.3f), kRoadY - 28.f, 46.f, PAL_END);

    float base = std::floor((camX_ - 24.f) / 4.f) * 4.f;
    for (int i = 0; i < 16; i++) {
        float bx = base + i * 4.f;
        sprBox(art_.road, sx(bx), kRoadY, 4.f * kPpm + 2.f, 34.f, PAL_ROAD);
    }
    for (int i = -1; i < 18; i++) {
        float bx = std::floor(camX_ / 8.f) * 8.f + i * 8.f;
        spr(art_.kerb, sx(bx), kRoadY - 20.f, 10.f, PAL_TOWN);
    }
    spr(art_.block, sx(8.f), kRoadY - 48.f, 40.f, PAL_TOWN);
    spr(art_.block, sx(36.f), kRoadY - 56.f, 50.f, PAL_TOWN);
    spr(art_.block, sx(54.f), kRoadY - 44.f, 36.f, PAL_TOWN);
    spr(art_.block, sx(96.f), kRoadY - 52.f, 46.f, PAL_TOWN);

    drawHud();
}

}  // namespace busbox
