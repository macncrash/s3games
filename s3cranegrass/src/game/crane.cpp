#include "game/crane.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cranegrass {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kBodyL = 6.6f;
constexpr float kBodyW = 3.15f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 8.f;
constexpr float kStartH = kPi * 0.5f;
constexpr float kRoad = 5.4f;
constexpr float kSouth = 0.5f;
constexpr float kGrassX0 = -9.5f;
constexpr float kGrassX1 = 9.5f;
constexpr float kGrassY0 = 38.f;
constexpr float kGrassY1 = 64.f;
constexpr float kAimX = 0.f;
constexpr float kAimY = 50.f;
constexpr float kAhead = 11.f;
constexpr float kBrake = 16.f;
constexpr float kStop = 0.16f;
constexpr float kHoldNeed = 0.75f;
constexpr float kShortNeed = 1.15f;
constexpr float kNoseNeed = 1.15f;
constexpr float kTimeLimit = 48.f;
constexpr float kPlayZoom = 10.5f;
constexpr float kTitleZoom = 7.2f;
constexpr float kTitleCamX = 1.5f;
constexpr float kTitleCamY = 46.f;

float wrap(float a) {
    while (a > kPi) a -= kPi * 2.f;
    while (a < -kPi) a += kPi * 2.f;
    return a;
}

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (mode_ != Mode::Run && mode_ != Mode::Pause) return 0;
    if (phase_ >= 3 || hold_ > 0.02f) return 3;
    if (planted_ || deep_) return 2;
    if (rolled_ || y_ > 16.f) return 1;
    return 0;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 0.f;
    clock_ = hold_ = shortT_ = noseT_ = 0.f;
    planted_ = deep_ = rolled_ = false;
    won_ = over_ = false;
    phase_ = 0;
    chimeN_ = chimeStep_ = 0;
    puffI_ = 0;
    why_[0] = 0;
    for (Puff& p : puffs_) p = {};
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::startRun() {
    begin();
    mode_ = Mode::Run;
    zoom_ = kPlayZoom;
    camX_ = x_;
    camY_ = y_;
    blip(440.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.06f, 0.12f, 0.05f);
    t_ = 0.f;
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& gas, float& brake) {
    const gs::Pad& p = sys_->pad;
    steer = gas = brake = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.2f) steer = clampf(-p.axisX, -1.f, 1.f);
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.12f) gas = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.12f) brake = 1.f;
    if (p.accel > 0.05f) gas = std::max(gas, p.accel);
    if (p.brake > 0.05f) brake = std::max(brake, p.brake);
}

void Game::pilot(float& steer, float& gas, float& brake) {
    float hdes = std::atan2(kAimY - y_, kAimX - x_);
    if (y_ > kGrassY0 + 2.f) hdes = kStartH + clampf((kAimX - x_) * 0.18f, -0.35f, 0.35f);
    float err = wrap(hdes - heading_);
    steer = clampf(err / 0.32f, -1.f, 1.f);
    gas = brake = 0.f;
    if (y_ < kAimY - 2.8f) {
        float cap = std::fabs(err) > 0.55f ? 4.2f : 6.4f;
        if (speed_ < cap) gas = 1.f;
        else if (speed_ > cap + 0.35f) brake = 0.55f;
        phase_ = y_ > kGrassY0 - 6.f ? 1 : 0;
    } else {
        phase_ = 3;
        brake = 1.f;
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone0_ = 0.08f;
}

void Game::chime() {
    chimeN_ = 5;
    chimeStep_ = 0;
    chimeT_ = 0.02f;
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0.f;
    std::snprintf(why_, sizeof why_, "full stop");
    chime();
    sys_->rumble(0.22f, 0.1f, 120);
    sys_->setLight(36, 170, 64);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    std::snprintf(why_, sizeof why_, "%s", why);
    shake_ = 1.f;
    sys_->rumble(0.45f, 0.2f, 150);
    sys_->setLight(170, 30, 22);
    sys_->apu.noiseBurst(0.35f, 90.f, 0.32f);
}

void Game::corners(float& x0, float& y0, float& x1, float& y1, float& x2, float& y2, float& x3, float& y3) const {
    float c = std::cos(heading_), s = std::sin(heading_);
    float hx = c * kBodyL * 0.5f, hy = s * kBodyL * 0.5f;
    float wx = -s * kBodyW * 0.5f, wy = c * kBodyW * 0.5f;
    x0 = x_ + hx + wx;
    y0 = y_ + hy + wy;
    x1 = x_ + hx - wx;
    y1 = y_ + hy - wy;
    x2 = x_ - hx - wx;
    y2 = y_ - hy - wy;
    x3 = x_ - hx + wx;
    y3 = y_ - hy + wy;
}

bool Game::insideGrass(float x, float y, float inset) const {
    return x >= kGrassX0 + inset && x <= kGrassX1 - inset && y >= kGrassY0 + inset && y <= kGrassY1 - inset;
}

bool Game::hullPlanted(float inset) const {
    float x0, y0, x1, y1, x2, y2, x3, y3;
    corners(x0, y0, x1, y1, x2, y2, x3, y3);
    return insideGrass(x0, y0, inset) && insideGrass(x1, y1, inset) && insideGrass(x2, y2, inset) &&
           insideGrass(x3, y3, inset);
}

void Game::physics(float steer, float gas, float brake) {
    clock_ += kDt;
    float rate = 1.35f + std::min(std::fabs(speed_), 7.f) * 0.06f;
    if (std::fabs(speed_) < 0.35f) rate *= 0.4f;
    heading_ = wrap(heading_ + steer * rate * kDt * (speed_ < 0.f ? -1.f : 1.f));

    if (brake > 0.05f) {
        float drop = brake * kBrake * kDt;
        if (speed_ > 0.f) speed_ = std::max(0.f, speed_ - drop);
        else speed_ = std::min(0.f, speed_ + drop);
    } else if (gas > 0.05f) {
        speed_ += gas * kAhead * kDt;
    } else {
        speed_ -= speed_ * 0.55f * kDt;
    }

    planted_ = hullPlanted(0.f);
    float drag = planted_ ? 1.35f : 0.22f;
    speed_ *= std::exp(-drag * kDt);
    speed_ = clampf(speed_, -2.2f, 7.6f);

    float hc = std::cos(heading_), hs = std::sin(heading_);
    x_ += hc * speed_ * kDt;
    y_ += hs * speed_ * kDt;

    auto thud = [&]() {
        if (thumpT_ > 0.f) return;
        sys_->apu.noiseBurst(0.18f, 140.f, 0.08f);
        thumpT_ = 0.2f;
        shake_ = std::max(shake_, 0.35f);
    };

    float x0, y0, x1, y1, x2, y2, x3, y3;
    corners(x0, y0, x1, y1, x2, y2, x3, y3);
    float minX = std::min(std::min(x0, x1), std::min(x2, x3));
    float maxX = std::max(std::max(x0, x1), std::max(x2, x3));
    float minY = std::min(std::min(y0, y1), std::min(y2, y3));
    float maxY = std::max(std::max(y0, y1), std::max(y2, y3));

    if (minY < kSouth) {
        y_ += kSouth - minY + 0.05f;
        if (speed_ < 0.f) speed_ *= 0.25f;
        thud();
    }
    corners(x0, y0, x1, y1, x2, y2, x3, y3);
    minY = std::min(std::min(y0, y1), std::min(y2, y3));
    maxY = std::max(std::max(y0, y1), std::max(y2, y3));
    minX = std::min(std::min(x0, x1), std::min(x2, x3));
    maxX = std::max(std::max(x0, x1), std::max(x2, x3));
    if (maxY < kGrassY0 + 1.2f) {
        if (minX < -kRoad) {
            x_ += -kRoad - minX + 0.05f;
            speed_ *= 0.86f;
            thud();
        }
        if (maxX > kRoad) {
            x_ += kRoad - maxX - 0.05f;
            speed_ *= 0.86f;
            thud();
        }
    }

    planted_ = hullPlanted(0.f);
    deep_ = hullPlanted(1.5f);
    if (std::fabs(speed_) > 1.4f || y_ > 22.f) rolled_ = true;

    corners(x0, y0, x1, y1, x2, y2, x3, y3);
    maxY = std::max(std::max(y0, y1), std::max(y2, y3));
    minY = std::min(std::min(y0, y1), std::min(y2, y3));
    minX = std::min(std::min(x0, x1), std::min(x2, x3));
    maxX = std::max(std::max(x0, x1), std::max(x2, x3));

    if (maxY > kGrassY1 + 0.15f) {
        fail("ran off the grass");
        return;
    }
    if (minY > kGrassY0 && (minX < kGrassX0 || maxX > kGrassX1)) {
        fail("off the grass");
        return;
    }
    if (clock_ > kTimeLimit) {
        fail("timed out");
        return;
    }

    float sp = std::fabs(speed_);
    if (deep_ && sp <= kStop) {
        shortT_ = noseT_ = 0.f;
        hold_ += kDt;
        phase_ = 3;
        if (hold_ >= kHoldNeed) {
            win();
            return;
        }
    } else if (!rolled_) {
        hold_ = 0.f;
    } else if (sp <= kStop && maxY < kGrassY0 - 0.4f) {
        hold_ = 0.f;
        shortT_ += kDt;
        if (shortT_ >= kShortNeed) {
            fail("stopped short of the grass");
            return;
        }
    } else if (sp <= kStop && !planted_) {
        hold_ = 0.f;
        noseT_ += kDt;
        if (noseT_ >= kNoseNeed) {
            fail("not fully on the grass");
            return;
        }
    } else {
        if (!deep_ || sp > kStop) hold_ = 0.f;
        shortT_ = noseT_ = 0.f;
        if (planted_) phase_ = 2;
    }

    if ((gas > 0.2f || sp > 1.1f) && int(t_ * 60.f) % 7 == 0) {
        Puff& p = puffs_[puffI_];
        p.x = x_ - hc * 2.6f;
        p.y = y_ - hs * 2.6f;
        p.life = 1.f;
        puffI_ = (puffI_ + 1) % 6;
    }
    for (Puff& p : puffs_)
        if (p.life > 0.f) p.life -= kDt * 0.85f;
}

void Game::audio() {
    float bed = mode_ == Mode::Run ? 0.01f + std::fabs(speed_) * 0.0012f : 0.006f;
    sys_->apu.noise(bed, planted_ ? 160.f : 380.f, false);
    if (mode_ == Mode::Run && std::fabs(speed_) > 0.2f) {
        float wob = 0.8f + 0.2f * std::sin(t_ * (9.f + std::fabs(speed_)));
        sys_->apu.tone(2, 42.f + std::fabs(speed_) * 6.5f, (0.012f + std::fabs(speed_) * 0.003f) * wob);
    } else {
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {349.f, 440.f, 523.f, 698.f, 880.f};
            sys_->apu.tone(0, notes[std::min(chimeStep_, 4)], 0.05f);
            tone0_ = 0.14f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else if (tone0_ > 0.f) {
        tone0_ -= kDt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (thumpT_ > 0.f) thumpT_ -= kDt;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) startRun();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(300.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            float steer = 0.f, gas = 0.f, brake = 0.f;
            if (bot_) pilot(steer, gas, brake);
            else controls(steer, gas, brake);
            physics(steer, gas, brake);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 2.f);
    camera();
    audio();
    if (mode_ == Mode::Win) sys.setLight(36, 170, 64);
    else if (mode_ == Mode::Fail) sys.setLight(170, 30, 22);
    else if (hold_ > 0.02f) sys.setLight(40, 150, 48);
    else sys.setLight(30, 70, 90);
    draw();
}

void Game::camera() {
    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
        return;
    }
    float lead = mode_ == Mode::Run ? 4.2f : 0.f;
    float gx = x_ + std::cos(heading_) * lead;
    float gy = y_ + std::sin(heading_) * lead;
    float k = 1.f - std::exp(-kDt * 4.5f);
    camX_ += (gx - camX_) * k;
    camY_ += (gy - camY_) * k;
    zoom_ += (kPlayZoom - zoom_) * k;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -12 || cy + h * 0.5f < -12 || cx - w * 0.5f > gs::SCREEN_W + 12 ||
        cy - h * 0.5f > gs::SCREEN_H + 12)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1600L);
    long sh = std::clamp(std::lround(h), 1L, 1600L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::worldSpr(const gs::Mipped& m, float wx, float wy, float worldH, int pal) {
    float sx = (wx - camX_) * zoom_ + gs::SCREEN_W * 0.5f;
    float sy = (camY_ - wy) * zoom_ + 118.f;
    float jx = (shake_ > 0.f) ? std::sin(t_ * 47.f) * shake_ * 3.f : 0.f;
    spr(m, sx + jx, sy, worldH * zoom_, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / float(gs::SCREEN_H - 1);
        int r = int(6 + (12 - 6) * u);
        int g = int(9 + (14 - 9) * u);
        int b = int(13 + (15 - 13) * u);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    worldSpr(art_.water, 0.f, kGrassY1 + 8.f, 16.f, PAL_YARD);
    worldSpr(art_.grass, (kGrassX0 + kGrassX1) * 0.5f, (kGrassY0 + kGrassY1) * 0.5f, kGrassY1 - kGrassY0, PAL_YARD);
    for (float yy = 2.f; yy < kGrassY0 - 1.f; yy += 8.5f) worldSpr(art_.slab, 0.f, yy, 9.f, PAL_YARD);

    const float trees[][2] = {{-12.f, 42.f}, {12.2f, 44.f}, {-11.5f, 58.f}, {12.f, 60.f}, {-13.f, 28.f}, {13.f, 30.f}};
    for (auto& tr : trees) worldSpr(art_.tree, tr[0], tr[1], 4.2f, PAL_YARD);

    for (const Puff& p : puffs_)
        if (p.life > 0.05f) worldSpr(art_.puff, p.x, p.y, 1.2f + (1.f - p.life) * 1.4f, PAL_FX);

    worldSpr(art_.shadow, x_, y_ - 0.35f, 3.4f, PAL_FX);

    int hi = int(std::lround(heading_ * float(HEADINGS) / (kPi * 2.f)));
    hi = (hi % HEADINGS + HEADINGS) % HEADINGS;
    worldSpr(art_.crane[hi], x_, y_, 7.4f, PAL_CRANE);
    if (hold_ > 0.05f || mode_ == Mode::Win) worldSpr(art_.legs, x_, y_, 4.6f, PAL_BOOM);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(6, "S3 CRANE GRASS", PAL_AMBER);
        hudC(9, "LAND ON THE GRASS", PAL_HUD);
        hudC(11, "COME TO A FULL STOP", PAL_HUD);
        hudC(16, "ARROWS STEER AND DRIVE", PAL_GREEN);
        hudC(18, "START TO ROLL", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(4, "FULL STOP", PAL_GREEN);
        hudC(6, "THE CRANE IS ON THE GRASS", PAL_HUD);
        std::snprintf(line, sizeof line, "%.1f S", clock_);
        hudC(8, line, PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        hudC(4, "MISSED", PAL_RED);
        hudC(6, why_, PAL_HUD);
        hudC(20, "START TO TRY AGAIN", PAL_AMBER);
    } else {
        std::snprintf(line, sizeof line, "SPD %d", int(std::lround(std::fabs(speed_) * 10.f)));
        hud(1, 1, line, PAL_HUD);
        std::snprintf(line, sizeof line, "%.0f", clock_);
        hud(34, 1, line, PAL_AMBER);
        if (hold_ > 0.02f) hudC(24, "HOLDING", PAL_GREEN);
        else if (!rolled_) hudC(24, "THE GRASS IS AHEAD", PAL_HUD);
        else if (!planted_) hudC(24, "GET THE WHOLE CRANE ON", PAL_AMBER);
        else hudC(24, "STOP", PAL_GREEN);
    }
}

}  // namespace cranegrass
