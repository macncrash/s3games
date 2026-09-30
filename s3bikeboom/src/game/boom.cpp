#include "boom.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace bikeboom {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStop = 0.28f;
constexpr float kHoldNeed = 0.55f;
constexpr float kOutNeed = 0.95f;
constexpr float kWest = 4.f;
constexpr float kEast = 98.f;
constexpr float kPedal = 8.2f;
constexpr float kBrake = 13.2f;
constexpr float kBack = 3.6f;
constexpr float kDrag = 0.38f;
constexpr float kRoll = 0.14f;
constexpr float kRoadY = 172.f;

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
    if (driveOnBoom()) return 2;
    return 1;
}

bool Game::driveOnBoom() const { return std::fabs((x_ + kRack) - kBoom) <= kWindow; }

void Game::begin() {
    x_ = 14.f;
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
    tickT_ = tone0_ = chimeT_ = thumpT_ = 0;
    dustT_ = shake_ = pedalIn_ = lift_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : dust_) p = {};
    limit_ = 40.f;
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
    blip(520.f);
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
    tone0_ = 0.10f;
}

void Game::chime(int notes) {
    chimeN_ = std::max(1, std::min(notes, 6));
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::controls(float& pedal, float& brake, float& back) {
    const gs::Pad& p = sys_->pad;
    pedal = brake = back = 0;
    bool go = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_UP) || p.down(gs::BTN_C);
    bool stop = p.down(gs::BTN_LEFT) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_A) || p.down(gs::BTN_B);
    if (p.accel > 0.12f) go = true;
    if (p.brake > 0.12f) stop = true;
    if (p.axisX > 0.25f) go = true;
    if (p.axisX < -0.25f) stop = true;
    if (go && !stop) pedal = 1.f;
    else if (stop && !go) {
        if (vel_ > 0.18f) brake = 1.f;
        else back = 1.f;
    }
    if (p.accel > 0.12f && pedal > 0.f) pedal = clampf(p.accel, 0.f, 1.f);
}

void Game::pilot(float& pedal, float& brake, float& back) {
    pedal = brake = back = 0;
    float goal = kBoom - kRack;
    float dx = goal - x_;
    float ad = std::fabs(dx);
    bool on = driveOnBoom();
    float stopDist = (vel_ * vel_) / (2.f * 10.4f);
    if (!on) {
        phase_ = x_ + kRack > kBoom - 6.f ? 1 : 0;
        if (dx > 0.25f) {
            bool needSlow = ad < stopDist + 2.4f || (vel_ > 6.8f && ad < 14.f);
            if (needSlow && vel_ > 1.05f) brake = clampf((vel_ - ad * 0.22f) / 3.2f, 0.2f, 1.f);
            else if (vel_ < 9.4f) pedal = ad > 16.f ? 1.f : 0.78f;
            if (vel_ < 1.5f && ad > 4.f) {
                brake = 0;
                pedal = 1.f;
            }
        } else {
            if (vel_ > 0.10f) brake = 1.f;
            else back = 0.9f;
        }
    } else {
        phase_ = 2;
        float want = clampf(dx * 1.8f, -1.4f, 1.4f);
        if (ad < 0.12f) want = 0.f;
        float err = want - vel_;
        if (err > 0.08f) pedal = clampf(err * 0.85f, 0.f, 1.f);
        else if (err < -0.08f) {
            if (vel_ > 0.04f) brake = clampf(-err, 0.f, 1.f);
            else back = clampf(-err * 0.55f, 0.f, 0.7f);
        }
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
    vel_ += a * kDt;
    vel_ = clampf(vel_, -2.6f, 11.6f);
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
        vel_ *= -0.1f;
        if (thumpT_ <= 0.f) {
            thumpT_ = 0.2f;
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
    std::snprintf(why_, sizeof why_, "drive on the boom");
    banner_ = &art_.delivered;
    chime(4);
    sys_->rumble(0.26f, 0.12f, 140);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    if (why && std::strcmp(why, "the other crew took the drive") == 0) banner_ = &art_.late;
    else if (why && std::strstr(why, "past")) banner_ = &art_.past;
    else banner_ = &art_.shortB;
    shake_ = 0.65f;
    sys_->rumble(0.46f, 0.2f, 150);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.3f, 130.f, 0.24f);
    sys_->apu.tone(0, 68.f, 0.05f);
    tone0_ = 0.3f;
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    bool on = driveOnBoom();
    if (on && !announced_) {
        announced_ = true;
        blip(760.f);
    }
    if (on && std::fabs(vel_) <= kStop) {
        outT_ = 0;
        hold_ += kDt;
        lift_ = clampf(hold_ / kHoldNeed, 0.f, 1.f);
        if (hold_ >= kHoldNeed) win();
    } else if (!on && std::fabs(vel_) <= kStop && race_ > 1.4f) {
        hold_ = 0;
        lift_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) {
            if (x_ + kRack < kBoom) fail("short of the boom");
            else fail("past the boom");
        }
    } else {
        if (!on) lift_ = 0;
        hold_ = 0;
        outT_ = 0;
    }
    if (mode_ == Mode::Run && race_ >= limit_) fail("the other crew took the drive");
}

void Game::audio() {
    float roll = mode_ == Mode::Run ? 0.010f + std::fabs(vel_) * 0.003f : 0.005f;
    sys_->apu.noise(roll, 480.f + std::fabs(vel_) * 36.f, false);
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {440.f, 554.f, 659.f, 880.f};
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
        float wob = 0.75f + 0.25f * std::sin(odo_ * 8.f);
        sys_->apu.tone(2, 96.f + pedalIn_ * 36.f + std::fabs(vel_) * 5.f, 0.018f * wob);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    float left = std::max(0.f, limit_ - race_);
    if (mode_ == Mode::Run && left < 9.f && std::fmod(t_, 0.5f) < kDt) {
        sys_->apu.tone(1, left < 4.f ? 920.f : 640.f, 0.04f);
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
        x_ = 20.f + std::sin(t_ * 0.65f) * 3.5f;
        vel_ = std::cos(t_ * 0.65f) * 2.2f;
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
            if (bot_) pilot(pedal, brake, back);
            else controls(pedal, brake, back);
            pedalIn_ = pedal;
            physics(pedal, brake, back);
            if (mode_ == Mode::Run) judge();
            if (std::fabs(vel_) > 1.5f || brake > 0.4f) {
                dustT_ -= kDt;
                if (dustT_ <= 0.f) {
                    dustT_ = brake > 0.4f ? 0.05f : 0.11f;
                    dust_[dustCursor_].x = x_ - kRear;
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
            p.rise += kDt * 7.f;
        }

    float left = std::max(0.f, limit_ - (mode_ == Mode::Run ? race_ : 0.f));
    if (mode_ == Mode::Win || hold_ > 0.05f) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 40, 28);
    else if (mode_ == Mode::Run && left < 8.f) sys.setLight(170, 90, 30);
    else if (mode_ == Mode::Run && driveOnBoom()) sys.setLight(40, 140, 90);
    else sys.setLight(30, 70, 140);

    float want = mode_ == Mode::Title ? 42.f : x_ + 6.f;
    camX_ += (want - camX_) * (1.f - std::exp(-kDt * 3.2f));
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 44.f) * shake_ * 0.35f;
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
        hudC(22, "DELIVER THE DRIVE TO THE BOOM", PAL_HUD);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(24, "RIGHT PEDALS   LEFT BRAKES", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "RETURN", PAL_WIN);
        return;
    }
    float left = std::max(0.f, limit_ - race_);
    int sec = int(left);
    int frac = int((left - sec) * 10.f);
    std::snprintf(buf, sizeof buf, "CREW %02d.%d", sec, frac);
    hud(1, 0, "S3 BIKE BOOM", PAL_BANNER);
    hud(28, 0, buf, left < 9.f ? PAL_ALERT : PAL_CLOCK);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "DRIVE ON THE BOOM", PAL_WIN);
        hudC(16, "BEFORE THE OTHER CREW", PAL_HUD);
        if (!bot_) hudC(18, "RETURN RIDES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(15, why_[0] ? why_ : "MISSED", PAL_ALERT);
        if (!bot_) hudC(17, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %+05.1f", vel_);
    hud(1, 1, buf, std::fabs(vel_) <= kStop && !driveOnBoom() ? PAL_ALERT : PAL_HUD);
    const char* line = "RIDE THE DRIVE TO THE BOOM";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        int n = std::max(1, std::min(5, int(hold_ / kHoldNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "HANDOFF %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (driveOnBoom()) {
        line = "DRIVE UNDER THE HOOK  STOP";
        pal = PAL_WIN;
    } else if (x_ + kRack > kBoom) {
        line = "BACK UNDER THE BOOM";
        pal = PAL_BANNER;
    } else if (left < 9.f) {
        line = "THE OTHER CREW IS DUE";
        pal = PAL_ALERT;
    }
    hudC(26, line, pal);
    hud(1, 27, "RIGHT PEDAL   LEFT BRAKE", PAL_HUD);
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
        float u = y / 150.f;
        uint16_t sky = lerpC(gs::rgb4(3, 5, 10), gs::rgb4(11, 9, 6), clampf(u, 0.f, 1.f));
        if (y > 156) sky = lerpC(gs::rgb4(3, 4, 4), gs::rgb4(2, 2, 3), clampf((y - 156.f) / 40.f, 0.f, 1.f));
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 78.f, 26.f, float(art_.title.h) * 1.15f, PAL_BANNER);
        spr(art_.boomWord, 230.f, 28.f, float(art_.boomWord.h) * 1.15f, PAL_BANNER);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 34.f, float(banner_->h), pal);
    }

    float bob = std::sin(odo_ * 6.2f) * (mode_ == Mode::Run ? 1.1f : 0.35f);
    float bikeH = 48.f;
    float by = kRoadY - 24.f + bob;
    float rise = (mode_ == Mode::Win ? 1.f : lift_) * 34.f;
    float driveWx = (mode_ == Mode::Win || lift_ > 0.02f) ? kBoom : x_ + kRack;
    float driveY = kRoadY - 36.f + bob * 0.2f - rise;
    spr(art_.reel, sx(driveWx), driveY - 2.f, 12.f, PAL_DRIVE);
    spr(art_.drive, sx(driveWx), driveY, 16.f, PAL_DRIVE);

    float hookDrop = 18.f - rise * 0.35f;
    spr(art_.hook, sx(kBoom), kRoadY - 78.f + hookDrop, 26.f, PAL_HOOK);
    spr(art_.arm, sx(kBoom + 1.6f), kRoadY - 96.f, 16.f, PAL_BOOM);
    spr(art_.mast, sx(kBoom + 4.4f), kRoadY - 62.f, 92.f, PAL_BOOM);
    spr(art_.pad, sx(kBoom), kRoadY - 8.f, 8.f, (hold_ > 0.02f || mode_ == Mode::Win) ? PAL_WIN : PAL_BOOM);

    spr(art_.bike, sx(x_) + 1.f, by + 3.f, bikeH, PAL_BIKE, true);
    int spin = int(odo_ * 3.2f) & 1;
    float wh = 18.f;
    spr(art_.wheel[spin], sx(x_ - 0.78f), kRoadY - 8.f + bob * 0.25f, wh, PAL_BIKE);
    spr(art_.wheel[spin], sx(x_ + 1.02f), kRoadY - 8.f + bob * 0.25f, wh, PAL_BIKE);
    spr(art_.bike, sx(x_), by, bikeH, PAL_BIKE);

    for (const Puff& p : dust_)
        if (p.life > 0.05f) spr(art_.dust, sx(p.x), kRoadY - 6.f - p.rise, 6.f + p.life * 7.f, PAL_DUST);

    float left = mode_ == Mode::Title ? limit_ : std::max(0.f, limit_ - race_);
    float u = 1.f - clampf(left / limit_, 0.f, 1.f);
    int hand = std::min(7, int(u * 8.f));
    float dock = kBoom + 7.2f;
    spr(art_.clock[hand], sx(dock), kRoadY - 86.f, 28.f, PAL_CLOCK);
    bool pace = mode_ == Mode::Run && left < 12.f && (int(t_ * 5.f) & 1);
    spr(art_.crew[pace ? 1 : 0], sx(dock - 1.4f), kRoadY - 32.f, 34.f, PAL_CREW);
    spr(art_.van, sx(dock + 2.4f), kRoadY - 22.f, 26.f, PAL_DOCK);
    spr(art_.lamp, sx(kBoom - 4.f), kRoadY - 46.f, 42.f, PAL_CLOCK);
    spr(art_.tree, sx(10.f), kRoadY - 54.f, 46.f, PAL_TOWN);
    spr(art_.tree, sx(38.f), kRoadY - 48.f, 38.f, PAL_TOWN);
    spr(art_.tree, sx(56.f), kRoadY - 50.f, 40.f, PAL_TOWN);

    float base = std::floor((camX_ - 22.f) / 4.f) * 4.f;
    for (int i = 0; i < 16; i++) {
        float bx = base + i * 4.f;
        sprBox(art_.road, sx(bx), kRoadY, 4.f * kPpm + 2.f, 34.f, PAL_ROAD);
    }
    for (int i = -1; i < 18; i++) {
        float bx = std::floor(camX_ / 8.f) * 8.f + i * 8.f;
        spr(art_.kerb, sx(bx), kRoadY - 18.f, 10.f, PAL_DOCK);
    }

    drawHud();
}

}  // namespace bikeboom
