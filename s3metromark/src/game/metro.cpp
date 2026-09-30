#include "metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace metromark {
namespace {

constexpr int HORIZON = 86;
constexpr float Z_NEAR = 4.4f;
constexpr float Z_TRAIN = 9.2f;
constexpr float PPM_NEAR = 46.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float HALF = 2.15f;
constexpr float ACCEL = 5.6f;
constexpr float BRAKE = 8.4f;
constexpr float VMAX = 16.2f;
constexpr float MARK = 142.f;
constexpr float CREW = 15.4f;
constexpr float WIN_ERR = 1.75f;
constexpr float STOP_V = 0.42f;
constexpr float DT = 1.f / 60.f;

int fogFor(float ahead) {
    return int(std::clamp((ahead - 8.f) * 0.16f, 0.f, 13.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (v_ < 2.4f && std::fabs(s_ - MARK) < 8.f) return 3;
    if (MARK - s_ < 38.f) return 2;
    return 1;
}

float Game::bendAt(float s) const {
    return 18.f * std::sin(s * 0.021f) + 6.f * std::sin(s * 0.053f + 0.7f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.08f);
    mark_ = MARK;
    toTitle();
    over_ = false;
    won_ = false;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    s_ = v_ = used_ = 0;
    clock_ = CREW;
    scenery_ = 18.f;
    why_ = "";
    won_ = false;
    over_ = false;
    braking_ = false;
}

void Game::begin() {
    s_ = v_ = used_ = 0;
    clock_ = CREW;
    won_ = false;
    over_ = false;
    why_ = "";
    braking_ = false;
    mode_ = Mode::Run;
    sys_->apu.keyOn(0, 98.f, 0.08f);
    sys_->rumble(0.06f, 0.02f, 80);
}

void Game::finish(bool good, const char* why) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = why;
    v_ = 0;
    if (good) {
        sys_->apu.keyOn(1, 392.f, 0.14f);
        sys_->apu.keyOn(2, 523.25f, 0.11f);
        sys_->setLight(40, 150, 70);
        sys_->rumble(0.1f, 0.22f, 140);
    } else {
        sys_->apu.keyOn(1, 98.f, 0.16f);
        sys_->setLight(150, 28, 18);
        sys_->rumble(0.35f, 0.06f, 160);
    }
}

void Game::update(float dt) {
    if (mode_ != Mode::Run) return;
    used_ += dt;
    clock_ -= dt;

    bool throttle = false;
    bool brake = false;
    if (bot_) {
        float remain = MARK - s_;
        float sd = (v_ * v_) / (2.f * BRAKE);
        if (v_ < STOP_V && std::fabs(remain) <= WIN_ERR) {
            brake = v_ > 0.04f;
        } else if (remain <= sd + 0.22f) {
            brake = true;
        } else {
            throttle = true;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        throttle = pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f;
        brake = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.2f;
    }
    if (brake) throttle = false;
    braking_ = brake;

    float a = 0;
    if (throttle) a += ACCEL;
    if (brake) a -= BRAKE;
    if (!throttle && !brake) a -= 0.55f;
    v_ += a * dt;
    if (v_ < 0) v_ = 0;
    if (v_ > VMAX) v_ = VMAX;
    s_ += v_ * dt;

    if (v_ < STOP_V && std::fabs(s_ - MARK) <= WIN_ERR) {
        s_ = std::clamp(s_, MARK - WIN_ERR, MARK + WIN_ERR);
        finish(true, "on the mark");
        return;
    }
    if (v_ < 0.18f && s_ > 12.f && s_ < MARK - WIN_ERR) {
        finish(false, "short of the mark");
        return;
    }
    if (s_ > MARK + WIN_ERR + 1.1f) {
        finish(false, "past the mark");
        return;
    }
    if (clock_ <= 0.f) {
        clock_ = 0;
        finish(false, "the other crew took the mark");
    }
}

void Game::audio() {
    if (mode_ == Mode::Title) {
        if (hum_ <= 0) {
            sys_->apu.tone(0, 146.f, 0.03f);
            hum_ = 0.8f;
        }
        hum_ -= DT;
        return;
    }
    if (mode_ != Mode::Run) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    float rate = 70.f + v_ * 9.f;
    sys_->apu.tone(0, rate, 0.035f + v_ * 0.003f);
    if (braking_ && v_ > 1.f) sys_->apu.noise(0.04f, 0.35f, false);
    else sys_->apu.noise(0, 0, false);
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    if (ahead < 0.6f) return p;
    float t = Z_NEAR / (ahead + Z_NEAR);
    if (t < 0.012f) return p;
    p.ppm = PPM_NEAR * t;
    p.y = HORIZON + t * SPAN;
    p.x = 160.f + bendAt(s_ + ahead) + wx * p.ppm;
    p.fog = fogFor(ahead);
    p.ok = p.y > HORIZON - 4 && p.y < gs::SCREEN_H + 30;
    return p;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (h < 1.5f) return;
    float sc = h / float(m.h);
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.w = int(std::lround(m.w * sc));
    sp.h = int(std::lround(h));
    if (sp.w < 1 || sp.h < 1) return;
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(cy - sp.h));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sp.shadow = shadow;
    sys_->vdp.sprite(sp);
}

void Game::tunnel(float view) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(1, 1, int(2 + u * 2));
            v.lineFog[y] = uint8_t(3 + u * 4);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_TRAIN;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendAt(view + ahead);
        rd.hw = std::max(6.f, HALF * ppm);
        rd.v = (view + ahead) * 40.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (int(std::floor((view + ahead) / 6.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 10);
        v.lineBackdrop[y] = gs::rgb4(1, 1, 2);
    }
}

void Game::station(float along) {
    auto place = [&](float m, auto fn) {
        float ahead = m - along;
        if (ahead < -2.f || ahead > 90.f) return;
        fn(ahead);
    };
    for (float m = 8.f; m < MARK + 36.f; m += 11.f) {
        place(m, [&](float ahead) {
            for (int side = -1; side <= 1; side += 2) {
                Proj p = project(side * (HALF + 1.35f), ahead);
                if (!p.ok) continue;
                spr(art_.arch, p.x, p.y, p.ppm * 3.4f, PAL_ARCH, side < 0, p.fog);
            }
        });
    }
    for (float m = MARK - 28.f; m <= MARK + 22.f; m += 6.f) {
        place(m, [&](float ahead) {
            Proj edge = project(HALF + 0.55f, ahead);
            if (edge.ok) spr(art_.plat, edge.x, edge.y, edge.ppm * 0.55f, PAL_PLAT, false, edge.fog);
            Proj stripe = project(HALF + 0.15f, ahead);
            if (stripe.ok)
                spr(art_.stripe, stripe.x, stripe.y - stripe.ppm * 0.15f, stripe.ppm * 0.16f, PAL_MARK, false,
                    stripe.fog);
        });
    }
    for (float m = MARK - 22.f; m <= MARK + 16.f; m += 10.f) {
        place(m, [&](float ahead) {
            Proj lamp = project(HALF + 1.55f, ahead);
            if (lamp.ok) spr(art_.lamp, lamp.x, lamp.y - lamp.ppm * 1.4f, lamp.ppm * 1.6f, PAL_LAMP, false, lamp.fog);
            Proj bench = project(HALF + 1.9f, ahead + 2.f);
            if (bench.ok) spr(art_.bench, bench.x, bench.y, bench.ppm * 0.7f, PAL_PLAT, false, bench.fog);
        });
    }
    Proj mark = project(HALF + 0.35f, MARK - along);
    if (mark.ok) spr(art_.mark, mark.x, mark.y - mark.ppm * 0.2f, mark.ppm * 1.55f, PAL_MARK, false, mark.fog);
}

void Game::trainAt() {
    float t = Z_NEAR / Z_TRAIN;
    float ppm = PPM_NEAR * t;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : s_;
    float x = 160.f + bendAt(along);
    float bob = (mode_ == Mode::Run) ? std::sin(t_ * (8.f + v_ * 0.4f)) * 0.6f : 0.f;
    spr(art_.metro, x, y + 8.f + bob, 52.f, PAL_METRO);
    spr(art_.pant, x - 8.f, y - 22.f + bob, 12.f, PAL_LAMP);
    spr(art_.pant, x + 10.f, y - 22.f + bob, 12.f, PAL_LAMP, true);
    spr(art_.shade, x, y + 16.f, 12.f, PAL_METRO, false, 0, true);
    (void)ppm;
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
    float view = (mode_ == Mode::Title) ? scenery_ : s_;
    tunnel(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 36, float(art_.title.h), PAL_MARK);
        spr(art_.crew, 160, 62, float(art_.crew.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 90, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.onmark, 160, 40, float(art_.onmark.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.missed, 160, 40, float(art_.missed.h), PAL_ALERT);
    }
    station(view);
    trainAt();

    if (mode_ == Mode::Title) {
        textC(24, "A THROTTLE   B BRAKE", PAL_HUD);
        textC(25, "SET DOWN ON THE MARK", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_MARK);
        return;
    }
    float left = MARK - s_;
    char dist[24], clock[24];
    std::snprintf(dist, sizeof dist, "MARK %+.1f", left);
    int cs = int(std::fabs(clock_) * 100) % 100;
    int sec = int(std::fabs(clock_)) % 60;
    int min = int(std::fabs(clock_)) / 60;
    std::snprintf(clock, sizeof clock, "CREW %d:%02d.%02d", min, sec, cs);
    int cpal = clock_ < 4.f ? PAL_ALERT : PAL_HUD;
    text(1, 0, dist, PAL_HUD);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, cpal);
    if (mode_ == Mode::Run && used_ < 2.4f) textC(2, "BEAT THE OTHER CREW", PAL_HUD);
    if (mode_ == Mode::Run && braking_) textC(2, "BRAKE", PAL_MARK);
    if (mode_ == Mode::Fail) textC(15, why_, PAL_ALERT);
    if (mode_ == Mode::Win) textC(15, "SET DOWN ON THE MARK", PAL_WIN);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += DT * 6.f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || (bot_ && t_ > 0.35f)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    } else if (mode_ == Mode::Pause) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    }
    audio();
    draw();
}

}  // namespace metromark
