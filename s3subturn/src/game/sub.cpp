#include "sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace subturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kCrew = 50.f;
constexpr float kHalf = 12.f;
constexpr float kTipG = 26.f;
constexpr float kPlayZoom = 4.6f;

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

int Game::subFrame(float hdg) const {
    float u = std::fmod(hdg, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

void Game::buildPath() {
    path_.clear();
    auto push = [&](float x, float y, int turn) {
        if (!path_.empty() && len2(path_.back().x - x, path_.back().y - y) < 0.36f) return;
        path_.push_back({x, y, turn});
    };
    auto line = [&](float x0, float y0, float x1, float y1, int turn) {
        float dx = x1 - x0, dy = y1 - y0;
        float L = std::sqrt(dx * dx + dy * dy);
        int n = std::max(2, int(L / 2.f));
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
    // Flooded cut: east, starboard to south, starboard to west, starboard to north.
    line(6.f, 30.f, 70.f, 30.f, 0);
    apex_[1] = int(path_.size()) + 9;
    arc(70.f, 12.f, 18.f, kPi * 0.5f, 0.f, 1);
    line(88.f, 12.f, 88.f, -46.f, 0);
    apex_[2] = int(path_.size()) + 9;
    arc(70.f, -46.f, 18.f, 0.f, -kPi * 0.5f, 2);
    line(70.f, -64.f, 10.f, -64.f, 0);
    apex_[3] = int(path_.size()) + 9;
    arc(10.f, -46.f, 18.f, -kPi * 0.5f, -kPi, 3);
    line(-8.f, -46.f, -8.f, 6.f, 0);
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

float Game::trenchDist(float x, float y) const {
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
    zoom_ = 2.1f;
    camX_ = 68.f;
    camY_ = 16.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    blip(420.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildPath();
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.18f, 0.22f, 0.08f);
    if (bot_) {
        begin();
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
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
    for (int i = prog_; i < prog_ + 16 && i < n; i++)
        if (path_[size_t(i)].turn) soon = true;
    bool inTurn = path_[size_t(prog_)].turn != 0;
    int aheadN = inTurn ? 6 : int(8 + speed_ * 0.22f);
    int look = std::min(n - 1, prog_ + aheadN);
    float wx = path_[size_t(look)].x - x_;
    float wy = path_[size_t(look)].y - y_;
    float want = std::atan2(wy, wx);
    float err = wrap(want - heading_);
    float coef = 1.85f * std::min(speed_, 10.f) / (3.6f + speed_ * 0.18f);
    float yawCap = (inTurn ? 12.f : 16.f) / std::max(speed_, 3.f);
    float sCap = coef > 0.05f ? std::min(1.f, yawCap / coef) : 1.f;
    if (std::fabs(lean_) > 0.5f) sCap *= 0.5f;
    steer = std::clamp(err / (inTurn ? 0.34f : 0.52f), -sCap, sCap);
    float target = soon ? 5.6f : 12.2f;
    if (std::fabs(lean_) > 0.55f) target = 4.2f;
    if (prog_ > finishI_ - 10) target = 6.5f;
    throttle = speed_ < target - 0.3f ? 1.f : 0.f;
    brake = speed_ > target + 0.4f ? 1.f : 0.f;
}

void Game::succeed() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    std::snprintf(why_, sizeof why_, "cleared");
    chime(5);
}

void Game::fail(const char* why) {
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    std::snprintf(why_, sizeof why_, "%s", why);
    blip(90.f);
}

void Game::physics(float dt, float steer, float throttle, float brake) {
    float accel = throttle * 9.2f - brake * 18.f - speed_ * 0.38f;
    speed_ = std::max(0.f, speed_ + accel * dt);
    if (speed_ > 15.f) speed_ = 15.f;
    float yaw = steer * 1.85f * std::min(speed_, 10.f) / (3.6f + speed_ * 0.18f);
    heading_ = wrap(heading_ + yaw * dt);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;
    float target = (speed_ * yaw) / kTipG;
    float k = 1.f - std::exp(-dt * 7.f);
    lean_ += (target - lean_) * k;

    if (mode_ != Mode::Run) return;
    raceTime_ += dt;

    int n = int(path_.size());
    int here = nearest(std::max(0, prog_ - 2), x_, y_, 26);
    if (here > prog_) prog_ = here;
    for (int t = turns_ + 1; t <= 3; t++) {
        if (prog_ >= apex_[t]) turns_ = t;
    }
    if (turns_ >= 3 && prog_ >= finishI_ - 2) {
        succeed();
        return;
    }
    if (std::fabs(lean_) >= 1.f) {
        fail("tipped the sub");
        return;
    }
    if (raceTime_ > 0.7f && trenchDist(x_, y_) > kHalf) {
        fail("left the trench");
        return;
    }
    int ghost = int((raceTime_ / kCrew) * float(n - 1));
    if (ghost >= finishI_ && !won_) fail("the other crew finished");
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    tone0_ = 0.14f;
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    if (mode_ == Mode::Run && speed_ > 0.4f) {
        float wob = 0.8f + 0.2f * std::sin(t_ * (6.f + speed_ * 0.4f));
        sys_->apu.tone(2, 48.f + speed_ * 6.f, 0.018f * wob);
    } else if (tone0_ <= 0.f) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (mode_ == Mode::Run && crewLeft() < 12.f && crewLeft() > 0.f && !over_) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) {
            blip(crewLeft() < 5.f ? 660.f : 330.f);
            tickT_ = crewLeft() < 5.f ? 0.28f : 0.55f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
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
            blip(220.f);
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
        float a = std::fabs(lean_);
        if (a > 0.7f) sys.setLight(160, 50, 20);
        else sys.setLight(20, 70, 80);
    }
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 76.f + 5.f * std::sin(t_ * 0.28f);
        camY_ = 14.f;
        zoom_ = 2.15f;
        return;
    }
    float lead = mode_ == Mode::Run ? 4.5f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    float k = 1.f - std::exp(-kDt * 3.5f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (gz - zoom_) * k;
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
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H);
        v.lineBackdrop[y] = gs::rgb4(1, 2 + int((1.f - u) * 3), 4 + int(u * 4));
        v.lineFog[y] = uint8_t(2 + int(u * 3));
        v.road[y] = {};
    }
    v.setFogColor(gs::rgb4(1, 3, 5));

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 22.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 100.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.tipped;
        if (std::strcmp(why_, "the other crew finished") == 0) msg = &art_.crew;
        else if (std::strcmp(why_, "left the trench") == 0) msg = &art_.trench;
        banner(*msg, 160.f, 28.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.cleared, 160.f, 24.f, PAL_WIN);
    }

    const float wrecks[][2] = {{42.f, 48.f}, {104.f, 24.f}, {104.f, -22.f}, {36.f, -80.f}, {-26.f, -62.f}, {-26.f, -8.f}};
    for (const float* b : wrecks) place(art_.wreck, b[0], b[1], 10.f, PAL_KELP, 5.f);
    const float kelp[][2] = {{28.f, 40.f}, {58.f, 18.f}, {78.f, -8.f}, {96.f, -40.f}, {48.f, -74.f}, {18.f, -50.f}, {-18.f, -20.f}};
    for (const float* k : kelp) place(art_.kelp, k[0], k[1], 7.f, PAL_KELP, 4.f);

    int step = zoom_ < 3.f ? 3 : 2;
    for (int i = 0; i < int(path_.size()); i += step)
        place(art_.sand, path_[size_t(i)].x, path_[size_t(i)].y, 10.f, PAL_TRENCH, 4.f);

    for (int t = 1; t <= 3; t++) {
        const auto& a = path_[size_t(apex_[t])];
        place(art_.buoy, a.x + 8.f, a.y, 3.4f, PAL_MARK, 3.f);
        place(art_.buoy, a.x - 8.f, a.y, 3.4f, PAL_MARK, 3.f);
    }
    const auto& fin = path_[size_t(finishI_)];
    place(art_.flag, fin.x, fin.y, 6.f, PAL_WIN, 5.f);

    int n = int(path_.size());
    float gU = mode_ == Mode::Title ? 0.32f : std::clamp(raceTime_ / kCrew, 0.f, 1.f);
    int gi = std::min(n - 1, int(gU * float(n - 1)));
    const auto& g = path_[size_t(gi)];
    float gh = 0.f;
    if (gi + 1 < n) gh = std::atan2(path_[size_t(gi + 1)].y - g.y, path_[size_t(gi + 1)].x - g.x);
    place(art_.sub[subFrame(gh)], g.x, g.y, 6.4f, PAL_CREW, 4.f);

    float side = lean_ * 1.6f;
    float ox = -std::sin(heading_) * side;
    float oy = std::cos(heading_) * side;
    float hullH = (mode_ == Mode::Title ? 9.f : 7.4f) * zoom;
    if (mode_ == Mode::Title) hullH = std::max(hullH, 28.f);
    float bsx = 160.f + (x_ + ox - camX_) * zoom;
    float bsy = 112.f - (y_ + oy - camY_) * zoom;
    const gs::Mipped& hull = art_.sub[subFrame(heading_)];
    spr(hull, bsx + 2.f, bsy + 3.f, hullH, PAL_SUB, true, false);
    spr(hull, bsx, bsy, hullH, PAL_SUB, false, false);
    if (speed_ > 1.f) {
        float bx = 160.f + (x_ - std::cos(heading_) * 4.2f - camX_) * zoom;
        float by = 112.f - (y_ - std::sin(heading_) * 4.2f - camY_) * zoom;
        float bob = 2.f + std::fmod(t_ * speed_, 3.f);
        spr(art_.bubble, bx, by - bob, 6.f + bob, PAL_MARK, false, false);
    }

    if (mode_ == Mode::Run || mode_ == Mode::Pause || mode_ == Mode::Title) {
        char line[40];
        std::snprintf(line, sizeof line, "TURNS %d/3", std::min(turns_, 3));
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "CREW %4.1f", mode_ == Mode::Title ? kCrew : crewLeft());
        hud(28, 1, line, crewLeft() < 10.f && mode_ != Mode::Title ? PAL_ALERT : PAL_HUD);
        int bars = int(std::fabs(lean_) * 10.f + 0.5f);
        bars = std::clamp(bars, 0, 10);
        char lean[16];
        lean[0] = 'L';
        lean[1] = ' ';
        for (int i = 0; i < 10; i++) lean[2 + i] = i < bars ? '#' : '-';
        lean[12] = 0;
        hud(1, 26, lean, std::fabs(lean_) > 0.7f ? PAL_ALERT : PAL_HUD);
        if (mode_ == Mode::Title) hudC(24, "ENTER  THREE TURNS  DO NOT TIP", PAL_HUD);
        else if (turns_ < 3) hud(1, 3, "EASE THE SUB OR IT TIPS", PAL_HUD);
    }
    if (mode_ == Mode::Win) hudC(26, "AHEAD OF THE OTHER CREW", PAL_WIN);
}

}  // namespace subturn
