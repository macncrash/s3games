#include "heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace helilane {
namespace {

constexpr int HORIZON = 72;
constexpr float Z_NEAR = 5.0f;
constexpr float Z_SHIP = 7.4f;
constexpr float PPM_NEAR = 48.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float COURSE = 400.f;
constexpr float LIMIT = 42.f;
constexpr float CREW = 10.6f;
constexpr float DT = 1.f / 60.f;

int fogFor(float ahead) {
    return int(std::clamp((ahead - 8.f) * 0.16f, 0.f, 12.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 60.f) return 3;
    if (std::fabs(x_) > halfAt(dist_) * 0.7f) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 42.f * std::sin(s * 0.013f) + 14.f * std::sin(s * 0.037f + 0.7f);
}

float Game::halfAt(float s) const {
    float pinch = 0.5f + 0.5f * std::sin(s * 0.026f + 0.4f);
    return 2.15f + pinch * 1.05f;
}

float Game::gustAt(float s) const {
    return 2.05f * std::sin(s * 0.048f) + 0.85f * std::sin(s * 0.11f + 1.3f);
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
    speed_ = 11.f;
    scenery_ = 30.f;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::begin() {
    dist_ = x_ = vx_ = helm_ = clock_ = 0;
    speed_ = 8.2f;
    won_ = false;
    over_ = false;
    why_ = "";
    mode_ = Mode::Run;
    sys_->rumble(0.1f, 0.04f, 80);
}

void Game::finish(bool good, const char* why) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = why;
    if (good) {
        sys_->apu.keyOn(1, 392.f, 0.15f);
        sys_->apu.keyOn(2, 523.25f, 0.12f);
        sys_->setLight(40, 170, 90);
        sys_->rumble(0.12f, 0.3f, 120);
    } else {
        sys_->apu.keyOn(1, 98.f, 0.16f);
        sys_->setLight(170, 40, 20);
        sys_->rumble(0.4f, 0.08f, 160);
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float want = 0;
    float throttle = 0.55f;
    float gust = gustAt(dist_);
    if (bot_) {
        want = std::clamp(-x_ * 1.7f - vx_ * 1.05f - gust * 0.32f, -1.f, 1.f);
        throttle = 1.f;
    } else {
        if (pad.down(gs::BTN_LEFT)) want -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) want += 1.f;
        if (std::fabs(pad.axisX) > 0.18f) want = pad.axisX;
        if (pad.down(gs::BTN_UP) || pad.accel > 0.2f) throttle = 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) throttle = 0.15f;
    }
    helm_ += (std::clamp(want, -1.f, 1.f) - helm_) * (1.f - std::exp(-3.1f * dt));
    float target = 7.4f + throttle * 5.6f;
    speed_ += (target - speed_) * (1.f - std::exp(-0.9f * dt));
    vx_ += (helm_ * 5.4f + gust) * dt;
    vx_ *= std::exp(-2.15f * dt);
    x_ += vx_ * dt;
    dist_ += speed_ * dt;
    clock_ += dt;
    float half = halfAt(dist_);
    float crewAlong = CREW * clock_;
    if (std::fabs(x_) > half) finish(false, "left the lane");
    else if (dist_ >= COURSE) finish(true, "held");
    else if (crewAlong >= COURSE || clock_ >= LIMIT) finish(false, "the other crew");
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = ahead + Z_SHIP;
    if (z < 0.35f) return p;
    float t = Z_NEAR / z;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON - 8 || p.y >= gs::SCREEN_H + 30) return p;
    p.ppm = PPM_NEAR * t;
    float along = ((mode_ == Mode::Title) ? scenery_ : dist_) + ahead;
    p.x = 160.f + bendAt(along) + wx * p.ppm;
    p.fog = fogFor(ahead);
    p.ok = true;
    return p;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
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
    sp.shadow = shadow;
    sys_->vdp.sprite(sp);
}

void Game::lane(float view) {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(6 + (1.f - u) * 6), int(9 + (1.f - u) * 4), int(13 + u));
            v.lineFog[y] = uint8_t(u * 3);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_SHIP;
        float ppm = PPM_NEAR * t;
        float along = view + ahead;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(along);
        rd.hw = std::max(6.f, halfAt(along) * ppm);
        rd.v = along * 28.f;
        rd.pal = PAL_LANE;
        rd.style = 1;
        rd.band = (int(std::floor(along / 10.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 8);
        v.lineBackdrop[y] = gs::rgb4(3, 5, 3);
    }
}

void Game::marks(float along) {
    auto row = [&](float step, auto place) {
        float d0 = std::floor((along - 4.f) / step) * step;
        for (float m = d0; m < along + 130.f; m += step) place(m, m - along);
    };
    row(22.f, [&](float m, float ahead) {
        if (m < 8.f) return;
        float half = halfAt(m);
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (half + 0.35f), ahead);
            if (!p.ok) continue;
            spr(art_.pylon, p.x, p.y, p.ppm * 2.4f, PAL_PYLON, side < 0, p.fog);
        }
    });
    row(38.f, [&](float m, float ahead) {
        int side = (int(m / 38.f) & 1) ? 1 : -1;
        Proj p = project(side * (halfAt(m) + 6.5f), ahead + 4.f);
        if (!p.ok) return;
        float cy = p.y - p.ppm * 3.2f;
        spr(art_.cloud, p.x, cy, p.ppm * 1.6f, PAL_ROTOR, side < 0, std::min(14, p.fog + 2));
    });
    float crewAhead = CREW * ((mode_ == Mode::Run || mode_ == Mode::Pause) ? clock_ : 12.f) - along;
    if (mode_ == Mode::Title) crewAhead = 48.f;
    if (crewAhead > 2.f && crewAhead < 140.f) {
        Proj p = project(0.15f * std::sin(crewAhead * 0.2f), crewAhead);
        if (p.ok) {
            spr(art_.body, p.x, p.y - p.ppm * 1.1f, p.ppm * 1.5f, PAL_RIVAL, false, p.fog);
            spr(art_.disc, p.x, p.y - p.ppm * 2.2f, p.ppm * 0.55f, PAL_ROTOR, rotorFrame_ & 1, p.fog);
        }
    }
    Proj gate = project(0.f, COURSE - along);
    if (gate.ok) spr(art_.gate, gate.x, gate.y - gate.ppm * 0.2f, gate.ppm * 2.6f, PAL_SIGN, false, gate.fog);
}

void Game::shipAt() {
    float t = Z_NEAR / Z_SHIP;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * (PPM_NEAR * t);
    float bob = std::sin(t_ * 6.2f) * 1.4f;
    float lean = helm_ * 10.f;
    spr(art_.shadow, x + lean * 0.3f, y + 18.f, 22.f, PAL_SHIP, false, 0, true);
    spr(art_.body, x + lean, y - 8.f + bob, 46.f, PAL_SHIP);
    spr(art_.fin, x - 22.f + lean * 0.4f, y - 6.f + bob, 18.f, PAL_SHIP, helm_ < -0.05f);
    float spin = 34.f + 6.f * std::sin(rotor_);
    spr(art_.disc, x + lean * 0.2f, y - 28.f + bob, spin, PAL_ROTOR);
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
    lane(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 34, float(art_.title.h), PAL_SIGN);
        spr(art_.stay, 160, 58, float(art_.stay.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 82, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.held, 160, 36, float(art_.held.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& banner = (why_ && std::strcmp(why_, "left the lane") == 0) ? art_.left : art_.missed;
        spr(banner, 160, 36, float(banner.h), PAL_ALERT);
    }
    shipAt();
    marks(view);

    if (mode_ == Mode::Title) {
        textC(23, "LEFT RIGHT HOLD THE LANE", PAL_HUD);
        textC(24, "UP IS THE COLLECTIVE", PAL_HUD);
        textC(25, "THE CLOCK IS THE OTHER CREW", PAL_SIGN);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_SIGN);
        textC(14, "START RESUMES", PAL_HUD);
        return;
    }
    int left = int(std::ceil(COURSE - dist_));
    if (left < 0 || mode_ == Mode::Win) left = 0;
    char m[24], clock[24];
    std::snprintf(m, sizeof m, "%d M", left);
    float crewLeft = COURSE - CREW * clock_;
    if (mode_ == Mode::Win) crewLeft = std::max(0.f, crewLeft);
    if (mode_ == Mode::Fail && why_ && std::strcmp(why_, "left the lane") != 0) crewLeft = 0;
    int remain = int(std::ceil(std::max(0.f, crewLeft / CREW)));
    std::snprintf(clock, sizeof clock, "CREW %d", remain);
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
    if (mode_ == Mode::Run && std::fabs(x_) > half * 0.7f) textC(2, "PAINT", PAL_ALERT);
    if (mode_ == Mode::Run && dist_ < 30.f) textC(3, "HOLD THE LANE", PAL_HUD);
    if (mode_ == Mode::Win) textC(16, "THE LEG IS IN", PAL_WIN);
    if (mode_ == Mode::Fail && why_ && std::strcmp(why_, "the other crew") == 0)
        textC(16, "THE OTHER CREW FINISHED", PAL_ALERT);
}

void Game::audio() {
    if (mode_ == Mode::Run || mode_ == Mode::Title) {
        rotor_ += DT * (18.f + speed_ * 0.35f);
        rotorFrame_ = int(rotor_ * 3.f);
        float wob = 1.f + 0.03f * std::sin(rotor_ * 2.f);
        sys_->apu.tone(0, 78.f * wob, mode_ == Mode::Title ? 0.02f : 0.04f);
        sys_->apu.tone(1, 156.f * wob, 0.012f);
        sys_->apu.noise(mode_ == Mode::Run ? 0.02f : 0.008f, 900.f, false);
    } else if (mode_ != Mode::Win && mode_ != Mode::Fail) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += 4.2f * DT;
        if (scenery_ > COURSE - 40.f) scenery_ = 24.f;
        if (pad.pressed(gs::BTN_START) || (bot_ && t_ > 0.35f)) begin();
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

}  // namespace helilane
