#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cablane {
namespace {

constexpr int HORIZON = 72;
constexpr float Z_NEAR = 4.0f;
constexpr float Z_CAB = 7.4f;
constexpr float PPM_NEAR = 52.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float LANE = 2.25f;
constexpr float SPEED = 17.5f;
constexpr float AUTH = 8.2f;
constexpr float COURSE = 420.f;
constexpr float DT = 1.f / 60.f;

float shove(float d) {
    return 3.35f * std::sin(d * 0.046f) + 1.55f * std::sin(d * 0.121f + 0.7f);
}

int fogFor(float ahead) {
    return int(std::clamp((ahead - 8.f) * 0.2f, 0.f, 13.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 48.f) return 3;
    if (std::fabs(x_) > LANE * 0.72f) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 32.f * std::sin(s * 0.017f) + 10.f * std::sin(s * 0.047f + 0.8f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.12f, 0.18f, 0.08f);
    toTitle();
    over_ = false;
    won_ = false;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = race_ = 0;
    scenery_ = 40.f;
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
    sys_->apu.keyOn(0, 220.f, 0.12f);
    sys_->rumble(0.12f, 0.04f, 70);
}

void Game::finish(bool good) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = good ? "held" : "left the lane";
    if (good) {
        sys_->apu.keyOn(1, 523.25f, 0.16f);
        sys_->apu.keyOn(2, 659.25f, 0.14f);
        sys_->setLight(40, 180, 70);
        sys_->rumble(0.15f, 0.35f, 140);
    } else {
        sys_->apu.keyOn(1, 146.8f, 0.16f);
        sys_->setLight(180, 30, 20);
        sys_->rumble(0.45f, 0.1f, 180);
    }
}

void Game::update(float dt) {
    float steer = 0;
    const gs::Pad& pad = sys_->pad;
    if (bot_) {
        steer = shove(dist_) / AUTH - x_ * 1.65f - vx_ * 0.9f;
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
    }
    steer = std::clamp(steer, -1.f, 1.f);
    vx_ += (steer * AUTH - shove(dist_)) * dt;
    vx_ *= std::exp(-3.4f * dt);
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
        scenery_ += SPEED * DT;
        if (scenery_ > COURSE) scenery_ = 20.f;
        if (pad.pressed(gs::BTN_START) || (bot_ && t_ > 0.45f)) begin();
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
        float wob = 1.f + 0.03f * std::sin(engine_ * 48.f);
        sys_->apu.tone(0, 78.f * wob, 0.05f);
        sys_->apu.noise(0.012f, 900.f, false);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = Z_CAB + ahead;
    if (z < 2.2f) return p;
    float t = Z_NEAR / z;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON - 8 || p.y >= gs::SCREEN_H + 20) return p;
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
            int r = int(3 + (1.f - u) * 6);
            int g = int(3 + (1.f - u) * 3);
            int b = int(7 + u * 3);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(u * 6);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_CAB;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(view + ahead);
        rd.hw = std::max(3.f, LANE * ppm);
        rd.v = (view + ahead) * 38.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (int(std::floor((view + ahead) / 6.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 10);
        v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
    }
}

void Game::street(float along) {
    auto row = [&](float step, float from, float to, auto place) {
        float d0 = std::floor((along - 8.f) / step) * step;
        for (float m = d0; m < along + 95.f; m += step) {
            if (m < from || m > to) continue;
            place(m, m - along);
        }
    };
    row(22.f, -10.f, COURSE + 30.f, [&](float m, float ahead) {
        int kind = int(std::floor(std::fabs(m) * 0.17f)) % 3;
        bool flip = (int(m) & 1) != 0;
        for (int side = -1; side <= 1; side += 2) {
            float wx = side * (LANE + 3.4f + float((int(m / 22.f) & 1) ? 0.6f : 0.f));
            Proj p = project(wx, ahead);
            if (!p.ok) continue;
            float h = p.ppm * (kind == 2 ? 5.2f : 4.4f);
            spr(art_.block[kind], p.x, p.y, h, PAL_BLOCK, flip, p.fog);
        }
    });
    row(18.f, 0.f, COURSE, [&](float m, float ahead) {
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (LANE + 1.15f), ahead);
            if (!p.ok) continue;
            spr(art_.lamp, p.x, p.y, p.ppm * 2.3f, PAL_LAMP, side < 0, p.fog);
        }
    });
    row(28.f, 12.f, COURSE - 20.f, [&](float m, float ahead) {
        int side = (int(m / 28.f) & 1) ? 1 : -1;
        Proj p = project(side * (LANE + 1.55f), ahead);
        if (!p.ok) return;
        spr(art_.park, p.x, p.y - p.ppm * 0.15f, p.ppm * 1.15f, PAL_PARK, side < 0, p.fog);
    });
    for (int mark = 100; mark < int(COURSE); mark += 100) {
        Proj p = project(-(LANE + 0.35f), float(mark) - along);
        if (!p.ok) continue;
        spr(art_.meter, p.x, p.y, p.ppm * 1.5f, PAL_SIGN, false, p.fog);
    }
    Proj gate = project(0.f, COURSE - along);
    if (gate.ok) spr(art_.gate, gate.x, gate.y - gate.ppm * 1.3f, gate.ppm * 2.6f, PAL_SIGN, false, gate.fog);
}

void Game::cabAt() {
    float t = Z_NEAR / Z_CAB;
    float ppm = PPM_NEAR * t;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * ppm;
    float bob = std::sin(t_ * 18.f) * 0.6f;
    spr(art_.cab, x, y + 8.f + bob, 70.f, PAL_CAB);
    spr(art_.shade, x, y + 16.f, 16.f, PAL_CAB, false, 0, true);
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
        spr(art_.title, 160, 36, float(art_.title.h), PAL_SIGN);
        spr(art_.stay, 160, 62, float(art_.stay.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 92, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.held, 160, 40, float(art_.held.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.left, 160, 40, float(art_.left.h), PAL_ALERT);
    }
    street(view);
    if (mode_ != Mode::Title) cabAt();
    else cabAt();

    if (mode_ == Mode::Title) {
        textC(24, "ARROWS STEER", PAL_HUD);
        textC(25, "THE CAB DRIVES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_SIGN);
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
    int pal = std::fabs(x_) > LANE * 0.72f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    if (mode_ == Mode::Run && std::fabs(x_) > LANE * 0.72f) textC(2, "LANE", PAL_ALERT);
    if (mode_ == Mode::Run && race_ < 2.4f) textC(3, "STAY IN THE LANE", PAL_HUD);
    if (mode_ == Mode::Fail) textC(14, "LEFT THE LANE", PAL_ALERT);
}

}  // namespace cablane
