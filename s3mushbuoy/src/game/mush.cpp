#include "game/mush.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mush {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kNorth = 1.5707963f;
constexpr float kMax = 16.5f;
constexpr float kCrew = 9.4f;
constexpr float kPlayZoom = 1.42f;
constexpr float kTitleZoom = 0.62f;

struct Way {
    float x, y;
    int legAfter;
};

// Leave each buoy to port: east of red, north of green, west of amber, then the same slip.
const Way kWay[] = {
    {124.f, 72.f, 0},
    {124.f, 132.f, 1},
    {28.f, 208.f, 1},
    {-28.f, 208.f, 2},
    {-124.f, 132.f, 2},
    {-124.f, 68.f, 3},
    {0.f, 52.f, 3},
};
constexpr int kWayN = 7;

struct Mark {
    float x, y;
    int pal;
    const char* name;
};

const Mark kMarks[3] = {
    {86.f, 102.f, PAL_RED, "RED"},
    {0.f, 176.f, PAL_GREEN, "GREEN"},
    {-86.f, 102.f, PAL_AMBER, "AMBER"},
};

struct Node {
    float x, y;
};

const Node kPath[] = {
    {0.f, 18.f}, {124.f, 72.f}, {124.f, 132.f}, {28.f, 208.f}, {-28.f, 208.f},
    {-124.f, 132.f}, {-124.f, 68.f}, {0.f, 52.f}, {0.f, 16.f},
};
constexpr int kPathN = 9;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

float pathLength() {
    float n = 0;
    for (int i = 0; i < kPathN - 1; i++) n += std::hypot(kPath[i + 1].x - kPath[i].x, kPath[i + 1].y - kPath[i].y);
    return n;
}

}  // namespace

float Game::speed() const { return std::hypot(vx_, vy_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (leg_ >= 3) return 3;
    if (leg_ >= 1) return 2;
    return 1;
}

void Game::begin() {
    x_ = 0.f;
    y_ = 18.f;
    heading_ = kNorth;
    vx_ = vy_ = 0;
    leg_ = 0;
    wp_ = 0;
    raceTime_ = 0;
    crewDist_ = 0;
    throttle_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    decoyT_ = 0;
    won_ = false;
    over_ = false;
    why_[0] = 0;
    chimeN_ = 0;
    chimeStep_ = 0;
    puffCursor_ = 0;
    for (Puff& p : spray_) p.life = 0;
    pathLen_ = pathLength();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = 0.f;
    camY_ = 112.f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 11, 13));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.16f, 0.2f, 0.08f);
    begin();
    if (bot_) {
        mode_ = Mode::Run;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::controls(float& steer, float& throttle) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    throttle = 0;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO)) throttle = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) throttle = -1.f;
    if (p.accel > 0.12f) throttle = p.accel;
    if (p.brake > 0.12f) throttle = -p.brake;
    if (std::fabs(p.axisY) > 0.2f) throttle = std::clamp(p.axisY, -1.f, 1.f);
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z)) yip();
}

void Game::pilot(float& steer, float& throttle) {
    float tx = 0.f, ty = 14.f;
    bool dock = leg_ >= 3 && wp_ >= kWayN;
    if (!dock) {
        const Way& w = kWay[std::clamp(wp_, 0, kWayN - 1)];
        tx = w.x;
        ty = w.y;
    }
    float err = wrap(std::atan2(ty - y_, tx - x_) - heading_);
    float dist = std::hypot(tx - x_, ty - y_);
    steer = std::clamp(err / 0.42f, -1.f, 1.f);
    throttle = std::fabs(err) > 0.9f ? 0.25f : 1.f;
    if (std::fabs(err) > 1.5f) throttle = 0.05f;
    if (dock) {
        if (dist < 26.f) throttle = speed() > 3.2f ? -1.f : 0.35f;
        if (inHome() && speed() > 1.4f) throttle = -1.f;
        if (inHome() && speed() <= 1.4f) throttle = 0.f;
    }
}

void Game::physics(float dt, float steer, float throttle) {
    throttle_ = throttle;
    float c = std::cos(heading_), s = std::sin(heading_);
    float along = vx_ * c + vy_ * s;
    float lat = -vx_ * s + vy_ * c;
    if (throttle > 0.f) along += throttle * 13.5f * dt;
    else if (throttle < 0.f) along += throttle * 22.f * dt;
    along -= along * 0.42f * dt;
    lat -= lat * 4.8f * dt;
    along = std::clamp(along, -4.f, kMax);
    vx_ = c * along - s * lat;
    vy_ = s * along + c * lat;
    float yaw = steer * (1.35f + std::fabs(along) * 0.045f);
    heading_ = wrap(heading_ + yaw * dt);
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    bool homeMouth = std::fabs(x_) < 14.f && y_ < 30.f;
    bool decoyMouth = x_ > -92.f && x_ < -48.f && y_ < 28.f;
    if (y_ < 6.f && !homeMouth && !decoyMouth) {
        y_ = 6.f;
        if (vy_ < 0.f) vy_ *= -0.2f;
        vx_ *= 0.6f;
    }
    if (y_ > 230.f) {
        y_ = 230.f;
        if (vy_ > 0.f) vy_ = 0;
    }
    if (std::fabs(x_) > 168.f) {
        x_ = std::clamp(x_, -168.f, 168.f);
        vx_ = 0;
    }

    if (speed() > 6.f && throttle > 0.4f) {
        Puff& p = spray_[puffCursor_];
        p.x = x_ - c * 6.f;
        p.y = y_ - s * 6.f;
        p.life = 0.45f;
        puffCursor_ = (puffCursor_ + 1) % 16;
    }
    for (Puff& p : spray_) p.life -= dt;
}

bool Game::inHome() const { return std::fabs(x_) <= 11.f && y_ >= 4.f && y_ <= 26.f; }

bool Game::inDecoy() const { return x_ <= -50.f && x_ >= -86.f && y_ >= 4.f && y_ <= 24.f; }

void Game::advanceMarks() {
    if (wp_ < kWayN) {
        const Way& w = kWay[wp_];
        if (std::hypot(w.x - x_, w.y - y_) < 20.f) {
            if (w.legAfter > leg_) {
                leg_ = w.legAfter;
                yip();
            }
            wp_++;
        }
    }
    if (inDecoy() && speed() < 3.f) {
        decoyT_ += kDt;
        if (decoyT_ > 0.35f) fail("the other dock");
    } else {
        decoyT_ = 0;
    }
    if (leg_ >= 3 && inHome() && speed() < 1.7f) finish();
}

void Game::crewAt(float dist, float& ox, float& oy, float& oh) const {
    float left = std::clamp(dist, 0.f, pathLen_);
    for (int i = 0; i < kPathN - 1; i++) {
        float dx = kPath[i + 1].x - kPath[i].x;
        float dy = kPath[i + 1].y - kPath[i].y;
        float seg = std::hypot(dx, dy);
        if (left <= seg || i == kPathN - 2) {
            float u = seg > 0.01f ? std::clamp(left / seg, 0.f, 1.f) : 1.f;
            ox = kPath[i].x + dx * u;
            oy = kPath[i].y + dy * u;
            oh = std::atan2(dy, dx);
            return;
        }
        left -= seg;
    }
    ox = kPath[kPathN - 1].x;
    oy = kPath[kPathN - 1].y;
    oh = -kNorth;
}

void Game::moveCrew(float dt) {
    if (mode_ != Mode::Run) return;
    crewDist_ += kCrew * dt;
    if (crewDist_ >= pathLen_ && !won_) fail("the other crew took the dock");
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    vx_ = vy_ = 0;
    std::snprintf(why_, sizeof(why_), "same dock");
    chime(5);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof(why_), "%s", why);
    chime(2);
}

void Game::yip() {
    yipT_ = 0.18f;
    sys_->apu.tone(2, 680.f, 0.08f);
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt) {
    if (yipT_ > 0.f) {
        yipT_ -= dt;
        if (yipT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    float rush = std::clamp(speed() / kMax, 0.f, 1.f);
    bool run = mode_ == Mode::Run;
    tone0_ = run ? 46.f + rush * 28.f : 0.f;
    sys_->apu.tone(0, tone0_, run && throttle_ > 0.2f ? 0.03f + rush * 0.02f : 0.f);
    if (chimeN_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 880.f, 1046.f};
            int n = chimeStep_ < 5 ? chimeStep_ : 4;
            sys_->apu.tone(1, notes[mode_ == Mode::Win ? n : 0], 0.07f);
            chimeStep_++;
            chimeT_ = 0.16f;
            if (chimeStep_ >= chimeN_) {
                chimeN_ = 0;
                sys_->apu.tone(1, 0, 0);
            }
        }
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    float x = 8.f + float(col) * 8.f;
    float y = 6.f + float(row) * 10.f;
    for (const char* p = s; *p; ++p) {
        int gi = int(*p) - 32;
        if (gi >= 0 && gi < 96) spr(art_.glyph[gi], x, y, 8.f, pal, false);
        x += 8.f;
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    int col = std::max(0, (40 - n) / 2);
    hud(col, row, s, pal);
}

void Game::drawHud() {
    if (mode_ == Mode::Title) {
        hudC(18, "LEFT RIGHT STEER", PAL_HUD);
        hudC(19, "UP MUSH   DOWN WHOA", PAL_HUD);
        hudC(21, "BEAT THE OTHER CREW HOME", PAL_HUD);
        return;
    }
    std::snprintf(line_, sizeof(line_), "LEG %d/3", std::min(leg_, 3));
    hud(1, 0, line_, PAL_HUD);
    float left = std::max(0.f, (pathLen_ - crewDist_) / kCrew);
    std::snprintf(line_, sizeof(line_), "CREW %4.1f", left);
    hud(28, 0, line_, left < 12.f ? PAL_ALERT : PAL_HUD);
    const char* order = "COAST";
    if (throttle_ > 0.55f) order = "MUSH";
    else if (throttle_ > 0.08f) order = "EASY";
    else if (throttle_ < -0.4f) order = "WHOA";
    hud(1, 1, order, PAL_HUD);
    if (leg_ < 3) {
        std::snprintf(line_, sizeof(line_), "LEAVE %s TO PORT", kMarks[std::min(leg_, 2)].name);
        hud(1, 20, line_, PAL_HUD);
    } else {
        hud(1, 20, "SAME DOCK. STOP.", PAL_HUD);
    }
}

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    spr(m, sx, sy, worldH * zoom_, pal, false);
}

int Game::sledFrame(float heading) const {
    float u = heading;
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    const uint16_t snow = gs::rgb4(13, 14, 15);
    const uint16_t ice = gs::rgb4(8, 12, 13);
    const uint16_t far = gs::rgb4(4, 8, 11);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - float(y)) / std::max(zoom_, 0.05f);
        uint16_t c = wy < 8.f ? lerpC(snow, ice, std::clamp((wy + 20.f) / 28.f, 0.f, 1.f))
                              : lerpC(ice, far, std::clamp((wy - 8.f) / 220.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    float follow = mode_ == Mode::Title ? 0.f : 0.08f;
    float wantX = mode_ == Mode::Title ? 0.f : x_;
    float wantY = mode_ == Mode::Title ? 112.f : y_;
    float wantZ = mode_ == Mode::Title ? kTitleZoom : kPlayZoom;
    camX_ += (wantX - camX_) * (mode_ == Mode::Title ? 1.f : follow);
    camY_ += (wantY - camY_) * (mode_ == Mode::Title ? 1.f : follow);
    zoom_ += (wantZ - zoom_) * (mode_ == Mode::Title ? 1.f : 0.06f);

    place(art_.dock, 0.f, 16.f, 34.f, PAL_DOCK);
    place(art_.flag, 0.f, 30.f, 16.f, PAL_FLAG);
    place(art_.shed, -68.f, 14.f, 22.f, PAL_SHED);
    place(art_.flag, -68.f, 26.f, 12.f, PAL_ALERT);
    for (int i = -3; i <= 3; i++) {
        if (i == 0) continue;
        place(art_.tree, float(i) * 28.f, -2.f, 18.f, PAL_TREE);
    }
    place(art_.tree, -120.f, 4.f, 20.f, PAL_TREE);
    place(art_.tree, 120.f, 4.f, 20.f, PAL_TREE);
    place(art_.lamp, 16.f, 22.f, 14.f, PAL_LAMP);
    place(art_.lamp, -16.f, 22.f, 14.f, PAL_LAMP);

    for (const Mark& m : kMarks) place(art_.buoy, m.x, m.y, 18.f, m.pal);

    float cx, cy, ch;
    crewAt(crewDist_, cx, cy, ch);
    if (mode_ != Mode::Title) place(art_.sled[sledFrame(ch)], cx, cy, 16.f, PAL_CREW);
    for (const Puff& p : spray_) {
        if (p.life > 0.f) place(art_.puff, p.x, p.y, 6.f + (0.45f - p.life) * 8.f, PAL_SNOW);
    }
    place(art_.sled[sledFrame(heading_)], x_, y_, 22.f, PAL_TEAM);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 28, float(art_.title.h), PAL_BANNER, false);
        spr(art_.sub, 160, 52, float(art_.sub.h) * 0.7f, PAL_BANNER, false);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160, 100, float(art_.paused.h), PAL_BANNER, false);
    } else if (mode_ == Mode::Win) {
        spr(art_.home, 160, 78, float(art_.home.h), PAL_WIN, false);
        spr(art_.beat, 160, 104, float(art_.beat.h), PAL_WIN, false);
    } else if (mode_ == Mode::Fail) {
        spr(art_.missed, 160, 90, float(art_.missed.h), PAL_ALERT, false);
    }
    drawHud();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Run;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
        }
        draw();
        audio(kDt);
        return;
    }
    if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) showTitle();
        draw();
        audio(kDt);
        return;
    }

    if (p.pressed(gs::BTN_START) && !bot_) {
        mode_ = Mode::Pause;
        draw();
        return;
    }

    float steer = 0, throttle = 0;
    if (bot_) pilot(steer, throttle);
    else controls(steer, throttle);
    physics(kDt, steer, throttle);
    advanceMarks();
    moveCrew(kDt);
    raceTime_ += kDt;

    float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
    if (moved < 1.2f && leg_ < 3) stuckT_ += kDt;
    else {
        stuckT_ = 0;
        stuckX_ = x_;
        stuckY_ = y_;
    }
    if (stuckT_ > 8.f) fail("the team stalled");

    audio(kDt);
    draw();
}

}  // namespace mush
