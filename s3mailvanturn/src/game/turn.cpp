#include "turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mailvanturn {
namespace {

constexpr int HORIZON = 76;
constexpr float Z_NEAR = 5.0f;
constexpr float Z_VAN = 8.2f;
constexpr float PPM_NEAR = 48.f;
constexpr float SPAN = float(gs::SCREEN_H - 1 - HORIZON);
constexpr float COURSE = 430.f;
constexpr float LIMIT = 78.f;
constexpr float DT = 1.f / 60.f;
constexpr float STEER_A = 13.5f;
constexpr float TIP = 1.02f;
constexpr float HALF = 2.35f;
constexpr float OFF = 3.7f;

struct Bend {
    float enter, apex, exit, dir, width;
};

constexpr Bend kBends[3] = {
    {58.f, 96.f, 134.f, 1.f, 22.f},
    {176.f, 216.f, 256.f, -1.f, 20.f},
    {300.f, 344.f, 388.f, 1.f, 21.f},
};

int fogFor(float ahead) { return int(std::clamp((ahead - 5.f) * 0.17f, 0.f, 12.f)); }

float kappa(float s) {
    float k = 0.f;
    for (const Bend& b : kBends) {
        float u = (s - b.apex) / b.width;
        k += b.dir * 0.078f * std::exp(-u * u);
    }
    return k;
}

float bendScreen(float s) {
    float y = 0.f;
    for (const Bend& b : kBends) {
        float u = (s - b.apex) / (b.width * 1.35f);
        y += b.dir * 168.f * u * std::exp(-u * u);
    }
    return y;
}

float halfAt(float s) {
    float pinch = 0.f;
    for (const Bend& b : kBends) {
        float u = (s - b.apex) / b.width;
        pinch = std::max(pinch, std::exp(-u * u));
    }
    return HALF - pinch * 0.28f;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail || over_) return 4;
    if (turns_ >= 2) return 3;
    float k = std::fabs(kappa(dist_));
    if (turns_ >= 1 || k > 0.03f) return 2;
    return 1;
}

const char* Game::tipWhy() const {
    if (turns_ <= 0) return "tipped before the first turn";
    if (turns_ == 1) return "tipped on the second turn";
    if (turns_ == 2) return "tipped on the third turn";
    return "tipped after the turns";
}

const char* Game::missWhy() const {
    if (turns_ <= 0) return "missed the first turn";
    if (turns_ == 1) return "missed the second turn";
    if (turns_ == 2) return "missed the third turn";
    return "slid off after the turns";
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
    dist_ = x_ = vx_ = helm_ = clock_ = lean_ = leanVel_ = 0;
    speed_ = 8.f;
    scenery_ = 24.f;
    turns_ = 0;
    for (bool& m : made_) m = false;
    why_ = "";
    won_ = false;
    over_ = false;
}

void Game::begin() {
    dist_ = x_ = vx_ = helm_ = clock_ = lean_ = leanVel_ = 0;
    speed_ = 6.4f;
    turns_ = 0;
    for (bool& m : made_) m = false;
    won_ = false;
    over_ = false;
    why_ = "";
    mode_ = Mode::Run;
    sys_->rumble(0.06f, 0.02f, 80);
}

void Game::finish(bool good, const char* why) {
    mode_ = good ? Mode::Win : Mode::Fail;
    won_ = good;
    over_ = true;
    why_ = why;
    if (good) {
        sys_->apu.keyOn(1, 392.f, 0.15f);
        sys_->apu.keyOn(2, 523.25f, 0.12f);
        sys_->setLight(40, 150, 70);
        sys_->rumble(0.1f, 0.24f, 110);
    } else {
        sys_->apu.keyOn(1, 90.f, 0.18f);
        sys_->setLight(150, 36, 18);
        sys_->rumble(0.5f, 0.16f, 180);
    }
}

void Game::pilot(float& steer, float& throttle) {
    float worst = 0.f;
    for (int i = 0; i <= 5; i++) worst = std::max(worst, std::fabs(kappa(dist_ + 6.f + float(i) * 7.f)));
    float now = kappa(dist_ + 4.f);
    float want = 10.6f;
    float load = speed_ * speed_ * worst;
    if (load > 6.2f) want = 8.4f;
    if (load > 8.4f) want = 7.1f;
    if (load > 10.f) want = 6.2f;
    if (std::fabs(lean_) > 0.55f) want = std::min(want, 6.0f);
    if (std::fabs(x_) > 1.15f) want = std::min(want, 7.2f);
    float match = (speed_ * speed_ * now) / STEER_A;
    float corr = -x_ * 0.62f - vx_ * 0.42f;
    float cap = 0.9f;
    if (std::fabs(lean_) > 0.62f) {
        bool adding = (match > 0.f && lean_ > 0.f) || (match < 0.f && lean_ < 0.f);
        if (adding) match *= 0.45f;
        cap = 0.62f;
    }
    steer = std::clamp(match + corr, -cap, cap);
    float gap = want - speed_;
    throttle = std::clamp(0.45f + gap * 0.28f, 0.05f, 1.f);
}

void Game::markTurns(float prev) {
    for (int i = 0; i < 3; i++) {
        if (made_[i]) continue;
        if (dist_ < kBends[i].exit) break;
        if (i > 0 && !made_[i - 1]) {
            finish(false, "skipped a turn");
            return;
        }
        if (prev >= kBends[i].exit) {
            finish(false, "skipped a turn");
            return;
        }
        if (std::fabs(x_) > halfAt(dist_) + 0.85f || speed_ < 2.4f) {
            finish(false, missWhy());
            return;
        }
        made_[i] = true;
        turns_ = i + 1;
        sys_->apu.keyOn(1, 420.f + float(i) * 70.f, 0.12f);
        sys_->rumble(0.12f, 0.05f, 60);
        if (i == 2) finish(true, "upright");
        return;
    }
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    float want = 0.f;
    float throttle = 0.45f;
    if (bot_) {
        pilot(want, throttle);
    } else {
        if (pad.down(gs::BTN_LEFT)) want -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) want += 1.f;
        if (std::fabs(pad.axisX) > 0.18f) want = pad.axisX;
        if (pad.down(gs::BTN_UP) || pad.accel > 0.2f) throttle = 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) throttle = 0.05f;
    }
    helm_ += (std::clamp(want, -1.f, 1.f) - helm_) * (1.f - std::exp(-3.4f * dt));
    float target = 4.2f + throttle * 9.4f;
    speed_ += (target - speed_) * (1.f - std::exp(-0.85f * dt));
    speed_ = std::clamp(speed_, 1.5f, 14.5f);

    float prev = dist_;
    float k = kappa(dist_);
    float steerA = helm_ * STEER_A;
    float cent = speed_ * speed_ * k;
    vx_ += (steerA - cent) * dt;
    vx_ *= std::exp(-2.1f * dt);
    x_ += vx_ * dt;
    dist_ += speed_ * dt;

    float leanTarget = steerA * 0.062f + (cent - steerA) * 0.012f;
    leanVel_ += ((leanTarget - lean_) * 16.f - leanVel_ * 5.4f) * dt;
    lean_ += leanVel_ * dt;
    lean_ = std::clamp(lean_, -1.6f, 1.6f);

    clock_ += dt;
    if (std::fabs(lean_) > TIP) {
        finish(false, tipWhy());
        return;
    }
    if (std::fabs(x_) > OFF) {
        finish(false, missWhy());
        return;
    }
    markTurns(prev);
    if (mode_ != Mode::Run) return;
    if (dist_ > COURSE) {
        finish(false, "missed the turns");
        return;
    }
    if (clock_ >= LIMIT) finish(false, "the clock ran out");
}

Game::Proj Game::project(float wx, float ahead) const {
    Proj p;
    float z = ahead + Z_VAN;
    if (z < 0.4f) return p;
    float t = Z_NEAR / z;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON - 6 || p.y >= gs::SCREEN_H + 28) return p;
    p.ppm = PPM_NEAR * t;
    float along = ((mode_ == Mode::Title) ? scenery_ : dist_) + ahead;
    p.x = 160.f + bendScreen(along) + wx * p.ppm;
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

void Game::street(float view) {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 60.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            v.road[y].on = false;
            float u = float(y) / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(4 + (1.f - u) * 5), int(5 + (1.f - u) * 4), int(9 + u * 3));
            v.lineFog[y] = uint8_t(u * 5);
            continue;
        }
        float t = std::max((y - HORIZON) / SPAN, 0.02f);
        float z = Z_NEAR / t;
        float ahead = z - Z_VAN;
        float ppm = PPM_NEAR * t;
        float along = view + ahead;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + bendScreen(along);
        rd.hw = std::max(4.f, halfAt(along) * ppm);
        rd.v = along * 34.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (int(std::floor(along / 7.f)) & 1) ? 1 : 0;
        rd.left = 0;
        rd.right = 0;
        float fogT = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 9);
        v.lineBackdrop[y] = gs::rgb4(3, 4, 3);
    }
}

void Game::sides(float along) {
    auto row = [&](float step, auto place) {
        float d0 = std::floor((along - 6.f) / step) * step;
        for (float m = d0; m < along + 120.f; m += step) place(m, m - along);
    };
    row(18.f, [&](float m, float ahead) {
        float half = halfAt(m);
        int side = (int(m / 18.f) & 1) ? 1 : -1;
        Proj p = project(side * (half + 0.9f), ahead);
        if (!p.ok) return;
        spr(art_.box, p.x, p.y, p.ppm * 1.55f, PAL_BOX, side < 0, p.fog);
    });
    row(16.f, [&](float m, float ahead) {
        float half = halfAt(m);
        int side = (int(m / 16.f) & 1) ? -1 : 1;
        Proj p = project(side * (half + 1.8f), ahead);
        if (!p.ok) return;
        spr(art_.lamp, p.x, p.y, p.ppm * 2.4f, PAL_LAMP, side < 0, p.fog);
    });
    row(46.f, [&](float m, float ahead) {
        int side = (int(m / 46.f) & 1) ? -1 : 1;
        Proj p = project(side * (halfAt(m) + 3.8f), ahead);
        if (!p.ok) return;
        spr(art_.house, p.x, p.y, p.ppm * 4.2f, PAL_HOUSE, side < 0, p.fog);
    });
    for (int i = 0; i < 3; i++) {
        float ahead = kBends[i].enter - along;
        int side = kBends[i].dir > 0.f ? 1 : -1;
        Proj p = project(side * (halfAt(kBends[i].enter) + 1.15f), ahead);
        if (!p.ok) continue;
        spr(art_.post, p.x, p.y, p.ppm * 1.8f, PAL_SIGN, false, p.fog);
        spr(art_.chevron, p.x, p.y - p.ppm * 1.5f, p.ppm * 1.15f, PAL_SIGN, side < 0, p.fog);
    }
}

void Game::vanAt() {
    float t = Z_NEAR / Z_VAN;
    float y = HORIZON + t * SPAN;
    float along = (mode_ == Mode::Title) ? scenery_ : dist_;
    float xoff = (mode_ == Mode::Title) ? 0.f : x_;
    float lean = (mode_ == Mode::Title) ? 0.22f * std::sin(t_ * 1.3f) : lean_;
    float x = 160.f + bendScreen(along) + xoff * (PPM_NEAR * t);
    float roll = lean * 22.f;
    bool tipped = mode_ == Mode::Fail && why_ && why_[0] == 't';
    float body = tipped ? 28.f : 52.f;
    spr(art_.shade, x, y + 26.f, 16.f, PAL_VAN, false, 0, true);
    spr(art_.wheel, x - 16.f, y + 18.f, 14.f, PAL_VAN);
    spr(art_.wheel, x + 16.f, y + 18.f, 14.f, PAL_VAN);
    spr(art_.van, x + roll, y + (tipped ? 16.f : 8.f), body, PAL_VAN, lean < -0.15f);
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

void Game::textC(int row, const char* s, int pal) { text((40 - int(std::strlen(s))) / 2, row, s, pal); }

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    float view = (mode_ == Mode::Title) ? scenery_ : dist_;
    street(view);
    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 32, float(art_.title.h), PAL_SIGN);
        spr(art_.job, 160, 58, float(art_.job.h), PAL_HUD);
        if (int(t_ * 2.f) & 1) spr(art_.start, 160, 86, float(art_.start.h), PAL_WIN);
    } else if (mode_ == Mode::Win) {
        spr(art_.upright, 160, 34, float(art_.upright.h), PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* banner = &art_.missed;
        if (why_ && why_[0] == 't') banner = &art_.tipped;
        else if (why_ && std::strcmp(why_, "the clock ran out") == 0) banner = &art_.clock;
        spr(*banner, 160, 34, float(banner->h), PAL_ALERT);
    }
    vanAt();
    sides(view);

    if (mode_ == Mode::Title) {
        textC(24, "LEFT RIGHT STEER", PAL_HUD);
        textC(25, "UP DOWN THE VAN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSE", PAL_SIGN);
        textC(14, "START RESUMES", PAL_HUD);
        return;
    }
    char turns[16], clock[20], lean[24];
    std::snprintf(turns, sizeof turns, "TURN %d/3", turns_);
    int remain = int(std::ceil(std::max(0.f, LIMIT - clock_)));
    if (mode_ == Mode::Win || mode_ == Mode::Fail) remain = int(clock_);
    std::snprintf(clock, sizeof clock, "%d S", remain);
    text(1, 0, turns, PAL_HUD);
    text(40 - int(std::strlen(clock)) - 1, 0, clock, mode_ == Mode::Run && remain < 10 ? PAL_ALERT : PAL_HUD);

    const int n = 19;
    int mid = n / 2;
    int pos = mid + int(std::lround((lean_ / TIP) * (mid - 1)));
    pos = std::clamp(pos, 0, n - 1);
    char bar[22];
    for (int i = 0; i < n; i++) bar[i] = (i == pos) ? '+' : '-';
    bar[0] = 'L';
    bar[n - 1] = 'R';
    bar[n] = 0;
    int pal = std::fabs(lean_) > TIP * 0.72f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);
    std::snprintf(lean, sizeof lean, "LEAN %+.0f", lean_ * 57.2958f);
    textC(2, lean, pal);
    if (mode_ == Mode::Run && dist_ < 28.f) textC(3, "EASE THE BENDS", PAL_HUD);
    if (mode_ == Mode::Win) textC(16, "THREE TURNS UPRIGHT", PAL_WIN);
    if (mode_ == Mode::Fail && why_) textC(16, why_, PAL_ALERT);
}

void Game::audio() {
    if (mode_ == Mode::Run) {
        idle_ += DT * (0.7f + speed_ * 0.09f);
        float wob = 1.f + 0.03f * std::sin(idle_ * 11.f);
        sys_->apu.tone(0, 58.f * wob + std::fabs(lean_) * 18.f, 0.045f);
        sys_->apu.tone(1, 90.f * wob, 0.016f);
        sys_->apu.noise(0.012f, 380.f, false);
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
        scenery_ += 3.2f * DT;
        if (scenery_ > 70.f) scenery_ = 18.f;
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

}  // namespace mailvanturn
