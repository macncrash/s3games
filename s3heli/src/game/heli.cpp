#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace heli {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLift = 300.f;
constexpr float kGrav = 168.f;
constexpr float kVDrag = 0.62f;
constexpr float kHDrag = 0.48f;
constexpr float kCyclic = 155.f;
constexpr float kBody = 20.f;
constexpr float kHoverVx = 32.f;
constexpr float kHoverVy = 20.f;
constexpr float kHoverRate = 3.f;
constexpr float kSettle = 0.48f;
constexpr float kClock = 58.f;
constexpr float kHoverCol = kGrav / kLift;
constexpr float kTitleZoom = 0.25f;
constexpr int kBldgPal[3] = {PAL_PIER, PAL_ROOF, PAL_FIELD};

float windAccel(float t) {
    return 8.2f * std::sin(t * 0.29f) + 2.4f * std::sin(t * 0.83f);
}

uint16_t lerp4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto c = [&](int u, int v) { return int(std::lround(u + (v - u) * t)); };
    return gs::rgb4(c(ar, br), c(ag, bg), c(ab, bb));
}

}  // namespace

void Game::begin() {
    for (int i = 0; i < 3; i++) {
        pads_[i].name = kPads[i].name;
        pads_[i].x = kPads[i].x;
        pads_[i].y = kPads[i].deck;
        pads_[i].half = kPads[i].half();
        pads_[i].done = false;
    }
    got_ = 0;
    lives_ = 3;
    grounded_ = false;
    hovering_ = false;
    on_ = -1;
    face_ = 1;
    why_ = "";
    over_ = false;
    won_ = false;
    t_ = 0;
    left_ = kClock;
    col_ = kHoverCol;
    cyc_ = 0;
    x_ = 48.f;
    y_ = 210.f;
    vx_ = 0;
    vy_ = 0;
    settle_ = 0;
    invuln_ = 0;
    shake_ = 0;
    popT_ = 0;
    pop_[0] = 0;
    blipT_ = 0;
    fanT_ = -1;
    fanStep_ = -1;
    rotorMute_ = 0;
    beepSec_ = -1;
    camX_ = x_;
    camY_ = y_ + 26.f;
    zoom_ = 1.f;
    mode_ = Mode::Play;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.68f);
    sys.apu.setEcho(0.14f, 0.28f, 0.16f);
    begin();
    if (!bot_) {
        mode_ = Mode::Title;
        camX_ = 560.f;
        camY_ = 130.f;
        zoom_ = kTitleZoom;
    }
}

int Game::goal() const {
    for (int i = 0; i < 3; i++)
        if (!pads_[i].done) return i;
    return 0;
}

float Game::cruiseY(int g) const {
    // Stay above every roof the path still crosses, then drop only once lined up.
    float floorY = pads_[g].y + 76.f;
    float a = std::min(x_, pads_[g].x);
    float b = std::max(x_, pads_[g].x);
    for (int i = 0; i < 3; i++) {
        float bl = pads_[i].x - pads_[i].half - 36.f;
        float br = pads_[i].x + pads_[i].half + 36.f;
        if (b > bl && a < br) floorY = std::max(floorY, pads_[i].y + 46.f);
    }
    return std::max(floorY, 96.f);
}

bool Game::slot(int i, float x) const {
    return std::fabs(x - pads_[i].x) <= pads_[i].half - kBody;
}

bool Game::faceHit(int i) const {
    const Pad& p = pads_[i];
    float hl = x_ - kBody, hr = x_ + kBody;
    float bl = p.x - p.half, br = p.x + p.half;
    if (hr <= bl || hl >= br) return false;
    return y_ < p.y - 1.f;
}

void Game::pilot(float& col, float& cyc) {
    int g = goal();
    const Pad& p = pads_[g];
    bool over = std::fabs(x_ - p.x) <= p.half - (kBody + 8.f);

    if (grounded_ && on_ == g) {
        col = 0.28f;
        cyc = std::clamp(-vx_ * 0.05f, -1.f, 1.f);
        return;
    }

    for (int i = 0; i < 3; i++) {
        if (slot(i, x_)) continue;
        float hl = x_ - kBody, hr = x_ + kBody;
        float bl = pads_[i].x - pads_[i].half, br = pads_[i].x + pads_[i].half;
        if (hr > bl && hl < br && y_ < pads_[i].y + 28.f) {
            col = 1.f;
            cyc = std::clamp(-vx_ * 0.03f, -1.f, 1.f);
            return;
        }
    }
    if (y_ < 32.f && !over) {
        col = 1.f;
        cyc = 0.f;
        return;
    }

    float ty = over ? (y_ < p.y + 20.f ? p.y + 0.5f : p.y + 8.f) : cruiseY(g);
    float dx = p.x - x_;
    float wantVx = std::clamp(dx * 2.1f, -110.f, 110.f);
    if (!over && y_ < ty - 18.f) {
        float dir = dx >= 0.f ? 1.f : -1.f;
        wantVx = dir * 58.f;
    }
    float wantVy = std::clamp((ty - y_) * 2.05f, -52.f, 125.f);
    if (over && y_ > p.y + 16.f && wantVy > -34.f) wantVy = -34.f;

    float wind = windAccel(t_);
    float wantAx = std::clamp((wantVx - vx_) * 3.6f, -120.f, 120.f);
    cyc = std::clamp((wantAx - wind + vx_ * kHDrag) / kCyclic, -1.f, 1.f);
    float wantAy = std::clamp((wantVy - vy_) * 3.5f, -170.f, 210.f);
    col = std::clamp((wantAy + kGrav + vy_ * kVDrag) / kLift, 0.f, 1.f);
}

void Game::human(float& col, float& cyc) {
    cyc = 0.f;
    if (sys_->pad.down(gs::BTN_LEFT)) cyc -= 1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) cyc += 1.f;
    cyc += sys_->pad.axisX;
    if (sys_->pad.down(gs::BTN_UP) || sys_->pad.down(gs::BTN_C)) col += 1.7f * kDt;
    if (sys_->pad.down(gs::BTN_DOWN) || sys_->pad.down(gs::BTN_B)) col -= 1.85f * kDt;
    if (sys_->pad.accel > 0.08f) col += sys_->pad.accel * 1.3f * kDt;
    if (sys_->pad.brake > 0.08f) col -= sys_->pad.brake * 1.4f * kDt;
    col = std::clamp(col, 0.f, 1.f);
    cyc = std::clamp(cyc, -1.f, 1.f);
}

void Game::groundOn(int i) {
    grounded_ = true;
    on_ = i;
    y_ = pads_[i].y;
    vy_ = 0.f;
    shake_ = 0.7f;
}

void Game::crash() {
    lives_ -= 1;
    shake_ = 7.f;
    rotorMute_ = 0.4f;
    if (sys_) {
        sys_->apu.noiseBurst(0.4f, 1600.f, 10.f);
        sys_->rumble(1.f, 0.6f, 220);
    }
    if (lives_ <= 0) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        why_ = "airframe";
        grounded_ = false;
        on_ = -1;
        return;
    }
    int g = goal();
    grounded_ = false;
    on_ = -1;
    settle_ = 0.f;
    x_ = pads_[g].x;
    y_ = pads_[g].y + 120.f;
    vx_ = 0.f;
    vy_ = 0.f;
    col_ = kHoverCol;
    invuln_ = 1.3f;
    left_ = std::max(0.f, left_ - 5.f);
}

void Game::win() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "pads";
    fanT_ = 0.f;
    fanStep_ = -1;
    if (sys_) sys_->rumble(0.2f, 0.55f, 220);
}

void Game::clockOut() {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = "clock";
    left_ = 0.f;
    grounded_ = false;
}

void Game::physics(float dt, float col, float cyc) {
    col_ = col;
    cyc_ = std::clamp(cyc, -1.f, 1.f);
    if (invuln_ > 0.f) invuln_ -= dt;

    if (grounded_) {
        int i = on_;
        vx_ += cyc_ * 42.f * dt;
        vx_ *= std::exp(-8.5f * dt);
        x_ += vx_ * dt;
        y_ = pads_[i].y;
        vy_ = 0.f;
        bool over = slot(i, x_);
        bool lift = col_ * kLift > kGrav + 38.f;
        if (!over || lift) {
            grounded_ = false;
            on_ = -1;
            settle_ = 0.f;
            vy_ = over ? 12.f : -10.f;
        }
    } else {
        float wind = windAccel(t_);
        float ax = cyc_ * kCyclic - vx_ * kHDrag + wind;
        float ay = col_ * kLift - kGrav - vy_ * kVDrag;
        vx_ = std::clamp(vx_ + ax * dt, -200.f, 200.f);
        vy_ = std::clamp(vy_ + ay * dt, -230.f, 230.f);
        float py = y_;
        x_ += vx_ * dt;
        y_ += vy_ * dt;
        if (x_ < 28.f) {
            x_ = 28.f;
            vx_ = std::max(0.f, vx_);
        }
        if (x_ > 1180.f) {
            x_ = 1180.f;
            vx_ = std::min(0.f, vx_);
        }
        if (y_ > 430.f) {
            y_ = 430.f;
            vy_ = std::min(0.f, vy_);
        }

        if (invuln_ <= 0.f) {
            int caught = -1;
            for (int i = 0; i < 3; i++) {
                if (!slot(i, x_)) continue;
                bool cross = py >= pads_[i].y - 1.5f && y_ <= pads_[i].y && vy_ < 0.f;
                bool soft = y_ <= pads_[i].y + 7.f && y_ >= pads_[i].y - 2.f && vy_ <= 8.f && vy_ > -76.f &&
                            std::fabs(vx_) < 48.f;
                if (!cross && !soft) continue;
                float impact = std::max(0.f, -vy_);
                if (impact > 150.f) {
                    crash();
                    return;
                }
                if (impact > 86.f) {
                    y_ = pads_[i].y;
                    vy_ = impact * 0.4f;
                    vx_ *= 0.6f;
                    shake_ = 3.5f;
                    caught = -2;
                    break;
                }
                groundOn(i);
                caught = i;
                break;
            }
            if (caught == -1) {
                if (y_ <= 0.f) {
                    crash();
                    return;
                }
                for (int i = 0; i < 3; i++) {
                    if (faceHit(i)) {
                        crash();
                        return;
                    }
                }
            }
        }
    }

    if (vx_ > 14.f) face_ = 1;
    else if (vx_ < -14.f) face_ = -1;

    hovering_ = !grounded_ && std::fabs(vx_) < kHoverVx && std::fabs(vy_) < kHoverVy;
    if (grounded_ && std::fabs(vx_) < 22.f) settle_ += dt;
    else settle_ = 0.f;

    if (grounded_ && settle_ >= kSettle && on_ >= 0 && !pads_[on_].done) {
        pads_[on_].done = true;
        got_ += 1;
        settle_ = 0.f;
        std::snprintf(pop_, sizeof pop_, "%s DOWN", pads_[on_].name);
        popT_ = 1.25f;
        shake_ = 1.1f;
        if (sys_) sys_->rumble(0.15f, 0.35f, 90);
        if (got_ >= 3) win();
        else {
            blipF_ = 620.f + got_ * 90.f;
            blipT_ = 0.14f;
        }
    }
}

void Game::ambience(float dt) {
    if (!sys_) return;
    if (rotorMute_ > 0.f) rotorMute_ -= dt;
    else {
        float f = 46.f + col_ * 24.f + std::fabs(vy_) * 0.03f;
        float vol = (mode_ == Mode::Play || mode_ == Mode::Title ? 0.022f : 0.01f) + col_ * 0.02f;
        sys_->apu.tone(0, f, vol);
        sys_->apu.tone(1, f * 1.97f, vol * 0.45f);
        sys_->apu.noise(0.012f + col_ * 0.02f, 28.f + col_ * 16.f, true);
    }

    if (fanT_ >= 0.f) {
        fanT_ += dt;
        static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f};
        int step = int(fanT_ / 0.11f);
        if (step != fanStep_ && step >= 0 && step < 4) {
            fanStep_ = step;
            sys_->apu.tone(2, notes[step], 0.09f);
        }
        if (fanT_ > 0.72f) {
            fanT_ = -1.f;
            sys_->apu.tone(2, 0.f, 0.f);
        }
        return;
    }
    if (blipT_ > 0.f) {
        blipT_ -= dt;
        sys_->apu.tone(2, blipF_, 0.07f);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
}

void Game::camera(float dt) {
    if (mode_ == Mode::Title) {
        camX_ = 560.f;
        camY_ = 130.f;
        zoom_ = kTitleZoom;
        return;
    }
    float tx = x_ + vx_ * 0.28f;
    float ty = y_ + 26.f;
    if (std::fabs(tx - camX_) > 260.f || std::fabs(ty - camY_) > 200.f) {
        camX_ = tx;
        camY_ = ty;
    } else {
        float k = 1.f - std::exp(-dt * 4.5f);
        camX_ += (tx - camX_) * k;
        camY_ += (ty - camY_) * k;
    }
    zoom_ = 1.f;
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 53.f) * shake_ * 1.6f;
        camY_ += std::cos(t_ * 47.f) * shake_;
    }
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 120.f - (wy - camY_) * zoom_;
}

void Game::local(float px, float py, float ox, float oy, float& wx, float& wy) const {
    float worldW = kHeliH * float(kHeliBmpW) / float(kHeliBmpH);
    float dx = (px - kSkidPxX) / float(kHeliBmpW) * worldW;
    float dy = (kSkidPxY - py) / float(kHeliBmpH) * kHeliH;
    if (face_ < 0) dx = -dx;
    wx = ox + dx;
    wy = oy + dy;
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldW, float worldH, int pal, bool flip, float ax, float ay,
                 int fog) {
    if (worldW < 0.4f || worldH < 0.4f || m.w < 1 || m.h < 1) return;
    float w = worldW * zoom_;
    float h = worldH * zoom_;
    if (w < 0.8f || h < 0.8f) return;
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    float anx = flip ? 1.f - ax : ax;
    float left = sx - anx * w;
    float top = sy - ay * h;
    if (left > gs::SCREEN_W + 12 || top > gs::SCREEN_H + 12 || left + w < -12 || top + h < -12) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(left), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(top), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy - h * 0.5f > gs::SCREEN_H + 8)
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
    s.hflip = flip;
    sys_->vdp.sprite(s);
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

void Game::drawClock() {
    float remain = std::max(0.f, left_);
    int secs = int(std::ceil(remain - 1e-4f));
    if (secs > 599) secs = 599;
    int pal = (hovering_ || secs <= 12) ? PAL_ALERT : PAL_HUD;
    if (hovering_ && (int(t_ * 8.f) & 1)) pal = PAL_BANNER;
    float h = float(art_.digit[0].h);
    float x = 214.f;
    auto dig = [&](int d) {
        spr(art_.digit[d], x + art_.digit[d].w * 0.5f, 16.f, h, pal, false);
        x += float(art_.digit[d].w) + 2.f;
    };
    dig(secs / 60);
    spr(art_.colon, x + art_.colon.w * 0.5f, 16.f, h, pal, false);
    x += float(art_.colon.w) + 2.f;
    dig((secs % 60) / 10);
    dig(secs % 10);
    if (hovering_ && mode_ == Mode::Play) spr(art_.times, 286.f, 36.f, float(art_.times.h), PAL_ALERT, false);
}

void Game::draw() {
    camera(kDt);
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (120.f - float(y)) / std::max(zoom_, 0.05f);
        const uint16_t zenith = gs::rgb4(2, 3, 8);
        const uint16_t mid = gs::rgb4(7, 5, 11);
        const uint16_t horizon = gs::rgb4(14, 8, 5);
        const uint16_t haze = gs::rgb4(10, 7, 6);
        const uint16_t water = gs::rgb4(2, 5, 8);
        const uint16_t deep = gs::rgb4(1, 3, 6);
        uint16_t c;
        if (wy < -8.f) {
            c = lerp4(water, deep, std::clamp((-8.f - wy) / 40.f, 0.f, 1.f));
        } else if (wy < 14.f) {
            float sh = 0.5f + 0.5f * std::sin(wy * 0.7f + t_ * 1.6f);
            c = lerp4(lerp4(deep, water, std::clamp((wy + 8.f) / 22.f, 0.f, 1.f)), gs::rgb4(5, 10, 12), sh * 0.22f);
        } else if (wy < 70.f) {
            c = lerp4(horizon, haze, (wy - 14.f) / 56.f);
        } else {
            c = lerp4(mid, zenith, std::clamp((wy - 70.f) / 260.f, 0.f, 1.f));
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 26, float(art_.title.h), PAL_BANNER, false);
        spr(art_.three, 160, 52, float(art_.three.h), PAL_HUD, false);
    } else if (mode_ == Mode::Win) {
        spr(art_.down, 160, 40, float(art_.down.h), PAL_WIN, false);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& word = why_[0] == 'c' ? art_.clocked : art_.airframe;
        spr(word, 160, 40, float(word.h), PAL_ALERT, false);
    } else {
        drawClock();
    }

    float hx = x_;
    float hy = y_;
    if (mode_ == Mode::Title) hy += std::sin(t_ * 2.2f) * 5.f;
    bool blink = invuln_ > 0.f && (int(t_ * 12.f) & 1);
    if (!blink) {
        float mx, my, tx, ty;
        local(kMastPxX, kMastPxY, hx, hy, mx, my);
        local(kTailPxX, kTailPxY, hx, hy, tx, ty);
        int rf = int(t_ * (16.f + col_ * 30.f)) & 3;
        int tf = int(t_ * 28.f) & 1;
        float rw = 92.f;
        float rh = rw * float(art_.rotor[rf].h) / float(art_.rotor[rf].w);
        place(art_.rotor[rf], mx, my, rw, rh, PAL_HELI, false, 0.5f, 0.5f);
        float tw = 12.f;
        float th = tw * float(art_.tail[tf].h) / float(std::max(1, art_.tail[tf].w));
        place(art_.tail[tf], tx, ty, tw, th, PAL_HELI, face_ < 0, 0.5f, 0.5f);
        float hw = kHeliH * float(kHeliBmpW) / float(kHeliBmpH);
        place(art_.heli, hx, hy, hw, kHeliH, PAL_HELI, face_ < 0, kSkidPxX / float(kHeliBmpW), kSkidPxY / float(kHeliBmpH));
    }

    float wind = windAccel(t_);
    int sock = wind > 3.2f ? 2 : wind < -3.2f ? 0 : 1;
    int nearest = goal();
    for (int i = 2; i >= 0; i--) {
        if (pads_[i].done) continue;
        float bob = std::sin(t_ * 4.f + i) * 3.f;
        int pal = i == nearest ? PAL_WIN : PAL_BANNER;
        place(art_.chevron, pads_[i].x, pads_[i].y + 34.f + bob, 14.f, 12.f, pal, false, 0.5f, 0.5f);
    }

    for (int i = 0; i < 3; i++) {
        const Pad& p = pads_[i];
        bool on = (int(t_ * 5.f + i * 1.7f) & 1) != 0;
        int lampPal = p.done ? PAL_WIN : on ? PAL_BANNER : PAL_DIM;
        place(art_.lamp, p.x - p.half + 10.f, p.y + 6.f, 8.f, 8.f, lampPal, false, 0.5f, 0.5f);
        place(art_.lamp, p.x + p.half - 10.f, p.y + 6.f, 8.f, 8.f, lampPal, false, 0.5f, 0.5f);
        place(art_.sock[sock], p.x - p.half * 0.62f, p.y, 16.f, 18.f, PAL_SOCK, false, 0.22f, 1.f);
        if (std::fabs(hx - p.x) < p.half && hy >= p.y - 2.f && hy - p.y < 64.f) {
            place(art_.shadow, hx, p.y, 30.f, 8.f, PAL_DUST, false, 0.5f, 0.5f);
            if (col_ > 0.35f && hy - p.y > 3.f) place(art_.dust, hx, p.y + 1.f, 26.f, 10.f, PAL_DUST, false, 0.5f, 0.5f);
        }
        place(art_.deck, p.x, p.y, p.half * 2.f, 10.f, p.done ? PAL_DECK_OK : PAL_DECK, false, 0.5f, 0.5f);
    }

    for (int i = 0; i < 3; i++) {
        const Pad& p = pads_[i];
        place(art_.bldg[i], p.x, 0.f, p.half * 2.f, p.y, kBldgPal[i], false, 0.5f, 1.f);
    }

    for (int i = 0; i < 3; i++) {
        float bx = std::fmod(90.f + i * 360.f + t_ * (16.f + i * 6.f), 1240.f) - 20.f;
        float by = 250.f + i * 18.f + std::sin(t_ * 1.4f + i) * 8.f;
        int fr = (int(t_ * 7.f + i) & 1);
        place(art_.bird[fr], bx, by, 22.f, 12.f, PAL_BIRD, false, 0.5f, 0.5f, 2);
    }
    for (int i = 0; i < 4; i++) {
        float cx = 140.f + i * 260.f + std::sin(t_ * 0.17f + i) * 24.f;
        float cy = 300.f + (i % 2) * 28.f;
        place(art_.cloud, cx, cy, 78.f, 36.f, PAL_CLOUD, false, 0.5f, 0.5f, 4);
    }
    place(art_.sun, 150.f, 340.f, 70.f, 70.f, PAL_SUN, false, 0.5f, 0.5f);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(23, "LEFT RIGHT CYCLIC", PAL_HUD);
        hudC(24, "UP C RAISES   X DOWN LOWERS", PAL_HUD);
        hudC(25, "LAND THREE PADS", PAL_BANNER);
        hudC(26, "DO NOT HOVER THE CLOCK OUT", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        return;
    }
    if (mode_ == Mode::Pause) {
        hud(1, 0, "S3 HELI", PAL_BANNER);
        hudC(13, "PAUSED", PAL_BANNER);
        hudC(15, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int s = int(std::ceil(std::max(0.f, left_) - 1e-4f));
        std::snprintf(buf, sizeof buf, "LEFT %d:%02d", s / 60, s % 60);
        hudC(24, buf, PAL_WIN);
        hudC(26, "THREE PADS", PAL_WIN);
        if (!bot_) hudC(27, "START FLIES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(26, why_[0] == 'c' ? "THE CLOCK WON" : "THE AIRFRAME IS LOST", PAL_ALERT);
        if (!bot_) hudC(27, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 HELI", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "%s %s  %s %s  %s %s", pads_[0].name, pads_[0].done ? "OK" : "--", pads_[1].name,
                  pads_[1].done ? "OK" : "--", pads_[2].name, pads_[2].done ? "OK" : "--");
    hud(1, 25, buf, PAL_HUD);

    const char* hint = "THREE PADS  ANY ORDER";
    int hintPal = PAL_HUD;
    if (popT_ > 0.f) {
        hint = pop_;
        hintPal = PAL_WIN;
    } else if (grounded_ && col_ * kLift > kGrav + 20.f) {
        hint = "EASE OFF TO STAY DOWN";
        hintPal = PAL_BANNER;
    } else if (grounded_ && on_ >= 0 && !pads_[on_].done) {
        hint = "HOLD STILL ON THE PAD";
        hintPal = PAL_WIN;
    } else if (hovering_) {
        hint = "HOVER EATS THE CLOCK";
        hintPal = PAL_ALERT;
    }
    if (!(hovering_ && (int(t_ * 6.f) & 1) && popT_ <= 0.f)) hud(1, 26, hint, hintPal);

    int bars = int(std::lround(col_ * 10.f));
    bars = std::clamp(bars, 0, 10);
    char bar[11];
    for (int i = 0; i < 10; i++) bar[i] = i < bars ? '#' : '-';
    bar[10] = 0;
    const char* wmark = wind > 3.2f ? ">" : wind < -3.2f ? "<" : "-";
    std::snprintf(buf, sizeof buf, "LIVES %d  %s  W%s", lives_, bar, wmark);
    hud(1, 27, buf, PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (popT_ > 0.f) popT_ -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 5.f);

    const bool start = sys.pad.pressed(gs::BTN_START);
    if (mode_ == Mode::Title) {
        if (start || bot_) begin();
        else if (sys.pad.pressed(gs::BTN_MODE)) sys.eject();
        ambience(kDt);
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
        ambience(kDt);
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        col_ *= std::exp(-kDt * 1.2f);
        if (start && !bot_) begin();
        ambience(kDt);
        draw();
        return;
    }

    if (start && !bot_) {
        mode_ = Mode::Pause;
        ambience(kDt);
        draw();
        return;
    }

    float col = col_;
    float cyc = 0.f;
    if (bot_) pilot(col, cyc);
    else human(col, cyc);
    physics(kDt, col, cyc);
    if (mode_ == Mode::Play) {
        int secs = int(std::ceil(std::max(0.f, left_) - 1e-4f));
        if (secs <= 10 && secs > 0 && secs != beepSec_) {
            blipF_ = secs <= 5 ? 880.f : 660.f;
            blipT_ = 0.06f;
            beepSec_ = secs;
        }
        float rate = hovering_ ? kHoverRate : 1.f;
        left_ -= kDt * rate;
        if (left_ <= 0.f) clockOut();
    }
    ambience(kDt);
    draw();
}

}  // namespace heli
