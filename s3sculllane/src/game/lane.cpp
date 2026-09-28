#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sculllane {
namespace {

constexpr int HORIZON = 72;
constexpr float Z_NEAR = 4.8f;
constexpr float Z_STERN = 7.4f;
constexpr float PPM_NEAR = 52.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float COURSE = 460.f;
constexpr float CREW_PACE = 8.85f;
constexpr float DT = 1.f / 60.f;

int fogFor(float ahead) {
    return int(std::clamp((ahead - 4.f) * 0.16f, 0.f, 12.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 70.f) return 3;
    if (std::fabs(x_) > halfAt(dist_) * 0.62f) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 36.f * std::sin(s * 0.011f) + 12.f * std::sin(s * 0.033f + 0.7f);
}

float Game::halfAt(float s) const {
    float pinch = 0.5f + 0.5f * std::sin(s * 0.022f + 0.4f);
    return 2.55f + pinch * 0.85f;
}

float Game::driftAt(float s) const {
    return 1.45f * std::sin(s * 0.047f) + 0.7f * std::sin(s * 0.11f + 1.2f);
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
    dist_ = x_ = vx_ = helm_ = clock_ = stroke_ = 0;
    speed_ = 8.f;
    scenery_ = 20.f;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::begin() {
    dist_ = x_ = vx_ = helm_ = clock_ = stroke_ = 0;
    speed_ = 6.4f;
    won_ = false;
    over_ = false;
    why_ = "";
    mode_ = Mode::Row;
    sys_->rumble(0.05f, 0.02f, 70);
}

void Game::finish(bool good, const char* why) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = why;
    if (good) {
        sys_->apu.keyOn(1, 392.f, 0.15f);
        sys_->apu.keyOn(2, 523.25f, 0.12f);
        sys_->setLight(40, 170, 110);
        sys_->rumble(0.1f, 0.22f, 110);
    } else {
        sys_->apu.keyOn(1, 98.f, 0.16f);
        sys_->setLight(170, 40, 24);
        sys_->rumble(0.35f, 0.06f, 140);
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float want = 0;
    bool stroke = false;
    float drift = driftAt(dist_);
    if (bot_) {
        float half = halfAt(dist_);
        float edge = x_ / std::max(half, 0.4f);
        want = std::clamp(-x_ * 2.4f - vx_ * 1.55f - drift * 0.42f - edge * edge * edge * 1.8f, -1.f, 1.f);
        if (std::fabs(x_) > half * 0.5f) want = std::copysign(1.f, -x_);
        stroke = true;
    } else {
        if (pad.down(gs::BTN_LEFT)) want -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) want += 1.f;
        if (std::fabs(pad.axisX) > 0.18f) want = pad.axisX;
        stroke = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_UP) || pad.accel > 0.2f;
        if (pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.25f) stroke = false;
    }
    helm_ += (std::clamp(want, -1.f, 1.f) - helm_) * (1.f - std::exp(-3.1f * dt));
    float target = stroke ? 13.4f : 3.2f;
    speed_ += (target - speed_) * (1.f - std::exp(-1.15f * dt));
    vx_ += (helm_ * 6.1f + drift) * dt;
    vx_ *= std::exp(-2.35f * dt);
    x_ += vx_ * dt;
    dist_ += speed_ * dt;
    clock_ += dt;
    if (stroke) stroke_ += dt * (1.6f + speed_ * 0.05f);
    float half = halfAt(dist_);
    float crewLeft = (COURSE / CREW_PACE) - clock_;
    if (std::fabs(x_) > half) finish(false, "left the lane");
    else if (dist_ >= COURSE) finish(true, "held the lane");
    else if (crewLeft <= 0.f) finish(false, "the other crew");
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = ahead + Z_STERN;
    if (z < 0.35f) return p;
    float t = Z_NEAR / z;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON - 8 || p.y >= gs::SCREEN_H + 28) return p;
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

void Game::water(float view) {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(6 + (1.f - u) * 6), int(9 + (1.f - u) * 4), int(12 + u));
            v.lineFog[y] = uint8_t(u * 3);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_STERN;
        float ppm = PPM_NEAR * t;
        float along = view + ahead;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(along);
        rd.hw = std::max(6.f, halfAt(along) * ppm);
        rd.v = along * 34.f;
        rd.pal = PAL_LANE;
        rd.style = 2;
        rd.band = (int(std::floor(along / 6.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        float fogT = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 8);
        v.lineBackdrop[y] = gs::rgb4(1, 4, 7);
    }
}

void Game::buoys(float along) {
    auto row = [&](float step, auto place) {
        float d0 = std::floor((along - 8.f) / step) * step;
        for (float m = d0; m < along + 120.f; m += step) place(m, m - along);
    };
    row(14.f, [&](float m, float ahead) {
        float half = halfAt(m);
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (half + 0.35f), ahead);
            if (!p.ok) continue;
            spr(art_.buoy, p.x, p.y, p.ppm * 1.15f, PAL_BUOY, side < 0, p.fog);
        }
    });
    float finishAhead = COURSE - along;
    if (finishAhead > -6.f && finishAhead < 130.f) {
        float half = halfAt(COURSE);
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (half + 0.15f), finishAhead);
            if (!p.ok) continue;
            spr(art_.flag, p.x, p.y - 4.f, p.ppm * 2.4f, PAL_BUOY, side < 0, p.fog);
        }
    }
}

void Game::shellAt() {
    float t = Z_NEAR / Z_STERN;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * (PPM_NEAR * t);
    float bob = std::sin(t_ * 2.1f) * 1.1f;
    float phase = stroke_ - std::floor(stroke_);
    float reach = std::sin(phase * 6.2831853f);
    spr(art_.splash, x, y + 18.f, 16.f + std::fabs(reach) * 4.f, PAL_OAR);
    spr(art_.shell, x, y + 8.f + bob, 58.f, PAL_SHELL);
    spr(art_.cox, x, y - 6.f + bob, 14.f, PAL_CREW);
    float oy = y + 2.f + bob + reach * 6.f;
    spr(art_.oar, x - 34.f, oy, 12.f, PAL_OAR, true);
    spr(art_.oar, x + 34.f, oy, 12.f, PAL_OAR, false);
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
    water(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 32, float(art_.title.h), PAL_SIGN);
        spr(art_.stay, 160, 58, float(art_.stay.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 86, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.held, 160, 34, float(art_.held.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& banner = (why_ && std::strcmp(why_, "the other crew") == 0) ? art_.crew : art_.left;
        spr(banner, 160, 34, float(banner.h), PAL_ALERT);
    }
    shellAt();
    buoys(view);

    if (mode_ == Mode::Title) {
        textC(22, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        textC(24, "LEFT RIGHT STEER", PAL_HUD);
        textC(25, "A OR UP TO ROW", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_SIGN);
        textC(14, "START RESUMES", PAL_HUD);
        return;
    }
    int left = int(std::ceil(COURSE - dist_));
    if (left < 0 || mode_ == Mode::Win) left = 0;
    float crewLeft = std::max(0.f, (COURSE / CREW_PACE) - clock_);
    if (mode_ == Mode::Win) crewLeft = std::max(0.f, crewLeft);
    char m[24], clock[24];
    std::snprintf(m, sizeof m, "%d M", left);
    std::snprintf(clock, sizeof clock, "CREW %d", int(std::ceil(crewLeft)));
    text(1, 0, m, PAL_HUD);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, mode_ == Mode::Row && crewLeft < 6.f ? PAL_ALERT : PAL_HUD);
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
    int pal = std::fabs(x_) > half * 0.62f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    if (mode_ == Mode::Row && std::fabs(x_) > half * 0.62f) textC(2, "LANE EDGE", PAL_ALERT);
    if (mode_ == Mode::Row && dist_ < 24.f) textC(3, "HOLD THE LANE", PAL_HUD);
    if (mode_ == Mode::Win) textC(16, "AHEAD OF THE CREW", PAL_WIN);
    if (mode_ == Mode::Fail && why_ && std::strcmp(why_, "the other crew") == 0)
        textC(16, "THE OTHER CREW", PAL_ALERT);
    else if (mode_ == Mode::Fail) textC(16, "OUT OF THE LANE", PAL_ALERT);
}

void Game::audio() {
    if (mode_ == Mode::Row) {
        float phase = stroke_ - std::floor(stroke_);
        if (phase < DT * 2.2f) sys_->apu.tone(1, 180.f + speed_ * 4.f, 0.05f);
        else if (phase > 0.48f && phase < 0.48f + DT * 2.2f) sys_->apu.tone(1, 120.f, 0.03f);
        sys_->apu.noise(0.012f, 280.f, false);
    } else if (mode_ != Mode::Win && mode_ != Mode::Fail) {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += 3.2f * DT;
        if (scenery_ > COURSE - 40.f) scenery_ = 16.f;
        stroke_ += DT * 1.1f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || (bot_ && t_ > 0.35f)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Row;
    } else if (mode_ == Mode::Row) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        toTitle();
    }
    draw();
    audio();
}

}  // namespace sculllane
