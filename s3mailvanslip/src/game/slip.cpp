#include "slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mailvanslip {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;

constexpr float kStartX = -28.f;
constexpr float kStartY = 14.2f;
constexpr float kLaneY = 15.f;

// Slip opens west. Piers sit north and south of the water; the head wall is the end.
constexpr float kMouth = 16.f;
constexpr float kHead = 58.f;
constexpr float kSouth0 = 6.f;
constexpr float kSouth1 = 11.f;
constexpr float kNorth0 = 19.f;
constexpr float kNorth1 = 24.f;
constexpr float kPier0 = 14.f;
constexpr float kPier1 = 66.f;

constexpr float kBerthX0 = 46.f;
constexpr float kBerthX1 = 53.5f;
constexpr float kBerthY0 = 12.4f;
constexpr float kBerthY1 = 17.6f;

constexpr float kNose = 3.4f;
constexpr float kTail = 2.8f;
constexpr float kBeam = 1.35f;

constexpr float kTide = 48.f;
constexpr float kHold = 0.4f;
constexpr float kStop = 0.32f;

struct Box {
    float x0, x1, y0, y1;
};

const Box kSolid[] = {
    {kPier0, kPier1, kSouth0, kSouth1},
    {kPier0, kPier1, kNorth0, kNorth1},
    {kHead, kPier1, kSouth0, kNorth1},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

bool inside(const Box& b, float x, float y) { return x >= b.x0 && x <= b.x1 && y >= b.y0 && y <= b.y1; }

bool solidAt(float x, float y) {
    for (const Box& b : kSolid)
        if (inside(b, x, y)) return true;
    return false;
}

}  // namespace

float Game::tideLeft() const { return std::max(0.f, kTide - race_); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (inBerth_ && std::fabs(spd_) < 1.1f) return 3;
    if (inSlip_) return 2;
    return 1;
}

void Game::begin() {
    x_ = kStartX;
    y_ = kStartY;
    hdg_ = 0.18f;
    spd_ = 0.f;
    thr_ = 0.f;
    race_ = 0.f;
    hold_ = 0.f;
    tick_ = 0.f;
    inSlip_ = false;
    inBerth_ = false;
    won_ = false;
    over_ = false;
    chime_ = -1;
    std::snprintf(why_, sizeof why_, "running");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.08f, 0.12f, 0.05f);
    begin();
    if (bot_) mode_ = Mode::Run;
    else mode_ = Mode::Title;
    cam_ = bot_ ? x_ : 18.f;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    spd_ = 0.f;
    thr_ = 0.f;
    std::snprintf(why_, sizeof why_, "%s", why);
    sys_->apu.noiseBurst(0.36f, 90.f, 0.35f);
    sys_->apu.tone(0, 70.f, 0.06f);
    tone_ = 0.35f;
    sys_->rumble(0.5f, 0.2f, 150);
    sys_->setLight(170, 30, 20);
}

void Game::win() {
    if (won_) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    spd_ = 0.f;
    thr_ = 0.f;
    std::snprintf(why_, sizeof why_, "berthed");
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.22f, 0.4f, 120);
    sys_->setLight(40, 160, 70);
}

void Game::human(float& steer, float& thr) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer += 1.f;
    if (p.down(gs::BTN_RIGHT)) steer -= 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(-p.axisX, -1.f, 1.f);
    const bool go = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.axisY > 0.25f || p.accel > 0.2f;
    const bool stop = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.axisY < -0.25f || p.brake > 0.2f;
    if (stop) thr_ = std::max(-1.f, thr_ - kDt * 2.f);
    else if (go) thr_ = std::min(1.f, thr_ + kDt * 1.15f);
    else {
        float decay = std::fabs(spd_) < 0.35f ? 2.8f : 0.5f;
        if (thr_ > 0.f) thr_ = std::max(0.f, thr_ - kDt * decay);
        else thr_ = std::min(0.f, thr_ + kDt * decay);
    }
    thr = thr_;
}

void Game::pilot(float& steer, float& thr) {
    float yErr = kLaneY - y_;
    float want;
    float th;
    if (x_ < kMouth - 2.f) {
        want = std::atan2(kLaneY - y_, (kMouth + 4.f) - x_);
        th = 0.72f;
    } else if (x_ < kBerthX0 - 1.f) {
        want = std::clamp(yErr * 0.22f, -0.28f, 0.28f);
        th = x_ < kMouth + 6.f ? 0.42f : 0.28f;
    } else {
        want = std::clamp(yErr * 0.35f, -0.16f, 0.16f);
        th = 0.22f;
    }
    float err = wrap(want - hdg_);
    steer = std::clamp(err / 0.28f, -1.f, 1.f);
    if (std::fabs(err) > 0.7f) th *= 0.15f;

    if (x_ > kBerthX0 + 1.4f && x_ < kHead - 1.f && std::fabs(yErr) < 3.2f) {
        if (spd_ > 0.18f) thr = -1.f;
        else if (x_ < (kBerthX0 + kBerthX1) * 0.5f && spd_ < 0.4f) thr = 0.32f;
        else if (spd_ < -0.06f) thr = 0.22f;
        else thr = 0.f;
        if (x_ > kBerthX1 - 0.8f) thr = spd_ > -0.05f ? -0.85f : 0.f;
        return;
    }
    float cap = x_ > kMouth ? 2.4f : 6.2f;
    if (spd_ > cap) th = -0.6f;
    thr = th;
}

void Game::step(float steer, float thr) {
    race_ += kDt;
    if (race_ >= kTide) {
        fail("tide turned");
        return;
    }

    float rate = 1.7f + std::min(std::fabs(spd_), 7.f) * 0.03f;
    hdg_ = wrap(hdg_ + steer * rate * kDt);

    float cap = (x_ > kMouth && y_ > kSouth1 && y_ < kNorth0) ? 3.2f : 7.2f;
    if (thr < -0.02f && spd_ > 0.f) {
        spd_ -= (-thr) * 7.2f * kDt;
        if (spd_ < 0.f) spd_ = std::max(spd_, thr * 2.f);
    } else {
        float target = thr >= 0.f ? thr * cap : thr * 2.4f;
        spd_ += (target - spd_) * (1.f - std::exp(-1.9f * kDt));
    }
    if (std::fabs(thr) < 0.04f && std::fabs(spd_) < 0.65f) spd_ *= std::exp(-6.f * kDt);
    spd_ = std::clamp(spd_, -2.4f, 8.f);

    float c = std::cos(hdg_), s = std::sin(hdg_);
    x_ += c * spd_ * kDt;
    y_ += s * spd_ * kDt;

    float u = std::clamp(race_ / kTide, 0.f, 1.f);
    float ebb = 0.15f + 1.6f * u * u;
    bool sheltered = x_ > kMouth + 1.f && x_ < kHead && y_ > kSouth1 && y_ < kNorth0;
    y_ -= (sheltered ? ebb * 0.05f : ebb) * kDt;

    auto sample = [&](float along, float beam, float& px, float& py) {
        px = x_ + c * along - s * beam;
        py = y_ + s * along + c * beam;
    };
    float pts[5][2];
    sample(kNose, 0.f, pts[0][0], pts[0][1]);
    sample(-kTail, 0.f, pts[1][0], pts[1][1]);
    sample(0.2f, -kBeam, pts[2][0], pts[2][1]);
    sample(0.2f, kBeam, pts[3][0], pts[3][1]);
    sample(-1.f, 0.f, pts[4][0], pts[4][1]);

    float hit = 0.f;
    bool head = false;
    for (int i = 0; i < 5; i++) {
        float px = pts[i][0], py = pts[i][1];
        if (!solidAt(px, py)) continue;
        if (px >= kHead - 0.05f && py > kSouth1 - 0.2f && py < kNorth0 + 0.2f) head = true;
        float best = 1e9f, ox = 0.f, oy = 0.f;
        for (const Box& b : kSolid) {
            if (!inside(b, px, py)) continue;
            float dl = px - b.x0, dr = b.x1 - px, db = py - b.y0, dtb = b.y1 - py;
            float m = dl, vx = -1.f, vy = 0.f;
            if (dr < m) {
                m = dr;
                vx = 1.f;
                vy = 0.f;
            }
            if (db < m) {
                m = db;
                vx = 0.f;
                vy = -1.f;
            }
            if (dtb < m) {
                m = dtb;
                vx = 0.f;
                vy = 1.f;
            }
            if (m < best) {
                best = m;
                ox = vx * (m + 0.08f);
                oy = vy * (m + 0.08f);
            }
        }
        x_ += ox;
        y_ += oy;
        hit = std::max(hit, std::fabs(spd_));
    }
    if (head && hit > 0.7f) {
        fail("missed the end");
        return;
    }
    if (hit > 2.4f) {
        fail("scraped the pier");
        return;
    }
    if (hit > 0.12f) spd_ *= 0.35f;

    x_ = std::clamp(x_, -70.f, 72.f);
    y_ = std::clamp(y_, -8.f, 48.f);

    inSlip_ = x_ > kMouth && x_ < kHead && y_ > kSouth1 + 0.15f && y_ < kNorth0 - 0.15f;
    inBerth_ = x_ > kBerthX0 && x_ < kBerthX1 && y_ > kBerthY0 && y_ < kBerthY1 && std::fabs(wrap(hdg_)) < 0.65f;
    if (inBerth_ && std::fabs(spd_) < kStop) {
        hold_ += kDt;
        if (hold_ >= kHold) win();
    } else {
        hold_ = 0.f;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Run;
            sys.apu.tone(1, 520.f, 0.05f);
            tone_ = 0.08f;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float steer = 0.f, thr = 0.f;
            if (bot_) pilot(steer, thr);
            else human(steer, thr);
            step(steer, thr);
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        begin();
        mode_ = Mode::Title;
        over_ = false;
    }

    float want = (mode_ == Mode::Title) ? 22.f : x_;
    cam_ += (want - cam_) * (1.f - std::exp(-kDt * 3.2f));

    if (mode_ == Mode::Run && tideLeft() < 12.f) {
        tick_ -= kDt;
        if (tick_ <= 0.f) {
            sys.apu.tone(1, tideLeft() < 4.f ? 840.f : 460.f, 0.04f);
            tone_ = 0.07f;
            tick_ = tideLeft() < 4.f ? 0.22f : 0.5f;
        }
        sys.setLight(170, 70, 20);
    } else if (mode_ == Mode::Run) {
        sys.setLight(30, 90, 50);
    }

    if (tone_ > 0.f) {
        tone_ -= kDt;
        if (tone_ <= 0.f) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    if (chime_ >= 0) {
        chimeT_ += kDt;
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
        int stepN = int(chimeT_ / 0.14f);
        if (stepN != chime_ && stepN < 4) {
            chime_ = stepN;
            sys.apu.tone(2, notes[stepN], 0.12f);
        }
        if (stepN >= 6) {
            sys.apu.tone(2, 0, 0);
            chime_ = -1;
        }
    } else if (mode_ == Mode::Run && (thr_ > 0.08f || std::fabs(spd_) > 1.2f)) {
        float wob = 0.8f + 0.2f * std::sin(t_ * 14.f);
        sys.apu.tone(2, 55.f + std::max(0.f, thr_) * 30.f, 0.018f * wob);
    } else {
        sys.apu.tone(2, 0, 0);
    }
    float wash = mode_ == Mode::Run ? 0.008f + std::fabs(spd_) * 0.001f : 0.004f;
    sys.apu.noise(inSlip_ ? wash * 0.4f : wash, 420.f, false);

    draw();
}

void Game::spr(const gs::Mipped& m, float wx, float wy, float h, int pal) {
    if (m.h < 1 || h < 1.f) return;
    float w = h * float(m.w) / float(m.h);
    float sx = (wx - cam_) * 4.2f + 150.f;
    float sy = 150.f - wy * 4.2f;
    if (sx + w * 0.5f < -20.f || sx - w * 0.5f > gs::SCREEN_W + 20.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 400L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 400L));
    s.x = int16_t(std::lround(sx - s.w * 0.5f));
    s.y = int16_t(std::lround(sy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float tide = mode_ == Mode::Title ? 0.08f : std::clamp(race_ / kTide, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = 6 + (y < 36 ? 2 : 0);
        int r = sky - int(tide * 3.f);
        int g = sky + 1 - int(tide * 2.f);
        int b = sky + 3 - int(tide * 4.f);
        vdp.lineBackdrop[y] = gs::rgb4(std::clamp(r, 1, 15), std::clamp(g, 1, 15), std::clamp(b, 1, 15));
        vdp.lineFog[y] = uint8_t(tide > 0.7f ? int((tide - 0.7f) * 12.f) : 0);
        vdp.road[y].on = false;
    }

    for (float x = std::floor((cam_ - 50.f) / 8.f) * 8.f; x < cam_ + 50.f; x += 8.f) {
        spr(art_.water, x + 4.f, 4.f, 34.f, PAL_WATER);
        spr(art_.water, x + 4.f, 26.f, 34.f, PAL_WATER);
        spr(art_.quay, x + 4.f, -2.f, 22.f, PAL_QUAY);
        spr(art_.quay, x + 4.f, 32.f, 22.f, PAL_QUAY);
    }
    for (float x = kPier0; x < kPier1; x += 11.f) {
        spr(art_.pier, x + 5.f, (kSouth0 + kSouth1) * 0.5f, 22.f, PAL_PIER);
        spr(art_.pier, x + 5.f, (kNorth0 + kNorth1) * 0.5f, 22.f, PAL_PIER);
    }
    spr(art_.pier, (kHead + kPier1) * 0.5f, kLaneY, 36.f, PAL_PIER);
    spr(art_.post, kMouth - 1.f, kSouth1 - 0.4f, 28.f, PAL_POST);
    spr(art_.post, kMouth - 1.f, kNorth0 + 0.4f, 28.f, PAL_POST);
    spr(art_.lamp, kBerthX0, kNorth1 + 1.2f, 22.f, PAL_POST);
    spr(art_.lamp, kBerthX1, kSouth0 - 1.2f, 22.f, PAL_POST);
    spr(art_.sack, (kBerthX0 + kBerthX1) * 0.5f, kNorth0 - 0.6f, 12.f, PAL_SACK);
    spr(art_.sack, kBerthX1 - 1.f, kSouth1 + 0.8f, 12.f, PAL_SACK);
    spr(art_.buoy, kMouth - 8.f, 8.f, 16.f, PAL_ALERT);
    spr(art_.buoy, -8.f, 22.f, 16.f, PAL_GOLD);

    float vh = 26.f;
    float vw = vh * float(art_.van.w) / float(art_.van.h);
    float sx = (x_ - cam_) * 4.2f + 150.f;
    float sy = 150.f - y_ * 4.2f;
    gs::Sprite van;
    van.w = int16_t(vw);
    van.h = int16_t(vh);
    van.x = int16_t(sx - vw * 0.5f);
    van.y = int16_t(sy - vh * 0.5f);
    van.img = art_.van.pick(vh);
    van.pal = PAL_VAN;
    van.hflip = std::cos(hdg_) < 0.f;
    vdp.sprite(van);

    if (mode_ == Mode::Title) {
        spr(art_.title, cam_ + 2.f, 20.f, 22.f, PAL_GOLD);
        hudC(16, "BERTH IN THE SLIP", PAL_HUD);
        hudC(18, "BEFORE THE TIDE TURNS", PAL_HUD);
        hudC(22, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        spr(art_.berthed, x_, y_ + 6.f, 20.f, PAL_WIN);
        hudC(24, "BERTHED BEFORE THE TIDE", PAL_WIN);
    } else if (mode_ == Mode::Fail) {
        const gs::Mipped& banner = std::strcmp(why_, "tide turned") == 0 ? art_.tide : art_.missed;
        spr(banner, x_, y_ + 6.f, 20.f, PAL_ALERT);
        hudC(24, why_, PAL_ALERT);
    } else {
        const char* hint = "LINE THE VAN ON THE SLIP";
        if (inBerth_ && std::fabs(spd_) > 0.5f) hint = "EASE OFF AND HOLD";
        else if (inBerth_) hint = "HOLD THE BERTH";
        else if (inSlip_) hint = "THE END OF THE SLIP IS AHEAD";
        else if (race_ > kTide * 0.55f) hint = "THE TIDE IS TURNING";
        hudC(25, hint, PAL_HUD);
        hud(1, 26, "ARROWS  AHEAD AND ACROSS", PAL_HUD);
    }

    hud(1, 0, "S3 MAILVAN SLIP", PAL_GOLD);
    char buf[24];
    if (mode_ == Mode::Title) std::snprintf(buf, sizeof buf, "TIDE");
    else std::snprintf(buf, sizeof buf, "TIDE %4.1f", tideLeft());
    hud(28, 0, buf, tideLeft() < 12.f && mode_ == Mode::Run ? PAL_ALERT : PAL_HUD);
}

}  // namespace mailvanslip
