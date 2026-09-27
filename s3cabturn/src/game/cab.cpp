#include "cab.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cabturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kCrew = 48.f;
constexpr float kHalf = 11.5f;
constexpr float kTipG = 24.f;
constexpr float kPlayZoom = 4.7f;

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

int Game::cabFrame(float hdg) const {
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
        int n = 16;
        for (int i = 0; i <= n; i++) {
            float a = a0 + (a1 - a0) * (float(i) / float(n));
            push(cx + r * std::cos(a), cy + r * std::sin(a), turn);
        }
    };
    // East, right onto south, right onto west, right onto north.
    line(4.f, 28.f, 72.f, 28.f, 0);
    apex_[1] = int(path_.size()) + 8;
    arc(72.f, 12.f, 16.f, kPi * 0.5f, 0.f, 1);
    line(88.f, 12.f, 88.f, -48.f, 0);
    apex_[2] = int(path_.size()) + 8;
    arc(72.f, -48.f, 16.f, 0.f, -kPi * 0.5f, 2);
    line(72.f, -64.f, 8.f, -64.f, 0);
    apex_[3] = int(path_.size()) + 8;
    arc(8.f, -48.f, 16.f, -kPi * 0.5f, -kPi, 3);
    line(-8.f, -48.f, -8.f, 8.f, 0);
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

float Game::roadDist(float x, float y) const {
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
    zoom_ = 2.15f;
    camX_ = 70.f;
    camY_ = 16.f;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    blip(620.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildPath();
    sys.apu.setMaster(0.72f);
    sys.apu.setEcho(0.1f, 0.16f, 0.06f);
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
    for (int i = prog_; i < prog_ + 14 && i < n; i++)
        if (path_[size_t(i)].turn) soon = true;
    bool inTurn = path_[size_t(prog_)].turn != 0;
    int aheadN = inTurn ? 5 : int(7 + speed_ * 0.28f);
    int look = std::min(n - 1, prog_ + aheadN);
    float wx = path_[size_t(look)].x - x_;
    float wy = path_[size_t(look)].y - y_;
    float want = std::atan2(wy, wx);
    float err = wrap(want - heading_);
    float coef = 2.05f * std::min(speed_, 11.f) / (3.4f + speed_ * 0.2f);
    float yawCap = (inTurn ? 13.5f : 18.f) / std::max(speed_, 3.f);
    float sCap = coef > 0.05f ? std::min(1.f, yawCap / coef) : 1.f;
    if (std::fabs(lean_) > 0.55f) sCap *= 0.55f;
    steer = std::clamp(err / (inTurn ? 0.32f : 0.5f), -sCap, sCap);
    float target = soon ? 6.15f : 13.6f;
    if (std::fabs(lean_) > 0.62f) target = 4.6f;
    if (prog_ > finishI_ - 8) target = 7.f;
    throttle = speed_ < target - 0.35f ? 1.f : 0.f;
    brake = speed_ > target + 0.45f ? 1.f : 0.f;
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
    blip(140.f);
}

void Game::physics(float dt, float steer, float throttle, float brake) {
    float accel = throttle * 10.5f - brake * 20.f - speed_ * 0.42f;
    speed_ = std::max(0.f, speed_ + accel * dt);
    if (speed_ > 16.5f) speed_ = 16.5f;
    float yaw = steer * 2.05f * std::min(speed_, 11.f) / (3.4f + speed_ * 0.2f);
    heading_ = wrap(heading_ + yaw * dt);
    x_ += std::cos(heading_) * speed_ * dt;
    y_ += std::sin(heading_) * speed_ * dt;
    float target = (speed_ * yaw) / kTipG;
    float k = 1.f - std::exp(-dt * 8.f);
    lean_ += (target - lean_) * k;

    if (mode_ != Mode::Run) return;
    raceTime_ += dt;

    int n = int(path_.size());
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
        fail("tipped the cab");
        return;
    }
    if (raceTime_ > 0.6f && roadDist(x_, y_) > kHalf) {
        fail("left the street");
        return;
    }
    int ghost = int((raceTime_ / kCrew) * float(n - 1));
    if (ghost >= finishI_ && !won_) fail("the other crew finished");
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    tone0_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::audio(float dt) {
    if (mode_ == Mode::Run && speed_ > 0.4f) {
        float wob = 0.75f + 0.25f * std::sin(t_ * (10.f + speed_));
        sys_->apu.tone(2, 70.f + speed_ * 9.f, 0.012f * wob + speed_ * 0.0015f);
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
            blip(crewLeft() < 5.f ? 880.f : 520.f);
            tickT_ = crewLeft() < 5.f ? 0.25f : 0.5f;
        }
    }
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 1318.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.12f;
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
            blip(320.f);
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
        if (a > 0.72f) sys.setLight(180, 40, 20);
        else sys.setLight(40, 90, 40);
    }
    camera();
    audio(kDt);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = 78.f + 6.f * std::sin(t_ * 0.35f);
        camY_ = 14.f;
        zoom_ = 2.2f;
        return;
    }
    float lead = mode_ == Mode::Run ? 5.f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float gz = kPlayZoom;
    float k = 1.f - std::exp(-kDt * 4.f);
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
        v.lineBackdrop[y] = gs::rgb4(2 + int(u * 2), 4 + int((1.f - u) * 2), 3);
        v.lineFog[y] = 0;
        v.road[y] = {};
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 22.f, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160.f, 100.f, PAL_BANNER);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.tipped;
        if (std::strcmp(why_, "the other crew finished") == 0) msg = &art_.crew;
        else if (std::strcmp(why_, "left the street") == 0) msg = &art_.street;
        banner(*msg, 160.f, 28.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.cleared, 160.f, 24.f, PAL_WIN);
    }

    const float blocks[][2] = {{40.f, 46.f}, {100.f, 28.f}, {100.f, -20.f}, {40.f, -82.f}, {-28.f, -64.f}, {-28.f, -10.f}, {50.f, 8.f}};
    for (const float* b : blocks) place(art_.block, b[0], b[1], 14.f, PAL_BLOCK, 6.f);

    int step = zoom_ < 3.f ? 3 : 2;
    for (int i = 0; i < int(path_.size()); i += step) {
        place(art_.road, path_[size_t(i)].x, path_[size_t(i)].y, 9.5f, PAL_ROAD, 4.f);
    }
    for (int t = 1; t <= 3; t++) {
        const auto& a = path_[size_t(apex_[t])];
        place(art_.cone, a.x + 7.f, a.y, 3.2f, PAL_MARK, 3.f);
        place(art_.cone, a.x - 7.f, a.y, 3.2f, PAL_MARK, 3.f);
        place(art_.lamp, a.x + 12.f, a.y + 6.f, 6.f, PAL_MARK, 4.f);
    }
    const auto& fin = path_[size_t(finishI_)];
    place(art_.flag, fin.x, fin.y, 6.f, PAL_WIN, 5.f);

    int n = int(path_.size());
    float gU = mode_ == Mode::Title ? 0.35f : std::clamp(raceTime_ / kCrew, 0.f, 1.f);
    int gi = std::min(n - 1, int(gU * float(n - 1)));
    const auto& g = path_[size_t(gi)];
    float gh = 0.f;
    if (gi + 1 < n) gh = std::atan2(path_[size_t(gi + 1)].y - g.y, path_[size_t(gi + 1)].x - g.x);
    place(art_.cab[cabFrame(gh)], g.x, g.y, 6.2f, PAL_CREW, 4.f);

    float side = lean_ * 1.4f;
    float ox = -std::sin(heading_) * side;
    float oy = std::cos(heading_) * side;
    float cabH = (mode_ == Mode::Title ? 8.5f : 7.2f) * zoom;
    if (mode_ == Mode::Title) cabH = std::max(cabH, 26.f);
    float bsx = 160.f + (x_ + ox - camX_) * zoom;
    float bsy = 112.f - (y_ + oy - camY_) * zoom;
    const gs::Mipped& hull = art_.cab[cabFrame(heading_)];
    spr(hull, bsx + 2.f, bsy + 3.f, cabH, PAL_CAB, true, false);
    spr(hull, bsx, bsy, cabH, PAL_CAB, false, false);

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
        hud(1, 26, lean, std::fabs(lean_) > 0.72f ? PAL_ALERT : PAL_HUD);
        if (mode_ == Mode::Title) hudC(24, "ENTER  THREE TURNS  DO NOT TIP", PAL_HUD);
        else if (turns_ < 3) hud(1, 3, "BRAKE OR THE CAB TIPS", PAL_HUD);
    }
    if (mode_ == Mode::Win) hudC(26, "AHEAD OF THE OTHER CREW", PAL_WIN);
}

}  // namespace cabturn
