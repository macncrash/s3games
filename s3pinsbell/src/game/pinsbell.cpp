#include "game/pinsbell.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pinsbell {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 56;
constexpr float FOCAL = 200.0f;
constexpr float LANE_K = 0.58f;
constexpr float Z_BALL = 1.58f;
constexpr float Z_HEAD = 4.10f;
constexpr float PIN_S = 0.48f;
constexpr float PIN_DZ = 0.54f;
constexpr float PIN_R = 0.09f;
constexpr float BALL_R = 0.16f;
constexpr float BALL_VZ = 3.25f;
constexpr float BALL_MASS = 2.6f;
constexpr float PIN_MASS = 1.0f;
constexpr float POCKET = 0.19f;
constexpr float POCKET_LO = 0.05f;
constexpr float POCKET_HI = 0.34f;
constexpr float MISS = 1.1f;
constexpr float PERIOD = 1.2f;
constexpr float LIFE = 12.0f;
constexpr float BELL_X = 0.36f;
constexpr float BELL_Z = 4.32f;
constexpr float BELL_R = 0.17f;

const float PIN_K[10] = {0, -0.5f, 0.5f, -1, 0, 1, -1.5f, -0.5f, 0.5f, 1.5f};
const int PIN_ROW[10] = {0, 1, 1, 2, 2, 2, 3, 3, 3, 3};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

void collide(float& ax, float& az, float& avx, float& avz, float ar, float am, float& bx, float& bz, float& bvx,
             float& bvz, float br, float bm, float e) {
    float dx = bx - ax, dz = bz - az;
    float d2 = dx * dx + dz * dz;
    float r = ar + br;
    if (d2 >= r * r || d2 < 1e-8f) return;
    float d = std::sqrt(d2);
    float nx = dx / d, nz = dz / d;
    float overlap = r - d;
    float invA = 1.0f / am, invB = 1.0f / bm;
    float inv = invA + invB;
    ax -= nx * overlap * (invA / inv);
    az -= nz * overlap * (invA / inv);
    bx += nx * overlap * (invB / inv);
    bz += nz * overlap * (invB / inv);
    float rv = (bvx - avx) * nx + (bvz - avz) * nz;
    if (rv >= 0) return;
    float j = -(1.0f + e) * rv / inv;
    avx -= j * nx * invA;
    avz -= j * nz * invA;
    bvx += j * nx * invB;
    bvz += j * nz * invB;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    over_ = won_ = rung_ = false;
    dead_ = 0;
    rungTry_ = 0;
    clock_ = 0;
    bellPh_ = 0;
    bellAmp_ = 0.06f;
    flash_ = 0;
    stance_ = 0;
    resetRack();
    mode_ = Mode::Title;
}

void Game::newGame() {
    dead_ = 0;
    rung_ = false;
    rungTry_ = 0;
    won_ = false;
    over_ = false;
    stance_ = 0;
    hook_ = 0;
    shake_ = 0;
    flash_ = 0;
    bellT_ = 0;
    wasLine_ = false;
    beginApproach();
}

void Game::beginApproach() {
    resetRack();
    life_ = LIFE;
    hook_ = 0;
    rollT_ = 0;
    swingT_ = 0;
    wasLine_ = false;
    mode_ = Mode::Approach;
}

void Game::resetRack() {
    for (int i = 0; i < 10; i++) {
        Pin& p = pin_[i];
        p.homeX = PIN_K[i] * PIN_S;
        p.homeZ = Z_HEAD + PIN_ROW[i] * PIN_DZ;
        p.x = p.homeX;
        p.z = p.homeZ;
        p.vx = p.vz = 0;
        p.down = false;
        p.fall = 0;
    }
    ballLive_ = false;
    gutter_ = false;
    gutterSnd_ = false;
    driven_ = false;
    pocket_ = false;
    crossed_ = false;
    driveT_ = 0;
    ripple_ = -1;
    entry_ = 0;
    ballVx_ = ballVz_ = 0;
}

void Game::beginSwing() {
    mode_ = Mode::Swing;
    swingT_ = 0;
    wasLine_ = false;
}

void Game::release() {
    float miss = (meter() - 0.5f) * MISS;
    ballX_ = clampf(stance_ + miss, -1.2f, 1.2f);
    ballZ_ = Z_BALL;
    ballVx_ = 0;
    ballVz_ = BALL_VZ;
    ballLive_ = true;
    gutter_ = std::fabs(ballX_) > 1.02f;
    gutterSnd_ = false;
    crossed_ = false;
    pocket_ = false;
    driven_ = false;
    driveT_ = 0;
    ripple_ = -1;
    entry_ = ballX_;
    hook_ = 0;
    rollT_ = 0;
    mode_ = Mode::Roll;
    sys_->apu.tone(2, 180, 0.05f);
    sys_->apu.noiseBurst(0.18f, 2200, 0.05f);
    beep_ = 0.08f;
}

void Game::ring() {
    if (rung_) return;
    if (mode_ != Mode::Roll) return;
    rung_ = true;
    rungTry_ = dead_ + 1;
    bellAmp_ = 1.0f;
    bellT_ = 1.6f;
    flash_ = 1.0f;
    shake_ = 0.45f;
    ringWait_ = 0.55f;
    mode_ = Mode::Ring;
    ballLive_ = false;
    sys_->apu.tone(0, 784, 0.13f);
    sys_->apu.tone(1, 1175, 0.10f);
    sys_->rumble(0.45f, 0.85f, 200);
    sys_->setLight(255, 196, 48);
}

void Game::dieTry() {
    if (rung_) return;
    if (mode_ != Mode::Approach && mode_ != Mode::Swing && mode_ != Mode::Roll) return;
    ballLive_ = false;
    dead_++;
    sys_->apu.noise(0, 1000);
    shake_ = 0.15f;
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        sys_->setLight(150, 28, 28);
        sys_->apu.tone(0, 90, 0.10f);
        sys_->apu.tone(1, 66, 0.08f);
        beep_ = 0.45f;
    } else {
        mode_ = Mode::Dead;
        deadWait_ = 0.8f;
        sys_->setLight(120, 48, 28);
        sys_->apu.tone(0, 128, 0.08f);
        beep_ = 0.32f;
    }
}

float Game::meter() const {
    float u = std::fmod(std::max(0.0f, swingT_), PERIOD) / PERIOD;
    return u < 0.5f ? u * 2.0f : 2.0f - u * 2.0f;
}

float Game::aimX() const {
    if (mode_ == Mode::Swing) return stance_ + (meter() - 0.5f) * MISS;
    if (mode_ == Mode::Roll && ballLive_) return ballX_;
    return stance_;
}

bool Game::onLine() const {
    float x = mode_ == Mode::Roll ? ballX_ : aimX();
    return x >= POCKET_LO && x <= POCKET_HI;
}

int Game::pose() const {
    if (mode_ == Mode::Swing) return 1;
    if (mode_ == Mode::Roll && rollT_ < 0.45f) return 2;
    if (mode_ == Mode::Leave) return 3;
    if (mode_ == Mode::Over && won_) return 3;
    return 0;
}

bool Game::rollDone() const {
    if (rung_) return false;
    bool quiet = true;
    for (const Pin& p : pin_)
        if (std::hypot(p.vx, p.vz) > 0.45f) quiet = false;
    bool past = ballZ_ > Z_HEAD + 3.2f * PIN_DZ;
    bool stalled = gutter_ && ballVz_ < 0.5f && rollT_ > 0.35f;
    return (past && quiet) || stalled || rollT_ > 2.7f;
}

void Game::physics(float dt) {
    const float h = dt / 4.0f;
    int fresh = 0;
    for (int step = 0; step < 4; step++) {
        if (ballLive_) {
            float px = ballX_, pz = ballZ_;
            if (!gutter_ && ballZ_ < Z_HEAD - 0.15f) ballVx_ += hook_ * 2.4f * h;
            ballX_ += ballVx_ * h;
            ballZ_ += ballVz_ * h;
            ballVx_ *= (1.0f - 0.5f * h);
            if (!gutter_ && ballZ_ < Z_HEAD + 2.2f) ballVz_ = BALL_VZ;
            else ballVz_ *= (1.0f - 1.3f * h);
            if (!gutter_ && std::fabs(ballX_) > 1.02f && ballZ_ < Z_HEAD + 0.4f) gutter_ = true;
            if (gutter_) {
                ballX_ = std::copysign(1.12f, ballX_);
                ballVz_ *= (1.0f - 1.5f * h);
            }
            if (!crossed_ && pz < Z_HEAD && ballZ_ >= Z_HEAD) {
                float u = (ballZ_ - pz) > 1e-5f ? (Z_HEAD - pz) / (ballZ_ - pz) : 1.0f;
                entry_ = px + (ballX_ - px) * u;
                crossed_ = true;
                pocket_ = !gutter_ && entry_ >= POCKET_LO && entry_ <= POCKET_HI && ballVz_ > 1.3f;
            }
            if (!gutter_) {
                for (Pin& p : pin_) {
                    bool was = p.down;
                    collide(ballX_, ballZ_, ballVx_, ballVz_, BALL_R, BALL_MASS, p.x, p.z, p.vx, p.vz, PIN_R, PIN_MASS,
                            0.3f);
                    float sp = std::hypot(p.vx, p.vz);
                    if (sp > 8.0f) {
                        p.vx *= 8.0f / sp;
                        p.vz *= 8.0f / sp;
                    }
                    if (!p.down && (sp > 1.2f || std::hypot(p.x - p.homeX, p.z - p.homeZ) > 0.28f)) p.down = true;
                    if (p.down && !was) fresh++;
                }
            }
        }
        for (int a = 0; a < 10; a++) {
            for (int b = a + 1; b < 10; b++) {
                Pin& pa = pin_[a];
                Pin& pb = pin_[b];
                if (std::hypot(pa.vx, pa.vz) < 0.05f && std::hypot(pb.vx, pb.vz) < 0.05f && !pa.down && !pb.down)
                    continue;
                bool wa = pa.down, wb = pb.down;
                collide(pa.x, pa.z, pa.vx, pa.vz, PIN_R, PIN_MASS, pb.x, pb.z, pb.vx, pb.vz, PIN_R, PIN_MASS, 0.5f);
                for (Pin* p : {&pa, &pb}) {
                    float sp = std::hypot(p->vx, p->vz);
                    if (!p->down && (sp > 1.1f || std::hypot(p->x - p->homeX, p->z - p->homeZ) > 0.28f)) p->down = true;
                }
                if (pa.down && !wa) fresh++;
                if (pb.down && !wb) fresh++;
            }
        }
        if (pocket_ && !driven_) {
            driven_ = true;
            pin_[0].down = true;
        }
        if (driven_ && !rung_) {
            Pin& head = pin_[0];
            float dx = BELL_X - head.x;
            float dz = BELL_Z - head.z;
            float d = std::hypot(dx, dz);
            head.down = true;
            if (d < 1e-4f) d = 1.0f;
            head.vx = dx / d * 2.8f;
            head.vz = dz / d * 2.8f;
        }
        for (Pin& p : pin_) {
            if (!p.down && std::hypot(p.vx, p.vz) < 0.04f) continue;
            p.x += p.vx * h;
            p.z += p.vz * h;
            float drag = 1.0f - 2.0f * h;
            p.vx *= drag;
            p.vz *= drag;
            p.x = clampf(p.x, -2.2f, 2.2f);
            p.z = clampf(p.z, 0.9f, 8.2f);
            float sp = std::hypot(p.vx, p.vz);
            if (!p.down && (sp > 1.1f || std::fabs(p.x) > 1.05f)) p.down = true;
            if (p.down && sp < 0.16f && !(driven_ && &p == &pin_[0] && !rung_)) p.vx = p.vz = 0;
            if (std::hypot(p.x - BELL_X, p.z - BELL_Z) <= BELL_R && (p.down || sp > 0.8f)) ring();
        }
    }
    if (driven_ && !rung_) {
        driveT_ += dt;
        if (driveT_ > 0.35f) ring();
    }
    if (pocket_ && ripple_ < 0) ripple_ = 0;
    if (ripple_ >= 0) {
        ripple_ += dt;
        for (Pin& p : pin_) {
            if (p.down) continue;
            float d = std::hypot(p.homeX - entry_, p.homeZ - Z_HEAD);
            if (ripple_ <= 0.04f + d * 0.05f) continue;
            float side = p.homeX >= entry_ ? 1.0f : -1.0f;
            p.vx += side * (1.5f + 0.25f * d);
            p.vz += 2.1f;
            p.down = true;
            fresh++;
        }
    }
    for (Pin& p : pin_)
        if (p.down) p.fall = std::min(1.0f, p.fall + dt * 1.5f);
    if (fresh && !rung_) {
        sys_->apu.noiseBurst(std::min(0.55f, 0.16f + fresh * 0.07f), 800.0f + fresh * 140.0f, 0.1f);
        sys_->apu.tone(2, 150.0f + fresh * 30.0f, 0.06f);
        beep_ = std::max(beep_, 0.08f);
        shake_ = std::min(0.4f, shake_ + fresh * 0.04f);
        sys_->rumble(0.2f, 0.4f, 50);
    }
    if (gutter_ && !gutterSnd_) {
        gutterSnd_ = true;
        sys_->apu.tone(2, 80, 0.07f);
        sys_->apu.noiseBurst(0.28f, 280, 0.16f);
        beep_ = std::max(beep_, 0.12f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    if (flash_ > 0) flash_ = std::max(0.0f, flash_ - DT * 1.4f);
    if (rung_) {
        bellAmp_ = std::max(0.35f, bellAmp_ - DT * 0.12f);
        bellPh_ += DT * (9.0f + bellAmp_ * 8.0f);
    } else {
        bellPh_ += DT * 1.7f;
        bellAmp_ = 0.06f;
    }
    if (bellT_ > 0) {
        bellT_ -= DT;
        if (bellT_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    } else if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(2, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    bool action = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    float slide = 0;
    if (pad.down(gs::BTN_LEFT)) slide -= 1;
    if (pad.down(gs::BTN_RIGHT)) slide += 1;
    if (std::fabs(pad.axisX) > 0.2f) slide = pad.axisX;

    if (bot_) {
        action = false;
        start = false;
        back = false;
        slide = 0;
        if (mode_ == Mode::Title && clock_ > 0.45f) start = true;
        else if (mode_ == Mode::Approach) {
            float d = POCKET - stance_;
            if (std::fabs(d) <= 0.02f) {
                stance_ = POCKET;
                action = true;
            } else slide = d > 0 ? 1.0f : -1.0f;
        }
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) mode_ = Mode::Title;
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start || action) newGame();
    } else if (mode_ == Mode::Over) {
        if (start || action) newGame();
        else if (back) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Approach) {
        life_ -= DT;
        stance_ = clampf(stance_ + slide * 0.78f * DT, -0.72f, 0.72f);
        bool lined = onLine();
        if (lined && !wasLine_) {
            sys.apu.tone(2, 880, 0.04f);
            beep_ = std::max(beep_, 0.05f);
        }
        wasLine_ = lined;
        if (life_ <= 0) dieTry();
        else if (action) beginSwing();
    } else if (mode_ == Mode::Swing) {
        life_ -= DT;
        swingT_ += DT;
        if (bot_ && meter() >= 0.5f && swingT_ < PERIOD * 0.5f) action = true;
        bool lined = onLine();
        if (lined && !wasLine_) {
            sys.apu.tone(2, 990, 0.04f);
            beep_ = std::max(beep_, 0.05f);
        }
        wasLine_ = lined;
        if (life_ <= 0) dieTry();
        else if (action) release();
    } else if (mode_ == Mode::Roll) {
        rollT_ += DT;
        if (ballLive_ && !gutter_ && ballZ_ < Z_HEAD - 0.2f)
            hook_ = clampf(hook_ + slide * DT * 1.6f, -1.0f, 1.0f);
        physics(DT);
        if (mode_ == Mode::Roll && rollDone()) dieTry();
    } else if (mode_ == Mode::Dead) {
        deadWait_ -= DT;
        physics(DT);
        if (deadWait_ <= 0) beginApproach();
    } else if (mode_ == Mode::Ring) {
        ringWait_ -= DT;
        physics(DT);
        if (ringWait_ <= 0) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
            sys.apu.noise(0, 1000);
        }
    } else if (mode_ == Mode::Leave) {
        leaveT_ += DT;
        stance_ -= 1.45f * DT;
        if (leaveT_ > 1.15f) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Over;
        }
    }

    if (mode_ == Mode::Roll && ballLive_ && !gutter_) sys.apu.noise(0.035f, 3600, true);
    else if (mode_ != Mode::Roll) sys.apu.noise(0, 1000);

    draw();
}

void Game::project(float x, float z, float& sx, float& sy, float& ppm) const {
    z = std::max(0.35f, z);
    float row = FOCAL / z;
    sy = float(HORIZON) + row;
    ppm = row * LANE_K;
    sx = 160.0f + x * ppm;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.0f, 400.0f)));
    s.h = int16_t(std::lround(clampf(h, 1.0f, 400.0f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (w < 1.0f || h < 1.0f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.0f, 400.0f)));
    s.h = int16_t(std::lround(clampf(h, 1.0f, 400.0f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s || !s[0]) return;
    const float adv = 16.0f * scale;
    int n = int(std::strlen(s));
    x -= float(n) * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + adv * 0.5f, y, g.h * scale, pal, false);
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(clock_ * 80.0f) * 3.5f * shake_;
        shy = std::cos(clock_ * 60.0f) * 2.0f * shake_;
    }
    bool celebrate = rung_ || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = y / float(HORIZON);
            int r = int(1 + u * (celebrate ? 5 : 1) + flash_ * 6);
            int g = int(1 + u * (celebrate ? 3 : 1) + flash_ * 4);
            int b = int(2 + u * 3);
            v.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - HORIZON);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.0f + shx;
        rd.hw = row * LANE_K;
        rd.v = 48;
        rd.pal = PAL_LANE;
        rd.band = 0;
        rd.style = 1;
        rd.left = rd.right = 0;
        int fog = std::clamp(int((72.0f - row) / 20.0f), 0, 4);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(1, 1, 2);
    }

    auto place = [&](float x, float z, float& sx, float& sy, float& ppm, int& fog) {
        project(x, z, sx, sy, ppm);
        sx += shx;
        sy += shy;
        fog = std::clamp(int((z - 2.2f) * 1.1f), 0, 4);
    };

    if (mode_ == Mode::Title) text("PINSBELL", 160, 22, 1.15f, PAL_GOLD);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 22, 1.2f, PAL_AMBER);
    else if (celebrate) {
        text("BELL", 160, 18, 1.35f, PAL_GOLD);
        if (mode_ != Mode::Ring) text("LEAVE", 160, 42, 1.05f, PAL_HUD);
    } else if (mode_ == Mode::Over) text("THIRD TRY DEAD", 160, 22, 0.95f, PAL_ALERT);
    else if (mode_ == Mode::Dead) {
        char buf[16];
        std::snprintf(buf, sizeof buf, "TRY %d DEAD", dead_);
        text(buf, 160, 22, 1.05f, PAL_ALERT);
    }

    for (int i = 0; i < 3; i++) {
        int pal = i < dead_ ? PAL_DEAD : (rung_ ? PAL_GOLD : PAL_LAMP);
        float h = 16.0f;
        bool live = mode_ == Mode::Approach || mode_ == Mode::Swing || mode_ == Mode::Roll;
        if (i == dead_ && !rung_ && live) h += 2.0f * std::sin(clock_ * 5.0f);
        spr(art_.lamp, 246.0f + i * 24.0f, 20, h, pal, false);
    }

    float feetX = 160.0f + stance_ * 150.0f + shx;
    float feetY = (mode_ == Mode::Title ? 176.0f : 200.0f) + shy;
    float feetH = mode_ == Mode::Title ? 64.0f : 78.0f;
    int bp = pose();
    bool hand = mode_ == Mode::Title || mode_ == Mode::Approach || mode_ == Mode::Swing || mode_ == Mode::Pause ||
                mode_ == Mode::Dead;
    if (hand) {
        float bx = feetX + 22, by = feetY - 42;
        if (bp == 1) {
            bx = feetX + 28;
            by = feetY - 22;
        }
        spr(art_.ball[int(clock_ * 3.0f) % 3], bx, by, 16, PAL_BALL, false);
    }
    spr(art_.bowler[bp], feetX, feetY, feetH, PAL_BOWLER, false, 0, true);
    stamp(art_.shadow, feetX, feetY, 34, 8, PAL_HUD, 0, true);

    float sx, sy, ppm;
    int fog;
    if (ballLive_) {
        place(ballX_, std::max(Z_BALL, ballZ_), sx, sy, ppm, fog);
        float dia = std::max(8.0f, ppm * 0.40f);
        if (gutter_) sy += 5;
        spr(art_.ball[int(ballZ_ * 3.0f) % 3], sx, sy - dia * 0.1f, dia, PAL_BALL, false, fog);
        stamp(art_.shadow, sx, sy + dia * 0.25f, dia * 0.9f, dia * 0.28f, PAL_HUD, fog, true);
    }

    bool hot = onLine() && (mode_ == Mode::Approach || mode_ == Mode::Swing);
    if (mode_ == Mode::Approach || mode_ == Mode::Swing || mode_ == Mode::Title) {
        for (int i = 0; i < 6; i++) {
            place(aimX(), 1.85f + i * 0.22f, sx, sy, ppm, fog);
            spr(art_.dot, sx, sy, hot ? 7.0f : 5.0f, hot ? PAL_GREEN : PAL_GOLD, false, fog);
        }
    }
    const float arrows[7] = {-0.60f, -0.40f, -0.20f, 0, 0.20f, 0.40f, 0.60f};
    for (float ax : arrows) {
        place(ax, 2.35f, sx, sy, ppm, fog);
        bool bell = std::fabs(ax - POCKET) < 0.08f;
        float ah = std::max(7.0f, ppm * (bell ? 0.28f : 0.16f));
        int pal = bell ? (hot ? PAL_GREEN : PAL_GOLD) : PAL_HUD;
        spr(art_.arrow, sx, sy, ah, pal, false, fog, true);
    }
    place(0, 1.72f, sx, sy, ppm, fog);
    stamp(art_.foul, sx, sy, ppm * 2.05f, 3, PAL_ALERT, 0, false);
    for (float side : {-1.22f, 1.22f}) {
        place(side, 2.15f, sx, sy, ppm, fog);
        spr(art_.post, sx, sy, std::max(18.0f, ppm * 0.62f), PAL_POST, side < 0, fog, true);
    }

    int order[10];
    for (int i = 0; i < 10; i++) order[i] = i;
    std::sort(order, order + 10, [&](int a, int b) { return pin_[a].z < pin_[b].z; });
    for (int k = 0; k < 10; k++) {
        const Pin& p = pin_[order[k]];
        place(p.x, p.z, sx, sy, ppm, fog);
        float ph = std::max(11.0f, ppm * 0.95f);
        int pal = order[k] == 0 ? PAL_HEAD : PAL_PIN;
        if (p.fall <= 0.58f) spr(art_.pin, sx, sy, ph * (1.0f - 0.2f * p.fall), pal, false, fog, true);
        else spr(art_.pinFlat, sx, sy, ph * 0.42f, pal, p.vx < 0, fog, true);
        stamp(art_.shadow, sx, sy, ph * 0.55f, ph * 0.16f, PAL_HUD, fog, true);
    }

    place(BELL_X, BELL_Z, sx, sy, ppm, fog);
    float bh = std::max(30.0f, ppm * 1.45f);
    float swing = std::sin(bellPh_) * bellAmp_ * 14.0f;
    spr(art_.yoke, sx, sy - bh * 1.15f, bh * 0.28f, PAL_BELL, false, fog);
    spr(art_.bell, sx + swing, sy - bh * 0.62f, bh, PAL_BELL, false, fog);
    spr(art_.clapper, sx + swing * 1.35f, sy - bh * 0.22f, bh * 0.22f, PAL_BELL, false, fog);

    place(0, Z_HEAD + 3.4f * PIN_DZ, sx, sy, ppm, fog);
    stamp(art_.curtain, sx, sy - 8, std::max(70.0f, ppm * 4.4f), 24, PAL_CURTAIN, fog, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(24, "RING THE BELL", PAL_GOLD);
        hudC(25, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        hudC(26, "ARROWS AIM   C RELEASES", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "ENTER   %s", S3_VERSION_STRING);
        hudC(27, buf, PAL_AMBER);
    } else {
        hud(1, 0, "S3 PINSBELL", PAL_GOLD);
        int show = rung_ ? rungTry_ : std::min(3, dead_ + 1);
        std::snprintf(buf, sizeof buf, "TRY %d", show);
        hud(33, 0, buf, rung_ ? PAL_GOLD : PAL_AMBER);
        if (mode_ == Mode::Approach || mode_ == Mode::Swing) {
            int fill = std::clamp(int(std::lround(life_ / LIFE * 20.0f)), 0, 20);
            char bar[21];
            for (int i = 0; i < 20; i++) bar[i] = i < fill ? '=' : '.';
            bar[20] = 0;
            hudC(1, bar, life_ < 3.0f ? PAL_ALERT : PAL_AMBER);
            if (life_ < 3.0f) hudC(2, "THE TRY DIES", PAL_ALERT);
        }
        if (mode_ == Mode::Swing) {
            const int cells = 17;
            int mid = cells / 2;
            int pos = int(std::lround(meter() * (cells - 1)));
            char bar[18];
            for (int i = 0; i < cells; i++) bar[i] = (std::abs(i - mid) <= 1) ? '=' : '-';
            bar[std::clamp(pos, 0, cells - 1)] = '*';
            bar[cells] = 0;
            hudC(3, bar, hot ? PAL_GREEN : PAL_AMBER);
            hudC(4, hot ? "BELL" : "RELEASE ON THE BELL", hot ? PAL_GREEN : PAL_HUD);
        } else if (mode_ == Mode::Approach) {
            hudC(3, hot ? "BELL" : "SLIDE ONTO THE BELL", hot ? PAL_GREEN : PAL_HUD);
            hudC(4, "C STARTS THE SWING", PAL_AMBER);
        } else if (mode_ == Mode::Roll) {
            const char* msg = gutter_ ? "GUTTER" : (pocket_ ? "BELL POCKET" : "ROLL");
            hudC(3, msg, gutter_ ? PAL_ALERT : PAL_GREEN);
            hudC(4, "HOLD A SIDE TO HOOK", PAL_HUD);
        } else if (mode_ == Mode::Dead) {
            hudC(3, "NEXT TRY", PAL_AMBER);
        } else if (mode_ == Mode::Over && !won_) {
            hudC(3, "THIRD TRY DEAD", PAL_ALERT);
            hudC(4, "C AGAIN", PAL_HUD);
        }
        hud(1, 27, "C RELEASE", PAL_AMBER);
        hud(22, 27, "START PAUSE", PAL_HUD);
    }
}

}  // namespace pinsbell
