#include "luge.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace lugebox {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStop = 0.28f;
constexpr float kHoldNeed = 0.55f;
constexpr float kOutNeed = 0.90f;
constexpr float kWest = 4.f;
constexpr float kEast = 150.f;
constexpr float kGrade = 1.72f;
constexpr float kBrake = 7.4f;
constexpr float kDrag = 0.085f;

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
    float tail = x_ - kHalf;
    float nose = x_ + kHalf;
    return tail >= kBoxL + kMargin && nose <= kBoxR - kMargin;
}

void Game::begin() {
    x_ = 16.f;
    vel_ = 7.4f;
    hold_ = 0;
    outT_ = 0;
    race_ = 0;
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = chimeStep_ = 0;
    sprayCursor_ = 0;
    hornT_ = tone0_ = chimeT_ = thumpT_ = 0;
    sprayT_ = shake_ = 0;
    brakeIn_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : spray_) p = {};
    limit_ = 34.f;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 48.f;
    x_ = 28.f;
    vel_ = 0.f;
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
    sys.vdp.setFogColor(gs::rgb4(10, 11, 12));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.06f, 0.12f, 0.05f);
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

void Game::controls(float& brake) {
    const gs::Pad& p = sys_->pad;
    brake = 0;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_A)) brake = 1.f;
    if (p.brake > 0.12f) brake = std::max(brake, p.brake);
    if (p.axisY < -0.28f) brake = std::max(brake, clampf(-p.axisY, 0.f, 1.f));
    brake = clampf(brake, 0.f, 1.f);
}

void Game::pilot(float& brake) {
    float dx = kGoal - x_;
    float stopA = kBrake - kGrade;
    float stopD = (vel_ * vel_) / (2.f * std::max(1.f, stopA));
    phase_ = hullInside() ? 2 : (x_ > kBoxL - 10.f ? 1 : 0);
    if (dx <= 0.25f) {
        brake = vel_ > 0.12f ? 1.f : (kGrade / kBrake) + 0.02f;
        if (vel_ < 0.05f) brake = kGrade / kBrake;
    } else if (stopD >= dx - 1.1f) {
        float over = stopD - (dx - 2.4f);
        brake = clampf(0.25f + over * 0.18f, 0.2f, 1.f);
        if (vel_ > 11.f && dx < 28.f) brake = 1.f;
    } else if (vel_ > 15.5f) {
        brake = 0.45f;
    } else {
        brake = 0.f;
    }
}

void Game::physics(float brake) {
    float accel = kGrade - brake * kBrake - vel_ * kDrag;
    vel_ += accel * kDt;
    vel_ = clampf(vel_, -1.2f, 18.f);
    x_ += vel_ * kDt;
    if (!std::isfinite(x_) || !std::isfinite(vel_)) {
        fail("lost the ice");
        return;
    }
    float tail = x_ - kHalf;
    if (tail < kWest) {
        x_ += kWest - tail;
        vel_ = std::max(0.f, vel_);
    }
    float nose = x_ + kHalf;
    if (nose > kEast) {
        x_ -= nose - kEast;
        vel_ *= -0.12f;
        if (thumpT_ <= 0.f) {
            thumpT_ = 0.3f;
            shake_ = 0.8f;
            sys_->apu.noiseBurst(0.28f, 160.f, 0.14f);
            sys_->rumble(0.4f, 0.16f, 90);
        }
        fail("ran past the box");
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
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    bool late = why && std::strcmp(why, "the other crew took the box") == 0;
    banner_ = late ? &art_.late : &art_.outside;
    shake_ = 0.7f;
    sys_->rumble(0.5f, 0.22f, 160);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.32f, 110.f, 0.28f);
    sys_->apu.tone(0, 70.f, 0.05f);
    tone0_ = 0.32f;
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    bool in = hullInside();
    if (in && !announced_) {
        announced_ = true;
        blip(680.f);
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
        if (!in) outT_ = 0;
    }
    if (mode_ == Mode::Run && race_ >= limit_) fail("the other crew took the box");
}

void Game::audio() {
    float hiss = mode_ == Mode::Run ? 0.012f + std::fabs(vel_) * 0.0035f : 0.008f;
    sys_->apu.noise(hiss, 520.f + std::fabs(vel_) * 40.f, false);
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.f, 659.f, 784.f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.16f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && brakeIn_ > 0.08f && std::fabs(vel_) > 0.4f) {
        sys_->apu.tone(2, 90.f + brakeIn_ * 40.f, 0.02f + brakeIn_ * 0.03f);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    float left = std::max(0.f, limit_ - race_);
    if (mode_ == Mode::Run && left < 10.f && std::fmod(t_, 1.f) < kDt) {
        sys_->apu.tone(1, left < 4.f ? 880.f : 620.f, 0.04f);
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
        x_ = 30.f + std::sin(t_ * 0.4f) * 4.f;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
            blip(280.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            float brake = 0;
            if (bot_) pilot(brake);
            else controls(brake);
            brakeIn_ = brake;
            physics(brake);
            if (mode_ == Mode::Run) judge();
            if (std::fabs(vel_) > 2.f) {
                sprayT_ -= kDt;
                if (sprayT_ <= 0.f) {
                    sprayT_ = 0.06f;
                    spray_[sprayCursor_].x = x_ - kHalf * 0.9f;
                    spray_[sprayCursor_].life = 1.f;
                    sprayCursor_ = (sprayCursor_ + 1) % 14;
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

    for (Puff& p : spray_)
        if (p.life > 0.f) p.life -= kDt * 0.9f;

    float left = std::max(0.f, limit_ - (mode_ == Mode::Run ? race_ : 0.f));
    if (mode_ == Mode::Win || hold_ > 0.05f) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 40, 28);
    else if (mode_ == Mode::Run && left < 10.f) sys.setLight(170, 90, 30);
    else if (mode_ == Mode::Run && hullInside()) sys.setLight(40, 140, 90);
    else sys.setLight(80, 120, 160);

    float look = mode_ == Mode::Run ? x_ + vel_ * 0.35f : (mode_ == Mode::Title ? 70.f : x_);
    camX_ += (look - camX_) * (1.f - std::exp(-kDt * 3.4f));
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 46.f) * shake_ * 0.4f;
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
        hudC(21, "TAKE THE LUGE", PAL_BANNER);
        hudC(22, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(24, "B BRAKES THE RUNNERS", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "RETURN", PAL_WIN);
        return;
    }
    float left = std::max(0.f, limit_ - race_);
    int sec = int(left);
    int frac = int((left - sec) * 10.f);
    std::snprintf(buf, sizeof buf, "CREW %02d.%d", sec, frac);
    hud(1, 0, "S3 LUGE BOX", PAL_BANNER);
    hud(29, 0, buf, left < 10.f ? PAL_ALERT : PAL_CLOCK);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "STOPPED IN THE BOX", PAL_WIN);
        hudC(17, "BEFORE THE OTHER CREW", PAL_HUD);
        if (!bot_) hudC(19, "RETURN TAKES THE ICE AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "OUTSIDE", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %05.1f", vel_);
    hud(1, 1, buf, std::fabs(vel_) <= kStop && !hullInside() && x_ > 40.f ? PAL_ALERT : PAL_HUD);
    const char* line = "RIDE THE GRADE";
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
        line = "ALL INSIDE  HOLD THE BRAKE";
        pal = PAL_WIN;
    } else if (x_ + kHalf > kBoxL && x_ - kHalf < kBoxR) {
        line = "CLOSE IS STILL OUTSIDE";
        pal = PAL_ALERT;
    } else if (x_ > kBoxR) {
        line = "PAST THE BOX";
        pal = PAL_ALERT;
    } else if (left < 10.f) {
        line = "THE OTHER CREW IS DUE";
        pal = PAL_ALERT;
    }
    hudC(26, line, pal);
    hud(1, 27, "B BRAKE", PAL_HUD);
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

float Game::sx(float wx) const { return 150.f + (wx - camX_) * kPpm; }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = lerpC(gs::rgb4(6, 9, 13), gs::rgb4(12, 13, 14), clampf(y / 140.f, 0.f, 1.f));
        if (y > 132) {
            float u = (y - 132.f) / 70.f;
            sky = lerpC(gs::rgb4(13, 14, 15), gs::rgb4(9, 11, 12), clampf(u, 0.f, 1.f));
        } else if (y > 108) {
            sky = lerpC(gs::rgb4(11, 12, 13), gs::rgb4(8, 10, 8), (y - 108.f) / 24.f);
        }
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 88.f, 28.f, float(art_.title.h), PAL_BANNER);
        spr(art_.boxWord, 210.f, 30.f, float(art_.boxWord.h), PAL_BANNER);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 36.f, float(banner_->h), pal);
    }

    for (int i = 0; i < 10; i++) {
        float fx = std::fmod(i * 37.f + t_ * (12.f + (i % 3) * 4.f), 340.f) - 10.f;
        float fy = std::fmod(i * 19.f + t_ * (18.f + (i % 4)), 200.f);
        spr(art_.flake, fx, fy, 6.f, PAL_SNOW);
    }

    float base = std::floor((camX_ - 30.f) / 12.f) * 12.f;
    for (int i = 0; i < 10; i++) {
        float bx = base + i * 12.f;
        spr(art_.bank, sx(bx), 118.f, 28.f, PAL_ICE);
        if ((int(bx) / 12) % 3 == 0) spr(art_.pine, sx(bx + 2.f), 86.f, 44.f, PAL_PINE);
    }

    int markPal = (hold_ > 0.02f || mode_ == Mode::Win) ? PAL_WIN : PAL_BOX;
    float boxCx = (kBoxL + kBoxR) * 0.5f;
    float boxW = (kBoxR - kBoxL) * kPpm;
    sprBox(art_.stripe, sx(boxCx), kIceY + 6.f, boxW, 14.f, markPal);
    spr(art_.post, sx(kBoxL), kIceY - 18.f, 52.f, PAL_POST);
    spr(art_.post, sx(kBoxR), kIceY - 18.f, 52.f, PAL_POST);
    spr(art_.lamp, sx(kBoxL - 3.f), kIceY - 28.f, 28.f, PAL_CLOCK);
    spr(art_.lamp, sx(kBoxR + 3.f), kIceY - 28.f, 28.f, PAL_CLOCK);

    float left = mode_ == Mode::Title ? limit_ : std::max(0.f, limit_ - race_);
    float u = 1.f - clampf(left / limit_, 0.f, 1.f);
    int hand = std::min(7, int(u * 8.f));
    float hutX = kBoxR + 9.f;
    spr(art_.hut, sx(hutX), kIceY - 36.f, 48.f, PAL_HUT);
    spr(art_.clock[hand], sx(hutX + 0.4f), kIceY - 58.f, 26.f, PAL_CLOCK);
    bool pace = mode_ == Mode::Run && left < 14.f && (int(t_ * 4.f) & 1);
    spr(art_.crew[pace ? 1 : 0], sx(hutX - 5.f), kIceY - 16.f, 32.f, PAL_CREW);

    float bob = std::sin(t_ * (3.f + std::fabs(vel_) * 0.4f)) * (0.4f + std::min(2.f, std::fabs(vel_) * 0.08f));
    float ly = kIceY - 10.f + bob;
    float lugeH = 22.f;
    spr(art_.luge, sx(x_) + 1.f, ly + 4.f, lugeH, PAL_LUGE, true);
    for (const Puff& p : spray_)
        if (p.life > 0.05f) spr(art_.spray, sx(p.x), kIceY - 2.f, 6.f + p.life * 8.f, PAL_SPRAY);
    spr(art_.luge, sx(x_), ly, lugeH, PAL_LUGE);

    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(30.f + t_ * 14.f + i * 110.f, 360.f) - 16.f;
        bool up = std::sin(t_ * 5.f + i) > 0.f;
        spr(art_.bird[up ? 0 : 1], gx, 22.f + i * 9.f, 8.f, PAL_BIRD);
    }

    drawHud();
}

}  // namespace lugebox
