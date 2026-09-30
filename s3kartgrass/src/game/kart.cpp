#include "kart.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace kartgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kG = 21.f;
constexpr float kStop = 0.16f;
constexpr float kHoldNeed = 0.48f;
constexpr float kThrottle = 9.4f;
constexpr float kBrake = 17.2f;
constexpr float kDrag = 0.38f;
constexpr float kRoll = 0.14f;
constexpr float kUphill = 3.4f;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
float signf(float v) { return v < 0.f ? -1.f : v > 0.f ? 1.f : 0.f; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

float groundH(float x, bool& solid) {
    if (x < kRamp0) {
        solid = true;
        return 0.f;
    }
    if (x <= kLip) {
        solid = true;
        float u = (x - kRamp0) / (kLip - kRamp0);
        return u * kLipH;
    }
    if (x >= kGrass0 && x <= kGrass1) {
        solid = true;
        return kGrassH;
    }
    solid = false;
    return -12.f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.04f) return 3;
    if (touchedGrass_ && !air_) return 2;
    return 1;
}

bool Game::wheelsOnGrass() const {
    if (air_) return false;
    float rear = x_ - kRear;
    float front = x_ + kFront;
    return rear >= kGrass0 && front <= kGrass1;
}

void Game::begin() {
    x_ = 2.6f;
    y_ = 0.f;
    vx_ = 0.f;
    vy_ = 0.f;
    air_ = false;
    touchedGrass_ = false;
    hold_ = 0;
    race_ = 0;
    crew_ = kCrew;
    phase_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = chimeStep_ = dustCursor_ = 0;
    tone0_ = chimeT_ = thumpT_ = 0;
    dustT_ = shake_ = throttleIn_ = spin_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : dust_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    x_ = 38.f;
    y_ = kGrassH;
    camX_ = 36.f;
    crew_ = kCrew;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    blip(392.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(4, 7, 5));
    sys.apu.setMaster(0.72f);
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

void Game::controls(float& throttle, float& brake) {
    const gs::Pad& p = sys_->pad;
    throttle = brake = 0;
    bool go = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_C) || p.down(gs::BTN_UP);
    bool stop = p.down(gs::BTN_LEFT) || p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_DOWN);
    if (p.accel > 0.12f) go = true;
    if (p.brake > 0.12f) stop = true;
    if (p.axisX > 0.28f) go = true;
    if (p.axisX < -0.28f) stop = true;
    if (go && !stop) throttle = 1.f;
    else if (stop && !go) brake = 1.f;
    if (p.accel > 0.12f && throttle > 0.f) throttle = clampf(p.accel, 0.f, 1.f);
    if (p.brake > 0.12f && brake > 0.f) brake = clampf(p.brake, 0.f, 1.f);
}

void Game::pilot(float& throttle, float& brake) {
    throttle = brake = 0;
    const float want = 13.2f;
    if (!touchedGrass_) {
        phase_ = air_ ? 1 : 0;
        if (air_) return;
        if (vx_ < want - 0.30f) throttle = vx_ < want - 2.8f ? 1.f : 0.72f;
        else if (vx_ > want + 0.40f) brake = 0.50f;
        return;
    }
    phase_ = 2;
    float room = (kGrass1 - kFront - 0.45f) - x_;
    if (vx_ > kStop) {
        float stopDist = (vx_ * vx_) / (2.f * 14.5f);
        brake = (room < stopDist + 2.6f || vx_ > 2.0f) ? 1.f : 0.42f;
        if (room < 1.4f) brake = 1.f;
    } else if (vx_ < -kStop) {
        throttle = 0.5f;
    } else if (room < 0.5f) {
        brake = 1.f;
    }
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    banner_ = std::strcmp(why, "the other crew") == 0 ? &art_.crew : &art_.missed;
    shake_ = 0.7f;
    vx_ = 0;
    sys_->rumble(0.5f, 0.22f, 160);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.32f, 120.f, 0.26f);
    sys_->apu.tone(0, 58.f, 0.05f);
    tone0_ = 0.32f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = 0;
    vy_ = 0;
    std::snprintf(why_, sizeof why_, "full stop");
    banner_ = &art_.stopped;
    chime(4);
    sys_->rumble(0.24f, 0.10f, 140);
    sys_->setLight(36, 170, 64);
}

void Game::physics(float throttle, float brake) {
    if (air_) {
        vy_ -= kG * kDt;
        vx_ -= vx_ * 0.07f * kDt;
        x_ += vx_ * kDt;
        y_ += vy_ * kDt;
        bool solid = false;
        float gh = groundH(x_, solid);
        if (solid && y_ <= gh && vy_ <= 0.4f) {
            y_ = gh;
            float hit = -vy_;
            vy_ = 0;
            air_ = false;
            vx_ *= hit > 9.f ? 0.84f : 0.93f;
            thumpT_ = 0.16f;
            shake_ = std::min(1.1f, 0.14f + hit * 0.04f);
            sys_->apu.noiseBurst(0.15f + std::min(hit, 12.f) * 0.01f, 240.f, 0.10f);
            sys_->rumble(0.20f, 0.08f, 70);
            if (x_ >= kGrass0 && x_ <= kGrass1) touchedGrass_ = true;
        } else if (y_ < -5.2f) {
            if (x_ >= kGrass1 - 0.05f) fail("missed the end");
            else fail("missed the grass");
        }
        return;
    }

    bool solid = false;
    float gh = groundH(x_, solid);
    if (!solid) {
        air_ = true;
        vy_ = std::min(vy_, 0.f);
        return;
    }
    y_ = gh;

    bool ahead = false;
    float ha = groundH(x_ + 0.25f, ahead);
    float slope = 0.f;
    if (ahead) slope = (ha - gh) / 0.25f;

    float a = throttle * kThrottle;
    if (brake > 0.f) {
        if (std::fabs(vx_) < 0.05f) vx_ = 0.f;
        else a -= brake * kBrake * signf(vx_);
    }
    a -= vx_ * kDrag;
    if (std::fabs(vx_) > 0.02f) a -= kRoll * signf(vx_);
    a -= slope * kUphill;
    vx_ += a * kDt;
    vx_ = clampf(vx_, -3.0f, 15.6f);
    x_ += vx_ * kDt;

    bool now = false;
    float hn = groundH(x_, now);
    if (!now) {
        bool back = false;
        float hb = groundH(x_ - 0.35f, back);
        float sl = back ? (y_ - hb) / 0.35f : (kLipH / (kLip - kRamp0));
        air_ = true;
        vy_ = vx_ * std::max(0.f, sl);
        y_ = back ? hb : y_;
        return;
    }
    y_ = hn;
    if (x_ >= kGrass0 && x_ <= kGrass1) touchedGrass_ = true;
    if (x_ - kRear < 0.35f && vx_ < 0.f) {
        x_ = 0.35f + kRear;
        vx_ = 0.f;
    }
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    if (crew_ <= 0.f) {
        fail("the other crew");
        return;
    }
    if (!air_ && touchedGrass_) {
        float front = x_ + kFront;
        if (front > kGrass1 + 0.04f) {
            fail("missed the end");
            return;
        }
    }
    if (wheelsOnGrass() && std::fabs(vx_) <= kStop) {
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else {
        hold_ = 0;
        if (!air_ && touchedGrass_ && std::fabs(vx_) <= kStop && race_ > 0.8f) {
            if (x_ - kRear < kGrass0) fail("rolled off the grass");
            else if (x_ + kFront > kGrass1) fail("missed the end");
        }
    }
}

void Game::audio() {
    float roll = (!air_ && mode_ == Mode::Run) ? 0.014f + std::fabs(vx_) * 0.0034f : 0.004f;
    sys_->apu.noise(roll, 360.f + std::fabs(vx_) * 42.f, false);
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {349.f, 440.f, 523.f, 698.f};
            int n = std::min(chimeStep_, 3);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.16f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && throttleIn_ > 0.05f && !air_) {
        float wob = 0.7f + 0.3f * std::sin(spin_ * 11.f);
        sys_->apu.tone(2, 72.f + throttleIn_ * 48.f + std::fabs(vx_) * 6.f, 0.020f * wob);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
    if (mode_ == Mode::Run && crew_ < 4.f && crew_ > 0.f) {
        float tick = std::fmod(crew_, 0.5f);
        if (tick < kDt) sys_->apu.tone(1, 880.f, 0.03f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
        x_ = 39.f + std::sin(t_ * 0.55f) * 1.2f;
        y_ = kGrassH;
        vx_ = std::cos(t_ * 0.55f) * 0.7f;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
            blip(240.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            race_ += kDt;
            crew_ -= kDt;
            float throttle = 0, brake = 0;
            if (bot_) pilot(throttle, brake);
            else controls(throttle, brake);
            throttleIn_ = throttle;
            physics(throttle, brake);
            if (mode_ == Mode::Run) judge();
            spin_ += std::fabs(vx_) * kDt;
            if (mode_ == Mode::Run && !air_ && (std::fabs(vx_) > 1.5f || brake > 0.45f)) {
                dustT_ -= kDt;
                if (dustT_ <= 0.f) {
                    dustT_ = brake > 0.45f ? 0.04f : 0.08f;
                    dust_[dustCursor_].x = x_ - kRear;
                    dust_[dustCursor_].y = y_;
                    dust_[dustCursor_].life = 1.f;
                    dustCursor_ = (dustCursor_ + 1) % 12;
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
        if (p.life > 0.f) p.life -= kDt * 0.9f;

    if (mode_ == Mode::Win || hold_ > 0.04f) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 40, 28);
    else if (mode_ == Mode::Run && wheelsOnGrass()) sys.setLight(48, 150, 55);
    else if (mode_ == Mode::Run && air_) sys.setLight(80, 130, 180);
    else if (mode_ == Mode::Run && crew_ < 4.f) sys.setLight(180, 90, 30);
    else sys.setLight(40, 90, 48);

    float look = mode_ == Mode::Title ? 38.f : x_ + clampf(vx_ * 0.26f, -1.4f, 4.2f);
    camX_ += (look - camX_) * (1.f - std::exp(-kDt * 3.1f));
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 46.f) * shake_ * 0.32f;
        shake_ = std::max(0.f, shake_ - kDt * 1.8f);
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
        hudC(20, "TAKE THE KART", PAL_BANNER);
        hudC(21, "LAND ON THE GRASS", PAL_HUD);
        hudC(22, "COME TO A FULL STOP", PAL_WIN);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(24, "RIGHT THROTTLE   LEFT BRAKE", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "RETURN", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 KART GRASS", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "CREW %04.1f", std::max(0.f, crew_));
    hud(28, 0, buf, crew_ < 4.f ? PAL_ALERT : PAL_CREW);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(15, "FULL STOP ON THE GRASS", PAL_WIN);
        std::snprintf(buf, sizeof buf, "AHEAD OF THE CREW  %.1f", std::max(0.f, crew_));
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "RETURN DRIVES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(15, why_[0] ? why_ : "MISSED", PAL_ALERT);
        hudC(16, "THE LEG IS OVER", PAL_HUD);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %04.1f", std::fabs(vx_));
    hud(1, 1, buf, PAL_HUD);
    const char* line = "TAKE THE RAMP";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        int n = std::max(1, std::min(5, int(hold_ / kHoldNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "HOLD %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (wheelsOnGrass()) {
        line = "ON THE GRASS  STOP";
        pal = PAL_WIN;
    } else if (air_) {
        line = x_ > kLip ? "LAND ON THE GRASS" : "IN THE AIR";
        pal = PAL_BANNER;
    } else if (touchedGrass_) {
        line = "STAY ON THE GRASS";
        pal = PAL_ALERT;
    } else if (x_ > kRamp0) {
        line = "HOLD THE SPEED";
        pal = PAL_HUD;
    }
    hudC(26, line, pal);
    hud(1, 27, "RIGHT GO   LEFT BRAKE", PAL_HUD);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool hflip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -30 || cy + h * 0.5f < -30 || cx - w * 0.5f > gs::SCREEN_W + 30 ||
        cy - h * 0.5f > gs::SCREEN_H + 30)
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
    if (cx + w * 0.5f < -20 || cx - w * 0.5f > gs::SCREEN_W + 20) return;
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
float Game::sy(float h) const { return kBaseY - h * kPpm; }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / 148.f;
        uint16_t sky = lerpC(gs::rgb4(3, 6, 12), gs::rgb4(12, 13, 8), clampf(u, 0.f, 1.f));
        if (y > 170) sky = lerpC(gs::rgb4(2, 3, 3), gs::rgb4(1, 2, 2), clampf((y - 170.f) / 40.f, 0.f, 1.f));
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 72.f, 26.f, float(art_.title.h) * 1.1f, PAL_BANNER);
        spr(art_.grassWord, 214.f, 28.f, float(art_.grassWord.h) * 1.1f, PAL_BANNER);
        spr(art_.clock, 160.f, 58.f, 22.f, PAL_CREW);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 32.f, float(banner_->h), pal);
    }

    auto groundSpan = [&](float x0, float x1, float h0, float h1, const gs::Mipped& tile, int pal, float thick) {
        float mid = (x0 + x1) * 0.5f;
        float hm = (h0 + h1) * 0.5f;
        float w = std::fabs(x1 - x0) * kPpm + 1.5f;
        sprBox(tile, sx(mid), sy(hm) + thick * 0.35f, w, thick, pal);
    };

    float bodyH = 34.f;
    float bob = air_ ? 0.f : std::sin(spin_ * 9.f) * 0.7f;
    float by = sy(y_) - 14.f + bob;
    int spin = int(spin_ * 4.f) & 1;
    float lean = air_ ? clampf(-vy_ * 0.35f, -5.f, 7.f) : 0.f;
    spr(art_.kart, sx(x_) + lean, by + 4.f, bodyH, PAL_KART, true);
    spr(art_.tyre[spin], sx(x_ - 0.72f) + lean * 0.3f, sy(y_) - 4.f, 14.f, PAL_KART);
    spr(art_.tyre[spin], sx(x_ + 0.78f) + lean * 0.5f, sy(y_) - 4.f, 14.f, PAL_KART);
    spr(art_.kart, sx(x_) + lean * 0.25f, by, bodyH, PAL_KART);

    for (const Puff& p : dust_)
        if (p.life > 0.05f)
            spr(art_.dust, sx(p.x), sy(p.y) - 3.f - (1.f - p.life) * 9.f, 5.f + p.life * 6.f, PAL_DUST);

    int gpal = (wheelsOnGrass() || mode_ == Mode::Win) ? PAL_WIN : PAL_GRASS;
    for (float x = kGrass0; x < kGrass1 - 0.05f; x += 1.2f) {
        float x1 = std::min(kGrass1, x + 1.2f);
        groundSpan(x, x1, kGrassH, kGrassH, art_.grass, gpal, 20.f);
        if (int(x * 2.f) % 4 == 0) spr(art_.tuft, sx(x + 0.35f), sy(kGrassH) - 7.f, 12.f, PAL_GRASS);
    }
    spr(art_.flag, sx(kGrass0 + 0.15f), sy(kGrassH) - 20.f, 34.f, PAL_FLAG);
    spr(art_.flag, sx(kGrass1 - 0.1f), sy(kGrassH) - 24.f, 38.f, PAL_FLAG);
    spr(art_.clock, sx((kGrass0 + kGrass1) * 0.5f), sy(kGrassH) - 28.f, 16.f, PAL_CREW);

    for (float x = -2.f; x < kRamp0; x += 1.45f) groundSpan(x, x + 1.45f, 0.f, 0.f, art_.dirt, PAL_DIRT, 24.f);
    for (float x = kRamp0; x < kLip; x += 1.1f) {
        float x1 = std::min(kLip, x + 1.1f);
        bool s0, s1;
        float h0 = groundH(x, s0);
        float h1 = groundH(x1, s1);
        groundSpan(x, x1, h0, h1, art_.ramp, PAL_RAMP, 18.f);
    }
    for (float x = kLip + 0.5f; x < kGrass0 - 0.2f; x += 1.5f)
        sprBox(art_.gap, sx(x), sy(-0.3f) + 16.f, 1.5f * kPpm, 34.f, PAL_GAP);
    for (float x = kGrass1 + 0.5f; x < kGrass1 + 8.f; x += 1.5f)
        sprBox(art_.gap, sx(x), sy(-0.5f) + 18.f, 1.5f * kPpm, 38.f, PAL_GAP);

    spr(art_.tree, sx(5.2f), sy(0.f) - 34.f, 50.f, PAL_TREE);
    {
        bool onRamp = false;
        spr(art_.tree, sx(16.4f), sy(groundH(16.4f, onRamp)) - 30.f, 42.f, PAL_TREE);
    }
    spr(art_.tree, sx(33.f), sy(kGrassH) - 38.f, 54.f, PAL_TREE);
    spr(art_.tree, sx(46.f), sy(kGrassH) - 30.f, 42.f, PAL_TREE);

    drawHud();
}

}  // namespace kartgrass
