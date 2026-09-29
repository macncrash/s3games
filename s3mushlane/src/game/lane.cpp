#include "lane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mushlane {
namespace {

constexpr int HORIZON = 78;
constexpr float Z_NEAR = 4.2f;
constexpr float Z_TEAM = 8.1f;
constexpr float PPM_NEAR = 48.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float LANE = 1.9f;
constexpr float SPEED = 20.4f;
constexpr float AUTH = 8.6f;
constexpr float COURSE = 448.f;
constexpr float CREW = 27.5f;
constexpr float DT = 1.f / 60.f;

float drift(float d) {
    return 2.85f * std::sin(d * 0.051f) + 1.25f * std::sin(d * 0.133f + 1.1f);
}

int fogFor(float ahead) {
    return int(std::clamp((ahead - 6.f) * 0.18f, 0.f, 12.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 55.f) return 3;
    if (std::fabs(x_) > LANE * 0.7f) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 26.f * std::sin(s * 0.019f) + 8.f * std::sin(s * 0.053f + 0.4f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.1f, 0.16f, 0.06f);
    if (bot_) {
        begin();
    } else {
        toTitle();
    }
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = race_ = 0;
    scenery_ = 36.f;
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
    sys_->apu.keyOn(0, 196.f, 0.1f);
    sys_->rumble(0.08f, 0.03f, 60);
}

void Game::finish(bool good, const char* why) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = why;
    if (good) {
        sys_->apu.keyOn(1, 523.25f, 0.15f);
        sys_->apu.keyOn(2, 659.25f, 0.12f);
        sys_->setLight(80, 180, 200);
        sys_->rumble(0.12f, 0.28f, 120);
    } else {
        sys_->apu.keyOn(1, 130.8f, 0.16f);
        sys_->setLight(180, 40, 30);
        sys_->rumble(0.4f, 0.08f, 160);
    }
}

void Game::update(float dt) {
    float steer = 0;
    const gs::Pad& pad = sys_->pad;
    if (bot_) {
        steer = drift(dist_) / AUTH - x_ * 1.8f - vx_ * 0.85f;
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
    }
    steer = std::clamp(steer, -1.f, 1.f);
    vx_ += (steer * AUTH - drift(dist_)) * dt;
    vx_ *= std::exp(-3.6f * dt);
    x_ += vx_ * dt;
    dist_ += SPEED * dt;
    race_ += dt;
    if (std::fabs(x_) > LANE) finish(false, "left the lane");
    else if (race_ >= CREW) finish(false, "the other crew");
    else if (dist_ >= COURSE) finish(true, "held");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += SPEED * 0.55f * DT;
        if (scenery_ > COURSE) scenery_ = 24.f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        toTitle();
    }
    draw();
    audio();
}

void Game::audio() {
    if (mode_ == Mode::Run) {
        runner_ += DT;
        float wob = 1.f + 0.04f * std::sin(runner_ * 11.f);
        sys_->apu.tone(0, 92.f * wob, 0.035f);
        sys_->apu.noise(0.018f, 1400.f, false);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = Z_TEAM + ahead;
    if (z < 2.4f) return p;
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

void Game::trail(float view) {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(view * 8.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(7 + u * 5), int(9 + u * 4), int(12 + u * 2));
            v.lineFog[y] = uint8_t((1.f - u) * 4);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_TEAM;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(view + ahead);
        rd.hw = std::max(4.f, LANE * ppm);
        rd.v = (view + ahead) * 34.f;
        rd.pal = PAL_ROAD;
        rd.style = gs::ROAD_SNOW;
        rd.band = (int(std::floor((view + ahead) / 7.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 8);
        v.lineBackdrop[y] = gs::rgb4(10, 11, 12);
    }
}

void Game::roadside(float along) {
    auto row = [&](float step, float from, float to, auto place) {
        float d0 = std::floor((along - 6.f) / step) * step;
        for (float m = d0; m < along + 110.f; m += step) {
            if (m < from || m > to) continue;
            place(m, m - along);
        }
    };
    row(16.f, -8.f, COURSE + 20.f, [&](float m, float ahead) {
        for (int side = -1; side <= 1; side += 2) {
            float wx = side * (LANE + 2.6f + ((int(m / 16.f) & 1) ? 0.8f : 0.f));
            Proj p = project(wx, ahead);
            if (!p.ok) continue;
            spr(art_.pine, p.x, p.y, p.ppm * 4.8f, PAL_PINE, side < 0, p.fog);
        }
    });
    row(11.f, 0.f, COURSE, [&](float m, float ahead) {
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (LANE + 0.35f), ahead);
            if (!p.ok) continue;
            spr(art_.stake, p.x, p.y, p.ppm * 1.35f, PAL_STAKE, side < 0, p.fog);
        }
    });
    Proj arch = project(0.f, COURSE - along);
    if (arch.ok) spr(art_.arch, arch.x, arch.y - arch.ppm * 1.1f, arch.ppm * 2.4f, PAL_BANNER, false, arch.fog);
}

void Game::teamAt() {
    float t = Z_NEAR / Z_TEAM;
    float ppm = PPM_NEAR * t;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * ppm;
    float bob = std::sin(t_ * 14.f) * 1.1f;
    spr(art_.dogs, x, y - 18.f + bob, 46.f, PAL_TEAM);
    spr(art_.sled, x, y + 10.f + bob * 0.4f, 28.f, PAL_TEAM);
    spr(art_.shade, x, y + 18.f, 12.f, PAL_TEAM, false, 0, true);
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
    trail(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 34, float(art_.title.h), PAL_BANNER);
        spr(art_.stay, 160, 58, float(art_.stay.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 86, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.held, 160, 38, float(art_.held.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(std::strcmp(why_, "the other crew") == 0 ? art_.late : art_.left, 160, 38,
            float(art_.left.h), PAL_ALERT);
    }
    roadside(view);
    teamAt();

    if (mode_ == Mode::Title) {
        textC(23, "ARROWS STEER THE MUSH", PAL_HUD);
        textC(24, "BEAT THE OTHER CREW", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_BANNER);
        return;
    }
    float leftT = CREW - race_;
    if (leftT < 0.f) leftT = 0.f;
    if (mode_ == Mode::Win) leftT = CREW - race_;
    int cs = int(leftT * 100) % 100;
    int sec = int(leftT);
    if (cs < 0) cs = 0;
    char clock[24], metres[16];
    std::snprintf(clock, sizeof clock, "CREW %d.%02d", sec, cs);
    int left = int(std::ceil(COURSE - dist_));
    if (left < 0 || mode_ == Mode::Win) left = 0;
    std::snprintf(metres, sizeof metres, "%d M", left);
    text(1, 0, metres, PAL_HUD);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, leftT < 6.f ? PAL_ALERT : PAL_HUD);

    char bar[22];
    const int n = 19;
    int mid = n / 2;
    int pos = mid + int(std::lround((x_ / LANE) * (mid - 1)));
    pos = std::clamp(pos, 0, n - 1);
    for (int i = 0; i < n; i++) bar[i] = (i == pos) ? '+' : '-';
    bar[0] = '<';
    bar[n - 1] = '>';
    bar[n] = 0;
    int pal = std::fabs(x_) > LANE * 0.7f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    if (mode_ == Mode::Run && race_ < 2.2f) textC(3, "HOLD THE LANE", PAL_HUD);
    if (mode_ == Mode::Fail) textC(14, why_, PAL_ALERT);
}

}  // namespace mushlane
