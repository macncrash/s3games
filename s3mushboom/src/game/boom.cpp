#include "game/boom.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace mushboom {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GRAV = 320.f;
constexpr float JUMP = 180.f;
constexpr float MAX_V = 200.f;
constexpr float ICE_END = 440.f;
constexpr float BOOM_X0 = 490.f;
constexpr float BOOM_X1 = 720.f;
constexpr float DECK = 22.f;
constexpr float DR = 8.f;
constexpr float LIMIT = 24.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::blip(float freq) { sys_->apu.tone(0, freq, 0.08f); }

void Game::fail(const char* why) {
    why_ = why;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    blip(90.f);
}

void Game::succeed() {
    why_ = "DELIVERED";
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    blip(523.f);
    sys_->apu.tone(1, 659.f, 0.07f);
}

void Game::begin() {
    x_ = 70.f;
    y_ = 0.f;
    vx_ = 0.f;
    vy_ = 0.f;
    carried_ = true;
    onGround_ = true;
    driveDeck_ = false;
    jumped_ = false;
    still_ = 0.f;
    time_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    dx_ = x_ + 8.f;
    dy_ = 16.f;
    dvx_ = 0.f;
    dvy_ = 0.f;
    cam_ = x_ - 70.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.resize(64, 32);
    sys.vdp.setFogColor(gs::rgb4(8, 10, 12));
    sys.apu.setMaster(0.7f);
    begin();
    mode_ = bot_ ? Mode::Run : Mode::Title;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Fail || mode_ == Mode::Win) return 4;
    if (driveDeck_) return 3;
    if (!carried_) return 2;
    return 1;
}

float Game::screenX(float wx) const { return wx - cam_; }
float Game::screenY(float wy) const { return 168.f - wy; }

void Game::botInput(float& gas, bool& jump, bool& release) {
    gas = 1.f;
    jump = false;
    release = false;
    if (!jumped_ && onGround_ && x_ >= 400.f && vx_ >= 170.f) jump = true;
    if (carried_ && !onGround_ && y_ >= 46.f && vy_ <= 0.f) release = true;
    if (driveDeck_) gas = 0.f;
}

void Game::stepRun() {
    const gs::Pad& p = sys_->pad;
    float gas = 0.f;
    bool jump = false;
    bool release = false;
    if (bot_) {
        botInput(gas, jump, release);
    } else {
        if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.accel > 0.15f) gas += 1.f;
        if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_DOWN) || p.brake > 0.15f) gas -= 1.f;
        jump = p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_C);
        release = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_Y);
    }

    if (onGround_) {
        if (gas > 0.f) vx_ += 280.f * DT;
        else if (gas < 0.f) vx_ -= 200.f * DT;
        else vx_ -= vx_ * 1.4f * DT;
        vx_ = clampf(vx_, -50.f, MAX_V);
        if (jump && !jumped_) {
            vy_ = JUMP;
            onGround_ = false;
            jumped_ = true;
            blip(240.f);
        }
    }
    vy_ -= GRAV * DT;
    x_ += vx_ * DT;
    y_ += vy_ * DT;
    if (x_ < 16.f) {
        x_ = 16.f;
        vx_ = 0.f;
    }

    const bool ice = x_ < ICE_END && y_ <= 0.f;
    if (ice) {
        y_ = 0.f;
        if (vy_ < 0.f) vy_ = 0.f;
        onGround_ = true;
    } else if (y_ > 0.5f) {
        onGround_ = false;
    }

    if (carried_) {
        dx_ = x_ + 10.f;
        dy_ = y_ + 16.f;
        dvx_ = vx_;
        dvy_ = vy_;
        const bool overDeck = dx_ > BOOM_X0 && dx_ < BOOM_X1 && dy_ < DECK + 20.f && dy_ > DECK - 4.f;
        if (overDeck) {
            fail("SCRAPED THE BOOM");
            return;
        }
        if (y_ < -6.f && x_ > ICE_END - 8.f) {
            fail("IN THE DRINK");
            return;
        }
        if (release && !onGround_) {
            carried_ = false;
            dvy_ = vy_ - 20.f;
            driveDeck_ = false;
            still_ = 0.f;
            blip(330.f);
        }
    } else if (!driveDeck_) {
        dvy_ -= GRAV * DT;
        dx_ += dvx_ * DT;
        dy_ += dvy_ * DT;
        const bool over = dx_ > BOOM_X0 + 6.f && dx_ < BOOM_X1 - 6.f;
        if (over && dy_ <= DECK + DR && dy_ >= DECK - 2.f && dvy_ <= 0.f) {
            dy_ = DECK + DR;
            dvy_ = 0.f;
            driveDeck_ = true;
            still_ = 0.f;
            blip(440.f);
        } else if (dy_ <= DR && dx_ < ICE_END) {
            dy_ = DR;
            dvy_ = 0.f;
            dvx_ *= 0.8f;
            still_ += DT;
            if (still_ > 0.25f) {
                fail("NOT THE BOOM");
                return;
            }
        } else if (dy_ < -8.f || dx_ > BOOM_X1 + 24.f || dx_ < -20.f) {
            fail(dx_ > BOOM_X1 ? "OFF THE END" : "MISSED THE BOOM");
            return;
        }
    } else {
        dvx_ *= 0.85f;
        if (std::fabs(dvx_) < 8.f) dvx_ = 0.f;
        dx_ += dvx_ * DT;
        dy_ = DECK + DR;
        if (dx_ < BOOM_X0 + DR || dx_ > BOOM_X1 - DR) {
            driveDeck_ = false;
            dvy_ = -30.f;
            fail("OFF THE END");
            return;
        }
        still_ += DT;
        if (still_ > 0.35f && dvx_ == 0.f) {
            succeed();
            return;
        }
    }

    if (!carried_ && y_ < -30.f) y_ = -30.f;

    time_ += DT;
    anim_++;
    if (time_ > LIMIT) {
        fail(carried_ ? "STILL ABOARD" : "TOO SLOW");
        return;
    }

    float focus = carried_ ? x_ : dx_;
    float want = focus - 110.f;
    cam_ += (want - cam_) * 0.12f;
}

void Game::paintSky() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = float(y) / float(gs::SCREEN_H - 1);
        int r = int(6 + (12 - 6) * t);
        int g = int(8 + (13 - 8) * t);
        int b = int(12 + (14 - 12) * t);
        if (y > 168) {
            r = 9;
            g = 11;
            b = 12;
        }
        sys_->vdp.lineBackdrop[y] = uint16_t(gs::rgb4(r, g, b));
        sys_->vdp.lineFog[y] = 0;
    }
}

void Game::spr(const gs::Mipped& m, float wx, float wy, float destH, int pal, bool flip, bool shadow) {
    if (destH < 1.5f || m.h < 1) return;
    float h = destH;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    float sx = screenX(wx) - s.w * 0.5f;
    float sy = screenY(wy) - s.h * 0.5f;
    s.x = int16_t(std::lround(sx));
    s.y = int16_t(std::lround(sy));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float left = x - float(std::strlen(s)) * adv * 0.5f;
    for (size_t i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float cx = left + float(i) * adv + g.w * scale * 0.5f;
        float ch = g.h * scale;
        gs::Sprite sp;
        sp.w = int16_t(std::lround(g.w * scale));
        sp.h = int16_t(std::lround(ch));
        sp.x = int16_t(std::lround(cx - sp.w * 0.5f));
        sp.y = int16_t(std::lround(y - sp.h * 0.5f));
        sp.img = g.pick(ch);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        begin();
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_C)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        stepRun();
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        begin();
        over_ = false;
        won_ = false;
        mode_ = Mode::Run;
    }

    sys.vdp.clearSprites();
    sys.vdp.HUD.clear();
    paintSky();

    for (float wx = std::floor(cam_ / 36.f) * 36.f - 36.f; wx < cam_ + 360.f; wx += 36.f) {
        if (wx < ICE_END - 10.f) spr(art_.snow, wx + 18.f, -2.f, 18.f, PAL_SNOW);
        else spr(art_.water, wx + 18.f, -6.f, 22.f, PAL_WATER);
    }
    spr(art_.post, BOOM_X0 + 18.f, DECK - 20.f, 70.f, PAL_BOOM);
    spr(art_.post, BOOM_X1 - 28.f, DECK - 20.f, 70.f, PAL_BOOM);
    for (float wx = BOOM_X0; wx < BOOM_X1 - 4.f; wx += 40.f) spr(art_.plank, wx + 22.f, DECK - 2.f, 16.f, PAL_BOOM);

    float mushH = 52.f;
    spr(art_.shadow, x_ + 6.f, 0.f, 12.f, 7, false, true);
    spr(art_.mush[(anim_ / 8) & 1], x_ + 8.f, y_ + 16.f, mushH, PAL_MUSH);
    spr(art_.drive, dx_, dy_, 16.f, PAL_DRIVE);

    if (mode_ == Mode::Title) {
        text("MUSH BOOM", 160.f, 36.f, 1.15f, PAL_HUD);
        text("SET THE DRIVE", 160.f, 64.f, 0.62f, PAL_HUD);
        hudC(22, "A RUN   C JUMP   B DROP", PAL_HUD);
        hudC(24, "START", PAL_HUD);
    } else if (mode_ == Mode::Run) {
        char buf[40];
        std::snprintf(buf, sizeof buf, carried_ ? "CARRY" : driveDeck_ ? "SET" : "FALL");
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%.0f", time_);
        hud(36, 1, buf, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("DELIVERED", 160.f, 40.f, 1.05f, PAL_HUD);
        hudC(24, "THE DRIVE IS ON THE BOOM", PAL_HUD);
    } else {
        text(why_ ? why_ : "FAIL", 160.f, 40.f, 0.7f, PAL_HUD);
        hudC(24, "START  AGAIN", PAL_HUD);
    }
    hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);

    if (mode_ != Mode::Run && mode_ != Mode::Title) {
        sys.apu.tone(0, 0, 0);
    }
}

}  // namespace mushboom
