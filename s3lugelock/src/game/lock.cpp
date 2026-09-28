#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lugelock {
namespace {

constexpr int HORIZON = 78;
constexpr float FOCAL = 200.f;
constexpr float CAM_H = 1.55f;
constexpr float HALF_W = 2.15f;
constexpr float DT = 1.f / 60.f;
constexpr float LOWER_Z = 64.f;
constexpr float UPPER_Z = 132.f;
constexpr float FINISH_Z = 196.f;
constexpr float WALL = 0.97f;
constexpr float NEAR_Z = 5.5f;
constexpr float FAR_Z = 220.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(10, 13, 15));
    sys.apu.setMaster(0.45f);
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
    mode_ = Mode::Title;
    modeTime_ = 0;
    over_ = false;
    won_ = false;
    why_ = "";
}

void Game::begin() {
    mode_ = Mode::Run;
    modeTime_ = 0;
    raceTime_ = 0;
    playerZ_ = 14.f;
    prevZ_ = playerZ_;
    playerX_ = 0;
    latV_ = 0;
    speed_ = 15.f;
    steerSm_ = 0;
    lower_ = Door::Shut;
    upper_ = Door::Shut;
    lowerOpen_ = 0;
    upperOpen_ = 0;
    hold_ = 0;
    fill_ = 0;
    shake_ = 0;
    chime_ = 0;
    won_ = false;
    over_ = false;
    why_ = "";
    tucking_ = false;
    braking_ = false;
}

void Game::scrape(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Result;
    modeTime_ = 0;
    shake_ = 1.f;
    speed_ = 0;
    sys_->apu.noiseBurst(0.45f, 1400.f, 0.18f);
}

void Game::finish() {
    if (mode_ != Mode::Run) return;
    why_ = "clear";
    won_ = true;
    over_ = true;
    mode_ = Mode::Result;
    modeTime_ = 0;
    gs::FMPatch p;
    p.alg = 0;
    p.vol = 0.28f;
    p.op[0].mul = 2;
    p.op[0].level = 1;
    p.op[0].ar = 0.01f;
    p.op[0].dr = 0.4f;
    p.op[0].sl = 0.3f;
    p.op[0].rr = 0.6f;
    sys_->apu.setPatch(0, p);
    sys_->apu.keyOn(0, 523.f, 0.3f);
    chime_ = 0.4f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Result) return 4;
    if (playerZ_ > UPPER_Z + 6.f) return 3;
    if (playerZ_ > LOWER_Z + 3.f) return 2;
    return 1;
}

void Game::pilot(float& steer, bool& tuck, bool& brake) {
    const gs::Pad& pad = sys_->pad;
    if (bot_) {
        steer = std::clamp(-playerX_ * 4.2f - latV_ * 0.85f, -1.f, 1.f);
        tuck = false;
        brake = false;
        const bool lowerReady = lowerOpen_ > 0.94f;
        const bool upperReady = upperOpen_ > 0.94f;
        if (!lowerReady && playerZ_ > 28.f && playerZ_ < LOWER_Z) brake = true;
        else if (playerZ_ > LOWER_Z + 3.f && playerZ_ < UPPER_Z - 2.f && !upperReady) brake = true;
        else tuck = true;
        return;
    }
    float ax = pad.axisX;
    if (std::fabs(ax) < 0.18f) ax = 0;
    float key = (pad.down(gs::BTN_RIGHT) ? 1.f : 0.f) - (pad.down(gs::BTN_LEFT) ? 1.f : 0.f);
    steer = std::clamp(ax + key, -1.f, 1.f);
    tuck = pad.down(gs::BTN_A) || pad.down(gs::BTN_Z) || pad.down(gs::BTN_UP);
    brake = pad.down(gs::BTN_B) || pad.down(gs::BTN_X) || pad.down(gs::BTN_DOWN) || pad.brake > 0.2f;
}

void Game::crossGates() {
    auto hit = [&](float gz, float open, const char* why) {
        if (prevZ_ < gz && playerZ_ >= gz) {
            float safe = 0.10f + open * 0.80f;
            if (std::fabs(playerX_) > safe) scrape(why);
        }
    };
    hit(LOWER_Z, lowerOpen_, "scraped the lower gate");
    if (mode_ != Mode::Run) return;
    hit(UPPER_Z, upperOpen_, "scraped the upper gate");
}

void Game::physics(float steer, bool tuck, bool brake) {
    tucking_ = tuck;
    braking_ = brake;
    steerSm_ = steerSm_ + (steer - steerSm_) * 0.22f;
    float latA = steerSm_ * 6.8f - latV_ * 3.4f - playerX_ * 0.35f;
    latV_ += latA * DT;
    playerX_ += latV_ * DT;
    if (std::fabs(playerX_) > WALL) {
        scrape("hit the wall");
        return;
    }
    float acc = brake ? -22.f : (tuck ? 11.5f : 7.2f);
    speed_ = std::clamp(speed_ + acc * DT, 0.f, tuck ? 30.f : 22.f);
    prevZ_ = playerZ_;
    playerZ_ += speed_ * DT;

    auto door = [&](Door& d, float& open) {
        if (d == Door::Opening) {
            open = std::min(1.f, open + DT / 0.72f);
            if (open >= 1.f) d = Door::Open;
        } else if (d == Door::Closing) {
            open = std::max(0.f, open - DT / 0.5f);
            if (open <= 0.f) d = Door::Shut;
        }
    };
    door(lower_, lowerOpen_);
    door(upper_, upperOpen_);

    const bool inHold = playerZ_ > 38.f && playerZ_ < 56.f && std::fabs(playerX_) < 0.42f && speed_ < 6.f;
    if (lower_ == Door::Shut && inHold) {
        hold_ += DT;
        if (hold_ > 0.42f) {
            lower_ = Door::Opening;
            sys_->apu.noiseBurst(0.16f, 420.f, 0.12f);
        }
    } else if (lower_ == Door::Shut) {
        hold_ = std::max(0.f, hold_ - DT * 0.5f);
    }
    if (playerZ_ > LOWER_Z + 8.f && (lower_ == Door::Open || lower_ == Door::Opening)) lower_ = Door::Closing;

    const bool inChamber = playerZ_ > LOWER_Z + 14.f && playerZ_ < UPPER_Z - 14.f && std::fabs(playerX_) < 0.45f &&
                           speed_ < 6.5f && prevZ_ > LOWER_Z;
    if (upper_ == Door::Shut && playerZ_ > LOWER_Z + 4.f && inChamber) {
        fill_ += DT;
        if (fill_ > 0.55f) {
            upper_ = Door::Opening;
            sys_->apu.noiseBurst(0.16f, 520.f, 0.12f);
        }
    } else if (upper_ == Door::Shut) {
        fill_ = std::max(0.f, fill_ - DT * 0.35f);
    }

    crossGates();
    if (mode_ != Mode::Run) return;
    if (playerZ_ >= FINISH_Z) finish();

    raceTime_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT * 1.6f);
    shakeX_ = int(std::sin(raceTime_ * 47.f) * shake_ * 4.f);
    shakeY_ = int(std::cos(raceTime_ * 39.f) * shake_ * 2.f);
}

void Game::audio() {
    if (chime_ > 0) {
        chime_ -= DT;
        if (chime_ <= 0) sys_->apu.keyOff(0);
    }
    if (mode_ != Mode::Run) {
        sys_->apu.tone(0, 0, 0);
        return;
    }
    float vol = braking_ ? 0.03f : 0.05f + speed_ * 0.002f;
    sys_->apu.tone(0, 70.f + speed_ * 4.f, vol);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    modeTime_ += DT;
    if (mode_ == Mode::Title) {
        if (bot_ && modeTime_ > 0.25f) begin();
        else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.typed.find('\n') != std::string::npos)
            begin();
    } else if (mode_ == Mode::Run) {
        float steer = 0;
        bool tuck = false, brake = false;
        pilot(steer, tuck, brake);
        physics(steer, tuck, brake);
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.typed.find('\n') != std::string::npos)) {
        begin();
    }
    audio();
    draw();
}

void Game::queue(float z, const gs::Sprite& s) { draws_.push_back({z, s}); }

void Game::blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog, float z, bool shadow) {
    if (h < 1.5f) return;
    const gs::Image& img = m.pick(h);
    if (img.w <= 0 || img.h <= 0) return;
    float sc = h / float(img.h);
    gs::Sprite s;
    s.img = img;
    s.h = std::max(1, int(std::lround(h)));
    s.w = std::max(1, int(std::lround(img.w * sc)));
    s.x = int(std::lround(cx - s.w * 0.5f));
    s.y = int(std::lround(foot - s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    queue(z, s);
}

void Game::stretch(const gs::Mipped& m, float x, float y, float w, float h, int pal, int fog, float z) {
    if (w < 1.f || h < 1.f) return;
    const gs::Image& img = m.pick(h);
    if (img.w <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = std::max(1, int(std::lround(std::min(w, 420.f))));
    s.h = std::max(1, int(std::lround(h)));
    s.x = int(std::lround(x));
    s.y = int(std::lround(y));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    queue(z, s);
}

void Game::ui(const gs::Image& img, float x, float y) {
    if (img.w <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int(std::lround(x));
    s.y = int(std::lround(y));
    s.pal = PAL_TITLE;
    queue(-4.f, s);
}

Game::Proj Game::project(float worldZ, float roadX) const {
    Proj p{};
    float dz = worldZ - camZ();
    p.z = dz;
    if (dz < NEAR_Z || dz > FAR_Z) return p;
    p.y = float(hor_) + FOCAL * CAM_H / dz;
    p.hw = FOCAL * HALF_W / dz;
    p.x = 160.f + float(shakeX_) + roadX * p.hw;
    p.fog = std::clamp((dz - 28.f) / 16.f, 0.f, 14.f);
    p.ok = p.y > -30.f && p.y < float(gs::SCREEN_H + 30);
    return p;
}

void Game::sky() {
    hor_ = std::clamp(HORIZON + shakeY_, 64, 108);
    gs::VDP& v = sys_->vdp;
    const bool chamber = mode_ != Mode::Title && playerZ_ > LOWER_Z && playerZ_ < UPPER_Z;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= hor_) {
            float u = std::clamp(float(y) / float(hor_), 0.f, 1.f);
            float s = u * u * (3.f - 2.f * u);
            int r = int(std::lround(2.f + (chamber ? 6.f : 10.f) * s));
            int g = int(std::lround(3.f + 7.f * s));
            int b = int(std::lround(8.f + 5.f * s));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = uint8_t(std::clamp((1.f - u) * 3.f, 0.f, 3.f));
        } else {
            v.lineBackdrop[y] = chamber ? gs::rgb4(6, 9, 11) : gs::rgb4(8, 11, 14);
            float dz = FOCAL * CAM_H / float(y - hor_);
            v.lineFog[y] = uint8_t(std::clamp((dz - 40.f) / 22.f, 0.f, 12.f));
        }
    }
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    const float origin = camZ();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& L = v.road[y];
        L = {};
        if (y <= hor_) continue;
        float dz = FOCAL * CAM_H / float(y - hor_);
        if (dz > 240.f) continue;
        L.on = true;
        L.cx = 160.f + float(shakeX_);
        L.hw = FOCAL * HALF_W / dz;
        L.v = origin + dz;
        L.pal = uint8_t(PAL_ICE);
        L.band = (int(std::floor((origin + dz) * 0.22f)) & 1) ? 1 : 0;
        L.style = gs::ROAD_ICE;
        L.left = gs::GROUND_SNOWWALL;
        L.right = gs::GROUND_SNOWWALL;
    }
}

void Game::gateAt(float z, float open, const char* label, bool showSign) {
    Proj p = project(z, 0.f);
    if (!p.ok) return;
    const float postH = FOCAL * 3.5f / p.z;
    const int fog = int(p.fog);
    blit(art_.post, p.x - p.hw * 1.02f, p.y, postH, PAL_POST, false, fog, p.z);
    blit(art_.post, p.x + p.hw * 1.02f, p.y, postH, PAL_POST, true, fog, p.z + 0.01f);
    const float bw = std::min(p.hw * 2.15f, 400.f);
    stretch(art_.beam, p.x - bw * 0.5f, p.y - postH, bw, std::max(2.f, postH * 0.07f), PAL_POST, fog, p.z - 0.2f);

    float gap = (0.10f + open * 0.80f) * 2.f * p.hw;
    float leafH = postH * 0.72f;
    float leftW = std::max(0.f, (p.hw * 2.f - gap) * 0.5f);
    if (leftW > 2.f) {
        stretch(art_.leaf, p.x - p.hw, p.y - leafH, leftW, leafH, PAL_STEEL, fog, p.z - 0.05f);
        stretch(art_.leaf, p.x + p.hw - leftW, p.y - leafH, leftW, leafH, PAL_STEEL, fog, p.z - 0.05f);
    }
    if (showSign && label) {
        float sh = std::max(8.f, postH * 0.16f);
        blit(art_.sign, p.x, p.y - postH - sh * 0.15f, sh, PAL_SIGN, false, fog, p.z - 0.3f);
        (void)label;
    }
}

void Game::rider() {
    const float foot = 206.f + float(hor_ - HORIZON);
    const bool hard = std::fabs(steerSm_) > 0.28f;
    const gs::Mipped* me = &art_.flat;
    float ph = 58.f;
    if (hard) me = &art_.lean;
    else if (tucking_) {
        me = &art_.tuck;
        ph = 50.f;
    }
    const float px = 160.f + steerSm_ * 12.f + float(shakeX_);
    blit(art_.shadow, px, foot + 2.f, 12.f, PAL_FX, false, 0, 0.4f, true);
    blit(*me, px, foot, ph, PAL_RIDER, hard && steerSm_ > 0.f, 0, 0.05f);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        if (x < 0 || x > 39 || row < 0 || row > 27) continue;
        unsigned char ch = (unsigned char)s[i];
        if (ch < 32 || ch > 126) ch = '?';
        int t = art_.font[ch];
        if (!t) continue;
        h.set(x, row, gs::entry(t, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hudText(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::hud() {
    gs::Plane& h = sys_->vdp.HUD;
    h.clear();
    if (mode_ == Mode::Title) {
        hudC(23, "ONE JOB  PASS THE LOCK", PAL_GOLD);
        hudC(25, "ARROWS STEER   Z TUCKS   X BRAKES", PAL_HUD);
        hudC(27, "ENTER TO DROP", PAL_GOLD);
        return;
    }
    char line[64];
    if (mode_ == Mode::Result) {
        if (won_) std::snprintf(line, sizeof line, "PASSED  %.1f S", raceTime_);
        else std::snprintf(line, sizeof line, "%s", why_);
        hudC(24, line, won_ ? PAL_GOLD : PAL_ALERT);
        if (!bot_) hudC(26, "ENTER FOR ANOTHER DROP", PAL_HUD);
        return;
    }
    const char* phase = "APPROACH THE LOWER GATE";
    if (playerZ_ > UPPER_Z) phase = "CLEAR  HOLD THE LINE";
    else if (upper_ == Door::Opening || upper_ == Door::Open) phase = "UPPER GATE OPEN";
    else if (playerZ_ > LOWER_Z) phase = "CHAMBER  WAIT FOR THE FILL";
    else if (lower_ == Door::Opening || lower_ == Door::Open) phase = "LOWER GATE OPEN";
    else if (hold_ > 0.05f) phase = "HOLD  THE GATE IS MOVING";
    hudC(26, phase, PAL_HUD);
    std::snprintf(line, sizeof line, "%.0f M   %.0f", std::max(0.f, playerZ_), speed_);
    hudText(1, 1, line, PAL_HUD);
}

void Game::draw() {
    draws_.clear();
    sky();
    road();
    gateAt(LOWER_Z, lowerOpen_, "LOCK", true);
    gateAt(UPPER_Z, upperOpen_, "LOCK", false);
    Proj fin = project(FINISH_Z, 0.f);
    if (fin.ok) {
        float bh = std::max(3.f, FOCAL * 0.35f / fin.z);
        stretch(art_.beam, fin.x - fin.hw, fin.y - bh, fin.hw * 2.f, bh, PAL_SIGN, int(fin.fog), fin.z);
    }
    if (mode_ == Mode::Run && braking_) {
        blit(art_.spark, 148.f, 198.f, 7.f, PAL_FX, false, 0, 0.2f);
        blit(art_.spark, 176.f, 200.f, 6.f, PAL_FX, false, 0, 0.2f);
    }
    rider();
    if (mode_ == Mode::Title) {
        ui(art_.title, (320.f - art_.title.w) * 0.5f, 18.f);
        ui(art_.sub, (320.f - art_.sub.w) * 0.5f, 58.f);
    }
    std::sort(draws_.begin(), draws_.end(), [](const Draw& a, const Draw& b) { return a.z > b.z; });
    sys_->vdp.clearSprites();
    for (const Draw& d : draws_) sys_->vdp.sprite(d.s);
    hud();
}

}  // namespace lugelock
