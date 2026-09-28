#include "game/plow.h"
#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace plow {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPpm = 9.2f;
constexpr float kDrive = 5.6f;
constexpr float kBrake = 8.4f;
constexpr float kDrag = 0.72f;
constexpr float kStop = 0.16f;
constexpr float kMark = 40.f;
constexpr float kBand = 0.34f;
constexpr float kPaint = 1.15f;
constexpr float kHoldNeed = 0.8f;
constexpr float kSitNeed = 0.42f;
constexpr float kEnd = 54.f;
constexpr float kClock = 34.f;
constexpr float kGround = 168.f;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.45f);
    sys.apu.setEcho(0.08f, 0.18f, 0.08f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    x_ = 0;
    v_ = 0;
    blade_ = 0;
    if (bot_) beginRun();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08f) return 3;
    if (std::fabs(x_ - kMark) <= kPaint) return 2;
    return 1;
}

void Game::beginRun() {
    x_ = 3.2f;
    v_ = 0.f;
    blade_ = 0.f;
    hold_ = 0;
    sit_ = 0;
    race_ = 0;
    spray_ = 0;
    won_ = false;
    over_ = false;
    why_ = nullptr;
    wasDown_ = false;
    mode_ = Mode::Run;
    blip(180.f);
}

void Game::blip(float freq) {
    tone_ = freq;
    toneT_ = 0.12f;
}

void Game::bot(bool& drive, bool& brake, bool& down) const {
    float err = kMark - x_;
    if (x_ > kEnd - 2.f) {
        brake = true;
        return;
    }
    if (blade_ > 0.15f) {
        down = true;
        if (v_ > 0.04f) brake = true;
        else if (v_ < -0.04f) drive = true;
        else if (err > 0.08f) drive = true;
        else if (err < -0.08f) brake = true;
        return;
    }
    float want;
    if (err > 16.f) want = 6.4f;
    else if (err > 8.f) want = 3.1f;
    else if (err > 3.f) want = 1.25f;
    else if (err > 0.7f) want = 0.42f;
    else want = clampf(err * 1.15f, -0.25f, 0.35f);
    if (v_ < want - 0.07f) drive = true;
    else if (v_ > want + 0.05f) brake = true;
    if (std::fabs(err) < 0.2f && std::fabs(v_) < 0.14f) down = true;
}

void Game::win() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    hold_ = kHoldNeed;
    blade_ = 1.f;
    blip(620.f);
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    blip(80.f);
}

void Game::update(float dt, bool drive, bool brake, bool down) {
    float plowDrag = blade_ > 0.45f ? blade_ * 4.2f * v_ : 0.f;
    float a = (drive ? kDrive : 0.f) - (brake ? kBrake : 0.f) - kDrag * v_ - plowDrag;
    if (!drive && !brake && std::fabs(v_) > 0.02f) a -= (v_ > 0.f ? 0.55f : -0.55f);
    v_ += a * dt;
    v_ = clampf(v_, -2.2f, 8.2f);
    x_ += v_ * dt;
    if (x_ < 0.f) {
        x_ = 0.f;
        if (v_ < 0.f) v_ = 0.f;
    }

    float target = down ? 1.f : 0.f;
    float rate = down ? 1.45f : 2.2f;
    float step = clampf(target - blade_, -rate * dt, rate * dt);
    blade_ += step;
    if (down && !wasDown_) blip(140.f);
    wasDown_ = down;
    if (blade_ > 0.72f && std::fabs(v_) > 0.35f) spray_ = 0.28f;
    if (spray_ > 0.f) spray_ -= dt;

    race_ += dt;
    bool seated = blade_ >= 0.96f && std::fabs(x_ - kMark) <= kBand && std::fabs(v_) <= kStop;
    if (seated) hold_ += dt;
    else hold_ = 0.f;
    if (hold_ >= kHoldNeed) {
        win();
        return;
    }
    if (blade_ >= 0.96f && std::fabs(v_) <= kStop && std::fabs(x_ - kMark) > kBand) sit_ += dt;
    else sit_ = 0.f;
    if (sit_ >= kSitNeed) {
        fail("OFF THE MARK");
        return;
    }
    if (x_ > kEnd) {
        fail("PAST THE YARD");
        return;
    }
    if (race_ >= kClock) fail("THE OTHER CREW TOOK THE MARK");
}

void Game::audio() {
    if (toneT_ > 0.f) {
        sys_->apu.tone(0, tone_, 0.12f);
        toneT_ -= kDt;
    } else if (mode_ == Mode::Win) {
        float notes[] = {392.f, 494.f, 587.f, 784.f};
        int step = int(t_ * 5.f) % 4;
        sys_->apu.tone(0, notes[step], 0.07f);
    } else if (mode_ == Mode::Run) {
        float hum = 70.f + std::fabs(v_) * 18.f + blade_ * 12.f;
        sys_->apu.tone(0, hum, 0.04f + std::fabs(v_) * 0.008f);
    } else {
        sys_->apu.tone(0, 0, 0);
    }
    float grit = (mode_ == Mode::Run && blade_ > 0.5f) ? std::fabs(v_) * 0.04f : 0.f;
    sys_->apu.noise(grit, 0.55f, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    tick_++;
    t_ += kDt;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE) && sys.hasHome()) sys.eject();
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) beginRun();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
        } else {
            bool drive = false, brake = false, down = false;
            if (bot_) bot(drive, brake, down);
            else {
                drive = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C);
                brake = pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_X) || pad.down(gs::BTN_B);
                down = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A);
                if (drive && brake) brake = false;
            }
            update(kDt, drive, brake, down);
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        v_ *= 0.9f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }
    audio();
    draw();
}

float Game::sx(float wx, float par) const { return 176.f + (wx - x_) * kPpm * par; }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -12 || cx - w * 0.5f > gs::SCREEN_W + 12) return;
    if (cy + h * 0.5f < -12 || cy - h * 0.5f > gs::SCREEN_H + 12) return;
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
            int t = y / 12;
            c = gs::rgb4(6 + t / 3, 8 + t / 4, 12);
        } else if (y < 132) {
            c = gs::rgb4(11, 12, 13);
        } else if (y < 150) {
            c = gs::rgb4(12, 13, 14);
        } else if (y < 186) {
            c = gs::rgb4(4, 4, 5);
        } else {
            c = gs::rgb4(13, 14, 15);
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

    int bi = std::clamp(int(std::lround(blade_ * 4.f)), 0, 4);
    float bob = std::sin(t_ * 14.f) * std::min(std::fabs(v_) * 0.6f, 1.4f);
    spr(art_.blade[bi], sx(x_ + 0.35f), kGround - 28.f + blade_ * 10.f + bob, 42.f, PAL_TRUCK);
    spr(art_.truck, sx(x_ - 2.55f), kGround - 22.f + bob * 0.4f, 44.f, PAL_TRUCK);
    if (spray_ > 0.02f) {
        spr(art_.spray, sx(x_ + 1.3f), kGround - 8.f, 16.f + spray_ * 18.f, PAL_SNOW);
        spr(art_.spray, sx(x_ + 2.1f), kGround - 16.f, 12.f, PAL_FLAKE);
    }

    float pcx = sx(kMark);
    float ph = (kPaint * 2.f * kPpm) * float(art_.paint.h) / float(art_.paint.w);
    spr(art_.paint, pcx, kGround + 6.f, std::max(ph, 12.f), PAL_MARK);
    spr(art_.stake, sx(kMark), kGround - 22.f, 40.f, PAL_MARK);
    spr(art_.word, sx(kMark), kGround - 48.f, 14.f, PAL_MARK);
    spr(art_.stake, sx(kEnd), kGround - 16.f, 30.f, PAL_BARN);

    for (int i = -1; i < 8; i++) {
        float base = std::floor(x_ / 6.f) * 6.f;
        float wx = base + float(i) * 6.f;
        spr(art_.fence, sx(wx, 0.92f), kGround - 8.f, 22.f, PAL_BARN);
    }
    for (int i = -2; i < 7; i++) {
        float base = std::floor(x_ * 0.4f / 18.f) * 18.f;
        float wx = base + float(i) * 18.f;
        spr(art_.pine, sx(wx, 0.42f), 108.f, 58.f, PAL_PINE);
    }
    spr(art_.barn, sx(18.f, 0.28f), 100.f, 46.f, PAL_BARN);

    for (int i = 0; i < 18; i++) {
        float fx = std::fmod(float(i * 47) + t_ * (18.f + float(i % 5) * 6.f), float(gs::SCREEN_W + 8));
        float fy = std::fmod(float(i * 29) + t_ * (22.f + float(i % 3) * 9.f), 150.f);
        spr(art_.flake, fx, fy, (i & 1) ? 5.f : 3.f, PAL_FLAKE);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 PLOWMARK", PAL_HUD);
        hudC(6, "SET THE BLADE DOWN ON THE MARK", 2);
        hudC(8, "CLOSE TO THE MARK IS NOT ON IT", 4);
        hudC(15, "RIGHT OR C ROLLS THE PLOW", 5);
        hudC(16, "LEFT OR X CHECKS IT", 5);
        hudC(17, "DOWN OR A SETS THE BLADE", 5);
        hudC(21, "START TO ROLL", 3);
        hud(1, 26, S3_VERSION_STRING, 6);
    } else {
        float left = std::max(0.f, kClock - race_);
        std::snprintf(buf, sizeof buf, "CREW %4.1f", left);
        hud(1, 0, buf, left < 8.f ? 4 : PAL_HUD);
        std::snprintf(buf, sizeof buf, "BLADE %s", blade_ > 0.9f ? "DOWN" : blade_ > 0.2f ? "SET " : "UP  ");
        hud(14, 0, buf, blade_ > 0.9f ? 3 : 2);
        float gap = x_ - kMark;
        std::snprintf(buf, sizeof buf, "MARK %+5.2f", -gap);
        hud(27, 0, buf, std::fabs(gap) <= kBand ? 3 : PAL_HUD);
        if (mode_ == Mode::Run) {
            if (hold_ > 0.05f) hudC(25, "HOLD THE BLADE ON THE MARK", 3);
            else if (std::fabs(gap) <= kPaint && blade_ < 0.4f) hudC(25, "THE PAINT IS UNDER THE BLADE", 2);
            else if (blade_ > 0.5f && std::fabs(v_) > 0.8f) hudC(25, "THE BLADE IS BITING", 5);
            else hudC(25, "SET DOWN ON THE MARK", PAL_HUD);
            hudC(26, "STOP OFF THE PAINT AND THE JOB IS LOST", 4);
        } else if (mode_ == Mode::Win) {
            hudC(23, "SET DOWN ON THE MARK", 3);
            hudC(25, "BEFORE THE OTHER CREW", 2);
        } else {
            hudC(23, why_ ? why_ : "MISSED THE MARK", 4);
            hudC(25, "THE JOB IS LOST", 4);
            hudC(26, "START TO ROLL AGAIN", PAL_HUD);
        }
    }
}

}  // namespace plow
