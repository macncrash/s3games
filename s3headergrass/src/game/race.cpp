#include "game/race.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace headergrass {
namespace {

constexpr float PI = 3.14159265f;
constexpr float TAU = 6.2831853f;
constexpr float GX0 = -100.0f;
constexpr float GX1 = 100.0f;
constexpr float GY0 = 360.0f;
constexpr float GY1 = 520.0f;

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
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

float Game::windFrom() const {
    float u = gun_ ? raceT_ : t_;
    return PI * 0.5f + 0.52f * std::sin(u * 0.70f);
}

bool Game::headerNow() const {
    float u = gun_ ? raceT_ : t_;
    float dShift = 0.52f * 0.70f * std::cos(u * 0.70f);
    float rel = wrap(heading_ - windFrom());
    if (std::fabs(rel) > 1.35f || std::fabs(rel) < 0.42f) return false;
    return dShift * rel < -0.04f;
}

bool Game::onGrass() const { return x_ > GX0 && x_ < GX1 && y_ > GY0 && y_ < GY1; }

void Game::beginRace() {
    wps_.clear();
    wps_.push_back({130, 180});
    wps_.push_back({-70, 300});
    wps_.push_back({0, 430});
    x_ = 0;
    y_ = 36;
    heading_ = PI * 0.5f + 0.85f;
    speed_ = 0;
    raceT_ = 0;
    gun_ = false;
    headerTaken_ = false;
    stopped_ = false;
    still_ = 0;
    wp_ = 0;
    botPhase_ = 0;
    tack_ = 0;
    boost_ = 0;
    won_ = false;
    over_ = false;
    msg_ = "TAKE THE HEADER";
    msgT_ = 2.2f;
    mode_ = Mode::Race;
    t_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 10, 8));
    for (int i = 0; i < gs::SCREEN_H; i++) {
        sys.vdp.road[i].on = false;
        sys.vdp.lineFog[i] = 0;
    }
    if (bot_) beginRace();
    else mode_ = Mode::Title;
}

void Game::steerBoat(float dt, float want) {
    if (tack_ > 0) {
        tack_ -= dt;
        float u = 1.0f - std::max(0.0f, tack_) / 0.36f;
        heading_ = tackFrom_ + wrap(tackTo_ - tackFrom_) * std::min(1.0f, u);
        return;
    }
    float dh = wrap(want - heading_);
    float step = 2.05f * dt;
    if (std::fabs(dh) <= step) heading_ = want;
    else heading_ += std::copysign(step, dh);
}

void Game::update(float dt) {
    gs::Pad& pad = sys_->pad;
    float want = heading_;
    bool doTack = false;
    bool grass = onGrass();

    if (bot_) {
        if (botPhase_ == 0) {
            want = windFrom() + 0.92f;
            if (headerNow() && gun_ && tack_ <= 0) doTack = true;
            if (headerTaken_) botPhase_ = 1;
        } else if (!grass) {
            if (wp_ < int(wps_.size())) {
                float dx = wps_[wp_].x - x_;
                float dy = wps_[wp_].y - y_;
                if (len(dx, dy) < 36.0f) wp_++;
            }
            float tx = wp_ < int(wps_.size()) ? wps_[wp_].x : 0;
            float ty = wp_ < int(wps_.size()) ? wps_[wp_].y : 440;
            want = std::atan2(ty - y_, tx - x_);
            float off = wrap(want - windFrom());
            if (std::fabs(off) < 0.78f) want = windFrom() + (off >= 0 ? 0.78f : -0.78f);
        } else {
            want = windFrom();
        }
    } else {
        if (pad.down(gs::BTN_LEFT)) want = heading_ + 1.45f * dt;
        else if (pad.down(gs::BTN_RIGHT)) want = heading_ - 1.45f * dt;
        if (std::fabs(pad.axisX) > 0.25f) want = heading_ - pad.axisX * 1.7f * dt;
        if (pad.pressed(gs::BTN_A)) doTack = true;
        if (grass && pad.down(gs::BTN_B)) want = windFrom();
    }

    if (doTack && tack_ <= 0 && !grass) {
        float rel = wrap(heading_ - windFrom());
        float sign = rel >= 0 ? 1.0f : -1.0f;
        float mag = std::max(0.88f, std::fabs(rel));
        tackFrom_ = heading_;
        tackTo_ = windFrom() - sign * mag;
        tack_ = 0.36f;
        sys_->apu.tone(0, 620.0f, 0.08f);
        if (headerNow()) {
            headerTaken_ = true;
            boost_ = 2.6f;
            msg_ = "HEADER TAKEN";
            msgT_ = 1.5f;
            sys_->apu.tone(1, 880.0f, 0.1f);
        }
    }

    if (!gun_) {
        steerBoat(dt, want);
        if (t_ > 1.2f) {
            gun_ = true;
            raceT_ = 0;
            sys_->apu.noiseBurst(0.35f, 420.0f, 0.12f);
            msg_ = "GUN";
            msgT_ = 0.7f;
        }
        return;
    }

    steerBoat(dt, want);
    float off = std::fabs(wrap(heading_ - windFrom()));
    float targetSpd;
    if (grass) {
        targetSpd = headerTaken_ ? 0.0f : 12.0f;
    } else if (off < 0.55f) {
        targetSpd = 8.0f;
    } else if (off < 1.05f) {
        targetSpd = 58.0f;
    } else if (off < 2.1f) {
        targetSpd = 78.0f;
    } else {
        targetSpd = 48.0f;
    }
    if (boost_ > 0 && !grass) {
        boost_ -= dt;
        targetSpd *= 1.16f;
    }
    float k = grass ? 3.4f : 2.2f;
    speed_ += (targetSpd - speed_) * std::min(1.0f, dt * k);
    if (speed_ < 0.15f) speed_ = 0;
    float blow = windFrom() + PI;
    float lee = grass ? 0.0f : (off < 1.1f ? 12.0f : 4.0f);
    x_ += (std::cos(heading_) * speed_ + std::cos(blow) * lee) * dt;
    y_ += (std::sin(heading_) * speed_ + std::sin(blow) * lee) * dt;
    raceT_ += dt;
    if (msgT_ > 0) msgT_ -= dt;

    if (x_ < -240) x_ = -240;
    if (x_ > 280) x_ = 280;
    if (y_ < -40) y_ = -40;
    if (y_ > 600) y_ = 600;

    grass = onGrass();
    if (grass && headerTaken_ && speed_ <= 0.2f) still_ += dt;
    else still_ = 0;

    if (grass && !headerTaken_ && msgT_ <= 0) {
        msg_ = "HEADER FIRST";
        msgT_ = 0.4f;
    } else if (grass && headerTaken_ && speed_ > 8.0f && msgT_ <= 0) {
        msg_ = "STOP ON THE GRASS";
        msgT_ = 0.35f;
    }

    if (still_ > 0.35f && raceT_ <= crew_) {
        stopped_ = true;
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        sys_->apu.tone(0, 523.0f, 0.12f);
        sys_->apu.tone(1, 784.0f, 0.1f);
    } else if (raceT_ > crew_ && !(still_ > 0.35f)) {
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
        lookX = 0;
        lookY = 280;
    }
    camX_ += (lookX - camX_) * 0.16f;
    camY_ += (lookY - camY_) * 0.16f;
    if (mode_ == Mode::Title) {
        camX_ = 0;
        camY_ = 300;
    }

    for (int row = 0; row < gs::SCREEN_H; row++) {
        float wy = camY_ + (gs::SCREEN_H * 0.5f - row);
        bool band = wy > GY0 - 18 && wy < GY1 + 18 && std::fabs(camX_) < 180;
        int ripple = int(std::floor(wy / 16.0f)) & 1;
        if (band && wy > GY0 && wy < GY1) v.lineBackdrop[row] = ripple ? gs::rgb4(2, 8, 3) : gs::rgb4(1, 7, 2);
        else if (band) v.lineBackdrop[row] = gs::rgb4(7, 8, 3);
        else v.lineBackdrop[row] = ripple ? gs::rgb4(2, 6, 11) : gs::rgb4(1, 5, 9);
        v.road[row].on = false;
    }

    auto field = [&](float by) {
        spr(art_.meadow, 0, (GY0 + GY1) * 0.5f, 168, PAL_MEADOW);
        spr(art_.flag, -70, GY1 - 16, 46, PAL_FLAG);
        spr(art_.flag, 70, GY1 - 16, 46, PAL_FLAG, true);
        for (int i = 0; i < 8; i++) {
            float tx = -78.0f + float(i % 4) * 52.0f;
            float ty = GY0 + 28.0f + float(i / 4) * 70.0f;
            spr(art_.tuft, tx, ty, 22, PAL_MEADOW, (i & 1) != 0);
        }
        spr(art_.boat[4], 0, by, 40, PAL_BOAT);
    };

    if (mode_ == Mode::Title) {
        field(40);
    } else {
        int frame = int(std::floor((heading_ < 0 ? heading_ + TAU : heading_) / TAU * 16.0f + 0.5f)) & 15;
        spr(art_.boat[frame], x_, y_, 42, PAL_BOAT);
        if (!onGrass()) {
            for (int i = 0; i < 3; i++) {
                float wx = x_ - std::cos(heading_) * (16.0f + i * 12);
                float wy = y_ - std::sin(heading_) * (16.0f + i * 12);
                spr(art_.wake, wx, wy, 16.0f - i * 3, PAL_WAKE);
            }
        }
        spr(art_.meadow, 0, (GY0 + GY1) * 0.5f, 176, PAL_MEADOW);
        spr(art_.flag, -78, GY1 - 12, 48, PAL_FLAG);
        spr(art_.flag, 78, GY1 - 12, 48, PAL_FLAG, true);
        for (int i = 0; i < 10; i++) {
            float tx = -80.0f + float(i % 5) * 40.0f;
            float ty = GY0 + 24.0f + float(i / 5) * 64.0f;
            spr(art_.tuft, tx, ty, 26, PAL_MEADOW, (i & 1) != 0);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 HEADER GRASS", PAL_AMBER);
        hudC(6, "TAKE THE HEADER", PAL_HUD);
        hudC(8, "LAND ON THE GRASS", PAL_GREEN);
        hudC(10, "COME TO A FULL STOP", PAL_HUD);
        hudC(12, "THE CLOCK IS THE OTHER CREW", PAL_GREEN);
        hudC(17, "ARROWS STEER    A TACKS", PAL_HUD);
        hudC(19, "B HOLDS THE BOW INTO THE WIND", PAL_AMBER);
        hudC(23, "START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
        return;
    }

    char buf[64];
    std::snprintf(buf, sizeof(buf), "TIME %5.1f", raceT_);
    hud(1, 1, gun_ ? buf : "STANDBY", PAL_HUD);
    std::snprintf(buf, sizeof(buf), "CREW %4.1f", crew_);
    hud(39 - int(std::strlen(buf)), 1, buf, raceT_ > crew_ * 0.82f ? PAL_RED : PAL_GREEN);
    hud(1, 26, headerTaken_ ? "HEADER" : "NO HEADER", headerTaken_ ? PAL_GREEN : PAL_AMBER);
    const char* leg = "BEAT NORTH";
    if (!gun_) leg = "GUN IN";
    else if (!headerTaken_) leg = "TAKE THE HEADER";
    else if (!onGrass()) leg = "LAND ON GRASS";
    else if (speed_ > 0.2f) leg = "FULL STOP";
    else leg = "HOLD";
    hud(39 - int(std::strlen(leg)), 26, leg, PAL_AMBER);

    float rel = wrap(heading_ - windFrom());
    float a = std::fabs(rel);
    const char* point = "REACH";
    if (a < 0.55f) point = "IN IRONS";
    else if (a < 1.05f) point = "HEADER";
    else if (a < 2.1f) point = "REACH";
    else point = "RUN";
    hudC(3, point, a < 0.55f ? PAL_RED : PAL_HUD);
    if (headerNow() && !headerTaken_ && mode_ == Mode::Race) hudC(5, "HEADER  TACK", PAL_AMBER);
    else if (msgT_ > 0) hudC(5, msg_, onGrass() ? PAL_GREEN : PAL_AMBER);

    if (mode_ == Mode::Win) {
        hudC(11, "FULL STOP", PAL_GREEN);
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

}  // namespace headergrass
