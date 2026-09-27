#include "game/tablemark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tablemark {
namespace {
constexpr float kDt = 1.f / 60.f;

float len(float x, float y) { return std::sqrt(x * x + y * y); }

void clampMallet(float& x, float& y, bool you) {
    float pad = kMalletR;
    x = std::clamp(x, kLeft + pad, kRight - pad);
    if (you) y = std::clamp(y, 118.f, kBot - pad);
    else y = std::clamp(y, kTop + pad, 104.f);
    float dx = x - kMarkX;
    float dy = y - kMarkY;
    float keep = kMarkR + kMalletR + 2.f;
    float d = len(dx, dy);
    if (d < keep && d > 0.01f) {
        x = kMarkX + dx / d * keep;
        y = kMarkY + dy / d * keep;
        if (you) y = std::max(y, 118.f);
        else y = std::min(y, 104.f);
    }
}

void seek(float& x, float& y, float& vx, float& vy, float tx, float ty, float speed, float dt) {
    float dx = tx - x;
    float dy = ty - y;
    float d = len(dx, dy);
    if (d < 0.6f) {
        x = tx;
        y = ty;
        vx = vy = 0.f;
        return;
    }
    vx = dx / d * speed;
    vy = dy / d * speed;
    float step = speed * dt;
    if (step >= d) {
        x = tx;
        y = ty;
        vx = vy = 0.f;
    } else {
        x += vx * dt;
        y += vy * dt;
    }
}
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.4f);
    begin();
}

void Game::begin() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    finished_ = false;
    slap_ = false;
    shots_ = 0;
    hold_ = 0;
    px_ = 160.f;
    py_ = 132.f;
    pvx_ = pvy_ = 0.f;
    yx_ = 160.f;
    yy_ = 164.f;
    yvx_ = yvy_ = 0.f;
    ox_ = 230.f;
    oy_ = 86.f;
    ovx_ = ovy_ = 0.f;
    t_ = 0.f;
}

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    mode_ = Mode::Win;
    hold_ = 0;
    pvx_ = pvy_ = 0.f;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.12f);
        if (!sys_->headless) sys_->rumble(0.3f, 0.5f, 120);
    }
}

void Game::aimShot() {
    float dx = kMarkX - px_;
    float dy = kMarkY - py_;
    float dist = len(dx, dy);
    if (dist < 1.f) return;
    float travel = std::max(0.f, dist - 2.5f);
    float speed = std::sqrt(2.f * kDecel * travel);
    pvx_ = dx / dist * speed;
    pvy_ = dy / dist * speed;
    shots_++;
    if (sys_) sys_->apu.noiseBurst(0.16f, 700.f, 0.05f);
}

void Game::hit(float& mx, float& my, float& mvx, float& mvy, bool you) {
    float dx = px_ - mx;
    float dy = py_ - my;
    float d = len(dx, dy);
    float rr = kPuckR + kMalletR;
    if (d >= rr || d < 0.01f) return;
    float nx = dx / d;
    float ny = dy / d;
    px_ = mx + nx * (rr + 0.2f);
    py_ = my + ny * (rr + 0.2f);
    float rel = (pvx_ - mvx) * nx + (pvy_ - mvy) * ny;
    if (rel >= 0.f && !(you && slap_)) return;
    if (you && slap_) {
        slap_ = false;
        aimShot();
        return;
    }
    float push = -rel * 1.25f + 40.f;
    pvx_ += nx * push;
    pvy_ += ny * push;
    if (you) shots_++;
    if (sys_) sys_->apu.tone(1, you ? 220.f : 160.f, 0.06f);
}

void Game::drive(float dt) {
    const gs::Pad& pad = sys_->pad;
    slap_ = false;
    if (bot_) {
        float sp = len(pvx_, pvy_);
        float tx = px_;
        float ty = py_ + 8.f;
        if (sp > 12.f) {
            tx = 160.f;
            ty = 172.f;
        }
        bool lined = std::fabs(yx_ - px_) < 4.f && yy_ > py_;
        seek(yx_, yy_, yvx_, yvy_, tx, ty, lined && sp < 10.f ? 240.f : 170.f, dt);
        if (lined && sp < 10.f && yy_ - py_ < kMalletR + kPuckR + 4.f) slap_ = true;
        ox_ = 232.f;
        oy_ = 88.f;
        ovx_ = ovy_ = 0.f;
    } else {
        float ax = 0.f, ay = 0.f;
        if (pad.down(gs::BTN_LEFT)) ax -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) ax += 1.f;
        if (pad.down(gs::BTN_UP)) ay -= 1.f;
        if (pad.down(gs::BTN_DOWN)) ay += 1.f;
        if (std::fabs(pad.axisX) > 0.2f) ax = pad.axisX;
        if (std::fabs(pad.axisY) > 0.2f) ay = -pad.axisY;
        float mag = len(ax, ay);
        float speed = pad.down(gs::BTN_A) ? 220.f : 145.f;
        if (mag > 0.05f) {
            yvx_ = ax / mag * speed;
            yvy_ = ay / mag * speed;
            yx_ += yvx_ * dt;
            yy_ += yvy_ * dt;
            if (pad.down(gs::BTN_A)) slap_ = true;
        } else {
            yvx_ = yvy_ = 0.f;
        }
        float tx = px_;
        float ty = 78.f;
        if (py_ < 110.f) ty = std::min(96.f, py_ - 16.f);
        float guard = std::fabs(px_ - kMarkX) < 28.f && py_ < 110.f ? 196.f : tx;
        seek(ox_, oy_, ovx_, ovy_, guard, ty, 90.f, dt);
    }
    clampMallet(yx_, yy_, true);
    clampMallet(ox_, oy_, false);
}

void Game::physics(float dt) {
    float sp = len(pvx_, pvy_);
    if (sp > 0.f) {
        float ns = sp - kDecel * dt;
        if (ns < 6.f) ns = 0.f;
        float s = ns / sp;
        pvx_ *= s;
        pvy_ *= s;
    }
    px_ += pvx_ * dt;
    py_ += pvy_ * dt;
    if (px_ < kLeft + kPuckR) {
        px_ = kLeft + kPuckR;
        pvx_ = std::fabs(pvx_) * 0.72f;
    }
    if (px_ > kRight - kPuckR) {
        px_ = kRight - kPuckR;
        pvx_ = -std::fabs(pvx_) * 0.72f;
    }
    if (py_ < kTop + kPuckR) {
        py_ = kTop + kPuckR;
        pvy_ = std::fabs(pvy_) * 0.72f;
    }
    if (py_ > kBot - kPuckR) {
        py_ = kBot - kPuckR;
        pvy_ = -std::fabs(pvy_) * 0.72f;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        drive(kDt);
        physics(kDt);
        hit(ox_, oy_, ovx_, ovy_, false);
        hit(yx_, yy_, yvx_, yvy_, true);
        float dx = px_ - kMarkX;
        float dy = py_ - kMarkY;
        if (shots_ > 0 && len(pvx_, pvy_) < 8.f && dx * dx + dy * dy <= kMarkR * kMarkR) finishMark();
    } else if (mode_ == Mode::Win) {
        hold_++;
        if (hold_ == 18) sys.apu.tone(0, 659.f, 0.1f);
        if (hold_ == 36) sys.apu.tone(0, 784.f, 0.12f);
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        v.lineBackdrop[y] = y < 28 || y > 196 ? gs::rgb4(1, 1, 2) : gs::rgb4(3, 2, 2);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    spr(art_.table, 160.f, 118.f, float(art_.table.w), float(art_.table.h), PAL_TABLE);
    spr(art_.mallet, ox_, oy_, float(art_.mallet.w), float(art_.mallet.h), PAL_THEM);
    spr(art_.mallet, yx_, yy_, float(art_.mallet.w), float(art_.mallet.h), PAL_YOU);
    spr(art_.puck, px_, py_, float(art_.puck.w), float(art_.puck.h), PAL_PUCK);
    if (mode_ == Mode::Title) spr(art_.title, 160.f, 40.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Win) spr(art_.win, 160.f, 40.f, float(art_.win.w), float(art_.win.h), PAL_WIN);

    hud(1, 1, "S3 TABLEMARK", PAL_INK);
    char buf[40];
    std::snprintf(buf, sizeof buf, "SHOTS %d", shots_);
    hud(30, 1, buf, PAL_GOLD);
    if (mode_ == Mode::Title) hudC(26, "START  FINISH THE MARK", PAL_HINT);
    else if (mode_ == Mode::Play) hudC(26, "STOP THE PUCK ON THE MARK", PAL_HINT);
    else hudC(26, "THE MARK IS FINISHED", PAL_WIN);
}

}  // namespace tablemark
