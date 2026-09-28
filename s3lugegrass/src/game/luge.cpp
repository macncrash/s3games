#include "luge.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace lugegrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kStop = 0.16f;
constexpr float kHoldNeed = 0.45f;
constexpr float kOutNeed = 0.70f;
constexpr float kWest = 3.f;
constexpr float kGrade = 1.82f;
constexpr float kBrake = 8.2f;
constexpr float kDrag = 0.05f;
constexpr float kGrass = 2.2f;
constexpr float kGoal = 108.f;

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
    if (onGrass()) return 2;
    return 1;
}

bool Game::onGrass() const {
    float tail = x_ - kHalf;
    float nose = x_ + kHalf;
    return tail >= kGrassL && nose <= kGrassR;
}

void Game::begin() {
    x_ = 14.f;
    vel_ = 8.4f;
    hold_ = 0;
    outT_ = 0;
    race_ = 0;
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chimeN_ = chimeStep_ = 0;
    sprayCursor_ = 0;
    tone0_ = chimeT_ = thumpT_ = 0;
    sprayT_ = shake_ = 0;
    brakeIn_ = 0;
    banner_ = nullptr;
    why_[0] = 0;
    for (Puff& p : spray_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = 70.f;
    x_ = 36.f;
    vel_ = 0.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = x_;
    blip(420.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 11, 9));
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.05f, 0.10f, 0.04f);
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
    bool grass = x_ >= kGrassL;
    float decel = (grass ? kGrass : 0.f) + kBrake;
    float stopD = (vel_ * vel_) / (2.f * std::max(1.f, decel));
    phase_ = grass ? 2 : (x_ > kGrassL - 18.f ? 1 : 0);
    if (dx <= 0.35f) {
        brake = vel_ > 0.12f ? 1.f : 0.2f;
    } else if (grass || stopD >= dx - 2.2f) {
        float over = stopD - (dx - 3.6f);
        brake = clampf(0.18f + over * 0.16f, 0.f, 1.f);
        if (vel_ > 11.f && dx < 20.f) brake = 1.f;
    } else if (vel_ > 15.f) {
        brake = 0.35f;
    } else {
        brake = 0.f;
    }
}

void Game::physics(float brake) {
    bool grass = x_ - kHalf * 0.2f >= kGrassL;
    float accel = (grass ? -kGrass : kGrade) - brake * kBrake - vel_ * kDrag;
    vel_ += accel * kDt;
    if (vel_ < 0.f) vel_ = 0.f;
    vel_ = std::min(vel_, 18.f);
    x_ += vel_ * kDt;
    if (!std::isfinite(x_) || !std::isfinite(vel_)) {
        fail("lost the run");
        return;
    }
    float tail = x_ - kHalf;
    if (tail < kWest) {
        x_ += kWest - tail;
        vel_ = std::max(0.f, vel_);
    }
    float nose = x_ + kHalf;
    if (nose > kGrassR) {
        if (thumpT_ <= 0.f) {
            thumpT_ = 0.28f;
            shake_ = 0.7f;
            sys_->apu.noiseBurst(0.26f, 140.f, 0.12f);
            sys_->rumble(0.35f, 0.12f, 80);
        }
        fail("missed the end");
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(why_, sizeof why_, "full stop on the grass");
    banner_ = &art_.stopped;
    chime(4);
    sys_->rumble(0.22f, 0.08f, 120);
    sys_->setLight(40, 160, 60);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    bool early = why && std::strstr(why, "short");
    banner_ = early ? &art_.shortStop : &art_.missed;
    shake_ = 0.65f;
    sys_->rumble(0.45f, 0.18f, 140);
    sys_->setLight(160, 40, 24);
    sys_->apu.noiseBurst(0.3f, 100.f, 0.24f);
    sys_->apu.tone(0, 74.f, 0.05f);
    tone0_ = 0.28f;
}

void Game::judge() {
    if (mode_ != Mode::Run) return;
    bool in = onGrass();
    if (in && !announced_) {
        announced_ = true;
        blip(640.f);
    }
    if (in && vel_ <= kStop) {
        outT_ = 0;
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else if (!in && vel_ <= kStop && x_ > 30.f) {
        hold_ = 0;
        outT_ += kDt;
        if (outT_ >= kOutNeed) fail("stopped short of the grass");
    } else {
        hold_ = 0;
        if (!in) outT_ = 0;
    }
}

void Game::audio() {
    float hiss = mode_ == Mode::Run ? 0.01f + vel_ * 0.0032f : 0.006f;
    sys_->apu.noise(hiss, 480.f + vel_ * 36.f, false);
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
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
    if (mode_ == Mode::Run && brakeIn_ > 0.08f && vel_ > 0.4f) {
        sys_->apu.tone(2, 80.f + brakeIn_ * 50.f, 0.02f + brakeIn_ * 0.03f);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
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
        x_ = 40.f + std::sin(t_ * 0.35f) * 6.f;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
            blip(260.f);
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
            if (vel_ > 2.f) {
                sprayT_ -= kDt;
                if (sprayT_ <= 0.f) {
                    sprayT_ = 0.07f;
                    spray_[sprayCursor_].x = x_ - kHalf * 0.85f;
                    spray_[sprayCursor_].life = 1.f;
                    sprayCursor_ = (sprayCursor_ + 1) % 12;
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
        if (p.life > 0.f) p.life -= kDt * 0.85f;

    if (mode_ == Mode::Win || hold_ > 0.05f) sys.setLight(36, 160, 64);
    else if (mode_ == Mode::Fail) sys.setLight(160, 36, 24);
    else if (mode_ == Mode::Run && onGrass()) sys.setLight(40, 140, 50);
    else sys.setLight(70, 110, 150);

    float look = mode_ == Mode::Run ? x_ + vel_ * 0.32f : (mode_ == Mode::Title ? 78.f : x_);
    camX_ += (look - camX_) * (1.f - std::exp(-kDt * 3.2f));
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
    char buf[72];
    if (mode_ == Mode::Title) {
        hudC(21, "LAND ON THE GRASS", PAL_BANNER);
        hudC(22, "COME TO A FULL STOP", PAL_HUD);
        hudC(23, "MISSING THE END FAILS THE LEG", PAL_ALERT);
        hudC(24, "B BRAKES THE RUNNERS", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "RETURN", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 LUGE GRASS", PAL_BANNER);
    int sec = int(race_);
    int frac = int((race_ - sec) * 10.f);
    std::snprintf(buf, sizeof buf, "LEG %02d.%d", sec, frac);
    hud(29, 0, buf, PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(16, "RETURN CONTINUES", PAL_HUD);
        hudC(17, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "FULL STOP ON THE GRASS", PAL_WIN);
        if (!bot_) hudC(18, "RETURN TAKES THE ICE AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "MISSED THE END", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %05.1f", vel_);
    hud(1, 1, buf, vel_ <= kStop && !onGrass() && x_ > 30.f ? PAL_ALERT : PAL_HUD);
    const char* line = "RIDE THE ICE";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        int n = std::max(1, std::min(5, int(hold_ / kHoldNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "STOP %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (onGrass()) {
        line = "ON THE GRASS  HOLD THE BRAKE";
        pal = PAL_WIN;
    } else if (x_ + kHalf > kGrassL) {
        line = "GET THE WHOLE SLED ON THE GRASS";
        pal = PAL_ALERT;
    } else if (x_ > kGrassL - 16.f) {
        line = "GRASS AHEAD";
        pal = PAL_BANNER;
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

float Game::sx(float wx) const { return 148.f + (wx - camX_) * kPpm; }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = lerpC(gs::rgb4(5, 8, 13), gs::rgb4(12, 14, 13), clampf(y / 130.f, 0.f, 1.f));
        if (y > 128) {
            float u = (y - 128.f) / 70.f;
            sky = lerpC(gs::rgb4(8, 12, 7), gs::rgb4(4, 7, 4), clampf(u, 0.f, 1.f));
        }
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 78.f, 28.f, float(art_.title.h), PAL_BANNER);
        spr(art_.grassWord, 210.f, 30.f, float(art_.grassWord.h), PAL_WIN);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 34.f, float(banner_->h), pal);
    }

    int flap = int(t_ * 6.f) & 1;
    spr(art_.bird[flap], 40.f + std::fmod(t_ * 18.f, 260.f), 36.f, 8.f, PAL_BIRD);
    spr(art_.bird[1 - flap], 200.f - std::fmod(t_ * 12.f, 180.f), 48.f, 7.f, PAL_BIRD);

    float base = std::floor((camX_ - 40.f) / 10.f) * 10.f;
    for (int i = 0; i < 12; i++) {
        float bx = base + i * 10.f;
        if (bx + 6.f < kGrassL) {
            spr(art_.ice, sx(bx), 132.f, 26.f, PAL_ICE);
            if ((int(bx) / 10) % 4 == 0) spr(art_.pine, sx(bx), 92.f, 42.f, PAL_PINE);
        } else if (bx < kGrassR) {
            spr(art_.grass, sx(bx), 134.f, 28.f, PAL_GRASS);
            if ((int(bx) / 10) % 2 == 0) spr(art_.tuft, sx(bx + 2.f), 118.f, 12.f, PAL_TUFT);
        } else if (bx < kGrassR + 40.f) {
            spr(art_.dirt, sx(bx), 136.f, 22.f, PAL_DIRT);
        }
    }

    spr(art_.post, sx(kGrassL), 108.f, 46.f, PAL_POST);
    spr(art_.post, sx(kGrassR), 108.f, 46.f, PAL_POST);
    spr(art_.flag, sx((kGrassL + kGrassR) * 0.5f), 96.f, 34.f, PAL_FLAG);
    spr(art_.hut, sx(kGrassL - 16.f), 100.f, 36.f, PAL_HUT);

    for (const Puff& p : spray_) {
        if (p.life <= 0.f) continue;
        float h = 8.f + (1.f - p.life) * 10.f;
        int pal = x_ > kGrassL ? PAL_SPRAY : PAL_ICE;
        spr(art_.spray, sx(p.x), 146.f - (1.f - p.life) * 8.f, h, pal);
    }

    float ly = 128.f;
    spr(art_.luge, sx(x_), ly, 28.f, PAL_LUGE, false, false);
    spr(art_.luge, sx(x_) + 3.f, ly + 6.f, 28.f, PAL_LUGE, true, false);

    drawHud();
    v.hudEnabled = true;
}

}  // namespace lugegrass
