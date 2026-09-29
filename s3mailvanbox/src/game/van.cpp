#include "van.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace vanbox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPpm = 5.1f;
constexpr float kStop = 0.24f;
constexpr float kHoldNeed = 0.55f;
constexpr float kOutNeed = 1.15f;
constexpr float kWest = 3.f;
constexpr float kEast = 168.f;
constexpr float kAhead = 5.6f;
constexpr float kAstern = 4.4f;
constexpr float kDrag = 1.40f;
constexpr float kRoadY = 148.f;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

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
    if (hullInside()) return 2;
    return 1;
}

bool Game::hullInside() const {
    float rear = x_ - kHalf;
    float nose = x_ + kHalf;
    return rear >= kBoxL + kMargin && nose <= kBoxR - kMargin;
}

void Game::begin() {
    x_ = 18.f;
    vel_ = 0.f;
    hold_ = 0;
    outT_ = 0;
    race_ = 0;
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = chimeStep_ = 0;
    smokeCursor_ = 0;
    hornT_ = tone0_ = chimeT_ = thumpT_ = 0;
    smokeT_ = shake_ = 0;
    thrustIn_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : smoke_) p = {};
    limit_ = 52.f;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 70.f;
    x_ = 42.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    blip(440.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(4, 4, 5));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.06f, 0.10f, 0.05f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    tone0_ = 0.10f;
}

void Game::chime(int notes) {
    chimeN_ = std::max(1, std::min(notes, 6));
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::controls(float& thrust) {
    const gs::Pad& p = sys_->pad;
    thrust = 0;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_RIGHT)) thrust += 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_LEFT)) thrust -= 1.f;
    if (p.accel > 0.12f) thrust = p.accel;
    if (p.brake > 0.12f) thrust = -p.brake;
    if (std::fabs(p.axisX) > 0.22f) thrust = clampf(p.axisX, -1.f, 1.f);
    if (std::fabs(p.axisY) > 0.22f && std::fabs(thrust) < 0.05f) thrust = clampf(p.axisY, -1.f, 1.f);
    thrust = clampf(thrust, -1.f, 1.f);
}

void Game::pilot(float& thrust) {
    float dx = kGoal - x_;
    float ad = std::fabs(dx);
    float sign = dx >= 0.f ? 1.f : -1.f;
    float want;
    if (!hullInside()) {
        phase_ = x_ > kBoxL - 8.f ? 1 : 0;
        if (ad > 30.f) want = 4.6f * sign;
        else if (ad > 14.f) want = 2.0f * sign;
        else if (ad > 6.f) want = 0.85f * sign;
        else want = clampf(dx * 0.32f, -0.48f, 0.48f);
    } else {
        phase_ = 2;
        if (ad < 8.f) want = 0.f;
        else want = clampf(dx * 0.18f, -0.22f, 0.22f);
    }
    thrust = clampf((want - vel_) * 2.6f, -1.f, 1.f);
}

void Game::physics(float thrust) {
    float accel = (thrust >= 0.f ? thrust * kAhead : thrust * kAstern) - vel_ * kDrag;
    vel_ += accel * kDt;
    vel_ = clampf(vel_, -3.4f, 5.2f);
    x_ += vel_ * kDt;
    if (!std::isfinite(x_) || !std::isfinite(vel_)) {
        fail("lost the street");
        return;
    }
    bool hit = false;
    float rear = x_ - kHalf;
    float nose = x_ + kHalf;
    if (rear < kWest) {
        x_ += kWest - rear;
        hit = true;
    }
    if (nose > kEast) {
        x_ -= nose - kEast;
        hit = true;
    }
    if (hit) {
        vel_ *= -0.16f;
        if (thumpT_ <= 0.f) {
            thumpT_ = 0.26f;
            shake_ = std::max(shake_, 0.55f);
            sys_->apu.noiseBurst(0.20f, 160.f, 0.10f);
            sys_->rumble(0.22f, 0.08f, 60);
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
    sys_->rumble(0.28f, 0.10f, 140);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    bool late = why && std::strcmp(why, "the other crew took the route") == 0;
    banner_ = late ? &art_.late : &art_.outside;
    shake_ = 0.7f;
    sys_->rumble(0.48f, 0.22f, 160);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.32f, 110.f, 0.28f);
    sys_->apu.tone(0, 74.f, 0.05f);
    tone0_ = 0.32f;
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    bool in = hullInside();
    if (in && !announced_) {
        announced_ = true;
        blip(720.f);
    }
    if (in && std::fabs(vel_) <= kStop) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else if (!in && std::fabs(vel_) <= kStop && x_ > 40.f) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            if (x_ + kHalf < kBoxL + kMargin) fail("stopped short of the box");
            else if (x_ - kHalf > kBoxR - kMargin) fail("stopped past the box");
            else fail("stopped outside the box");
        }
    } else {
        hold_ = 0;
        outT_ = 0;
    }
    if (mode_ == Mode::Run && race_ >= limit_) fail("the other crew took the route");
}

void Game::audio() {
    float roll = mode_ == Mode::Run ? 0.012f + std::fabs(vel_) * 0.006f : 0.008f;
    sys_->apu.noise(roll, 220.f + std::fabs(vel_) * 40.f, false);
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.f, 659.f, 784.f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.15f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && (std::fabs(thrustIn_) > 0.04f || std::fabs(vel_) > 0.30f)) {
        float wob = 0.82f + 0.18f * std::sin(t_ * (9.f + std::fabs(thrustIn_) * 8.f));
        float vol = (0.016f + std::fabs(thrustIn_) * 0.028f) * wob;
        float f = 48.f + std::fabs(thrustIn_) * 28.f + std::fabs(vel_) * 2.0f;
        sys_->apu.tone(2, f, vol);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    float left = std::max(0.f, limit_ - race_);
    if (mode_ == Mode::Run && left < 12.f && std::fmod(t_, 1.f) < kDt) {
        sys_->apu.tone(1, left < 5.f ? 880.f : 620.f, 0.04f);
        hornT_ = 0.08f;
    } else if (hornT_ > 0.f) {
        hornT_ -= kDt;
        if (hornT_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    } else if (chimeN_ == 0) {
        sys_->apu.tone(1, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
        x_ = 42.f + std::sin(t_ * 0.40f) * 4.f;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
            blip(280.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            float thrust = 0;
            if (bot_) pilot(thrust);
            else controls(thrust);
            thrustIn_ = thrust;
            physics(thrust);
            if (mode_ == Mode::Run) judge();
            if (std::fabs(thrustIn_) > 0.06f || std::fabs(vel_) > 1.2f) {
                smokeT_ -= kDt;
                if (smokeT_ <= 0.f) {
                    smokeT_ = 0.10f;
                    smoke_[smokeCursor_].x = x_ - kHalf * 0.9f;
                    smoke_[smokeCursor_].life = 1.f;
                    smoke_[smokeCursor_].rise = 0.f;
                    smokeCursor_ = (smokeCursor_ + 1) % 8;
                }
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Run;
            banner_ = nullptr;
        } else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) startRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
    }

    for (Puff& p : smoke_)
        if (p.life > 0.f) {
            p.life -= kDt * 0.45f;
            p.rise += kDt * 7.f;
            p.x -= 0.15f;
        }

    float left = std::max(0.f, limit_ - (mode_ == Mode::Run ? race_ : 0.f));
    if (mode_ == Mode::Win || hold_ > 0.05f) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 40, 28);
    else if (mode_ == Mode::Run && left < 10.f) sys.setLight(170, 90, 30);
    else if (mode_ == Mode::Run && hullInside()) sys.setLight(40, 140, 80);
    else sys.setLight(90, 70, 30);

    float want = mode_ == Mode::Title ? 78.f : x_;
    camX_ += (want - camX_) * (1.f - std::exp(-kDt * 3.4f));
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 40.f) * shake_ * 0.30f;
        shake_ = std::max(0.f, shake_ - kDt * 1.6f);
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
    char buf[72];
    if (mode_ == Mode::Title) {
        hudC(22, "TAKE THE MAIL VAN", PAL_BANNER);
        hudC(23, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(24, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(25, "ARROWS DRIVE THE VAN", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "RETURN", PAL_WIN);
        return;
    }
    float left = std::max(0.f, limit_ - race_);
    int sec = int(left);
    int frac = int((left - sec) * 10.f);
    std::snprintf(buf, sizeof buf, "CREW %02d.%d", sec, frac);
    hud(1, 0, "S3 MAIL VAN", PAL_BANNER);
    hud(29, 0, buf, left < 12.f ? PAL_ALERT : PAL_CLOCK);
    if (mode_ == Mode::Pause) {
        hudC(17, "RETURN CONTINUES", PAL_HUD);
        hudC(18, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "STOPPED IN THE BOX", PAL_WIN);
        hudC(17, "BEFORE THE OTHER CREW", PAL_HUD);
        if (!bot_) hudC(19, "RETURN TAKES HER AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "OUTSIDE", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %+05.2f", vel_);
    hud(1, 1, buf, std::fabs(vel_) <= kStop && !hullInside() ? PAL_ALERT : PAL_HUD);
    const char* line = "MAKE THE BOX";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        int n = std::max(1, std::min(5, int(hold_ / kHoldNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "HOLD %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (hullInside()) {
        line = "ALL INSIDE  STOP";
        pal = PAL_WIN;
    } else if (x_ + kHalf > kBoxL && x_ - kHalf < kBoxR) {
        line = "CLOSE IS STILL OUTSIDE";
        pal = PAL_ALERT;
    } else if (x_ > kBoxR) {
        line = "BACK HER UP";
        pal = PAL_BANNER;
    } else if (left < 12.f) {
        line = "THE OTHER CREW IS DUE";
        pal = PAL_ALERT;
    }
    hudC(26, line, pal);
    hud(1, 27, "RIGHT AHEAD   LEFT BACK", PAL_HUD);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool hflip) {
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
    s.hflip = hflip;
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
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = lerpC(gs::rgb4(8, 10, 14), gs::rgb4(13, 11, 7), clampf(y / 130.f, 0.f, 1.f));
        if (y > 150) sky = gs::rgb4(3, 3, 4);
        else if (y > 128) sky = lerpC(gs::rgb4(5, 6, 4), gs::rgb4(3, 4, 3), (y - 128.f) / 22.f);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 100.f, 22.f, float(art_.title.h), PAL_BANNER);
        spr(art_.boxWord, 236.f, 24.f, float(art_.boxWord.h), PAL_BANNER);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 32.f, float(banner_->h), pal);
    }

    float bob = std::sin(t_ * (6.f + std::fabs(vel_) * 4.f)) * (0.4f + std::fabs(vel_) * 0.15f);
    float by = kRoadY - 18.f + bob;
    float vanH = 36.f;

    for (int i = -2; i < 12; i++) {
        float bx = std::floor(camX_ / 12.f) * 12.f + i * 12.f;
        sprBox(art_.asphalt, sx(bx), kRoadY + 6.f, 12.f * kPpm, 22.f, PAL_ROAD);
    }
    spr(art_.tree, sx(34.f), 92.f, 48.f, PAL_TREE);
    spr(art_.tree, sx(154.f), 88.f, 54.f, PAL_TREE);
    spr(art_.office, sx(kBoxR + 16.f), 96.f, 62.f, PAL_OFFICE);

    int markPal = (hold_ > 0.02f || mode_ == Mode::Win) ? PAL_WIN : PAL_BOX;
    float boxCx = (kBoxL + kBoxR) * 0.5f;
    float boxW = (kBoxR - kBoxL) * kPpm;
    sprBox(art_.asphalt, sx(boxCx), kRoadY + 6.f, boxW, 22.f, markPal);
    for (int i = 0; i < 5; i++) {
        float hx = kBoxL + (kBoxR - kBoxL) * (i + 0.5f) / 5.f;
        spr(art_.stripe, sx(hx), kRoadY + 8.f, 12.f, markPal);
    }
    spr(art_.post, sx(kBoxL), kRoadY - 16.f, 52.f, PAL_POST);
    spr(art_.post, sx(kBoxR), kRoadY - 16.f, 52.f, PAL_POST);
    spr(art_.lamp, sx(kBoxL - 5.f), kRoadY - 22.f, 40.f, PAL_CLOCK);
    spr(art_.lamp, sx(kBoxR + 5.f), kRoadY - 22.f, 40.f, PAL_CLOCK);
    spr(art_.sack, sx(kBoxL + 4.f), kRoadY - 2.f, 12.f, PAL_MAIL);
    spr(art_.sack, sx(kBoxR - 5.f), kRoadY - 1.f, 11.f, PAL_MAIL);

    float left = mode_ == Mode::Title ? limit_ : std::max(0.f, limit_ - race_);
    float u = 1.f - clampf(left / limit_, 0.f, 1.f);
    int hand = std::min(7, int(u * 8.f));
    float quay = kBoxR + 16.f;
    spr(art_.clock[hand], sx(quay), 58.f, 28.f, PAL_CLOCK);
    bool pace = mode_ == Mode::Run && left < 16.f && (int(t_ * 3.f) & 1);
    spr(art_.crew[pace ? 1 : 0], sx(quay - 8.f + (pace ? 0.5f : 0.f)), 118.f, 32.f, PAL_CREW);

    spr(art_.van, sx(x_) + 2.f, by + 6.f, vanH, PAL_VAN, true);
    for (const Puff& p : smoke_)
        if (p.life > 0.05f) spr(art_.smoke, sx(p.x), by - 8.f - p.rise, 7.f + p.life * 6.f, PAL_SMOKE);
    spr(art_.van, sx(x_), by, vanH, PAL_VAN, false, vel_ < -0.15f);

    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(16.f + t_ * 10.f + i * 100.f, 360.f) - 16.f;
        bool up = std::sin(t_ * 7.f + i) > 0.f;
        spr(art_.bird[up ? 0 : 1], gx, 26.f + i * 9.f, 9.f, PAL_BIRD);
    }

    drawHud();
}

}  // namespace vanbox
