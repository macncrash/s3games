#include "game/keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace keel {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FINISH = 980.f;
constexpr float APEX[3] = {210.f, 490.f, 760.f};
constexpr float HALF = 9.2f;
constexpr float FOCAL = 260.f;
constexpr float HORIZON = 84.f;

float smooth01(float t) {
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    return t * t * (3.f - 2.f * t);
}

float courseX(float z) {
    float x = 0;
    x += -24.f * smooth01((z - 120.f) / 180.f);
    x += 36.f * smooth01((z - 390.f) / 210.f);
    x += -30.f * smooth01((z - 670.f) / 180.f);
    return x;
}

float courseSlope(float z) {
    return (courseX(z + 8.f) - courseX(z - 8.f)) / 16.f;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

std::string clockOf(float sec) {
    if (sec < 0) sec = 0;
    int s = int(sec);
    int f = int((sec - s) * 10.f);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d:%02d.%d", s / 60, s % 60, f);
    return buf;
}

}  // namespace

void Game::begin() {
    x_ = courseX(12.f);
    z_ = 12.f;
    vx_ = 0;
    sheet_ = bot_ ? 0.46f : 0.35f;
    heel_ = 0;
    steer_ = 0;
    turns_ = 0;
    got_[0] = got_[1] = got_[2] = false;
    raceT_ = 0;
    hold_ = 0;
    over_ = false;
    won_ = false;
    tipped_ = false;
    mode_ = Mode::Race;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 196.f, 0.04f);
}

void Game::tip() {
    if (mode_ != Mode::Race) return;
    mode_ = Mode::Tipped;
    tipped_ = true;
    hold_ = 0;
    heel_ = 1.15f;
    sys_->rumble(0.8f, 1.0f, 240);
    sys_->apu.noiseBurst(0.6f, 500.f, 0.35f);
    sys_->apu.tone(1, 0, 0);
}

void Game::finish(bool win) {
    if (mode_ != Mode::Race) return;
    won_ = win;
    mode_ = win ? Mode::Won : Mode::Lost;
    hold_ = 0;
    over_ = true;
    sys_->apu.tone(1, 0, 0);
    if (win) {
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->rumble(0.2f, 0.4f, 120);
    } else {
        sys_->apu.tone(0, 110.f, 0.1f);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    if (bot_) begin();
}

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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

bool Game::project(float wx, float wy, float wz, float& sx, float& sy, float& sh, int& fog) const {
    float heading = std::atan(courseSlope(z_));
    float S = std::sin(heading), C = std::cos(heading);
    const float back = 13.5f;
    const float eye = 5.0f;
    float camX = x_ - S * back;
    float camZ = z_ - C * back;
    float dx = wx - camX;
    float dz = wz - camZ;
    float rz = dx * S + dz * C;
    float rx = dx * C - dz * S;
    if (rz < 1.2f || rz > 220.f) return false;
    float scale = FOCAL / rz;
    sx = 160.f + rx * scale;
    sy = HORIZON + (eye - wy) * scale;
    sh = 2.4f * scale;
    fog = rz > 70.f ? std::clamp(int((rz - 70.f) / 12.f), 0, 12) : 0;
    return true;
}

void Game::water() {
    gs::VDP& v = sys_->vdp;
    float heading = std::atan(courseSlope(z_));
    float S = std::sin(heading), C = std::cos(heading);
    const float back = (mode_ == Mode::Title) ? 16.f : 13.5f;
    const float eye = 5.0f + bob_;
    float camX = x_ - S * back;
    float camZ = z_ - C * back;
    const uint16_t zenith = gs::rgb4(3, 6, 11);
    const uint16_t mid = gs::rgb4(8, 12, 15);
    const uint16_t haze = gs::rgb4(13, 14, 14);
    v.roadTime = int(t_ * 20.f);
    v.setFogColor(gs::rgb4(10, 12, 13));

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < HORIZON) {
            float u = float(y) / HORIZON;
            v.lineBackdrop[y] = u < 0.55f ? lerpC(zenith, mid, u / 0.55f) : lerpC(mid, haze, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = std::max(1.f, float(y) - HORIZON);
        float dist = eye * FOCAL / row;
        float x0 = camX + S * dist;
        float z0 = camZ + C * dist;
        float c0 = courseX(z0);
        float c1 = courseSlope(z0);
        float e0 = x0 - c0;
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_FIELD);
        r.left = r.right = gs::GROUND_LAND;
        r.style = 2;
        r.v = z0 * 18.f;
        r.band = (int(std::floor(z0 * 0.18f)) & 1) ? 1 : 0;
        if (std::fabs(denom) < 0.05f) {
            r.cx = std::fabs(e0) <= HALF ? 160.f : -4000.f;
            r.hw = std::fabs(e0) <= HALF ? 900.f : 2.f;
        } else {
            float uA = (HALF - e0) / denom;
            float uB = (-HALF - e0) / denom;
            float scl = FOCAL / std::max(dist, 0.4f);
            r.cx = 160.f + 0.5f * (uA + uB) * scl;
            r.hw = std::min(4000.f, 0.5f * std::fabs(uA - uB) * scl);
        }
        int fog = dist > 75.f ? std::clamp(int((dist - 75.f) / 28.f), 0, 11) : 0;
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(2, 5, 7);
    }
}

void Game::update() {
    gs::Pad& pad = sys_->pad;
    if (mode_ == Mode::Title) {
        z_ = 40.f + std::sin(t_ * 0.15f) * 6.f;
        x_ = courseX(z_);
        bob_ = std::sin(t_ * 1.7f) * 0.12f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        return;
    }
    if (mode_ == Mode::Tipped) {
        hold_ += DT;
        if (hold_ > 1.4f) {
            mode_ = Mode::Lost;
            over_ = true;
            won_ = false;
        }
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return;

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Title;
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        return;
    }

    float steer = 0;
    if (bot_) {
        float vzGuess = 14.f + sheet_ * 12.f;
        float err = courseX(z_ + 4.f) - x_;
        float vmax = 6.2f + sheet_ * 2.2f;
        float wantVx = courseSlope(z_) * vzGuess + err * 3.2f;
        steer = std::clamp(wantVx / std::max(vmax, 1.f), -1.f, 1.f);
        sheet_ = 0.42f;
    } else {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.15f) steer = std::clamp(pad.axisX, -1.f, 1.f);
        float pull = 0;
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) pull += 1.f;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B)) pull -= 1.f;
        pull += pad.accel - pad.brake;
        sheet_ = std::clamp(sheet_ + pull * DT * 0.85f, 0.12f, 1.f);
    }
    steer_ = steer;

    float vmax = 6.2f + sheet_ * 2.2f;
    vx_ += (steer * vmax - vx_) * DT * 5.0f;
    float vz = 14.f + sheet_ * 12.f;
    float prevZ = z_;
    x_ += vx_ * DT;
    z_ += vz * DT;
    raceT_ += DT;
    bob_ = std::sin(raceT_ * 2.4f) * 0.08f;

    float edge = x_ - courseX(z_);
    float carve = std::fabs(steer) * (0.22f + sheet_ * 0.8f);
    float skid = std::min(1.f, std::fabs(edge) / HALF) * 0.45f;
    float want = std::min(1.2f, carve + skid);
    heel_ += (want - heel_) * DT * 3.4f;

    int frame = 2 + int(std::lround(std::clamp(steer, -1.f, 1.f) * 2.f));
    heelFrame_ = std::clamp(frame, 0, 4);

    if (std::fabs(edge) > HALF + 2.4f || heel_ > 0.97f) {
        tip();
        return;
    }

    for (int i = 0; i < 3; i++) {
        if (got_[i]) continue;
        if (prevZ < APEX[i] && z_ >= APEX[i] && std::fabs(edge) <= HALF) {
            got_[i] = true;
            turns_++;
            sys_->apu.tone(0, 660.f + turns_ * 80.f, 0.08f);
            sys_->rumble(0.15f, 0.3f, 60);
        }
    }

    if (turns_ == 3 && z_ >= FINISH) {
        finish(raceT_ < crewLimit_);
        return;
    }
    if (raceT_ >= crewLimit_) finish(false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    water();

    if (mode_ == Mode::Race || mode_ == Mode::Won || mode_ == Mode::Lost) {
        float gz = (std::min(raceT_, crewLimit_) / crewLimit_) * FINISH;
        float sx, sy, sh;
        int fog;
        if (project(courseX(gz), 0.4f, gz, sx, sy, sh, fog) && gz > z_ + 6.f)
            spr(art_.ghost, sx, sy, sh * 7.2f, PAL_GHOST, false, fog + 3);
        for (int i = 2; i >= 0; i--) {
            float mz = APEX[i];
            if (!project(courseX(mz), 0.2f, mz, sx, sy, sh, fog)) continue;
            spr(art_.buoy, sx, sy - sh * 2.2f, sh * 5.5f, PAL_MARK, false, fog);
        }
    }

    float wakeY = 188.f + bob_ * 8.f;
    spr(art_.wake, 160.f, wakeY + 18.f, 22.f, PAL_BOAT, false, 0);
    int fr = std::clamp(heelFrame_, 0, 4);
    float lean = (fr - 2) * 10.f;
    spr(art_.boat[fr], 160.f + lean, 168.f + bob_ * 10.f, mode_ == Mode::Title ? 78.f : 92.f, PAL_BOAT, false);

    if (mode_ == Mode::Title) {
        text("S3 KEEL TURN", 160, 28, 1.05f, PAL_HUD);
        text("TAKE THE KEEL", 160, 52, 0.72f, PAL_MARK);
        hudC(18, "THREE TURNS  DO NOT TIP", PAL_HUD);
        hudC(20, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(23, "LEFT RIGHT STEER", PAL_HUD);
        hudC(24, "A SHEET IN   B EASE", PAL_HUD);
        if (int(t_ * 2.f) % 2 == 0) hudC(26, "START", 6);
    } else {
        char line[40];
        std::snprintf(line, sizeof line, "TURN %d/3", std::min(turns_, 3));
        hud(1, 1, line, turns_ >= 3 ? 5 : PAL_HUD);
        std::snprintf(line, sizeof line, "HEEL %d", int(std::lround(std::min(heel_, 1.f) * 100.f)));
        hud(28, 1, line, heel_ > 0.72f ? 4 : PAL_HUD);
        int bars = int(std::lround(std::min(heel_, 1.f) * 10.f));
        std::string bar(10, '.');
        for (int i = 0; i < bars && i < 10; i++) bar[i] = '#';
        hud(28, 2, bar, heel_ > 0.72f ? 4 : 6);
        hud(1, 25, "TIME " + clockOf(raceT_), PAL_HUD);
        hud(22, 25, "CREW " + clockOf(crewLimit_), 3);
        float left = crewLimit_ - raceT_;
        hud(22, 26, "LEFT " + clockOf(left), left < 8.f ? 4 : 5);
        if (mode_ == Mode::Tipped) hudC(12, "TIPPED", 4);
        if (mode_ == Mode::Won) {
            hudC(11, "THREE TURNS CLEAN", 5);
            hudC(13, "THE KEEL IS YOURS", PAL_HUD);
        }
        if (mode_ == Mode::Lost && !won_) {
            hudC(11, tipped_ ? "TIPPED" : "THE OTHER CREW", 4);
            hudC(13, "START TO TAKE IT AGAIN", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    update();
    if ((mode_ == Mode::Lost || mode_ == Mode::Won) && !bot_) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            over_ = false;
            begin();
        }
    }
    if (mode_ == Mode::Race) sys.apu.noise(0.025f, 700.f);
    draw();
}

}  // namespace keel
