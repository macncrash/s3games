#include "mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace bikemark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStop = 0.16f;
constexpr float kSetNeed = 0.90f;
constexpr float kOffNeed = 1.05f;
constexpr float kWest = 2.4f;
constexpr float kEast = 88.f;
constexpr float kPedal = 6.8f;
constexpr float kBrake = 11.2f;
constexpr float kBack = 2.8f;
constexpr float kDrag = 0.48f;
constexpr float kRoll = 0.18f;
constexpr float kRoadY = 170.f;

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
    if (set_ > 0.08f) return 3;
    if (wheelsOn()) return 2;
    return 1;
}

bool Game::wheelsOn() const {
    float rear = x_ - kRear;
    float front = x_ + kFront;
    return rear >= kMarkL && front <= kMarkR;
}

void Game::begin() {
    x_ = 10.f;
    vel_ = 0.3f;
    set_ = 0;
    offT_ = 0;
    race_ = 0;
    odo_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = chimeStep_ = dustCursor_ = 0;
    tickT_ = tone0_ = chimeT_ = thumpT_ = 0;
    dustT_ = shake_ = pedalIn_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    report_[0] = 0;
    for (Puff& p : dust_) p = {};
    limit_ = 46.f;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 36.f;
    x_ = 22.f;
    vel_ = 0;
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
    sys.vdp.setFogColor(gs::rgb4(5, 4, 6));
    sys.apu.setMaster(0.70f);
    sys.apu.setEcho(0.05f, 0.09f, 0.03f);
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

void Game::controls(float& pedal, float& brake, float& back, bool& set) {
    const gs::Pad& p = sys_->pad;
    pedal = brake = back = 0;
    set = p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_Y);
    bool go = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_C) || p.down(gs::BTN_X);
    bool stop = p.down(gs::BTN_LEFT) || p.down(gs::BTN_A) || p.down(gs::BTN_Z);
    if (p.accel > 0.12f) go = true;
    if (p.brake > 0.12f) stop = true;
    if (p.axisX > 0.25f) go = true;
    if (p.axisX < -0.25f) stop = true;
    if (set) {
        go = false;
        if (std::fabs(vel_) > kStop) stop = true;
    }
    if (go && !stop) pedal = 1.f;
    else if (stop && !go) {
        if (vel_ > 0.16f) brake = 1.f;
        else back = 1.f;
    }
    if (p.accel > 0.12f && pedal > 0.f) pedal = clampf(p.accel, 0.f, 1.f);
}

void Game::pilot(float& pedal, float& brake, float& back, bool& set) {
    pedal = brake = back = 0;
    set = false;
    float dx = kGoal - x_;
    float ad = std::fabs(dx);
    bool on = wheelsOn();
    float stopDist = (vel_ * vel_) / (2.f * 8.4f);
    if (!on || ad > 0.22f || std::fabs(vel_) > 0.35f) {
        if (dx > 0.12f) {
            bool needSlow = ad < stopDist + 2.6f || (vel_ > 5.6f && ad < 14.f);
            if (needSlow && vel_ > 0.9f) brake = clampf((vel_ - ad * 0.22f) / 3.0f, 0.2f, 1.f);
            else if (vel_ < 7.4f) pedal = ad > 16.f ? 1.f : 0.62f;
            if (vel_ < 1.3f && ad > 4.f) {
                brake = 0;
                pedal = 0.85f;
            }
        } else if (dx < -0.12f) {
            if (vel_ > 0.08f) brake = 1.f;
            else back = 0.7f;
        } else if (std::fabs(vel_) > kStop) {
            brake = 1.f;
        }
    } else {
        set = true;
    }
}

void Game::physics(float pedal, float brake, float back) {
    float a = pedal * kPedal - back * kBack;
    if (brake > 0.f) {
        if (std::fabs(vel_) < 0.07f) vel_ = 0.f;
        else a -= brake * kBrake * signf(vel_);
    }
    a -= vel_ * kDrag;
    if (std::fabs(vel_) > 0.02f) a -= kRoll * signf(vel_);
    if (set_ > 0.2f) a -= vel_ * 6.f;
    vel_ += a * kDt;
    vel_ = clampf(vel_, -2.0f, 9.6f);
    x_ += vel_ * kDt;
    odo_ += std::fabs(vel_) * kDt;
    if (!std::isfinite(x_) || !std::isfinite(vel_)) {
        fail("lost the street");
        return;
    }
    bool hit = false;
    if (x_ - kRear < kWest) {
        x_ += kWest - (x_ - kRear);
        hit = true;
    }
    if (x_ + kFront > kEast) {
        x_ -= (x_ + kFront) - kEast;
        hit = true;
    }
    if (hit) {
        vel_ *= -0.10f;
        if (thumpT_ <= 0.f) {
            thumpT_ = 0.20f;
            shake_ = std::max(shake_, 0.5f);
            sys_->apu.noiseBurst(0.16f, 220.f, 0.09f);
            sys_->rumble(0.18f, 0.05f, 50);
        }
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    set_ = 1.f;
    std::snprintf(why_, sizeof why_, "set down on the mark");
    std::snprintf(report_, sizeof report_,
                  "S3 BIKE MARK  SET DOWN  on the mark before the other crew  (%.1f s)", race_);
    banner_ = &art_.setWord;
    chime(4);
    sys_->rumble(0.26f, 0.10f, 130);
    sys_->setLight(40, 160, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    bool late = why && std::strcmp(why, "the other crew took the mark") == 0;
    banner_ = late ? &art_.late : &art_.offWord;
    std::snprintf(report_, sizeof report_, "S3 BIKE MARK  FAIL  %s  x %.1f spd %.2f  (%.1f s)", why_, x_, vel_,
                  race_);
    shake_ = 0.65f;
    sys_->rumble(0.46f, 0.20f, 150);
    sys_->setLight(160, 32, 22);
    sys_->apu.noiseBurst(0.30f, 130.f, 0.24f);
    sys_->apu.tone(0, 66.f, 0.05f);
    tone0_ = 0.30f;
}

void Game::judge(bool set) {
    if (mode_ != Mode::Run) return;
    bool on = wheelsOn();
    if (on && !announced_) {
        announced_ = true;
        blip(680.f);
    }
    bool still = std::fabs(vel_) <= kStop;
    if (on && still && set) {
        offT_ = 0;
        set_ += kDt;
        if (set_ >= kSetNeed) win();
    } else {
        if (set_ > 0.f && (!on || !still)) {
            set_ = std::max(0.f, set_ - kDt * 2.4f);
            if (thumpT_ <= 0.f && set_ > 0.15f) {
                thumpT_ = 0.12f;
                sys_->apu.noiseBurst(0.08f, 180.f, 0.05f);
            }
        } else if (!set) {
            set_ = std::max(0.f, set_ - kDt * 1.4f);
        }
        if (!on && still && race_ > 1.4f && std::fabs(vel_) < 0.05f) {
            offT_ += kDt;
            if (offT_ >= kOffNeed) {
                if (x_ + kFront < kMarkL) fail("set down short of the mark");
                else if (x_ - kRear > kMarkR) fail("set down past the mark");
                else fail("set down off the mark");
            }
        } else {
            offT_ = 0;
        }
    }
    if (mode_ == Mode::Run && race_ >= limit_) fail("the other crew took the mark");
}

void Game::audio() {
    float roll = mode_ == Mode::Run ? 0.009f + std::fabs(vel_) * 0.0028f : 0.005f;
    sys_->apu.noise(roll, 480.f + std::fabs(vel_) * 36.f, false);
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {349.f, 440.f, 523.f, 698.f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && pedalIn_ > 0.05f) {
        float wob = 0.7f + 0.3f * std::sin(odo_ * 8.f);
        sys_->apu.tone(2, 80.f + pedalIn_ * 36.f + std::fabs(vel_) * 5.f, 0.018f * wob);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    float left = std::max(0.f, limit_ - race_);
    if (mode_ == Mode::Run && left < 9.f && std::fmod(t_, 0.5f) < kDt) {
        sys_->apu.tone(1, left < 4.f ? 840.f : 580.f, 0.04f);
        tickT_ = 0.07f;
    } else if (tickT_ > 0.f) {
        tickT_ -= kDt;
        if (tickT_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
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
        x_ = 20.f + std::sin(t_ * 0.6f) * 3.2f;
        vel_ = std::cos(t_ * 0.6f) * 1.9f;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
            blip(260.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            float pedal = 0, brake = 0, back = 0;
            bool set = false;
            if (bot_) pilot(pedal, brake, back, set);
            else controls(pedal, brake, back, set);
            pedalIn_ = pedal;
            physics(pedal, brake, back);
            if (mode_ == Mode::Run) judge(set);
            if (std::fabs(vel_) > 1.6f || brake > 0.45f) {
                dustT_ -= kDt;
                if (dustT_ <= 0.f) {
                    dustT_ = brake > 0.45f ? 0.05f : 0.11f;
                    dust_[dustCursor_].x = x_ - kRear * 0.4f;
                    dust_[dustCursor_].life = 1.f;
                    dust_[dustCursor_].rise = 0.f;
                    dustCursor_ = (dustCursor_ + 1) % 8;
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

    for (Puff& p : dust_)
        if (p.life > 0.f) {
            p.life -= kDt * 0.75f;
            p.rise += kDt * 5.f;
        }

    float left = std::max(0.f, limit_ - (mode_ == Mode::Run ? race_ : 0.f));
    if (mode_ == Mode::Win || set_ > 0.2f) sys.setLight(40, 160, 60);
    else if (mode_ == Mode::Fail) sys.setLight(160, 36, 24);
    else if (mode_ == Mode::Run && left < 8.f) sys.setLight(170, 80, 28);
    else if (mode_ == Mode::Run && wheelsOn()) sys.setLight(150, 120, 30);
    else sys.setLight(50, 60, 130);

    float want = mode_ == Mode::Title ? 42.f : x_;
    camX_ += (want - camX_) * (1.f - std::exp(-kDt * 3.2f));
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 42.f) * shake_ * 0.35f;
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
        hudC(21, "TAKE THE BIKE", PAL_BANNER);
        hudC(22, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(24, "RIGHT PEDALS  B SETS THE STAND", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "RETURN", PAL_WIN);
        return;
    }
    float left = std::max(0.f, limit_ - race_);
    int sec = int(left);
    int frac = int((left - sec) * 10.f);
    std::snprintf(buf, sizeof buf, "CREW %02d.%d", sec, frac);
    hud(1, 0, "S3 BIKE MARK", PAL_BANNER);
    hud(28, 0, buf, left < 9.f ? PAL_ALERT : PAL_CLOCK);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "SET DOWN ON THE MARK", PAL_WIN);
        hudC(16, "BEFORE THE OTHER CREW", PAL_HUD);
        if (!bot_) hudC(18, "RETURN RIDES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(15, why_[0] ? why_ : "OFF THE MARK", PAL_ALERT);
        if (!bot_) hudC(17, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %+05.1f", vel_);
    hud(1, 1, buf, PAL_HUD);
    const char* line = "RIDE TO THE MARK";
    int pal = PAL_HUD;
    if (set_ > 0.02f) {
        int n = std::max(1, std::min(5, int(set_ / kSetNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "STAND %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (wheelsOn()) {
        line = std::fabs(vel_) <= kStop ? "HOLD B  SET THE STAND" : "WHEELS ON  SLOW DOWN";
        pal = PAL_WIN;
    } else if (x_ + kFront > kMarkL && x_ - kRear < kMarkR) {
        line = "A WHEEL IS OFF THE PAINT";
        pal = PAL_ALERT;
    } else if (x_ - kRear > kMarkR) {
        line = "BACK ONTO THE MARK";
        pal = PAL_BANNER;
    } else if (left < 9.f) {
        line = "THE OTHER CREW IS DUE";
        pal = PAL_ALERT;
    }
    hudC(26, line, pal);
    hud(1, 27, "RIGHT PEDAL  LEFT BRAKE  B SET", PAL_HUD);
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
        float u = y / 148.f;
        uint16_t sky = lerpC(gs::rgb4(3, 3, 8), gs::rgb4(12, 7, 4), clampf(u, 0.f, 1.f));
        if (y > 152) sky = lerpC(gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 2), clampf((y - 152.f) / 36.f, 0.f, 1.f));
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 78.f, 30.f, float(art_.title.h) * 1.05f, PAL_BANNER);
        spr(art_.markWord, 232.f, 32.f, float(art_.markWord.h) * 1.05f, PAL_BANNER);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 34.f, float(banner_->h), pal);
    }

    float base = std::floor((camX_ - 18.f) / 3.4f) * 3.4f;
    for (int i = 0; i < 16; i++) {
        float bx = base + i * 3.4f;
        sprBox(art_.road, sx(bx), kRoadY, 3.4f * kPpm + 2.f, 34.f, PAL_ROAD);
    }

    int markPal = (set_ > 0.05f || mode_ == Mode::Win) ? PAL_WIN : PAL_MARK;
    float cx = (kMarkL + kMarkR) * 0.5f;
    float mw = (kMarkR - kMarkL) * kPpm;
    sprBox(art_.paint, sx(cx), kRoadY - 1.f, mw, 16.f, markPal);
    spr(art_.cross, sx(cx), kRoadY - 2.f, 22.f, markPal);
    spr(art_.peg, sx(kMarkL), kRoadY - 22.f, 40.f, PAL_STAND);
    spr(art_.peg, sx(kMarkR), kRoadY - 22.f, 40.f, PAL_STAND);

    float left = mode_ == Mode::Title ? limit_ : std::max(0.f, limit_ - race_);
    float u = 1.f - clampf(left / limit_, 0.f, 1.f);
    int hand = std::min(7, int(u * 8.f));
    float yard = kMarkR + 7.5f;
    spr(art_.yard, sx(yard), kRoadY - 42.f, 42.f, PAL_YARD);
    spr(art_.clock[hand], sx(yard - 0.4f), kRoadY - 72.f, 28.f, PAL_CLOCK);
    bool pace = mode_ == Mode::Run && left < 12.f && (int(t_ * 4.f) & 1);
    spr(art_.crew[pace ? 1 : 0], sx(yard - 2.6f), kRoadY - 30.f, 32.f, PAL_CREW);
    spr(art_.lamp, sx(kMarkL - 2.2f), kRoadY - 38.f, 36.f, PAL_CLOCK);
    spr(art_.hedge, sx(18.f), kRoadY - 28.f, 28.f, PAL_YARD);
    spr(art_.hedge, sx(40.f), kRoadY - 24.f, 22.f, PAL_YARD);
    spr(art_.hedge, sx(82.f), kRoadY - 26.f, 26.f, PAL_YARD);

    for (const Puff& p : dust_)
        if (p.life > 0.05f) spr(art_.dust, sx(p.x), kRoadY - 5.f - p.rise, 5.f + p.life * 7.f, PAL_DUST);

    float drop = set_ * 5.f;
    float bob = (set_ > 0.2f ? 0.f : std::sin(odo_ * 5.5f) * (mode_ == Mode::Run ? 1.0f : 0.3f));
    float by = kRoadY - 20.f + bob + drop * 0.35f;
    spr(art_.frame, sx(x_) + 2.f, by + 5.f, 42.f, PAL_BIKE, true);
    int spin = int(odo_ * 2.6f) & 1;
    float wh = 16.f;
    spr(art_.wheel[spin], sx(x_ - kRear), kRoadY - 7.f + bob * 0.2f, wh, PAL_BIKE);
    spr(art_.wheel[spin], sx(x_ + kFront * 0.72f), kRoadY - 7.f + bob * 0.2f, wh, PAL_BIKE);
    if (set_ > 0.04f) {
        float sh = 8.f + set_ * 14.f;
        spr(art_.stand, sx(x_ + 0.15f), kRoadY - 4.f - (1.f - set_) * 6.f, sh, PAL_STAND);
    }
    spr(art_.frame, sx(x_), by, 42.f, PAL_BIKE);

    drawHud();
}

}  // namespace bikemark
