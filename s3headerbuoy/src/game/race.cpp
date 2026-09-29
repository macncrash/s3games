#include "game/race.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace headerbuoy {
namespace {

constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float RING = 108.0f;
constexpr float NEED = 2.05f;
constexpr float LAY = 0.80f;

float wrap(float a) {
    while (a > PI) a -= TAU;
    while (a < -PI) a += TAU;
    return a;
}

float len(float x, float y) { return std::sqrt(x * x + y * y); }

}  // namespace

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float sx = (wx - camX_) + gs::SCREEN_W * 0.5f;
    float sy = gs::SCREEN_H * 0.5f - (wy - camY_);
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(sx - s.w * 0.5f));
    s.y = int16_t(std::lround(sy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

float Game::windFrom() const {
    // From the north, sliding east and west. A header is this shift on your bow.
    float u = gun_ ? raceT_ : t_;
    return PI * 0.5f + 0.48f * std::sin(u * 0.62f);
}

bool Game::headerNow() const {
    float u = gun_ ? raceT_ : t_;
    float dShift = 0.48f * 0.62f * std::cos(u * 0.62f);
    float rel = wrap(heading_ - windFrom());
    if (std::fabs(rel) > 1.35f || std::fabs(rel) < 0.45f) return false;
    // Shift moving toward the bow heads the boat.
    return dShift * rel < -0.05f;
}

void Game::beginRace() {
    buoys_.clear();
    buoys_.push_back({0, 520, 0, 0, false, false, "WEATHER"});
    buoys_.push_back({250, 250, 0, 0, false, false, "WING"});
    buoys_.push_back({-20, 90, 0, 0, false, false, "LEEWARD"});
    auto arc = [&](float cx, float cy, float r, float a0, float a1, int n) {
        for (int i = 0; i <= n; i++) {
            float a = a0 + (a1 - a0) * (float(i) / float(n));
            wps_.push_back({cx + std::cos(a) * r, cy + std::sin(a) * r});
        }
    };
    wps_.clear();
    // Port roundings: counterclockwise arcs, then the same dock.
    arc(0, 520, 70, -1.15f, 2.55f, 16);
    arc(250, 250, 70, 2.15f, 2.15f + 2.55f, 14);
    arc(-20, 90, 68, 0.15f, 0.15f + 4.5f, 16);
    wps_.push_back({0, 10});
    wps_.push_back({0, -6});
    x_ = 0;
    y_ = 36;
    heading_ = PI * 0.5f;
    speed_ = 0;
    raceT_ = 0;
    gun_ = false;
    leftDock_ = false;
    cleared_ = 0;
    wp_ = 0;
    tack_ = 0;
    boost_ = 0;
    headerFlash_ = 0;
    won_ = false;
    over_ = false;
    msg_ = "TAKE THE HEADER";
    msgT_ = 2.5f;
    mode_ = Mode::Race;
    t_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(4, 8, 12));
    for (int i = 0; i < gs::SCREEN_H; i++) {
        sys.vdp.road[i].on = false;
        sys.vdp.lineFog[i] = 0;
    }
    if (bot_) beginRace();
    else mode_ = Mode::Title;
}

void Game::layHeading(float tx, float ty, float& want) const {
    want = std::atan2(ty - y_, tx - x_);
    float off = wrap(want - windFrom());
    if (std::fabs(off) < LAY) want = windFrom() + (off >= 0 ? LAY : -LAY);
}

void Game::steerBoat(float dt, float want) {
    if (tack_ > 0) {
        tack_ -= dt;
        float u = 1.0f - std::max(0.0f, tack_) / 0.38f;
        heading_ = tackFrom_ + wrap(tackTo_ - tackFrom_) * std::min(1.0f, u);
        return;
    }
    float dh = wrap(want - heading_);
    float step = 2.15f * dt;
    if (std::fabs(dh) <= step) heading_ = want;
    else heading_ += std::copysign(step, dh);
}

void Game::update(float dt) {
    gs::Pad& pad = sys_->pad;
    float want = heading_;
    bool doTack = false;

    if (bot_) {
        if (wp_ < int(wps_.size())) {
            float dx = wps_[wp_].x - x_;
            float dy = wps_[wp_].y - y_;
            if (len(dx, dy) < 34.0f) wp_++;
        }
        float tx = wp_ < int(wps_.size()) ? wps_[wp_].x : 0;
        float ty = wp_ < int(wps_.size()) ? wps_[wp_].y : -6;
        layHeading(tx, ty, want);
        if (headerNow() && tack_ <= 0 && gun_) {
            float rel = wrap(heading_ - windFrom());
            if (std::fabs(rel) < 1.2f) doTack = true;
        }
    } else {
        if (pad.down(gs::BTN_LEFT)) want = heading_ + 1.4f * dt;
        else if (pad.down(gs::BTN_RIGHT)) want = heading_ - 1.4f * dt;
        else want = heading_;
        // Analog stick, if a pad is on the board.
        if (std::fabs(pad.axisX) > 0.25f) want = heading_ - pad.axisX * 1.8f * dt;
        if (pad.pressed(gs::BTN_A)) doTack = true;
    }

    if (doTack && tack_ <= 0) {
        float rel = wrap(heading_ - windFrom());
        float sign = rel >= 0 ? 1.0f : -1.0f;
        float mag = std::max(0.9f, std::fabs(rel));
        tackFrom_ = heading_;
        tackTo_ = windFrom() - sign * mag;
        tack_ = 0.38f;
        sys_->apu.tone(0, 660.0f, 0.08f);
        if (headerNow()) {
            boost_ = 2.4f;
            msg_ = "HEADER TAKEN";
            msgT_ = 1.4f;
            sys_->apu.tone(1, 880.0f, 0.1f);
        }
    }

    if (!gun_) {
        steerBoat(dt, want);
        if (t_ > 1.6f) {
            gun_ = true;
            raceT_ = 0;
            sys_->apu.noiseBurst(0.4f, 500.0f, 0.15f);
            msg_ = "GUN";
            msgT_ = 0.8f;
        }
        return;
    }

    steerBoat(dt, want);
    float off = std::fabs(wrap(heading_ - windFrom()));
    float targetSpd;
    if (off < 0.58f) targetSpd = 10.0f;
    else if (off < 1.05f) targetSpd = 62.0f;
    else if (off < 2.05f) targetSpd = 84.0f;
    else targetSpd = 54.0f;
    if (boost_ > 0) {
        boost_ -= dt;
        targetSpd *= 1.18f;
    }
    if (headerFlash_ > 0) headerFlash_ -= dt;
    if (headerNow()) headerFlash_ = 0.15f;
    speed_ += (targetSpd - speed_) * std::min(1.0f, dt * 2.4f);
    float blow = windFrom() + PI;
    float lee = off < 1.15f ? 14.0f : 5.0f;
    x_ += (std::cos(heading_) * speed_ + std::cos(blow) * lee) * dt;
    y_ += (std::sin(heading_) * speed_ + std::sin(blow) * lee) * dt;
    raceT_ += dt;
    if (msgT_ > 0) msgT_ -= dt;
    if (y_ > 120) leftDock_ = true;

    // Soft harbour edges. The race is the water inside the marks.
    if (x_ < -180) x_ = -180;
    if (x_ > 430) x_ = 430;
    if (y_ < -80) y_ = -80;
    if (y_ > 680) y_ = 680;

    cleared_ = 0;
    for (Buoy& b : buoys_) {
        float dx = x_ - b.x;
        float dy = y_ - b.y;
        float d = len(dx, dy);
        if (d < 16.0f && !b.done) {
            float push = 16.0f - d;
            x_ += dx / (d + 0.01f) * push;
            y_ += dy / (d + 0.01f) * push;
            speed_ *= 0.4f;
            sys_->apu.tone(1, 180.0f, 0.12f);
        }
        float ang = std::atan2(dy, dx);
        if (d < RING && d > 8.0f) {
            if (b.inRing && !b.done) {
                b.sweep += wrap(ang - b.lastAng);
                if (b.sweep >= NEED) {
                    b.done = true;
                    msg_ = std::string(b.name) + " ROUNDED";
                    msgT_ = 1.3f;
                    sys_->apu.tone(0, 520.0f, 0.1f);
                }
            }
            b.inRing = true;
            b.lastAng = ang;
        } else if (b.inRing) {
            b.inRing = false;
            if (!b.done) b.sweep *= 0.2f;
        }
        if (b.done) cleared_++;
    }

    bool home = leftDock_ && cleared_ >= 3 && len(x_, y_) < 46.0f && y_ < 28.0f;
    if (home && raceT_ <= crew_) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        sys_->apu.tone(0, 523.0f, 0.12f);
        sys_->apu.tone(1, 784.0f, 0.1f);
    } else if (raceT_ > crew_ && !home) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float lookX = x_, lookY = y_;
    if (mode_ == Mode::Title) {
        lookX = 80;
        lookY = 260;
    }
    camX_ += (lookX - camX_) * 0.18f;
    camY_ += (lookY - camY_) * 0.18f;
    if (mode_ == Mode::Title) {
        camX_ = 90;
        camY_ = 280;
    }
    for (int row = 0; row < gs::SCREEN_H; row++) {
        float wy = camY_ + (gs::SCREEN_H * 0.5f - row);
        int band = int(std::floor(wy / 18.0f)) & 1;
        int shimmer = int(t_ * 8 + row) & 7;
        v.lineBackdrop[row] = band ? gs::rgb4(2, 6, 11) : gs::rgb4(1, 5, 10);
        if (shimmer == 0) v.lineBackdrop[row] = gs::rgb4(3, 8, 12);
        v.road[row].on = false;
    }

    if (mode_ == Mode::Title) {
        // A still of the harbour so the picture is on screen before the start.
        spr(art_.dock, 0, -8, 54, PAL_DOCK);
        spr(art_.buoy, 0, 520, 36, PAL_BUOY);
        spr(art_.buoy, 250, 250, 36, PAL_BUOY);
        spr(art_.buoy, -20, 90, 36, PAL_BUOY);
        spr(art_.boat[4], 0, 36, 40, PAL_BOAT);
    } else {
        spr(art_.dock, 0, -8, 58, PAL_DOCK);
        for (int i = 0; i < 3; i++) {
            float wx = x_ - std::cos(heading_) * (18.0f + i * 14);
            float wy = y_ - std::sin(heading_) * (18.0f + i * 14);
            spr(art_.wake, wx, wy, 18.0f - i * 3, PAL_WAKE);
        }
        for (const Buoy& b : buoys_) spr(art_.buoy, b.x, b.y, b.done ? 30 : 40, b.done ? PAL_GREEN : PAL_BUOY);
        int frame = int(std::floor((heading_ < 0 ? heading_ + TAU : heading_) / TAU * 16.0f + 0.5f)) & 15;
        spr(art_.boat[frame], x_, y_, 44, PAL_BOAT);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 HEADER BUOY", PAL_AMBER);
        hudC(6, "ROUND THE BUOYS", PAL_HUD);
        hudC(8, "RETURN TO THE DOCK", PAL_HUD);
        hudC(10, "THE CLOCK IS THE OTHER CREW", PAL_GREEN);
        hudC(16, "ARROWS STEER", PAL_HUD);
        hudC(18, "A TACKS  TAKE THE HEADER", PAL_AMBER);
        hudC(22, "START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
        return;
    }

    char buf[64];
    std::snprintf(buf, sizeof(buf), "TIME %5.1f", raceT_);
    hud(1, 1, gun_ ? buf : "STANDBY", PAL_HUD);
    std::snprintf(buf, sizeof(buf), "CREW %4.1f", crew_);
    hud(39 - int(std::strlen(buf)), 1, buf, raceT_ > crew_ * 0.82f ? PAL_RED : PAL_GREEN);
    std::snprintf(buf, sizeof(buf), "BUOYS %d/3", cleared_);
    hud(1, 26, buf, cleared_ == 3 ? PAL_GREEN : PAL_HUD);
    const char* leg = "LEAVE DOCK";
    if (cleared_ == 0) leg = "BEAT TO WEATHER";
    else if (cleared_ == 1) leg = "REACH THE WING";
    else if (cleared_ == 2) leg = "ROUND LEEWARD";
    else leg = "DOCK";
    if (!gun_) leg = "GUN IN";
    hud(39 - int(std::strlen(leg)), 26, leg, PAL_AMBER);

    float rel = wrap(heading_ - windFrom());
    const char* point = "IRONS";
    float a = std::fabs(rel);
    if (a < 0.58f) point = "IN IRONS";
    else if (a < 1.05f) point = "HEADER";
    else if (a < 2.05f) point = "REACH";
    else point = "RUN";
    hudC(3, point, a < 0.58f ? PAL_RED : PAL_HUD);
    if (headerNow() && mode_ == Mode::Race) hudC(5, "HEADER  TACK", PAL_AMBER);
    else if (msgT_ > 0) hudC(5, msg_, PAL_GREEN);

    if (mode_ == Mode::Win) {
        hudC(11, "DOCK", PAL_GREEN);
        hudC(13, "YOU BEAT THE CREW", PAL_AMBER);
        std::snprintf(buf, sizeof(buf), "%.1fS  CREW %.1fS", raceT_, crew_);
        hudC(15, buf, PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        hudC(11, "THE CREW TOOK IT", PAL_RED);
        hudC(13, "THE CLOCK WINS", PAL_AMBER);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    float dt = 1.0f / 60.0f;
    t_ += dt;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginRace();
    } else if (mode_ == Mode::Race) {
        update(dt);
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) {
        beginRace();
        over_ = false;
        won_ = false;
    }
    draw();
}

}  // namespace headerbuoy
