#include "game/table.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace table {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int SUB = 4;
constexpr float kAim = 36.f;
constexpr float kPlayerSpeed = 330.f;
constexpr float kCharge = 460.f;
constexpr float kOppSpeed = 176.f;
constexpr float kRestitution = 0.86f;

const float kBias[] = {22.f, -18.f, 26.f, -14.f, 20.f, -24.f, 12.f, -16.f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float unfold(float x, float vx, float t) {
    float left = kLeft + kPuckR;
    float right = kRight - kPuckR;
    float w = right - left;
    float u = (x - left) + vx * t;
    float per = w * 2.f;
    float m = std::fmod(u, per);
    if (m < 0) m += per;
    if (m > w) m = per - m;
    return left + m;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 2, 4));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineBackdrop[y] = gs::rgb4(1, 2, 4);
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    mode_ = Mode::Title;
    clock_ = 0;
    px_ = kCenter;
    py_ = kBot - kMalletR - 8;
    ox_ = kCenter;
    oy_ = kTop + kMalletR + 8;
    puckX_ = kCenter;
    puckY_ = kMid;
}

void Game::newGame() {
    you_ = 0;
    them_ = 0;
    won_ = false;
    over_ = false;
    biasI_ = 0;
    flash_ = 0;
    faceoff();
}

void Game::faceoff() {
    puckX_ = kCenter;
    puckY_ = kMid;
    puckVX_ = puckVY_ = 0;
    px_ = kCenter;
    py_ = kBot - kMalletR - 8;
    pvx_ = pvy_ = 0;
    ox_ = kCenter;
    oy_ = kTop + kMalletR + 8;
    ovx_ = ovy_ = 0;
    track_ = kCenter;
    bias_ = kBias[biasI_ % 8];
    biasI_++;
    live_ = false;
    serve_ = 0.42f;
    stall_ = 0;
    rally_ = 0;
    fanStep_ = -1;
    mode_ = Mode::Serve;
}

void Game::score(bool you) {
    yours_ = you;
    if (you) {
        you_++;
        puckY_ = kTop - kPuckR - 2.f;
        sys_->setLight(255, 210, 40);
    } else {
        them_++;
        puckY_ = kBot + kPuckR + 2.f;
        sys_->setLight(255, 40, 50);
    }
    puckX_ = clampf(puckX_, kMouthL + kPuckR + 2.f, kMouthR - kPuckR - 2.f);
    puckVX_ = puckVY_ = 0;
    live_ = false;
    flash_ = 0.45f;
    banner_ = 0.72f;
    fanStep_ = 0;
    fanT_ = 0;
    mode_ = Mode::Goal;
    blip(you ? 660.f : 180.f, 0.1f);
    sys_->apu.noiseBurst(0.35f, you ? 1800.f : 400.f, 0.12f);
    sys_->rumble(0.35f, you ? 0.7f : 0.25f, 90);
}

bool Game::crossed(bool top) const {
    if (puckX_ <= kMouthL || puckX_ >= kMouthR) return false;
    if (top) return puckY_ + kPuckR < kTop;
    return puckY_ - kPuckR > kBot;
}

float Game::openX() const {
    float dir = (ox_ > kCenter + 6.f) ? -1.f : 1.f;
    float x = kCenter + dir * kAim;
    return clampf(x, kMouthL + kPuckR + 5.f, kMouthR - kPuckR - 5.f);
}

void Game::steer(float& vx, float& vy, float x, float y, float tx, float ty, float speed, bool charge, float nx,
                 float ny) {
    if (charge) {
        vx = nx * speed;
        vy = ny * speed;
        return;
    }
    float dx = tx - x, dy = ty - y;
    float d = std::hypot(dx, dy);
    float wx = 0, wy = 0;
    if (d > 0.8f) {
        float sp = (d < 14.f) ? speed * (d / 14.f) : speed;
        wx = dx / d * sp;
        wy = dy / d * sp;
    }
    float k = std::min(1.f, DT * 14.f);
    vx += (wx - vx) * k;
    vy += (wy - vy) * k;
}

void Game::drivePlayer() {
    float tx = px_, ty = py_;
    float speed = kPlayerSpeed;
    bool charge = false;
    float nx = aimNX_, ny = aimNY_;
    float gap = kPuckR + kMalletR + 7.f;
    if (!live_) {
        float ax = openX();
        float dx = ax - puckX_, dy = (kTop - 4.f) - puckY_;
        float len = std::max(1.f, std::hypot(dx, dy));
        nx = dx / len;
        ny = dy / len;
        tx = puckX_ - nx * gap;
        ty = puckY_ - ny * gap;
    } else if (puckY_ > py_ || (puckVY_ > 80.f && puckY_ > kMid - 24.f)) {
        float tHit = 0.12f;
        if (puckVY_ > 40.f) tHit = clampf((kBot - kPuckR - puckY_) / puckVY_, 0.f, 0.45f);
        tx = clampf(unfold(puckX_, puckVX_, tHit), kMouthL + 2.f, kMouthR - 2.f);
        ty = kBot - kMalletR - 1.f;
        speed = kCharge;
    } else if (puckY_ > kMid - 18.f) {
        float ax = openX();
        float dx = ax - puckX_, dy = (kTop - 2.f) - puckY_;
        float len = std::max(1.f, std::hypot(dx, dy));
        nx = dx / len;
        ny = dy / len;
        tx = puckX_ - nx * gap;
        ty = puckY_ - ny * gap;
        float dist = std::hypot(px_ - puckX_, py_ - puckY_);
        if (dist < gap + 16.f && py_ >= puckY_ - 2.f) charge = true;
        speed = kCharge;
    } else {
        tx = clampf(puckX_, kLeft + kMalletR, kRight - kMalletR);
        ty = kMid + 22.f;
    }
    tx = clampf(tx, kLeft + kMalletR, kRight - kMalletR);
    ty = clampf(ty, kMid, kBot - kMalletR);
    aimNX_ = nx;
    aimNY_ = ny;
    steer(pvx_, pvy_, px_, py_, tx, ty, speed, charge, nx, ny);
}

void Game::driveOpp() {
    track_ += (puckX_ - track_) * 0.045f;
    bool blast = live_ && puckVY_ < -240.f && puckY_ < kMid + 16.f;
    float tx = clampf(track_, kLeft + kMalletR, kRight - kMalletR);
    float ty = kTop + kMalletR + 18.f;
    float speed = kOppSpeed;
    bool charge = false;
    float nx = 0, ny = 1.f;
    if (!live_ || blast) {
        tx = kCenter;
        ty = kTop + kMalletR + 8.f;
        speed = blast ? 24.f : 120.f;
    } else if (puckY_ < oy_) {
        tx = clampf(puckX_, kMouthL, kMouthR);
        ty = kTop + kMalletR + 1.f;
        speed = 210.f;
    } else if (puckY_ < kMid + 8.f) {
        float ax = clampf(kCenter + bias_, kMouthL + kPuckR + 4.f, kMouthR - kPuckR - 4.f);
        float dx = ax - puckX_, dy = (kBot + 2.f) - puckY_;
        float len = std::max(1.f, std::hypot(dx, dy));
        nx = dx / len;
        ny = dy / len;
        float gap = kPuckR + kMalletR + 6.f;
        tx = puckX_ - nx * gap;
        ty = puckY_ - ny * gap;
        float dist = std::hypot(ox_ - puckX_, oy_ - puckY_);
        if (dist < gap + 14.f && oy_ <= puckY_ + 2.f) {
            charge = true;
            speed = 230.f;
        }
    }
    tx = clampf(tx, kLeft + kMalletR, kRight - kMalletR);
    ty = clampf(ty, kTop + kMalletR, kMid);
    steer(ovx_, ovy_, ox_, oy_, tx, ty, speed, charge, nx, ny);
}

void Game::clampMallet(float& x, float& y, float& vx, float& vy, bool you) {
    if (x < kLeft + kMalletR) {
        x = kLeft + kMalletR;
        vx = 0;
    }
    if (x > kRight - kMalletR) {
        x = kRight - kMalletR;
        vx = 0;
    }
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
}

void Game::splitMallets() {
    float dx = ox_ - px_, dy = oy_ - py_;
    float need = kMalletR * 2.f;
    float d2 = dx * dx + dy * dy;
    if (d2 >= need * need) return;
    float d = std::sqrt(std::max(d2, 1e-6f));
    float nx = dx / d, ny = dy / d;
    float push = (need - d) * 0.5f + 0.15f;
    px_ -= nx * push;
    py_ -= ny * push;
    ox_ += nx * push;
    oy_ += ny * push;
}

void Game::walls() {
    const float e = kRestitution;
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
            if (puckVY_ < 0) puckVY_ = -puckVY_ * e;
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
            if (puckVY_ > 0) puckVY_ = -puckVY_ * e;
            tapped_ = true;
        }
    }
    if (puckY_ < kTop && puckY_ > kTop - kPocket) {
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
    }
    if (puckY_ > kBot && puckY_ < kBot + kPocket) {
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
    }
}

bool Game::hitMallet(float& mx, float& my, float& mvx, float& mvy) {
    float dx = puckX_ - mx, dy = puckY_ - my;
    float r = kPuckR + kMalletR;
    float d2 = dx * dx + dy * dy;
    if (d2 > r * r || d2 < 1e-8f) return false;
    float d = std::sqrt(d2);
    float nx = dx / d, ny = dy / d;
    float overlap = r - d;
    puckX_ += nx * overlap;
    puckY_ += ny * overlap;
    float rv = (puckVX_ - mvx) * nx + (puckVY_ - mvy) * ny;
    if (rv < 0.f) {
        float invP = 1.f, invM = 0.16f;
        float j = -(1.05f) * rv / (invP + invM);
        puckVX_ += j * nx * invP;
        puckVY_ += j * ny * invP;
        mvx -= j * nx * invM;
        mvy -= j * ny * invM;
        float leave = std::hypot(mvx, mvy) * 1.25f;
        float sp = std::hypot(puckVX_, puckVY_);
        if (std::hypot(mvx, mvy) > 180.f && sp < leave) {
            puckVX_ = nx * leave;
            puckVY_ = ny * leave;
        }
    }
    float sp = std::hypot(puckVX_, puckVY_);
    if (sp > 620.f) {
        puckVX_ *= 620.f / sp;
        puckVY_ *= 620.f / sp;
    }
    slapped_ = true;
    return true;
}

void Game::physics() {
    float h = DT / float(SUB);
    for (int i = 0; i < SUB; i++) {
        px_ += pvx_ * h;
        py_ += pvy_ * h;
        ox_ += ovx_ * h;
        oy_ += ovy_ * h;
        clampMallet(px_, py_, pvx_, pvy_, true);
        clampMallet(ox_, oy_, ovx_, ovy_, false);
        splitMallets();
        clampMallet(px_, py_, pvx_, pvy_, true);
        clampMallet(ox_, oy_, ovx_, ovy_, false);
        if (!live_ || mode_ != Mode::Play) continue;
        puckX_ += puckVX_ * h;
        puckY_ += puckVY_ * h;
        walls();
        hitMallet(px_, py_, pvx_, pvy_);
        hitMallet(ox_, oy_, ovx_, ovy_);
        if (crossed(true)) {
            score(true);
            return;
        }
        if (crossed(false)) {
            score(false);
            return;
        }
    }
    if (!live_) return;
    puckVX_ *= 0.989f;
    puckVY_ *= 0.989f;
    float sp = std::hypot(puckVX_, puckVY_);
    if (sp < 10.f) puckVX_ = puckVY_ = 0;
    if (sp < 16.f) stall_ += DT;
    else stall_ = 0;
    rally_ += DT;
    if (stall_ > 1.35f || rally_ > 7.5f) faceoff();
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = std::max(beep_, 0.07f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (flash_ > 0) flash_ = std::max(0.f, flash_ - DT);
    if (tapCd_ > 0) tapCd_ -= DT;

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
    bool back = pad.pressed(gs::BTN_MODE);
    float ix = 0, iy = 0;
    if (pad.down(gs::BTN_LEFT)) ix -= 1;
    if (pad.down(gs::BTN_RIGHT)) ix += 1;
    if (pad.down(gs::BTN_UP)) iy -= 1;
    if (pad.down(gs::BTN_DOWN)) iy += 1;
    if (std::fabs(pad.axisX) > 0.18f) ix = pad.axisX;
    float m = std::hypot(ix, iy);
    if (m > 1.f) {
        ix /= m;
        iy /= m;
    }

    if (bot_) {
        start = false;
        back = false;
        ix = iy = 0;
        if (mode_ == Mode::Title && clock_ > 0.45f) start = true;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) mode_ = Mode::Title;
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start) newGame();
        float bob = std::sin(clock_ * 1.7f);
        px_ = kCenter + bob * 14.f;
        py_ = kBot - kMalletR - 6.f;
        ox_ = kCenter - bob * 18.f;
        oy_ = kTop + kMalletR + 6.f;
        puckX_ = kCenter + std::sin(clock_ * 0.9f) * 22.f;
        puckY_ = kMid;
        pvx_ = pvy_ = ovx_ = ovy_ = 0;
    } else if (mode_ == Mode::Over) {
        banner_ -= DT;
        if (banner_ <= 0) over_ = true;
        if (!bot_ && start) newGame();
        else if (!bot_ && back) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else if (mode_ == Mode::Goal) {
        banner_ -= DT;
        fanT_ += DT;
        const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        if (fanStep_ >= 0 && fanStep_ < 4 && fanT_ > float(fanStep_) * 0.08f) {
            blip(notes[fanStep_], 0.08f);
            fanStep_++;
        }
        if (banner_ <= 0) {
            if (you_ >= kSeven || them_ >= kSeven) {
                won_ = you_ >= kSeven && them_ < kSeven;
                mode_ = Mode::Over;
                banner_ = 0.4f;
                if (won_) blip(880.f, 0.09f);
            } else faceoff();
        }
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else {
        if (mode_ == Mode::Serve) {
            serve_ -= DT;
            if (serve_ <= 0) {
                live_ = true;
                mode_ = Mode::Play;
                blip(440.f, 0.05f);
            }
        }
        tapped_ = false;
        slapped_ = false;
        if (bot_) drivePlayer();
        else steer(pvx_, pvy_, px_, py_, px_ + ix * 40.f, py_ + iy * 40.f, kPlayerSpeed, false, 0, 0);
        driveOpp();
        physics();
        if (slapped_) {
            blip(140.f + std::min(400.f, std::hypot(puckVX_, puckVY_) * 0.45f), 0.07f);
            sys.apu.noiseBurst(0.18f, 2200.f, 0.05f);
            sys.rumble(0.15f, 0.4f, 30);
        } else if (tapped_ && tapCd_ <= 0 && std::hypot(puckVX_, puckVY_) > 140.f) {
            sys.apu.tone(1, 90.f, 0.04f);
            beep_ = std::max(beep_, 0.04f);
            tapCd_ = 0.07f;
        }
    }

    if (mode_ == Mode::Goal || (mode_ == Mode::Over && won_)) {
        float pulse = 0.5f + 0.5f * std::sin(clock_ * 18.f);
        sys.vdp.setColor(PAL_ICE * 16 + 4, gs::rgb4(11 + int(pulse * 3), 14, 15));
    } else sys.vdp.setColor(PAL_ICE * 16 + 4, gs::rgb4(10, 14, 15));

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::pips(int col, int row, int n, int on, int off) {
    for (int i = 0; i < kSeven; i++) hud(col + i * 2, row, i < n ? "*" : "-", i < n ? on : off);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    bool hangTop = puckY_ < kTop && puckY_ + kPuckR >= kTop && puckX_ > kMouthL && puckX_ < kMouthR;
    bool hangBot = puckY_ > kBot && puckY_ - kPuckR <= kBot && puckX_ > kMouthL && puckX_ < kMouthR;

    if (mode_ == Mode::Title) {
        text("S3 TABLE", kCenter, 96, 1.15f, PAL_GOLD);
        text("FIRST TO SEVEN", kCenter, 132, 0.72f, PAL_WHITE);
    } else if (mode_ == Mode::Goal) {
        text(yours_ ? "CROSSED" : "THEIRS", kCenter, kMid, 1.05f, yours_ ? PAL_GOLD : PAL_RED);
    } else if (mode_ == Mode::Over && won_) {
        text("SEVEN", kCenter, kMid - 8, 1.35f, PAL_GOLD);
        text("CROSSED", kCenter, kMid + 18, 0.7f, PAL_MINT);
    } else if (mode_ == Mode::Over) text("THEIRS", kCenter, kMid, 1.2f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", kCenter, kMid, 1.1f, PAL_GOLD);
    else if (hangTop || hangBot) text("NOT ACROSS", kCenter, kMid, 0.7f, PAL_RED);

    spr(art_.puck, puckX_, puckY_, 14, PAL_PUCK);
    spr(art_.malletYou, px_, py_, 28, PAL_YOU);
    spr(art_.malletThem, ox_, oy_, 28, PAL_THEM);
    stamp(art_.shadow, px_, py_ + 8.f, 26, 8, PAL_WHITE, true);
    stamp(art_.shadow, ox_, oy_ + 8.f, 26, 8, PAL_WHITE, true);
    stamp(art_.shadow, puckX_, puckY_ + 5.f, 14, 5, PAL_WHITE, true);
    int linePal = (hangTop || hangBot) ? PAL_WHITE : PAL_GOLD;
    float mouthW = kMouthR - kMouthL;
    stamp(art_.bar, kCenter, kTop, mouthW, hangTop ? 5.f : 3.f, linePal);
    stamp(art_.bar, kCenter, kBot, mouthW, hangBot ? 5.f : 3.f, linePal);

    char buf[32];
    if (mode_ == Mode::Title) {
        hudC(1, "FIRST TO SEVEN", PAL_GOLD);
        hudC(2, "THE PUCK HAS TO CROSS", PAL_WHITE);
        hudC(26, "ARROWS SLIDE    HIT THE CORNER HARD", PAL_DIM);
        hud(1, 27, "START", PAL_GOLD);
        hud(22, 27, S3_VERSION_STRING, PAL_DIM);
    } else {
        hud(1, 0, "S3 TABLE", PAL_GOLD);
        hud(28, 0, "FIRST TO 7", PAL_DIM);
        std::snprintf(buf, sizeof buf, "YOU %d", you_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "THEM %d", them_);
        hud(31, 1, buf, PAL_RED);
        pips(1, 2, you_, PAL_GOLD, PAL_DIM);
        pips(25, 2, them_, PAL_RED, PAL_DIM);
        if (mode_ == Mode::Serve) hudC(26, "FACE OFF", PAL_GOLD);
        else if (mode_ == Mode::Goal) hudC(26, "THE PUCK CROSSED", PAL_MINT);
        else if (mode_ == Mode::Over && won_) hudC(26, "FIRST TO SEVEN", PAL_GOLD);
        else if (mode_ == Mode::Over) hudC(26, "THEY GOT TO SEVEN", PAL_RED);
        else if (hangTop || hangBot) hudC(26, "NOT ACROSS YET", PAL_RED);
        else hudC(26, "THE PUCK HAS TO CROSS", PAL_WHITE);
        hud(1, 27, "ARROWS", PAL_DIM);
        hud(26, 27, mode_ == Mode::Pause ? "START RESUME" : "START PAUSE", PAL_DIM);
    }
}

}  // namespace table
