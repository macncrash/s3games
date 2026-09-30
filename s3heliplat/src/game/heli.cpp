#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace heliplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kDeck = 34.f;
constexpr float kFace = 360.f;
constexpr float kEnd = 500.f;
constexpr float kMark = 428.f;
constexpr float kXTol = 9.f;
constexpr float kYTol = 2.4f;
constexpr float kPTol = 0.10f;
constexpr float kStop = 1.8f;
constexpr float kHoldNeed = 0.50f;
constexpr float kLimit = 36.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    sys.vdp.setFogColor(gs::rgb4(7, 10, 14));
    sys.apu.setMaster(0.4f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
}

void Game::begin() {
    x_ = 70.f;
    y_ = 62.f;
    vx_ = vy_ = 0;
    pitch_ = pitchV_ = 0;
    hold_ = idle_ = 0;
    time_ = 0;
    cam_ = x_;
    shake_ = 0;
    why_ = "";
    over_ = false;
    won_ = false;
    chime_ = -1;
    mode_ = Mode::Run;
    blip(220.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.11f);
    beep_ = 0.08f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    over_ = true;
    won_ = false;
    mode_ = Mode::Fail;
    shake_ = 1.f;
    vx_ *= 0.2f;
    vy_ *= 0.2f;
    sys_->apu.noiseBurst(0.34f, 700.f, 0.22f);
    sys_->apu.tone(1, 70.f, 0.22f);
    beep_ = 0.28f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    why_ = "level";
    vx_ = vy_ = 0;
    pitch_ = 0;
    chime_ = 0;
    chimeT_ = 0;
    sys_->rumble(0.18f, 0.05f, 110);
    sys_->setLight(30, 150, 70);
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win) return 4;
    if (mode_ != Mode::Run && mode_ != Mode::Pause) return 0;
    bool at = std::fabs(x_ - kMark) <= kXTol && std::fabs(y_ - kDeck) <= kYTol && std::fabs(pitch_) <= kPTol;
    if (at && hold_ > 0.12f) return 3;
    if (x_ >= kFace && x_ <= kEnd) return 2;
    return 1;
}

void Game::pilot(float& thrust, float& climb, float& lean) {
    const gs::Pad& pad = sys_->pad;
    thrust = climb = lean = 0;
    if (pad.down(gs::BTN_RIGHT)) thrust += 1.f;
    if (pad.down(gs::BTN_LEFT)) thrust -= 1.f;
    if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) climb += 1.f;
    if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B)) climb -= 1.f;
    if (pad.down(gs::BTN_X)) lean -= 1.f;
    if (pad.down(gs::BTN_Y)) lean += 1.f;
    if (std::fabs(pad.axisX) > 0.18f) thrust = pad.axisX;
    if (std::fabs(pad.axisY) > 0.18f) climb = pad.axisY;
    if (!bot_) return;

    float dx = kMark - x_;
    bool settle = std::fabs(dx) < 22.f && std::fabs(vx_) < 7.f;
    float ty = settle ? kDeck : 68.f;
    thrust = clampf(dx * 0.040f - vx_ * 0.20f, -1.f, 1.f);
    if (settle) thrust = clampf(dx * 0.09f - vx_ * 0.32f, -1.f, 1.f);
    climb = clampf((ty - y_) * 0.11f - vy_ * 0.22f, -1.f, 1.f);
    lean = clampf(-pitch_ * 6.f - pitchV_ * 0.5f, -1.f, 1.f);
}

void Game::step(float thrust, float climb, float lean) {
    time_ += kDt;
    float nose = -thrust * 0.16f + lean * 0.22f;
    pitchV_ += ((nose - pitch_) * 9.f - pitchV_ * 5.5f) * kDt;
    pitch_ += pitchV_ * kDt;
    pitch_ = clampf(pitch_, -0.7f, 0.7f);

    vx_ += thrust * 34.f * kDt;
    vy_ += climb * 30.f * kDt;
    vx_ *= std::exp(-kDt * 1.15f);
    vy_ *= std::exp(-kDt * 1.55f);
    vx_ = clampf(vx_, -28.f, 36.f);
    vy_ = clampf(vy_, -22.f, 22.f);
    x_ += vx_ * kDt;
    y_ += vy_ * kDt;

    bool atMark = std::fabs(x_ - kMark) <= kXTol;
    bool level = std::fabs(y_ - kDeck) <= kYTol && std::fabs(pitch_) <= kPTol;
    bool stopped = std::fabs(vx_) <= kStop && std::fabs(vy_) <= kStop * 0.7f;
    bool onBay = x_ >= kFace && x_ <= kEnd;
    if (onBay && atMark && level && stopped) {
        hold_ += kDt;
        idle_ = 0;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else {
        hold_ = 0;
    }

    if (x_ > kEnd + 4.f) {
        fail("missed the end");
        return;
    }
    if (y_ < kDeck - 14.f && x_ > kFace - 8.f) {
        fail("missed the end");
        return;
    }
    if (y_ < 8.f || y_ > 120.f) {
        fail("missed the end");
        return;
    }

    bool quiet = stopped && std::fabs(thrust) < 0.08f && std::fabs(climb) < 0.08f;
    if (quiet && !(onBay && atMark && level)) {
        idle_ += kDt;
        if (idle_ > 0.9f) {
            if (x_ < kFace - 1.f) fail("short of the platform");
            else if (!level) fail("not level");
            else fail("missed the end");
            return;
        }
    } else if (!quiet) {
        idle_ = 0;
    }
    if (time_ > kLimit) fail("missed the end");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float thrust = 0, climb = 0, lean = 0;
            pilot(thrust, climb, lean);
            step(thrust, climb, lean);
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            over_ = false;
        }
    }

    float want = x_ - 18.f;
    cam_ += (want - cam_) * (1.f - std::exp(-kDt * 3.2f));
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.6f);
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    rotor_ = int((time_ + 0.4f) * 24.f) % 3;
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        int stepN = int(chimeT_ / 0.15f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.15f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    }
    if (mode_ == Mode::Run) {
        bool good = std::fabs(x_ - kMark) <= kXTol && std::fabs(y_ - kDeck) <= kYTol;
        sys.setLight(good ? 30 : 20, good ? 140 : 40, good ? 70 : 90);
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::quad(float x0, float y0, float x1, float y1, const gs::Mipped& m, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float jx = (shake_ > 0.f) ? std::sin(time_ * 70.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) * 1.15f + 108.f + jx; };
    auto sy = [&](float alt) { return 178.f - alt * 1.7f; };
    float px0 = sx(x0), px1 = sx(x1), py0 = sy(y1), py1 = sy(y0);
    if (px1 < -40.f || px0 > gs::SCREEN_W + 40.f || py1 < -40.f || py0 > gs::SCREEN_H + 40.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(px1 - px0), 1L, 600L));
    s.h = int16_t(std::clamp(std::lround(py1 - py0), 1L, 400L));
    s.x = int16_t(std::lround(px0));
    s.y = int16_t(std::lround(py0));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    for (const char* p = s; *p; ++p) {
        char ch = *p;
        if (ch < 32 || ch > 126) {
            x += 6.f * scale;
            continue;
        }
        const gs::Mipped& m = art_.glyph[ch - 32];
        float h = std::max(7.f * scale, 1.f);
        spr(m, x + h * 0.45f, y, h, pal);
        x += (ch == ' ' ? 4.4f : 5.8f) * scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int(3 + 7 * (1.f - u));
        int g = int(6 + 6 * (1.f - u));
        int b = int(11 + 4 * (1.f - u));
        if (y > 186) {
            r = 2;
            g = 6;
            b = 3;
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = uint8_t(y < 18 ? 1 : 0);
        vdp.road[y].on = false;
    }

    float jx = (shake_ > 0.f) ? std::sin(time_ * 70.f) * shake_ * 3.f : 0.f;
    auto sx = [&](float x) { return (x - cam_) * 1.15f + 108.f + jx; };
    auto sy = [&](float alt) { return 178.f - alt * 1.7f; };

    if (mode_ == Mode::Title) {
        text("S3 HELIPLAT", 86, 58, 2.0f, PAL_HUD);
        text("STOP LEVEL WITH THE PLATFORM", 28, 86, 1.05f, PAL_HUD);
        text("MISS THE END AND THE LEG FAILS", 24, 104, 1.0f, PAL_HUD);
        text("ARROWS FLY   X Y LEAN   ENTER", 36, 128, 1.0f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 118, 40, 2.f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("LEVEL", 126, 28, 2.f, PAL_HUD);
        text("STOPPED ON THE PLATFORM", 48, 50, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(why_, 70, 28, 1.3f, PAL_HUD);
    } else {
        const char* tag = "APPROACH";
        if (marker() == 3) tag = "HOLD LEVEL";
        else if (marker() == 2) tag = "ON THE BAY";
        text(tag, 8, 14, 1.05f, PAL_HUD);
    }

    bool faceL = vx_ < -4.f;
    float hx = sx(x_);
    float hy = sy(y_ + 6.f);
    float tilt = pitch_ * 10.f;
    spr(art_.body, hx, hy + tilt, 34.f, PAL_HELI, faceL);
    spr(art_.rotor[rotor_], hx + (faceL ? -6.f : 6.f), hy - 16.f, 11.f, PAL_HELI, faceL);

    quad(kFace, kDeck - 6.f, kEnd, kDeck + 2.f, art_.deck, PAL_DECK);
    spr(art_.mark, sx(kMark), sy(kDeck + 1.f), 12.f, PAL_MARK);
    quad(kEnd - 8.f, kDeck - 5.f, kEnd, kDeck + 3.f, art_.deck, PAL_END);
    for (int i = 0; i < 4; i++) {
        float px = kFace + 16.f + i * 36.f;
        quad(px, 4.f, px + 8.f, kDeck - 4.f, art_.post, PAL_POST);
    }

    float base = cam_ - 80.f;
    for (int i = 0; i < 6; i++) {
        float wx = base + i * 90.f;
        quad(wx, 0.f, wx + 70.f, 16.f + (i & 1) * 6.f, art_.hill, PAL_HILL);
    }
    spr(art_.cloud, 60.f + std::sin(time_ * 0.25f) * 10.f, 36.f, 16.f, PAL_CLOUD);
    spr(art_.cloud, 230.f, 48.f, 12.f, PAL_CLOUD);
}

}  // namespace heliplat
