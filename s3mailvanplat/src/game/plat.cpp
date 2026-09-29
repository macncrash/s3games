#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mailplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kFace = 78.f;
constexpr float kMark = 80.55f;
constexpr float kEnd = 90.f;
constexpr float kPlat = 1.62f;
constexpr float kXTol = 0.36f;
constexpr float kHTol = 0.10f;
constexpr float kStopSpd = 0.07f;
constexpr float kHoldNeed = 0.85f;
constexpr float kDrive = 7.4f;
constexpr float kBrake = 16.f;
constexpr float kLift = 1.05f;
constexpr float kVan = 8.2f;
constexpr float kLegLimit = 36.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto c = [&](int x, int y) { return int(std::lround(x + (y - x) * t)); };
    return gs::rgb4(c(ar, br), c(ag, bg), c(ab, bb));
}

}  // namespace

void Game::showTitle() {
    mode_ = Mode::Title;
    door_ = 18.f;
    deck_ = kPlat;
    v_ = 0.f;
    hold_ = 0.f;
    idle_ = 0.f;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    camX_ = kMark - 6.f;
    camH_ = 1.6f;
    camS_ = 11.f;
    anim_ = 0.f;
}

void Game::startRun() {
    door_ = 8.f;
    deck_ = 0.62f;
    v_ = 5.1f;
    lift_ = 0.f;
    liftV_ = 0.f;
    thrust_ = 0.f;
    hold_ = 0.f;
    idle_ = 0.f;
    legT_ = 0.f;
    atDock_ = false;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    chime_ = -1;
    shake_ = 0.f;
    mode_ = Mode::Run;
    camX_ = door_ + 6.f;
    camH_ = 1.5f;
    camS_ = 13.f;
    blip(440.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 7, 8));
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.05f, 0.08f, 0.04f);
    if (bot_) startRun();
    else showTitle();
}

void Game::pilot(float& thrust, float& lift) const {
    float err = kMark - door_;
    float hErr = kPlat - deck_;
    lift = clampf(hErr * 5.2f - liftV_ * 0.9f, -1.f, 1.f);
    if (door_ > kFace - 16.f && deck_ > kPlat + 0.06f) lift = -1.f;

    if (err < 7.f && std::fabs(deck_ - kPlat) < 0.32f) {
        if (door_ < kMark - 0.08f) {
            float want = clampf(err * 0.42f, 0.16f, 0.9f);
            thrust = clampf((want - v_) * 3.2f, -1.f, 1.f);
        } else if (v_ > 0.05f || door_ > kMark + kXTol * 0.45f) {
            thrust = v_ > 0.35f ? -1.f : -0.55f;
        } else if (v_ < -0.02f) {
            thrust = 0.45f;
        } else {
            thrust = 0.f;
        }
        return;
    }
    float remain = std::max(0.f, err - 2.2f);
    float vStop = std::sqrt(std::max(0.f, 2.f * (kBrake * 0.5f) * remain));
    float vWant = err > 22.f ? 6.0f : std::min(4.6f, std::max(0.7f, vStop * 0.72f));
    if (v_ > vWant + 0.12f) thrust = -1.f;
    else if (v_ < vWant - 0.18f) thrust = 1.f;
    else thrust = 0.12f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "level";
    banner_ = "LEVEL";
    deck_ = kPlat;
    v_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.2f, 0.06f, 140);
    sys_->setLight(30, 150, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = "MISSED";
    if (why && std::strstr(why, "scrap")) banner_ = "SCRAPED";
    else if (why && std::strstr(why, "under")) banner_ = "TOO LOW";
    else if (why && std::strstr(why, "past")) banner_ = "PAST";
    else if (why && std::strstr(why, "not level")) banner_ = "NOT LEVEL";
    else if (why && std::strstr(why, "late")) banner_ = "TOO LATE";
    shake_ = 1.f;
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(150, 30, 20);
    sys_->apu.noiseBurst(0.32f, 180.f, 0.24f);
}

void Game::physics(float thrust, float lift) {
    thrust_ = clampf(thrust, -1.f, 1.f);
    lift_ = clampf(lift, -1.f, 1.f);
    legT_ += kDt;

    liftV_ += (lift_ - liftV_) * 7.f * kDt;
    deck_ += liftV_ * kLift * kDt;
    deck_ = clampf(deck_, 0.35f, 2.85f);

    float accel = thrust_ > 0.f ? thrust_ * kDrive : thrust_ * kBrake;
    accel -= v_ * 0.12f;
    v_ += accel * kDt;
    v_ = clampf(v_, -2.4f, 7.2f);
    if (std::fabs(v_) < 0.03f && std::fabs(thrust_) < 0.06f) v_ = 0.f;
    door_ += v_ * kDt;

    float nose = door_ + kVan;
    atDock_ = door_ > kFace - 6.f && door_ < kEnd;

    if (door_ > kEnd) {
        fail("past the platform");
        return;
    }
    if (door_ > kFace + 0.25f && door_ < kEnd && deck_ > kPlat + 0.52f) {
        fail("scraped the platform");
        return;
    }
    if (door_ > kFace - 0.05f && door_ < kEnd && deck_ < kPlat - 0.78f) {
        fail("under the lip");
        return;
    }
    if (v_ < -0.25f && door_ < kFace - 12.f && legT_ > 4.f) {
        fail("turned back");
        return;
    }
    (void)nose;

    bool atMark = std::fabs(door_ - kMark) <= kXTol;
    bool level = std::fabs(deck_ - kPlat) <= kHTol;
    bool stopped = std::fabs(v_) <= kStopSpd;
    if (atMark && level && stopped && door_ >= kFace - 0.2f) {
        hold_ += kDt;
        idle_ = 0.f;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else {
        hold_ = 0.f;
    }

    bool settled = std::fabs(v_) <= 0.045f && thrust_ <= 0.02f;
    if (settled && !(atMark && level)) {
        idle_ += kDt;
        if (idle_ > 1.05f && door_ > 30.f) {
            if (door_ > kFace && !level) fail("not level");
            else fail("missed the platform");
            return;
        }
    } else if (!settled) {
        idle_ = 0.f;
    }
    if (mode_ == Mode::Run && legT_ > kLegLimit) fail("too late");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
}

void Game::audio() {
    if (mode_ != Mode::Run) {
        sys_->apu.noise(0.f, 400.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
    } else {
        float roll = clampf(std::fabs(v_) / 7.f, 0.f, 1.f) * 0.05f;
        sys_->apu.noise(roll, 280.f + std::fabs(v_) * 30.f, false);
        if (std::fabs(liftV_) > 0.18f) sys_->apu.tone(2, 90.f + std::fabs(liftV_) * 40.f, 0.03f);
        else sys_->apu.tone(2, 0.f, 0.f);
        bool at = std::fabs(door_ - kMark) <= kXTol && std::fabs(deck_ - kPlat) <= kHTol;
        if (at) sys_->setLight(30, 140, 70);
        else if (door_ > kFace - 8.f) sys_->setLight(40, 90, 30);
        else sys_->setLight(20, 40, 70);
    }
    if (chime_ >= 0) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f && chime_ < 4) {
            sys_->apu.tone(0, notes[chime_], 0.05f);
            chimeT_ = 0.14f;
            chime_++;
        }
        if (chime_ >= 4) {
            chime_ = -1;
            sys_->apu.tone(0, 0.f, 0.f);
        }
    }
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 6, 11);
    uint16_t mid = gs::rgb4(7, 10, 13);
    uint16_t hor = gs::rgb4(13, 12, 9);
    if (mode_ == Mode::Fail) hor = lerpC(hor, gs::rgb4(12, 4, 3), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(8, 13, 8), 0.4f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.55f ? lerpC(zen, mid, t / 0.55f) : lerpC(mid, hor, (t - 0.55f) / 0.45f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

float Game::sx(float wx) const {
    float j = shake_ > 0.f ? std::sin(anim_ * 47.f) * shake_ * 3.f : 0.f;
    return (wx - camX_) * camS_ + 150.f + j;
}

float Game::sy(float wy) const {
    float j = shake_ > 0.f ? std::cos(anim_ * 39.f) * shake_ * 2.f : 0.f;
    return 168.f - (wy - camH_) * camS_ + j;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    bool framing = mode_ == Mode::Win || mode_ == Mode::Fail || hold_ > 0.02f;
    float wantS = framing ? 16.f : 13.5f;
    float wantX = framing ? kMark + 2.2f : door_ + 4.5f + clampf(v_ * 0.35f, 0.f, 6.f);
    float wantH = framing ? kPlat : 1.4f;
    if (mode_ == Mode::Title) {
        wantX = kMark - 2.f;
        wantH = kPlat;
        wantS = 12.f;
    }
    float k = (mode_ == Mode::Title) ? 1.f : 0.08f;
    camX_ += (wantX - camX_) * k;
    camH_ += (wantH - camH_) * k;
    camS_ += (wantS - camS_) * k;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.4f);

    float ground = sy(0.f);
    for (float x = std::floor((camX_ - 20.f) / 4.f) * 4.f; x < camX_ + 28.f; x += 4.f) {
        spr(art_.road, sx(x + 2.f), (ground + sy(-0.15f)) * 0.5f + 6.f, std::max(8.f, camS_ * 0.7f), PAL_ROAD);
    }

    float dockTop = sy(kPlat);
    float dockBot = sy(-0.2f);
    spr(art_.office, sx(kFace + 10.f), sy(2.6f), camS_ * 3.4f, PAL_OFFICE);
    spr(art_.dock, sx((kFace + kEnd) * 0.5f), (dockTop + dockBot) * 0.5f, std::max(10.f, dockBot - dockTop), PAL_DOCK);
    spr(art_.stripe, sx(kFace + 3.f), dockTop + 3.f, 8.f, PAL_STRIPE);
    spr(art_.lamp, sx(kFace - 1.2f), sy(2.1f), camS_ * 1.6f, PAL_LAMP);
    spr(art_.crate, sx(kMark + 3.4f), sy(kPlat + 0.35f), camS_ * 0.7f, PAL_BOX);
    spr(art_.sack, sx(kMark + 5.2f), sy(kPlat + 0.32f), camS_ * 0.62f, PAL_SACK);

    float markX = sx(kMark);
    spr(art_.stripe, markX, dockTop - 6.f, 10.f, PAL_MARK);

    float bodyH = camS_ * 1.55f;
    float vanCx = sx(door_ + kVan * 0.48f);
    float vanCy = sy(deck_ + 0.55f);
    spr(art_.van, vanCx, vanCy, bodyH, PAL_VAN);
    float wheelH = camS_ * 0.55f;
    spr(art_.wheel, sx(door_ + 1.5f), sy(0.22f), wheelH, PAL_WHEEL);
    spr(art_.wheel, sx(door_ + kVan - 1.6f), sy(0.22f), wheelH, PAL_WHEEL);
    spr(art_.sack, sx(door_ + 3.2f), sy(deck_ + 0.95f), camS_ * 0.45f, PAL_SACK);

    int flap = int(anim_ * 4.f) & 1;
    spr(art_.bird[flap], sx(kFace - 18.f + std::sin(anim_ * 0.7f) * 4.f), sy(4.2f), 10.f, PAL_BIRD);
    spr(art_.cloud, sx(20.f + std::fmod(anim_ * 1.2f, 40.f)), sy(5.4f), 16.f, PAL_SKY);
    spr(art_.cloud, sx(48.f), sy(5.8f), 12.f, PAL_SKY);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 MAIL VAN", PAL_AMBER);
        hudC(5, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(8, "RIGHT GO   LEFT BRAKE", PAL_HUD);
        hudC(9, "UP RAISE THE BED   DOWN LOWER", PAL_HUD);
        hudC(12, "THE DOOR MUST SIT FLUSH", PAL_HUD);
        hudC(13, "CLOSE IS NOT LEVEL", PAL_AMBER);
        hudC(22, "START", PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_AMBER);
        hudC(12, "START TO ROLL", PAL_HUD);
    } else {
        std::snprintf(line, sizeof line, "BED %+.2f  DOCK %.2f", deck_ - kPlat, kPlat);
        hud(1, 1, line, std::fabs(deck_ - kPlat) <= kHTol ? PAL_GOOD : PAL_HUD);
        std::snprintf(line, sizeof line, "DOOR %+.2f", door_ - kMark);
        hud(1, 2, line, std::fabs(door_ - kMark) <= kXTol ? PAL_GOOD : PAL_HUD);
        std::snprintf(line, sizeof line, "%.0f", std::max(0.f, kLegLimit - legT_));
        hud(36, 1, line, legT_ > kLegLimit - 8.f ? PAL_BAD : PAL_HUD);
        if (hold_ > 0.02f && mode_ == Mode::Run) hudC(4, "HOLD", PAL_GOOD);
        if (mode_ == Mode::Win) {
            hudC(8, "LEVEL", PAL_GOOD);
            hudC(10, "STOPPED WITH THE PLATFORM", PAL_HUD);
        } else if (mode_ == Mode::Fail) {
            hudC(8, banner_, PAL_BAD);
            if (why_ && why_[0]) hudC(10, why_, PAL_HUD);
            hudC(22, "START TO TRY AGAIN", PAL_AMBER);
        }
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.05f) return 3;
    if (atDock_ || door_ > kFace - 4.f) return 2;
    return 1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || bot_) startRun();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) {
            mode_ = Mode::Run;
            blip(520.f);
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else {
            float thrust = 0.f, lift = 0.f;
            if (bot_) {
                pilot(thrust, lift);
            } else {
                if (p.down(gs::BTN_RIGHT) || p.accel > 0.15f) thrust += 1.f;
                if (p.down(gs::BTN_LEFT) || p.brake > 0.15f) thrust -= 1.f;
                if (std::fabs(p.axisX) > 0.2f) thrust = p.axisX;
                if (p.down(gs::BTN_UP)) lift += 1.f;
                if (p.down(gs::BTN_DOWN)) lift -= 1.f;
                if (p.down(gs::BTN_A)) lift += 1.f;
                if (p.down(gs::BTN_B)) lift -= 1.f;
            }
            physics(thrust, lift);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        }
    }
    audio();
    draw();
    sys.vdp.hudEnabled = true;
}

}  // namespace mailplat
