#include "game/cliff.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace cliff {

namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float FINISH = 1320.f;
constexpr float RIVAL = 48.f;
constexpr float ROAD_W = 4.15f;
constexpr float TURN_C[3] = {260.f, 640.f, 1040.f};
constexpr float TURN_H = 58.f;
constexpr float TURN_A[3] = {-0.020f, 0.022f, -0.019f};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::curveAt(float s) const {
    float y = 0;
    for (int i = 0; i < 3; i++) {
        float d = (s - TURN_C[i]) / TURN_H;
        if (d < -1.f || d > 1.f) continue;
        float w = 0.5f + 0.5f * std::cos(d * PI);
        y += TURN_A[i] * w;
    }
    return y;
}

int Game::cliffSide(float s) const {
    float c = curveAt(s);
    if (std::fabs(c) > 0.004f) return c > 0 ? -1 : 1;
    float best = 1e9f;
    int side = -1;
    for (int i = 0; i < 3; i++) {
        float d = std::fabs(s - TURN_C[i]);
        if (d < best) {
            best = d;
            side = TURN_A[i] > 0 ? -1 : 1;
        }
    }
    return side;
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
}

void Game::beginRace() {
    mode_ = Mode::Race;
    s_ = 0;
    x_ = 0.4f;
    vx_ = 0;
    speed_ = 10.f;
    lean_ = 0;
    clock_ = RIVAL;
    turns_ = 0;
    taken_[0] = taken_[1] = taken_[2] = 0;
    hold_ = 0;
    over_ = false;
    won_ = false;
    summary_.clear();
    sys_->apu.noise(0.04f, 0.25f, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.4f);
    mode_ = Mode::Title;
    summary_ = "CLIFFTURN  the stage is still open";
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float w = float(s.size()) * 8.f * scale;
    float ox = x;
    if (align == 0) ox = x - w * 0.5f;
    else if (align < 0) ox = x;
    else ox = x - w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char ch = static_cast<unsigned char>(s[i]);
        if (ch < 32 || ch > 127) continue;
        const gs::Image& img = art_.glyph[ch - 32].pick(8.f * scale);
        gs::Sprite sp;
        sp.img = img;
        sp.w = int(8.f * scale);
        sp.h = int(8.f * scale);
        sp.x = int(ox + float(i) * 8.f * scale);
        sp.y = int(y);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
    }
}

void Game::project(float z, float lateral, float& sx, float& sy, float& ppm) const {
    z = std::max(z, 1.2f);
    float row = 200.f / z;
    int y = int(horizon_ + row);
    y = std::max(0, std::min(gs::SCREEN_H - 1, y));
    ppm = ppm_[y] > 0.01f ? ppm_[y] : row * 2.05f;
    float xoff = xoff_[y];
    sx = 160.f + (-x_ + xoff + lateral) * ppm;
    sy = float(y);
}

void Game::physics(float dt, float steer, float thr, float brk) {
    steer = clampf(steer, -1.f, 1.f);
    float curv = curveAt(s_);
    side_ = cliffSide(s_);
    float accel = thr ? (9.5f + (1.f - speed_ / 48.f) * 8.f) : 0.f;
    float drag = 1.6f + (brk ? 24.f : 0.f);
    speed_ = clampf(speed_ + (accel - drag) * dt, 0.f, 44.f);
    s_ += speed_ * dt;

    float push = -curv * speed_ * speed_;
    float grip = 15.5f + 0.12f * speed_;
    float a = push + steer * grip;
    vx_ += a * dt;
    vx_ *= std::exp(-dt * 1.35f);
    x_ += vx_ * dt;

    // cliffSide -1: drop on the left. outDir is that side in +x.
    float outDir = float(side_);
    float inDir = -outDir;
    if (x_ * inDir > ROAD_W) {
        x_ = inDir * ROAD_W;
        if (vx_ * inDir > 0) vx_ *= -0.25f;
        lean_ += inDir * 0.02f;
    }

    float want = clampf(steer * -0.55f + (-curv) * speed_ * speed_ * 0.0011f, -1.35f, 1.35f);
    lean_ += (want - lean_) * std::min(1.f, dt * 5.f);

    bool off = x_ * outDir > ROAD_W + 0.35f;
    bool tip = lean_ * outDir > 0.96f && x_ * outDir > 0.9f;
    if (off || tip) {
        mode_ = Mode::Tip;
        hold_ = 0;
        shake_ = 1;
        speed_ = 0;
        sys_->apu.noiseBurst(0.45f, 0.7f, 0.35f);
        blip(90.f, 0.2f);
        return;
    }

    for (int i = 0; i < 3; i++) {
        if (!taken_[i] && s_ > TURN_C[i] + TURN_H * 0.85f) {
            taken_[i] = 1;
            turns_++;
            blip(520.f + float(turns_) * 80.f, 0.12f);
        }
    }
    clock_ -= dt;
    if (s_ >= FINISH && turns_ >= 3 && clock_ > 0.f) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        hold_ = 0;
        char buf[96];
        std::snprintf(buf, sizeof buf, "CLIFFTURN WIN  three turns taken  clock %.1fs left", clock_);
        summary_ = buf;
        blip(660.f, 0.18f);
        return;
    }
    if (clock_ <= 0.f) {
        clock_ = 0;
        mode_ = Mode::Lose;
        over_ = true;
        hold_ = 0;
        summary_ = "CLIFFTURN LOSE  the other crew took the cliff";
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    float shy = 0;
    if (shake_ > 0) shy = std::sin(t_ * 40.f) * 3.f * shake_;
    horizon_ = 84 + int(shy);

    float zPrev = 1.1f;
    float yaw = 0;
    float xoff = 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        xoff_[y] = 0;
        ppm_[y] = 0;
    }
    for (int y = gs::SCREEN_H - 1; y >= horizon_; --y) {
        float row = float(y - horizon_) + 0.35f;
        float z = std::min(110.f, 200.f / row);
        float dz = std::max(0.f, z - zPrev);
        float mid = s_ + (zPrev + z) * 0.5f;
        yaw += curveAt(mid) * dz;
        xoff += std::sin(yaw) * dz;
        zPrev = z;
        float ppm = row * 2.15f;
        xoff_[y] = xoff;
        ppm_[y] = ppm;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.hw = std::max(8.f, ROAD_W * ppm);
        r.cx = 160.f + (-x_ + xoff) * ppm;
        r.v = s_ + z;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(r.v / 18.f)) & 1) ? 1 : 0;
        r.style = 1;
        int side = cliffSide(s_ + z);
        r.left = side < 0 ? gs::GROUND_DROP : gs::GROUND_LAND;
        r.right = side > 0 ? gs::GROUND_DROP : gs::GROUND_LAND;
        v.lineFog[y] = uint8_t(clampf(14.f - row / 6.5f, 0.f, 14.f));
        float sea = clampf((y - horizon_) / 90.f, 0.f, 1.f);
        int br = int(2 + sea * 3);
        int bg = int(4 + sea * 5);
        int bb = int(8 + sea * 5);
        v.lineBackdrop[y] = gs::rgb4(br, bg, bb);
    }
    for (int y = 0; y < horizon_; y++) {
        float u = y / float(horizon_);
        v.lineBackdrop[y] = gs::rgb4(int(12 - u * 6), int(7 - u * 3), int(5 + u * 4));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    auto spr = [&](const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0) {
        if (h < 2.f) return;
        const gs::Image& img = m.pick(h);
        float sc = h / float(std::max(1, int(m.h)));
        gs::Sprite sp;
        sp.img = img;
        sp.w = std::max(1, int(m.w * sc));
        sp.h = std::max(1, int(h));
        sp.x = int(cx - sp.w * 0.5f);
        sp.y = int(cy - sp.h);
        sp.pal = uint8_t(pal);
        sp.hflip = flip;
        sp.fog = uint8_t(fog);
        v.sprite(sp);
    };

    // Posts and turn boards, far first so the car covers them (car is submitted first = on top).
    struct Mark {
        float z, lat, h;
        int kind;
        int fog;
    };
    Mark marks[24];
    int nm = 0;
    for (int i = 1; i <= 14 && nm < 24; i++) {
        float z = 4.f + float(i) * 6.5f;
        int side = cliffSide(s_ + z);
        float lat = -float(side) * (ROAD_W + 0.7f);
        int fog = int(clampf((z - 10.f) / 6.f, 0.f, 12.f));
        marks[nm++] = {z, lat, 0, 0, fog};
    }
    for (int i = 0; i < 3 && nm < 24; i++) {
        float z = (TURN_C[i] - TURN_H) - s_;
        if (z > 3.f && z < 90.f) marks[nm++] = {z, 0.f, 0, 1, int(clampf((z - 8.f) / 7.f, 0.f, 12.f))};
    }

    int bank = std::fabs(lean_) > 0.55f ? 2 : std::fabs(lean_) > 0.22f ? 1 : 0;
    bool showCar = mode_ != Mode::Title || true;
    if (showCar && mode_ != Mode::Tip) {
        float bob = std::sin(s_ * 0.7f) * 1.2f;
        spr(art_.car[bank], 168.f + lean_ * 18.f, 206.f + bob + shy, 52.f, PAL_CAR, lean_ > 0.05f);
    } else if (mode_ == Mode::Tip) {
        float spin = hold_ * 4.f;
        spr(art_.car[2], 168.f + std::sin(spin) * 40.f, 200.f + hold_ * 30.f, 52.f - hold_ * 20.f, PAL_CAR, std::sin(spin) > 0);
    }

    float ghostS = (RIVAL - clock_) / RIVAL * FINISH;
    float gz = ghostS - s_;
    if (mode_ == Mode::Race && gz > 3.f && gz < 80.f) {
        float sx, sy, ppm;
        project(gz, 0.2f, sx, sy, ppm);
        int fog = int(clampf((gz - 8.f) / 6.f, 0.f, 13.f));
        spr(art_.ghost, sx, sy, clampf(ppm * 1.15f, 6.f, 40.f), PAL_GHOST, false, fog);
    }

    for (int i = nm - 1; i >= 0; --i) {
        float sx, sy, ppm;
        project(marks[i].z, marks[i].lat, sx, sy, ppm);
        if (marks[i].kind == 0) spr(art_.post, sx, sy, clampf(ppm * 1.6f, 4.f, 48.f), PAL_ROCK, false, marks[i].fog);
        else spr(art_.board, sx, sy - ppm * 1.2f, clampf(ppm * 1.3f, 6.f, 36.f), PAL_SIGN, false, marks[i].fog);
    }

    if (mode_ == Mode::Title) {
        text("S3 CLIFFTURN", 160, 28, 2.f, PAL_HUD);
        text("THREE TURNS ON THE CLIFF", 160, 52, 1.f, PAL_HUD);
        text("DO NOT TIP", 160, 66, 1.f, PAL_SIGN);
        text("THE CLOCK IS THE OTHER CREW", 160, 84, 1.f, PAL_HUD);
        text("START", 160, 112, 1.f, PAL_SIGN);
    } else if (mode_ == Mode::Race || mode_ == Mode::Win || mode_ == Mode::Lose || mode_ == Mode::Tip) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "TURN %d/3", turns_);
        text(buf, 8, 6, 1.f, PAL_HUD, -1);
        std::snprintf(buf, sizeof buf, "CLOCK %04.1f", clock_);
        text(buf, 312, 6, 1.f, clock_ < 10.f ? PAL_SIGN : PAL_HUD, 1);
        std::snprintf(buf, sizeof buf, "SPD %02.0f", speed_);
        text(buf, 8, 18, 1.f, PAL_HUD, -1);
        text(side_ < 0 ? "DROP L" : "DROP R", 312, 18, 1.f, PAL_SIGN, 1);
    }
    if (mode_ == Mode::Win) text("THREE TURNS TAKEN", 160, 78, 1.f, PAL_HUD);
    if (mode_ == Mode::Lose) text("OTHER CREW", 160, 78, 1.f, PAL_SIGN);
    if (mode_ == Mode::Tip) text("TIPPED", 160, 78, 2.f, PAL_SIGN);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    gs::Pad& p = sys.pad;

    if (mode_ == Mode::Title) {
        speed_ = 0;
        s_ = 40.f + std::sin(t_ * 0.3f) * 6.f;
        x_ = 0.3f;
        lean_ = std::sin(t_ * 0.8f) * 0.08f;
        clock_ = RIVAL;
        side_ = cliffSide(s_);
        if (bot_) {
            if (t_ > 0.4f) beginRace();
        } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
            beginRace();
        }
    } else if (mode_ == Mode::Race) {
        float steer = 0, thr = 0, brk = 0;
        if (bot_) {
            float c0 = curveAt(s_);
            float c1 = curveAt(s_ + 16.f + speed_ * 0.45f);
            int ahead = std::fabs(c1) > 0.004f ? (c1 > 0 ? -1 : 1) : side_;
            float target = -float(ahead) * 1.15f;
            if (std::fabs(c1) < 0.003f) target = 0.2f;
            float need = (c0 * speed_ * speed_) / (15.5f + 0.12f * speed_);
            steer = clampf(need + (target - x_) * 0.62f - vx_ * 0.18f, -1.f, 1.f);
            float want = 40.f - std::fabs(c1) * 620.f;
            want = clampf(want, 22.f, 42.f);
            if (speed_ < want) thr = 1;
            if (speed_ > want + 1.5f) brk = 1;
        } else {
            if (p.down(gs::BTN_LEFT)) steer -= 1;
            if (p.down(gs::BTN_RIGHT)) steer += 1;
            if (std::fabs(p.axisX) > 0.15f) steer = p.axisX;
            thr = (p.down(gs::BTN_A) || p.down(gs::BTN_UP) || p.accel > 0.2f) ? 1.f : 0.f;
            brk = (p.down(gs::BTN_B) || p.down(gs::BTN_DOWN) || p.brake > 0.2f) ? 1.f : 0.f;
        }
        physics(DT, steer, thr, brk);
        float rpm = 70.f + speed_ * 7.5f;
        sys.apu.tone(1, rpm, mode_ == Mode::Race ? 0.05f : 0.f);
        sys.apu.noise(mode_ == Mode::Race ? 0.03f + speed_ * 0.001f : 0.f, 0.15f + speed_ * 0.01f, false);
    } else if (mode_ == Mode::Tip) {
        hold_ += DT;
        sys.apu.tone(1, 0, 0);
        if (hold_ > 1.6f) {
            if (bot_) {
                over_ = true;
                won_ = false;
                char buf[80];
                std::snprintf(buf, sizeof buf, "CLIFFTURN LOSE  tipped on turn %d", std::min(turns_ + 1, 3));
                summary_ = buf;
            } else if (clock_ > 2.f) {
                float back = 0;
                for (int i = 0; i < 3; i++)
                    if (!taken_[i]) {
                        back = std::max(0.f, TURN_C[i] - TURN_H - 30.f);
                        break;
                    }
                s_ = std::min(s_, back);
                x_ = -float(cliffSide(s_)) * 0.4f;
                vx_ = 0;
                lean_ = 0;
                speed_ = 12;
                clock_ -= 1.5f;
                mode_ = Mode::Race;
            } else {
                mode_ = Mode::Lose;
                over_ = true;
                summary_ = "CLIFFTURN LOSE  tipped  the other crew kept the cliff";
            }
        }
    }

    draw();
}

}  // namespace cliff
