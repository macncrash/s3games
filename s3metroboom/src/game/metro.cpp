#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace metro {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 108.f;
constexpr float kAccel = 100.f;
constexpr float kBrake = 170.f;
constexpr float kDrag = 0.4f;
constexpr float kMax = 108.f;
constexpr float kCrew = 40.f;
constexpr float kBoom0 = 1480.f;
constexpr float kBoom1 = 1600.f;
constexpr float kMiss = 1680.f;
constexpr float kStart = 160.f;

struct Gate {
    float x;
    float limit;
};
const Gate kGates[2] = {{520.f, 48.f}, {1000.f, 40.f}};

}  // namespace

float Game::cam() const {
    if (mode_ == Mode::Title) return 240.f;
    return nose_ - 120.f;
}

bool Game::inBoom() const { return nose_ >= kBoom0 && nose_ <= kBoom1 && std::fabs(speed_) < 7.f; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int shade = y < 36 ? 1 : (y < 150 ? 2 : 1);
        sys.vdp.lineBackdrop[y] = gs::rgb4(shade, shade + 1, shade + 2);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    seated_ = false;
    strikes_ = 0;
    clock_ = 0;
    crewLeft_ = kCrew;
    nose_ = 420.f;
    speed_ = 0;
    msgT_ = 0;
    why_ = nullptr;
    song_ = -1;
    songT_ = 0;
    crossed_[0] = crossed_[1] = false;
    if (sys_) sys_->apu.tone(0, 0, 0);
}

void Game::begin() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    seated_ = false;
    strikes_ = 0;
    clock_ = 0;
    crewLeft_ = kCrew;
    nose_ = kStart;
    speed_ = 0;
    msgT_ = 1.2f;
    why_ = nullptr;
    crossed_[0] = crossed_[1] = false;
}

void Game::pilot(bool& gas, bool& brake) const {
    gas = false;
    brake = false;
    float hold = 100.f;
    if (nose_ > 470.f && nose_ < 560.f) hold = 36.f;
    else if (nose_ >= 560.f && nose_ < 940.f) hold = 100.f;
    else if (nose_ >= 940.f && nose_ < 1040.f) hold = 32.f;
    else if (nose_ >= 1040.f && nose_ < 1420.f) hold = 100.f;
    else if (nose_ < 1472.f) hold = 18.f;
    else if (nose_ < 1540.f) hold = 4.f;
    else hold = 0.f;
    if (speed_ > hold + 1.2f) brake = true;
    else if (speed_ < hold - 1.2f && nose_ < 1548.f) gas = true;
}

void Game::physics(float dt, bool gas, bool brake) {
    if (gas) speed_ += kAccel * dt;
    if (brake) speed_ -= kBrake * dt;
    if (!gas && !brake) {
        if (speed_ > 0) speed_ = std::max(0.f, speed_ - kDrag);
        else speed_ = std::min(0.f, speed_ + kDrag);
    }
    speed_ = std::max(-24.f, std::min(kMax, speed_));
    float prev = nose_;
    nose_ += speed_ * dt;
    if (nose_ < 40.f) {
        nose_ = 40.f;
        speed_ = 0;
    }
    for (int i = 0; i < 2; i++) {
        if (crossed_[i]) continue;
        if (prev < kGates[i].x && nose_ >= kGates[i].x) {
            crossed_[i] = true;
            if (speed_ > kGates[i].limit) {
                strikes_++;
                speed_ = kGates[i].limit * 0.5f;
                msgT_ = 1.1f;
                why_ = "GATE TOO FAST";
                if (sys_) sys_->apu.noiseBurst(0.2f, 900.f, 0.15f);
            }
        }
    }
}

void Game::judge() {
    crewLeft_ = std::max(0.f, kCrew - clock_);
    if (seated_) return;
    if (inBoom()) {
        seated_ = true;
        won_ = true;
        over_ = true;
        mode_ = Mode::Over;
        why_ = nullptr;
        if (sys_) sys_->apu.tone(0, 523.f, 0.08f);
        return;
    }
    if (strikes_ >= 3) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Over;
        why_ = "DRIVE JARRED OFF";
        return;
    }
    if (nose_ > kMiss) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Over;
        why_ = "MISSED THE BOOM";
        return;
    }
    if (clock_ >= kCrew) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Over;
        why_ = "CREW TOOK THE BOOM";
    }
}

void Game::audio(float dt) {
    if (!sys_) return;
    gs::APU& apu = sys_->apu;
    if (mode_ == Mode::Run && speed_ > 8.f) apu.noise(0.03f, 400.f + speed_ * 6.f, true);
    else apu.noise(0, 0, false);
    songT_ -= dt;
    if (songT_ > 0.f) return;
    static const float notes[] = {220, 277, 330, 277, 247, 330, 370, 330};
    song_ = (song_ + 1) % 8;
    float vol = (mode_ == Mode::Title || won_) ? 0.06f : 0.03f;
    apu.tone(0, notes[song_], vol);
    songT_ = (song_ % 2) ? 0.26f : 0.16f;
}

void Game::lights() {
    if (!sys_) return;
    if (won_) sys_->setLight(40, 180, 90);
    else if (mode_ == Mode::Over) sys_->setLight(180, 30, 30);
    else if (crewLeft_ < 4.f && mode_ == Mode::Run) sys_->setLight(200, 70, 20);
    else sys_->setLight(30, 80, 140);
}

void Game::blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!sys_) return;
    if (cx + w * 0.5f < -12 || cx - w * 0.5f > gs::SCREEN_W + 12) return;
    if (cy + h * 0.5f < -12 || cy - h * 0.5f > gs::SCREEN_H + 12) return;
    gs::Sprite s;
    s.img = m.pick(h);
    s.x = int16_t(cx - w * 0.5f);
    s.y = int16_t(cy - h * 0.5f);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (; *s && col < 40; ++s, ++col) {
        unsigned char c = static_cast<unsigned char>(*s);
        if (c < 32 || c > 127) c = ' ';
        sys_->vdp.HUD.set(col, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    int col = std::max(0, (40 - n) / 2);
    hudText(col, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float c = cam();
    vdp.B.scroll(int(c * 0.4f), 0);
    vdp.A.scroll(int(c), 0);
    auto sx = [&](float wx) { return wx - c; };

    for (int i = 0; i < 8; i++) {
        float x = 180.f + i * 190.f;
        blit(art_.lamp, sx(x), 78, 14, 32, PAL_LAMP, false);
    }
    for (int i = 0; i < 2; i++) {
        float x = sx(kGates[i].x);
        bool hot = !crossed_[i] && mode_ == Mode::Run;
        blit(art_.gate, x, 128, 18, 40, hot ? PAL_GATE : PAL_LAMP, false);
    }

    float boomX = sx((kBoom0 + kBoom1) * 0.5f);
    blit(art_.boom, boomX + 10, 118, 86, 62, PAL_BOOM, false);
    blit(art_.hook, boomX + 28, 96 + std::sin(anim_ * 3.f) * 2.f, 12, 18, PAL_BOOM, false);

    float rival = kStart + (kBoom0 - kStart) * std::min(clock_ / kCrew, 1.f);
    if (mode_ == Mode::Title) rival = 260.f + std::fmod(anim_ * 28.f, 220.f);
    blit(art_.crew, sx(rival) - 70, 64, 86, 28, PAL_CREW, false);

    float carX = sx(nose_) - kLen * 0.5f;
    blit(art_.car, carX, 150, 112, 44, PAL_CAR, false);
    if (!seated_) blit(art_.drive, carX + 18, 118, 22, 16, PAL_DRIVE, false);
    else blit(art_.drive, boomX + 24, 108, 22, 16, PAL_DRIVE, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 METROBOOM", PAL_HUD);
        hudC(6, "DELIVER THE DRIVE TO THE BOOM", PAL_HUD);
        hudC(8, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(12, "RIGHT GO    LEFT BRAKE", PAL_HUD);
        hudC(14, "EASE THROUGH THE GATES", PAL_HUD);
        hudC(18, "ENTER TO ROLL", PAL_HUD);
        return;
    }

    int sec = int(clock_);
    std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
    hudText(1, 26, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "SPD %02d", int(std::fabs(speed_)));
    hudText(14, 26, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "CREW %4.1f", crewLeft_);
    hudText(24, 26, buf, crewLeft_ < 4.f ? PAL_HUD : PAL_HUD);
    std::snprintf(buf, sizeof buf, "GATES %d", strikes_);
    hudText(1, 1, buf, strikes_ ? PAL_HUD : PAL_HUD);
    if (nose_ > kBoom0 - 180.f && !seated_) hudText(28, 1, "BOOM", PAL_HUD);

    if (mode_ == Mode::Over && won_) {
        hudC(8, "DRIVE ON THE BOOM", PAL_HUD);
        std::snprintf(buf, sizeof buf, "CREW %.1f S BEHIND", crewLeft_);
        hudC(10, buf, PAL_HUD);
        hudC(13, "ENTER RIDES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(8, why_ ? why_ : "LOST THE LINE", PAL_HUD);
        hudC(10, "THE OTHER CREW KEEPS THE CLOCK", PAL_HUD);
        hudC(13, "ENTER RIDES AGAIN", PAL_HUD);
    } else if (msgT_ > 0.f && why_) {
        hudC(10, why_, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    if (msgT_ > 0.f) msgT_ -= kDt;

    if (bot_ && mode_ == Mode::Title) begin();

    if (mode_ == Mode::Title) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        audio(kDt);
        lights();
        draw();
        return;
    }
    if (mode_ == Mode::Over) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) showTitle();
        audio(kDt);
        lights();
        draw();
        return;
    }

    if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
        showTitle();
        audio(kDt);
        lights();
        draw();
        return;
    }

    bool gas = false, brake = false;
    if (bot_) pilot(gas, brake);
    else {
        gas = sys.pad.down(gs::BTN_RIGHT) || sys.pad.down(gs::BTN_A) || sys.pad.accel > 0.2f;
        brake = sys.pad.down(gs::BTN_LEFT) || sys.pad.down(gs::BTN_B) || sys.pad.brake > 0.2f;
        if (sys.pad.axisX > 0.3f) gas = true;
        if (sys.pad.axisX < -0.3f) brake = true;
    }
    physics(kDt, gas, brake);
    clock_ += kDt;
    judge();
    audio(kDt);
    lights();
    draw();
}

}  // namespace metro
