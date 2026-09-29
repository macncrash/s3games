#include "turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rickshawturn {
namespace {

constexpr int HORIZON = 78;
constexpr float Z_NEAR = 5.0f;
constexpr float Z_AXLE = 8.0f;
constexpr float PPM_NEAR = 46.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float COURSE = 300.f;
constexpr float LIMIT = 48.f;
constexpr float CRUISE = 9.2f;
constexpr float DT = 1.f / 60.f;
constexpr float TIP = 1.02f;
constexpr float NEED_K = 0.145f;
constexpr int TURN_N = 3;

const float kCentre[TURN_N] = {68.f, 152.f, 236.f};
const float kWidth[TURN_N] = {16.f, 15.f, 15.f};
const float kAmp[TURN_N] = {-0.082f, 0.090f, -0.084f};

int fogFor(float ahead) {
    return int(std::clamp((ahead - 5.f) * 0.15f, 0.f, 12.f));
}

float gauss(float s, float c, float w) {
    float d = (s - c) / w;
    if (std::fabs(d) > 2.2f) return 0.f;
    return std::exp(-d * d * 1.7f);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (turns_ >= 2 || dist_ > kCentre[2] - 28.f) return 3;
    if (std::fabs(curveAt(dist_)) > 0.02f) return 2;
    return 1;
}

float Game::curveAt(float s) const {
    float k = 0.f;
    for (int i = 0; i < TURN_N; i++) k += kAmp[i] * gauss(s, kCentre[i], kWidth[i]);
    return k;
}

float Game::bendAt(float s) const {
    float b = 0.f;
    const float pix[TURN_N] = {-62.f, 70.f, -58.f};
    for (int i = 0; i < TURN_N; i++) b += pix[i] * gauss(s, kCentre[i], kWidth[i] + 6.f);
    return b;
}

float Game::halfAt(float s) const {
    float pinch = 0.f;
    for (int i = 0; i < TURN_N; i++) pinch = std::max(pinch, gauss(s, kCentre[i], kWidth[i]));
    return 2.35f - pinch * 0.45f;
}

float Game::needAt(float s, float speed) const {
    return curveAt(s) * speed * speed * NEED_K;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.1f, 0.16f, 0.06f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    dist_ = x_ = vx_ = helm_ = lean_ = clock_ = slip_ = 0;
    speed_ = CRUISE;
    scenery_ = 18.f;
    turns_ = passed_ = 0;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::begin() {
    dist_ = x_ = vx_ = helm_ = lean_ = clock_ = slip_ = 0;
    speed_ = 7.2f;
    turns_ = passed_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    mode_ = Mode::Run;
    sys_->rumble(0.05f, 0.02f, 60);
    sys_->apu.keyOn(1, 620.f, 0.07f);
}

void Game::finish(bool good, const char* why) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = why;
    if (good) {
        sys_->apu.keyOn(1, 523.25f, 0.14f);
        sys_->apu.keyOn(2, 659.25f, 0.12f);
        sys_->setLight(40, 160, 50);
        sys_->rumble(0.08f, 0.2f, 120);
    } else {
        sys_->apu.keyOn(1, 82.f, 0.18f);
        sys_->setLight(160, 28, 12);
        sys_->rumble(0.4f, 0.08f, 180);
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float want = 0;
    float throttle = 0.55f;
    float look = dist_ + speed_ * 0.42f;
    float aheadNeed = needAt(look, speed_);
    if (bot_) {
        float sharp = std::fabs(curveAt(look));
        throttle = sharp > 0.035f ? 0.05f : (sharp > 0.012f ? 0.35f : 1.f);
        float hold = std::clamp(speed_, 5.4f, 8.4f);
        aheadNeed = needAt(look, hold);
        want = std::clamp(aheadNeed * 1.08f - x_ * 0.55f - vx_ * 0.42f, -1.f, 1.f);
    } else {
        if (pad.down(gs::BTN_LEFT)) want -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) want += 1.f;
        if (std::fabs(pad.axisX) > 0.16f) want = pad.axisX;
        if (pad.down(gs::BTN_UP) || pad.accel > 0.2f) throttle = 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) throttle = 0.f;
    }
    helm_ += (std::clamp(want, -1.f, 1.f) - helm_) * (1.f - std::exp(-6.5f * dt));
    lean_ += (helm_ * 0.96f - lean_) * (1.f - std::exp(-4.2f * dt));
    float target = 5.2f + throttle * 6.6f;
    if (bot_) target = std::min(target, 8.1f);
    speed_ += (target - speed_) * (1.f - std::exp(-1.15f * dt));
    float need = needAt(dist_, speed_);
    float err = lean_ - need;
    vx_ += err * 3.1f * dt;
    vx_ *= std::exp(-2.2f * dt);
    x_ += vx_ * dt;
    dist_ += speed_ * dt;
    clock_ += dt;

    if (std::fabs(err) > 0.48f || std::fabs(need) > 0.92f) slip_ += dt;
    else slip_ = std::max(0.f, slip_ - dt * 1.6f);

    while (passed_ < TURN_N && dist_ >= kCentre[passed_]) {
        if (std::fabs(lean_) < 0.9f && std::fabs(x_) < halfAt(dist_)) turns_++;
        passed_++;
    }

    if (std::fabs(lean_) > TIP || std::fabs(need) > TIP || slip_ > 0.42f) finish(false, "tipped");
    else if (std::fabs(x_) > halfAt(dist_)) finish(false, "tipped");
    else if (dist_ >= COURSE) {
        if (turns_ >= TURN_N) finish(true, "made the three turns");
        else finish(false, "missed a turn");
    } else if (clock_ >= LIMIT) finish(false, "ran out of road");
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = ahead + Z_AXLE;
    if (z < 0.45f) return p;
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

void Game::street(float view) {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(11 - u * 3), int(6 - u * 2), int(4 + u * 5));
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
        rd.hw = std::max(5.f, halfAt(along) * ppm);
        rd.v = along * 28.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (int(std::floor(along / 7.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.24f - t) / 0.24f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 7);
        v.lineBackdrop[y] = gs::rgb4(4, 3, 3);
    }
}

void Game::furniture(float along) {
    auto row = [&](float step, auto place) {
        float d0 = std::floor((along - 8.f) / step) * step;
        for (float m = d0; m < along + 120.f; m += step) place(m, m - along);
    };
    row(16.f, [&](float m, float ahead) {
        float half = halfAt(m);
        for (int side = -1; side <= 1; side += 2) {
            Proj p = project(side * (half + 0.85f), ahead);
            if (!p.ok) continue;
            spr(art_.lamp, p.x, p.y, p.ppm * 2.5f, PAL_LAMP, side < 0, p.fog);
        }
    });
    for (int i = 0; i < TURN_N; i++) {
        float k = curveAt(kCentre[i]);
        int side = k >= 0.f ? 1 : -1;
        for (int n = -2; n <= 2; n++) {
            float m = kCentre[i] + n * 7.f;
            Proj p = project(side * (halfAt(m) + 1.15f), m - along);
            if (!p.ok) continue;
            spr(art_.chevron, p.x, p.y - p.ppm * 1.1f, p.ppm * 1.15f, PAL_SIGN, side < 0, p.fog);
        }
        Proj post = project(side * (halfAt(kCentre[i]) + 2.3f), kCentre[i] - along);
        if (post.ok) spr(art_.post, post.x, post.y, post.ppm * 2.1f, PAL_POST, false, post.fog);
    }
    Proj flag = project(0.f, COURSE - along);
    if (flag.ok) spr(art_.flag, flag.x, flag.y, flag.ppm * 3.4f, PAL_WIN, false, flag.fog);
}

void Game::rickshawAt() {
    float t = Z_NEAR / Z_AXLE;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float x = 160.f + bendAt(along) + xoff * (PPM_NEAR * t);
    float bob = std::sin(t_ * 8.f) * 0.7f;
    float tilt = lean_ * 22.f;
    bool flop = mode_ == Mode::Fail;
    float fall = flop ? 16.f : 0.f;
    spr(art_.shadow, x + tilt * 0.3f, y + 20.f, 16.f, PAL_WHEEL, false, 0);
    spr(art_.wheel, x - 18.f, y + 16.f + bob, 20.f, PAL_WHEEL);
    spr(art_.wheel, x + 18.f, y + 16.f + bob, 20.f, PAL_WHEEL);
    spr(art_.cab, x + tilt * 0.55f + fall, y + 8.f + bob, 42.f, PAL_CAB, flop);
    spr(art_.rider, x + tilt + fall * 1.2f, y - 6.f + bob, 24.f, PAL_CAB, flop);
    spr(art_.wheel, x + tilt * 1.3f, y + bob, 13.f, PAL_WHEEL);
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
        spr(art_.title, 160, 30, float(art_.title.h), PAL_SIGN);
        spr(art_.job, 160, 54, float(art_.job.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 82, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.made, 160, 32, float(art_.made.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        spr(art_.tipped, 160, 32, float(art_.tipped.h), PAL_ALERT);
    }
    rickshawAt();
    furniture(view);

    if (mode_ == Mode::Title) {
        textC(24, "LEFT RIGHT LEAN", PAL_HUD);
        textC(25, "UP DOWN THE PEDALS", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_SIGN);
        textC(14, "START RESUMES", PAL_HUD);
        return;
    }
    char turns[16];
    std::snprintf(turns, sizeof turns, "TURN %d/3", std::min(turns_, 3));
    text(1, 0, turns, PAL_HUD);
    int remain = int(std::ceil(std::max(0.f, LIMIT - clock_)));
    if (mode_ != Mode::Run) remain = 0;
    char clock[16];
    std::snprintf(clock, sizeof clock, "%d S", remain);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, mode_ == Mode::Run && remain < 8 ? PAL_ALERT : PAL_HUD);

    char bar[22];
    const int n = 17;
    int mid = n / 2;
    int pos = mid + int(std::lround(std::clamp(lean_, -1.15f, 1.15f) * (mid - 1)));
    pos = std::clamp(pos, 0, n - 1);
    for (int i = 0; i < n; i++) bar[i] = '-';
    bar[mid] = ':';
    bar[pos] = '+';
    bar[0] = '<';
    bar[n - 1] = '>';
    bar[n] = 0;
    int pal = std::fabs(lean_) > 0.78f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    if (mode_ == Mode::Run && std::fabs(lean_) > 0.78f) textC(2, "LEAN", PAL_ALERT);
    if (mode_ == Mode::Run && dist_ < 22.f) textC(3, "LEAN INTO THE TURN", PAL_HUD);
    if (mode_ == Mode::Win) textC(16, "NO TIP", PAL_WIN);
    if (mode_ == Mode::Fail) textC(16, "THE CAB WENT OVER", PAL_ALERT);
}

void Game::audio() {
    if (mode_ == Mode::Run) {
        bell_ += DT * (0.4f + speed_ * 0.08f);
        float wob = 1.f + 0.04f * std::sin(bell_ * 9.f) + std::fabs(lean_) * 0.05f;
        sys_->apu.tone(0, 68.f * wob, 0.028f + std::fabs(lean_) * 0.02f);
        sys_->apu.noise(0.01f + std::fabs(lean_) * 0.02f, 700.f + std::fabs(lean_) * 400.f, false);
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
        scenery_ += CRUISE * 0.28f * DT;
        if (scenery_ > COURSE - 24.f) scenery_ = 12.f;
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

}  // namespace rickshawturn
