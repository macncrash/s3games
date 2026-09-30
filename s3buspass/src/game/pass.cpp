#include "game/pass.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace buspass {
namespace {

constexpr float BUS_L = 92.f;
constexpr float EXIT_X = 1380.f;
constexpr float CREW_SECONDS = 34.f;
constexpr float GATE_X[3] = {380.f, 760.f, 1120.f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    busX_ = 40.f;
    offset_ = 0;
    crewLeft_ = CREW_SECONDS;
    for (float& g : gateOpen_) g = 0;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    why_ = "";
    time_ = 0;
    crewLeft_ = CREW_SECONDS;
    busX_ = 36.f;
    offset_ = 0;
    speed_ = 0;
    cam_ = 0;
    wheel_ = 0;
    shake_ = 0;
    beep_ = 0;
    for (float& g : gateOpen_) g = 0;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
}

float Game::roadY(float x) const {
    return 150.f + std::sin(x * 0.0075f) * 28.f + std::sin(x * 0.019f) * 8.f;
}

float Game::halfAt(float x) const {
    float hw = 26.f;
    for (float gx : GATE_X) {
        float d = std::fabs(x - gx);
        if (d < 70.f) hw = std::min(hw, 16.f + d * 0.08f);
    }
    return hw;
}

void Game::failRun(const char* why) {
    if (mode_ != Mode::Run) return;
    why_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    speed_ = 0;
    shake_ = 14;
    sys_->apu.noiseBurst(0.5f, 520.f, 0.32f);
    sys_->apu.tone(0, 0, 0);
}

void Game::succeed() {
    if (mode_ != Mode::Run) return;
    why_ = "cleared";
    won_ = true;
    over_ = true;
    mode_ = Mode::Win;
    speed_ = 0;
    sys_->apu.tone(1, 523.f, 0.08f);
    beep_ = 28;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (won_ || mode_ == Mode::Win) return 4;
    if (busX_ > 1000.f) return 3;
    for (int i = 0; i < 3; i++) {
        if (std::fabs(busX_ - GATE_X[i]) < 90.f && gateOpen_[i] < 0.95f) return 2;
    }
    return 1;
}

void Game::blip(float freq, float vol) {
    if (beep_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    beep_ = 5;
}

void Game::pilot(float& accel, float& steer) {
    accel = 0;
    steer = 0;
    float wind = std::sin(time_ * 1.7f) * 0.28f + (time_ / CREW_SECONDS) * 0.42f;
    if (bot_) {
        int gate = -1;
        for (int i = 0; i < 3; i++) {
            if (gateOpen_[i] < 0.82f && busX_ + BUS_L < GATE_X[i] + 8.f) {
                gate = i;
                break;
            }
        }
        if (gate >= 0) {
            float dist = GATE_X[gate] - (busX_ + BUS_L);
            if (dist < 96.f) {
                if (dist > 36.f) {
                    if (speed_ > 0.85f) accel = -1.f;
                    else if (speed_ < 0.4f) accel = 0.45f;
                } else if (speed_ > 0.32f) {
                    accel = -1.f;
                }
            } else if (speed_ < 2.35f) {
                accel = 1.f;
            }
        } else if (speed_ < 2.35f) {
            accel = 1.f;
        }
        float aim = -wind * 1.15f;
        float err = offset_ - aim;
        if (err > 1.2f) steer = -1.f;
        else if (err < -1.2f) steer = 1.f;
    } else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.axisX > 0.35f || p.accel > 0.2f) accel += 1.f;
        if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.axisX < -0.35f || p.brake > 0.2f) accel -= 1.f;
        if (p.down(gs::BTN_UP) || p.axisY > 0.35f) steer -= 1.f;
        if (p.down(gs::BTN_DOWN) || p.axisY < -0.35f) steer += 1.f;
    }
    speed_ += accel * 0.07f;
    if (accel == 0.f) speed_ *= 0.94f;
    speed_ = clampf(speed_, -1.2f, 2.55f);
    if (std::fabs(speed_) < 0.03f) speed_ = 0;
    offset_ += wind;
    offset_ += steer * 0.95f;
    offset_ *= 0.992f;
}

void Game::logic() {
    time_ += 1.f / 60.f;
    crewLeft_ = std::max(0.f, CREW_SECONDS - time_);
    if (crewLeft_ <= 0.f) {
        failRun("the other crew closed the pass");
        return;
    }
    float accel = 0, steer = 0;
    pilot(accel, steer);
    busX_ += speed_;
    if (busX_ < 8.f) {
        busX_ = 8.f;
        speed_ = std::max(0.f, speed_);
    }
    wheel_ = (wheel_ + int(std::fabs(speed_) * 3.f)) & 1023;

    float mid = busX_ + BUS_L * 0.45f;
    if (std::fabs(offset_) > halfAt(mid)) {
        failRun("left the road in the pass");
        return;
    }

    for (int i = 0; i < 3; i++) {
        float dist = GATE_X[i] - (busX_ + BUS_L);
        if (dist < 78.f && dist > -6.f && speed_ < 0.5f && speed_ >= 0.f) gateOpen_[i] = std::min(1.f, gateOpen_[i] + 0.028f);
        bool across = busX_ + BUS_L > GATE_X[i] && busX_ < GATE_X[i] + 12.f;
        if (across && gateOpen_[i] < 0.72f) {
            failRun("hit the snow gate");
            return;
        }
    }

    if (busX_ > EXIT_X) {
        succeed();
        return;
    }
    if (mode_ == Mode::Run && std::fabs(speed_) > 0.5f && (wheel_ % 22) < 3) blip(80.f + speed_ * 16.f, 0.03f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    float storm = clampf(time_ / CREW_SECONDS, 0.f, 1.f);
    if (cx < 40.f + storm * 90.f) s.fog = uint8_t(std::min(12, int(storm * 10.f)));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    if (!s) return;
    const float adv = 6.f * scale;
    float w = float(std::strlen(s)) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c >= 96) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        if (g.h < 1) continue;
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    float shake = shake_ > 0 ? ((int(shake_) & 1) ? 2.f : -2.f) : 0.f;
    cam_ = busX_ - 64.f;
    if (cam_ < 0) cam_ = 0;
    if (cam_ > EXIT_X - 200.f) cam_ = EXIT_X - 200.f;
    auto sx = [&](float wx) { return wx - cam_ + shake; };

    float storm = clampf(time_ / CREW_SECONDS, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = 8 - int(storm * 5.f) - y / 80;
        if (sky < 1) sky = 1;
        if (y < 90) v.lineBackdrop[y] = gs::rgb4(sky + 1, sky + 2, sky + 5);
        else if (y < 150) v.lineBackdrop[y] = gs::rgb4(sky, sky + 1, sky + 2);
        else v.lineBackdrop[y] = gs::rgb4(7 + int((1.f - storm) * 4), 8, 9);
        v.lineFog[y] = uint8_t(y < 40 ? int(storm * 8.f) : 0);
        v.road[y].on = false;
    }

    const char* label = "HOLD THE ROAD";
    for (int i = 0; i < 3; i++) {
        float dist = GATE_X[i] - (busX_ + BUS_L);
        if (dist < 160.f && dist > -20.f && gateOpen_[i] < 0.9f) label = "SLOW FOR THE SNOW GATE";
    }
    if (busX_ > 1180.f) label = "CLEAR THE PASS";

    char crew[48];
    std::snprintf(crew, sizeof(crew), "OTHER CREW  %.1f", crewLeft_);

    if (mode_ == Mode::Title) {
        text("S3 BUS PASS", 160, 36, 3.f, PAL_HUD, 0);
        text("CLEAR THE PASS", 160, 74, 2.f, PAL_HUD, 0);
        text("BEFORE THE STORM CLOCK", 160, 100, 1.6f, PAL_HUD, 0);
        text("THE CLOCK IS THE OTHER CREW", 160, 124, 1.4f, PAL_HUD, 0);
        text("A GO   B BRAKE   UP DOWN HOLD", 160, 156, 1.3f, PAL_HUD, 0);
        text("PRESS START", 160, 186, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 96, 3.f, PAL_HUD, 0);
        text(label, 160, 132, 1.5f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fail) {
        text("PASS CLOSED", 160, 44, 2.4f, PAL_ALERT, 0);
        text(why_, 160, 84, 1.4f, PAL_HUD, 0);
        text("PRESS START", 160, 128, 2.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Win) {
        text("PASS CLEARED", 160, 40, 2.4f, PAL_HUD, 0);
        text("AHEAD OF THE STORM", 160, 76, 1.5f, PAL_HUD, 0);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "CREW STILL HAD %.1f S", crewLeft_);
        text(buf, 160, 110, 1.5f, PAL_HUD, 0);
    } else {
        text(label, 8, 10, 1.4f, PAL_HUD, -1);
        text(crew, 312, 10, 1.3f, PAL_CREW, 1);
        text("STAY ON THE SHELF", 8, 206, 1.3f, PAL_HUD, -1);
        float mark = 36.f + (1.f - crewLeft_ / CREW_SECONDS) * 248.f;
        spr(art_.crew, mark, 34, 14, PAL_CREW, false);
    }

    float rivalX = 48.f + (time_ / CREW_SECONDS) * (EXIT_X - 48.f);
    spr(art_.crew, sx(rivalX), roadY(rivalX) - 18.f, 22, PAL_CREW, true);

    float feet = roadY(busX_ + 20.f) + offset_;
    spr(art_.bus, sx(busX_ + BUS_L * 0.5f), feet, 48, PAL_BUS, true);

    for (int i = 0; i < 3; i++) {
        float lift = gateOpen_[i] * 70.f;
        float gy = roadY(GATE_X[i]) - 36.f - lift;
        int pal = (mode_ == Mode::Fail && why_ && std::strcmp(why_, "hit the snow gate") == 0) ? PAL_ALERT : PAL_SNOW;
        spr(art_.gate, sx(GATE_X[i]), gy, 64, pal, false);
    }

    for (float wx = std::floor(cam_ / 36.f) * 36.f; wx < cam_ + 400.f; wx += 36.f) {
        spr(art_.road, sx(wx), roadY(wx) + 4.f, 14, PAL_ROAD, false);
        if (int(wx) % 108 == 0) spr(art_.pine, sx(wx - 18.f), roadY(wx) - 8.f, 36, PAL_PINE, true);
        if (int(wx) % 144 == 36) spr(art_.rock, sx(wx + 16.f), roadY(wx) + 16.f, 20, PAL_ROCK, true);
    }
    for (int i = 0; i < 4; i++) {
        float px = 180.f + i * 340.f;
        spr(art_.peak, sx(px), 78.f - i * 4.f, 40, PAL_ROCK, false);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_--;

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || (mode_ != Mode::Run && p.pressed(gs::BTN_A));
    if (mode_ == Mode::Title) {
        if (start) begin();
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && start) begin();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else logic();
    }
    draw();
}

}  // namespace buspass
