#include "game/rail.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace railbuoy {
namespace {

constexpr float W = 340.f;
constexpr float R = 160.f;
constexpr float PI = 3.14159265f;
constexpr float ARC = PI * R;
constexpr float DOCK = 100.f;
constexpr float LIMIT = 78.f;

float wrap(float s, float L) {
    while (s < 0) s += L;
    while (s >= L) s -= L;
    return s;
}

float fwd(float from, float to, float L) {
    float d = to - from;
    while (d < 0) d += L;
    while (d >= L) d -= L;
    return d;
}

}  // namespace

float Game::length() const { return 4.f * W + 2.f * ARC; }

void Game::poseAt(float s, float& x, float& y, float& hx, float& hy) const {
    const float L = length();
    s = wrap(s, L);
    if (s < W) {
        x = s;
        y = 0;
        hx = 1;
        hy = 0;
        return;
    }
    s -= W;
    if (s < ARC) {
        float ang = -PI * 0.5f + s / R;
        x = W + R * std::cos(ang);
        y = R + R * std::sin(ang);
        hx = -std::sin(ang);
        hy = std::cos(ang);
        return;
    }
    s -= ARC;
    if (s < 2.f * W) {
        x = W - s;
        y = 2.f * R;
        hx = -1;
        hy = 0;
        return;
    }
    s -= 2.f * W;
    if (s < ARC) {
        float ang = PI * 0.5f + s / R;
        x = -W + R * std::cos(ang);
        y = R + R * std::sin(ang);
        hx = -std::sin(ang);
        hy = std::cos(ang);
        return;
    }
    s -= ARC;
    x = -W + s;
    y = 0;
    hx = 1;
    hy = 0;
}

bool Game::inDock(float s) const {
    const float L = length();
    s = wrap(s, L);
    return s <= DOCK || s >= L - DOCK;
}

bool Game::inFar(float s) const {
    const float mid = W + ARC + W;
    s = wrap(s, length());
    float d = s - mid;
    if (d > length() * 0.5f) d -= length();
    if (d < -length() * 0.5f) d += length();
    return std::fabs(d) <= 110.f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (wp_ >= 3) return 3;
    if (wp_ >= 1) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    why_[0] = 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.road[y].on = false;
        int band = y < 28 ? 0 : (y * 6) / gs::SCREEN_H;
        sys.vdp.lineBackdrop[y] = gs::rgb4(1, 4 + band / 3, 7 + band / 2);
        sys.vdp.lineFog[y] = 0;
    }
}

void Game::begin() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    left_ = false;
    wp_ = 0;
    s_ = 0;
    speed_ = 0;
    stopT_ = 0;
    farT_ = 0;
    raceTime_ = 0;
    chimeStep_ = -1;
    why_[0] = 0;
    blip(520);
}

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.18f);
    tone_ = 0.08f;
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return;
    std::snprintf(why_, sizeof(why_), "%s", why);
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    blip(140);
}

void Game::finish() {
    if (mode_ == Mode::Win) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::controls(float& thr, float& brk) {
    thr = 0;
    brk = 0;
    if (bot_) {
        pilot(thr, brk);
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.accel > 0.2f) thr = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.2f) brk = 1.f;
    if (p.axisY > 0.35f) thr = p.axisY;
    if (p.axisY < -0.35f) brk = -p.axisY;
}

void Game::pilot(float& thr, float& brk) {
    thr = 1.f;
    brk = 0;
    if (wp_ < 3) return;
    if (inDock(s_) && left_) {
        thr = 0;
        brk = speed_ > 8.f ? 1.f : 0.45f;
        return;
    }
    float dist = fwd(s_, 0.f, length());
    if (dist < 8.f) dist = 8.f;
    if (dist < 560.f) {
        float want = dist * 0.16f;
        if (want < 10.f) want = 10.f;
        if (speed_ > want) {
            thr = 0;
            brk = std::min(1.f, (speed_ - want) / 36.f);
        } else {
            thr = 0.65f;
        }
    }
}

void Game::checkPass(float prev) {
    const float marks[3] = {W + 0.5f * ARC, W + ARC + W, 3.f * W + ARC + 0.5f * ARC};
    auto crossed = [&](float mark) {
        if (prev <= s_) return prev < mark && s_ >= mark;
        return prev < mark || s_ >= mark;
    };
    if (wp_ < 3 && crossed(marks[wp_])) wp_++;
    if (!inDock(s_)) left_ = true;
    if (wp_ >= 3 && left_ && inDock(prev) && !inDock(s_)) fail("missed the end");
}

void Game::physics(float dt, float thr, float brk) {
    float prev = s_;
    speed_ += (thr * 150.f - brk * 240.f - speed_ * 1.55f) * dt;
    if (speed_ < 0) speed_ = 0;
    if (speed_ > 96.f) speed_ = 96.f;
    s_ = wrap(s_ + speed_ * dt, length());
    checkPass(prev);
    if (mode_ != Mode::Run) return;

    if (wp_ >= 3 && left_ && inDock(s_) && speed_ < 9.f) {
        stopT_ += dt;
        if (stopT_ > 0.32f) finish();
    } else {
        stopT_ = 0;
    }
    if (mode_ != Mode::Run) return;

    if (left_ && inFar(s_) && speed_ < 9.f) {
        farT_ += dt;
        if (farT_ > 0.45f) fail("wrong dock");
    } else {
        farT_ = 0;
    }
    if (raceTime_ > LIMIT) fail("missed the end");
}

void Game::audio(float dt) {
    if (tone_ > 0) {
        tone_ -= dt;
        if (tone_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (mode_ == Mode::Run) {
        float vol = 0.04f + speed_ * 0.0014f;
        sys_->apu.noise(vol, 700.f + speed_ * 18.f, true);
    } else if (mode_ != Mode::Title) {
        sys_->apu.noise(0, 0, false);
    }
    if (chimeStep_ >= 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
            if (chimeStep_ < 4) {
                sys_->apu.tone(1, notes[chimeStep_], 0.2f);
                chimeT_ = 0.16f;
                chimeStep_++;
            } else {
                sys_->apu.tone(1, 0, 0);
                chimeStep_ = -1;
            }
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    float dt = 1.f / 60.f;
    t_ += dt;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Run) {
        raceTime_ += dt;
        float thr = 0, brk = 0;
        controls(thr, brk);
        physics(dt, thr, brk);
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
    }
    audio(dt);
    draw();
}

void Game::sky() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wave = 0.5f + 0.5f * std::sin(t_ * 1.3f + y * 0.08f);
        int g = 4 + (y * 5) / gs::SCREEN_H + int(wave);
        int b = 8 + (y * 4) / gs::SCREEN_H;
        if (g > 12) g = 12;
        if (b > 14) b = 14;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(1, g, b);
    }
}

void Game::sprite(const gs::Image& img, float x, float y, int pal, int sw, int sh) {
    if (sw <= 0) sw = img.w;
    if (sh <= 0) sh = img.h;
    if (x + sw < 0 || y + sh < 0 || x > gs::SCREEN_W || y > gs::SCREEN_H) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(float x, float y, const char* s, int pal) {
    float cx = x;
    for (const char* p = s; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c > 127) {
            cx += 6;
            continue;
        }
        const gs::Image& g = art_.font[c - 32];
        sprite(g, cx, y, pal);
        cx += float(g.w > 1 ? g.w + 1 : 4);
    }
}

void Game::textC(float y, const char* s, int pal) {
    float w = 0;
    for (const char* p = s; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c > 127) {
            w += 6;
            continue;
        }
        int gw = art_.font[c - 32].w;
        w += float(gw > 1 ? gw + 1 : 4);
    }
    text((gs::SCREEN_W - w) * 0.5f, y, s, pal);
}

void Game::draw() {
    sky();
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = false;

    if (mode_ == Mode::Title) {
        textC(46, "S3 RAIL BUOY", PAL_TEXT);
        textC(70, "ROUND THE BUOYS", PAL_AMBER);
        textC(86, "RETURN TO THE SAME DOCK", PAL_DIM);
        textC(112, "THE FAR PIER IS NOT THE END", PAL_DIM);
        textC(148, "A THROTTLE    B BRAKE", PAL_TEXT);
        textC(168, bot_ ? "AUTO" : "START", PAL_AMBER);
        return;
    }

    float cx, cy, hx, hy;
    poseAt(s_, cx, cy, hx, hy);
    const float zoom = 0.46f;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = (wx - cx) * zoom + 168.f;
        sy = 118.f - (wy - cy) * zoom;
    };

    const float L = length();
    for (float u = 0; u < L; u += 22.f) {
        float x, y, ihx, ihy;
        poseAt(u, x, y, ihx, ihy);
        float sx, sy;
        project(x, y, sx, sy);
        sprite(art_.sleeper, sx - 5, sy - 5, PAL_RAIL);
        float ox = ihy * 10.f, oy = -ihx * 10.f;
        float fx, fy;
        project(x + ox * 0.15f, y + oy * 0.15f, fx, fy);
        if ((int(u) / 22) % 4 == int(t_ * 3.f) % 4) sprite(art_.foam, fx - 6, fy - 3, PAL_FOAM);
    }

    const float marks[3] = {W + 0.5f * ARC, W + ARC + W, 3.f * W + ARC + 0.5f * ARC};
    for (int i = 0; i < 3; i++) {
        float x, y, ihx, ihy;
        poseAt(marks[i], x, y, ihx, ihy);
        float ix = -ihy, iy = ihx;
        float sx, sy;
        project(x + ix * 48.f, y + iy * 48.f, sx, sy);
        float bob = std::sin(t_ * 2.f + i) * 2.f;
        sprite(art_.buoy, sx - 13, sy - 16 + bob, i < wp_ ? PAL_DOCK : PAL_BUOY);
    }

    float dx, dy;
    project(0, -40.f, dx, dy);
    sprite(art_.dock, dx - 44, dy - 20, PAL_DOCK);
    float fpx, fpy, fhx, fhy;
    poseAt(W + ARC + W, fpx, fpy, fhx, fhy);
    project(fpx, fpy + 46.f, dx, dy);
    sprite(art_.far, dx - 44, dy - 20, PAL_FAR);

    int dir = int(std::floor((std::atan2(hy, hx) + PI + PI * 0.125f) / (PI * 0.25f))) & 7;
    sprite(art_.car[dir], 168 - 20, 118 - 20, PAL_CAR);

    char line[48];
    std::snprintf(line, sizeof(line), "BUOY %d/3", wp_ > 3 ? 3 : wp_);
    text(8, 8, line, PAL_TEXT);
    std::snprintf(line, sizeof(line), "%d", int(speed_));
    text(276, 8, line, PAL_DIM);
    if (wp_ >= 3 && mode_ == Mode::Run) text(8, 20, "HOME AHEAD", PAL_AMBER);
    if (mode_ == Mode::Win) {
        textC(92, "SAME DOCK", PAL_AMBER);
        textC(108, "LEG MADE", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        textC(92, "LEG FAILED", PAL_AMBER);
        textC(108, why_[0] ? why_ : "MISSED THE END", PAL_TEXT);
    }
}

}  // namespace railbuoy
