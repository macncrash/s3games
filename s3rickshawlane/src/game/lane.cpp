#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rickshawlane {
namespace {

constexpr int HORIZON = 76;
constexpr float Z_NEAR = 5.0f;
constexpr float Z_AXLE = 8.2f;
constexpr float PPM_NEAR = 48.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float COURSE = 320.f;
constexpr float LIMIT = 42.f;
constexpr float CRUISE = 10.5f;
constexpr float DT = 1.f / 60.f;

int fogFor(float ahead) {
    return int(std::clamp((ahead - 6.f) * 0.16f, 0.f, 12.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 50.f) return 3;
    if (std::fabs(x_) > halfAt(dist_) * 0.7f) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 42.f * std::sin(s * 0.016f) + 14.f * std::sin(s * 0.047f + 0.7f);
}

float Game::halfAt(float s) const {
    float pinch = 0.5f + 0.5f * std::sin(s * 0.031f + 0.4f);
    return 1.95f + pinch * 0.7f;
}

float Game::leanAt(float s) const {
    return 1.05f * std::sin(s * 0.048f) + 0.4f * std::sin(s * 0.11f + 0.5f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.08f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = helm_ = clock_ = 0;
    speed_ = CRUISE;
    scenery_ = 20.f;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::begin() {
    dist_ = x_ = vx_ = helm_ = clock_ = 0;
    speed_ = 6.8f;
    won_ = false;
    over_ = false;
    why_ = "";
    mode_ = Mode::Run;
    sys_->rumble(0.06f, 0.02f, 70);
    sys_->apu.keyOn(1, 660.f, 0.08f);
}

void Game::finish(bool good, const char* why) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = why;
    if (good) {
        sys_->apu.keyOn(1, 523.25f, 0.14f);
        sys_->apu.keyOn(2, 659.25f, 0.11f);
        sys_->setLight(40, 150, 50);
        sys_->rumble(0.1f, 0.22f, 110);
    } else {
        sys_->apu.keyOn(1, 98.f, 0.16f);
        sys_->setLight(150, 30, 16);
        sys_->rumble(0.35f, 0.06f, 150);
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float want = 0;
    float throttle = 0.55f;
    if (bot_) {
        float lean = leanAt(dist_);
        want = std::clamp(-x_ * 1.7f - vx_ * 1.05f - lean * 0.32f, -1.f, 1.f);
        throttle = 1.f;
    } else {
        if (pad.down(gs::BTN_LEFT)) want -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) want += 1.f;
        if (std::fabs(pad.axisX) > 0.18f) want = pad.axisX;
        if (pad.down(gs::BTN_UP) || pad.accel > 0.2f) throttle = 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) throttle = 0.f;
    }
    helm_ += (std::clamp(want, -1.f, 1.f) - helm_) * (1.f - std::exp(-2.8f * dt));
    float target = 5.6f + throttle * 6.2f;
    speed_ += (target - speed_) * (1.f - std::exp(-0.85f * dt));
    float lean = leanAt(dist_);
    vx_ += (helm_ * 4.2f + lean) * dt;
    vx_ *= std::exp(-2.05f * dt);
    x_ += vx_ * dt;
    dist_ += speed_ * dt;
    clock_ += dt;
    float half = halfAt(dist_);
    if (std::fabs(x_) > half) finish(false, "left the lane");
    else if (dist_ >= COURSE) finish(true, "held");
    else if (clock_ >= LIMIT) finish(false, "missed the end");
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = ahead + Z_AXLE;
    if (z < 0.4f) return p;
    float t = Z_NEAR / z;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON - 6 || p.y >= gs::SCREEN_H + 24) return p;
    p.ppm = PPM_NEAR * t;
    float along = ((mode_ == Mode::Title) ? scenery_ : dist_) + ahead;
    p.x = 160.f + bendAt(along) + wx * p.ppm;
    p.fog = fogFor(ahead);
    p.ok = true;
    return p;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!(h > 1.4f) || m.h <= 0) return;
    const gs::Image& img = m.pick(h);
    float s = h / float(img.h);
    gs::Sprite sp;
    sp.img = img;
    sp.h = std::max(1, int(std::lround(h)));
    sp.w = std::max(1, int(std::lround(img.w * s)));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(cy - sp.h));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(sp);
}

void Game::street(float view) {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(12 - u * 4), int(7 - u * 2), int(5 + u * 4));
            v.lineFog[y] = uint8_t((1.f - u) * 3);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_AXLE;
        float ppm = PPM_NEAR * t;
        float along = view + ahead;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(along);
        rd.hw = std::max(4.f, halfAt(along) * ppm);
        rd.v = along * 32.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (int(std::floor(along / 6.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 8);
        v.lineBackdrop[y] = gs::rgb4(4, 4, 4);
    }
}

void Game::sides(float along) {
    auto row = [&](float step, auto place) {
        float d0 = std::floor((along - 6.f) / step) * step;
        for (float m = d0; m < along + 110.f; m += step) place(m, m - along);
    };
    row(14.f, [&](float m, float ahead) {
        float half = halfAt(m);
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (half + 0.7f), ahead);
            if (!p.ok) continue;
            spr(art_.lamp, p.x, p.y, p.ppm * 2.4f, PAL_LAMP, side < 0, p.fog);
        }
    });
    row(22.f, [&](float m, float ahead) {
        if (m < 12.f) return;
        int side = (int(m / 22.f) & 1) ? 1 : -1;
        Proj p = project(side * (halfAt(m) + 2.6f), ahead);
        if (!p.ok) return;
        spr(art_.stall, p.x, p.y, p.ppm * 2.2f, PAL_STALL, side < 0, p.fog);
    });
    Proj gate = project(0.f, COURSE - along);
    if (gate.ok) spr(art_.arch, gate.x, gate.y - gate.ppm * 0.2f, gate.ppm * 3.2f, PAL_SIGN, false, gate.fog);
}

void Game::rickshawAt() {
    float t = Z_NEAR / Z_AXLE;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * (PPM_NEAR * t);
    float bob = std::sin(t_ * 7.f) * 0.8f;
    float lean = helm_ * 4.f;
    spr(art_.shadow, x, y + 18.f, 14.f, PAL_WHEEL, false, 0);
    spr(art_.wheel, x - 16.f, y + 14.f + bob, 18.f, PAL_WHEEL);
    spr(art_.wheel, x + 16.f, y + 14.f + bob, 18.f, PAL_WHEEL);
    spr(art_.cab, x + lean * 0.3f, y + 6.f + bob, 40.f, PAL_CAB);
    spr(art_.rider, x + lean, y - 8.f + bob, 22.f, PAL_CAB);
    spr(art_.wheel, x + lean * 1.4f, y - 2.f + bob, 12.f, PAL_WHEEL);
}

void Game::text(int col, int row, const char* s, int pal) {
    for (int i = 0; s[i]; i++, col++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
        if (c <= 32 || c > 127 || col < 0 || col > 39 || row < 0 || row > 27) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(col, row, gs::entry(tile, pal));
    }
}

void Game::textC(int row, const char* s, int pal) {
    text((40 - int(std::strlen(s))) / 2, row, s, pal);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    float view = (mode_ == Mode::Title) ? scenery_ : dist_;
    street(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 32, float(art_.title.h), PAL_SIGN);
        spr(art_.stay, 160, 58, float(art_.stay.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 86, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.held, 160, 34, float(art_.held.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& banner = (why_ && std::strcmp(why_, "missed the end") == 0) ? art_.missed : art_.left;
        spr(banner, 160, 34, float(banner.h), PAL_ALERT);
    }
    rickshawAt();
    sides(view);

    if (mode_ == Mode::Title) {
        textC(24, "LEFT RIGHT STEER", PAL_HUD);
        textC(25, "UP DOWN THE PEDALS", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_SIGN);
        textC(14, "START RESUMES", PAL_HUD);
        return;
    }
    int left = int(std::ceil(COURSE - dist_));
    if (left < 0 || mode_ == Mode::Win) left = 0;
    char m[24], clock[20];
    std::snprintf(m, sizeof m, "%d M", left);
    int remain = int(std::ceil(std::max(0.f, LIMIT - clock_)));
    if (mode_ == Mode::Fail) remain = 0;
    std::snprintf(clock, sizeof clock, "%d S", remain);
    text(1, 0, m, PAL_HUD);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, mode_ == Mode::Run && remain < 8 ? PAL_ALERT : PAL_HUD);
    float half = std::max(0.4f, halfAt(dist_));
    char bar[22];
    const int n = 19;
    int mid = n / 2;
    int pos = mid + int(std::lround((x_ / half) * (mid - 1)));
    pos = std::clamp(pos, 0, n - 1);
    for (int i = 0; i < n; i++) bar[i] = (i == pos) ? '+' : '-';
    bar[0] = '<';
    bar[n - 1] = '>';
    bar[n] = 0;
    int pal = std::fabs(x_) > half * 0.7f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    if (mode_ == Mode::Run && std::fabs(x_) > half * 0.7f) textC(2, "EDGE", PAL_ALERT);
    if (mode_ == Mode::Run && dist_ < 26.f) textC(3, "HOLD THE LANE", PAL_HUD);
    if (mode_ == Mode::Win) textC(16, "THE LEG IS IN", PAL_WIN);
    if (mode_ == Mode::Fail && why_ && std::strcmp(why_, "missed the end") == 0) textC(16, "THE END WAS MISSED", PAL_ALERT);
}

void Game::audio() {
    if (mode_ == Mode::Run) {
        bell_ += DT * (0.55f + speed_ * 0.07f);
        float wob = 1.f + 0.03f * std::sin(bell_ * 11.f);
        sys_->apu.tone(0, 74.f * wob, 0.03f);
        sys_->apu.noise(0.012f, 900.f, false);
    } else if (mode_ != Mode::Win && mode_ != Mode::Fail) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += CRUISE * 0.32f * DT;
        if (scenery_ > COURSE - 28.f) scenery_ = 16.f;
        if (pad.pressed(gs::BTN_START) || (bot_ && t_ > 0.4f)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE) && !bot_) toTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
        toTitle();
    }
    draw();
    audio();
}

}  // namespace rickshawlane
