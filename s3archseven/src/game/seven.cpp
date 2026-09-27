#include "game/seven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace archseven {
namespace {

constexpr int kFullDraw = 18;
constexpr int kWobbleAt = 10;
constexpr int kFlight = 10;
constexpr int kCall = 16;
constexpr float kDrop = 46.f;

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

bool Game::audit() {
    auto at = [](float r, int pts, const char* name) {
        Hit h = hitAt(r);
        if (h.pts != pts || std::strcmp(h.name, name) != 0) {
            std::fprintf(stderr, "s3archseven ring %s scored %s +%d at r %.2f\n", name, h.name, h.pts, r);
            return false;
        }
        return true;
    };
    if (!at(0.f, 3, "GOLD")) return false;
    if (!at(kRGold, 3, "GOLD")) return false;
    if (!at(kRGold + 0.01f, 2, "RING")) return false;
    if (!at(kRRing, 2, "RING")) return false;
    if (!at(kRRing + 0.01f, 1, "RED")) return false;
    if (!at(kRRed, 1, "RED")) return false;
    if (!at(kRRed + 0.01f, 0, "MISS")) return false;
    if (!at(80.f, 0, "MISS")) return false;
    Hit step = hitAt(0.f);
    if (6 + step.pts < kRace) {
        std::fprintf(stderr, "s3archseven a gold from six must reach seven\n");
        return false;
    }
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3archseven rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    mode_ = Mode::Title;
    aimX_ = kCx;
    aimY_ = kCy;
    last_[0] = 0;
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    gain_ = 0;
    markN_ = 0;
    yours_ = true;
    won_ = false;
    over_ = false;
    last_[0] = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    drawTick_ = 0;
    steady_ = 0;
    mode_ = Mode::Aim;
    skipHold_ = !bot_;
}

bool Game::drawHeld() const {
    if (!sys_) return false;
    if (sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_Z)) return true;
    return sys_->pad.accel > 0.45f;
}

void Game::steer() {
    if (!sys_) return;
    float x = sys_->pad.axisX;
    float y = -sys_->pad.axisY;
    if (sys_->pad.down(gs::BTN_LEFT)) x = -1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) x = 1.f;
    if (sys_->pad.down(gs::BTN_UP)) y = -1.f;
    if (sys_->pad.down(gs::BTN_DOWN)) y = 1.f;
    float mag = std::sqrt(x * x + y * y);
    if (mag > 1.f) {
        x /= mag;
        y /= mag;
    }
    float sp = sys_->pad.down(gs::BTN_TURBO) ? 4.2f : 2.4f;
    aimX_ = clampf(aimX_ + x * sp, kCx - 90.f, kCx + 70.f);
    aimY_ = clampf(aimY_ + y * sp, kCy - 70.f, kCy + 70.f);
    if (sys_->pad.pressed(gs::BTN_B)) {
        aimX_ = kCx;
        aimY_ = kCy;
    }
}

Hit Game::predict(float& x, float& y) const {
    float power = 1.f;
    int steady = 0;
    if (mode_ == Mode::Nock) {
        power = std::min(1.f, float(drawTick_) / float(kFullDraw));
        steady = steady_;
    }
    float ox = 0.f;
    float oy = 0.f;
    if (steady > kWobbleAt) {
        float mag = std::min(18.f, float(steady - kWobbleAt) * 0.7f);
        oy = std::sin(float(steady) * 0.45f) * mag;
        ox = oy * 0.25f;
    }
    x = aimX_ + ox;
    y = aimY_ + (1.f - power) * kDrop + oy;
    float dx = x - kCx;
    float dy = y - kCy;
    return hitAt(std::sqrt(dx * dx + dy * dy));
}

void Game::loose() {
    predict(landX_, landY_);
    mode_ = Mode::Flight;
    flightT_ = 0;
    if (sys_) {
        sys_->apu.tone(0, 220.f, 0.05f);
        blipLeft_ = 6;
        blipCh_ = 0;
    }
}

void Game::arrive() {
    Hit h = hitAt(std::hypot(landX_ - kCx, landY_ - kCy));
    if (markN_ < 12) {
        marks_[markN_].x = landX_;
        marks_[markN_].y = landY_;
        marks_[markN_].pts = h.pts;
        marks_[markN_].yours = yours_;
        markN_++;
    }
    gain_ = h.pts;
    std::snprintf(last_, sizeof last_, "%s", h.name);
    if (yours_) you_ += h.pts;
    else them_ += h.pts;
    mode_ = Mode::Call;
    callT_ = 0;
    if (!sys_) return;
    if (h.pts >= 3) {
        sys_->apu.tone(1, 880.f, 0.08f);
        blipLeft_ = 10;
        blipCh_ = 1;
    } else if (h.pts > 0) {
        sys_->apu.tone(1, 440.f, 0.06f);
        blipLeft_ = 8;
        blipCh_ = 1;
    } else {
        sys_->apu.noiseBurst(0.12f, 180.f, 0.06f);
    }
}

void Game::nextTurn() {
    if (yours_ && you_ >= kRace) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        if (sys_) {
            sys_->apu.tone(0, 523.f, 0.08f);
            sys_->apu.tone(1, 659.f, 0.07f);
            blipLeft_ = 18;
            blipCh_ = 0;
        }
        return;
    }
    if (!yours_ && them_ >= kRace && you_ < kRace) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        if (sys_) sys_->apu.tone(0, 110.f, 0.1f);
        return;
    }
    yours_ = !yours_;
    aimX_ = yours_ ? kCx : (kCx + 64.f);
    aimY_ = yours_ ? kCy : kCy;
    drawTick_ = 0;
    steady_ = 0;
    skipHold_ = !bot_;
    mode_ = Mode::Aim;
}

void Game::blip(int ch, float hz, float vol, int frames) {
    if (!sys_) return;
    sys_->apu.tone(ch, hz, vol);
    blipCh_ = ch;
    blipLeft_ = frames;
}

void Game::pump() {
    if (blipLeft_ > 0 && --blipLeft_ == 0 && sys_) sys_->apu.tone(blipCh_, 0, 0);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_) return;
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) c = ' ';
        int tile = art_.font[c - 32];
        h.set(col + i, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    int col = std::max(0, (40 - n) / 2);
    hud(col, row, s, pal);
}

void Game::stamp(const gs::Image& img, float x, float y, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(std::max(1.f, w));
    s.h = int16_t(std::max(1.f, h));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = y < 140 ? gs::rgb4(5, 8, 13) : gs::rgb4(3, 7, 3);
        if (y < 40) sky = gs::rgb4(7, 10, 14);
        if (y > 168) sky = gs::rgb4(2, 6, 2);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        stamp(art_.wordArch, 28.f, 36.f, float(art_.wordArch.w), float(art_.wordArch.h), PAL_WORD);
        stamp(art_.wordSeven, 28.f, 78.f, float(art_.wordSeven.w), float(art_.wordSeven.h), PAL_WORD);
    }
    if (mode_ == Mode::Win) {
        float x = (gs::SCREEN_W - art_.wordWin.w) * 0.5f;
        stamp(art_.wordWin, x, 18.f, float(art_.wordWin.w), float(art_.wordWin.h), PAL_WORD);
    }

    float gx = 0, gy = 0;
    Hit pred{};
    bool aiming = mode_ == Mode::Aim || mode_ == Mode::Nock;
    if (aiming) pred = predict(gx, gy);

    int pose = 0;
    if (mode_ == Mode::Nock) pose = 1;
    else if (mode_ == Mode::Flight) pose = 2;
    stamp(art_.archer[pose], 8.f, 48.f, float(art_.archer[pose].w), float(art_.archer[pose].h), PAL_ARCH);
    stamp(art_.face, kCx - kFaceMid, kCy - kFaceMid, float(kFace), float(kFace), PAL_FACE);

    for (int i = 0; i < markN_; i++) {
        int di = marks_[i].yours ? 0 : 3;
        if (marks_[i].pts == 0) di = 3;
        stamp(art_.dot[di], marks_[i].x - 2.f, marks_[i].y - 2.f, 5.f, 5.f, PAL_MARK);
    }

    if (mode_ == Mode::Flight) {
        float u = std::min(1.f, float(flightT_) / float(kFlight));
        float x = kLooseX + (landX_ - kLooseX) * u;
        float y = kLooseY + (landY_ - kLooseY) * u - std::sin(u * 3.14159265f) * 10.f;
        stamp(art_.arrow, x - float(art_.arrow.w), y - 3.f, float(art_.arrow.w), float(art_.arrow.h), PAL_ARROW);
    }
    if (aiming) {
        stamp(art_.sight, aimX_ - 5.f, aimY_ - 5.f, 11.f, 11.f, PAL_SIGHT);
        stamp(art_.dot[pred.pts > 0 ? 0 : 3], gx - 2.f, gy - 2.f, 5.f, 5.f, PAL_MARK);
        float power = mode_ == Mode::Nock ? std::min(1.f, float(drawTick_) / float(kFullDraw)) : 0.f;
        stamp(art_.px, 12.f, 200.f, 90.f, 6.f, PAL_FILL);
        if (power > 0.02f) stamp(art_.px, 12.f, 200.f, 90.f * power, 6.f, PAL_HUD_GOLD);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 ARCH SEVEN", PAL_HUD_GOLD);
        hudC(24, "ONE ARROW APIECE", PAL_HUD);
        hudC(25, "GOLD IS THREE  RING IS TWO", PAL_HUD_GOLD);
        hudC(26, "FIRST TO SEVEN", PAL_HUD);
        hudC(27, "Z DRAWS  LET GO LOOSES  START", PAL_HUD);
        return;
    }
    std::snprintf(line, sizeof line, "YOU %d", you_);
    hud(1, 0, line, PAL_HUD_GOLD);
    std::snprintf(line, sizeof line, "THEM %d", them_);
    hud(28, 0, line, PAL_HUD);
    if (mode_ == Mode::Win) {
        hudC(25, "FIRST TO SEVEN", PAL_HUD_GOLD);
        std::snprintf(line, sizeof line, "YOU %d  THEM %d", you_, them_);
        hudC(26, line, PAL_HUD);
        hudC(27, "START SHOOTS AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(25, "THEY REACHED SEVEN", PAL_HUD_ALERT);
        std::snprintf(line, sizeof line, "YOU %d  THEM %d", you_, them_);
        hudC(26, line, PAL_HUD);
        hudC(27, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Call) {
        if (gain_ > 0) std::snprintf(line, sizeof line, "%s  +%d", last_, gain_);
        else std::snprintf(line, sizeof line, "MISS");
        hudC(25, line, gain_ >= 3 ? PAL_HUD_GOLD : (gain_ ? PAL_HUD : PAL_HUD_ALERT));
    } else if (yours_) {
        hudC(25, mode_ == Mode::Nock ? "LOOSE ON THE GOLD" : "HOLD Z TO DRAW", PAL_HUD_GOLD);
    } else {
        hudC(25, "THEIR ARROW", PAL_HUD);
    }
    int shownYou = std::min(you_, kRace);
    int shownThem = std::min(them_, kRace);
    char lamps[24];
    int n = 0;
    lamps[n++] = 'Y';
    lamps[n++] = ' ';
    for (int i = 0; i < kRace; i++) lamps[n++] = i < shownYou ? '*' : '.';
    lamps[n] = 0;
    hudC(26, lamps, PAL_HUD_GOLD);
    n = 0;
    lamps[n++] = 'T';
    lamps[n++] = ' ';
    for (int i = 0; i < kRace; i++) lamps[n++] = i < shownThem ? '*' : '.';
    lamps[n] = 0;
    hudC(27, lamps, PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (!rules_) {
        draw();
        return;
    }

    if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
        if (mode_ == Mode::Title) {
            if (sys.hasHome()) sys.eject();
        } else {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }

    if (mode_ == Mode::Title) {
        bool go = bot_ ? (t_ >= 8) : (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A));
        if (go) begin();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) begin();
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            aimX_ = yours_ ? kCx : (kCx + 70.f);
            aimY_ = kCy;
            mode_ = Mode::Nock;
            drawTick_ = 0;
            steady_ = 0;
        } else {
            steer();
            if (skipHold_) {
                if (!drawHeld()) skipHold_ = false;
            } else if (drawHeld()) {
                mode_ = Mode::Nock;
                drawTick_ = 0;
                steady_ = 0;
            }
        }
    } else if (mode_ == Mode::Nock) {
        if (!bot_) steer();
        bool release = bot_ ? (drawTick_ >= kFullDraw) : !drawHeld();
        if (release && (bot_ || drawTick_ > 0)) {
            loose();
        } else if (drawTick_ < kFullDraw) {
            drawTick_++;
        } else {
            steady_++;
        }
    } else if (mode_ == Mode::Flight) {
        if (++flightT_ > kFlight) arrive();
    } else if (mode_ == Mode::Call) {
        if (++callT_ > kCall) nextTurn();
    }

    pump();
    draw();
}

}  // namespace archseven
