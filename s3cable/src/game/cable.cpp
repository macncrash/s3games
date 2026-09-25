#include "cable.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace cable {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kCable = 5.1f;
constexpr float kResponse = 4.4f;
constexpr float kMaxPull = 8.8f;
constexpr float kBrake = 8.2f;
constexpr float kPark = 0.24f;
constexpr float kPast = 3.4f;

gs::FMPatch bellPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.15f;
    p.op[0].mul = 3.5f;
    p.op[0].level = 0.45f;
    p.op[0].ar = 0.001f;
    p.op[0].dr = 0.35f;
    p.op[0].sl = 0.f;
    p.op[0].rr = 0.28f;
    p.op[1].mul = 1.f;
    p.op[1].level = 0.85f;
    p.op[1].ar = 0.001f;
    p.op[1].dr = 0.9f;
    p.op[1].sl = 0.f;
    p.op[1].rr = 0.55f;
    p.op[2].mul = 6.f;
    p.op[2].level = 0.22f;
    p.op[2].ar = 0.001f;
    p.op[2].dr = 0.2f;
    p.op[2].sl = 0.f;
    p.op[2].rr = 0.22f;
    p.op[3].mul = 2.f;
    p.op[3].level = 0.3f;
    p.op[3].ar = 0.001f;
    p.op[3].dr = 0.7f;
    p.op[3].sl = 0.f;
    p.op[3].rr = 0.4f;
    p.vol = 0.2f;
    p.echo = 0.28f;
    return p;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

const Game::Stop& Game::stop() const { return kStops[std::clamp(cleared_, 0, kStopN - 1)]; }

float Game::gradeAt(float s) const { return 1.2f + 0.0155f * std::max(0.f, s); }

int Game::marker() const {
    if (over_ || mode_ == Mode::Result) return 4;
    if (mode_ == Mode::Title) return 0;
    if (dwell_ > 0.f) return 3;
    if (cleared_ < kStopN && std::fabs(s_ - stop().at) < 2.f) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    scatter();
    gs::FMPatch bell = bellPatch();
    sys.apu.setPatch(0, bell);
    sys.apu.setPatch(1, bell);
    sys.apu.setPatch(2, bell);
    sys.apu.setEcho(0.14f, 0.28f, 0.14f);
    sys.apu.setMaster(0.86f);
    report_[0] = 0;
    banner_[0] = 0;
}

void Game::scatter() {
    props_.clear();
    uint32_t rng = 0xC0ABu;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 8) * (1.f / 16777216.f);
    };
    auto blocked = [&](float s) {
        for (const Stop& st : kStops)
            if (std::fabs(s - st.at) < 6.f) return true;
        return false;
    };
    for (float s = -2.f; s < 136.f; s += 8.2f) {
        if (!blocked(s)) {
            Prop h;
            h.s = s + rnd() * 1.6f;
            h.lateral = 38.f + rnd() * 16.f;
            h.h = 52.f + rnd() * 14.f;
            h.kind = int(rnd() * 3.f);
            h.flip = rnd() > 0.5f;
            props_.push_back(h);
        }
        Prop t;
        t.s = s + 3.4f;
        t.lateral = -30.f - rnd() * 18.f;
        t.h = 44.f + rnd() * 22.f;
        t.kind = 3;
        t.flip = false;
        props_.push_back(t);
        Prop r;
        r.s = s + 1.2f;
        r.lateral = 78.f + rnd() * 28.f;
        r.h = 26.f + rnd() * 12.f;
        r.kind = 5;
        r.flip = rnd() > 0.5f;
        props_.push_back(r);
    }
    for (float s = 6.f; s < 130.f; s += 16.f) {
        if (blocked(s)) continue;
        Prop L;
        L.s = s;
        L.lateral = -14.f;
        L.h = 42.f;
        L.kind = 4;
        L.flip = false;
        props_.push_back(L);
    }
}

void Game::begin() {
    s_ = 4.f;
    v_ = 0.f;
    cleared_ = 0;
    held_ = 0.f;
    dwell_ = 0.f;
    time_ = 0.f;
    over_ = false;
    won_ = false;
    grip_ = false;
    brake_ = false;
    gripped_ = false;
    bell_ = 0;
    made_[0] = 0;
    banner_[0] = 0;
    report_[0] = 0;
    cam_ = s_ - 0.34f * kView;
    mode_ = Mode::Run;
}

void Game::control(bool& grip, bool& brake) {
    const float mark = stop().at;
    const float tol = stop().tol;
    const float dist = mark - s_;
    if (dist <= 0.06f && dist >= -tol) {
        grip = false;
        brake = true;
        return;
    }
    if (dist < -tol) {
        grip = false;
        brake = v_ > 0.02f || v_ < -0.40f;
        return;
    }
    float vWant;
    if (dist > 8.f) vWant = 3.45f;
    else if (dist > 1.4f) vWant = std::min(3.1f, std::sqrt(dist * 2.15f));
    else vWant = std::max(0.16f, std::min(0.55f, dist * 0.42f));
    if (dist < 2.2f && v_ > vWant + 0.35f) {
        grip = false;
        brake = true;
        return;
    }
    if (v_ > vWant + 0.05f) {
        grip = false;
        brake = true;
    } else if (v_ < vWant - 0.08f) {
        grip = true;
        brake = false;
    } else {
        grip = false;
        brake = false;
    }
}

void Game::arrive() {
    std::snprintf(made_, sizeof made_, "%s", kStops[cleared_].name);
    std::snprintf(banner_, sizeof banner_, "LEVEL  %s", made_);
    cleared_++;
    held_ = 0.f;
    bell_ = 20;
    sys_->rumble(0.22f, 0.08f, 90);
    sys_->setLight(60, 170, 90);
    if (cleared_ >= kStopN) {
        win();
        return;
    }
    dwell_ = 1.25f;
}

void Game::win() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Result;
    std::snprintf(banner_, sizeof banner_, "LEVEL WITH THE MARK");
    std::snprintf(report_, sizeof report_,
                  "S3 CABLE  LEVEL  stopped with the mark at WHARF, TERRACE, and CROWN  (%.1f s)", time_);
    sys_->apu.keyOn(0, 523.25f, 0.18f);
    sys_->apu.keyOn(1, 659.25f, 0.15f);
    sys_->apu.keyOn(2, 783.99f, 0.13f);
    sys_->setLight(80, 220, 120);
    sys_->rumble(0.35f, 0.22f, 220);
}

void Game::fail(const char* why) {
    if (over_) return;
    won_ = false;
    over_ = true;
    mode_ = Mode::Result;
    std::snprintf(banner_, sizeof banner_, "%s", why);
    std::snprintf(report_, sizeof report_, "S3 CABLE  FAIL  %s", why);
    sys_->apu.noiseBurst(0.42f, 160.f, 0.22f);
    sys_->apu.keyOn(0, 146.83f, 0.16f);
    sys_->setLight(190, 30, 24);
    sys_->rumble(0.45f, 0.15f, 180);
}

void Game::physics(bool grip, bool brake) {
    const float mark = stop().at;
    const float tol = stop().tol;
    float a = -gradeAt(s_);
    if (brake && !grip) {
        if (v_ > 0.02f) a -= kBrake;
        else if (v_ < -0.02f) a += kBrake;
    } else if (grip) {
        float pull = (kCable - v_) * kResponse;
        pull = std::max(-1.5f, std::min(kMaxPull, pull));
        a += pull;
    } else {
        a -= v_ * 0.22f;
    }
    v_ += a * DT;
    v_ = std::clamp(v_, -8.f, 7.f);
    s_ += v_ * DT;
    if (s_ < 0.f) {
        s_ = 0.f;
        if (v_ < 0.f) {
            if (v_ < -0.8f) sys_->apu.noiseBurst(0.2f, 90.f, 0.08f);
            v_ = 0.f;
        }
    }
    if (brake && !grip && std::fabs(v_) < kPark) v_ = 0.f;

    if (std::fabs(s_ - mark) <= tol && std::fabs(v_) <= 0.10f && brake && !grip) {
        held_ += DT;
        if (held_ >= 0.42f) {
            arrive();
            return;
        }
    } else if (std::fabs(s_ - mark) > tol + 0.05f) {
        held_ = 0.f;
    }
    if (s_ > mark + tol + kPast) {
        char why[48];
        std::snprintf(why, sizeof why, "ran past %s", stop().name);
        fail(why);
    }
}

void Game::audio() {
    if (bell_ > 0) {
        if (bell_ == 18 || bell_ == 8) sys_->apu.keyOn(0, bell_ > 10 ? 784.f : 1046.f, 0.17f);
        bell_--;
    }
    float hum = (mode_ == Mode::Run && grip_ && dwell_ <= 0.f) ? 0.046f : 0.f;
    sys_->apu.tone(0, 76.f + std::fabs(v_) * 16.f, hum);
    sys_->apu.tone(1, 152.f + std::fabs(v_) * 16.f, hum * 0.32f);
    float squeal = (brake_ && dwell_ <= 0.f && std::fabs(v_) > 0.35f) ? std::min(0.05f, std::fabs(v_) * 0.012f) : 0.f;
    sys_->apu.noise(squeal, 2300.f, false);
}

void Game::project(float s, float lateral, float& x, float& y) const {
    const float len = std::hypot(kDx, kDy);
    const float tx = kDx / len, ty = kDy / len;
    const float nx = -ty, ny = tx;
    const float u = (s - viewCam_) / kView;
    x = kX0 + u * kDx + nx * lateral;
    y = kY0 + u * kDy + ny * lateral;
}

void Game::stamp(const gs::Mipped& m, float ax, float ay, float sx, float sy, float scale, int pal, int fog, bool flip) {
    if (m.h < 1 || scale < 0.02f) return;
    if (sx < -180.f || sy < -180.f || sx > gs::SCREEN_W + 180.f || sy > gs::SCREEN_H + 180.f) return;
    gs::Sprite s;
    s.w = int16_t(std::max(1, int(std::lround(m.w * scale))));
    s.h = int16_t(std::max(1, int(std::lround(m.h * scale))));
    s.x = int16_t(std::lround(sx - ax * scale));
    s.y = int16_t(std::lround(sy - ay * scale));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::ui(const gs::Image& img, float x, float y, int pal) {
    if (img.w < 1) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(img.w);
    s.h = int16_t(img.h);
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hudText(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::sky() {
    const uint16_t top = gs::rgb4(2, 3, 8);
    const uint16_t mid = gs::rgb4(8, 8, 14);
    const uint16_t hor = gs::rgb4(14, 10, 8);
    const uint16_t low = gs::rgb4(9, 7, 6);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / 223.f;
        uint16_t c = t < 0.45f ? lerpC(top, mid, t / 0.45f) : t < 0.72f ? lerpC(mid, hor, (t - 0.45f) / 0.27f) : lerpC(hor, low, (t - 0.72f) / 0.28f);
        sys_->vdp.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const bool title = mode_ == Mode::Title;
    float showS = s_;
    if (title) {
        showS = kStops[0].at - 3.6f;
        viewCam_ = showS - 0.74f * kView;
    } else {
        viewCam_ = cam_;
    }

    auto fogOf = [&](float s) {
        float u = (s - viewCam_) / kView;
        if (u > 0.92f) return 8;
        if (u > 0.78f) return 4;
        return 0;
    };
    auto feet = [&](const gs::Mipped& m, float s, float lateral, float h, int pal, bool flip) {
        float x, y;
        project(s, lateral, x, y);
        float sc = h / float(std::max(1, m.h));
        stamp(m, m.w * 0.5f, float(m.h - 1), x, y, sc, pal, fogOf(s), flip);
    };

    // Earlier sprites sit on top. Words and the car go in first.
    if (title) ui(art_.title, 12.f, 10.f);
    else if (mode_ == Mode::Result) {
        if (won_) ui(art_.level, 12.f, 8.f);
        else ui(art_.ran, 12.f, 8.f);
    }

    int focus = title ? 0 : (dwell_ > 0.f && cleared_ > 0 ? cleared_ - 1 : std::min(cleared_, kStopN - 1));
    {
        float x, y;
        project(showS, 0.f, x, y);
        const float len = std::hypot(kDx, kDy);
        const float tx = kDx / len, ty = kDy / len;
        x += ty * 6.f;
        y += -tx * 6.f;
        stamp(art_.car, art_.sillX, art_.sillY, x, y, 1.f, PAL_CAR, 0, false);
    }
    for (int i = 0; i < kStopN; i++) {
        if (!title && i != focus) continue;
        float x, y;
        project(kStops[i].at, 0.f, x, y);
        stamp(art_.bar, 0.f, art_.bar.h * 0.5f, x + 18.f, y, 1.f, PAL_MARK, 0, false);
        if (i != focus) continue;
        float x0, y0, x1, y1;
        project(kStops[i].at - kStops[i].tol, 0.f, x0, y0);
        project(kStops[i].at + kStops[i].tol, 0.f, x1, y1);
        stamp(art_.tick, art_.tick.w * 0.5f, art_.tick.h * 0.5f, x0, y0, 1.f, PAL_MARK, 0, false);
        stamp(art_.tick, art_.tick.w * 0.5f, art_.tick.h * 0.5f, x1, y1, 1.f, PAL_MARK, 0, false);
    }

    for (int i = 0; i < kStopN; i++) {
        float x, y;
        project(kStops[i].at, 0.f, x, y);
        bool current = i == focus;
        bool waiting = i >= cleared_ || (dwell_ > 0.f && i == cleared_ - 1) || (won_ && i == kStopN - 1) || title;
        if (waiting) {
            float walk = 0.f;
            if (dwell_ > 0.f && i == cleared_ - 1) walk = (1.25f - dwell_) * 18.f;
            if (won_ && i == kStopN - 1) walk = 16.f;
            for (int n = 0; n < 2; n++) {
                float px = x + 34.f + n * 16.f - walk;
                float py = y + 2.f;
                const gs::Mipped& body = art_.person[n];
                float sc = 30.f / float(std::max(1, body.h));
                stamp(body, body.w * 0.5f, float(body.h - 1), px, py, sc, PAL_PEOPLE, 0, n == 1);
            }
        }
        stamp(art_.plat[kStops[i].face], art_.platX, art_.platY, x, y, 1.f, PAL_LAND, current ? 0 : fogOf(kStops[i].at), false);
    }

    for (const Prop& p : props_) {
        if (p.kind == 4) feet(art_.lamp, p.s, p.lateral, p.h, PAL_LAMP, false);
    }
    {
        float s0 = viewCam_ - 1.2f;
        float s1 = viewCam_ + kView + 1.2f;
        for (float s = std::floor(s0 / 0.85f) * 0.85f; s < s1; s += 0.85f) {
            float x, y;
            project(s, 0.f, x, y);
            stamp(art_.sleeper, art_.sleepX, art_.sleepY, x, y, 0.85f, PAL_STEEL, fogOf(s), false);
        }
    }
    feet(art_.bumper, 0.f, 0.f, 30.f, PAL_STEEL, false);
    feet(art_.sheave, 128.f, -6.f, 58.f, PAL_STEEL, false);
    for (const Prop& p : props_) {
        if (p.kind >= 0 && p.kind <= 2) feet(art_.house[p.kind], p.s, p.lateral, p.h, PAL_HOUSE, p.flip);
    }
    for (const Prop& p : props_) {
        if (p.kind == 3) feet(art_.tree, p.s, p.lateral, p.h, PAL_TREE, false);
        else if (p.kind == 5) feet(art_.rock, p.s, p.lateral, p.h, PAL_ROCK, p.flip);
    }
    if (viewCam_ < 30.f) {
        stamp(art_.water, art_.water.w * 0.5f, art_.water.h * 0.5f, 48.f, 206.f, 1.f, PAL_WATER, 0, false);
        stamp(art_.water, art_.water.w * 0.5f, art_.water.h * 0.5f, 130.f, 214.f, 0.8f, PAL_WATER, 0, false);
    }
    for (int i = 0; i < 3; i++) {
        float x = 18.f + i * 36.f + std::sin(t_ * 0.7f + i) * 10.f;
        float y = 18.f + (i == 1 ? 10.f : 0.f);
        stamp(art_.bird, art_.bird.w * 0.5f, art_.bird.h * 0.5f, x, y, 1.f, PAL_HUD, 0, false);
    }

    if (title) {
        hudC(23, "STOP LEVEL WITH THE MARK", 7);
        hudC(25, "C GRIP     X BRAKE", 7);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "ENTER TO HAUL", 3);
        hudC(27, S3_VERSION_STRING, 2);
        return;
    }

    if (mode_ == Mode::Result) {
        hudC(24, banner_, won_ ? 5 : 4);
        if (!bot_) hudC(26, "ENTER", 3);
        return;
    }

    char right[24];
    if (dwell_ > 0.f) std::snprintf(right, sizeof right, "LEVEL %s", made_);
    else std::snprintf(right, sizeof right, "NEXT %s", stop().name);
    hudText(1, 0, "S3 CABLE", 7);
    hudText(1, 1, right, dwell_ > 0.f ? 5 : 3);

    const float dist = stop().at - s_;
    char mid[32];
    int midPal = 1;
    if (dwell_ > 0.f) {
        std::snprintf(mid, sizeof mid, "LEVEL");
        midPal = 5;
    } else if (s_ < 8.f && v_ < 1.f && !grip_) {
        std::snprintf(mid, sizeof mid, "HOLD C TO HAUL");
        midPal = 3;
    } else if (dist > 6.f) {
        std::snprintf(mid, sizeof mid, "MARK %d M", int(dist + 0.5f));
        midPal = 3;
    } else if (std::fabs(s_ - stop().at) <= stop().tol && std::fabs(v_) < 0.2f) {
        std::snprintf(mid, sizeof mid, "LEVEL");
        midPal = 5;
    } else {
        int cm = int(std::lround((s_ - stop().at) * 100.f));
        if (cm > 0) std::snprintf(mid, sizeof mid, "LONG %d CM", cm);
        else std::snprintf(mid, sizeof mid, "SHORT %d CM", -cm);
        midPal = cm > 0 ? 4 : 3;
    }
    hudC(25, mid, midPal);

    hudText(1, 26, "C GRIP", grip_ ? 5 : 2);
    char spd[12];
    std::snprintf(spd, sizeof spd, "%.1f", std::fabs(v_));
    hudC(26, spd, 1);
    hudText(32, 26, "X BRAKE", brake_ ? 4 : 2);

    char meter[18];
    const int N = 17;
    const int center = 8;
    float span = 1.8f;
    float err = s_ - stop().at;
    int pos = center + int(std::lround(std::clamp(err, -span, span) / span * center));
    pos = std::clamp(pos, 0, N - 1);
    for (int i = 0; i < N; i++) meter[i] = '-';
    meter[center] = '+';
    int half = int(std::lround(stop().tol / span * center));
    if (half >= 1) {
        meter[center - half] = ':';
        meter[center + half] = ':';
    }
    meter[pos] = 'O';
    meter[N] = 0;
    char row[40];
    std::snprintf(row, sizeof row, "SHORT %s LONG", meter);
    int barPal = 1;
    if (std::fabs(err) <= stop().tol && std::fabs(v_) < 0.2f) barPal = 5;
    else if (err > 0.f) barPal = 4;
    hudC(27, row, barPal);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;

    auto go = [&]() {
        return sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C) || sys.pad.pressed(gs::BTN_A);
    };
    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 0.45f) begin();
        else if (!bot_ && go()) begin();
    } else if (mode_ == Mode::Result) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
    }

    bool grip = false, brake = false;
    if (mode_ == Mode::Run) {
        time_ += DT;
        if (dwell_ > 0.f) {
            dwell_ -= DT;
            if (dwell_ < 0.f) dwell_ = 0.f;
            brake = true;
        } else if (!over_) {
            if (bot_) control(grip, brake);
            else {
                const gs::Pad& p = sys.pad;
                grip = p.down(gs::BTN_C) || p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.accel > 0.25f;
                brake = p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.down(gs::BTN_X) || p.down(gs::BTN_TURBO) || p.brake > 0.25f;
            }
            if (brake) grip = false;
            physics(grip, brake);
        }
        float target;
        if (dwell_ > 0.f && cleared_ > 0) target = kStops[cleared_ - 1].at - 0.62f * kView;
        else if (won_) target = kStops[kStopN - 1].at - 0.62f * kView;
        else {
            target = s_ - 0.34f * kView;
            if (!over_ && cleared_ < kStopN) {
                float dist = stop().at - s_;
                if (dist < 11.f && dist > -4.f) {
                    float lock = stop().at - 0.62f * kView;
                    float k = std::clamp((11.f - dist) / 7.f, 0.f, 1.f);
                    k = k * k * (3.f - 2.f * k);
                    target = target * (1.f - k) + lock * k;
                }
            }
        }
        cam_ += (target - cam_) * 0.12f;
    }

    if (grip && !gripped_) sys.apu.noiseBurst(0.1f, 420.f, 0.04f);
    grip_ = grip;
    brake_ = brake;
    gripped_ = grip;
    audio();
    draw();
}

}  // namespace cable
