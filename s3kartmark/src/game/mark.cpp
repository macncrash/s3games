#include "game/mark.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace kartmark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPpm = 9.2f;
constexpr float kGas = 6.4f;
constexpr float kBrake = 9.2f;
constexpr float kDrag = 0.85f;
constexpr float kStop = 0.22f;
constexpr float kHold = 0.40f;
constexpr float kMark = 38.5f;
constexpr float kPaint = 1.55f;
constexpr float kCam = 108.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    sys.apu.setEcho(0.06f, 0.14f, 0.06f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    x_ = 0;
    v_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.1f) return 3;
    if (onPaint()) return 2;
    return 1;
}

bool Game::onPaint() const { return std::fabs(x_ - kMark) <= kPaint; }

bool Game::pastMark() const { return x_ > kMark + kPaint + 0.55f; }

void Game::begin() {
    x_ = 0;
    v_ = 0;
    spin_ = 0;
    hold_ = 0;
    race_ = 0;
    won_ = false;
    over_ = false;
    wasGas_ = false;
    why_[0] = 0;
    report_[0] = 0;
    mode_ = Mode::Run;
    sys_->apu.tone(0, 180.f, 0.04f);
}

void Game::fail(const char* why) {
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 KARTMARK  FAIL  %s  x %.1f  spd %.2f  (%.1f s)", why_, x_, v_, race_);
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    sys_->apu.noiseBurst(0.3f, 640.f, 0.18f);
}

void Game::settleWin() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    std::snprintf(report_, sizeof report_, "S3 KARTMARK  PASS  set down on the mark  x %.1f  spd %.2f  (%.1f s)", x_, v_,
                  race_);
    sys_->apu.tone(0, 523.f, 0.12f);
    sys_->apu.tone(1, 784.f, 0.08f);
}

void Game::controls(bool& gas, bool& brake) {
    gs::Pad& pad = sys_->pad;
    gas = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.2f;
    brake = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.brake > 0.2f;
    if (gas && brake) brake = false;
}

void Game::pilot(bool& gas, bool& brake) {
    float err = kMark - x_;
    float want;
    if (err > 18.f) want = 7.2f;
    else if (err > 9.f) want = 3.6f;
    else if (err > 4.f) want = 1.6f;
    else if (err > 1.3f) want = 0.55f;
    else want = std::clamp(err * 0.7f, -0.3f, 0.32f);
    if (x_ > kMark + 0.35f) want = std::min(want, -0.4f);

    if (std::fabs(err) < 0.62f) {
        if (v_ > 0.05f) brake = true;
        else if (v_ < -0.05f) gas = true;
        else if (err > 0.14f) gas = true;
        else if (err < -0.14f) brake = true;
        return;
    }
    if (v_ < want - 0.08f) gas = true;
    else if (v_ > want + 0.06f) brake = true;
}

void Game::physics(bool gas, bool brake) {
    float a = (gas ? kGas : 0.f) - (brake ? kBrake : 0.f) - kDrag * v_;
    v_ += a * kDt;
    if (v_ > 9.5f) v_ = 9.5f;
    if (v_ < -3.2f) v_ = -3.2f;
    if (!gas && !brake && std::fabs(v_) < 0.08f) v_ = 0;
    if (onPaint() && std::fabs(v_) < kStop && !gas && !brake) v_ = 0;
    x_ += v_ * kDt;
    if (x_ < -4.f) {
        x_ = -4.f;
        if (v_ < 0) v_ = 0;
    }
    spin_ += std::fabs(v_) * 0.55f * kDt;
    if (spin_ >= 1.f) spin_ -= 1.f;
    if (gas && !wasGas_) sys_->apu.noiseBurst(0.07f, 520.f, 0.03f);
    wasGas_ = gas;
    if (mode_ == Mode::Run && gas) sys_->apu.tone(2, 90.f + std::fabs(v_) * 28.f, 0.035f);
    else sys_->apu.tone(2, 0, 0);

    race_ += kDt;
    if (pastMark()) {
        fail("rolled past the mark");
        return;
    }
    if (race_ >= limit_) {
        fail("the clock took the mark");
        return;
    }
    bool quiet = onPaint() && std::fabs(v_) < kStop;
    if (quiet) hold_ += kDt;
    else hold_ = 0;
    if (hold_ >= kHold) settleWin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    tick_++;
    t_ += kDt;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
        } else {
            bool gas = false, brake = false;
            if (bot_) pilot(gas, brake);
            else controls(gas, brake);
            physics(gas, brake);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        v_ *= 0.88f;
        sys.apu.tone(2, 0, 0);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    if (mode_ != Mode::Run) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.tone(2, 0, 0);
    }
    draw();
}

float Game::sx(float wx, float par) const { return kCam + (wx - x_) * kPpm * par; }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8) return;
    if (cy + h * 0.5f < -8 || cy - h * 0.5f > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::lround(w), sh = std::lround(h);
    s.w = int16_t(std::clamp(sw, 1L, 400L));
    s.h = int16_t(std::clamp(sh, 1L, 400L));
    s.x = int16_t(std::lround(cx - sw * 0.5f));
    s.y = int16_t(std::lround(cy - sh * 0.5f));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sky() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 96) {
            c = gs::rgb4(4 + y / 32, 7 + y / 24, 13 - y / 28);
        } else if (y < 150) {
            int g = 9 - (y - 96) / 18;
            c = gs::rgb4(3, std::max(g, 5), 3);
        } else {
            int d = (y - 150) / 12;
            c = gs::rgb4(4 + std::min(d, 3), 4 + std::min(d, 3), 5 + std::min(d, 2));
        }
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
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

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();

    for (int i = -1; i < 6; i++) {
        float base = std::floor(x_ * 0.22f / 18.f) * 18.f;
        float wx = base + float(i) * 18.f;
        spr(art_.stand, sx(wx, 0.22f), 92.f, 40.f, PAL_STAND);
    }
    for (int i = -1; i < 10; i++) {
        float base = std::floor(x_ * 0.45f / 12.f) * 12.f;
        float wx = base + float(i) * 12.f;
        spr(art_.lamp, sx(wx, 0.45f), 118.f, 36.f, PAL_WHEEL);
    }

    float roadY = 168.f;
    for (int i = -2; i < 16; i++) {
        float base = std::floor(x_ / 4.f) * 4.f;
        float wx = base + float(i) * 4.f;
        spr(art_.paint, sx(wx), roadY, 10.f, PAL_ASPHALT);
    }
    spr(art_.paint, sx(kMark), roadY - 2.f, 22.f, PAL_MARK);
    spr(art_.cone, sx(kMark - kPaint - 0.4f), roadY - 18.f, 26.f, PAL_CONE);
    spr(art_.cone, sx(kMark + kPaint + 0.4f), roadY - 18.f, 26.f, PAL_CONE);

    float ky = roadY - 22.f;
    int wf = spin_ < 0.5f ? 0 : 1;
    spr(art_.kart, sx(x_), ky, 28.f, PAL_KART);
    spr(art_.wheel[wf], sx(x_ - 1.15f), ky + 10.f, 14.f, PAL_WHEEL);
    spr(art_.wheel[wf], sx(x_ + 1.25f), ky + 10.f, 14.f, PAL_WHEEL);
    spr(art_.driver, sx(x_ - 0.15f), ky - 10.f, 26.f, PAL_DRIVER);

    if (mode_ == Mode::Title) {
        spr(art_.banner, 160.f, 46.f, 36.f, PAL_BANNER);
        hudC(10, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(12, "RIGHT GAS   LEFT BRAKE", PAL_HUD);
        hudC(14, "STOP STILL INSIDE THE BOX", PAL_HUD);
        hudC(18, "PRESS START", PAL_HUD);
    } else {
        char line[48];
        std::snprintf(line, sizeof line, "SPD %4.1f", std::fabs(v_));
        hud(1, 1, line, PAL_HUD);
        float left = std::max(0.f, limit_ - race_);
        std::snprintf(line, sizeof line, "CLOCK %4.1f", left);
        hud(28, 1, line, left < 6.f ? PAL_HUD : PAL_HUD);
        if (mode_ == Mode::Run) {
            if (onPaint()) hudC(24, "ON THE MARK", PAL_HUD);
            else if (x_ < kMark - kPaint) hudC(24, "THE BOX IS AHEAD", PAL_HUD);
            else hudC(24, "TOO FAR", PAL_HUD);
        } else if (mode_ == Mode::Win) {
            hudC(8, "SET DOWN", PAL_HUD);
            hudC(22, "THE MARK IS YOURS", PAL_HUD);
        } else {
            hudC(8, "MISSED", PAL_HUD);
            hudC(10, why_, PAL_HUD);
        }
    }
}

}  // namespace kartmark
