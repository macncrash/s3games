#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace slip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kBusLen = 64.f;
constexpr float kJoint = 200.f;
constexpr float kBumper = 328.f;
constexpr float kBerthLo = 228.f;
constexpr float kBerthHi = 256.f;
constexpr float kSlack = 4.f;
constexpr float kTurn = 26.f;
constexpr float kFailAt = 28.f;

float deckDelta(float t) {
    if (t < kSlack) return 22.f * (1.f - t / kSlack);
    if (t < kTurn) return 0.f;
    return (t - kTurn) * 10.f;
}

}  // namespace

void Game::toTitle() {
    if (sys_) sys_->apu.silence();
    x_ = 24.f;
    vel_ = 0.f;
    playT_ = 0.f;
    hold_ = 0.f;
    onSlip_ = false;
    tide_ = deckDelta(0);
    won_ = false;
    over_ = false;
    why_ = "";
    shake_ = 0.f;
    blip_ = 0.f;
    mode_ = Mode::Title;
}

void Game::begin() {
    x_ = 24.f;
    vel_ = 0.f;
    playT_ = 0.f;
    hold_ = 0.f;
    onSlip_ = false;
    won_ = false;
    over_ = false;
    why_ = "";
    shake_ = 0.f;
    mode_ = Mode::Play;
    if (sys_) sys_->apu.noise(0.04f, 900.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    t_ = 0.f;
    toTitle();
    if (bot_) begin();
}

void Game::pilot(float& thrust) const {
    const float target = 240.f;
    if (tide_ > 2.f && x_ < 150.f) {
        thrust = vel_ > 1.f ? -80.f : 0.f;
        return;
    }
    if (x_ < target - 22.f) {
        thrust = vel_ < 34.f ? 78.f : 0.f;
        return;
    }
    if (x_ < target - 4.f) {
        thrust = vel_ > 10.f ? -110.f : 16.f;
        return;
    }
    thrust = vel_ > 1.2f ? -130.f : -20.f;
}

void Game::controls(float& thrust) {
    if (bot_) {
        pilot(thrust);
        return;
    }
    const gs::Pad& p = sys_->pad;
    float ax = p.axisX;
    if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A)) thrust += 80.f;
    if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B)) thrust -= 110.f;
    if (ax > 0.2f) thrust += 80.f * ax;
    if (ax < -0.2f) thrust += 110.f * ax;
    thrust += p.accel * 70.f;
    thrust -= p.brake * 120.f;
}

void Game::step() {
    float thrust = 0.f;
    controls(thrust);
    vel_ += thrust * kDt;
    if (std::fabs(thrust) < 1.f) vel_ *= 0.975f;
    vel_ = std::clamp(vel_, -18.f, 52.f);
    x_ += vel_ * kDt;
    wheel_ += std::fabs(vel_) * kDt * 0.35f;
    tide_ = deckDelta(playT_);

    float nose = x_ + kBusLen;
    if (!onSlip_ && nose >= kJoint) {
        if (tide_ > 8.f) {
            x_ = kJoint - kBusLen - 1.f;
            vel_ = -6.f;
            shake_ = 0.6f;
            why_ = "the slip was not level";
            won_ = false;
            over_ = true;
            mode_ = Mode::Fail;
            sys_->apu.noiseBurst(0.4f, 200.f, 0.25f);
            return;
        }
        onSlip_ = true;
    }
    if (onSlip_ && nose < kJoint - 4.f) onSlip_ = false;

    if (nose >= kBumper) {
        x_ = kBumper - kBusLen;
        if (vel_ > 16.f) {
            vel_ = 0.f;
            why_ = "hit the stop";
            won_ = false;
            over_ = true;
            mode_ = Mode::Fail;
            shake_ = 0.8f;
            sys_->apu.noiseBurst(0.5f, 120.f, 0.3f);
            return;
        }
        vel_ = 0.f;
    }
    if (x_ < -12.f) {
        why_ = "rolled off the quay";
        won_ = false;
        over_ = true;
        mode_ = Mode::Fail;
        return;
    }

    bool slack = playT_ >= kSlack && playT_ < kTurn && tide_ < 1.f;
    bool parked = onSlip_ && x_ >= kBerthLo && x_ <= kBerthHi && std::fabs(vel_) < 8.f;
    if (parked && slack) hold_ += kDt;
    else hold_ = 0.f;

    if (hold_ >= 0.7f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        vel_ = 0.f;
        sys_->apu.noise(0.f, 400.f);
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->apu.tone(1, 659.f, 0.1f);
        sys_->apu.tone(2, 784.f, 0.08f);
        return;
    }

    if (playT_ >= kFailAt || (onSlip_ && tide_ > 12.f && !won_)) {
        why_ = "the tide turned";
        won_ = false;
        over_ = true;
        mode_ = Mode::Fail;
        sys_->apu.noiseBurst(0.35f, 80.f, 0.4f);
    }
}

void Game::audio() {
    if (mode_ != Mode::Play) return;
    float vol = 0.02f + std::min(std::fabs(vel_) / 50.f, 1.f) * 0.06f;
    sys_->apu.noise(vol, 500.f + std::fabs(vel_) * 18.f);
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f) return;
    float s = h / float(m.h);
    float w = float(m.w) * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int16_t(std::lround(cx - w * 0.5f));
    sp.y = int16_t(std::lround(cy - h * 0.5f));
    sp.w = int16_t(std::lround(w));
    sp.h = int16_t(std::lround(h));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float cx = x;
    for (; *s; ++s) {
        unsigned char ch = (unsigned char)*s;
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        int gi = (ch >= 32 && ch < 128) ? ch - 32 : 0;
        float h = 7.f * scale;
        float w = 8.f * scale;
        spr(art_.glyph[gi], cx + w * 0.5f, y, h, pal, false);
        cx += 6.f * scale;
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = float(y) / float(gs::SCREEN_H);
        if (y < 128) {
            v.lineBackdrop[y] = gs::rgb4(4 + int(6 * (1.f - u)), 7 + int(4 * (1.f - u * 0.4f)), 12);
        } else {
            int deep = std::min(15, 3 + (y - 128) / 8);
            v.lineBackdrop[y] = gs::rgb4(1, 3 + (15 - deep) / 5, deep);
        }
        v.lineFog[y] = 0;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = false;
    sky();

    float camTarget = x_ - 70.f;
    cam_ += (camTarget - cam_) * 0.12f;
    float sh = (shake_ > 0.f) ? std::sin(t_ * 40.f) * shake_ * 3.f : 0.f;

    auto wx = [&](float world) { return world - cam_ + sh; };

    for (int i = -1; i < 8; i++) {
        float wy = 168.f + std::sin(t_ * 1.6f + i) * 2.f;
        spr(art_.wave, wx(i * 48.f + std::fmod(t_ * 18.f, 48.f)), wy, 12.f, PAL_WATER);
    }

    float deckY = 142.f;
    for (int i = -1; i < 5; i++) {
        float qx = i * 46.f;
        if (qx + 46.f < kJoint - 8.f) spr(art_.quay, wx(qx + 23.f), deckY + 6.f, 28.f, PAL_QUAY);
    }
    for (int i = 0; i < 4; i++) {
        float px = kJoint + 16.f + i * 32.f;
        spr(art_.piling, wx(px), deckY + tide_ + 22.f, 40.f, PAL_SLIP);
    }
    float slipL = kJoint;
    float slipR = kBumper + 8.f;
    spr(art_.quay, wx((slipL + slipR) * 0.5f), deckY + tide_ + 4.f, 22.f, PAL_SLIP);
    spr(art_.quay, wx((slipL + slipR) * 0.5f + 40.f), deckY + tide_ + 4.f, 22.f, PAL_SLIP);
    spr(art_.fender, wx(kJoint + 6.f), deckY + tide_ - 2.f, 20.f, PAL_ALERT);
    spr(art_.fender, wx(kBumper - 6.f), deckY + tide_ - 2.f, 20.f, PAL_ALERT);
    spr(art_.stop, wx(kBumper + 2.f), deckY + tide_ - 16.f, 26.f, PAL_ALERT);
    spr(art_.lamp, wx(40.f), deckY - 28.f, 30.f, PAL_HUD);
    spr(art_.lamp, wx(150.f), deckY - 28.f, 30.f, PAL_HUD);

    float gullX = std::fmod(t_ * 22.f, 360.f) - 20.f;
    spr(art_.gull, wx(gullX), 36.f + std::sin(t_ * 3.f) * 4.f, 8.f, PAL_SKY);

    float by = onSlip_ ? deckY + tide_ - 20.f : deckY - 22.f;
    float bob = std::sin(t_ * 2.f) * (onSlip_ ? 1.2f : 0.f);
    spr(art_.bus, wx(x_ + kBusLen * 0.5f), by + bob, 36.f, PAL_BUS);
    float spin = std::sin(wheel_) * 0.f;
    (void)spin;
    spr(art_.wheel, wx(x_ + 16.f), by + bob + 14.f, 12.f, PAL_BUS);
    spr(art_.wheel, wx(x_ + 48.f), by + bob + 14.f, 12.f, PAL_BUS);

    int left = std::max(0, int(std::ceil(kTurn - playT_)));
    char buf[64];
    if (mode_ == Mode::Title) {
        text("S3 BUS SLIP", 78.f, 48.f, 2.f, PAL_HUD);
        text("BERTH BEFORE THE TIDE TURNS", 52.f, 72.f, 1.f, PAL_TEXT);
        text("RIGHT ACCEL   LEFT BRAKE", 70.f, 96.f, 1.f, PAL_TEXT);
        text("PRESS START", 110.f, 118.f, 1.f, PAL_GOOD);
    } else if (mode_ == Mode::Play) {
        std::snprintf(buf, sizeof(buf), "TIDE %02d", left);
        text(buf, 8.f, 12.f, 1.f, left <= 6 ? PAL_ALERT : PAL_HUD);
        text(onSlip_ ? "ON THE SLIP" : "QUAY", 8.f, 24.f, 1.f, PAL_TEXT);
        if (tide_ < 1.f && playT_ < kTurn) text("LEVEL", 250.f, 12.f, 1.f, PAL_GOOD);
        else if (playT_ < kSlack) text("RISING", 240.f, 12.f, 1.f, PAL_ALERT);
        else text("TURNING", 228.f, 12.f, 1.f, PAL_ALERT);
        std::snprintf(buf, sizeof(buf), "SPD %d", int(std::lround(vel_)));
        text(buf, 8.f, 210.f, 1.f, PAL_TEXT);
    } else if (mode_ == Mode::Win) {
        text("BERTHED", 112.f, 40.f, 2.f, PAL_GOOD);
        text("BEFORE THE TIDE TURNED", 70.f, 64.f, 1.f, PAL_TEXT);
        text("START", 136.f, 90.f, 1.f, PAL_HUD);
    } else {
        text("MISSED THE SLIP", 86.f, 40.f, 2.f, PAL_ALERT);
        text(why_, 70.f, 66.f, 1.f, PAL_TEXT);
        text("START", 136.f, 90.f, 1.f, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);
    const gs::Pad& p = sys.pad;

    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || bot_) begin();
    } else if (mode_ == Mode::Play) {
        playT_ += kDt;
        step();
        audio();
    } else if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) {
        toTitle();
    }

    draw();
}

}  // namespace slip
