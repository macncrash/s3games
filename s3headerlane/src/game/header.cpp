#include "header.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace headerlane {
namespace {

constexpr int HORIZON = 78;
constexpr float Z_NEAR = 4.0f;
constexpr float Z_BOAT = 7.2f;
constexpr float PPM_NEAR = 50.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float LANE = 2.15f;
constexpr float AUTH = 9.4f;
constexpr float COURSE = 520.f;
constexpr float HEADER_AT = 148.f;
constexpr float WINDOW = 78.f;
constexpr float CREW = 36.f;
constexpr float DT = 1.f / 60.f;

int fogFor(float ahead) {
    return int(std::clamp((ahead - 10.f) * 0.18f, 0.f, 13.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > COURSE - 70.f) return 3;
    if (!taken_ && dist_ >= HEADER_AT && dist_ <= HEADER_AT + WINDOW) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 26.f * std::sin(s * 0.013f) + 8.f * std::sin(s * 0.041f + 1.1f);
}

float Game::chopAt(float s) const {
    return 2.15f * std::sin(s * 0.057f) + 1.05f * std::sin(s * 0.139f + 0.6f);
}

float Game::windAt(float s) const {
    float w = chopAt(s);
    if (early_) return w + 16.f;
    if (!taken_) {
        if (s > HEADER_AT) {
            float into = std::min(s - HEADER_AT, WINDOW + 40.f);
            w += 5.5f + into * 0.22f;
        }
        return w;
    }
    if (settle_ > 0.f) w -= 2.4f * (settle_ / 0.5f);
    return w;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.16f, 0.22f, 0.1f);
    toTitle();
    over_ = false;
    won_ = false;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = race_ = settle_ = heel_ = 0;
    scenery_ = 30.f;
    taken_ = false;
    early_ = false;
    why_ = "";
    won_ = false;
    over_ = false;
    tackFlash_ = 0;
}

void Game::begin() {
    dist_ = x_ = vx_ = race_ = settle_ = 0;
    taken_ = false;
    early_ = false;
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
        sys_->apu.keyOn(1, 523.25f, 0.16f);
        sys_->apu.keyOn(2, 659.25f, 0.13f);
        sys_->setLight(40, 160, 90);
        sys_->rumble(0.12f, 0.32f, 150);
    } else {
        sys_->apu.keyOn(1, 130.8f, 0.16f);
        sys_->setLight(160, 30, 24);
        sys_->rumble(0.4f, 0.08f, 180);
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    bool tack = false;
    float steer = 0;
    if (bot_) {
        float w = windAt(dist_);
        steer = w / AUTH - x_ * 1.85f - vx_ * 0.95f;
        if (!taken_ && !early_ && dist_ >= HEADER_AT + 6.f && dist_ < HEADER_AT + WINDOW - 8.f) tack = true;
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) steer = pad.axisX;
        tack = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z);
    }
    steer = std::clamp(steer, -1.f, 1.f);

    if (tack && !taken_ && !early_) {
        if (dist_ < HEADER_AT) {
            early_ = true;
            tackFlash_ = 40;
            sys_->apu.keyOn(3, 110.f, 0.14f);
        } else if (dist_ <= HEADER_AT + WINDOW) {
            taken_ = true;
            settle_ = 0.5f;
            tackFlash_ = 28;
            sys_->apu.keyOn(3, 392.f, 0.14f);
            sys_->rumble(0.2f, 0.15f, 90);
        }
    }

    float wind = windAt(dist_);
    vx_ += (steer * AUTH - wind) * dt;
    vx_ *= std::exp(-3.1f * dt);
    x_ += vx_ * dt;
    heel_ += (std::clamp(-vx_ * 0.35f, -1.f, 1.f) - heel_) * std::min(1.f, 6.f * dt);
    if (settle_ > 0.f) settle_ -= dt;

    float spd = taken_ ? 20.4f : 16.2f;
    if (!taken_ && dist_ > HEADER_AT) spd = 11.4f;
    if (early_) spd = 10.f;
    dist_ += spd * dt;
    race_ += dt;

    if (std::fabs(x_) > LANE) {
        if (early_) finish(false, "tacked off the header");
        else if (!taken_) finish(false, "sailed the header");
        else finish(false, "left the lane");
        return;
    }
    if (!taken_ && !early_ && dist_ > HEADER_AT + WINDOW + 36.f) {
        finish(false, "sailed the header");
        return;
    }
    if (dist_ >= COURSE) {
        if (!taken_) finish(false, "sailed the header");
        else if (race_ >= CREW) finish(false, "the other crew kept the clock");
        else finish(true, "took the header");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += 15.f * DT;
        if (scenery_ > COURSE * 0.5f) scenery_ = 20.f;
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
    if (tackFlash_ > 0) tackFlash_--;
    draw();
    audio();
}

void Game::audio() {
    if (mode_ == Mode::Run) {
        float wash = 0.03f + 0.012f * std::sin(t_ * 7.f);
        sys_->apu.tone(0, 62.f + 4.f * std::sin(t_ * 3.1f), 0.04f);
        sys_->apu.noise(wash, 700.f, false);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.noise(0, 0, false);
    }
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = Z_BOAT + ahead;
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

void Game::sea(float view) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            int r = int(6 + (1.f - u) * 6);
            int g = int(8 + (1.f - u) * 4);
            int b = int(12 + u * 3);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t((1.f - u) * 4);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_BOAT;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(view + ahead);
        rd.hw = std::max(4.f, LANE * ppm);
        rd.v = (view + ahead) * 42.f;
        rd.pal = PAL_SEA;
        rd.style = 2;
        rd.band = (int(std::floor((view + ahead) / 7.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        float fogT = std::clamp((0.24f - t) / 0.24f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 9);
        v.lineBackdrop[y] = gs::rgb4(1, 4, 7);
    }
}

void Game::course(float along) {
    auto row = [&](float step, float from, float to, auto place) {
        float d0 = std::floor((along - 6.f) / step) * step;
        for (float m = d0; m < along + 110.f; m += step) {
            if (m < from || m > to) continue;
            place(m, m - along);
        }
    };
    row(26.f, 0.f, COURSE - 8.f, [&](float m, float ahead) {
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (LANE + 0.55f), ahead);
            if (!p.ok) continue;
            spr(art_.buoy, p.x, p.y, p.ppm * 1.35f, PAL_BUOY, side < 0, p.fog);
        }
    });
    Proj mark = project(0.f, COURSE - along);
    if (mark.ok) {
        spr(art_.mark, mark.x - mark.ppm * 0.2f, mark.y, mark.ppm * 3.4f, PAL_MARK, false, mark.fog);
    }
    if (mode_ == Mode::Run && !taken_) {
        float ghost = CREW > 0.f ? (race_ / CREW) * COURSE : 0.f;
        float ahead = ghost - along;
        if (ahead > 4.f && ahead < 90.f) {
            Proj g = project(0.35f, ahead);
            if (g.ok) {
                spr(art_.sail, g.x, g.y - g.ppm * 0.4f, g.ppm * 2.2f, PAL_ALERT, false, g.fog + 2);
            }
        }
    }
}

void Game::boatAt() {
    float t = Z_NEAR / Z_BOAT;
    float ppm = PPM_NEAR * t;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * ppm;
    float bob = std::sin(t_ * 5.5f) * 1.4f;
    bool flip = heel_ < -0.08f;
    float lean = heel_ * 10.f;
    spr(art_.shade, x, y + 18.f, 14.f, PAL_HULL, false, 0, true);
    spr(art_.hull, x + lean * 0.3f, y + 10.f + bob, 36.f, PAL_HULL, flip);
    spr(art_.sail, x + lean, y - 6.f + bob, 78.f, PAL_SAIL, flip);
    spr(art_.jib, x + lean * 0.6f + (flip ? 10.f : -10.f), y + bob, 42.f, PAL_SAIL, flip);
    if (mode_ == Mode::Run && !taken_ && dist_ + 18.f >= HEADER_AT && dist_ <= HEADER_AT + WINDOW) {
        spr(art_.flag, x + 28.f, y - 52.f + bob, 18.f, PAL_ALERT, int(t_ * 8.f) & 1);
    }
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
    sea(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 34, float(art_.title.h), PAL_SIGN);
        spr(art_.sub, 160, 58, float(art_.sub.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 86, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.took, 160, 36, float(art_.took.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.missed, 160, 36, float(art_.missed.h), PAL_ALERT);
    } else if (marker() == 2 && (int(t_ * 6.f) & 1)) {
        spr(art_.header, 160, 40, float(art_.header.h), PAL_ALERT);
    }
    course(view);
    boatAt();

    if (mode_ == Mode::Title) {
        textC(23, "ARROWS STEER THE BOAT", PAL_HUD);
        textC(24, "A TAKES THE HEADER", PAL_HUD);
        textC(25, "BEAT THE OTHER CREW", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_SIGN);
        return;
    }

    int left = int(std::ceil(COURSE - dist_));
    if (left < 0) left = 0;
    if (mode_ == Mode::Win) left = 0;
    char m[24], clock[24], crew[20];
    std::snprintf(m, sizeof m, "%d M", mode_ == Mode::Fail ? int(dist_ + 0.5f) : left);
    int cs = int(race_ * 100) % 100;
    int sec = int(race_) % 60;
    int min = int(race_) / 60;
    if (cs < 0) cs = 0;
    std::snprintf(clock, sizeof clock, "%d:%02d.%02d", min, sec, cs);
    std::snprintf(crew, sizeof crew, "CREW 0:%02.0f", CREW);
    text(1, 0, m, PAL_HUD);
    text(12, 0, crew, PAL_SIGN);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, race_ + 4.f > CREW && mode_ == Mode::Run ? PAL_ALERT : PAL_HUD);

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
    if (mode_ == Mode::Run && race_ < 2.2f) textC(3, "STAY IN THE LANE", PAL_HUD);
    if (mode_ == Mode::Run && marker() == 2) textC(3, "TAKE THE HEADER", PAL_ALERT);
    if (mode_ == Mode::Run && taken_ && tackFlash_ > 0) textC(3, "LIFTED", PAL_WIN);
    if (mode_ == Mode::Fail && why_) textC(14, why_, PAL_ALERT);
    if (mode_ == Mode::Win) textC(14, "UNDER THE OTHER CREW", PAL_WIN);
}

}  // namespace headerlane
