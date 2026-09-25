#include "row.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace row {
namespace {

constexpr int HORIZON = 100;
constexpr float Z_NEAR = 4.5f;
constexpr float Z_BOAT = 8.f;
constexpr float PPM_NEAR = 49.8f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float LANE = 3.15f;
constexpr float ROAD_HW = LANE / 0.935f;  // white paint lands on the foul line
constexpr float BUOY_X = LANE * 1.14f;
constexpr float OUTER = LANE * 2.4f;
constexpr float BANK = LANE * 3.15f;
constexpr float PERIOD = 1.36f;
constexpr float DRIVE = 0.44f;
constexpr float POWER = 4.15f;
constexpr float DRAG_K = 0.16f;
constexpr float DRAG_C = 0.05f;
constexpr float AUTH0 = 2.7f;
constexpr float AUTHV = 0.04f;
constexpr float DAMP = 1.55f;
constexpr float DT = 1.f / 60.f;
constexpr float COURSE = 500.f;

// Sustained pushes that swap sides, plus a shorter wobble. Full lock the wrong
// way leaves the lane; watching the shell and steering back stays in it.
float river(float d) {
    float push = std::tanh(std::sin(d * 0.028f) * 4.f);
    return 2.15f * push + 0.35f * std::sin(d * 0.11f + 0.5f);
}

int fogFor(float ahead) {
    return int(std::clamp((ahead - 10.f) * 0.22f, 0.f, 12.f));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Foul) return 3;
    return 1;
}

int Game::shellFrame(float phase) const {
    if (phase < 0.16f) return 0;
    if (phase < 0.40f) return 1;
    if (phase < 0.62f) return 2;
    return 3;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.16f, 0.22f, 0.1f);
    gs::FMPatch thunk;
    thunk.alg = 4;
    thunk.vol = 0.2f;
    thunk.op[0] = {1.f, 0.9f, 0.001f, 0.07f, 0.15f, 0.1f, 0.f};
    thunk.op[1] = {2.2f, 0.45f, 0.001f, 0.06f, 0.f, 0.08f, 0.f};
    thunk.op[2] = {0.5f, 0.35f, 0.002f, 0.1f, 0.2f, 0.14f, 0.f};
    thunk.op[3] = {3.f, 0.25f, 0.001f, 0.08f, 0.f, 0.1f, 0.f};
    sys.apu.setPatch(0, thunk);
    gs::FMPatch horn;
    horn.alg = 6;
    horn.vol = 0.16f;
    horn.op[0] = {1.f, 0.8f, 0.01f, 0.18f, 0.35f, 0.25f, 0.f};
    horn.op[1] = {2.f, 0.35f, 0.01f, 0.2f, 0.2f, 0.3f, 0.f};
    horn.op[2] = {3.f, 0.25f, 0.01f, 0.22f, 0.15f, 0.3f, 0.f};
    horn.op[3] = {1.f, 0.5f, 0.008f, 0.25f, 0.3f, 0.35f, 0.f};
    for (int i = 1; i < 6; i++) sys.apu.setPatch(i, horn);
    toTitle();
    over_ = false;
    won_ = false;
    report_[0] = 0;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = v_ = race_ = 0;
    phase_ = PERIOD;
    for (auto& w : wakes_) w.life = 0;
}

void Game::begin() {
    dist_ = x_ = vx_ = v_ = race_ = 0;
    phase_ = PERIOD;
    won_ = false;
    over_ = false;
    mode_ = Mode::Race;
    for (auto& w : wakes_) w.life = 0;
    sys_->apu.keyOn(2, 349.2f, 0.14f);
    sys_->rumble(0.15f, 0.05f, 80);
}

void Game::blip() {
    thunk_ = 0.11f;
    sys_->apu.keyOn(0, 96.f, 0.14f);
    sys_->rumble(0.12f, 0.04f, 30);
}

void Game::fanfare(bool win) {
    if (win) {
        sys_->apu.keyOn(3, 523.25f, 0.18f);
        sys_->apu.keyOn(4, 659.25f, 0.16f);
        sys_->apu.keyOn(5, 783.99f, 0.15f);
        sys_->rumble(0.2f, 0.4f, 160);
        sys_->setLight(40, 180, 90);
    } else {
        sys_->apu.keyOn(3, 196.f, 0.16f);
        sys_->apu.keyOn(4, 155.6f, 0.14f);
        sys_->rumble(0.45f, 0.15f, 200);
        sys_->setLight(180, 30, 20);
    }
}

void Game::finish(bool win) {
    if (mode_ != Mode::Race) return;
    won_ = win;
    over_ = true;
    mode_ = win ? Mode::Won : Mode::Foul;
    if (win) {
        std::snprintf(report_, sizeof report_, "S3 ROW  PASS  five hundred meters, still in the lane  %.1f s", race_);
    } else {
        std::snprintf(report_, sizeof report_, "S3 ROW  FAIL  left the lane at %d m", int(dist_ + 0.5f));
    }
    fanfare(win);
}

float Game::steerOf() const {
    float auth = AUTH0 + v_ * AUTHV;
    if (bot_) {
        float cur = river(dist_);
        float want = -x_ * 2.6f - vx_ * 3.f;
        return std::clamp((want - cur) / auth, -1.f, 1.f);
    }
    const gs::Pad& p = sys_->pad;
    float s = p.axisX;
    if (p.down(gs::BTN_LEFT)) s -= 1.f;
    if (p.down(gs::BTN_RIGHT)) s += 1.f;
    return std::clamp(s, -1.f, 1.f);
}

void Game::update(float dt) {
    bool rowing = bot_ || sys_->pad.down(gs::BTN_C) || sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_Z) ||
                  sys_->pad.down(gs::BTN_TURBO) || sys_->pad.accel > 0.45f;
    float auth = AUTH0 + v_ * AUTHV;
    float cur = river(dist_);
    float steer = steerOf();
    if (rowing && phase_ >= PERIOD) {
        phase_ = 0;
        blip();
        for (int side = -1; side <= 1; side += 2) {
            for (auto& w : wakes_) {
                if (w.life > 0) continue;
                w.course = dist_ - 0.3f;
                w.x = x_ + side * 0.85f;
                w.life = 1.05f;
                break;
            }
        }
    }
    if (rowing && phase_ < DRIVE) v_ += POWER * dt;
    if (rowing || phase_ < PERIOD) phase_ += dt;
    if (phase_ > PERIOD) phase_ = PERIOD;
    v_ -= (DRAG_K * v_ + DRAG_C) * dt;
    if (v_ < 0) v_ = 0;
    vx_ += (steer * auth + cur) * dt;
    vx_ -= vx_ * DAMP * dt;
    x_ += vx_ * dt;
    dist_ += v_ * dt;
    race_ += dt;
    for (auto& w : wakes_)
        if (w.life > 0) w.life -= dt;

    if (std::fabs(x_) > LANE) finish(false);
    else if (dist_ >= COURSE) finish(true);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += DT * 3.5f;
        demo_ += DT;
        if (pad.pressed(gs::BTN_START) || (bot_ && t_ > 0.6f)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Race;
        else if (pad.pressed(gs::BTN_MODE) && !bot_) toTitle();
    } else if (mode_ == Mode::Race) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) toTitle();
        else update(DT);
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
        toTitle();
    }
    draw();
    audio();
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = Z_BOAT + ahead;
    if (z < 2.4f) return p;
    float t = Z_NEAR / z;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON || p.y >= gs::SCREEN_H) return p;
    p.ppm = PPM_NEAR * t;
    p.x = 160.f + wx * p.ppm;
    p.ok = true;
    return p;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (!(h > 1.6f) || m.h <= 0) return;
    const gs::Image& img = m.pick(h);
    float s = h / float(img.h);
    gs::Sprite sp;
    sp.img = img;
    sp.h = std::max(1, int(std::lround(h)));
    sp.w = std::max(1, int(std::lround(img.w * s)));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(cy - sp.h * 0.5f));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sp.shadow = shadow;
    sys_->vdp.sprite(sp);
}

void Game::water(float view) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            int r = std::clamp(int(6 + u * 9), 0, 15);
            int g = std::clamp(int(8 + u * 5), 0, 15);
            int b = std::clamp(int(13 - u * 2), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (y - HORIZON) / SPAN;
        t = std::max(t, 0.02f);
        float z = Z_NEAR / t;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f;
        rd.hw = std::max(2.f, ROAD_HW * ppm);
        rd.v = (view + (z - Z_BOAT)) * 46.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (int(std::floor((view + z) / 4.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_WATER;
        rd.right = gs::GROUND_WATER;
        float fogT = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11);
        v.lineBackdrop[y] = gs::rgb4(1, 4, 8);
    }
}

void Game::bank(float view) {
    int hs = int(std::floor(view * 4.f));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.B.hscroll[y] = int16_t(-hs);
        sys_->vdp.B.vscroll[y] = 0;
    }
}

void Game::field(float along) {
    auto chain = [&](float wx, float step, float from, float to) {
        float d0 = std::floor((along - 6.f) / step) * step;
        for (float m = d0; m < along + 80.f; m += step) {
            if (m < from || m > to) continue;
            float ahead = m - along;
            Proj p = project(wx, ahead);
            if (!p.ok) continue;
            int fog = fogFor(ahead);
            if (step > 12.f) {
                float h = p.ppm * 1.15f;
                int kind = int(std::floor(m)) & 1;
                spr(art_.tree[kind], p.x, p.y - h * 0.35f, h, PAL_SCENE, kind, fog);
            } else {
                spr(art_.buoy, p.x, p.y - p.ppm * 0.2f, p.ppm * 0.62f, PAL_BUOY, false, fog);
            }
        }
    };
    // Far bank and outer buoys first so the racing lane sits on top of them.
    chain(-BANK, 18.f, -20.f, 540.f);
    chain(BANK, 18.f, -20.f, 540.f);
    chain(-OUTER, 10.f, 0.f, COURSE + 8.f);
    chain(OUTER, 10.f, 0.f, COURSE + 8.f);
    chain(-BUOY_X, 8.f, 0.f, COURSE + 4.f);
    chain(BUOY_X, 8.f, 0.f, COURSE + 4.f);

    Proj house = project(-BANK * 0.92f, 8.f - along);
    if (house.ok) spr(art_.house, house.x, house.y - house.ppm * 0.7f, house.ppm * 2.1f, PAL_SCENE, false, fogFor(8.f - along));
    Proj grand = project(BANK * 0.9f, COURSE - along);
    if (grand.ok) spr(art_.stand, grand.x, grand.y - grand.ppm * 0.8f, grand.ppm * 2.4f, PAL_SCENE, false, fogFor(COURSE - along));

    for (int i = 0; i < 4; i++) {  // the gantry is the 500 mark
        float mark = (i + 1) * 100.f;
        Proj p = project(-(BUOY_X + 0.35f), mark - along);
        if (!p.ok) continue;
        spr(art_.sign[i], p.x, p.y - p.ppm * 0.55f, p.ppm * 1.15f, PAL_SCENE, false, fogFor(mark - along));
    }
    // A few metres past the line, and up off the water, so the bow does not cover the 500.
    Proj gate = project(0, (COURSE + 8.f) - along);
    if (gate.ok) {
        float h = gate.ppm * 2.4f;
        spr(art_.banner, gate.x, gate.y - h * 1.15f, h, PAL_BANNER, false, fogFor((COURSE + 8.f) - along));
    }
    Proj coach = project(BUOY_X + 1.6f, 14.f);
    if (coach.ok) spr(art_.launch, coach.x, coach.y, coach.ppm * 1.35f, PAL_LAUNCH, false, fogFor(14.f));

    for (auto& w : wakes_) {
        if (w.life <= 0 || mode_ == Mode::Title) continue;
        Proj p = project(w.x, w.course - along);
        if (!p.ok) continue;
        spr(art_.wake, p.x, p.y, 6.f + w.life * 6.f, PAL_WAKE, false, 0);
    }
}

void Game::shellAt(float phase) {
    float t = Z_NEAR / Z_BOAT;
    float ppm = PPM_NEAR * t;
    float y = HORIZON + t * SPAN;
    float rock = std::sin(phase * 6.2831853f / PERIOD) * 1.6f;
    float bob = std::sin(phase * 6.2831853f / PERIOD) * 1.2f;
    float x = 160.f + x_ * ppm + rock;
    float h = 78.f;
    int frame = shellFrame(phase >= PERIOD ? 0.f : phase);
    bool driving = phase < DRIVE;
    // Earlier sprites draw on top. The shell covers its splash and shadow.
    spr(art_.shell[frame], x, y - 6.f + bob, h, PAL_SHELL);
    if (driving) {
        spr(art_.splash, x - 30.f, y + 6.f, 12.f, PAL_WAKE);
        spr(art_.splash, x + 30.f, y + 6.f, 12.f, PAL_WAKE);
    }
    spr(art_.shade, x, y + 16.f + bob, 14.f, PAL_SHELL, false, 0, true);
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
    int n = int(std::strlen(s));
    text((40 - n) / 2, row, s, pal);
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        textC(10, "HOLD C TO ROW", PAL_ALERT);
        textC(11, "ARROWS STEER", PAL_ALERT);
        if (int(t_ * 2.f) & 1) textC(12, "START", PAL_AMBER);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(10, "PAUSE", PAL_AMBER);
        return;
    }
    float shown = dist_;
    if (mode_ == Mode::Race || mode_ == Mode::Won || mode_ == Mode::Foul) {
        int left = int(std::ceil(COURSE - shown - 0.001f));
        if (left < 0) left = 0;
        char m[16], clock[16];
        int showM = mode_ == Mode::Won ? 500 : mode_ == Mode::Foul ? int(dist_ + 0.5f) : left;
        std::snprintf(m, sizeof m, "%d M", showM);
        int cs = int(race_ * 100) % 100;
        int sec = int(race_) % 60;
        int min = int(race_) / 60;
        if (cs < 0) cs = 0;
        std::snprintf(clock, sizeof clock, "%d:%02d.%02d", min, sec, cs);
        text(1, 0, m, PAL_HUD);
        text(40 - int(std::strlen(clock)) - 1, 0, clock, PAL_HUD);

        char bar[20];
        const int n = 19;
        int mid = n / 2;
        int pos = mid + int(std::lround((x_ / LANE) * (mid - 1)));
        pos = std::clamp(pos, 0, n - 1);
        for (int i = 0; i < n; i++) bar[i] = (i == pos) ? '+' : '=';
        bar[0] = '<';
        bar[n - 1] = '>';
        bar[n] = 0;
        int pal = std::fabs(x_) > LANE * 0.78f ? PAL_ALERT : PAL_HUD;
        textC(1, bar, pal);
        if (std::fabs(x_) > LANE * 0.78f && mode_ == Mode::Race) textC(2, "LANE", PAL_ALERT);
        if (mode_ == Mode::Race && race_ < 3.f) textC(3, "STAY IN THE LANE", PAL_DIM);
    }
    if (mode_ == Mode::Foul) {
        char at[32];
        std::snprintf(at, sizeof at, "AT %d M", int(dist_ + 0.5f));
        textC(12, at, PAL_ALERT);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    float view = (mode_ == Mode::Title) ? scenery_ : dist_;
    water(view);
    bank(view);
    float phase = (mode_ == Mode::Title) ? std::fmod(demo_, PERIOD) : (phase_ >= PERIOD ? 0.f : phase_);
    // First sprite wins the pixel, so the title and the shell go in before the river.
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 22, float(art_.title.h), PAL_TITLE);
        spr(art_.sub, 160, 50, float(art_.sub.h), PAL_TITLE);
        spr(art_.stay, 160, 70, float(art_.stay.h), PAL_AMBER);
    } else if (mode_ == Mode::Won) {
        spr(art_.win, 160, 48, float(art_.win.h), PAL_TITLE);
    } else if (mode_ == Mode::Foul) {
        spr(art_.out, 160, 48, float(art_.out.h), PAL_ALERT);
    }
    shellAt(phase);
    field(view);
    float drift = t_ * 8.f;
    spr(art_.sun, 262, 22, 18, PAL_SKY);
    spr(art_.cloud, std::fmod(30.f + drift, 360.f) - 20.f, 16, 14, PAL_SKY);
    spr(art_.cloud, std::fmod(180.f + drift * 0.7f, 380.f) - 30.f, 34, 18, PAL_SKY);
    spr(art_.bird, std::fmod(70.f + t_ * 22.f, 340.f), 48, 7, PAL_SKY);
    spr(art_.bird, std::fmod(200.f + t_ * 18.f, 360.f), 58, 6, PAL_SKY);
    hud();
}

void Game::audio() {
    float water = (mode_ == Mode::Title) ? 0.012f : 0.02f;
    sys_->apu.noise(water, 620.f, false);
    thunk_ *= 0.86f;
    sys_->apu.tone(0, 88.f, thunk_);
    if (mode_ == Mode::Race && std::fabs(x_) > LANE * 0.78f) {
        float pulse = 0.5f + 0.5f * std::sin(t_ * 26.f);
        sys_->apu.tone(1, 620.f, 0.04f * pulse);
        sys_->setLight(200, 40, 30);
    } else {
        sys_->apu.tone(1, 0, 0);
        if (mode_ == Mode::Race) sys_->setLight(30, 90, 140);
        else if (mode_ == Mode::Title) sys_->setLight(40, 60, 120);
    }
}

}  // namespace row
