#include "game/tram.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tramboom {
namespace {

constexpr float kBoom = 500.f;
constexpr float kPix = 2.2f;
constexpr float kDt = 1.f / 60.f;
constexpr float kPocket = 5.2f;
constexpr float kCrawl = 1.12f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = false;
    sys.apu.setMaster(0.8f);
    if (bot_) beginLeg();
    else mode_ = Mode::Title;
}

void Game::beginLeg() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    why_ = "";
    x_ = 36.f;
    v_ = 0.f;
    hold_ = 0.f;
    still_ = 0.f;
    slide_ = 0.f;
    t_ = 0.f;
    frames_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (mode_ == Mode::Deliver) return 3;
    if (kBoom - x_ < 150.f) return 2;
    return 1;
}

float Game::screenX(float world) const { return 150.f + (world - x_) * kPix; }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float h, int pal, int align) {
    if (s.empty()) return;
    const float adv = h * 0.72f;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        spr(art_.glyph[c - 32], x + i * adv + adv * 0.5f, y, h, pal, false);
    }
}

void Game::bot(bool& accel, bool& brake) const {
    float d = kBoom - x_;
    float want = 0.f;
    if (d > kPocket) want = std::clamp(d * 0.28f, 0.45f, 16.f);
    float err = want - v_;
    accel = err > 0.12f;
    brake = err < -0.12f;
}

void Game::update(float dt) {
    bool accel = false, brake = false;
    if (bot_) bot(accel, brake);
    else {
        const gs::Pad& p = sys_->pad;
        accel = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.accel > 0.25f;
        brake = p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.brake > 0.25f;
        if (brake) accel = false;
    }
    float thrust = 0.f;
    if (accel) thrust += 9.5f;
    if (brake) thrust -= 14.f;
    if (!accel && !brake && v_ > 0.f) thrust -= 1.5f;
    v_ += thrust * dt;
    if (v_ < 0.f) v_ = 0.f;
    if (v_ > 18.f) v_ = 18.f;
    x_ += v_ * dt;

    float d = kBoom - x_;
    bool pocket = std::fabs(d) <= kPocket && v_ <= kCrawl;
    if (mode_ == Mode::Run || mode_ == Mode::Deliver) {
        if (v_ > 7.6f && std::fabs(d) < 9.f) {
            mode_ = Mode::Lost;
            why_ = "broke the boom";
            over_ = true;
            won_ = false;
            sys_->apu.noiseBurst(0.4f, 1800.f, 0.25f);
        } else if (x_ > kBoom + 12.f) {
            mode_ = Mode::Lost;
            why_ = "missed the end";
            over_ = true;
            won_ = false;
            sys_->apu.noiseBurst(0.35f, 900.f, 0.3f);
        } else if (v_ < 0.18f && d > 16.f && t_ > 1.2f) {
            still_ += dt;
            if (still_ > 2.0f) {
                mode_ = Mode::Lost;
                why_ = "stopped short of the boom";
                over_ = true;
                won_ = false;
            }
        } else {
            still_ = 0.f;
        }
    }
    if (mode_ == Mode::Run && pocket) mode_ = Mode::Deliver;
    if (mode_ == Mode::Deliver) {
        if (!pocket && v_ > kCrawl) {
            mode_ = Mode::Run;
            hold_ = 0.f;
            slide_ = 0.f;
        } else {
            hold_ += dt;
            slide_ = std::min(1.f, slide_ + dt * 1.8f);
            if (hold_ > 0.6f && std::fabs(d) <= kPocket && v_ <= kCrawl) {
                mode_ = Mode::Won;
                won_ = true;
                over_ = true;
                why_ = "the drive is on the boom";
                v_ = 0.f;
                sys_->apu.noiseBurst(0.2f, 400.f, 0.15f);
            }
        }
    }
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int(2 + (10 - 2) * u);
        int g = int(2 + (6 - 2) * u);
        int b = int(6 + (4 - 6) * u);
        if (y > 150) {
            float gnd = (y - 150) / 74.f;
            r = int(6 + 2 * gnd);
            g = int(5 + 1 * gnd);
            b = int(4);
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    sky();
    gs::VDP& v = sys_->vdp;
    v.clearSprites();

    auto scenery = [&](float wx, const gs::Mipped& m, float cy, float h, int pal) {
        spr(m, screenX(wx), cy, h, pal, false);
    };
    for (int i = 0; i < 8; i++) {
        float wx = 80.f + i * 70.f;
        scenery(wx, (i % 2) ? art_.block : art_.shed, 118.f, (i % 2) ? 70.f : 78.f, PAL_CITY);
        scenery(wx + 8.f, art_.pole, 108.f, 64.f, PAL_DUSK);
    }
    scenery(kBoom + 28.f, art_.shed, 112.f, 86.f, PAL_CITY);
    for (float wx = -20.f; wx < kBoom + 80.f; wx += 28.f) spr(art_.wire, screenX(wx), 78.f, 4.f, PAL_DUSK, false);
    for (float wx = -10.f; wx < kBoom + 40.f; wx += 22.f) spr(art_.rail, screenX(wx), 176.f, 14.f, PAL_DUSK, false);
    spr(art_.bumper, screenX(kBoom + 16.f), 164.f, 32.f, PAL_ALERT, false);
    spr(art_.boom, screenX(kBoom + 6.f), 132.f, 92.f, PAL_BOOM, false);

    float tramS = screenX(x_);
    bool spin = (int(x_ * 2.f) & 1) != 0;
    spr(art_.wheel, tramS - 46.f, 176.f, 22.f, PAL_DUSK, spin);
    spr(art_.wheel, tramS + 40.f, 176.f, 22.f, PAL_DUSK, spin);
    spr(art_.tram, tramS, 150.f, 64.f, PAL_TRAM, false);

    float driveX = x_ + (kBoom - x_) * slide_;
    float driveY = 148.f + 10.f * slide_;
    spr(art_.drive, screenX(driveX), driveY, 22.f, PAL_DRIVE, false);

    char buf[64];
    std::snprintf(buf, sizeof buf, "SPD %02d", int(v_ * 4.f));
    text(buf, 8.f, 16.f, 14.f, PAL_HUD, -1);
    float dist = std::max(0.f, kBoom - x_);
    std::snprintf(buf, sizeof buf, "END %03d", int(dist));
    text(buf, 312.f, 16.f, 14.f, dist < 20.f ? PAL_OK : PAL_HUD, 1);
    text("TRAM BOOM", 160.f, 16.f, 14.f, PAL_HUD, 0);

    if (mode_ == Mode::Title) {
        text("S3 TRAM BOOM", 160.f, 78.f, 22.f, PAL_HUD, 0);
        text("DELIVER THE DRIVE", 160.f, 104.f, 14.f, PAL_OK, 0);
        text("TO THE BOOM", 160.f, 122.f, 14.f, PAL_OK, 0);
        if ((blink_ / 30) % 2 == 0) text("PRESS START", 160.f, 156.f, 16.f, PAL_HUD, 0);
        text("A GO   B BRAKE", 160.f, 188.f, 12.f, PAL_HUD, 0);
        text("MISS THE END AND THE LEG FAILS", 160.f, 206.f, 11.f, PAL_ALERT, 0);
    } else if (mode_ == Mode::Deliver) {
        text("EASE IT ON", 160.f, 48.f, 16.f, PAL_OK, 0);
    } else if (mode_ == Mode::Won) {
        text("LEG MADE", 160.f, 48.f, 20.f, PAL_OK, 0);
        text("DRIVE IS ON THE BOOM", 160.f, 70.f, 13.f, PAL_HUD, 0);
    } else if (mode_ == Mode::Lost) {
        text("LEG FAILED", 160.f, 48.f, 18.f, PAL_ALERT, 0);
        text(why_, 160.f, 70.f, 13.f, PAL_HUD, 0);
        if ((blink_ / 30) % 2 == 0) text("START TO RETRY", 160.f, 96.f, 12.f, PAL_HUD, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    blink_++;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) beginLeg();
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && p.pressed(gs::BTN_START)) beginLeg();
    } else {
        t_ += kDt;
        frames_++;
        update(kDt);
        float hum = mode_ == Mode::Run || mode_ == Mode::Deliver ? 0.05f + v_ * 0.008f : 0.f;
        sys.apu.tone(0, 70.f + v_ * 9.f, hum);
        sys.apu.tone(1, 140.f + v_ * 6.f, hum * 0.35f);
    }
    if (mode_ == Mode::Title || mode_ == Mode::Won || mode_ == Mode::Lost) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    draw();
}

}  // namespace tramboom
