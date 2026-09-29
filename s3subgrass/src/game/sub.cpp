#include "sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace subgrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHalfL = 7.2f;
constexpr float kGrass0 = 186.f;
constexpr float kGrass1 = 258.f;
constexpr float kFlat0 = 176.f;
constexpr float kFlat1 = 268.f;
constexpr float kStop = 0.22f;
constexpr float kHoldNeed = 0.72f;
constexpr float kTimeLimit = 52.f;
constexpr float kPlayZoom = 4.4f;
constexpr float kTitleZoom = 1.35f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t + 0.5f), int(ag + (bg - ag) * t + 0.5f), int(ab + (bb - ab) * t + 0.5f));
}

const float kKelp[][2] = {
    {40.f, 30.f}, {70.f, 28.f}, {110.f, 32.f}, {200.f, 1.6f}, {220.f, 1.6f}, {240.f, 1.6f},
};

}  // namespace

float Game::bedAt(float x) const {
    if (x < 128.f) return 34.f;
    if (x < kFlat0) {
        float t = (x - 128.f) / (kFlat0 - 128.f);
        t = t * t * (3.f - 2.f * t);
        return 34.f + (2.15f - 34.f) * t;
    }
    if (x <= kFlat1) return 2.15f;
    if (x < 300.f) {
        float t = (x - kFlat1) / (300.f - kFlat1);
        return 2.15f + (28.f - 2.15f) * t;
    }
    return 28.f;
}

float Game::shelfUnder() const {
    float m = bedAt(x_ - kHalfL);
    m = std::min(m, bedAt(x_));
    m = std::min(m, bedAt(x_ + kHalfL));
    return m;
}

bool Game::hullOnGrass() const {
    float a = x_ - kHalfL;
    float b = x_ + kHalfL;
    if (a < kGrass0 || b > kGrass1) return false;
    return bedAt(a) < 3.f && bedAt(b) < 3.f && bedAt(x_) < 3.f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08f) return 3;
    if (hullOnGrass()) return 2;
    return 1;
}

void Game::showTitle() {
    x_ = 150.f;
    depth_ = 8.f;
    pitch_ = 0.08f;
    speed_ = 0.f;
    race_ = hold_ = 0.f;
    grounded_ = false;
    launched_ = false;
    won_ = over_ = false;
    why_[0] = 0;
    shake_ = 0.f;
    chimeN_ = 0;
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = 150.f;
    camD_ = 10.f;
    for (int i = 0; i < 12; i++) bubA_[i] = 0.f;
}

void Game::startRun() {
    x_ = 28.f;
    depth_ = 11.f;
    pitch_ = 0.f;
    speed_ = 0.f;
    race_ = hold_ = 0.f;
    grounded_ = false;
    launched_ = true;
    won_ = over_ = false;
    why_[0] = 0;
    shake_ = 0.f;
    chimeN_ = chimeStep_ = 0;
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camD_ = depth_;
    blip(520.f);
    bubbles_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.09f, 0.12f, 0.05f);
    t_ = 0.f;
    if (bot_) startRun();
    else showTitle();
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.07f;
}

void Game::chime(int notes) {
    chimeN_ = std::clamp(notes, 1, 6);
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::toneOff() {
    if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (tone1_ > 0.f) {
        tone1_ -= kDt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.f, 659.f, 784.f, 880.f, 1046.f};
            int n = notes[std::min(chimeStep_, 4)];
            sys_->apu.tone(2, n, 0.06f);
            chimeStep_++;
            chimeT_ = 0.16f;
            if (chimeStep_ >= chimeN_) {
                chimeN_ = 0;
                sys_->apu.tone(2, 0.f, 0.f);
            }
        }
    }
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    chime(5);
    sys_->rumble(0.25f, 0.1f, 140);
    sys_->setLight(40, 170, 70);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.45f, 0.2f, 160);
    sys_->setLight(170, 40, 30);
    sys_->apu.noiseBurst(0.35f, 90.f, 0.32f);
}

void Game::controls(float& pitchIn, float& thrust) {
    const gs::Pad& p = sys_->pad;
    pitchIn = 0.f;
    thrust = 0.f;
    if (p.down(gs::BTN_UP)) pitchIn += 1.f;
    if (p.down(gs::BTN_DOWN)) pitchIn -= 1.f;
    if (p.axisY > 0.22f) pitchIn = p.axisY;
    if (p.axisY < -0.22f) pitchIn = p.axisY;
    if (p.down(gs::BTN_RIGHT) || p.down(gs::BTN_C) || p.down(gs::BTN_A)) thrust += 1.f;
    if (p.down(gs::BTN_LEFT) || p.down(gs::BTN_B) || p.down(gs::BTN_X)) thrust -= 1.f;
    if (p.axisX > 0.2f) thrust = p.axisX;
    if (p.axisX < -0.2f) thrust = p.axisX;
    if (p.accel > 0.08f) thrust = std::max(thrust, p.accel);
    if (p.brake > 0.08f) thrust = std::min(thrust, -p.brake);
    pitchIn = clampf(pitchIn, -1.f, 1.f);
    thrust = clampf(thrust, -1.f, 1.f);
}

void Game::pilot(float& pitchIn, float& thrust) {
    float shelf = shelfUnder();
    float target = 9.5f;
    if (x_ > 120.f) target = std::min(9.5f, shelf - 1.4f);
    if (hullOnGrass()) target = shelf - 0.05f;
    float wantPitch = clampf((depth_ - target) * 0.42f, -0.42f, 0.5f);
    if (x_ > 150.f && x_ < kGrass0 && depth_ > shelf - 0.4f) wantPitch = std::max(wantPitch, 0.15f);
    pitchIn = clampf((wantPitch - pitch_) / 0.22f, -1.f, 1.f);

    if (hullOnGrass() && grounded_) {
        if (speed_ > 0.05f) thrust = -1.f;
        else if (speed_ < -0.05f) thrust = 1.f;
        else thrust = 0.f;
        return;
    }
    float want = 7.1f;
    if (x_ > 150.f) want = 4.0f;
    if (x_ > kGrass0 - 4.f) want = 2.2f;
    if (speed_ > want + 0.28f) thrust = -1.f;
    else if (speed_ < want - 0.2f) thrust = 1.f;
    else thrust = 0.45f;
}

void Game::physics(float pitchIn, float thrust) {
    race_ += kDt;
    pitch_ += pitchIn * 1.15f * kDt;
    pitch_ += -pitch_ * 0.35f * kDt;
    pitch_ = clampf(pitch_, -0.62f, 0.62f);

    speed_ += thrust * 9.2f * kDt;
    speed_ -= speed_ * 0.55f * kDt;
    speed_ = clampf(speed_, -4.5f, 9.2f);

    x_ += std::cos(pitch_) * speed_ * kDt;
    depth_ += -std::sin(pitch_) * speed_ * kDt;

    if (!grounded_) {
        float cruise = 10.f;
        depth_ += (cruise - depth_) * 0.35f * kDt;
    }
    if (depth_ < -0.4f) depth_ += 6.f * kDt;

    float shelf = shelfUnder();
    grounded_ = false;
    if (depth_ > shelf) {
        float hit = depth_ - shelf;
        depth_ = shelf;
        grounded_ = true;
        speed_ -= speed_ * (1.6f + std::min(hit, 4.f)) * kDt;
        if (hit > 1.2f && std::fabs(speed_) > 3.f) shake_ = std::min(1.f, shake_ + 0.15f);
    }

    if (mode_ == Mode::Run) {
        if (hullOnGrass() && grounded_ && std::fabs(speed_) < kStop) {
            hold_ += kDt;
            if (hold_ >= kHoldNeed) win();
        } else {
            hold_ = std::max(0.f, hold_ - kDt * 0.5f);
        }
        if (x_ > 292.f) fail("past the bank");
        else if (x_ < 4.f && race_ > 2.f) fail("backed out");
        else if (race_ > kTimeLimit) fail("the clock ran out");
    }

    if (std::fabs(speed_) > 0.35f && (bubbles_++ % 8) == 0) {
        for (int i = 0; i < 12; i++) {
            if (bubA_[i] <= 0.f) {
                bubX_[i] = x_ - 8.f;
                bubD_[i] = depth_ + 0.4f;
                bubA_[i] = 1.f;
                break;
            }
        }
    }
    for (int i = 0; i < 12; i++) {
        if (bubA_[i] <= 0.f) continue;
        bubA_[i] -= kDt * 0.7f;
        bubD_[i] -= kDt * 2.2f;
        bubX_[i] -= kDt * 0.4f;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.5f || m.h <= 0) return;
    const gs::Image& im = m.pick(h);
    if (im.h <= 0) return;
    float s = h / float(im.h);
    gs::Sprite sp;
    sp.img = im;
    sp.w = std::max(1, int(im.w * s));
    sp.h = std::max(1, int(h));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(cy - sp.h * 0.5f));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::hud(float x, float y, const std::string& s, int pal, float scale) {
    float cw = 6.f * scale;
    float h = 7.f * scale;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 32 || c > 127) c = '?';
        spr(art_.glyph[c - 32], x + float(i) * cw, y, h, pal);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;

    float shakeX = 0.f;
    if (shake_ > 0.f) {
        shakeX = std::sin(t_ * 48.f) * shake_ * 3.f;
        shake_ = std::max(0.f, shake_ - kDt);
    }
    float look = mode_ == Mode::Title ? 0.f : clampf(speed_ * 1.1f, -8.f, 14.f);
    float wantX = x_ + look;
    float wantD = depth_;
    if (mode_ == Mode::Title) {
        wantX = 150.f;
        wantD = 12.f;
        zoom_ = kTitleZoom;
    } else {
        zoom_ = kPlayZoom;
    }
    camX_ += (wantX - camX_) * 0.08f;
    camD_ += (wantD - camD_) * 0.08f;

    auto toS = [&](float wx, float wd, float& sx, float& sy) {
        sx = (wx - camX_) * zoom_ + 160.f + shakeX;
        sy = (wd - camD_) * zoom_ + 96.f;
    };

    const uint16_t skyTop = gs::rgb4(4, 8, 12);
    const uint16_t skyHor = gs::rgb4(9, 13, 14);
    const uint16_t sea0 = gs::rgb4(2, 7, 11);
    const uint16_t sea1 = gs::rgb4(1, 2, 5);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wd = (float(y) - 96.f) / zoom_ + camD_;
        if (wd < 0.f) {
            float t = clampf((wd + 18.f) / 18.f, 0.f, 1.f);
            vdp.lineBackdrop[y] = lerpC(skyTop, skyHor, t);
        } else {
            float t = clampf(wd / 30.f, 0.f, 1.f);
            vdp.lineBackdrop[y] = lerpC(sea0, sea1, t);
        }
        vdp.lineFog[y] = 0;
    }

    float x0 = camX_ - 180.f / zoom_;
    float x1 = camX_ + 180.f / zoom_;
    for (float wx = std::floor(x0 / 8.f) * 8.f; wx < x1; wx += 8.f) {
        float bed = bedAt(wx + 4.f);
        float sx, sy, bx, by;
        toS(wx + 4.f, bed, sx, sy);
        toS(wx + 4.f, bed + 16.f, bx, by);
        (void)bx;
        float gh = std::max(8.f, by - sy);
        int pal = PAL_ROCK;
        if (bed < 3.f) pal = PAL_GRASS;
        else if (bed < 16.f) pal = PAL_MUD;
        spr(art_.ground, sx, sy + gh * 0.45f, gh, pal);
        if (bed < 3.2f && (int(wx) / 8) % 2 == 0) {
            float tx, ty;
            toS(wx + 4.f, bed - 0.2f, tx, ty);
            spr(art_.tuft, tx, ty - 6.f, 16.f, PAL_GRASS);
        }
    }

    for (const auto& k : kKelp) {
        if (k[0] < x0 || k[0] > x1) continue;
        float sx, sy;
        toS(k[0], bedAt(k[0]) - 1.2f, sx, sy);
        spr(art_.tuft, sx, sy, k[1] > 10.f ? 22.f : 14.f, PAL_KELP);
    }

    for (int i = 0; i < 12; i++) {
        if (bubA_[i] <= 0.f) continue;
        float sx, sy;
        toS(bubX_[i], bubD_[i], sx, sy);
        spr(art_.bubble, sx, sy, 6.f + bubA_[i] * 4.f, PAL_BUB);
    }

    int frame = 0;
    if (pitch_ > 0.12f) frame = 1;
    else if (pitch_ < -0.12f) frame = 2;
    float hx, hy;
    toS(x_, depth_, hx, hy);
    float hh = 36.f * (zoom_ / kPlayZoom);
    if (mode_ == Mode::Title) hh = 28.f;
    spr(art_.sub[frame], hx, hy, hh, PAL_SUB);
    float fx, fy;
    toS(x_ + 4.2f, depth_ - 2.4f, fx, fy);
    spr(art_.sailflag, fx, fy - hh * 0.28f, 8.f, PAL_SUB);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 36.f, 28.f, PAL_TITLE);
        hud(70.f, 168.f, "LAND ON THE GRASS", PAL_HUD, 2.f);
        hud(78.f, 188.f, "THEN A FULL STOP", PAL_HUD, 2.f);
        hud(92.f, 208.f, "ENTER TO DIVE", PAL_TITLE, 1.f);
    } else if (mode_ == Mode::Win) {
        spr(art_.wordStop, 160.f, 36.f, 22.f, PAL_WIN);
        spr(art_.wordGrass, 160.f, 62.f, 18.f, PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        hud(100.f, 28.f, "MISSED", PAL_FAIL, 2.f);
        hud(70.f, 50.f, why_, PAL_FAIL, 1.f);
        hud(86.f, 200.f, "ENTER RETRIES", PAL_HUD, 1.f);
    } else {
        const char* hint = "PLANES UP, SCREW AHEAD";
        if (hold_ > 0.02f) hint = "HOLD THE FULL STOP";
        else if (hullOnGrass() && grounded_) hint = "KILL THE SCREW";
        else if (x_ > 140.f) hint = "THE GRASS SHELF IS AHEAD";
        hud(8.f, 12.f, hint, hold_ > 0.02f ? PAL_WIN : PAL_HUD, 1.f);
        char buf[40];
        std::snprintf(buf, sizeof buf, "SPD %4.1f", speed_);
        hud(8.f, 210.f, buf, PAL_HUD, 1.f);
        std::snprintf(buf, sizeof buf, "%4.0f S", kTimeLimit - race_);
        hud(250.f, 210.f, buf, race_ > kTimeLimit - 10.f ? PAL_FAIL : PAL_HUD, 1.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_A)) startRun();
        else if (p.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_C))) showTitle();
        else if (!bot_ && p.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) {
            /* keep diving; start is not pause */
        }
        if (!bot_ && p.pressed(gs::BTN_MODE)) showTitle();
        float pitchIn = 0, thrust = 0;
        if (bot_) pilot(pitchIn, thrust);
        else controls(pitchIn, thrust);
        physics(pitchIn, thrust);
        if (std::fabs(speed_) > 0.4f) {
            sys.apu.tone(0, 48.f + std::fabs(speed_) * 7.f, 0.035f);
            tone0_ = 0.1f;
        }
    }
    toneOff();
    draw();
}

}  // namespace subgrass
