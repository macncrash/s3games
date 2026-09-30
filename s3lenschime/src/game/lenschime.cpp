#include "game/lenschime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lenschime {
namespace {
constexpr float kPi = 3.14159265f;
constexpr float kFWin = 0.06f;
constexpr float kXWin = 12.f;
constexpr float kFBad = 0.16f;
constexpr float kXBad = 26.f;
}  // namespace

bool Game::audit() const {
    auto secAt = [](int frames) { return (kHourSec - kLeadSec) + frames / kFpc; };
    if (kLeadSec < 12 || kGraceSec < 8 || kFpc < 2 || kPlates < 2) return false;
    if (secAt(0) >= kHourSec) return false;
    if (secAt(kLeadSec * kFpc) != kHourSec) return false;
    if (secAt(kLeadSec * kFpc + (kGraceSec - 1) * kFpc) != kHourSec + kGraceSec - 1) return false;
    if (secAt((kLeadSec + kGraceSec) * kFpc) < kHourSec + kGraceSec) return false;
    return true;
}

int Game::clockSec() const { return (kHourSec - kLeadSec) + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    face(h, m, s);
    return h;
}
int Game::minute() const {
    int h, m, s;
    face(h, m, s);
    return m;
}
int Game::second() const {
    int h, m, s;
    face(h, m, s);
    return s;
}

float Game::wantF() const { return 0.62f + 0.18f * std::sin(playFrames_ * 0.045f); }
float Game::wantX() const { return 22.f * std::sin(playFrames_ * 0.031f); }

bool Game::sharp() const {
    return std::fabs(focus_ - wantF()) < kFWin && std::fabs(pan_ - wantX()) < kXWin;
}

void Game::begin() {
    plates_ = kPlates;
    cracked_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    bad_ = 0;
    strikes_ = 0;
    focus_ = 0.35f;
    pan_ = -20.f;
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    if (sys_) sys_->apu.silence();
}

void Game::fail(const char* why) {
    reason_ = why;
    won_ = false;
    mode_ = Mode::Fail;
    hold_ = 0;
    if (sys_) sys_->apu.tone(0, 90.f, 0.08f);
}

void Game::crack() {
    cracked_++;
    plates_--;
    bad_ = 0;
    hold_ = 0;
    mode_ = Mode::Crack;
    if (sys_) {
        sys_->apu.tone(0, 140.f, 0.08f);
        sys_->apu.noiseBurst(0.16f, 900.f, 0.05f);
    }
    if (plates_ <= 0) {
        reason_ = "PLATES";
        won_ = false;
        mode_ = Mode::Fail;
    }
}

void Game::chime() {
    won_ = true;
    reason_ = "CHIME";
    hold_ = 0;
    strikes_ = 0;
    mode_ = Mode::Chime;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->rumble(0.2f, 0.4f, 80);
    }
}

void Game::leave() {
    mode_ = Mode::Leave;
    hold_ = 0;
    if (sys_) sys_->apu.tone(1, 784.f, 0.08f);
}

void Game::botAim() {
    float df = wantF() - focus_;
    focus_ = std::clamp(focus_ + std::clamp(df, -0.035f, 0.035f), 0.f, 1.f);
    float dx = wantX() - pan_;
    pan_ = std::clamp(pan_ + std::clamp(dx, -2.6f, 2.6f), -80.f, 80.f);
    if (onHour() && sharp()) {
        if (++hold_ > 8) chime();
    } else {
        hold_ = 0;
    }
}

void Game::human(const gs::Pad& pad) {
    if (pad.down(gs::BTN_LEFT)) pan_ -= 1.8f;
    if (pad.down(gs::BTN_RIGHT)) pan_ += 1.8f;
    if (pad.down(gs::BTN_DOWN)) focus_ -= 0.008f;
    if (pad.down(gs::BTN_UP)) focus_ += 0.008f;
    if (std::fabs(pad.axisX) > 0.2f) pan_ += pad.axisX * 2.2f;
    if (std::fabs(pad.axisY) > 0.2f) focus_ += pad.axisY * 0.01f;
    pan_ = std::clamp(pan_, -80.f, 80.f);
    focus_ = std::clamp(focus_, 0.f, 1.f);
    if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
        if (!onHour()) fail("EARLY");
        else if (!sharp()) fail("SOFT");
        else chime();
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
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::ray(float cx, float cy, float ang, float len, int pal) {
    int n = std::max(1, int(len / 5.f));
    for (int i = 1; i <= n; i++) {
        float t = len * (i / float(n));
        spr(art_.dot, cx + std::sin(ang) * t, cy - std::cos(ang) * t, 4.f, pal);
    }
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

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::sky() {
    bool gold = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Fail && won_);
    bool bad = mode_ == Mode::Fail && !won_;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r = 2 + y / 40;
        int g = 3 + y / 48;
        int b = 8 - y / 40;
        if (y > 150) {
            r = 3;
            g = 4;
            b = 3;
        }
        if (gold) {
            r += 4;
            g += 2;
        }
        if (bad) r += 3;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    sky();

    float ferr = std::fabs(focus_ - wantF());
    float xerr = pan_ - wantX();
    int fog = (mode_ == Mode::Chime || mode_ == Mode::Leave) ? 0 : int(std::clamp(ferr * 80.f, 0.f, 14.f));
    float tx = 160.f - xerr;
    float swing = 0.f;
    if (mode_ == Mode::Chime || mode_ == Mode::Leave)
        swing = std::sin(hold_ * 0.45f) * 10.f * std::exp(-hold_ * 0.02f);

    spr(art_.beam, tx, 28, 14, PAL_STONE);
    spr(art_.bell, tx + swing, 46, 28, PAL_BELL);
    spr(art_.clapper, tx + swing * 1.4f, 54, 10, PAL_BELL);
    spr(art_.tower, tx, 128, 110, PAL_STONE, false, fog / 3);
    spr(art_.face, tx, 92, 46, PAL_CLOCK, false, fog);

    int h, m, s;
    face(h, m, s);
    if (mode_ == Mode::Title) {
        h = 11;
        m = 59;
        s = 38;
    }
    ray(tx, 92, (h % 12) * kPi / 6.f + (m / 60.f) * (kPi / 6.f), 10.f, PAL_BAD);
    ray(tx, 92, m * kPi / 30.f, 16.f, PAL_STONE);
    ray(tx, 92, s * kPi / 30.f, 18.f, PAL_OK);

    const float fx0 = 108, fy0 = 58, fx1 = 212, fy1 = 128;
    spr(art_.corner, fx0, fy0, 12, PAL_GLASS);
    spr(art_.corner, fx1, fy0, 12, PAL_GLASS, true);
    spr(art_.corner, fx0, fy1, 12, PAL_GLASS);
    spr(art_.corner, fx1, fy1, 12, PAL_GLASS, true);

    float split = (focus_ - wantF()) * 80.f;
    spr(art_.caret, 160 - 8 - split, 48, 12, PAL_OK);
    spr(art_.caret, 160 + 8 + split, 48, 12, PAL_BAD);

    spr(art_.body, 150, 198, 40, PAL_BRASS);
    spr(art_.barrel, 214, 190, 26, PAL_BRASS);
    spr(art_.glass, 214, 190, 14, PAL_GLASS, false, fog / 2);

    for (int i = 0; i < plates_; i++) spr(art_.plate, 28.f + i * 18.f, 200.f, 12, PAL_CLOCK);

    char buf[48];
    if (mode_ != Mode::Title) {
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hud(1, 1, buf, onHour() || mode_ == Mode::Chime || mode_ == Mode::Leave ? PAL_OK : PAL_HUD);
        std::snprintf(buf, sizeof buf, "PLATES %d", plates_);
        hud(30, 1, buf, plates_ < kPlates ? PAL_BAD : PAL_HUD);
    }

    if (mode_ == Mode::Title) {
        hudC(8, "S3 LENSCHIME", PAL_HUD);
        hudC(11, "PLAY THE LENS", PAL_HUD);
        hudC(13, "LEAVE WHEN THE HOUR CHIMES", PAL_OK);
        hudC(16, "ARROWS  FOCUS AND PAN", PAL_HUD);
        if ((t_ & 16) == 0) hudC(22, "PRESS START", PAL_OK);
    } else if (mode_ == Mode::Play) {
        if (onHour() && sharp()) hudC(25, "THE HOUR  LEAVE", PAL_OK);
        else if (onHour()) hudC(25, "HOLD THE LENS", PAL_BAD);
        else if (sharp()) hudC(25, "SHARP  WAIT FOR NOON", PAL_OK);
        else hudC(25, "RACK THE LENS", PAL_HUD);
        hudC(26, "START LEAVES", PAL_HUD);
    } else if (mode_ == Mode::Crack) {
        hudC(24, "PLATE CRACKED", PAL_BAD);
    } else if (mode_ == Mode::Chime) {
        hudC(24, "THE HOUR CHIMES", PAL_OK);
    } else if (mode_ == Mode::Leave) {
        hudC(24, "LEFT ON THE HOUR", PAL_OK);
    } else if (mode_ == Mode::Fail) {
        hudC(23, reason_, PAL_BAD);
        if (!bot_ && (hold_ & 16) == 0) hudC(25, "START RETRIES", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.apu.setMaster(0.7f);
    mode_ = Mode::Title;
    t_ = 0;
    over_ = false;
    won_ = false;
    plates_ = kPlates;
    focus_ = 0.35f;
    pan_ = -20.f;
    reason_ = "";
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_++;

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_ && t_ > 10) go = true;
        if (go) {
            if (!rules_) fail("RULES");
            else begin();
        }
    } else if (mode_ == Mode::Play) {
        playFrames_++;
        if (pastHour()) {
            fail("HOUR");
        } else if (bot_) {
            botAim();
        } else {
            human(pad);
        }
        if (mode_ == Mode::Play) {
            bool bad = std::fabs(focus_ - wantF()) > kFBad || std::fabs(pan_ - wantX()) > kXBad;
            if (bad) {
                if (++bad_ > 48) crack();
            } else {
                bad_ = 0;
            }
        }
    } else if (mode_ == Mode::Crack) {
        playFrames_++;
        if (pastHour()) fail("HOUR");
        else if (++hold_ > 28) mode_ = Mode::Play;
    } else if (mode_ == Mode::Chime) {
        if (hold_ % 7 == 0 && strikes_ < 4) {
            sys.apu.tone(0, (strikes_ % 2) ? 659.f : 523.f, 0.1f);
            strikes_++;
        }
        if (++hold_ > 28) leave();
    } else if (mode_ == Mode::Leave) {
        pan_ += 1.4f;
        if (++hold_ > 24) {
            over_ = true;
            won_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        if (bot_ && ++hold_ > 6) over_ = true;
        else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }

    draw();
}

}  // namespace lenschime
