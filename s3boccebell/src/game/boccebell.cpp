#include "game/boccebell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace boccebell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kDrag = 78.f;
constexpr float kStop = 1.4f;
constexpr float kL = 52.f;
constexpr float kR = 268.f;
constexpr float kBack = 48.f;
constexpr float kFoul = 158.f;
constexpr float kFoot = 206.f;
constexpr float kBowlR = 7.f;
constexpr float kBellX = 160.f;
constexpr float kBellY = 96.f;
constexpr float kBellR = 14.f;
constexpr float kThrowY = 196.f;

float span(float x, float y) { return std::sqrt(x * x + y * y); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.36f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    n_ = 0;
    hold_ = 0;
    why_ = "";
    aim_ = 0.f;
    power_ = 0.45f;
    powerDir_ = 1.f;
    bellAmp_ = 0.12f;
    for (Bowl& b : bowl_) b = Bowl{};
}

void Game::begin() {
    dead_ = 0;
    tryNo_ = 0;
    n_ = 0;
    hold_ = 0;
    won_ = false;
    rung_ = false;
    over_ = false;
    why_ = "";
    aim_ = 0.f;
    power_ = 0.55f;
    for (Bowl& b : bowl_) b = Bowl{};
    mode_ = Mode::Aim;
}

void Game::launch() {
    if (n_ >= 3 || mode_ != Mode::Aim) return;
    tryNo_ = n_ + 1;
    Bowl& b = bowl_[n_++];
    b.live = true;
    b.moving = true;
    b.burned = false;
    b.y = kThrowY;
    b.x = 160.f;

    float tx = kBellX;
    float ty = kBellY;
    if (bot_) {
        tx = kBellX;
        ty = kBellY;
        b.x = 160.f;
    } else {
        float reach = 48.f + power_ * 150.f;
        float ang = std::clamp(aim_, -0.85f, 0.85f);
        tx = b.x + std::sin(ang) * reach;
        ty = b.y - std::cos(ang) * reach;
    }
    float dx = tx - b.x;
    float dy = ty - b.y;
    float d = span(dx, dy);
    if (d < 1.f) {
        b.x = tx;
        b.y = ty;
        b.vx = b.vy = 0.f;
        b.moving = false;
    } else {
        float v = std::sqrt(2.f * kDrag * d);
        b.vx = dx / d * v;
        b.vy = dy / d * v;
    }
    if (sys_) sys_->apu.noiseBurst(0.18f, 520.f, 0.06f);
    mode_ = Mode::Roll;
    hold_ = 0;
}

void Game::coast(float dt) {
    for (int i = 0; i < n_; i++) {
        Bowl& b = bowl_[i];
        if (!b.moving) continue;
        float sp = span(b.vx, b.vy);
        if (sp < kStop) {
            b.vx = b.vy = 0.f;
            b.moving = false;
            continue;
        }
        float stopIn = sp / kDrag;
        float use = dt;
        bool halt = false;
        if (use >= stopIn) {
            use = stopIn;
            halt = true;
        }
        float dist = sp * use - 0.5f * kDrag * use * use;
        b.x += b.vx / sp * dist;
        b.y += b.vy / sp * dist;
        if (halt) {
            b.vx = b.vy = 0.f;
            b.moving = false;
        } else {
            float ns = sp - kDrag * use;
            b.vx = b.vx / sp * ns;
            b.vy = b.vy / sp * ns;
        }
        if (b.x < kL + kBowlR) {
            b.x = kL + kBowlR;
            b.vx = std::fabs(b.vx) * 0.45f;
            b.burned = true;
        }
        if (b.x > kR - kBowlR) {
            b.x = kR - kBowlR;
            b.vx = -std::fabs(b.vx) * 0.45f;
            b.burned = true;
        }
        if (b.y < kBack + kBowlR) {
            b.y = kBack + kBowlR;
            b.vy = std::fabs(b.vy) * 0.35f;
            b.burned = true;
        }
        if (b.y > kFoot - kBowlR) {
            b.y = kFoot - kBowlR;
            b.vy = -std::fabs(b.vy) * 0.2f;
        }
    }
}

void Game::separate() {
    for (int i = 0; i < n_; i++) {
        if (!bowl_[i].live) continue;
        for (int j = i + 1; j < n_; j++) {
            if (!bowl_[j].live) continue;
            float dx = bowl_[j].x - bowl_[i].x;
            float dy = bowl_[j].y - bowl_[i].y;
            float d = span(dx, dy);
            float need = kBowlR * 2.f;
            if (d >= need || d < 0.05f) continue;
            float nx = dx / d;
            float ny = dy / d;
            float push = (need - d) * 0.5f;
            bowl_[i].x -= nx * push;
            bowl_[i].y -= ny * push;
            bowl_[j].x += nx * push;
            bowl_[j].y += ny * push;
            if (!bowl_[i].moving && !bowl_[j].moving) continue;
            float rel = (bowl_[j].vx - bowl_[i].vx) * nx + (bowl_[j].vy - bowl_[i].vy) * ny;
            if (rel < 0.f) {
                bowl_[i].vx += nx * rel * 0.8f;
                bowl_[i].vy += ny * rel * 0.8f;
                bowl_[j].vx -= nx * rel * 0.8f;
                bowl_[j].vy -= ny * rel * 0.8f;
                bowl_[i].moving = true;
                bowl_[j].moving = true;
            }
        }
    }
}

void Game::dieTry(const char* why) {
    why_ = why;
    dead_++;
    mode_ = Mode::Dead;
    hold_ = 0;
    bellAmp_ = 0.05f;
    if (sys_) sys_->apu.tone(0, 146.f, 0.08f);
}

void Game::ring() {
    rung_ = true;
    won_ = true;
    why_ = "RUNG";
    mode_ = Mode::Ring;
    hold_ = 0;
    bellAmp_ = 1.f;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.14f);
        if (!sys_->headless) sys_->rumble(0.25f, 0.45f, 140);
    }
}

void Game::settle() {
    if (n_ <= 0) return;
    Bowl& b = bowl_[n_ - 1];
    if (b.burned && b.y <= kBack + kBowlR + 1.f) {
        dieTry("LONG");
        return;
    }
    if (b.burned) {
        dieTry("WIDE");
        return;
    }
    if (b.y > kFoul) {
        dieTry("SHORT");
        return;
    }
    float d = span(b.x - kBellX, b.y - kBellY);
    if (d <= kBellR) ring();
    else dieTry("OUT");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 9.f : 2.2f);
    bellAmp_ *= rung_ ? 0.992f : 0.98f;
    if (bellAmp_ < 0.08f) bellAmp_ = rung_ ? 0.35f : 0.08f;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            launch();
        } else {
            if (pad.down(gs::BTN_LEFT)) aim_ -= 1.2f * kDt;
            if (pad.down(gs::BTN_RIGHT)) aim_ += 1.2f * kDt;
            aim_ = std::clamp(aim_, -0.85f, 0.85f);
            power_ += powerDir_ * kDt * 0.72f;
            if (power_ >= 1.f) {
                power_ = 1.f;
                powerDir_ = -1.f;
            }
            if (power_ <= 0.08f) {
                power_ = 0.08f;
                powerDir_ = 1.f;
            }
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) launch();
        }
    } else if (mode_ == Mode::Roll) {
        coast(kDt);
        separate();
        bool moving = false;
        for (int i = 0; i < n_; i++)
            if (bowl_[i].moving) moving = true;
        if (!moving) {
            hold_++;
            if (hold_ > (bot_ ? 4 : 18)) settle();
        } else {
            hold_ = 0;
        }
    } else if (mode_ == Mode::Dead) {
        hold_++;
        int wait = bot_ ? 18 : 50;
        if (hold_ > wait) {
            if (dead_ >= 3) {
                won_ = false;
                mode_ = Mode::Over;
                hold_ = 0;
            } else {
                mode_ = Mode::Aim;
                hold_ = 0;
            }
        }
    } else if (mode_ == Mode::Ring) {
        hold_++;
        if (hold_ == 12) sys.apu.tone(0, 659.f, 0.1f);
        if (hold_ == 24) sys.apu.tone(0, 784.f, 0.12f);
        if (hold_ > 36) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > (bot_ ? 8 : 40)) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Over) {
        if (!won_) {
            if (bot_) {
                hold_++;
                if (hold_ > 30) toTitle();
            } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
                toTitle();
            }
        }
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int sky = y < 28 ? 3 : (y < 48 ? 2 : 1);
        v.lineBackdrop[y] = gs::rgb4(sky, sky + 2, sky + 4);
    }

    float swing = std::sin(bellPh_) * bellAmp_ * 6.f;
    spr(art_.bell, kBellX + swing, kBellY - 22.f, 22.f, 20.f, PAL_BELL);
    for (int i = n_ - 1; i >= 0; i--) {
        if (!bowl_[i].live) continue;
        spr(art_.bowl, bowl_[i].x + 2.f, bowl_[i].y + 3.f, 16.f, 8.f, PAL_BOWL, true);
        spr(art_.bowl, bowl_[i].x, bowl_[i].y, 16.f, 16.f, PAL_BOWL);
    }
    spr(art_.pallino, kBellX, kBellY, 8.f, 8.f, PAL_PALLINO);
    spr(art_.court, 160.f, 112.f, float(art_.court.w), float(art_.court.h), PAL_COURT);

    if (mode_ == Mode::Title || mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_))
        spr(art_.banner, 160.f, 16.f, float(art_.banner.w), float(art_.banner.h),
            (mode_ == Mode::Title) ? PAL_TITLE : PAL_WIN);

    if (mode_ == Mode::Title) {
        hudC(18, "THE BELL RINGS", PAL_INK);
        hudC(19, "BEFORE THE THIRD TRY DIES", PAL_HINT);
        hudC(22, "START", PAL_TITLE);
    } else if (mode_ == Mode::Aim) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "TRY %d   DEAD %d", tryNo_ + 1, dead_);
        hudC(1, buf, PAL_INK);
        hudC(25, "LEFT RIGHT AIM", PAL_HINT);
        int bars = int(power_ * 10.f + 0.5f);
        char meter[20];
        std::snprintf(meter, sizeof(meter), "POWER %.*s%.*s", bars, "##########", 10 - bars, "..........");
        hudC(26, meter, PAL_TITLE);
    } else if (mode_ == Mode::Roll) {
        hudC(1, "ROLLING", PAL_INK);
    } else if (mode_ == Mode::Dead) {
        hudC(1, why_, PAL_DEAD);
        hudC(25, dead_ >= 3 ? "THE THIRD TRY DIED" : "THAT TRY DIED", PAL_DEAD);
    } else if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        hudC(24, "THE BELL RINGS", PAL_WIN);
        hudC(25, "BEFORE THE THIRD TRY DIES", PAL_HINT);
    } else if (mode_ == Mode::Over) {
        hudC(24, "END", PAL_DEAD);
        hudC(25, "START", PAL_HINT);
    }
}

}  // namespace boccebell
