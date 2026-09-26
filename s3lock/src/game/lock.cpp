#include "game/lock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace s3lock {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float PI = 3.14159265f;
constexpr float PIX = 8.5f;
constexpr float ANCHOR = 170.f;
constexpr float BOAT_L = 10.f;
constexpr float BOAT_B = 2.f;
constexpr float LOCK_HALF = 2.70f;
constexpr float CANAL_HALF = 7.2f;
constexpr float GATE_LO = 28.f;
constexpr float GATE_HI = 48.f;
constexpr float MEET = 0.72f;
constexpr float YAW_LIM = YAW_SPAN * PI / 180.f;
constexpr float HIT_R = 0.30f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpColor(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float wrapPi(float a) {
    while (a > PI) a -= 2.f * PI;
    while (a < -PI) a += 2.f * PI;
    return a;
}

float segDist(float px, float py, float ax, float ay, float bx, float by) {
    float abx = bx - ax, aby = by - ay;
    float den = abx * abx + aby * aby;
    float t = den < 1e-6f ? 0.f : ((px - ax) * abx + (py - ay) * aby) / den;
    t = clampf(t, 0.f, 1.f);
    float dx = px - (ax + abx * t);
    float dy = py - (ay + aby * t);
    return std::sqrt(dx * dx + dy * dy);
}

}  // namespace

float Game::bowY() const { return y_ + std::cos(hdg_) * (BOAT_L * 0.5f); }
float Game::sternY() const { return y_ - std::cos(hdg_) * (BOAT_L * 0.5f); }

float Game::halfAt(float y) const {
    auto smooth = [](float u) {
        u = clampf(u, 0.f, 1.f);
        return u * u * (3.f - 2.f * u);
    };
    if (y < GATE_LO - 9.f) return CANAL_HALF;
    if (y < GATE_LO - 1.1f) {
        float u = (y - (GATE_LO - 9.f)) / (9.f - 1.1f);
        return CANAL_HALF + (LOCK_HALF - CANAL_HALF) * smooth(u);
    }
    if (y <= GATE_HI + 1.1f) return LOCK_HALF;
    if (y < GATE_HI + 9.f) {
        float u = (y - (GATE_HI + 1.1f)) / (9.f - 1.1f);
        return LOCK_HALF + (CANAL_HALF - LOCK_HALF) * smooth(u);
    }
    return CANAL_HALF;
}

float Game::windAt(float y) const {
    float dLo = y - GATE_LO;
    float dHi = y - GATE_HI;
    float g = std::exp(-dLo * dLo / 14.f) * 0.28f + std::exp(-dHi * dHi / 14.f) * 0.18f;
    if (phase_ == Phase::Fill) g += 0.16f * std::sin(fill_ * PI);
    return 0.14f + g;
}

float Game::sx(float wx) const { return 160.f + wx * PIX + shx_; }
float Game::sy(float wy) const { return ANCHOR - (wy - y_) * PIX + shy_; }

void Game::leafGeom(int which, float open, float& px, float& py, float& tx, float& ty) const {
    const bool upper = which >= 2;
    const bool right = (which & 1) != 0;
    const float gy = upper ? GATE_HI : GATE_LO;
    const float sign = right ? 1.f : -1.f;
    const float len = std::sqrt(LOCK_HALF * LOCK_HALF + MEET * MEET);
    px = sign * LOCK_HALF;
    py = gy;
    const float meetX = 0.f;
    const float meetY = gy + (upper ? MEET : -MEET);
    const float cang = std::atan2(meetY - py, meetX - px);
    // Housed leaf sits just inboard of the wall, swung out of the cut.
    const float oX = px - sign * 0.18f;
    const float oY = gy + (upper ? len : -len);
    const float oang = std::atan2(oY - py, oX - px);
    const float ang = cang + wrapPi(oang - cang) * clampf(open, 0.f, 1.f);
    tx = px + std::cos(ang) * len;
    ty = py + std::sin(ang) * len;
}

int Game::yawFrame() const {
    float deg = clampf(hdg_ * 180.f / PI, -YAW_SPAN, YAW_SPAN);
    const float step = (2.f * YAW_SPAN) / float(YAWS - 1);
    int i = int(std::lround((deg + YAW_SPAN) / step));
    return std::clamp(i, 0, YAWS - 1);
}

void Game::reset() {
    phase_ = Phase::Approach;
    over_ = false;
    won_ = false;
    why_ = "";
    playT_ = 0;
    x_ = 0.42f;
    y_ = 13.9f;
    hdg_ = 0.045f;
    speed_ = 0;
    latV_ = 0;
    yawV_ = 0;
    thrust_ = 0;
    rudder_ = 0;
    openLo_ = openHi_ = 0;
    tgtLo_ = tgtHi_ = 0;
    dwell_ = hold_ = fill_ = 0;
    shake_ = 0;
    called_ = false;
}

void Game::begin() {
    reset();
    mode_ = Mode::Play;
    t_ = 0;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.55f, 640.f, 0.26f);
    sys_->apu.tone(1, 0, 0);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "clear";
    bell(true);
}

void Game::bell(bool big) {
    bellBig_ = big;
    bellStep_ = 0;
    bellT_ = 0;
}

void Game::pilot(float& thrust, float& rudder) const {
    const float aimH = clampf(-x_ * 0.7f - latV_ * 0.4f, -0.18f, 0.18f);
    rudder = clampf((aimH - hdg_) * 3.2f - yawV_ * 0.8f - x_ * 0.85f, -1.f, 1.f);

    const float bow = bowY();
    const float stern = sternY();
    float want = 0.f;

    if (phase_ == Phase::Approach || phase_ == Phase::Lower) {
        const float dist = GATE_LO - bow;
        if (phase_ == Phase::Approach) {
            want = dist > 9.f ? 1.35f : 0.62f;
        } else if (openLo_ < 0.84f) {
            // Sit a few metres off the leaf until the mouth is actually open.
            if (dist < 4.6f) want = -0.3f;
            else if (dist < 6.0f) want = 0.f;
            else want = 0.55f;
        } else if (std::fabs(x_) < 0.62f) {
            want = 1.12f;
        } else if (dist < 5.2f) {
            want = 0.f;
        } else {
            want = 0.4f;
        }
    } else if (phase_ == Phase::Chamber || phase_ == Phase::Fill) {
        const float mid = (GATE_LO + GATE_HI) * 0.5f;
        want = clampf(-(y_ - mid) * 0.72f, -0.5f, 0.5f);
    } else {
        const float dist = GATE_HI - bow;
        if (openHi_ < 0.84f) {
            if (dist < 4.4f) want = -0.25f;
            else if (dist < 5.8f) want = 0.f;
            else want = 0.45f;
        } else if (std::fabs(x_) < 0.62f) {
            want = 1.15f;
        } else if (dist < 5.f) {
            want = 0.f;
        } else {
            want = 0.4f;
        }
        if (stern > GATE_HI + 0.3f) want = 1.3f;
    }
    const float err = want - speed_;
    thrust = std::fabs(err) < 0.035f ? 0.f : clampf(err * 2.1f, -1.f, 1.f);
}

void Game::confine() {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    float pushL = 0.f, pushR = 0.f;
    const float along[5] = {0.46f, 0.23f, 0.f, -0.23f, -0.46f};
    const float side[2] = {-0.5f, 0.5f};
    for (float s : along) {
        for (float b : side) {
            float wx = x_ + fx * s * BOAT_L + rx * b * BOAT_B;
            float wy = y_ + fy * s * BOAT_L + ry * b * BOAT_B;
            float lim = halfAt(wy) - 0.06f;
            pushR = std::max(pushR, wx - lim);
            pushL = std::max(pushL, -lim - wx);
        }
    }
    if (pushR > pushL) x_ -= pushR;
    else x_ += pushL;
    const float push = pushR > pushL ? -pushR : pushL;
    if (push < -0.001f && latV_ > 0) latV_ *= 0.25f;
    if (push > 0.001f && latV_ < 0) latV_ *= 0.25f;
    if (std::fabs(push) > 0.02f) {
        hdg_ *= 0.9f;
        yawV_ *= 0.45f;
    }
}

const char* Game::gateHit() {
    const float fx = std::sin(hdg_), fy = std::cos(hdg_);
    const float rx = std::cos(hdg_), ry = -std::sin(hdg_);
    const float along[7] = {0.50f, 0.32f, 0.14f, 0.f, -0.14f, -0.32f, -0.50f};
    const float side[3] = {-0.5f, 0.f, 0.5f};
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        if (open >= 0.90f) continue;  // leaf is housed in the recess
        float ax, ay, bx, by;
        leafGeom(which, open, ax, ay, bx, by);
        // The hinge sits in the wall. Only the leaf out in the cut can take the hull.
        const float dx = bx - ax, dy = by - ay;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.5f) {
            const float cut = 0.42f / len;
            ax += dx * cut;
            ay += dy * cut;
        }
        for (float s : along) {
            for (float b : side) {
                float wx = x_ + fx * s * BOAT_L + rx * b * BOAT_B;
                float wy = y_ + fy * s * BOAT_L + ry * b * BOAT_B;
                if (segDist(wx, wy, ax, ay, bx, by) < HIT_R) return which < 2 ? "hit the lower gate" : "hit the upper gate";
            }
        }
    }
    return nullptr;
}

void Game::step() {
    playT_ += DT;
    if (playT_ > 75.f) {
        fail("timed out");
        return;
    }

    float thrust = 0.f, rudder = 0.f;
    bool call = false;
    if (bot_) {
        pilot(thrust, rudder);
        const float dist = GATE_LO - bowY();
        if (phase_ == Phase::Approach && dist < 8.2f && std::fabs(speed_) < 1.05f) call = true;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) rudder -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) rudder += 1.f;
        rudder += pad.axisX;
        rudder = clampf(rudder, -1.f, 1.f);
        const bool astern = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B) || pad.brake > 0.2f;
        const bool ahead = pad.down(gs::BTN_UP) || pad.down(gs::BTN_C) || pad.accel > 0.15f;
        if (astern) thrust = -1.f;
        else if (ahead) thrust = 1.f;
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) call = true;
        const float dist = GATE_LO - bowY();
        if (phase_ == Phase::Approach && dist < 8.2f && std::fabs(speed_) < 1.05f) call = true;
    }
    thrust_ = thrust;
    rudder_ = rudder;

    if (call && phase_ == Phase::Approach && !called_) {
        phase_ = Phase::Lower;
        tgtLo_ = 1.f;
        dwell_ = 0.f;
        called_ = true;
        bell(false);
    }

    // Heavy tiller: yaw and a little crab, so a correction shows on the stern.
    yawV_ += rudder_ * (0.55f + std::fabs(speed_) * 0.40f) * DT;
    yawV_ -= yawV_ * 2.6f * DT;
    hdg_ += yawV_ * DT;
    hdg_ = clampf(hdg_, -YAW_LIM, YAW_LIM);

    const float drive = thrust_ * (thrust_ >= 0.f ? 2.15f : 4.4f);
    speed_ += drive * DT;
    const float drag = std::fabs(thrust_) > 0.12f ? 0.28f : 0.22f;
    speed_ -= speed_ * drag * DT;
    speed_ = clampf(speed_, -1.15f, 2.25f);

    latV_ += windAt(y_) * DT;
    latV_ += rudder_ * 0.95f * DT;
    latV_ -= latV_ * 2.5f * DT;

    x_ += std::sin(hdg_) * speed_ * DT + latV_ * DT;
    y_ += std::cos(hdg_) * speed_ * DT;

    confine();
    if (const char* hit = gateHit()) {
        fail(hit);
        return;
    }

    auto ease = [&](float& o, float target) {
        const float rate = (target > o ? 0.46f : 0.62f) * DT;
        if (o < target) o = std::min(target, o + rate);
        else o = std::max(target, o - rate);
    };

    if (phase_ == Phase::Lower) {
        // Shut only if the boat never came in. A leaf does not close on a hull already in the mouth.
        const bool outside = bowY() < GATE_LO - 0.5f;
        if (outside && openLo_ > 0.96f) dwell_ += DT;
        else dwell_ = 0.f;
        if (dwell_ > 7.f && outside) tgtLo_ = 0.f;
        if (tgtLo_ < 0.5f && openLo_ < 0.04f && outside) {
            phase_ = Phase::Approach;
            called_ = false;
            dwell_ = 0.f;
        }
        if (sternY() > GATE_LO + 0.7f && bowY() < GATE_HI - 1.2f) {
            phase_ = Phase::Chamber;
            tgtLo_ = 0.f;
            hold_ = 0.f;
            dwell_ = 0.f;
        }
    } else if (phase_ == Phase::Chamber) {
        // Anywhere clear of both leaves counts. The pound is the hold, not a single point.
        const bool inPound = sternY() > GATE_LO + 1.6f && bowY() < GATE_HI - 1.6f;
        const bool settled = inPound && std::fabs(speed_) < 0.42f && std::fabs(x_) < 0.95f && std::fabs(hdg_) < 0.20f;
        if (settled) hold_ += DT;
        else hold_ = std::max(0.f, hold_ - DT * 0.35f);
        if (hold_ > 0.45f) {
            phase_ = Phase::Fill;
            fill_ = 0.f;
            bell(false);
        }
    } else if (phase_ == Phase::Fill) {
        fill_ = std::min(1.f, fill_ + DT / 2.3f);
        if (fill_ >= 1.f) {
            phase_ = Phase::Upper;
            tgtHi_ = 1.f;
            dwell_ = 0.f;
            bell(false);
        }
    } else if (phase_ == Phase::Upper) {
        if (openHi_ > 0.96f) dwell_ += DT;
        if (sternY() > GATE_HI + 3.4f && openHi_ > 0.72f) {
            win();
            return;
        }
    }

    ease(openLo_, tgtLo_);
    ease(openHi_, tgtHi_);
}

const char* Game::hint() const {
    if (mode_ == Mode::Win) return "ONE LOCK CLEAR";
    if (mode_ == Mode::Fail) {
        if (why_ && why_[0]) return why_;
        return "HIT A GATE";
    }
    if (mode_ == Mode::Pause) return "PAUSED";
    if (mode_ == Mode::Title) return "ENTER TO CAST OFF";
    const float dist = GATE_LO - bowY();
    switch (phase_) {
    case Phase::Approach:
        if (dist < 9.f && std::fabs(speed_) > 1.15f) return "TOO FAST - EASE UP";
        if (dist < 10.f) return "EASE UP TO OPEN THE GATE";
        return "ONE LOCK AHEAD";
    case Phase::Lower:
        if (tgtLo_ < 0.5f) return "THE LOWER GATE IS CLOSING";
        if (openLo_ < 0.75f) return "WAIT FOR THE GAP";
        return "STRAIGHT THROUGH - DON'T CLIP IT";
    case Phase::Chamber:
        return "STOP IN THE POUND";
    case Phase::Fill:
        return "HOLD - THE POUND IS FILLING";
    case Phase::Upper:
        if (openHi_ < 0.75f) return "WAIT FOR THE UPPER GATE";
        return "CLEAR THE UPPER GATE";
    }
    return "";
}

void Game::audio() {
    if (bellStep_ >= 0) {
        bellT_ -= DT;
        if (bellT_ <= 0.f) {
            static const float notes[] = {523.f, 784.f, 659.f, 1046.f};
            const int count = bellBig_ ? 4 : 2;
            if (bellStep_ >= count) {
                sys_->apu.tone(0, 0, 0);
                bellStep_ = -1;
            } else {
                sys_->apu.tone(0, notes[bellStep_], 0.07f);
                bellStep_++;
                bellT_ = 0.13f;
            }
        }
    }
    if (mode_ == Mode::Play) {
        const float vol = 0.025f + std::fabs(speed_) * 0.018f + std::fabs(thrust_) * 0.02f;
        sys_->apu.tone(1, 46.f + std::fabs(speed_) * 16.f, vol);
        sys_->apu.noise(0.018f, 140.f + std::fabs(speed_) * 40.f, false);
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.noise(0.012f, 120.f, false);
    } else {
        sys_->apu.tone(1, 0, 0);
        sys_->apu.noise(0.f, 0.f, false);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !*s) return;
    const float adv = float(art_.glyph[0].w) * scale;
    const float w = float(std::strlen(s)) * adv;
    x -= w * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        blit(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.w * scale, g.h * scale, pal);
    }
}

void Game::blit(const gs::Mipped& m, float cx, float cy, float dw, float dh, int pal, bool flip) {
    if (dw < 1.f || dh < 1.f || m.h < 1) return;
    if (cx + dw * 0.5f < -30.f || cx - dw * 0.5f > gs::SCREEN_W + 30.f) return;
    if (cy + dh * 0.5f < -30.f || cy - dh * 0.5f > gs::SCREEN_H + 30.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(dw)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(dh)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::drawBar(float ax, float ay, float bx, float by) {
    const float x0 = sx(ax), y0 = sy(ay), x1 = sx(bx), y1 = sy(by);
    const float len = std::sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    if (len < 2.f) return;
    float deg = std::atan2(y1 - y0, x1 - x0) * 180.f / PI;
    int bin = int(std::lround(deg / (360.f / float(GATE_DIRS))));
    bin = (bin % GATE_DIRS + GATE_DIRS) % GATE_DIRS;
    const gs::Mipped& m = art_.leaf[bin];
    const float scale = len / float(LEAF_SRC_L);
    blit(m, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, m.w * scale, m.h * scale, PAL_GATE);
}

void Game::drawGates() {
    for (int which = 0; which < 4; which++) {
        const float open = which < 2 ? openLo_ : openHi_;
        float px, py, tx, ty;
        leafGeom(which, open, px, py, tx, ty);
        const float len = std::sqrt((tx - px) * (tx - px) + (ty - py) * (ty - py));
        const float beam = 1.85f;
        // Balance beam swings the other way, onto the towpath.
        const float bx = px - (tx - px) / len * beam;
        const float by = py - (ty - py) / len * beam;
        drawBar(px, py, bx, by);
        drawBar(px, py, tx, ty);
    }
}

void Game::drawBoat() {
    const float bob = std::sin(t_ * 2.1f) * (phase_ == Phase::Fill ? 2.2f : 0.6f);
    const float scale = (BOAT_L * PIX) / float(BOAT_SRC_H);
    const gs::Mipped& hull = art_.boat[yawFrame()];
    const float cx = sx(x_);
    const float cy = sy(y_) + bob;
    blit(hull, cx, cy, hull.w * scale, hull.h * scale, PAL_BOAT);
    if (std::fabs(speed_) > 0.25f) {
        const float fx = std::sin(hdg_), fy = std::cos(hdg_);
        for (int i = 0; i < 3; i++) {
            float k = 0.55f + float(i) * 0.22f;
            float wx = x_ - fx * BOAT_L * k;
            float wy = y_ - fy * BOAT_L * k;
            float z = 10.f + float(i) * 4.f;
            blit(art_.ripple, sx(wx), sy(wy), z, z * 0.55f, PAL_FX);
        }
    }
}

void Game::drawWorld() {
    const float top = y_ + ANCHOR / PIX + 2.f;
    const float bot = y_ - (gs::SCREEN_H - ANCHOR) / PIX - 2.f;

    // Grass first in the list would cover the walls. Walls are added first so they
    // sit on top of the banks (earlier sprites draw above later ones).
    for (float wy = std::floor(bot); wy < top; wy += 1.55f) {
        if (wy < GATE_LO - 8.5f || wy > GATE_HI + 8.5f) continue;
        const float edge = halfAt(wy + 0.4f);
        const float dw = 36.f, dh = 16.f;
        blit(art_.stone, sx(-edge) - dw * 0.5f, sy(wy), dw, dh, PAL_STONE);
        blit(art_.stone, sx(edge) + dw * 0.5f, sy(wy), dw, dh, PAL_STONE, true);
    }

    const float trees[][2] = {{-9.2f, 6.f}, {8.6f, 16.f}, {-9.6f, 34.f}, {9.1f, 44.f}, {-8.8f, 56.f}, {8.4f, 24.f}};
    for (auto& tr : trees) blit(art_.tree, sx(tr[0]), sy(tr[1]), 30.f, 38.f, PAL_BANK);

    // Keeper stands the lower beam, out on the towpath. He does not open the gate.
    blit(art_.keeper, sx(-(LOCK_HALF + 3.6f)), sy(GATE_LO + 0.15f), 18.f, 30.f, PAL_KEEP);

    blit(art_.buoy, sx(-1.55f), sy(GATE_LO - 6.5f) + std::sin(t_ * 1.7f) * 1.2f, 12.f, 16.f, PAL_GREEN);
    blit(art_.buoy, sx(1.55f), sy(GATE_LO - 6.5f) + std::sin(t_ * 1.7f + 1.f) * 1.2f, 12.f, 16.f, PAL_RED);

    const float tileW = 52.f, tileH = 16.f;
    for (float wy = std::floor(bot / 1.7f) * 1.7f; wy < top; wy += 1.7f) {
        const float edge = halfAt(wy);
        const float syw = sy(wy);
        const float left = sx(-edge);
        const float right = sx(edge);
        blit(art_.grass, left - tileW * 0.5f, syw, tileW, tileH, PAL_BANK);
        blit(art_.grass, right + tileW * 0.5f, syw, tileW, tileH, PAL_BANK, true);
        for (int i = 1; i <= 3; i++) {
            blit(art_.field, left - tileW * (float(i) + 0.5f), syw, tileW, tileH, PAL_BANK);
            blit(art_.field, right + tileW * (float(i) + 0.5f), syw, tileW, tileH, PAL_BANK);
        }
    }

    // A few rings on the pound so the water isn't a flat fill.
    for (int i = 0; i < 5; i++) {
        float wy = y_ - 4.f + float(i) * 3.1f + std::sin(t_ * 0.6f + float(i)) * 0.3f;
        float wx = std::sin(t_ * 0.4f + float(i) * 1.7f) * 1.1f;
        blit(art_.ripple, sx(wx), sy(wy), 16.f, 8.f, PAL_FX);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(gs::rgb4(1, 3, 5));
    const uint16_t lower = gs::rgb4(1, 4, 6);
    const uint16_t lowerHi = gs::rgb4(2, 7, 9);
    const uint16_t upper = gs::rgb4(3, 10, 12);
    const uint16_t upperHi = gs::rgb4(5, 13, 14);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        const float wy = y_ + (ANCHOR - float(y)) / PIX;
        float band = std::fmod(wy * 0.45f + t_ * 0.55f, 1.f);
        if (band < 0.f) band += 1.f;
        const bool streak = band < 0.14f;
        uint16_t c = streak ? lowerHi : lower;
        if (wy >= GATE_HI) c = streak ? upperHi : upper;
        else if (wy >= GATE_LO) {
            uint16_t lo = streak ? lowerHi : lower;
            uint16_t hi = streak ? upperHi : upper;
            c = lerpColor(lo, hi, fill_);
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    shx_ = shy_ = 0;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 90.f) * 3.2f * shake_;
        shy_ = std::cos(t_ * 73.f) * 2.1f * shake_;
        shake_ = std::max(0.f, shake_ - DT * (mode_ == Mode::Play ? 1.f : 0.8f));
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) {
        text("S3 LOCK", 160.f, 12.f, 0.85f, PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        text("GATES CLEAR", 160.f, 78.f, 0.95f, PAL_GREEN);
        text("ONE LOCK", 160.f, 104.f, 0.7f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text("GATE HIT", 160.f, 78.f, 1.05f, PAL_RED);
        text(why_ && why_[0] ? why_ : "HIT A GATE", 160.f, 104.f, 0.62f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 96.f, 1.f, PAL_HUD);
    }

    drawBoat();
    drawGates();
    drawWorld();

    char buf[48];
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 LOCK", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "%d.%d M/S", int(std::fabs(speed_)), int(std::fabs(speed_) * 10.f) % 10);
        if (speed_ < -0.05f) hud(32, 0, "ASTERN", PAL_RED);
        else hud(40 - int(std::strlen(buf)), 0, buf, PAL_HUD);
        hud(1, 1, hint(), phase_ == Phase::Approach && std::fabs(speed_) > 1.15f ? PAL_RED : PAL_HUD);

        char lane[] = "[       ]";
        int slot = std::clamp(int(std::lround(x_ / LOCK_HALF * 3.f)) + 4, 1, 7);
        lane[slot] = '|';
        const char* gate = "LOWER SHUT";
        if (phase_ == Phase::Lower) gate = tgtLo_ < 0.5f ? "LOWER CLOSING" : openLo_ > 0.9f ? "LOWER OPEN" : "LOWER OPENING";
        else if (phase_ == Phase::Chamber) gate = "IN THE POUND";
        else if (phase_ == Phase::Fill) gate = "POUND FILLING";
        else if (phase_ == Phase::Upper) gate = openHi_ > 0.9f ? "UPPER OPEN" : "UPPER OPENING";
        std::snprintf(buf, sizeof buf, "%s  %s", lane, gate);
        hud(1, 2, buf, PAL_GREEN);
    } else if (mode_ == Mode::Title) {
        hudC(8, "DON'T HIT THE GATES", PAL_AMBER);
        hudC(9, int(t_ * 2.f) % 2 == 0 ? "ENTER TO CAST OFF" : "ARROWS DRIVE", PAL_HUD);
        hudC(10, "Z CALLS THE LOCK", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(3, "THE GATES NEVER TOUCHED THE HULL", PAL_GREEN);
        hudC(4, "ENTER SAILS IT AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, "A GATE TOOK THE HULL", PAL_RED);
        hudC(4, "ENTER TRIES THE LOCK AGAIN", PAL_HUD);
    }
    hud(40 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.8f);
    reset();
    t_ = 0;
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || bot_) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else step();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            reset();
            mode_ = Mode::Title;
        }
    } else if (!bot_) {
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE)) {
            reset();
            mode_ = Mode::Title;
        }
    }

    draw();
    audio();
}

}  // namespace s3lock
