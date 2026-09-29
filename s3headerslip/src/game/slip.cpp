#include "slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace headerslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = -30.f;
constexpr float kStartY = -3.2f;
constexpr float kStartH = 0.22f;

constexpr float kMouth = 12.f;
constexpr float kHead = 40.f;
constexpr float kPier0 = 12.f;
constexpr float kPier1 = 48.f;
constexpr float kSouth0 = -6.5f;
constexpr float kSouth1 = -3.15f;
constexpr float kNorth0 = 3.15f;
constexpr float kNorth1 = 6.5f;
constexpr float kBerthX0 = 30.f;
constexpr float kBerthX1 = 36.5f;
constexpr float kBerthY = 1.55f;
constexpr float kParkX = 33.2f;

constexpr float kTide = 36.f;
constexpr float kCrewLine = 26.f;
constexpr float kZoom = 5.1f;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

bool pierAt(float x, float y) {
    if (x >= kHead && x <= kPier1 && y >= kSouth0 && y <= kNorth1) return true;
    if (x >= kPier0 && x <= kPier1 && y >= kSouth0 && y <= kSouth1) return true;
    if (x >= kPier0 && x <= kPier1 && y >= kNorth0 && y <= kNorth1) return true;
    return false;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

float Game::crewShort() const {
    float crew = race_ * 0.82f + penalty_;
    return std::max(0.f, kCrewLine - crew);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (x_ > kMouth && std::fabs(y_) < 3.f) return 2;
    return 1;
}

int Game::hullFrame(float h) const {
    float u = std::fmod(h, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 8.f)) % 8;
    if (i < 0) i += 8;
    return i;
}

const char* Game::hint() const {
    if (headerLive_) return "A  TAKE THE HEADER";
    if (hold_ > 0.f) return "HOLD THE BERTH";
    if (x_ > kMouth) return "EASE HER IN THE SLIP";
    if (phase_ == Phase::Lift) return "THE LIFT IS YOURS";
    return "THE OTHER CREW IS THE CLOCK";
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 6.2f;
    throttle_ = 0.8f;
    race_ = 0.f;
    call_ = 0.f;
    lift_ = 0.f;
    hold_ = 0.f;
    penalty_ = 0.f;
    scrapes_ = 0;
    phase_ = Phase::Lay;
    headerLive_ = false;
    won_ = false;
    over_ = false;
    std::snprintf(why_, sizeof why_, "running");
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    speed_ = 0.f;
    camX_ = 8.f;
    camY_ = 0.f;
    zoom_ = 3.4f;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    if (bot_) {
        begin();
        mode_ = Mode::Run;
        camX_ = x_;
        camY_ = y_;
        zoom_ = kZoom;
    } else {
        showTitle();
    }
}

void Game::succeed() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    sys_->apu.tone(0, 523.f, 0.12f);
    sys_->apu.tone(1, 659.f, 0.08f);
    toneT_ = 0.45f;
    sys_->rumble(0.2f, 0.4f, 120);
    sys_->setLight(40, 170, 80);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.35f, 90.f, 0.35f);
    sys_->apu.tone(0, 70.f, 0.08f);
    toneT_ = 0.35f;
    sys_->rumble(0.45f, 0.15f, 140);
    sys_->setLight(160, 30, 20);
}

void Game::human(float& steer, float& throttle, bool& tack) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.2f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (stop) throttle_ = std::max(-1.f, throttle_ - kDt * 2.2f);
    else if (go) throttle_ = std::min(1.f, throttle_ + kDt * 1.3f);
    else {
        if (throttle_ > 0.f) throttle_ = std::max(0.f, throttle_ - kDt * 0.35f);
        else throttle_ = std::min(0.f, throttle_ + kDt * 0.8f);
    }
    throttle = throttle_;
    tack = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Y);
}

void Game::pilot(float& steer, float& throttle, bool& tack) {
    tack = headerLive_;
    float tx = 4.f, ty = -0.4f;
    if (phase_ == Phase::Lift || phase_ == Phase::Header2) {
        tx = 20.f;
        ty = 0.f;
    } else if (phase_ == Phase::Final) {
        tx = kParkX;
        ty = 0.f;
    }
    float want = std::atan2(ty - y_, tx - x_);
    float err = wrap(want - heading_);
    steer = std::clamp(err / 0.28f, -1.f, 1.f);
    if (x_ < kParkX - 1.4f) throttle = x_ < 26.f ? 1.f : 0.7f;
    else if (speed_ > 0.28f) throttle = -1.f;
    else if (x_ < kParkX - 0.3f) throttle = 0.45f;
    else throttle = 0.f;
    if (std::fabs(err) > 0.9f) throttle = std::min(throttle, 0.2f);
}

void Game::tick(float steer, float throttle, bool tack) {
    race_ += kDt;
    t_ += kDt;
    if (lift_ > 0.f) lift_ -= kDt;
    if (call_ > 0.f) call_ -= kDt;

    if (phase_ == Phase::Lay && race_ >= 2.15f) {
        phase_ = Phase::Header1;
        headerLive_ = true;
        call_ = 2.05f;
        sys_->apu.tone(2, 880.f, 0.07f);
    } else if ((phase_ == Phase::Lift) && x_ >= 8.5f) {
        phase_ = Phase::Header2;
        headerLive_ = true;
        call_ = 2.05f;
        sys_->apu.tone(2, 740.f, 0.07f);
    }

    if (headerLive_ && tack) {
        float tx = phase_ == Phase::Header1 ? 18.f : kParkX;
        heading_ = std::atan2(0.f - y_, tx - x_);
        headerLive_ = false;
        call_ = 0.f;
        lift_ = 1.15f;
        phase_ = phase_ == Phase::Header1 ? Phase::Lift : Phase::Final;
        speed_ = std::max(speed_, 7.4f);
        sys_->apu.tone(0, 392.f, 0.1f);
        toneT_ = 0.18f;
        sys_->rumble(0.15f, 0.25f, 60);
    } else if (headerLive_ && call_ <= 0.f) {
        headerLive_ = false;
        penalty_ += 8.f;
        y_ -= phase_ == Phase::Header1 ? 2.4f : -1.6f;
        phase_ = phase_ == Phase::Header1 ? Phase::Lift : Phase::Final;
        sys_->apu.tone(0, 140.f, 0.08f);
        toneT_ = 0.2f;
    }

    if (headerLive_) heading_ += (phase_ == Phase::Header1 ? -0.55f : 0.42f) * kDt;

    heading_ = wrap(heading_ + steer * 1.55f * kDt);
    float cap = headerLive_ ? 4.4f : (lift_ > 0.f ? 9.4f : 8.1f);
    float want = throttle >= 0.f ? throttle * cap : throttle * 3.6f;
    speed_ += (want - speed_) * kDt * 2.4f;

    float nx = x_ + std::cos(heading_) * speed_ * kDt;
    float ny = y_ + std::sin(heading_) * speed_ * kDt;
    float bx = nx + std::cos(heading_) * 2.1f;
    float by = ny + std::sin(heading_) * 2.1f;
    if (pierAt(nx, ny) || pierAt(bx, by)) {
        speed_ = std::max(0.f, speed_) * 0.15f;
        if (ny > 0.f) y_ -= 0.08f;
        else y_ += 0.08f;
        if (nx > kHead - 1.f) x_ -= 0.12f;
        scrapes_++;
        if (scrapes_ >= 4) {
            fail("hit the head wall");
            return;
        }
    } else {
        x_ = nx;
        y_ = ny;
    }

    if (x_ < kMouth && std::fabs(y_) > 13.5f) {
        fail("aground");
        return;
    }
    if (race_ >= kTide) {
        fail("the tide turned");
        return;
    }
    float crew = race_ * 0.82f + penalty_;
    bool inBerth = x_ >= kBerthX0 && x_ <= kBerthX1 && std::fabs(y_) <= kBerthY && std::fabs(wrap(heading_)) < 0.62f;
    if (inBerth && std::fabs(speed_) < 0.38f) hold_ += kDt;
    else hold_ = 0.f;
    if (hold_ >= 0.4f) {
        succeed();
        return;
    }
    if (crew >= kCrewLine && hold_ < 0.4f) fail("the other crew took the slip");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (toneT_ > 0.f) {
        toneT_ -= kDt;
        if (toneT_ <= 0.f) {
            sys.apu.tone(0, 0.f, 0.f);
            sys.apu.tone(1, 0.f, 0.f);
            sys.apu.tone(2, 0.f, 0.f);
        }
    }
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
            begin();
            mode_ = Mode::Run;
            zoom_ = kZoom;
            camX_ = x_;
            camY_ = y_;
        }
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        t_ += kDt;
        if (!bot_ && p.pressed(gs::BTN_START)) showTitle();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
        draw();
        return;
    }
    if (!bot_ && p.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    float steer = 0.f, throttle = 0.f;
    bool tack = false;
    if (bot_) pilot(steer, throttle, tack);
    else human(steer, throttle, tack);
    tick(steer, throttle, tack);
    float follow = mode_ == Mode::Run ? 0.08f : 1.f;
    camX_ += (x_ - camX_) * follow;
    camY_ += (y_ * 0.65f - camY_) * follow;
    zoom_ = kZoom;
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char ch = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || ch < 32 || ch > 127 || ch == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[ch - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
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
    sys_->vdp.sprite(spt);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx = 160.f + (wx - camX_) * zoom_;
    float sy = 112.f - (wy - camY_) * zoom_;
    spr(m, sx, sy, std::max(worldH * zoom_, 2.f), pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    v.roadTime = int(t_ * 24.f);
    v.setFogColor(gs::rgb4(5, 8, 9));
    float tide = std::clamp(race_ / kTide, 0.f, 1.f);
    float zoom = std::max(zoom_, 0.2f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / zoom;
        float u = std::clamp((wy + 18.f) / 36.f, 0.f, 1.f);
        uint16_t water = lerpC(gs::rgb4(2, 6, 9), gs::rgb4(1, 3, 6), 1.f - u);
        water = lerpC(water, gs::rgb4(6, 6, 3), tide * 0.45f);
        if (std::fabs(wy) > 8.f) water = lerpC(water, gs::rgb4(3, 5, 2), std::clamp((std::fabs(wy) - 8.f) / 6.f, 0.f, 1.f));
        v.lineBackdrop[y] = water;
        v.lineFog[y] = uint8_t(std::clamp(int((std::fabs(wy - camY_) - 18.f) * 0.12f), 0, 5));
        gs::RoadLine& r = v.road[y];
        r = {};
        if (wy > kSouth1 && wy < kNorth0) {
            float sx0 = 160.f + (kMouth - camX_) * zoom;
            float sx1 = 160.f + (kHead - camX_) * zoom;
            r.on = true;
            r.cx = (sx0 + sx1) * 0.5f;
            r.hw = std::max(3.f, std::fabs(sx1 - sx0) * 0.5f);
            r.v = camX_ * 4.f + t_ * 6.f;
            r.pal = uint8_t(PAL_WATER);
            r.band = (int(std::floor(wy)) & 1) ? 1 : 0;
            r.style = 2;
            r.left = gs::GROUND_LAND;
            r.right = gs::GROUND_LAND;
        }
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal); };
    if (mode_ == Mode::Title) banner(art_.title, 160.f, 22.f, PAL_BANNER);
    else if (mode_ == Mode::Win) banner(art_.berthed, 160.f, 22.f, PAL_WIN);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped* msg = &art_.crew;
        if (std::strcmp(why_, "the tide turned") == 0) msg = &art_.tide;
        else if (std::strcmp(why_, "hit the head wall") == 0) msg = &art_.wall;
        else if (std::strcmp(why_, "aground") == 0) msg = &art_.aground;
        banner(*msg, 160.f, 22.f, PAL_ALERT);
    } else if (headerLive_) banner(art_.take, 160.f, 28.f, PAL_ALERT);

    for (int i = 0; i < 4; i++) {
        float px = kPier0 + 6.f + i * 8.f;
        place(art_.pier, px, (kSouth0 + kSouth1) * 0.5f, kSouth1 - kSouth0, PAL_PIER);
        place(art_.pier, px, (kNorth0 + kNorth1) * 0.5f, kNorth1 - kNorth0, PAL_PIER);
    }
    place(art_.post, kHead - 0.4f, 0.f, 7.2f, PAL_PIER);
    place(art_.buoy, kMouth - 2.2f, kSouth1 + 0.2f, 2.6f, PAL_MARK);
    place(art_.buoy, kMouth - 2.2f, kNorth0 - 0.2f, 2.6f, PAL_MARK);
    place(art_.cleat, 33.f, kSouth1 - 0.15f, 0.9f, PAL_PIER);
    place(art_.cleat, 33.f, kNorth0 + 0.15f, 0.9f, PAL_PIER);

    float crew = std::min(kCrewLine, race_ * 0.82f + penalty_);
    float rx = -26.f + (crew / kCrewLine) * (kParkX + 26.f);
    float ry = 1.35f;
    place(art_.foam, rx - 1.6f, ry, 1.2f, PAL_HULL);
    {
        float rsx = 160.f + (rx - camX_) * zoom_;
        float rsy = 112.f - (ry - camY_) * zoom_;
        spr(art_.hull[0], rsx, rsy, 6.4f * zoom_, PAL_RIVAL, false);
    }

    float bob = 0.18f * std::sin(t_ * 2.1f);
    float bsx = 160.f + (x_ - camX_) * zoom_;
    float bsy = 112.f - (y_ - camY_) * zoom_ + bob;
    float hullH = 7.2f * zoom_;
    if (mode_ == Mode::Title) hullH = std::max(hullH, 28.f);
    const gs::Mipped& hull = art_.hull[hullFrame(mode_ == Mode::Title ? 0.4f : heading_)];
    spr(hull, bsx + 2.f, bsy + 3.f, hullH, PAL_HULL, true);
    spr(hull, bsx, bsy, hullH, PAL_HULL, false);
    place(art_.foam, x_ - std::cos(heading_) * 2.4f, y_ - std::sin(heading_) * 2.4f, 1.3f, PAL_HULL);

    float wind = headerLive_ ? (phase_ == Phase::Header1 ? -0.9f : 0.7f) : (phase_ == Phase::Final ? 0.05f : 1.15f);
    spr(art_.vane[hullFrame(wind)], 28.f, 26.f, 22.f, PAL_WIND, false);

    if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        char line[40];
        float left = std::max(0.f, kTide - race_);
        std::snprintf(line, sizeof line, "TIDE %4.1f", left);
        hud(1, 1, line, left < 10.f ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "CREW %4.1f", crewShort());
        hud(1, 2, line, crewShort() < 6.f ? PAL_ALERT : PAL_HUD);
        hud(30, 1, "WIND", PAL_WIND);
        hudC(25, hint(), headerLive_ ? PAL_ALERT : PAL_HUD);
        hudC(26, "BERTH BEFORE THE OTHER CREW", PAL_BANNER);
    } else if (mode_ == Mode::Title) {
        hudC(22, "TAKE THE HEADER", PAL_ALERT);
        hudC(24, "BERTH IN THE SLIP", PAL_HUD);
        hudC(26, "START    A TACKS THE HEADER", PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        hudC(14, "PAUSED", PAL_BANNER);
    }
}

}  // namespace headerslip
