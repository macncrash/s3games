#include "mark.h"

#include <cmath>
#include <cstdio>

namespace bargemark {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kMarkX = 68.f;
constexpr float kAlong = 1.35f;
constexpr float kAcross = 0.62f;
constexpr float kStop = 0.16f;
constexpr float kHoldNeed = 1.05f;
constexpr float kWaterY = 128.f;
constexpr float kPpm = 3.15f;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

}  // namespace

void Game::begin() {
    mode_ = Mode::Title;
    banner_ = nullptr;
    over_ = false;
    won_ = false;
    why_[0] = 0;
    report_[0] = 0;
    t_ = 0;
    race_ = 0;
    hold_ = 0;
    chimeN_ = 0;
    chimeStep_ = 0;
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    x_ = 8.f;
    y_ = 1.35f;
    vel_ = 0;
    latV_ = 0;
    hold_ = 0;
    race_ = 0;
    camX_ = 28.f;
    banner_ = nullptr;
}

void Game::startRun() {
    x_ = 8.f;
    y_ = 1.35f;
    vel_ = 0;
    latV_ = 0;
    hold_ = 0;
    race_ = 0;
    over_ = false;
    won_ = false;
    why_[0] = 0;
    report_[0] = 0;
    mode_ = Mode::Run;
    banner_ = nullptr;
    blip(220.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.resize(64, 32);
    sys.apu.setMaster(0.8f);
    begin();
    if (bot_) startRun();
}

bool Game::onPaint() const { return std::fabs(x_ - kMarkX) <= kAlong && std::fabs(y_) <= kAcross; }

bool Game::settled() const { return std::fabs(vel_) <= kStop && std::fabs(latV_) <= kStop; }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (hold_ > 0.08f) return 3;
    if (onPaint()) return 2;
    return 1;
}

void Game::controls(float& thrust, float& steer) {
    const gs::Pad& p = sys_->pad;
    thrust = 0;
    steer = 0;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A)) thrust += 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) thrust -= 1.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    thrust += p.axisY;
    steer += p.axisX;
    thrust = clampf(thrust, -1.f, 1.f);
    steer = clampf(steer, -1.f, 1.f);
}

void Game::pilot(float& thrust, float& steer) {
    float err = kMarkX - x_;
    float wantV;
    if (std::fabs(err) < 1.6f && std::fabs(y_) < 0.9f && std::fabs(vel_) < 0.7f) {
        // Hold against the downstream current once the paint is under the keel.
        float cancel = -0.62f;
        thrust = clampf(cancel + (0.f - vel_) * 1.5f + err * 0.45f, -1.f, 1.f);
    } else {
        if (err > 16.f) wantV = 2.15f;
        else if (err > 6.f) wantV = 1.05f;
        else if (err > 0.4f) wantV = err * 0.22f;
        else wantV = err * 0.55f;
        thrust = clampf((wantV - vel_) * 2.1f, -1.f, 1.f);
    }
    steer = clampf(-y_ * 2.4f - latV_ * 1.6f, -1.f, 1.f);
}

void Game::physics(float thrust, float steer) {
    float drive = thrust * (thrust >= 0.f ? 1.25f : 0.95f);
    vel_ += (0.58f + drive - vel_ * 0.72f) * kDt;
    float cross = 0.32f * std::sin(x_ * 0.085f);
    latV_ += (steer * 0.95f + cross - latV_ * 1.9f) * kDt;
    vel_ = clampf(vel_, -2.2f, 3.1f);
    latV_ = clampf(latV_, -1.6f, 1.6f);
    x_ += vel_ * kDt;
    y_ += latV_ * kDt;
    y_ = clampf(y_, -3.2f, 3.2f);
    if (x_ < 2.f) {
        x_ = 2.f;
        if (vel_ < 0.f) vel_ = 0.f;
    }
}

void Game::judge() {
    if (onPaint() && settled()) {
        hold_ += kDt;
        if (hold_ >= kHoldNeed) win();
    } else {
        hold_ = 0.f;
    }
    if (mode_ == Mode::Run && race_ >= limit_) fail("THE OTHER CREW TOOK THE MARK");
}

void Game::win() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    banner_ = &art_.setDown;
    hold_ = kHoldNeed;
    std::snprintf(report_, sizeof report_,
                  "S3 BARGE MARK  SET DOWN  on the mark before the other crew  (%.1f s)", race_);
    chime(5);
    shake_ = 0.4f;
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    banner_ = &art_.lost;
    std::snprintf(why_, sizeof why_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 BARGE MARK  FAIL  %s  x %.1f  y %.2f  spd %.2f  (%.1f s)", why_, x_,
                  y_, vel_, race_);
    blip(90.f);
    shake_ = 1.2f;
}

void Game::blip(float freq) {
    tone0_ = freq;
    thumpT_ = 0.12f;
}

void Game::chime(int notes) {
    chimeN_ = notes;
    chimeStep_ = 0;
    chimeT_ = 0.f;
}

void Game::audio() {
    if (thumpT_ > 0.f) {
        sys_->apu.tone(0, tone0_, 0.12f);
        thumpT_ -= kDt;
    } else {
        sys_->apu.tone(0, 0, 0);
    }
    if (chimeN_ > 0) {
        chimeT_ -= kDt;
        if (chimeT_ <= 0.f) {
            float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f};
            sys_->apu.tone(1, notes[chimeStep_ % 5], 0.14f);
            chimeStep_++;
            chimeT_ = 0.16f;
            if (chimeStep_ >= chimeN_) chimeN_ = 0;
        }
    } else {
        float hum = mode_ == Mode::Run ? 70.f + std::fabs(thrustIn_) * 40.f : 0.f;
        sys_->apu.tone(1, hum, mode_ == Mode::Run ? 0.04f : 0.f);
    }
    float wash = mode_ == Mode::Run ? std::fabs(vel_) * 0.04f : 0.f;
    sys_->apu.noise(wash, 0.4f, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Run) {
        race_ += kDt;
        float thrust = 0, steer = 0;
        if (bot_) pilot(thrust, steer);
        else controls(thrust, steer);
        thrustIn_ = thrust;
        steerIn_ = steer;
        physics(thrust, steer);
        if (std::fabs(vel_) > 0.3f) {
            wakeT_ -= kDt;
            if (wakeT_ <= 0.f) {
                wake_[wakeCursor_].x = x_ - (vel_ > 0 ? 4.f : -4.f);
                wake_[wakeCursor_].life = 1.f;
                wake_[wakeCursor_].rise = 0;
                wakeCursor_ = (wakeCursor_ + 1) % 10;
                wakeT_ = 0.12f;
            }
        }
        smokeT_ -= kDt;
        if (smokeT_ <= 0.f && std::fabs(thrust) > 0.2f) {
            smoke_[smokeCursor_].x = x_ - 3.2f;
            smoke_[smokeCursor_].life = 1.f;
            smoke_[smokeCursor_].rise = 0;
            smokeCursor_ = (smokeCursor_ + 1) % 5;
            smokeT_ = 0.18f;
        }
        for (Puff& p : wake_) p.life -= kDt * 0.7f;
        for (Puff& p : smoke_) {
            p.life -= kDt * 0.45f;
            p.rise += kDt * 7.f;
        }
        judge();
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            banner_ = &art_.paused;
        }
    } else if (mode_ == Mode::Pause) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        if (!bot_ && pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) startRun();
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }

    float left = std::max(0.f, limit_ - (mode_ == Mode::Run ? race_ : 0.f));
    if (mode_ == Mode::Win) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 40, 28);
    else if (mode_ == Mode::Run && left < 10.f) sys.setLight(170, 90, 30);
    else if (mode_ == Mode::Run && onPaint()) sys.setLight(40, 140, 80);
    else sys.setLight(30, 70, 120);

    float want = mode_ == Mode::Title ? 30.f : x_;
    camX_ += (want - camX_) * (1.f - std::exp(-kDt * 3.f));
    if (shake_ > 0.f) {
        camX_ += std::sin(t_ * 40.f) * shake_ * 0.3f;
        shake_ = std::max(0.f, shake_ - kDt * 1.4f);
    }
    audio();
    draw();
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

void Game::drawHud() {
    char buf[72];
    if (mode_ == Mode::Title) {
        hudC(21, "TAKE THE BARGE", PAL_BANNER);
        hudC(22, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(23, "THE CLOCK IS THE OTHER CREW", PAL_ALERT);
        hudC(25, "UP AHEAD  DOWN ASTERN", PAL_HUD);
        hudC(26, "LEFT AND RIGHT SET THE BEAM", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "RETURN", PAL_WIN);
        return;
    }
    float left = std::max(0.f, limit_ - race_);
    int sec = int(left);
    int frac = int((left - sec) * 10.f);
    std::snprintf(buf, sizeof buf, "CREW %02d.%d", sec, frac);
    hud(1, 0, "S3 BARGE MARK", PAL_BANNER);
    hud(28, 0, buf, left < 12.f ? PAL_ALERT : PAL_CLOCK);
    if (mode_ == Mode::Pause) {
        hudC(17, "RETURN CONTINUES", PAL_HUD);
        hudC(18, "MODE TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "SET DOWN ON THE MARK", PAL_WIN);
        hudC(17, "BEFORE THE OTHER CREW", PAL_HUD);
        if (!bot_) hudC(19, "RETURN TAKES HER AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, why_[0] ? why_ : "MISSED", PAL_ALERT);
        if (!bot_) hudC(18, "RETURN TRIES AGAIN", PAL_HUD);
        return;
    }
    std::snprintf(buf, sizeof buf, "SPD %+05.2f", vel_);
    hud(1, 1, buf, PAL_HUD);
    const char* line = "MAKE THE MARK";
    int pal = PAL_HUD;
    if (hold_ > 0.02f) {
        int n = std::max(1, std::min(5, int(hold_ / kHoldNeed * 5.f + 0.001f)));
        char pips[8];
        for (int i = 0; i < 5; i++) pips[i] = i < n ? '#' : '-';
        pips[5] = 0;
        std::snprintf(buf, sizeof buf, "SET %s", pips);
        line = buf;
        pal = PAL_WIN;
    } else if (onPaint()) {
        line = "ON THE PAINT  STOP";
        pal = PAL_WIN;
    } else if (x_ > kMarkX + kAlong) {
        line = "BACK HER DOWN";
        pal = PAL_BANNER;
    } else if (std::fabs(y_) > kAcross && std::fabs(x_ - kMarkX) < 6.f) {
        line = "SET THE BEAM";
        pal = PAL_ALERT;
    } else if (left < 12.f) {
        line = "THE OTHER CREW IS DUE";
        pal = PAL_ALERT;
    }
    hudC(26, line, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow, bool hflip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -24 || cy + h * 0.5f < -24 || cx - w * 0.5f > gs::SCREEN_W + 24 ||
        cy - h * 0.5f > gs::SCREEN_H + 24)
        return;
    gs::Sprite s;
    long sw = std::lround(w);
    long sh = std::lround(h);
    if (sw < 1) sw = 1;
    if (sh < 1) sh = 1;
    if (sw > 1800) sw = 1800;
    if (sh > 1800) sh = 1800;
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    long sx0 = std::lround(cx - sw * 0.5f);
    long sy0 = std::lround(cy - sh * 0.5f);
    if (sx0 < -2000) sx0 = -2000;
    if (sy0 < -2000) sy0 = -2000;
    if (sx0 > 2000) sx0 = 2000;
    if (sy0 > 2000) sy0 = 2000;
    s.x = int16_t(sx0);
    s.y = int16_t(sy0);
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

float Game::sx(float wx) const { return 150.f + (wx - camX_) * kPpm; }

float Game::syLat(float wy) const { return kWaterY + 16.f + wy * 10.f; }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky;
        if (y < 96) {
            float u = y / 96.f;
            int r = int(6 + u * 6);
            int g = int(8 + u * 2);
            int b = int(12 - u * 4);
            sky = gs::rgb4(r, g, b);
        } else if (y < 118) {
            sky = gs::rgb4(4, 6, 4);
        } else {
            float u = (y - 118.f) / 90.f;
            int b = int(7 - u * 3);
            sky = gs::rgb4(1, 4, b < 3 ? 3 : b);
        }
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 86.f, 22.f, float(art_.title.h), PAL_BANNER);
        spr(art_.wordMark, 230.f, 24.f, float(art_.wordMark.h), PAL_BANNER);
    } else if (banner_ && (mode_ == Mode::Win || mode_ == Mode::Fail || mode_ == Mode::Pause)) {
        int pal = mode_ == Mode::Win ? PAL_WIN : mode_ == Mode::Fail ? PAL_ALERT : PAL_BANNER;
        spr(*banner_, 160.f, 34.f, float(banner_->h), pal);
    }

    float bob = std::sin(t_ * 2.1f) * 1.0f;
    float by = kWaterY - 6.f + bob + y_ * 5.f;
    float bargeH = 36.f;
    spr(art_.barge, sx(x_) + 2.f, by + 5.f, bargeH, PAL_BARGE, true);
    for (const Puff& p : smoke_)
        if (p.life > 0.05f) spr(art_.smoke, sx(p.x), by - 18.f - p.rise, 7.f + p.life * 6.f, PAL_FOAM);
    spr(art_.barge, sx(x_), by, bargeH, PAL_BARGE, false, vel_ < -0.15f);

    for (const Puff& p : wake_)
        if (p.life > 0.05f) spr(art_.wake, sx(p.x), kWaterY + 12.f, 6.f + p.life * 5.f, PAL_FOAM);

    int markPal = (hold_ > 0.02f || mode_ == Mode::Win) ? PAL_WIN : PAL_MARK;
    spr(art_.ring, sx(kMarkX), syLat(0.f), 14.f, markPal);
    spr(art_.cross, sx(kMarkX), syLat(0.f) - 2.f, 22.f, markPal);
    spr(art_.post, sx(kMarkX - 3.2f), kWaterY - 8.f, 48.f, PAL_POST);
    spr(art_.post, sx(kMarkX + 3.2f), kWaterY - 8.f, 48.f, PAL_POST);
    spr(art_.lamp, sx(kMarkX - 6.f), 98.f, 30.f, PAL_CLOCK);
    spr(art_.lamp, sx(kMarkX + 6.f), 98.f, 30.f, PAL_CLOCK);

    float left = mode_ == Mode::Title ? limit_ : std::max(0.f, limit_ - race_);
    float u = 1.f - clampf(left / limit_, 0.f, 1.f);
    int hand = int(u * 8.f);
    if (hand > 7) hand = 7;
    float quay = kMarkX + 9.f;
    spr(art_.shed, sx(quay), 84.f, 50.f, PAL_SHED);
    spr(art_.clock[hand], sx(quay), 60.f, 28.f, PAL_CLOCK);
    bool pace = mode_ == Mode::Run && left < 16.f && (int(t_ * 3.f) & 1);
    spr(art_.crew[pace ? 1 : 0], sx(quay - 8.f), 100.f, 30.f, PAL_CREW);

    for (int i = -1; i < 9; i++) {
        float bx = std::floor(camX_ / 16.f) * 16.f + i * 16.f;
        spr(art_.bank, sx(bx), 108.f, 24.f, PAL_BANK);
        if ((i & 1) == 0) spr(art_.reed, sx(bx + 5.f), 116.f, 14.f, PAL_TREE);
    }
    spr(art_.tree, sx(18.f), 78.f, 40.f, PAL_TREE);
    spr(art_.tree, sx(kMarkX + 16.f), 72.f, 46.f, PAL_TREE);

    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(12.f + t_ * 9.f + i * 100.f, 360.f) - 20.f;
        bool up = std::sin(t_ * 6.f + i) > 0.f;
        spr(art_.bird[up ? 0 : 1], gx, 26.f + i * 9.f, 10.f, PAL_BIRD);
    }

    drawHud();
}

}  // namespace bargemark
