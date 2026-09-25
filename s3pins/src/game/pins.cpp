#include "game/pins.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace pins {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 52;
constexpr float FOCAL = 188.0f;
constexpr float LANE_K = 0.52f;
constexpr float Z_BALL = 1.38f;
constexpr float Z_HEAD = 3.72f;
constexpr float PIN_S = 0.50f;
constexpr float PIN_DZ = 0.58f;
constexpr float PIN_R = 0.105f;
constexpr float BALL_R = 0.19f;
constexpr float BALL_VZ = 2.85f;
constexpr float POCKET_X = 0.15f;
constexpr float POCKET_LO = 0.07f;
constexpr float POCKET_HI = 0.27f;
constexpr float MISS_SCALE = 1.15f;
constexpr float SWING_PERIOD = 1.15f;
constexpr float BALL_MASS = 3.0f;
constexpr float PIN_MASS = 1.0f;

// A mark is a strike or a spare. Either one ends the ten-frame game.
// The pockets are the gaps beside the head pin.

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
    sys.vdp.setFogColor(gs::rgb4(2, 1, 2));
    resetRack();
    mode_ = Mode::Title;
    clock_ = 0;
}

void Game::newGame() {
    frame_ = 0;
    ball_ = 0;
    stance_ = 0;
    won_ = false;
    over_ = false;
    kind_ = "open";
    markFrame_ = 1;
    pinsDown_ = 0;
    std::fill(std::begin(board_), std::end(board_), '\0');
    resetRack();
    ballLive_ = false;
    gutter_ = false;
    pocket_ = false;
    hook_ = 0;
    shake_ = 0;
    mode_ = Mode::Approach;
    wasPocket_ = false;
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
    pocket_ = false;
    entryTaken_ = false;
    rippleT_ = -1;
    hook_ = 0;
    crashed_ = 0;
}

void Game::beginSwing() {
    mode_ = Mode::Swing;
    swingT_ = 0;
    wasPocket_ = false;
}

void Game::release() {
    float miss = (meter() - 0.5f) * MISS_SCALE;
    ballX_ = clampf(stance_ + miss, -1.35f, 1.35f);
    ballZ_ = Z_BALL;
    ballVx_ = miss * 0.35f;
    ballVz_ = BALL_VZ;
    ballLive_ = true;
    gutter_ = std::fabs(ballX_) > 1.02f;
    gutterSnd_ = false;
    entryTaken_ = false;
    pocket_ = false;
    rippleT_ = -1;
    hook_ = 0;
    rollT_ = 0;
    crashed_ = 0;
    mode_ = Mode::Roll;
    blip(220);
    sys_->apu.noiseBurst(0.22f, 2400, 0.06f);
}

float Game::meter() const {
    float u = std::fmod(std::max(0.0f, swingT_), SWING_PERIOD) / SWING_PERIOD;
    return u < 0.5f ? u * 2.0f : 2.0f - u * 2.0f;
}

float Game::aimX() const {
    if (mode_ == Mode::Swing) return stance_ + (meter() - 0.5f) * MISS_SCALE;
    if (mode_ == Mode::Roll && ballLive_) return ballX_;
    return stance_;
}

float Game::aimTarget() const {
    if (ball_ == 0) return POCKET_X;
    float sum = 0;
    int n = 0;
    float nearest = 0;
    float best = 100;
    for (const Pin& p : pin_) {
        if (p.down) continue;
        sum += p.x;
        n++;
        float d = std::fabs(p.x);
        if (d < best) {
            best = d;
            nearest = p.x;
        }
    }
    if (!n) return 0;
    float span = 0;
    for (const Pin& p : pin_)
        if (!p.down) span = std::max(span, std::fabs(p.x - sum / n));
    return clampf(span < 0.45f ? sum / n : nearest, -0.78f, 0.78f);
}

int Game::standing() const {
    int n = 0;
    for (const Pin& p : pin_)
        if (!p.down) n++;
    return n;
}

bool Game::onPocket() const {
    float a = std::fabs(aimX());
    return std::fabs(a - POCKET_X) <= 0.09f && a >= POCKET_LO && a <= POCKET_HI;
}

int Game::pose() const {
    if (mode_ == Mode::Swing) return 1;
    if (mode_ == Mode::Roll && rollT_ < 0.18f) return 2;
    if (mode_ == Mode::Roll && rollT_ < 0.55f) return 3;
    if (mode_ == Mode::Mark) return 3;
    return 0;
}

void Game::finishMark(bool strike) {
    for (Pin& p : pin_) {
        p.down = true;
        if (p.fall < 0.25f) p.fall = 0.25f;
    }
    pinsDown_ = 10;
    kind_ = strike ? "strike" : "spare";
    markFrame_ = frame_ + 1;
    board_[frame_] = strike ? 'X' : '/';
    mode_ = Mode::Mark;
    markT_ = 0;
    ballLive_ = false;
    shake_ = 0.35f;
    sys_->apu.tone(0, 523, 0.12f);
    sys_->apu.tone(1, 659, 0.10f);
    sys_->apu.tone(2, 784, 0.10f);
    beep_ = 0.7f;
    sys_->rumble(0.4f, 0.8f, 180);
    sys_->setLight(255, 196, 40);
}

void Game::judge() {
    ballLive_ = false;
    sys_->apu.noise(0, 1000, false);
    pinsDown_ = 10 - standing();
    if (standing() == 0) {
        finishMark(ball_ == 0);
        return;
    }
    if (ball_ == 0) {
        ball_ = 1;
        mode_ = Mode::Hold;
        wait_ = 0.7f;
        return;
    }
    int knocked = std::clamp(pinsDown_, 0, 9);
    board_[frame_] = char('0' + knocked);
    if (frame_ >= 9) {
        kind_ = "open";
        markFrame_ = 10;
        won_ = false;
        mode_ = Mode::Over;
        over_ = true;
        sys_->setLight(140, 20, 20);
        return;
    }
    frame_++;
    ball_ = 0;
    resetRack();
    mode_ = Mode::Hold;
    wait_ = 0.55f;
}

void Game::physics(float dt) {
    const float h = dt / 5.0f;
    int fresh = 0;
    for (int step = 0; step < 5; step++) {
        if (ballLive_ && !gutter_) {
            float along = (ballZ_ - Z_BALL) / std::max(0.2f, Z_HEAD - Z_BALL);
            if (along > 0.32f && along < 1.05f) ballVx_ += hook_ * 5.0f * h;
        }
        if (ballLive_) {
            float px = ballX_, pz = ballZ_;
            ballX_ += ballVx_ * h;
            ballZ_ += ballVz_ * h;
            ballVx_ *= (1.0f - 0.35f * h);
            if (!gutter_ && std::fabs(ballX_) > 1.02f && ballZ_ < Z_HEAD + 0.5f) gutter_ = true;
            if (gutter_) {
                ballX_ = std::copysign(1.15f, ballX_);
                ballVz_ *= (1.0f - 1.4f * h);
            }
            if (!entryTaken_ && pz < Z_HEAD && ballZ_ >= Z_HEAD) {
                float u = (ballZ_ - pz) > 1e-5f ? (Z_HEAD - pz) / (ballZ_ - pz) : 1.0f;
                entry_ = px + (ballX_ - px) * u;
                entryTaken_ = true;
                float ax = std::fabs(entry_);
                pocket_ = !gutter_ && ax >= POCKET_LO && ax <= POCKET_HI && ballVz_ > 1.4f;
            }
            if (!gutter_) {
                for (Pin& p : pin_) {
                    bool was = p.down;
                    collide(ballX_, ballZ_, ballVx_, ballVz_, BALL_R, BALL_MASS, p.x, p.z, p.vx, p.vz, PIN_R, PIN_MASS,
                            0.25f);
                    float sp = std::hypot(p.vx, p.vz);
                    if (sp > 9.0f) {
                        p.vx *= 9.0f / sp;
                        p.vz *= 9.0f / sp;
                    }
                    if (!p.down && (sp > 1.25f || std::hypot(p.x - p.homeX, p.z - p.homeZ) > 0.32f)) p.down = true;
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
                collide(pa.x, pa.z, pa.vx, pa.vz, PIN_R, PIN_MASS, pb.x, pb.z, pb.vx, pb.vz, PIN_R, PIN_MASS, 0.55f);
                for (Pin* p : {&pa, &pb}) {
                    float sp = std::hypot(p->vx, p->vz);
                    if (!p->down && (sp > 1.15f || std::hypot(p->x - p->homeX, p->z - p->homeZ) > 0.30f)) p->down = true;
                }
                if (pa.down && !wa) fresh++;
                if (pb.down && !wb) fresh++;
            }
        }
        for (Pin& p : pin_) {
            if (std::hypot(p.vx, p.vz) < 0.04f && !p.down) continue;
            p.x += p.vx * h;
            p.z += p.vz * h;
            float drag = 1.0f - 2.2f * h;
            p.vx *= drag;
            p.vz *= drag;
            p.x = clampf(p.x, -2.4f, 2.4f);
            p.z = clampf(p.z, 0.8f, 8.5f);
            float sp = std::hypot(p.vx, p.vz);
            if (!p.down && (sp > 1.15f || std::fabs(p.x) > 1.05f || p.z > Z_HEAD + 3.4f * PIN_DZ)) p.down = true;
            if (p.down && sp < 0.18f) p.vx = p.vz = 0;
        }
    }
    if (pocket_ && rippleT_ < 0) rippleT_ = 0;
    if (rippleT_ >= 0) {
        rippleT_ += dt;
        for (Pin& p : pin_) {
            if (p.down) continue;
            float d = std::hypot(p.homeX - entry_, p.homeZ - Z_HEAD);
            if (rippleT_ <= d * 0.07f) continue;
            float side = (p.homeX >= entry_) ? 1.0f : -1.0f;
            if (std::fabs(p.homeX - entry_) < 0.08f) side = (p.homeX >= 0) ? 1.0f : -1.0f;
            p.vx += side * (2.4f + 0.35f * d);
            p.vz += 2.6f;
            p.down = true;
            fresh++;
        }
    }
    for (Pin& p : pin_)
        if (p.down) p.fall = std::min(1.0f, p.fall + dt * 3.2f);
    if (fresh) {
        crashed_ += fresh;
        sys_->apu.noiseBurst(std::min(0.7f, 0.25f + fresh * 0.08f), 900.0f + fresh * 180.0f, 0.12f);
        sys_->apu.tone(2, 140.0f + crashed_ * 28.0f, 0.08f);
        beep_ = std::max(beep_, 0.09f);
        shake_ = std::min(0.45f, shake_ + fresh * 0.05f);
        sys_->rumble(0.25f, 0.55f, 70);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.06f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);

    const gs::Pad& pad = sys.pad;
    bool action = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    float slide = 0;
    if (pad.down(gs::BTN_LEFT)) slide -= 1;
    if (pad.down(gs::BTN_RIGHT)) slide += 1;
    if (std::fabs(pad.axisX) > 0.18f) slide = pad.axisX;

    if (bot_) {
        action = false;
        start = false;
        back = false;
        slide = 0;
        if (mode_ == Mode::Title && clock_ > 0.55f) start = true;
        else if (mode_ == Mode::Approach) {
            float d = aimTarget() - stance_;
            if (std::fabs(d) < 0.012f) action = true;
            else slide = d > 0 ? 1.0f : -1.0f;
        } else if (mode_ == Mode::Swing && swingT_ > 0.05f && swingT_ < SWING_PERIOD * 0.5f && meter() >= 0.5f)
            action = true;
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
        }
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Approach) {
        stance_ = clampf(stance_ + slide * 0.72f * DT, -0.78f, 0.78f);
        if (action) beginSwing();
    } else if (mode_ == Mode::Swing) {
        swingT_ += DT;
        bool lined = onPocket();
        if (lined && !wasPocket_) blip(980);
        wasPocket_ = lined;
        if (action) release();
    } else if (mode_ == Mode::Roll) {
        rollT_ += DT;
        if (ballLive_ && !gutter_ && ballZ_ < Z_HEAD - 0.05f)
            hook_ = clampf(hook_ + slide * DT * 2.2f, -1.0f, 1.0f);
        physics(DT);
        if (gutter_ && !gutterSnd_) {
            gutterSnd_ = true;
            blip(90);
            sys.apu.noiseBurst(0.3f, 300, 0.18f);
        }
        if (ballLive_ && !gutter_) sys.apu.noise(0.035f, 4200, false);
        bool quiet = true;
        for (const Pin& p : pin_)
            if (std::hypot(p.vx, p.vz) > 0.35f) quiet = false;
        bool ballDone = (gutter_ && (ballZ_ > Z_HEAD || ballVz_ < 0.4f)) || ballZ_ > Z_HEAD + 3.3f * PIN_DZ + 0.45f ||
                        (pocket_ && rippleT_ > 0.48f) || rollT_ > 2.8f;
        if (ballDone && (standing() == 0 || quiet || rollT_ > 2.8f)) judge();
    } else if (mode_ == Mode::Hold) {
        wait_ -= DT;
        if (wait_ <= 0) mode_ = Mode::Approach;
    } else if (mode_ == Mode::Mark) {
        physics(DT);
        markT_ += DT;
        if (markT_ > 1.05f) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Over;
        }
    }

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
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (w < 1.0f || h < 1.0f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.0f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + adv * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(clock_ * 90.0f) * 4.0f * shake_;
        shy = std::cos(clock_ * 70.0f) * 2.5f * shake_;
    }
    bool win = mode_ == Mode::Mark || (mode_ == Mode::Over && won_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            float u = y / float(HORIZON);
            int r = int(1 + u * (win ? 6 : 3));
            int g = int(1 + u * (win ? 3 : 1));
            int b = int(2 + u * 2);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - HORIZON);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.0f + shx;
        rd.hw = row * LANE_K;
        rd.v = 40;
        rd.pal = PAL_LANE;
        rd.band = 0;
        rd.style = 1;
        rd.left = rd.right = 0;
        int fog = std::clamp(int((78.0f - row) / 12.0f), 0, 6);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(2, 1, 2);
    }

    auto place = [&](float x, float z, float& sx, float& sy, float& ppm, int& fog) {
        project(x, z, sx, sy, ppm);
        sx += shx;
        sy += shy;
        fog = std::clamp(int((z - 1.6f) * 1.4f), 0, 8);
    };

    // Earlier sprites draw on top. Banners, the bowler, and the ball go in first.
    if (mode_ == Mode::Title) text("S3 PINS", 160, 16, 1.35f, PAL_GOLD);
    else if (win) {
        text("MARK", 160, 34, 1.8f, PAL_GOLD);
        text(std::strcmp(kind_, "spare") == 0 ? "SPARE" : "STRIKE", 160, 62, 1.15f, PAL_HUD);
    } else if (mode_ == Mode::Over) text("OPEN", 160, 40, 1.7f, PAL_AMBER);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 40, 1.4f, PAL_AMBER);

    float feetX = 160.0f + stance_ * (FOCAL / 1.5f) * LANE_K + shx;
    float feetY = 206.0f + shy;
    int bp = pose();
    if (mode_ == Mode::Title) {
        feetX = 78;
        feetY = 186;
        bp = 0;
    }
    bool hand = !ballLive_ && mode_ != Mode::Hold && mode_ != Mode::Mark && mode_ != Mode::Over &&
                !(mode_ == Mode::Roll && rollT_ >= 0.18f);
    if (mode_ == Mode::Title) hand = true;
    if (hand) {
        float bx = feetX + 18, by = feetY - 34;
        if (bp == 1) {
            bx = feetX + 26;
            by = feetY - 24;
        } else if (bp == 2) {
            bx = feetX + 10;
            by = feetY - 56;
        }
        spr(art_.ball[int(clock_ * 3.0f) & 3], bx, by, 16, PAL_BALL, false);
    }
    float csx, csy, cppm;
    int cfog;
    if (ballLive_) {
        place(ballX_, std::max(Z_BALL, ballZ_), csx, csy, cppm, cfog);
        float dia = std::max(8.0f, cppm * 0.40f);
        if (gutter_) csy += 6;
        spr(art_.ball[int(ballZ_ * 4.0f) & 3], csx, csy - dia * 0.15f, dia, PAL_BALL, false, cfog, false);
        stamp(art_.shadow, csx, csy + dia * 0.35f, dia * 0.9f, dia * 0.28f, PAL_HUD, cfog, true);
    }
    spr(art_.bowler[bp], feetX, feetY, mode_ == Mode::Title ? 68.0f : 76.0f, PAL_BOWLER, false, 0, true);
    stamp(art_.shadow, feetX, feetY, 36, 8, PAL_HUD, 0, true);

    int order[10];
    for (int i = 0; i < 10; i++) order[i] = i;
    std::sort(order, order + 10, [&](int a, int b) { return pin_[a].z < pin_[b].z; });
    for (int k = 0; k < 10; k++) {
        const Pin& p = pin_[order[k]];
        float sx, sy, ppm;
        int fog;
        place(p.x, p.z, sx, sy, ppm, fog);
        float ph = std::max(12.0f, ppm * 0.82f);
        bool flat = p.fall > 0.4f;
        if (!flat) spr(art_.pin, sx, sy, ph * (1.0f - 0.25f * p.fall), PAL_PIN, false, fog, true);
        else spr(art_.pinFlat, sx, sy - 1.0f, ph * 0.40f, PAL_PIN, p.vx < 0, fog, true);
        stamp(art_.shadow, sx, sy, ph * 0.7f, ph * 0.18f, PAL_HUD, fog, true);
    }

    bool hot = onPocket();
    if (mode_ == Mode::Approach || mode_ == Mode::Swing) {
        for (int i = 0; i < 7; i++) {
            float z = 1.75f + i * 0.28f;
            place(aimX(), z, csx, csy, cppm, cfog);
            spr(art_.dot, csx, csy, hot ? 7.0f : 5.0f, hot ? PAL_GREEN : PAL_GOLD, false, cfog, false);
        }
    }
    float pulse = 1.0f + 0.12f * std::sin(clock_ * 7.0f);
    for (float px : {-POCKET_X, POCKET_X}) {
        place(px, 2.15f, csx, csy, cppm, cfog);
        float ah = std::max(8.0f, cppm * 0.22f) * ((hot && mode_ != Mode::Title) ? pulse : 1.0f);
        spr(art_.arrow, csx, csy, ah, hot && mode_ != Mode::Title ? PAL_GREEN : PAL_AMBER, false, cfog, true);
    }
    if (mode_ != Mode::Title) {
        for (float ax : {-0.45f, -0.30f, -0.15f, 0.0f, 0.15f, 0.30f, 0.45f}) {
            place(ax, 2.45f, csx, csy, cppm, cfog);
            spr(art_.arrow, csx, csy, std::max(6.0f, cppm * 0.16f), PAL_HUD, false, cfog, true);
        }
    }
    place(0, 1.62f, csx, csy, cppm, cfog);
    stamp(art_.foul, csx, csy, cppm * 2.05f, 3, PAL_HUD, 0, false);
    place(0, Z_HEAD + 0.9f, csx, csy, cppm, cfog);
    stamp(art_.glow, csx, csy, cppm * 2.4f, cppm * 0.55f, PAL_GLOW, 2, false);
    for (float side : {-1.28f, 1.28f}) {
        place(side, 2.15f, csx, csy, cppm, cfog);
        spr(art_.ret, csx, csy, std::max(18.0f, cppm * 0.85f), PAL_RETURN, side < 0, cfog, true);
    }
    place(0, Z_HEAD + 3.2f * PIN_DZ, csx, csy, cppm, cfog);
    stamp(art_.curtain, csx, csy - 6, cppm * 2.6f, 22, PAL_CURTAIN, cfog, false);
    for (int i = 0; i < 3; i++) spr(art_.lamp, 48.0f + i * 112.0f, 20, 26, PAL_LAMP, false);

    if (mode_ != Mode::Title) {
        hud(1, 0, "S3 PINS", PAL_GOLD);
        char buf[24];
        std::snprintf(buf, sizeof buf, "FRAME %d", std::min(10, frame_ + 1));
        hud(32, 0, buf, PAL_AMBER);
        int col = 5;
        bool playing = mode_ != Mode::Over;
        for (int i = 0; i < 10; i++) {
            char lab[4];
            if (i < 9) std::snprintf(lab, sizeof lab, " %d ", i + 1);
            else std::snprintf(lab, sizeof lab, "10 ");
            char sym = board_[i];
            if (!sym) sym = (playing && i == frame_) ? ((int(clock_ * 3) & 1) ? '>' : '-') : '.';
            char cell[4];
            std::snprintf(cell, sizeof cell, " %c ", sym);
            int pal = (i == frame_) ? PAL_AMBER : PAL_HUD;
            if (sym == 'X' || sym == '/') pal = PAL_GOLD;
            hud(col, 1, lab, i == frame_ ? PAL_AMBER : PAL_HUD);
            hud(col, 2, cell, pal);
            col += 3;
        }
        if (mode_ == Mode::Swing) {
            const int cells = 17;
            int mid = cells / 2;
            int pos = int(std::lround(meter() * (cells - 1)));
            std::string bar(cells, '-');
            for (int i = 0; i < cells; i++)
                if (i - mid <= 1 && mid - i <= 1) bar[size_t(i)] = '=';
            bar[size_t(std::clamp(pos, 0, cells - 1))] = '*';
            hudC(3, bar, onPocket() ? PAL_GREEN : PAL_AMBER);
            hudC(4, onPocket() ? "POCKET" : "RELEASE ON THE LINE", onPocket() ? PAL_GREEN : PAL_HUD);
        } else if (mode_ == Mode::Approach) {
            hudC(3, onPocket() ? "POCKET" : "SLIDE THE DOTS ONTO A POCKET", onPocket() ? PAL_GREEN : PAL_HUD);
            hudC(4, "C STARTS THE SWING", PAL_AMBER);
        } else if (mode_ == Mode::Roll) {
            std::snprintf(buf, sizeof buf, "DOWN %d", 10 - standing());
            hudC(3, gutter_ ? "GUTTER" : (pocket_ ? "POCKET" : buf), gutter_ ? PAL_AMBER : PAL_GREEN);
            hudC(4, "HOLD A SIDE TO HOOK", PAL_HUD);
        } else if (mode_ == Mode::Hold) {
            std::snprintf(buf, sizeof buf, "LEFT %d", standing());
            hudC(3, ball_ == 1 ? "SECOND BALL" : "NEXT FRAME", PAL_AMBER);
            hudC(4, buf, PAL_HUD);
        }
        std::snprintf(buf, sizeof buf, "BALL %d", ball_ + 1);
        hud(1, 26, buf, PAL_HUD);
        hud(1, 27, "C", PAL_AMBER);
        hud(3, 27, mode_ == Mode::Swing ? "RELEASE" : "SWING", PAL_HUD);
        hud(28, 27, "START PAUSE", PAL_HUD);
        if (mode_ == Mode::Over && !won_) hudC(9, "NO MARK IN TEN", PAL_HUD);
    } else {
        hudC(5, "TEN FRAMES", PAL_AMBER);
        hudC(6, "KNOCK THEM DOWN", PAL_HUD);
        hudC(7, "A MARK FINISHES THE GAME", PAL_GOLD);
        hudC(8, "SLIDE   C TIMES THE LINE   L R HOOKS", PAL_HUD);
        hudC(27, std::string("START    ") + S3_VERSION_STRING, PAL_AMBER);
    }
}

}  // namespace pins
