#include "tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tramturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kCrew = 52.f;
constexpr float kHalf = 12.f;
constexpr float kTipG = 24.f;
constexpr float kZoom = 4.35f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float len2(float x, float y) { return x * x + y * y; }

}  // namespace

float Game::crewLeft() const { return std::max(0.f, kCrew - raceTime_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    return std::min(turns_ + 1, 3);
}

int Game::tramFrame(float hdg) const {
    float u = std::fmod(hdg, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

void Game::buildPath() {
    path_.clear();
    auto push = [&](float x, float y, int turn) {
        if (!path_.empty() && len2(path_.back().x - x, path_.back().y - y) < 0.4f) return;
        path_.push_back({x, y, turn});
    };
    auto line = [&](float x0, float y0, float x1, float y1, int turn) {
        float dx = x1 - x0, dy = y1 - y0;
        float L = std::sqrt(dx * dx + dy * dy);
        int n = std::max(2, int(L / 2.2f));
        for (int i = 0; i <= n; i++) {
            float u = float(i) / float(n);
            push(x0 + dx * u, y0 + dy * u, turn);
        }
    };
    auto arc = [&](float cx, float cy, float r, float a0, float a1, int turn) {
        int n = 18;
        for (int i = 0; i <= n; i++) {
            float a = a0 + (a1 - a0) * (float(i) / float(n));
            push(cx + r * std::cos(a), cy + r * std::sin(a), turn);
        }
    };
    // Quay east, left onto the canal, right across the bridge, left into the barn.
    const float r = 18.f;
    line(0.f, 0.f, 78.f, 0.f, 0);
    apex_[1] = int(path_.size()) + 9;
    arc(78.f, r, r, -kPi * 0.5f, 0.f, 1);
    line(96.f, r, 96.f, 88.f, 0);
    apex_[2] = int(path_.size()) + 9;
    arc(96.f + r, 88.f, r, kPi, kPi * 0.5f, 2);
    line(114.f, 106.f, 186.f, 106.f, 0);
    apex_[3] = int(path_.size()) + 9;
    arc(186.f, 106.f + r, r, -kPi * 0.5f, 0.f, 3);
    line(204.f, 124.f, 204.f, 176.f, 0);
    finishI_ = int(path_.size()) - 1;
}

int Game::nearest(int from, float x, float y, int window) const {
    int n = int(path_.size());
    int a = std::max(0, from);
    int b = std::min(n - 1, from + window);
    int best = a;
    float bd = 1e9f;
    for (int i = a; i <= b; i++) {
        float d = len2(path_[size_t(i)].x - x, path_[size_t(i)].y - y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

float Game::railDist(float x, float y) const {
    float best = 1e9f;
    int n = int(path_.size());
    for (int i = 1; i < n; i++) {
        float ax = path_[size_t(i - 1)].x, ay = path_[size_t(i - 1)].y;
        float bx = path_[size_t(i)].x, by = path_[size_t(i)].y;
        float dx = bx - ax, dy = by - ay;
        float L2 = dx * dx + dy * dy;
        float t = L2 > 1e-4f ? std::clamp(((x - ax) * dx + (y - ay) * dy) / L2, 0.f, 1.f) : 0.f;
        float px = ax + dx * t - x, py = ay + dy * t - y;
        best = std::min(best, px * px + py * py);
    }
    return std::sqrt(best);
}

void Game::begin() {
    if (path_.empty()) buildPath();
    x_ = path_[0].x;
    y_ = path_[0].y;
    heading_ = 0.f;
    speed_ = 0.f;
    lean_ = 0.f;
    raceTime_ = 0.f;
    turns_ = 0;
    prog_ = 0;
    won_ = false;
    over_ = false;
    chimeN_ = 0;
    why_[0] = 0;
    std::snprintf(why_, sizeof why_, "running");
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = 2.05f;
    camX_ = 90.f;
    camY_ = 40.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kZoom;
    camX_ = x_;
    camY_ = y_;
    blip(540.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildPath();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.18f, 0.07f);
    if (bot_) {
        begin();
        mode_ = Mode::Run;
        zoom_ = kZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::human(float& steer, float& throttle, float& brake) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    throttle = go && !stop ? 1.f : 0.f;
    brake = stop ? 1.f : 0.f;
}

void Game::pilot(float& steer, float& throttle, float& brake) {
    int n = int(path_.size());
    prog_ = nearest(prog_, x_, y_, 18);
    bool soon = false;
    for (int i = prog_; i < prog_ + 12 && i < n; i++)
        if (path_[size_t(i)].turn) soon = true;
    bool inTurn = path_[size_t(prog_)].turn != 0;
    int aheadN = inTurn ? 5 : int(6 + speed_ * 0.25f);
    int look = std::min(n - 1, prog_ + aheadN);
    float wx = path_[size_t(look)].x - x_;
    float wy = path_[size_t(look)].y - y_;
    float want = std::atan2(wy, wx);
    float err = wrap(want - heading_);
    float coef = 1.9f * std::min(speed_, 10.5f) / (3.6f + speed_ * 0.22f);
    float yawCap = (inTurn ? 12.f : 16.f) / std::max(speed_, 3.f);
    float sCap = coef > 0.05f ? std::min(1.f, yawCap / coef) : 1.f;
    if (std::fabs(lean_) > 0.5f) sCap *= 0.5f;
    steer = std::clamp(err / (inTurn ? 0.3f : 0.48f), -sCap, sCap);
    float target = soon ? 5.8f : 12.8f;
    if (std::fabs(lean_) > 0.58f) target = 4.2f;
    if (prog_ > finishI_ - 6) target = 6.5f;
    throttle = speed_ < target - 0.3f ? 1.f : 0.f;
    brake = speed_ > target + 0.4f ? 1.f : 0.f;
}

void Game::succeed() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    std::snprintf(why_, sizeof why_, "cleared");
    chime(5);
    sys_->setLight(40, 180, 70);
    sys_->rumble(0.1f, 0.25f, 140);
}

void Game::fail(const char* why) {
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(why_, sizeof why_, "%s", why);
    blip(120.f);
    sys_->setLight(180, 30, 20);
    sys_->rumble(0.45f, 0.1f, 180);
}

void Game::physics(float dt, float steer, float throttle, float brake) {
    float accel = throttle * 9.2f - brake * 18.f - speed_ * 0.38f;
    speed_ = std::max(0.f, speed_ + accel * dt);
    if (speed_ > 15.5f) speed_ = 15.5f;
    float yaw = steer * 1.9f * std::min(speed_, 10.5f) / (3.6f + speed_ * 0.22f);
    heading_ = wrap(heading_ + yaw * dt);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;
    float target = (speed_ * yaw) / kTipG;
    float k = 1.f - std::exp(-dt * 7.f);
    lean_ += (target - lean_) * k;

    if (mode_ != Mode::Run) return;
    raceTime_ += dt;

    int here = nearest(std::max(0, prog_ - 2), x_, y_, 24);
    if (here > prog_) prog_ = here;
    for (int t = turns_ + 1; t <= 3; t++) {
        if (prog_ >= apex_[t]) turns_ = t;
    }
    if (turns_ >= 3 && prog_ >= finishI_ - 2) {
        succeed();
        return;
    }
    if (std::fabs(lean_) >= 1.f) {
        fail("tipped the tram");
        return;
    }
    if (raceTime_ > 0.7f && railDist(x_, y_) > kHalf) {
        fail("left the rails");
        return;
    }
    int n = int(path_.size());
    int ghost = int((raceTime_ / kCrew) * float(n - 1));
    if (ghost >= finishI_ && !won_) fail("the other crew finished");
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.055f);
    tone0_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    if (mode_ == Mode::Run && speed_ > 0.3f) {
        float wob = 0.8f + 0.2f * std::sin(t_ * (8.f + speed_ * 0.6f));
        sys_->apu.tone(2, 58.f + speed_ * 7.f, 0.02f * wob);
        sys_->apu.noise(0.006f, 500.f + speed_ * 20.f, false);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
        sys_->apu.noise(0, 0, false);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && crewLeft() < 12.f && crewLeft() > 0.f && !over_) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(crewLeft() < 5.f ? 920.f : 480.f);
            tickT_ = crewLeft() < 5.f ? 0.22f : 0.5f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 523.f, 659.f, 784.f, 1046.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.14f;
            chimeT_ = 0.13f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, thr = 0.f, br = 0.f;
            if (bot_) pilot(steer, thr, br);
            else human(steer, thr, br);
            physics(kDt, steer, thr, br);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (mode_ == Mode::Run) {
        if (std::fabs(lean_) > 0.7f) sys.setLight(170, 40, 24);
        else sys.setLight(30, 80, 50);
    }
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 100.f + 8.f * std::sin(t_ * 0.28f);
        camY_ = 48.f + 4.f * std::cos(t_ * 0.21f);
        zoom_ = 1.85f;
        return;
    }
    float lead = mode_ == Mode::Run ? 6.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 3.6f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kZoom - zoom_) * k;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool hflip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite spt;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    spt.w = int16_t(sw);
    spt.h = int16_t(sh);
    spt.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    spt.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    spt.img = m.pick(float(sh));
    spt.pal = uint8_t(pal);
    spt.shadow = shadow;
    spt.hflip = hflip;
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx, bool hflip) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false, hflip);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H);
        v.lineBackdrop[y] = gs::rgb4(2 + int(u * 2), 5 + int((1.f - u) * 2), 4 + int(u));
        v.lineFog[y] = 0;
        v.road[y] = {};
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 22.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 100.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.tipped;
        if (std::strcmp(why_, "the other crew finished") == 0) msg = &art_.crew;
        else if (std::strcmp(why_, "left the rails") == 0) msg = &art_.rails;
        banner(*msg, 160.f, 28.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.cleared, 160.f, 24.f, PAL_WIN);
    }

    const float sheds[][2] = {{36.f, 22.f}, {120.f, 70.f}, {150.f, 132.f}, {176.f, 70.f}, {60.f, -22.f}};
    for (const float* b : sheds) place(art_.shed, b[0], b[1], 16.f, PAL_YARD, 5.f);

    int n = int(path_.size());
    int step = zoom_ < 3.f ? 4 : 2;
    for (int i = 0; i < n; i += step) place(art_.rail, path_[size_t(i)].x, path_[size_t(i)].y, 8.f, PAL_RAIL, 3.f);
    for (int i = 0; i < n; i += 10) {
        const auto& a = path_[size_t(i)];
        place(art_.pole, a.x + 8.f, a.y + 6.f, 7.f, PAL_WIRE, 3.f);
    }
    for (int t = 1; t <= 3; t++) {
        const auto& a = path_[size_t(apex_[t])];
        place(art_.clock, a.x - 9.f, a.y + 4.f, 4.f, PAL_WIRE, 3.f);
    }
    const auto& fin = path_[size_t(finishI_)];
    place(art_.flag, fin.x, fin.y + 6.f, 7.f, PAL_WIN, 4.f);

    float gU = mode_ == Mode::Title ? 0.42f : std::clamp(raceTime_ / kCrew, 0.f, 1.f);
    int gi = std::min(n - 1, int(gU * float(n - 1)));
    const auto& g = path_[size_t(gi)];
    float gh = 0.f;
    if (gi + 1 < n) gh = std::atan2(path_[size_t(gi + 1)].y - g.y, path_[size_t(gi + 1)].x - g.x);
    place(art_.tram[tramFrame(gh)], g.x, g.y, 7.4f, PAL_CREW, 4.f);

    float side = lean_ * 1.6f;
    float ox = -std::sin(heading_) * side;
    float oy = std::cos(heading_) * side;
    float body = (mode_ == Mode::Title ? 9.f : 8.f) * std::max(zoom_, 0.2f);
    if (mode_ == Mode::Title) body = std::max(body, 28.f);
    float bsx = 160.f + (x_ + ox - camX_) * zoom_;
    float bsy = 112.f - (y_ + oy - camY_) * zoom_;
    const gs::Mipped& hull = art_.tram[tramFrame(heading_)];
    spr(hull, bsx + 2.f, bsy + 3.f, body, PAL_TRAM, true, false);
    spr(hull, bsx, bsy, body, PAL_TRAM, false, false);

    if (mode_ == Mode::Run || mode_ == Mode::Pause || mode_ == Mode::Title) {
        char line[40];
        std::snprintf(line, sizeof line, "TURNS %d/3", std::min(turns_, 3));
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "CREW %4.1f", mode_ == Mode::Title ? kCrew : crewLeft());
        hud(28, 1, line, crewLeft() < 10.f && mode_ != Mode::Title ? PAL_ALERT : PAL_HUD);
        int bars = std::clamp(int(std::fabs(lean_) * 10.f + 0.5f), 0, 10);
        char lean[16];
        lean[0] = 'L';
        lean[1] = ' ';
        for (int i = 0; i < 10; i++) lean[2 + i] = i < bars ? '#' : '-';
        lean[12] = 0;
        hud(1, 26, lean, std::fabs(lean_) > 0.72f ? PAL_ALERT : PAL_HUD);
        if (mode_ == Mode::Title) hudC(24, "THREE TURNS  DO NOT TIP", PAL_HUD);
        else if (turns_ < 3) hud(1, 3, "EASE THE BEND", PAL_HUD);
    }
    if (mode_ == Mode::Win) hudC(26, "AHEAD OF THE OTHER CREW", PAL_WIN);
}

}  // namespace tramturn
