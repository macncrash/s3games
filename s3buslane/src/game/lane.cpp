#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace buslane {
namespace {

constexpr int HORIZON = 82;
constexpr float Z_NEAR = 3.8f;
constexpr float Z_BUS = 7.6f;
constexpr float PPM_NEAR = 52.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float LANE = 1.95f;
constexpr float SPEED = 17.4f;
constexpr float AUTH = 8.4f;
constexpr float COURSE = 420.f;
constexpr float DT = 1.f / 60.f;

int fogFor(float ahead) {
    return int(std::clamp((ahead - 8.f) * 0.16f, 0.f, 12.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 48.f) return 3;
    if (std::fabs(x_) > LANE * 0.68f) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 22.f * std::sin(s * 0.012f) + 11.f * std::sin(s * 0.033f + 0.7f);
}

float Game::shove(float d) const {
    return 2.4f * std::sin(d * 0.029f) + 1.35f * std::sin(d * 0.073f + 0.4f) + 0.55f * std::sin(d * 0.16f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.14f, 0.05f);
    toTitle();
    over_ = false;
    won_ = false;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = race_ = 0;
    scenery_ = 24.f;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::begin() {
    dist_ = x_ = vx_ = race_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    mode_ = Mode::Run;
    sys_->apu.keyOn(0, 146.f, 0.09f);
    sys_->rumble(0.1f, 0.04f, 70);
}

void Game::finish(bool good) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = good ? "held" : "left the lane";
    if (good) {
        sys_->apu.keyOn(1, 349.23f, 0.14f);
        sys_->apu.keyOn(2, 440.f, 0.11f);
        sys_->setLight(40, 150, 30);
        sys_->rumble(0.1f, 0.26f, 130);
    } else {
        sys_->apu.keyOn(1, 98.f, 0.16f);
        sys_->setLight(170, 30, 12);
        sys_->rumble(0.42f, 0.06f, 170);
    }
}

void Game::update(float dt) {
    float steer = 0;
    const gs::Pad& pad = sys_->pad;
    if (bot_) {
        steer = shove(dist_) / AUTH - x_ * 2.05f - vx_ * 1.15f;
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
    }
    steer = std::clamp(steer, -1.f, 1.f);
    vx_ += (steer * AUTH - shove(dist_)) * dt;
    vx_ *= std::exp(-2.7f * dt);
    x_ += vx_ * dt;
    dist_ += SPEED * dt;
    race_ += dt;
    if (std::fabs(x_) > LANE) finish(false);
    else if (dist_ >= COURSE) finish(true);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += SPEED * DT * 0.45f;
        if (scenery_ > COURSE) scenery_ = 16.f;
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

void Game::audio() {
    if (mode_ == Mode::Run) {
        hum_ += DT;
        float wob = 1.f + 0.015f * std::sin(hum_ * 28.f);
        sys_->apu.tone(0, 55.f * wob, 0.05f);
        sys_->apu.noise(0.01f, 520.f, false);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = Z_BUS + ahead;
    if (z < 2.0f) return p;
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
            int r = int(7 + (1.f - u) * 3);
            int g = int(8 + (1.f - u) * 2);
            int b = int(10 + u * 3);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(u * 4);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_BUS;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(view + ahead);
        rd.hw = std::max(5.f, LANE * ppm);
        rd.v = (view + ahead) * 30.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (int(std::floor((view + ahead) / 6.5f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 8);
        v.lineBackdrop[y] = gs::rgb4(3, 3, 3);
    }
}

void Game::street(float along) {
    auto row = [&](float step, float from, float to, auto place) {
        float d0 = std::floor((along - 8.f) / step) * step;
        for (float m = d0; m < along + 110.f; m += step) {
            if (m < from || m > to) continue;
            place(m, m - along);
        }
    };
    row(32.f, 0.f, COURSE - 30.f, [&](float m, float ahead) {
        (void)m;
        Proj p = project(LANE + 2.6f, ahead);
        if (p.ok) spr(art_.shelter, p.x, p.y, p.ppm * 3.8f, PAL_SHELTER, false, p.fog);
    });
    row(18.f, -4.f, COURSE, [&](float m, float ahead) {
        (void)m;
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (LANE + 1.15f), ahead);
            if (!p.ok) continue;
            spr(art_.lamp, p.x, p.y, p.ppm * 2.8f, PAL_LAMP, side < 0, p.fog);
        }
    });
    row(22.f, 10.f, COURSE - 40.f, [&](float m, float ahead) {
        float pass = std::fmod(m * 0.37f + t_ * 9.f, 18.f) - 4.f;
        Proj p = project(-(LANE + 4.2f), ahead - pass);
        if (p.ok) spr(art_.car, p.x, p.y, p.ppm * 1.5f, PAL_CAR, false, p.fog);
    });
    row(11.f, 4.f, COURSE - 8.f, [&](float m, float ahead) {
        if ((int(std::floor(m)) & 1) == 0) return;
        Proj p = project(LANE + 0.35f, ahead);
        if (p.ok) spr(art_.cone, p.x, p.y, p.ppm * 0.85f, PAL_DEPOT, false, p.fog);
    });
    Proj gate = project(0.f, COURSE - along);
    if (gate.ok) spr(art_.depot, gate.x, gate.y - gate.ppm * 0.6f, gate.ppm * 2.8f, PAL_DEPOT, false, gate.fog);
}

void Game::busAt() {
    float t = Z_NEAR / Z_BUS;
    float ppm = PPM_NEAR * t;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * ppm;
    float bob = std::sin(t_ * 11.f) * 0.6f;
    spr(art_.bus, x, y + 8.f + bob, 62.f, PAL_BUS);
    spr(art_.stripe, x, y - 18.f + bob, 5.f, PAL_ALERT);
    spr(art_.shade, x, y + 16.f, 12.f, PAL_BUS, false, 0, true);
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
        spr(art_.title, 160, 32, float(art_.title.h), PAL_DEPOT);
        spr(art_.stay, 160, 58, float(art_.stay.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 86, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.held, 160, 36, float(art_.held.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.left, 160, 36, float(art_.left.h), PAL_ALERT);
    }
    street(view);
    busAt();

    if (mode_ == Mode::Title) {
        textC(24, "ARROWS STEER THE BUS", PAL_HUD);
        textC(25, "HOLD THE LANE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_DEPOT);
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
    int pos = mid + int(std::lround((x_ / LANE) * (mid - 1)));
    pos = std::clamp(pos, 0, n - 1);
    for (int i = 0; i < n; i++) bar[i] = (i == pos) ? '+' : '-';
    bar[0] = '<';
    bar[n - 1] = '>';
    bar[n] = 0;
    int pal = std::fabs(x_) > LANE * 0.68f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    if (mode_ == Mode::Run && std::fabs(x_) > LANE * 0.68f) textC(2, "LANE", PAL_ALERT);
    if (mode_ == Mode::Run && race_ < 2.4f) textC(3, "STAY IN THE LANE", PAL_HUD);
    if (mode_ == Mode::Fail) textC(14, "LEFT THE LANE", PAL_ALERT);
    if (mode_ == Mode::Win) textC(14, "LEG HELD", PAL_WIN);
}

}  // namespace buslane
