#include "mark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rickmark {
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr double kMark = 32.0;
constexpr double kTol = 0.48;
constexpr double kPast = 40.5;
constexpr double kStop = 0.22;
constexpr double kSitNeed = 0.70;
constexpr double kClock = 16.5;
constexpr double kStartX = 7.0;
constexpr double kPedal = 10.2;
constexpr double kBrake = 16.5;
constexpr double kDrag = 2.1;
constexpr double kMax = 12.0;
constexpr float kZoom = 9.2f;
constexpr float kCabH = 50.f;

double clampd(double v, double a, double b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (drop_ > 0.4f) return 3;
    if (onMark() && std::fabs(speed_) <= kStop) return 2;
    return 1;
}

bool Game::onMark() const { return std::fabs(x_ - kMark) <= kTol; }

void Game::begin() {
    x_ = kStartX;
    speed_ = 0;
    pedalIn_ = 0;
    brakeIn_ = 0;
    sit_ = 0;
    race_ = 0;
    clock_ = kClock;
    roll_ = 0;
    phase_ = 0;
    pedal_ = 0;
    won_ = false;
    over_ = false;
    setPulse_ = false;
    drop_ = 0;
    chimeN_ = 0;
    chimeStep_ = 0;
    why_[0] = 0;
    shake_ = 0;
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    camX_ = float(kMark - 6.0);
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    camX_ = float(x_);
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.08f, 0.14f, 0.06f);
    t_ = 0;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_RIGHT) || p.down(gs::BTN_Y);
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.down(gs::BTN_X);
    pedalIn_ = go ? 1.0 : 0.0;
    brakeIn_ = stop ? 1.0 : 0.0;
    if (p.accel > 0.05f) pedalIn_ = std::max(pedalIn_, double(p.accel));
    if (p.brake > 0.05f) brakeIn_ = std::max(brakeIn_, double(p.brake));
    setPulse_ = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
}

void Game::pilot() {
    const double err = kMark - x_;
    double want = 0;
    if (err > 12.0) want = 7.6;
    else if (err > 4.0) want = std::sqrt(err * 8.0);
    else if (err > kTol) want = 1.15;
    else if (err > 0.05) want = 0.28;
    else want = 0;
    if (err < -0.08) want = -0.45;
    const double dv = want - speed_;
    pedalIn_ = dv > 0.05 ? clampd(dv / 1.6, 0.2, 1.0) : 0;
    brakeIn_ = dv < -0.05 ? clampd(-dv / 1.1, 0.3, 1.0) : 0;
    setPulse_ = onMark() && std::fabs(speed_) < 0.06 && std::fabs(err) < kTol * 0.85;
    phase_ = onMark() ? 1 : 0;
}

void Game::trySet() {
    if (!setPulse_ || mode_ != Mode::Run) return;
    if (std::fabs(speed_) > kStop) {
        fail("set down while still rolling");
        return;
    }
    if (!onMark()) {
        fail("set down off the mark");
        return;
    }
    drop_ = 1.f;
    win();
}

void Game::physics() {
    double a = pedalIn_ * kPedal - brakeIn_ * kBrake;
    if (std::fabs(speed_) > 0.02) a -= kDrag * (speed_ > 0 ? 1.0 : -1.0);
    else if (pedalIn_ < 0.05 && brakeIn_ < 0.05) speed_ = 0;
    speed_ += a * kDt;
    speed_ = clampd(speed_, -3.2, kMax);
    x_ += speed_ * kDt;
    if (x_ < 3.2) {
        x_ = 3.2;
        if (speed_ < 0) speed_ = 0;
    }
    if (pedalIn_ > 0.2 && speed_ > 0.35) roll_ += speed_ * kDt;
    pedal_ = int(roll_ * 1.6) & 1;

    if (mode_ != Mode::Run) return;
    trySet();
    if (mode_ != Mode::Run) return;

    if (x_ > kPast) {
        fail("rolled past the mark");
        return;
    }
    const bool slow = std::fabs(speed_) <= kStop;
    if (slow && !onMark() && pedalIn_ < 0.05 && brakeIn_ < 0.2) {
        sit_ += kDt;
        if (sit_ >= kSitNeed) fail(x_ < kMark ? "stopped short of the mark" : "stopped past the mark");
    } else {
        sit_ = 0;
    }
    clock_ -= kDt;
    if (mode_ == Mode::Run && clock_ <= 0) {
        clock_ = 0;
        fail("the other crew took the mark");
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0;
    drop_ = 1.f;
    std::snprintf(why_, sizeof why_, "set down on the mark");
    chime(4);
    sys_->rumble(0.22f, 0.10f, 140);
    sys_->setLight(40, 160, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.5f, 0.22f, 160);
    sys_->setLight(160, 28, 18);
    sys_->apu.noiseBurst(0.35f, 110.f, 0.3f);
    sys_->apu.tone(0, 64.f, 0.05f);
    tone0_ = 0.35f;
}

void Game::audio() {
    if (tone0_ > 0) {
        tone0_ -= float(kDt);
        if (tone0_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (tone1_ > 0) {
        tone1_ -= float(kDt);
        if (tone1_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (chimeN_ > 0) {
        chimeT_ -= float(kDt);
        if (chimeT_ <= 0) {
            static const float notes[] = {494.f, 622.f, 740.f, 988.f};
            sys_->apu.tone(1, notes[chimeStep_ % 4], 0.06f);
            tone1_ = 0.15f;
            chimeStep_++;
            if (chimeStep_ >= chimeN_) chimeN_ = 0;
            else chimeT_ = 0.13f;
        }
    }
    if (mode_ == Mode::Run && pedalIn_ > 0.35 && speed_ > 0.4) {
        sys_->apu.tone(0, 78.f + float(speed_) * 7.f, 0.028f);
        tone0_ = 0.06f;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || bot_) startRun();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
        if (p.pressed(gs::BTN_B)) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            if (bot_) pilot();
            else controls();
            physics();
            race_ += kDt;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) showTitle();
    }
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - float(kDt) * 0.85f);
    float follow = mode_ == Mode::Title ? float(kMark) : float(x_);
    camX_ += (follow - camX_) * (mode_ == Mode::Title ? 0.05f : 0.14f);
    audio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (m.h < 1 || h < 1.f) return;
    float sc = h / float(m.h);
    float w = float(m.w) * sc;
    if (cx + w * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy + h * 0.5f < -8 || cy - h * 0.5f > gs::SCREEN_H + 8)
        return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 400L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 300L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 800L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 400L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    const float adv = 6.f * scale;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        if (c != ' ') spr(g, x + i * adv + g.w * scale * 0.5f, y, std::max(7.f, g.h * scale), pal, false, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.A.enabled = false;
    v.B.enabled = false;
    v.HUD.clear();
    v.hudEnabled = true;

    float jx = shake_ > 0 ? std::sin(float(t_) * 48.f) * shake_ * 3.f : 0.f;
    float cam = camX_ + jx / kZoom;
    auto wx = [&](double x) { return 160.f + float(x - cam) * kZoom; };

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H);
        uint16_t sky = lerpC(gs::rgb4(4, 3, 8), gs::rgb4(15, 9, 4), std::clamp((u - 0.1f) * 1.3f, 0.f, 1.f));
        if (y > 148) sky = lerpC(gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 3), (y - 148) / 76.f);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    char line[64];
    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 18.f, float(art_.title.h), PAL_BANNER, false, false);
        text("THE CLOCK IS THE OTHER CREW", 28.f, 42.f, 1.f, PAL_HUD);
        text("PEDAL  RIGHT     BRAKE  B", 46.f, 184.f, 1.f, PAL_HUD);
        text("A SETS THE SHAFTS ON THE MARK", 34.f, 200.f, 1.f, PAL_BANNER);
    } else if (mode_ == Mode::Pause) {
        spr(art_.paused, 160.f, 88.f, float(art_.paused.h), PAL_BANNER, false, false);
    } else if (mode_ == Mode::Win) {
        spr(art_.setdown, 160.f, 18.f, float(art_.setdown.h), PAL_WIN, false, false);
        text("THE OTHER CREW IS BEHIND", 48.f, 40.f, 1.f, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped* b = &art_.offmark;
        if (!std::strcmp(why_, "rolled past the mark") || !std::strcmp(why_, "stopped past the mark")) b = &art_.missed;
        else if (!std::strcmp(why_, "set down while still rolling")) b = &art_.rolling;
        else if (!std::strcmp(why_, "the other crew took the mark")) b = &art_.crew;
        spr(*b, 160.f, 18.f, float(b->h), PAL_ALERT, false, false);
    }
    if (mode_ != Mode::Title) {
        std::snprintf(line, sizeof line, "CREW %4.1f", std::max(0.0, clock_));
        text(line, 46.f, 12.f, 1.f, clock_ < 5.0 ? PAL_ALERT : PAL_HUD);
        spr(art_.clock, 22.f, 14.f, 16.f, clock_ < 5.0 ? PAL_ALERT : PAL_CREW, false, false);
        std::snprintf(line, sizeof line, "SPD %4.1f", std::fabs(speed_));
        text(line, 214.f, 12.f, 1.f, PAL_HUD);
        float frac = float(std::clamp(clock_ / kClock, 0.0, 1.0));
        sprBox(art_.stripe, 160.f, 26.f, 170.f, 4.f, PAL_HUD);
        sprBox(art_.hatch, 75.f + frac * 85.f, 26.f, std::max(2.f, frac * 170.f), 3.f, clock_ < 5.0 ? PAL_ALERT : PAL_WIN);
    }

    double crewX = kMark + 3.2 + std::max(0.0, clock_) * 0.85;
    int step = int(t_ * 6.0) & 1;
    spr(art_.walker[step], wx(crewX), 138.f, 28.f, PAL_CREW, true, false);

    bool down = mode_ == Mode::Win || drop_ > 0.5f;
    float cabY = 152.f + (down ? 3.f : 0.f);
    if (!down && std::fabs(float(speed_)) > 0.45f) cabY += std::sin(float(t_) * 10.f) * 1.1f;
    const gs::Mipped& cabSpr = down ? art_.cabDown : art_.cabUp[pedal_];
    spr(art_.shade, wx(x_) + 4.f, cabY + 18.f, 12.f, PAL_CAB, false, true);
    spr(cabSpr, wx(x_), cabY, kCabH, PAL_CAB, false, false);

    int markPal = onMark() && std::fabs(speed_) <= kStop * 2 ? PAL_WIN : PAL_MARK;
    spr(art_.chalk, wx(kMark), 176.f, 22.f, markPal, false, false);
    spr(art_.flag, wx(kMark), 128.f, 36.f, markPal, false, false);
    sprBox(art_.stripe, wx(kPast), 168.f, 3.f, 40.f, PAL_ALERT);

    sprBox(art_.stripe, 160.f, 168.f, 360.f, 70.f, PAL_STREET);
    for (int i = 0; i < 14; i++) sprBox(art_.hatch, wx(i * 6.5), 198.f, 16.f, 3.f, PAL_HUD);

    for (int i = -1; i < 9; i++) {
        double bx = i * 12.5;
        spr(art_.stall, wx(bx), 74.f, 64.f, PAL_STREET, (i & 1) != 0, false);
        spr(art_.awning, wx(bx), 48.f, 16.f, PAL_MARK, false, false);
        if ((i % 3) == 0) spr(art_.lamp, wx(bx + 5.0), 92.f, 32.f, PAL_LAMP, false, false);
    }
}

}  // namespace rickmark
