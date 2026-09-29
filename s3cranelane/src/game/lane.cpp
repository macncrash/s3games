#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cranelane {
namespace {

constexpr int HORIZON = 68;
constexpr float Z_NEAR = 4.0f;
constexpr float Z_CRANE = 8.2f;
constexpr float PPM_NEAR = 50.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float LANE = 2.35f;
constexpr float SPEED = 15.2f;
constexpr float AUTH = 9.4f;
constexpr float COURSE = 360.f;
constexpr float DT = 1.f / 60.f;

int fogFor(float ahead) {
    return int(std::clamp((ahead - 6.f) * 0.18f, 0.f, 12.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 55.f) return 3;
    if (std::fabs(hookX_) > LANE * 0.72f) return 2;
    return 1;
}

float Game::gustAt(float s) const {
    return 2.15f * std::sin(s * 0.041f) + 1.05f * std::sin(s * 0.097f + 1.1f);
}

float Game::bendAt(float s) const {
    return 26.f * std::sin(s * 0.014f) + 8.f * std::sin(s * 0.039f + 0.4f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.1f, 0.16f, 0.06f);
    toTitle();
    over_ = false;
    won_ = false;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = swing_ = swv_ = hookX_ = race_ = 0;
    scenery_ = 30.f;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::begin() {
    dist_ = x_ = vx_ = swing_ = swv_ = hookX_ = race_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    mode_ = Mode::Run;
    sys_->apu.keyOn(0, 196.f, 0.11f);
    sys_->rumble(0.18f, 0.05f, 90);
}

void Game::finish(bool good) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = good ? "held" : "left the lane";
    if (good) {
        sys_->apu.keyOn(1, 392.f, 0.15f);
        sys_->apu.keyOn(2, 523.25f, 0.13f);
        sys_->setLight(30, 160, 50);
        sys_->rumble(0.12f, 0.3f, 160);
    } else {
        sys_->apu.keyOn(1, 110.f, 0.16f);
        sys_->setLight(170, 28, 16);
        sys_->rumble(0.5f, 0.12f, 200);
    }
}

void Game::update(float dt) {
    float steer = 0;
    const gs::Pad& pad = sys_->pad;
    float gust = gustAt(dist_);
    if (bot_) {
        float aim = -gust * 0.045f;
        steer = (aim - x_) * 1.85f - vx_ * 1.15f - swing_ * 1.55f - swv_ * 0.35f;
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
    }
    steer = std::clamp(steer, -1.f, 1.f);
    vx_ += (steer * AUTH - gust) * dt;
    vx_ *= std::exp(-2.6f * dt);
    x_ += vx_ * dt;

    float pull = -swing_ * 6.5f - vx_ * 1.8f;
    swv_ += pull * dt;
    swv_ *= std::exp(-1.6f * dt);
    swing_ += swv_ * dt;
    swing_ = std::clamp(swing_, -3.2f, 3.2f);
    hookX_ = x_ + swing_ * 0.85f;

    dist_ += SPEED * dt;
    race_ += dt;
    if (std::fabs(hookX_) > LANE || std::fabs(x_) > LANE) finish(false);
    else if (dist_ >= COURSE) finish(true);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += SPEED * DT;
        if (scenery_ > COURSE) scenery_ = 18.f;
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

void Game::audio() {
    if (mode_ == Mode::Run) {
        engine_ += DT;
        float wob = 1.f + 0.025f * std::sin(engine_ * 36.f);
        sys_->apu.tone(0, 62.f * wob, 0.055f);
        sys_->apu.noise(0.018f, 700.f, false);
        float cable = 180.f + std::fabs(swing_) * 40.f;
        sys_->apu.tone(3, cable, 0.012f + std::fabs(swv_) * 0.01f);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(3, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = Z_CRANE + ahead;
    if (z < 2.2f) return p;
    float t = Z_NEAR / z;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON - 8 || p.y >= gs::SCREEN_H + 24) return p;
    p.ppm = PPM_NEAR * t;
    float along = ((mode_ == Mode::Title) ? scenery_ : dist_) + ahead;
    p.x = 160.f + bendAt(along) + wx * p.ppm;
    p.fog = fogFor(ahead);
    p.ok = true;
    return p;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (!(h > 1.5f) || m.h <= 0) return;
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

void Game::road(float view) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            int r = int(4 + (1.f - u) * 4);
            int g = int(5 + (1.f - u) * 3);
            int b = int(6 + u * 4);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(u * 5);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_CRANE;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(view + ahead);
        rd.hw = std::max(4.f, LANE * ppm);
        rd.v = (view + ahead) * 34.f;
        rd.pal = PAL_ROAD;
        rd.style = gs::ROAD_RUTS;
        rd.band = (int(std::floor((view + ahead) / 8.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 9);
        v.lineBackdrop[y] = gs::rgb4(3, 3, 2);
    }
}

void Game::yard(float along) {
    auto row = [&](float step, float from, float to, auto place) {
        float d0 = std::floor((along - 10.f) / step) * step;
        for (float m = d0; m < along + 100.f; m += step) {
            if (m < from || m > to) continue;
            place(m, m - along);
        }
    };
    row(16.f, 0.f, COURSE, [&](float m, float ahead) {
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (LANE + 0.85f), ahead);
            if (!p.ok) continue;
            spr(art_.cone, p.x, p.y, p.ppm * 0.85f, PAL_CONE, side < 0, p.fog);
        }
    });
    row(26.f, -8.f, COURSE + 8.f, [&](float m, float ahead) {
        int side = (int(m / 26.f) & 1) ? 1 : -1;
        Proj p = project(side * (LANE + 2.6f), ahead);
        if (!p.ok) return;
        bool pipes = (int(m) / 26) % 3 != 0;
        if (pipes) spr(art_.stack, p.x, p.y, p.ppm * 2.4f, PAL_YARD, side < 0, p.fog);
        else spr(art_.barrier, p.x, p.y, p.ppm * 1.1f, PAL_CONE, side < 0, p.fog);
    });
    row(34.f, 6.f, COURSE - 12.f, [&](float m, float ahead) {
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (LANE + 1.7f), ahead);
            if (!p.ok) continue;
            spr(art_.flood, p.x, p.y, p.ppm * 2.6f, PAL_LIGHT, side < 0, p.fog);
        }
    });
    Proj gate = project(0.f, COURSE - along);
    if (gate.ok) spr(art_.gate, gate.x, gate.y - gate.ppm * 0.4f, gate.ppm * 3.1f, PAL_GATE, false, gate.fog);
}

void Game::craneAt() {
    float t = Z_NEAR / Z_CRANE;
    float ppm = PPM_NEAR * t;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float swing = (mode_ == Mode::Title) ? std::sin(t_ * 1.4f) * 0.35f : swing_;
    float x = 160.f + bendAt(along) + xoff * ppm;
    float bob = std::sin(t_ * 14.f) * 0.5f;
    spr(art_.shadow, x, y + 18.f, 14.f, PAL_CRANE, false, 0, true);
    spr(art_.crane, x, y + 10.f + bob, 78.f, PAL_CRANE);
    float hx = x + swing * ppm * 0.85f;
    spr(art_.hook, hx, y + 4.f, 16.f, PAL_CRANE);
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
    road(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 34, float(art_.title.h), PAL_GATE);
        spr(art_.stay, 160, 58, float(art_.stay.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 88, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.held, 160, 38, float(art_.held.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.left, 160, 38, float(art_.left.h), PAL_ALERT);
    }
    yard(view);
    craneAt();

    if (mode_ == Mode::Title) {
        textC(23, "ARROWS STEER THE CRANE", PAL_HUD);
        textC(24, "THE HOOK MUST STAY IN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_GATE);
        return;
    }
    int left = int(std::ceil(COURSE - dist_));
    if (left < 0) left = 0;
    if (mode_ == Mode::Win) left = 0;
    char m[24], clock[20];
    std::snprintf(m, sizeof m, "%d M", mode_ == Mode::Fail ? int(dist_ + 0.5f) : left);
    int cs = int(race_ * 100) % 100;
    int sec = int(race_) % 60;
    int min = int(race_) / 60;
    if (cs < 0) cs = 0;
    std::snprintf(clock, sizeof clock, "%d:%02d.%02d", min, sec, cs);
    text(1, 0, m, PAL_HUD);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, PAL_HUD);
    char bar[22];
    const int n = 19;
    int mid = n / 2;
    int pos = mid + int(std::lround((hookX_ / LANE) * (mid - 1)));
    pos = std::clamp(pos, 0, n - 1);
    for (int i = 0; i < n; i++) bar[i] = (i == pos) ? '+' : '-';
    bar[0] = '<';
    bar[n - 1] = '>';
    bar[n] = 0;
    int pal = std::fabs(hookX_) > LANE * 0.72f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    if (mode_ == Mode::Run && std::fabs(hookX_) > LANE * 0.72f) textC(2, "HOOK", PAL_ALERT);
    if (mode_ == Mode::Run && race_ < 2.6f) textC(3, "STAY IN THE LANE", PAL_HUD);
    if (mode_ == Mode::Fail) textC(14, "LEFT THE LANE", PAL_ALERT);
    if (mode_ == Mode::Win) textC(14, "THE LEG IS HELD", PAL_WIN);
}

}  // namespace cranelane
