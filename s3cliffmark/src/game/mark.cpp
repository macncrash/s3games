#include "game/mark.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cliffmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kNose = 5.4f;
constexpr float kMarkZ = 104.f;
constexpr float kHalfLen = 1.05f;
constexpr float kMarkU = 0.18f;
constexpr float kEnd = 116.f;
constexpr float kClock = 46.f;
constexpr float kStop = 0.38f;
constexpr float kHoldNeed = 0.55f;
constexpr float kOutNeed = 0.9f;
constexpr float kHorizon = 78.f;
constexpr float kScale = 92.f;
constexpr float kHw = 1.02f;
constexpr float kSteer = 1.15f;
constexpr float kMaxSpd = 22.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bend(float z) { return std::sin(z * 0.021f) * 0.34f; }

float roadRate(float z) { return std::cos(z * 0.021f) * 0.34f * 0.021f; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (onMark()) return 2;
    return 1;
}

float Game::noseZ() const { return z_ + kNose; }

bool Game::onMark() const {
    return std::fabs(noseZ() - kMarkZ) <= kHalfLen && std::fabs(u_) <= kMarkU;
}

const char* Game::stopWhy() const {
    float err = noseZ() - kMarkZ;
    if (std::fabs(err) <= kHalfLen + 0.55f && std::fabs(u_) > kMarkU) return "stopped wide of the mark";
    if (std::fabs(err) <= kHalfLen + 1.6f) return "close to the mark is still off it";
    if (err < 0.f) return "stopped short of the mark";
    return "stopped long of the mark";
}

void Game::begin() {
    z_ = 12.f;
    u_ = 0.04f;
    speed_ = 5.f;
    hold_ = 0.f;
    outT_ = 0.f;
    raceT_ = 0.f;
    clock_ = kClock;
    noseErr_ = kMarkZ - noseZ();
    phase_ = 0;
    won_ = false;
    over_ = false;
    announced_ = false;
    chime_ = 0;
    shake_ = 0.f;
    why_.clear();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    blip(520.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    tone_ = 0.1f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "set down on the mark";
    chime_ = 5;
    chimeT_ = 0.02f;
    sys_->rumble(0.28f, 0.1f, 140);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    shake_ = 1.f;
    sys_->rumble(0.5f, 0.22f, 160);
    sys_->setLight(170, 36, 24);
    sys_->apu.noiseBurst(0.4f, 120.f, 0.32f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.58f);
    sys.apu.setEcho(0.08f, 0.12f, 0.05f);
    t_ = 0.f;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& gas, float& brake, float& steer, float& rev) {
    const gs::Pad& p = sys_->pad;
    steer = gas = brake = rev = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = clampf(p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.axisY > 0.25f) gas = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.brake > 0.12f || p.axisY < -0.25f) brake = 1.f;
    if (p.accel > 0.12f) gas = std::max(gas, p.accel);
    if (p.down(gs::BTN_X) || p.down(gs::BTN_Z)) rev = 1.f;
}

void Game::pilot(float& gas, float& brake, float& steer, float& rev) {
    const float park = kMarkZ - kNose;
    const float dist = park - z_;
    const float drift = roadRate(z_ + 8.f) * std::max(speed_, 0.f);
    steer = clampf((drift + u_ * 3.4f) / kSteer, -1.f, 1.f);
    gas = brake = rev = 0.f;
    if (std::fabs(dist) < 2.4f && std::fabs(u_) < 0.28f && speed_ < 1.4f) {
        steer = clampf(u_ * 6.f, -1.f, 1.f);
        if (dist < -0.18f) {
            rev = speed_ < 1.1f ? 1.f : 0.f;
            brake = speed_ > 0.2f ? 1.f : 0.f;
        } else if (dist > 0.22f) {
            gas = speed_ < 1.1f ? 0.45f : 0.f;
            brake = speed_ > 1.3f ? 1.f : 0.f;
        } else {
            brake = std::fabs(speed_) > 0.08f ? 1.f : 0.f;
        }
        return;
    }
    float want = dist > 36.f ? 17.f : std::max(0.f, dist * 0.40f);
    if (dist < 1.1f) want = 0.f;
    if (speed_ > want + 0.25f) brake = 1.f;
    else if (speed_ < want - 0.4f) gas = dist > 28.f ? 1.f : 0.5f;
    else gas = 0.12f;
}

void Game::physics(float dt, float gas, float brake, float steer, float rev) {
    const bool near = std::fabs(noseZ() - kMarkZ) < 8.f;
    if (gas > 0.f) speed_ += gas * (near ? 8.f : 13.f) * dt;
    if (brake > 0.f) speed_ -= brake * (near ? 28.f : 18.f) * dt;
    if (rev > 0.f) speed_ -= rev * 9.f * dt;
    speed_ -= speed_ * (near ? 1.35f : 0.28f) * dt;
    speed_ = clampf(speed_, -3.2f, kMaxSpd);

    float push = roadRate(z_) * speed_;
    u_ += (push - steer * kSteer) * dt;
    z_ += speed_ * dt;
    if (z_ < 1.f) {
        z_ = 1.f;
        if (speed_ < 0.f) speed_ = 0.f;
    }
    noseErr_ = kMarkZ - noseZ();

    const bool planted = onMark();
    const bool closing = std::fabs(noseErr_) > 0.02f && ((noseErr_ > 0.f && speed_ > 0.15f) || (noseErr_ < 0.f && speed_ < -0.12f));
    if (planted && std::fabs(speed_) < kStop) {
        outT_ = 0.f;
        hold_ += dt;
        speed_ *= 0.82f;
        u_ *= 0.9f;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else if (std::fabs(speed_) < kStop && !closing && z_ > 28.f) {
        hold_ = 0.f;
        outT_ += dt;
        if (outT_ >= kOutNeed) {
            fail(stopWhy());
            return;
        }
    } else {
        hold_ = 0.f;
        if (std::fabs(speed_) >= kStop || closing) outT_ = std::max(0.f, outT_ - dt);
    }

    if (noseZ() > kEnd) {
        fail("missed the end");
        return;
    }
    if (std::fabs(u_) > 1.02f) {
        fail("left the shelf");
        return;
    }
    if (clock_ <= 0.f) {
        fail("the leg ran out");
        return;
    }
    if (planted && !announced_) {
        announced_ = true;
        blip(680.f);
    }
    phase_ = hold_ > 0.05f ? 3 : (planted ? 2 : (noseErr_ < 22.f ? 1 : 0));
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 2.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

bool Game::project(float lat, float z, float& sx, float& sy, float& sh) const {
    float ahead = z - z_;
    if (mode_ == Mode::Title) ahead = z - (kMarkZ - 22.f);
    if (ahead < 1.1f || ahead > 220.f) return false;
    float row = kScale / ahead;
    sy = kHorizon + row;
    if (sy < kHorizon - 2.f || sy > gs::SCREEN_H + 12.f) return false;
    float viewU = mode_ == Mode::Title ? 0.f : u_;
    float viewZ = mode_ == Mode::Title ? (kMarkZ - 22.f) : z_;
    float hw = kHw * row;
    float cx = 160.f + (bend(z) - bend(viewZ) - viewU) * hw;
    sx = cx + lat * hw;
    sh = row * 0.36f;
    return true;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 60.f);
    const float viewZ = mode_ == Mode::Title ? kMarkZ - 22.f : z_;
    const float viewU = mode_ == Mode::Title ? 0.f : u_;
    const float shakeX = shake_ > 0.f ? std::sin(t_ * 47.f) * shake_ * 3.f : 0.f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        if (y <= int(kHorizon)) {
            float sky = float(y) / kHorizon;
            v.lineBackdrop[y] = gs::rgb4(4 + int((1.f - sky) * 3.f), 6 + int(sky * 3.f), 10 + int((1.f - sky) * 3.f));
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y) - kHorizon;
        float ahead = kScale / std::max(row, 1.f);
        float wz = viewZ + ahead;
        float sea = std::clamp(row / 130.f, 0.f, 1.f);
        v.lineBackdrop[y] = gs::rgb4(1, 2 + int(sea * 3.f), 5 + int((1.f - sea) * 4.f));
        v.lineFog[y] = uint8_t(std::clamp(int(8.f - row * 0.055f), 0, 8));
        if (wz > kEnd) {
            r.on = false;
            continue;
        }
        r.on = true;
        bool paint = std::fabs(wz - kMarkZ) <= kHalfLen;
        float hw = kHw * row;
        r.hw = hw;
        r.cx = 160.f + shakeX + (bend(wz) - bend(viewZ) - viewU) * hw;
        r.v = wz * 34.f;
        r.pal = paint ? PAL_MARK : PAL_SHELF;
        r.style = paint ? 1 : gs::ROAD_ROCKY;
        r.band = (int(std::floor(wz / 6.f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
    }

    auto prop = [&](float lat, float z, float mul, const gs::Mipped& img, int pal) {
        float sx, sy, sh;
        if (!project(lat, z, sx, sy, sh)) return;
        spr(img, sx + shakeX, sy, std::max(4.f, sh * mul), pal, lat < 0.f);
    };

    for (int i = 0; i < 7; i++) {
        float zz = std::floor(viewZ / 18.f) * 18.f + float(i) * 18.f;
        if (zz > kEnd - 2.f) continue;
        float side = (i & 1) ? 1.28f : -1.28f;
        prop(side, zz, 2.0f, art_.crag, PAL_CRAG);
    }
    prop(-0.55f, kMarkZ, 1.8f, art_.post, PAL_POST);
    prop(0.55f, kMarkZ, 1.8f, art_.post, PAL_POST);
    prop(0.f, kMarkZ, 0.7f, art_.paint, PAL_PAINT);
    prop(0.f, kEnd - 0.4f, 1.15f, art_.ribbon, PAL_RIBBON);
    prop(-0.7f, kEnd - 0.2f, 1.6f, art_.post, PAL_POST);
    prop(0.7f, kEnd - 0.2f, 1.6f, art_.post, PAL_POST);

    if (mode_ != Mode::Title) {
        float lean = u_ * 14.f;
        spr(art_.cart, 160.f + lean + shakeX, 206.f, 46.f, PAL_CART, false);
    }

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(7, "S3 CLIFF MARK", 3);
        hudC(11, "SET DOWN ON THE MARK", 1);
        hudC(13, "MISSING THE END FAILS", 4);
        hudC(17, "LEFT RIGHT  STEER", 1);
        hudC(18, "UP GAS   DOWN BRAKE", 1);
        hudC(19, "X REVERSE ONTO THE PAINT", 1);
        if ((int(t_ * 2.f) & 1) == 0) hudC(23, "PRESS START", 3);
    } else if (mode_ == Mode::Win) {
        hudC(8, "SET DOWN", 5);
        hudC(10, "THE LEG IS MADE", 1);
    } else if (mode_ == Mode::Fail) {
        hudC(8, "LEG FAILED", 4);
        std::string up = why_;
        for (char& c : up)
            if (c >= 'a' && c <= 'z') c = char(c - 32);
        hudC(10, up, 1);
        hudC(14, "PRESS START", 3);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", 3);
        hudC(13, "START TO ROLL", 1);
    } else {
        std::snprintf(line, sizeof line, "SPD %d", int(std::fabs(speed_) * 4.f));
        hud(1, 1, line, 1);
        int left = int(std::max(0.f, kEnd - noseZ()));
        std::snprintf(line, sizeof line, "END %d", left);
        hud(31, 1, line, left < 8 ? 4 : 3);
        std::snprintf(line, sizeof line, "CLK %d", int(std::ceil(std::max(clock_, 0.f))));
        hud(1, 26, line, clock_ < 8.f ? 4 : 1);
        if (hold_ > 0.02f) hudC(3, "HOLD THE SET", 5);
        else if (onMark()) hudC(3, "NOSE IS ON IT  STOP", 5);
        else if (std::fabs(noseErr_) < 3.f) hudC(3, "CLOSE IS STILL OFF", 4);
        else if (noseErr_ < 16.f) hudC(3, "SET THE NOSE ON THE MARK", 3);
    }
    hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, 6);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(380.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceT_ += kDt;
            clock_ -= kDt;
            float gas, brake, steer, rev;
            if (bot_) pilot(gas, brake, steer, rev);
            else controls(gas, brake, steer, rev);
            physics(kDt, gas, brake, steer, rev);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }

    if (chime_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {392.f, 494.f, 587.f, 784.f, 988.f};
            sys_->apu.tone(0, notes[std::min(5 - chime_, 4)], 0.05f);
            tone_ = 0.14f;
            chimeT_ = 0.12f;
            if (--chime_ <= 0) chime_ = 0;
        }
    } else if (tone_ > 0.f) {
        tone_ -= kDt;
        if (tone_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    } else if (mode_ == Mode::Run && std::fabs(speed_) > 0.6f) {
        sys_->apu.tone(1, 46.f + std::fabs(speed_) * 3.6f, 0.028f);
    } else {
        sys_->apu.tone(1, 0.f, 0.f);
    }
    if (mode_ == Mode::Win) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 36, 24);
    else if (hold_ > 0.02f) sys.setLight(200, 160, 40);
    draw();
}

}  // namespace cliffmark
