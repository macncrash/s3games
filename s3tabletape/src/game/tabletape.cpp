#include "game/tabletape.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace tabletape {
namespace {

constexpr float kLeft = 46.f;
constexpr float kRight = 274.f;
constexpr float kTop = 28.f;
constexpr float kBot = 148.f;
constexpr float kMouthL = 118.f;
constexpr float kMouthR = 202.f;
constexpr float kPocket = 16.f;
constexpr float kPuckR = 7.f;
constexpr float kMalletR = 13.f;
constexpr float kCenter = 160.f;
constexpr float kMid = (kTop + kBot) * 0.5f;
constexpr float kDT = 1.f / 60.f;
constexpr int kSub = 4;
constexpr float kYouSpeed = 260.f;
constexpr float kThemSpeed = 160.f;
constexpr int kTapeN = 3;
constexpr int kTapeScore[kTapeN] = {4, 6, 2};
constexpr const char* kTapeName[kTapeN] = {"L4", "C6", "R2"};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bandCenter(int i) {
    float third = (kMouthR - kMouthL) / 3.f;
    return kMouthL + third * (float(i) + 0.5f);
}

}  // namespace

int Game::drawerScore() const {
    int n = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) n += kTapeScore[i];
    return n;
}

const char* Game::tapeLabel(int i) const { return (i >= 0 && i < kTapeN) ? kTapeName[i] : ""; }

int Game::tapeScore(int i) const { return (i >= 0 && i < kTapeN) ? kTapeScore[i] : 0; }

int Game::nextOpen() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return -1;
}

int Game::bandOf(float x) const {
    float t = (x - kMouthL) / (kMouthR - kMouthL);
    if (t < 1.f / 3.f) return 0;
    if (t < 2.f / 3.f) return 1;
    return 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 3, 2));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineBackdrop[y] = gs::rgb4(1, 3, 2);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    mode_ = Mode::Title;
    clock_ = 0;
    std::snprintf(reason_, sizeof reason_, "the drawer is empty");
    px_ = kCenter;
    py_ = kBot - kMalletR - 4;
    ox_ = kCenter;
    oy_ = kTop + kMalletR + 4;
    puckX_ = kCenter;
    puckY_ = kMid;
}

void Game::begin() {
    you_ = them_ = traps_ = 0;
    won_ = over_ = left_ = rules_ = false;
    held_[0] = held_[1] = held_[2] = false;
    std::snprintf(reason_, sizeof reason_, "the drawer is empty");
    faceoff();
}

void Game::faceoff() {
    puckX_ = kCenter;
    puckY_ = kMid + 6.f;
    puckVX_ = puckVY_ = 0;
    px_ = kCenter;
    py_ = kBot - kMalletR - 8.f;
    pvx_ = pvy_ = 0;
    ox_ = bot_ ? kLeft + kMalletR + 4.f : kCenter;
    oy_ = kTop + kMalletR + 6.f;
    ovx_ = ovy_ = 0;
    live_ = false;
    shot_ = false;
    serve_ = bot_ ? 0.12f : 0.4f;
    stall_ = 0;
    mode_ = Mode::Serve;
}

void Game::award(bool you, int band) {
    yours_ = you;
    lastBand_ = band;
    live_ = false;
    puckVX_ = puckVY_ = 0;
    if (you) {
        you_++;
        puckY_ = kTop - kPuckR - 1.f;
        int need = nextOpen();
        if (band == need) {
            held_[band] = true;
            std::snprintf(reason_, sizeof reason_, "%s dropped in", kTapeName[band]);
            sys_->setLight(40, 220, 90);
        } else {
            traps_++;
            std::snprintf(reason_, sizeof reason_, "that band is not the tape");
            sys_->setLight(220, 160, 30);
        }
    } else {
        them_++;
        puckY_ = kBot + kPuckR + 1.f;
        std::snprintf(reason_, sizeof reason_, "their goal stays out of the till");
        sys_->setLight(255, 40, 50);
    }
    puckX_ = clampf(puckX_, kMouthL + kPuckR, kMouthR - kPuckR);
    banner_ = 0.38f;
    mode_ = Mode::Goal;
    blip(you ? 700.f : 160.f, 0.1f);
    sys_->apu.noiseBurst(0.28f, you ? 1500.f : 360.f, 0.08f);
    sys_->rumble(0.25f, you ? 0.55f : 0.15f, 70);
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
    float k = std::min(1.f, kDT * 14.f);
    vx += (wx - vx) * k;
    vy += (wy - vy) * k;
}

void Game::driveYou(float ix, float iy) {
    if (bot_) {
        int need = nextOpen();
        if (need < 0) need = 1;
        if (live_ && !shot_) {
            puckX_ = bandCenter(need);
            puckY_ = kMid + 4.f;
            puckVX_ = 0;
            puckVY_ = -540.f;
            px_ = puckX_;
            py_ = puckY_ + kPuckR + kMalletR + 2.f;
            shot_ = true;
        } else {
            px_ = clampf(puckX_, kLeft + kMalletR, kRight - kMalletR);
            py_ = clampf(puckY_ + 28.f, kMid + 2.f, kBot - kMalletR);
        }
        pvx_ = pvy_ = 0;
        return;
    }
    float tx = clampf(px_ + ix * 34.f, kLeft + kMalletR, kRight - kMalletR);
    float ty = clampf(py_ + iy * 34.f, kMid, kBot - kMalletR);
    steer(pvx_, pvy_, px_, py_, tx, ty, kYouSpeed);
}

void Game::driveThem() {
    if (bot_) {
        ox_ += (kLeft + kMalletR + 4.f - ox_) * 0.25f;
        oy_ += (kTop + kMalletR + 4.f - oy_) * 0.25f;
        ovx_ = ovy_ = 0;
        return;
    }
    float tx = puckX_;
    float ty = kTop + kMalletR + 14.f;
    float speed = kThemSpeed;
    if (puckY_ < kMid && puckVY_ < -40.f) {
        tx = puckX_ + (puckX_ > kCenter ? 26.f : -26.f);
        ty = kTop + kMalletR + 2.f;
        speed = 200.f;
    }
    tx = clampf(tx, kLeft + kMalletR, kRight - kMalletR);
    ty = clampf(ty, kTop + kMalletR, kMid);
    steer(ovx_, ovy_, ox_, oy_, tx, ty, speed);
}

void Game::walls() {
    const float e = 0.82f;
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
            if (puckVY_ < 0) puckVY_ = -puckVY_ * 0.15f;
        }
    }
    if (puckY_ > kBot - kPuckR) {
        if (!slot) {
            puckY_ = kBot - kPuckR;
            if (puckVY_ > 0) puckVY_ = -puckVY_ * e;
            tapped_ = true;
        } else if (puckY_ > kBot + kPocket - kPuckR) {
            puckY_ = kBot + kPocket - kPuckR;
            if (puckVY_ > 0) puckVY_ = -puckVY_ * 0.15f;
        }
    }
    auto posts = [&]() {
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
    };
    if (puckY_ < kTop && puckY_ > kTop - kPocket) posts();
    if (puckY_ > kBot && puckY_ < kBot + kPocket) posts();
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
        float j = -1.05f * rv / 1.2f;
        puckVX_ += j * nx;
        puckVY_ += j * ny;
        mvx -= j * nx * 0.16f;
        mvy -= j * ny * 0.16f;
    }
    float sp = std::hypot(puckVX_, puckVY_);
    if (sp > 620.f) {
        puckVX_ *= 620.f / sp;
        puckVY_ *= 620.f / sp;
    }
    slapped_ = true;
    return true;
}

void Game::stepBodies() {
    auto clampM = [](float& x, float& y, float& vy, bool you) {
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
    };
    float h = kDT / float(kSub);
    for (int i = 0; i < kSub; i++) {
        px_ += pvx_ * h;
        py_ += pvy_ * h;
        ox_ += ovx_ * h;
        oy_ += ovy_ * h;
        clampM(px_, py_, pvy_, true);
        clampM(ox_, oy_, ovy_, false);
        if (!live_ || mode_ != Mode::Play) continue;
        puckX_ += puckVX_ * h;
        puckY_ += puckVY_ * h;
        walls();
        bump(px_, py_, pvx_, pvy_);
        bump(ox_, oy_, ovx_, ovy_);
        if (crossed(true)) {
            award(true, bandOf(puckX_));
            return;
        }
        if (crossed(false)) {
            award(false, bandOf(puckX_));
            return;
        }
    }
    if (!live_) return;
    puckVX_ *= 0.993f;
    puckVY_ *= 0.993f;
    if (std::hypot(puckVX_, puckVY_) < 14.f) {
        puckVX_ = puckVY_ = 0;
        stall_ += kDT;
    } else stall_ = 0;
    if (stall_ > 1.4f) faceoff();
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
        start = mode_ == Mode::Title && clock_ > 0.3f;
        back = false;
        ix = iy = 0;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = heldMode_;
        else if (back) mode_ = Mode::Title;
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start) begin();
        float bob = std::sin(clock_ * 1.5f);
        px_ = kCenter + bob * 18.f;
        py_ = kBot - kMalletR - 6.f;
        ox_ = kCenter - bob * 14.f;
        oy_ = kTop + kMalletR + 6.f;
        puckX_ = kCenter + std::sin(clock_ * 2.f) * 24.f;
        puckY_ = kMid;
        puckVX_ = puckVY_ = 0;
    } else if (mode_ == Mode::Leave || mode_ == Mode::Lose) {
        banner_ -= kDT;
        if (banner_ <= 0) over_ = true;
        if (!bot_ && start) begin();
        else if (!bot_ && back) {
            mode_ = Mode::Title;
            over_ = won_ = left_ = rules_ = false;
        }
    } else if (mode_ == Mode::Goal) {
        banner_ -= kDT;
        if (banner_ <= 0) {
            if (matched() && drawerScore() == 12 && traps_ == 0 && them_ == 0) {
                won_ = true;
                left_ = true;
                rules_ = true;
                std::snprintf(reason_, sizeof reason_, "the drawer matches the tape");
                mode_ = Mode::Leave;
                banner_ = 0.3f;
                blip(880.f, 0.09f);
            } else if (them_ >= 5) {
                won_ = false;
                std::snprintf(reason_, sizeof reason_, "they closed the table");
                mode_ = Mode::Lose;
                banner_ = 0.4f;
            } else faceoff();
        }
    } else if (start && !bot_) {
        heldMode_ = mode_;
        mode_ = Mode::Pause;
    } else {
        if (mode_ == Mode::Serve) {
            serve_ -= kDT;
            if (serve_ <= 0) {
                live_ = true;
                mode_ = Mode::Play;
                blip(400.f, 0.05f);
            }
        }
        slapped_ = tapped_ = false;
        driveYou(ix, iy);
        driveThem();
        stepBodies();
        if (slapped_) {
            blip(110.f + std::min(340.f, std::hypot(puckVX_, puckVY_) * 0.35f), 0.05f);
            sys.apu.noiseBurst(0.14f, 1800.f, 0.03f);
        } else if (tapped_ && tapCd_ <= 0 && std::hypot(puckVX_, puckVY_) > 100.f) {
            sys.apu.tone(1, 70.f, 0.03f);
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
    const float adv = 16.f * scale;
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    if (mode_ == Mode::Title) {
        text("S3 TABLETAPE", kCenter, 78, 0.7f, PAL_GOLD);
        text("MATCH THE TAPE", kCenter, 100, 0.5f, PAL_WHITE);
    } else if (mode_ == Mode::Goal) {
        text(yours_ ? (lastBand_ >= 0 && held_[lastBand_] ? kTapeName[lastBand_] : "NO") : "THEIRS", kCenter, kMid,
             0.9f, yours_ ? PAL_GOLD : PAL_RED);
    } else if (mode_ == Mode::Leave) {
        text("MATCH", kCenter, kMid - 6, 0.9f, PAL_GOLD);
        text("LEAVE", kCenter, kMid + 16, 0.6f, PAL_MINT);
    } else if (mode_ == Mode::Lose) text("CLOSED", kCenter, kMid, 0.9f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", kCenter, kMid, 0.9f, PAL_GOLD);

    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        stamp(art_.slip, 144.f + float(i) * 52.f, 196.f, 40, 16, PAL_TAPE);
        text(kTapeName[i], 144.f + float(i) * 52.f, 196.f, 0.35f, PAL_WOOD);
    }

    spr(art_.puck, puckX_, puckY_, 16, PAL_PUCK);
    spr(art_.malletYou, px_, py_, 26, PAL_YOU);
    spr(art_.malletThem, ox_, oy_, 26, PAL_THEM);
    stamp(art_.shadow, px_, py_ + 8.f, 22, 7, PAL_WHITE, true);
    stamp(art_.shadow, ox_, oy_ + 8.f, 22, 7, PAL_WHITE, true);
    stamp(art_.bar, kCenter, kTop, kMouthR - kMouthL, 3.f, PAL_GOLD);
    stamp(art_.bar, kCenter, kBot, kMouthR - kMouthL, 3.f, PAL_GOLD);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(1, "PLAY TABLE", PAL_GOLD);
        hudC(2, "DRAWER MUST MATCH THE TAPE", PAL_WHITE);
        hudC(3, "L4 THEN C6 THEN R2", PAL_MINT);
        hudC(25, "CROSS THE NEXT BAND", PAL_DIM);
        hud(1, 27, "START", PAL_GOLD);
        hud(22, 27, S3_VERSION_STRING, PAL_DIM);
    } else {
        hud(1, 0, "S3 TABLETAPE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", kTapeName[0], kTapeName[1], kTapeName[2]);
        hud(1, 1, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "TILL %s %s %s", held_[0] ? kTapeName[0] : "--", held_[1] ? kTapeName[1] : "--",
                      held_[2] ? kTapeName[2] : "--");
        hud(1, 2, buf, PAL_GOLD);
        if (mode_ == Mode::Serve) hudC(22, "FACE OFF", PAL_GOLD);
        else if (mode_ == Mode::Leave) hudC(22, "THE DRAWER MATCHES THE TAPE", PAL_MINT);
        else if (mode_ == Mode::Lose) hudC(22, "THEY CLOSED THE TABLE", PAL_RED);
        else if (mode_ == Mode::Goal) hudC(22, reason_, yours_ ? PAL_GOLD : PAL_RED);
        else {
            int n = nextOpen();
            if (n >= 0) {
                std::snprintf(buf, sizeof buf, "NEXT %s", kTapeName[n]);
                hudC(22, buf, PAL_MINT);
            }
        }
        hud(1, 27, "ARROWS", PAL_DIM);
        hud(24, 27, mode_ == Mode::Pause ? "START RESUME" : "START PAUSE", PAL_DIM);
    }
}

}  // namespace tabletape
