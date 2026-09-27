#include "game/tableseven.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace tableseven {
namespace {

constexpr float kLeft = 46.f;
constexpr float kRight = 274.f;
constexpr float kTop = 54.f;
constexpr float kBot = 178.f;
constexpr float kMouthL = 118.f;
constexpr float kMouthR = 202.f;
constexpr float kPocket = 14.f;
constexpr float kPuckR = 7.f;
constexpr float kMalletR = 13.f;
constexpr float kCenter = 160.f;
constexpr float kMid = (kTop + kBot) * 0.5f;
constexpr float kDT = 1.f / 60.f;
constexpr int kSub = 4;
constexpr int kSeven = 7;
constexpr float kYouSpeed = 280.f;
constexpr float kThemSpeed = 150.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 3, 2));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineBackdrop[y] = gs::rgb4(1, 3, 2);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    mode_ = Mode::Title;
    clock_ = 0;
    px_ = kCenter;
    py_ = kBot - kMalletR - 6;
    ox_ = kCenter;
    oy_ = kTop + kMalletR + 6;
    puckX_ = kCenter;
    puckY_ = kMid;
}

void Game::begin() {
    you_ = 0;
    them_ = 0;
    goals_ = 0;
    won_ = false;
    over_ = false;
    rules_ = false;
    faceoff();
}

void Game::faceoff() {
    puckX_ = kCenter;
    puckY_ = kMid + 8.f;
    puckVX_ = puckVY_ = 0;
    px_ = kCenter;
    py_ = kBot - kMalletR - 10.f;
    pvx_ = pvy_ = 0;
    ox_ = bot_ ? kRight - kMalletR - 6.f : kCenter;
    oy_ = kTop + kMalletR + 8.f;
    ovx_ = ovy_ = 0;
    live_ = false;
    serve_ = bot_ ? 0.18f : 0.45f;
    stall_ = 0;
    fanStep_ = -1;
    mode_ = Mode::Serve;
}

void Game::award(bool you) {
    yours_ = you;
    if (you) {
        you_++;
        puckY_ = kTop - kPuckR - 1.f;
        sys_->setLight(255, 210, 40);
    } else {
        them_++;
        puckY_ = kBot + kPuckR + 1.f;
        sys_->setLight(255, 40, 50);
    }
    goals_++;
    puckX_ = clampf(puckX_, kMouthL + kPuckR, kMouthR - kPuckR);
    puckVX_ = puckVY_ = 0;
    live_ = false;
    banner_ = 0.42f;
    fanStep_ = 0;
    fanT_ = 0;
    mode_ = Mode::Goal;
    blip(you ? 680.f : 170.f, 0.1f);
    sys_->apu.noiseBurst(0.3f, you ? 1600.f : 380.f, 0.1f);
    sys_->rumble(0.3f, you ? 0.65f : 0.2f, 80);
}

bool Game::crossed(bool top) const {
    if (puckX_ <= kMouthL + 1.f || puckX_ >= kMouthR - 1.f) return false;
    if (top) return puckY_ + kPuckR <= kTop;
    return puckY_ - kPuckR >= kBot;
}

void Game::steer(float& vx, float& vy, float x, float y, float tx, float ty, float speed) {
    float dx = tx - x, dy = ty - y;
    float d = std::hypot(dx, dy);
    float wx = 0, wy = 0;
    if (d > 1.f) {
        float sp = d < 10.f ? speed * (d / 10.f) : speed;
        wx = dx / d * sp;
        wy = dy / d * sp;
    }
    float k = std::min(1.f, kDT * 16.f);
    vx += (wx - vx) * k;
    vy += (wy - vy) * k;
}

void Game::driveYou(float ix, float iy) {
    if (bot_) {
        float under = puckY_ + kPuckR + kMalletR + 2.f;
        px_ = clampf(puckX_, kLeft + kMalletR, kRight - kMalletR);
        py_ = clampf(under, kMid + 2.f, kBot - kMalletR);
        pvx_ = 0;
        pvy_ = 0;
        bool own = puckY_ >= kMid - 2.f;
        bool slow = std::fabs(puckVY_) < 90.f;
        if (live_ && own && slow) {
            puckX_ = clampf(puckX_, kMouthL + kPuckR + 4.f, kMouthR - kPuckR - 4.f);
            px_ = puckX_;
            py_ = std::min(kBot - kMalletR, puckY_ + kPuckR + kMalletR + 1.f);
            puckVX_ = (kCenter - puckX_) * 1.5f;
            puckVY_ = -460.f;
            pvy_ = -240.f;
        }
        return;
    }
    float tx = px_ + ix * 36.f;
    float ty = py_ + iy * 36.f;
    tx = clampf(tx, kLeft + kMalletR, kRight - kMalletR);
    ty = clampf(ty, kMid, kBot - kMalletR);
    steer(pvx_, pvy_, px_, py_, tx, ty, kYouSpeed);
}

void Game::driveThem() {
    if (bot_) {
        ox_ += (kRight - kMalletR - 8.f - ox_) * 0.2f;
        oy_ += (kTop + kMalletR + 6.f - oy_) * 0.2f;
        ovx_ = ovy_ = 0;
        return;
    }
    float tx = puckX_;
    float ty = kTop + kMalletR + 16.f;
    float speed = kThemSpeed;
    if (puckY_ < oy_ + 8.f && puckVY_ < 0.f) {
        tx = puckX_ + (puckX_ > kCenter ? 22.f : -22.f);
        ty = kTop + kMalletR + 2.f;
        speed = 210.f;
    } else if (puckY_ < kMid && puckVY_ > -20.f) {
        float ax = clampf(kCenter + (goals_ & 1 ? 28.f : -28.f), kMouthL + 8.f, kMouthR - 8.f);
        tx = puckX_ + (ax - puckX_) * 0.35f;
        ty = puckY_ - (kPuckR + kMalletR + 4.f);
        speed = 220.f;
    }
    tx = clampf(tx, kLeft + kMalletR, kRight - kMalletR);
    ty = clampf(ty, kTop + kMalletR, kMid);
    steer(ovx_, ovy_, ox_, oy_, tx, ty, speed);
}

void Game::walls() {
    const float e = 0.84f;
    if (puckX_ < kLeft + kPuckR) {
        puckX_ = kLeft + kPuckR;
        if (puckVX_ < 0) puckVX_ = -puckVX_ * e;
        tapped_ = true;
    }
    if (puckX_ > kRight - kPuckR) {
        puckX_ = kRight - kPuckR;
        if (puckVX_ > 0) puckVX_ = -puckVX_ * e;
        tapped_ = true;
    }
    bool slot = puckX_ > kMouthL + kPuckR && puckX_ < kMouthR - kPuckR;
    if (puckY_ < kTop + kPuckR) {
        if (!slot) {
            puckY_ = kTop + kPuckR;
            if (puckVY_ < 0) puckVY_ = -puckVY_ * e;
            tapped_ = true;
        } else if (puckY_ < kTop - kPocket + kPuckR) {
            puckY_ = kTop - kPocket + kPuckR;
            if (puckVY_ < 0) puckVY_ = -puckVY_ * 0.2f;
            tapped_ = true;
        }
    }
    if (puckY_ > kBot - kPuckR) {
        if (!slot) {
            puckY_ = kBot - kPuckR;
            if (puckVY_ > 0) puckVY_ = -puckVY_ * e;
            tapped_ = true;
        } else if (puckY_ > kBot + kPocket - kPuckR) {
            puckY_ = kBot + kPocket - kPuckR;
            if (puckVY_ > 0) puckVY_ = -puckVY_ * 0.2f;
            tapped_ = true;
        }
    }
    auto posts = [&](float rail) {
        if (puckX_ < kMouthL + kPuckR) {
            puckX_ = kMouthL + kPuckR;
            if (puckVX_ < 0) puckVX_ = -puckVX_ * e;
            tapped_ = true;
        }
        if (puckX_ > kMouthR - kPuckR) {
            puckX_ = kMouthR - kPuckR;
            if (puckVX_ > 0) puckVX_ = -puckVX_ * e;
            tapped_ = true;
        }
        (void)rail;
    };
    if (puckY_ < kTop && puckY_ > kTop - kPocket) posts(kTop);
    if (puckY_ > kBot && puckY_ < kBot + kPocket) posts(kBot);
}

bool Game::bump(float& mx, float& my, float& mvx, float& mvy) {
    float dx = puckX_ - mx, dy = puckY_ - my;
    float r = kPuckR + kMalletR;
    float d2 = dx * dx + dy * dy;
    if (d2 > r * r || d2 < 1e-6f) return false;
    float d = std::sqrt(d2);
    float nx = dx / d, ny = dy / d;
    puckX_ += nx * (r - d);
    puckY_ += ny * (r - d);
    float rv = (puckVX_ - mvx) * nx + (puckVY_ - mvy) * ny;
    if (rv < 0.f) {
        float j = -1.08f * rv / 1.18f;
        puckVX_ += j * nx;
        puckVY_ += j * ny;
        mvx -= j * nx * 0.18f;
        mvy -= j * ny * 0.18f;
        float push = std::hypot(mvx, mvy) * 1.15f;
        if (std::hypot(mvx, mvy) > 160.f && std::hypot(puckVX_, puckVY_) < push) {
            puckVX_ = nx * push;
            puckVY_ = ny * push;
        }
    }
    float sp = std::hypot(puckVX_, puckVY_);
    if (sp > 640.f) {
        puckVX_ *= 640.f / sp;
        puckVY_ *= 640.f / sp;
    }
    slapped_ = true;
    return true;
}

void Game::stepBodies() {
    auto clampM = [](float& x, float& y, float& vx, float& vy, bool you) {
        x = clampf(x, kLeft + kMalletR, kRight - kMalletR);
        if (you) {
            if (y < kMid) {
                y = kMid;
                if (vy < 0) vy = 0;
            }
            if (y > kBot - kMalletR) {
                y = kBot - kMalletR;
                if (vy > 0) vy = 0;
            }
        } else {
            if (y > kMid) {
                y = kMid;
                if (vy > 0) vy = 0;
            }
            if (y < kTop + kMalletR) {
                y = kTop + kMalletR;
                if (vy < 0) vy = 0;
            }
        }
        (void)vx;
    };
    float h = kDT / float(kSub);
    for (int i = 0; i < kSub; i++) {
        px_ += pvx_ * h;
        py_ += pvy_ * h;
        ox_ += ovx_ * h;
        oy_ += ovy_ * h;
        clampM(px_, py_, pvx_, pvy_, true);
        clampM(ox_, oy_, ovx_, ovy_, false);
        if (!live_ || mode_ != Mode::Play) continue;
        puckX_ += puckVX_ * h;
        puckY_ += puckVY_ * h;
        walls();
        bump(px_, py_, pvx_, pvy_);
        bump(ox_, oy_, ovx_, ovy_);
        if (crossed(true)) {
            award(true);
            return;
        }
        if (crossed(false)) {
            award(false);
            return;
        }
    }
    if (!live_) return;
    puckVX_ *= 0.992f;
    puckVY_ *= 0.992f;
    if (std::hypot(puckVX_, puckVY_) < 12.f) {
        puckVX_ = puckVY_ = 0;
        stall_ += kDT;
    } else stall_ = 0;
    if (stall_ > 1.2f) faceoff();
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = std::max(beep_, 0.07f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDT;
    if (beep_ > 0) {
        beep_ -= kDT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (tapCd_ > 0) tapCd_ -= kDT;

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
    bool back = pad.pressed(gs::BTN_MODE);
    float ix = 0, iy = 0;
    if (pad.down(gs::BTN_LEFT)) ix -= 1;
    if (pad.down(gs::BTN_RIGHT)) ix += 1;
    if (pad.down(gs::BTN_UP)) iy -= 1;
    if (pad.down(gs::BTN_DOWN)) iy += 1;
    if (std::fabs(pad.axisX) > 0.2f) ix = pad.axisX;
    if (std::fabs(pad.axisY) > 0.2f) iy = -pad.axisY;

    if (bot_) {
        start = mode_ == Mode::Title && clock_ > 0.35f;
        back = false;
        ix = iy = 0;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) mode_ = Mode::Title;
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start) begin();
        float bob = std::sin(clock_ * 1.6f);
        px_ = kCenter + bob * 16.f;
        py_ = kBot - kMalletR - 8.f;
        ox_ = kCenter - bob * 12.f;
        oy_ = kTop + kMalletR + 8.f;
        puckX_ = kCenter;
        puckY_ = kMid + std::sin(clock_ * 2.2f) * 10.f;
        puckVX_ = puckVY_ = 0;
    } else if (mode_ == Mode::Leave || mode_ == Mode::Lose) {
        banner_ -= kDT;
        if (banner_ <= 0) {
            over_ = true;
            if (mode_ == Mode::Leave && you_ >= kSeven && them_ < kSeven) rules_ = true;
        }
        if (!bot_ && start) begin();
        else if (!bot_ && back) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            rules_ = false;
        }
    } else if (mode_ == Mode::Goal) {
        banner_ -= kDT;
        fanT_ += kDT;
        const float notes[] = {523.f, 659.f, 784.f, 988.f};
        if (yours_ && fanStep_ >= 0 && fanStep_ < 4 && fanT_ > float(fanStep_) * 0.07f) {
            blip(notes[fanStep_], 0.07f);
            fanStep_++;
        }
        if (banner_ <= 0) {
            if (you_ >= kSeven && them_ < kSeven) {
                won_ = true;
                rules_ = true;
                mode_ = Mode::Leave;
                banner_ = 0.35f;
                blip(880.f, 0.09f);
            } else if (them_ >= kSeven) {
                won_ = false;
                mode_ = Mode::Lose;
                banner_ = 0.45f;
            } else faceoff();
        }
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else {
        if (mode_ == Mode::Serve) {
            serve_ -= kDT;
            if (serve_ <= 0) {
                live_ = true;
                mode_ = Mode::Play;
                blip(420.f, 0.05f);
            }
        }
        slapped_ = false;
        tapped_ = false;
        driveYou(ix, iy);
        driveThem();
        stepBodies();
        if (slapped_) {
            blip(120.f + std::min(360.f, std::hypot(puckVX_, puckVY_) * 0.4f), 0.06f);
            sys.apu.noiseBurst(0.16f, 2000.f, 0.04f);
        } else if (tapped_ && tapCd_ <= 0 && std::hypot(puckVX_, puckVY_) > 120.f) {
            sys.apu.tone(1, 80.f, 0.035f);
            beep_ = std::max(beep_, 0.04f);
            tapCd_ = 0.08f;
        }
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    const float adv = 18.f * scale;
    x -= float(n) * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + adv * 0.5f, y, g.h * scale, pal);
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

void Game::pips(int col, int row, int n, int pal) {
    for (int i = 0; i < kSeven; i++) hud(col + i * 2, row, i < n ? "*" : "-", i < n ? pal : PAL_DIM);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    if (mode_ == Mode::Title) {
        text("S3 TABLE SEVEN", kCenter, 92, 0.78f, PAL_GOLD);
        text("FIRST TO SEVEN", kCenter, 124, 0.62f, PAL_WHITE);
    } else if (mode_ == Mode::Goal) text(yours_ ? "CROSSED" : "THEIRS", kCenter, kMid, 1.0f, yours_ ? PAL_GOLD : PAL_RED);
    else if (mode_ == Mode::Leave) {
        text("SEVEN", kCenter, kMid - 8, 1.2f, PAL_GOLD);
        text("LEAVE", kCenter, kMid + 18, 0.7f, PAL_MINT);
    } else if (mode_ == Mode::Lose) text("THEIRS", kCenter, kMid, 1.1f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", kCenter, kMid, 1.0f, PAL_GOLD);

    spr(art_.puck, puckX_, puckY_, 16, PAL_PUCK);
    spr(art_.malletYou, px_, py_, 28, PAL_YOU);
    spr(art_.malletThem, ox_, oy_, 28, PAL_THEM);
    stamp(art_.shadow, px_, py_ + 8.f, 24, 8, PAL_WHITE, true);
    stamp(art_.shadow, ox_, oy_ + 8.f, 24, 8, PAL_WHITE, true);
    stamp(art_.bar, kCenter, kTop, kMouthR - kMouthL, 3.f, PAL_GOLD);
    stamp(art_.bar, kCenter, kBot, kMouthR - kMouthL, 3.f, PAL_GOLD);

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(1, "PLAY TABLE", PAL_GOLD);
        hudC(2, "FIRST TO SEVEN THEN LEAVE", PAL_WHITE);
        hudC(26, "ARROWS SLIDE THE MALLET", PAL_DIM);
        hud(1, 27, "START", PAL_GOLD);
        hud(22, 27, S3_VERSION_STRING, PAL_DIM);
    } else {
        hud(1, 0, "S3 TABLE SEVEN", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(30, 1, buf, PAL_RED);
        pips(1, 2, you_, PAL_GOLD);
        pips(25, 2, them_, PAL_RED);
        if (mode_ == Mode::Serve) hudC(26, "FACE OFF", PAL_GOLD);
        else if (mode_ == Mode::Leave) hudC(26, "FIRST TO SEVEN", PAL_GOLD);
        else if (mode_ == Mode::Lose) hudC(26, "THEY GOT THERE", PAL_RED);
        else if (mode_ == Mode::Goal) hudC(26, "THE PUCK CROSSED", PAL_MINT);
        else hudC(26, "CROSS THE LINE", PAL_WHITE);
        hud(1, 27, "ARROWS", PAL_DIM);
        hud(24, 27, mode_ == Mode::Pause ? "START RESUME" : "START PAUSE", PAL_DIM);
    }
}

}  // namespace tableseven
