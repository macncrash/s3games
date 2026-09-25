#include "game/keel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace keel {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kUp = 1.5707963f;
constexpr float kMaxSpeed = 30.f;
constexpr float kPlayZoom = 1.85f;
constexpr float kTitleZoom = 0.36f;
constexpr float kTitleCamX = 10.f;
constexpr float kTitleCamY = 214.f;
constexpr float kStartX = 0.f;
constexpr float kStartY = 36.f;
constexpr float kStartH = 0.62f;
constexpr float kMarkRadius = 30.f;
constexpr float kBerthX = 16.f;
constexpr float kBerthY0 = 6.f;
constexpr float kBerthY1 = 20.f;
constexpr float kBerthSpeed = 12.f;

struct Mark {
    float x, y;
    const char* name;
};

struct Way {
    float x, y;
};

const Mark kMarks[3] = {
    {0.f, 400.f, "1 WINDWARD"},
    {260.f, 210.f, "2 WING"},
    {-170.f, 120.f, "3 LEEWARD"},
};

// Tack points, then the three buoys, a gate, and the berth.
const Way kWay[] = {
    {170.f, 145.f}, {-160.f, 280.f}, {55.f, 365.f}, {0.f, 400.f}, {260.f, 210.f}, {-170.f, 120.f}, {0.f, 72.f}, {0.f, 13.f},
};
constexpr int kWayLast = 7;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float windAngle(float heading) { return std::fabs(wrap(heading - kUp)); }

float polar(float ang) {
    if (ang < 0.50f) return 0.04f;
    if (ang < 1.15f) {
        float t = (ang - 0.50f) / (1.15f - 0.50f);
        return 0.58f + 0.32f * t;
    }
    if (ang < 2.00f) return 1.f;
    if (ang < 2.55f) return 0.88f;
    return 0.68f;
}

const char* sailName(float heading) {
    float a = windAngle(heading);
    if (a < 0.50f) return "IN IRONS";
    if (a < 1.10f) return "CLOSE HAULED";
    if (a < 1.80f) return "BEAM REACH";
    if (a < 2.45f) return "BROAD REACH";
    return "RUNNING";
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

}  // namespace

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    heading_ = kStartH;
    speed_ = 8.f;
    yaw_ = 0;
    leg_ = 0;
    wp_ = 0;
    tack_ = -1;
    tackTime_ = 10.f;
    legTime_ = 0;
    raceTime_ = 0;
    wakeT_ = 0;
    stuckT_ = 0;
    stuckX_ = x_;
    stuckY_ = y_;
    brake_ = false;
    won_ = false;
    over_ = false;
    chime_ = 0;
    wakes_.clear();
}

void Game::showTitle() {
    begin();
    mode_ = Mode::Title;
    zoom_ = kTitleZoom;
    camX_ = kTitleCamX;
    camY_ = kTitleCamY;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(1, 4, 9));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.22f, 0.14f);
    begin();
    if (bot_) {
        mode_ = Mode::Sail;
        zoom_ = kPlayZoom;
        camX_ = x_;
        camY_ = y_;
    } else {
        showTitle();
    }
}

void Game::controls(float& steer, float& trim) {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    trim = 0.82f;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO)) trim = 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) trim = 0.18f;
    brake_ = false;
}

void Game::pilot(float& steer, float& trim) {
    int wp = std::clamp(wp_, 0, kWayLast);
    float tx = kWay[wp].x;
    float ty = kWay[wp].y;
    float dist = std::hypot(tx - x_, ty - y_);
    float desired = std::atan2(ty - y_, tx - x_);
    float off = std::fabs(wrap(desired - kUp));
    float aim = desired;
    if (off < 0.52f) {
        float side = wrap(desired - kUp);
        int want = side >= 0.f ? 1 : -1;
        if (want != tack_ && tackTime_ > 0.9f) {
            tack_ = want;
            tackTime_ = 0;
        }
        aim = kUp + float(tack_) * 0.86f;
    }
    trim = leg_ >= 3 ? 0.40f : 1.f;
    brake_ = false;
    if (wp == kWayLast) {
        trim = 0.32f;
        if (dist < 24.f) {
            trim = 0.06f;
            brake_ = true;
            if (dist < 7.f) aim = heading_;
        }
    }
    float err = wrap(aim - heading_);
    steer = std::clamp(err / 0.42f, -1.f, 1.f);
}

void Game::physics(float dt, float steer, float trim) {
    tackTime_ += dt;
    legTime_ += dt;
    float rate = 1.35f + std::min(speed_, 18.f) * 0.02f;
    yaw_ += (steer * rate - yaw_) * std::min(1.f, dt * 5.f);
    yaw_ = std::clamp(yaw_, -2.2f, 2.2f);
    heading_ = wrap(heading_ + yaw_ * dt);

    float gust = 1.f + 0.04f * std::sin(t_ * 0.7f) * std::sin(t_ * 0.17f + 1.3f);
    float target = kMaxSpeed * polar(windAngle(heading_)) * std::clamp(trim, 0.f, 1.f) * gust;
    float ak = target > speed_ ? 1.6f : 2.4f;
    speed_ += (target - speed_) * (1.f - std::exp(-ak * dt));
    if (brake_) speed_ *= std::exp(-3.2f * dt);
    speed_ = std::clamp(speed_, 0.f, 34.f);

    float c = std::cos(heading_), s = std::sin(heading_);
    x_ += c * speed_ * dt;
    y_ += s * speed_ * dt;
    y_ -= 1.05f * dt;
    float lee = c * speed_ * 0.08f;
    x_ += -s * lee * dt;
    y_ += c * lee * dt;

    bool hit = false;
    if (y_ < 0.f) {
        if (leg_ >= 3 && std::fabs(x_) < kBerthX + 4.f && speed_ <= kBerthSpeed) {
            y_ = (kBerthY0 + kBerthY1) * 0.5f;
            x_ = std::clamp(x_, -kBerthX + 2.f, kBerthX - 2.f);
            speed_ = std::min(speed_, 3.f);
        } else {
            y_ = 0.8f;
            if (s < 0.f) heading_ = std::atan2(std::fabs(s), c);
            speed_ *= 0.55f;
            yaw_ = 0;
            hit = true;
        }
    }
    if (x_ < -380.f) {
        x_ = -376.f;
        speed_ *= 0.4f;
        yaw_ = 0;
        hit = true;
    } else if (x_ > 440.f) {
        x_ = 436.f;
        speed_ *= 0.4f;
        yaw_ = 0;
        hit = true;
    }
    if (y_ > 530.f) {
        y_ = 524.f;
        speed_ *= 0.4f;
        yaw_ = 0;
        hit = true;
    }
    auto rock = [&](float rx, float ry, float rad) {
        float dx = x_ - rx, dy = y_ - ry;
        float d = std::hypot(dx, dy);
        if (d < rad && d > 0.01f) {
            x_ = rx + dx / d * (rad + 0.5f);
            y_ = ry + dy / d * (rad + 0.5f);
            speed_ *= 0.35f;
            yaw_ = 0;
            hit = true;
        }
    };
    rock(-300.f, 360.f, 22.f);
    rock(330.f, 70.f, 16.f);
    if (hit && thumpT_ <= 0.f) {
        sys_->apu.noiseBurst(0.32f, 420.f, 0.14f);
        thumpT_ = 0.4f;
    }

    wakeT_ -= dt;
    if (wakeT_ <= 0.f && speed_ > 5.f) {
        wakeT_ = 0.07f;
        Wake w;
        w.x = x_ - c * 7.f;
        w.y = y_ - s * 7.f;
        w.life = 1.f;
        wakes_.push_back(w);
        if (wakes_.size() > 22) wakes_.erase(wakes_.begin());
    }
    for (Wake& w : wakes_) w.life -= dt;
    wakes_.erase(std::remove_if(wakes_.begin(), wakes_.end(), [](const Wake& w) { return w.life <= 0.f; }), wakes_.end());

    if (bot_) {
        stuckT_ += dt;
        if (stuckT_ > 3.f) {
            float moved = std::hypot(x_ - stuckX_, y_ - stuckY_);
            stuckX_ = x_;
            stuckY_ = y_;
            stuckT_ = 0;
            if (moved < 6.f) {
                heading_ = wrap(heading_ + 0.9f);
                yaw_ = 0;
                tack_ = -tack_;
                tackTime_ = 0;
                bool mark = wp_ >= 3 && wp_ <= 5;
                if (!mark && wp_ < kWayLast) wp_++;
            }
        }
    }
}

void Game::scoreMarks() {
    if (leg_ >= 3) return;
    float d = std::hypot(x_ - kMarks[leg_].x, y_ - kMarks[leg_].y);
    if (d < kMarkRadius) {
        leg_++;
        legTime_ = 0;
        chime(leg_);
    }
}

void Game::guide() {
    if (!bot_ || wp_ >= kWayLast) return;
    float reach = (wp_ >= 3 && wp_ <= 5) ? 16.f : 34.f;
    if (std::hypot(x_ - kWay[wp_].x, y_ - kWay[wp_].y) < reach) wp_++;
}

bool Game::inSlip() const {
    return std::fabs(x_) <= kBerthX && y_ >= kBerthY0 && y_ <= kBerthY1 && speed_ <= kBerthSpeed;
}

void Game::berth() {
    if (mode_ != Mode::Sail || leg_ < 3 || !inSlip()) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0;
    yaw_ = 0;
    chime(5);
    std::printf("S3 KEEL  PASS  sailed the triangle and docked (%.1fs)\n", raceTime_);
    std::fflush(stdout);
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.05f);
    tone1_ = 0.08f;
}

void Game::chime(int notes) {
    chime_ = std::clamp(notes, 1, 5);
    chimeStep_ = 0;
    chimeT_ = 0;
}

void Game::audio(float dt) {
    float wind = mode_ == Mode::Sail ? 0.026f + speed_ * 0.0007f : 0.012f;
    sys_->apu.noise(wind, 680.f + speed_ * 28.f, false);
    if (tone0_ > 0.f) {
        tone0_ -= dt;
        if (tone0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (tone1_ > 0.f) {
        tone1_ -= dt;
        if (tone1_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (thumpT_ > 0.f) thumpT_ -= dt;
    if (chime_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f, 1318.5f};
            int n = std::min(chimeStep_, 4);
            sys_->apu.tone(0, notes[n], 0.05f);
            tone0_ = 0.12f;
            chimeT_ = 0.14f;
            if (++chimeStep_ >= chime_) chime_ = 0;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Sail;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(880.f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Sail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(440.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        } else {
            raceTime_ += kDt;
            float steer = 0, trim = 0.82f;
            if (bot_) pilot(steer, trim);
            else controls(steer, trim);
            physics(kDt, steer, trim);
            scoreMarks();
            guide();
            berth();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Sail;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (mode_ == Mode::Win) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Sail;
            zoom_ = kPlayZoom;
            camX_ = x_;
            camY_ = y_;
            blip(880.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
    }

    if (mode_ == Mode::Title) {
        camX_ = kTitleCamX;
        camY_ = kTitleCamY;
        zoom_ = kTitleZoom;
    } else {
        float lead = mode_ == Mode::Sail ? 14.f : 0.f;
        float gx = x_ + std::cos(heading_) * lead;
        float gy = y_ + std::sin(heading_) * lead;
        float k = 1.f - std::exp(-kDt * 4.f);
        camX_ += (gx - camX_) * k;
        camY_ += (gy - camY_) * k;
        zoom_ += (kPlayZoom - zoom_) * k;
    }
    audio(kDt);
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
    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "ARROWS STEER   UP SHEET   DOWN EASE", PAL_HUD);
        hudC(25, "ROUND 1  2  3  THEN THE SAME DOCK", PAL_BANNER);
        hudC(26, "NO SAILING INTO THE WIND", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "START", PAL_WIN);
        return;
    }
    hud(3, 0, "S3 KEEL", PAL_BANNER);
    hud(31, 0, "WIND N", PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(17, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        int sec = int(raceTime_);
        std::snprintf(buf, sizeof buf, "TIME %d:%02d", sec / 60, sec % 60);
        hudC(16, buf, PAL_HUD);
        if (!bot_) hudC(18, "START SAILS AGAIN", PAL_HUD);
        return;
    }
    if (leg_ < 3) std::snprintf(buf, sizeof buf, "NEXT %s", kMarks[leg_].name);
    else std::snprintf(buf, sizeof buf, "NEXT THE DOCK");
    hud(1, 1, buf, leg_ < 3 ? PAL_MARK0 + leg_ : PAL_WIN);
    float wa = windAngle(heading_);
    hud(1, 2, sailName(heading_), wa < 0.50f ? PAL_ALERT : PAL_HUD);
    int sec = int(raceTime_);
    std::snprintf(buf, sizeof buf, "SPD %02d   %d:%02d", int(std::lround(speed_)), sec / 60, sec % 60);
    hud(1, 3, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "%d/3 BUOYS", std::min(leg_, 3));
    hud(1, 26, buf, PAL_WIN);
    if (leg_ >= 3) hud(1, 27, speed_ > kBerthSpeed ? "EASE OFF TO BERTH" : "SLIP BETWEEN THE PILES", speed_ > kBerthSpeed ? PAL_ALERT : PAL_BANNER);
    else if (raceTime_ < 7.f) hud(1, 27, "UP SHEET   DOWN EASE", PAL_HUD);
}

int Game::boatFrame() const {
    float u = std::fmod(heading_, kTau);
    if (u < 0.f) u += kTau;
    int i = int(std::lround(u / kTau * 16.f)) % 16;
    if (i < 0) i += 16;
    if (i > 15) i = 0;
    return i;
}

int Game::sailSide() const { return std::cos(heading_) >= 0.f ? 1 : 0; }

void Game::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = 160.f + (wx - camX_) * zoom_;
    sy = 112.f - (wy - camY_) * zoom_;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w < -8 || cy + h < -8 || cx - w > gs::SCREEN_W + 8 || cy - h > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx) {
    float sx, sy;
    worldToScreen(wx, wy, sx, sy);
    float h = worldH * zoom_;
    if (h < minPx) h = minPx;
    spr(m, sx, sy, h, pal, false);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    const uint16_t sand = gs::rgb4(13, 11, 6);
    const uint16_t sandDeep = gs::rgb4(10, 8, 4);
    const uint16_t wet = gs::rgb4(11, 10, 6);
    const uint16_t shallow = gs::rgb4(3, 11, 13);
    const uint16_t deep = gs::rgb4(1, 4, 9);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = camY_ + (112.f - y) / std::max(zoom_, 0.05f);
        uint16_t c;
        if (wy < 0.f) c = lerpC(sandDeep, sand, std::clamp((wy + 50.f) / 50.f, 0.f, 1.f));
        else if (wy < 16.f) c = lerpC(wet, shallow, wy / 16.f);
        else c = lerpC(shallow, deep, std::clamp((wy - 16.f) / 380.f, 0.f, 1.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
        float wob = std::sin(y * 0.09f + t_ * 1.6f) * 6.f + std::sin(y * 0.03f - t_ * 0.7f) * 3.f;
        v.B.hscroll[y] = int16_t(wob + t_ * 14.f);
        v.B.vscroll[y] = int16_t(t_ * 5.f);
    }

    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, false); };
    if (mode_ == Mode::Title) banner(art_.title, 160, 18, PAL_BANNER);
    else if (mode_ == Mode::Pause) banner(art_.paused, 160, 96, PAL_BANNER);
    else if (mode_ == Mode::Win) {
        banner(art_.docked, 160, 72, PAL_WIN);
        banner(art_.closed, 160, 108, PAL_WIN);
    }

    if (mode_ == Mode::Sail) {
        float tx, ty;
        int pal;
        if (leg_ < 3) {
            tx = kMarks[leg_].x;
            ty = kMarks[leg_].y;
            pal = PAL_MARK0 + leg_;
        } else {
            tx = 0;
            ty = 13;
            pal = PAL_WIN;
        }
        float sx, sy;
        worldToScreen(tx, ty, sx, sy);
        if (sx < 18 || sx > 302 || sy < 18 || sy > 206) {
            float dx = sx - 160.f, dy = sy - 112.f;
            float k = 1.f;
            float ax = std::fabs(dx), ay = std::fabs(dy);
            if (ax > 1.f) k = std::min(k, 146.f / ax);
            if (ay > 1.f) k = std::min(k, 94.f / ay);
            spr(art_.pin, 160.f + dx * k, 112.f + dy * k, 13.f, pal, false);
        }
    }

    if (mode_ != Mode::Title) {
        auto chart = [&](float wx, float wy, float& sx, float& sy) {
            sx = 276.f + (wx - 40.f) / 8.f;
            sy = 88.f - (wy - 200.f) / 8.f;
        };
        float sx, sy;
        chart(x_, y_, sx, sy);
        spr(art_.dot, sx, sy, 5.f, PAL_BANNER, false);
        chart(0.f, 13.f, sx, sy);
        spr(art_.dot, sx, sy, 4.f, PAL_DOCK, false);
        for (int i = 0; i < 3; i++) {
            chart(kMarks[i].x, kMarks[i].y, sx, sy);
            spr(art_.dot, sx, sy, i == leg_ ? 6.f : 4.f, PAL_MARK0 + i, false);
        }
        spr(art_.panel, 276.f, 88.f, 60.f, PAL_MAP, false);
    }

    spr(art_.wind, 16, 12, 18, PAL_BANNER, false);

    float bsx, bsy;
    worldToScreen(x_, y_, bsx, bsy);
    bsy += std::sin(t_ * 2.3f) * 1.15f;
    float boatH = 36.f * zoom_;
    if (mode_ == Mode::Title) boatH = std::max(boatH, 26.f);
    const gs::Mipped& hull = art_.boat[boatFrame()][sailSide()];
    spr(hull, bsx + 3.f, bsy + 2.f, boatH, PAL_BOAT, true);
    spr(hull, bsx, bsy, boatH, PAL_BOAT, false);

    if (speed_ > 6.f) {
        float c = std::cos(heading_), s = std::sin(heading_);
        place(art_.foam, x_ + c * 12.f, y_ + s * 12.f, 5.f + speed_ * 0.08f, PAL_FOAM, 3.f);
    }
    for (const Wake& w : wakes_) {
        float h = (3.5f + (1.f - w.life) * 6.f) * (zoom_ / kPlayZoom);
        float sx, sy;
        worldToScreen(w.x, w.y, sx, sy);
        spr(art_.foam, sx, sy, std::max(2.5f, h), PAL_FOAM, false);
    }

    for (int i = 0; i < 3; i++) {
        float bh = (i == leg_ && leg_ < 3) ? 16.f : 13.f;
        float pulse = (i == leg_ && leg_ < 3) ? 1.f + 0.06f * std::sin(t_ * 4.f) : 1.f;
        place(art_.buoy[i], kMarks[i].x, kMarks[i].y, bh * pulse, PAL_MARK0 + i, mode_ == Mode::Title ? 12.f : 0.f);
    }
    if (mode_ == Mode::Title) {
        for (int i = 0; i < 3; i++) place(art_.ring, kMarks[i].x, kMarks[i].y, 60.f, PAL_MARK0 + i, 0.f);
    } else if (leg_ < 3) {
        place(art_.ring, kMarks[leg_].x, kMarks[leg_].y, 60.f, PAL_MARK0 + leg_, 0.f);
    }

    const float piles[4][2] = {{-18.f, 6.f}, {18.f, 6.f}, {-18.f, 20.f}, {18.f, 20.f}};
    for (const float* p : piles) place(art_.pile, p[0], p[1], 8.f, PAL_DOCK, mode_ == Mode::Title ? 8.f : 0.f);

    for (int i = 0; i < 3; i++) {
        float u = t_ * (0.32f + i * 0.05f) + i * 2.2f;
        float gx = 30.f + std::sin(u) * 150.f + i * 20.f;
        float gy = 230.f + std::cos(u * 0.73f) * 80.f;
        int fr = (int(t_ * 5.f + i * 3.f) & 1);
        place(art_.gull[fr], gx, gy, 7.f, PAL_GULL, mode_ == Mode::Title ? 8.f : 0.f);
    }

    float cx[5] = {0.f, kMarks[0].x, kMarks[1].x, kMarks[2].x, 0.f};
    float cy[5] = {16.f, kMarks[0].y, kMarks[1].y, kMarks[2].y, 14.f};
    for (int i = 0; i < 4; i++) {
        float dx = cx[i + 1] - cx[i], dy = cy[i + 1] - cy[i];
        float len = std::hypot(dx, dy);
        int n = std::max(1, int(len / 40.f));
        bool hot = mode_ != Mode::Title && i == std::min(leg_, 3);
        for (int s = 1; s < n; s++) {
            float u = float(s) / float(n);
            float sx, sy;
            worldToScreen(cx[i] + dx * u, cy[i] + dy * u, sx, sy);
            spr(art_.dot, sx, sy, hot ? 5.f : 3.4f, hot ? PAL_BANNER : PAL_WAVE, false);
        }
    }

    place(art_.dock, 0.f, 2.f, 48.f, PAL_DOCK, mode_ == Mode::Title ? 34.f : 0.f);
    place(art_.wind, 14.f, 10.f, 12.f, PAL_BANNER, mode_ == Mode::Title ? 12.f : 0.f);
    place(art_.rock, -300.f, 360.f, 40.f, PAL_ROCK, mode_ == Mode::Title ? 14.f : 0.f);
    place(art_.rock, 330.f, 70.f, 28.f, PAL_ROCK, mode_ == Mode::Title ? 10.f : 0.f);

    drawHud();
}

}  // namespace keel
