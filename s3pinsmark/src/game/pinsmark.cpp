#include "game/pinsmark.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace pinsmark {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 76;
constexpr float FOCAL = 200.0f;
constexpr float LANE_K = 0.60f;
constexpr float Z_BALL = 1.42f;
constexpr float Z_HEAD = 4.05f;
constexpr float PIN_S = 0.50f;
constexpr float PIN_DZ = 0.46f;
constexpr float PIN_R = 0.11f;
constexpr float BALL_R = 0.17f;
constexpr float BALL_VZ = 7.0f;
constexpr float POCKET_X = 0.15f;
constexpr float POCKET_LO = 0.08f;
constexpr float POCKET_HI = 0.22f;
constexpr float LINE = 0.05f;
constexpr float MISS = 0.50f;
constexpr float SWING_PERIOD = 1.0f;
constexpr float BALL_MASS = 6.0f;
constexpr float PIN_MASS = 1.2f;
constexpr float Z_POCKET = Z_HEAD - 0.55f;
constexpr float Z_BACK = Z_HEAD + 3.0f * PIN_DZ;

const float PIN_K[10] = {0, -0.5f, 0.5f, -1.f, 0, 1.f, -1.5f, -0.5f, 0.5f, 1.5f};
const int PIN_ROW[10] = {0, 1, 1, 2, 2, 2, 3, 3, 3, 3};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

int clampI(float v, int lo, int hi) {
    int i = int(std::lround(v));
    if (i < lo) return lo;
    if (i > hi) return hi;
    return i;
}

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
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 4, 2));
    sys.setLight(40, 24, 16);
    clock_ = 0;
    newGame();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    rolling_ = false;
}

void Game::newGame() {
    frame_ = 0;
    ball_ = 0;
    stance_ = 0;
    marked_ = false;
    strike_ = false;
    bonusN_ = 0;
    bonus_[0] = bonus_[1] = 0;
    markFrame_ = 1;
    framePins_ = 0;
    armed_ = 10;
    won_ = false;
    over_ = false;
    rolling_ = false;
    after_ = After::None;
    hook_ = 0;
    shake_ = 0;
    ballLive_ = false;
    std::fill(std::begin(board_), std::end(board_), '\0');
    resetRack();
    mode_ = Mode::Aim;
}

void Game::resetRack() {
    for (int i = 0; i < 10; i++) {
        Pin& p = pin_[i];
        p.row = PIN_ROW[i];
        p.homeX = PIN_K[i] * PIN_S;
        p.homeZ = Z_HEAD + PIN_ROW[i] * PIN_DZ;
        p.x = p.homeX;
        p.z = p.homeZ;
        p.vx = p.vz = 0;
        p.down = false;
        p.swept = false;
        p.fall = 0;
    }
    ballLive_ = false;
    gutter_ = false;
    pocket_ = false;
    sent_ = false;
    hook_ = 0;
}

void Game::sweepDead() {
    for (Pin& p : pin_) {
        if (!p.down) continue;
        p.swept = true;
        p.vx = p.vz = 0;
    }
}

void Game::beginSwing() {
    mode_ = Mode::Swing;
    swingT_ = 0;
    wasLined_ = false;
}

void Game::release() {
    float miss = (meter() - 0.5f) * MISS;
    float target = aimTarget();
    ballX_ = clampf(stance_ + miss, -1.30f, 1.30f);
    ballVx_ = miss * 0.45f;
    fullRack_ = standing() == 10;
    if (bot_ && std::fabs(ballX_ - target) <= 0.12f) {
        ballX_ = target;
        ballVx_ = 0;
    }
    ballZ_ = Z_BALL;
    ballVz_ = BALL_VZ;
    ballLive_ = true;
    gutter_ = std::fabs(ballX_) > 1.02f;
    pocket_ = false;
    sent_ = false;
    hook_ = 0;
    rollT_ = 0;
    armed_ = standing();
    rolling_ = true;
    mode_ = Mode::Roll;
    blip(180);
    sys_->apu.noiseBurst(0.18f, 1800, 0.05f);
}

float Game::meter() const {
    float u = std::fmod(std::max(0.0f, swingT_), SWING_PERIOD) / SWING_PERIOD;
    return u < 0.5f ? u * 2.0f : 2.0f - u * 2.0f;
}

float Game::aimX() const {
    if (mode_ == Mode::Swing) return stance_ + (meter() - 0.5f) * MISS;
    if (mode_ == Mode::Roll && ballLive_) return ballX_;
    return stance_;
}

float Game::aimTarget() const {
    if (marked_ || ball_ == 0) return POCKET_X;
    float sum = 0;
    int n = 0;
    for (const Pin& p : pin_) {
        if (p.down || p.swept) continue;
        sum += p.x;
        n++;
    }
    if (!n) return POCKET_X;
    return clampf(sum / float(n), -0.80f, 0.80f);
}

int Game::standing() const {
    int n = 0;
    for (const Pin& p : pin_)
        if (!p.down && !p.swept) n++;
    return n;
}

bool Game::lined() const { return std::fabs(aimX() - aimTarget()) <= LINE; }

int Game::pose() const {
    if (mode_ == Mode::Swing) return 1;
    if (mode_ == Mode::Roll && rollT_ < 0.45f) return 2;
    return 0;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = std::max(beep_, 0.05f);
}

void Game::writeCount(char* dst, int n) const {
    if (!marked_) {
        std::snprintf(dst, size_t(n), "NO MARK");
        return;
    }
    if (strike_) {
        if (bonusN_ <= 0) std::snprintf(dst, size_t(n), "10+_+_");
        else if (bonusN_ == 1) std::snprintf(dst, size_t(n), "10+%d+_", bonus_[0]);
        else std::snprintf(dst, size_t(n), "10+%d+%d", bonus_[0], bonus_[1]);
    } else if (bonusN_ <= 0) {
        std::snprintf(dst, size_t(n), "10+_");
    } else {
        std::snprintf(dst, size_t(n), "10+%d", bonus_[0]);
    }
}

void Game::closeMark() {
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    ballLive_ = false;
    beep_ = 0.9f;
    sys_->apu.tone(0, 392, 0.12f);
    sys_->apu.tone(1, 523, 0.10f);
    sys_->apu.tone(2, 659, 0.09f);
    sys_->rumble(0.35f, 0.85f, 220);
    sys_->setLight(255, 210, 70);
}

void Game::openLoss() {
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    ballLive_ = false;
    markFrame_ = 10;
    beep_ = 0.45f;
    sys_->apu.tone(0, 110, 0.10f);
    sys_->setLight(140, 24, 20);
}

void Game::judge() {
    ballLive_ = false;
    sys_->apu.noise(0, 1000, false);
    int left = standing();
    int got = std::max(0, armed_ - left);
    if (marked_) {
        if (bonusN_ < 2) bonus_[bonusN_] = got;
        bonusN_++;
        int need = strike_ ? 2 : 1;
        if (bonusN_ >= need) {
            closeMark();
            return;
        }
        after_ = After::Fill;
        mode_ = Mode::Hold;
        wait_ = 0.40f;
        return;
    }
    if (left == 0) {
        strike_ = ball_ == 0;
        marked_ = true;
        markFrame_ = frame_ + 1;
        board_[frame_] = strike_ ? 'X' : '/';
        bonusN_ = 0;
        bonus_[0] = bonus_[1] = 0;
        after_ = After::Fill;
        mode_ = Mode::Hold;
        wait_ = 0.50f;
        beep_ = 0.40f;
        sys_->apu.tone(0, 330, 0.08f);
        sys_->apu.tone(1, 415, 0.07f);
        sys_->setLight(200, 110, 30);
        sys_->rumble(0.25f, 0.55f, 120);
        return;
    }
    framePins_ += got;
    if (ball_ == 0) {
        ball_ = 1;
        after_ = After::Second;
        mode_ = Mode::Hold;
        wait_ = 0.40f;
        return;
    }
    board_[frame_] = char('0' + std::min(framePins_, 9));
    after_ = After::Frame;
    mode_ = Mode::Hold;
    wait_ = 0.42f;
}

void Game::commit() {
    After why = after_;
    after_ = After::None;
    if (why == After::Fill) {
        ball_ = 0;
        resetRack();
        mode_ = Mode::Aim;
        return;
    }
    if (why == After::Second) {
        sweepDead();
        mode_ = Mode::Aim;
        return;
    }
    if (why == After::Frame) {
        frame_++;
        framePins_ = 0;
        ball_ = 0;
        if (frame_ >= 10) {
            openLoss();
            return;
        }
        resetRack();
        mode_ = Mode::Aim;
    }
}

int Game::physics(float dt) {
    const float h = dt / 6.0f;
    int fresh = 0;
    for (int step = 0; step < 6; step++) {
        float prevX = ballX_, prevZ = ballZ_;
        if (ballLive_ && !gutter_ && ballZ_ < Z_POCKET) ballVx_ += hook_ * 2.8f * h;
        if (ballLive_) {
            ballX_ += ballVx_ * h;
            ballZ_ += ballVz_ * h;
            ballVx_ *= (1.0f - 0.45f * h);
            if (!gutter_ && std::fabs(ballX_) > 1.02f && ballZ_ < Z_HEAD) gutter_ = true;
            if (gutter_) {
                ballX_ = std::copysign(1.16f, ballX_ == 0 ? 1.0f : ballX_);
                ballVz_ *= (1.0f - 1.6f * h);
            }
            if (!sent_ && prevZ < Z_POCKET && ballZ_ >= Z_POCKET) {
                float u = (ballZ_ - prevZ) > 1e-5f ? (Z_POCKET - prevZ) / (ballZ_ - prevZ) : 1.0f;
                entry_ = prevX + (ballX_ - prevX) * u;
                sent_ = true;
                bool right = entry_ >= POCKET_LO && entry_ <= POCKET_HI;
                bool left = entry_ <= -POCKET_LO && entry_ >= -POCKET_HI;
                pocket_ = fullRack_ && !gutter_ && (right || left);
                if (pocket_) {
                    for (Pin& p : pin_) {
                        if (p.swept || p.down) continue;
                        float side = (p.homeX >= entry_) ? 1.0f : -1.0f;
                        if (std::fabs(p.homeX - entry_) < 0.06f) side = p.homeX >= 0 ? 1.0f : -1.0f;
                        p.vx = side * (2.1f + p.row * 0.35f);
                        p.vz = 1.5f + p.row * 0.40f;
                        p.down = true;
                        fresh++;
                    }
                }
            }
            if (!gutter_) {
                for (Pin& p : pin_) {
                    if (p.swept) continue;
                    bool was = p.down;
                    collide(ballX_, ballZ_, ballVx_, ballVz_, BALL_R, BALL_MASS, p.x, p.z, p.vx, p.vz, PIN_R, PIN_MASS,
                            0.35f);
                    float sp = std::hypot(p.vx, p.vz);
                    if (sp > 8.0f) {
                        p.vx *= 8.0f / sp;
                        p.vz *= 8.0f / sp;
                    }
                    if (!p.down && (sp > 0.85f || std::hypot(p.x - p.homeX, p.z - p.homeZ) > 0.26f)) p.down = true;
                    if (p.down && !was) fresh++;
                }
                float spb = std::hypot(ballVx_, ballVz_);
                if (spb > 9.0f) {
                    ballVx_ *= 9.0f / spb;
                    ballVz_ *= 9.0f / spb;
                }
            }
        }
        for (int a = 0; a < 10; a++) {
            for (int b = a + 1; b < 10; b++) {
                Pin& pa = pin_[a];
                Pin& pb = pin_[b];
                if (pa.swept || pb.swept) continue;
                if (std::hypot(pa.vx, pa.vz) < 0.04f && std::hypot(pb.vx, pb.vz) < 0.04f && !pa.down && !pb.down)
                    continue;
                bool wa = pa.down, wb = pb.down;
                collide(pa.x, pa.z, pa.vx, pa.vz, PIN_R, PIN_MASS, pb.x, pb.z, pb.vx, pb.vz, PIN_R, PIN_MASS, 0.40f);
                for (Pin* p : {&pa, &pb}) {
                    float sp = std::hypot(p->vx, p->vz);
                    if (!p->down && (sp > 0.8f || std::hypot(p->x - p->homeX, p->z - p->homeZ) > 0.26f)) p->down = true;
                }
                if (pa.down && !wa) fresh++;
                if (pb.down && !wb) fresh++;
            }
        }
        for (Pin& p : pin_) {
            if (p.swept) continue;
            if (std::hypot(p.vx, p.vz) < 0.03f && !p.down) continue;
            p.x += p.vx * h;
            p.z += p.vz * h;
            float drag = 1.0f - 5.5f * h;
            p.vx *= drag;
            p.vz *= drag;
            p.x = clampf(p.x, -2.2f, 2.2f);
            p.z = clampf(p.z, 0.8f, 9.0f);
            float sp = std::hypot(p.vx, p.vz);
            if (!p.down && (sp > 0.8f || std::fabs(p.x) > 1.02f || p.z > Z_BACK + 0.8f)) p.down = true;
            if (p.down && sp < 0.16f) p.vx = p.vz = 0;
        }
    }
    for (Pin& p : pin_)
        if (p.down) p.fall = std::min(1.0f, p.fall + dt * 3.4f);
    return fresh;
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
        if (mode_ == Mode::Title && clock_ > 0.40f) action = true;
        else if (mode_ == Mode::Aim) {
            float d = aimTarget() - stance_;
            if (std::fabs(d) < 0.012f) action = true;
            else slide = d > 0 ? 1.0f : -1.0f;
        }
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) {
            mode_ = Mode::Title;
            over_ = false;
            rolling_ = false;
        }
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start || action) newGame();
    } else if (mode_ == Mode::Over) {
        if (start || action) newGame();
        else if (back) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            rolling_ = false;
        }
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        stance_ = clampf(stance_ + slide * 1.25f * DT, -0.82f, 0.82f);
        if (action) beginSwing();
    } else if (mode_ == Mode::Swing) {
        swingT_ += DT;
        bool on = lined();
        if (on && !wasLined_) blip(920);
        wasLined_ = on;
        if (action || (bot_ && swingT_ >= 0.25f)) release();
    } else if (mode_ == Mode::Roll) {
        rollT_ += DT;
        if (ballLive_ && !gutter_ && ballZ_ < Z_POCKET) hook_ = clampf(hook_ + slide * DT * 2.4f, -1.0f, 1.0f);
        int fresh = physics(DT);
        if (fresh) {
            sys.apu.noiseBurst(std::min(0.65f, 0.22f + fresh * 0.07f), 800.0f + fresh * 140.0f, 0.10f);
            sys.apu.tone(2, 120.0f + fresh * 18.0f, 0.06f);
            beep_ = std::max(beep_, 0.07f);
            shake_ = std::min(0.5f, shake_ + fresh * 0.05f);
            sys.rumble(0.2f, 0.5f, 60);
        }
        if (gutter_ && rollT_ > 0.08f && rollT_ < 0.12f) blip(80);
        bool quiet = true;
        for (const Pin& p : pin_)
            if (!p.swept && std::hypot(p.vx, p.vz) > 0.55f) quiet = false;
        bool past = ballZ_ > Z_BACK + 0.25f;
        bool crashed = standing() == 0 && ballZ_ > Z_HEAD && rollT_ > 0.40f;
        bool gutterDone = gutter_ && rollT_ > 0.48f;
        bool settled = (past || rollT_ > 2.05f) && (quiet || rollT_ > 1.20f);
        if (crashed || gutterDone || settled) judge();
    } else if (mode_ == Mode::Hold) {
        physics(DT);
        wait_ -= DT;
        if (wait_ <= 0) commit();
    }

    draw();
}

void Game::project(float x, float z, float& sx, float& sy, float& ppm) const {
    z = std::max(0.45f, z);
    float row = FOCAL / z;
    sy = float(HORIZON) + row;
    ppm = row * LANE_K;
    sx = 160.0f + x * ppm;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(clampI(w, 1, 400));
    s.h = int16_t(clampI(h, 1, 400));
    s.x = int16_t(clampI(cx - s.w * 0.5f, -300, 500));
    s.y = int16_t(clampI(feet ? cy - s.h : cy - s.h * 0.5f, -300, 500));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (w < 1.0f || h < 1.0f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(clampI(w, 1, 500));
    s.h = int16_t(clampI(h, 1, 400));
    s.x = int16_t(clampI(cx - s.w * 0.5f, -300, 500));
    s.y = int16_t(clampI(cy - s.h * 0.5f, -300, 500));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w < 1 || img.h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(clampI(cx - img.w * 0.5f, -300, 500));
    s.y = int16_t(clampI(cy - img.h * 0.5f, -300, 500));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127) continue;
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
        shx = std::sin(clock_ * 80.0f) * 3.5f * shake_;
        shy = std::cos(clock_ * 60.0f) * 2.0f * shake_;
    }
    bool win = mode_ == Mode::Over && won_;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = y / float(HORIZON);
            int r = 1 + int(u * (win ? 5 : 2));
            int g = 1 + int(u * (win ? 3 : 1));
            int b = 3 + int(u * 2);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - HORIZON);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.0f + shx * 0.25f;
        rd.hw = row * LANE_K;
        rd.v = 140;
        rd.pal = PAL_LANE;
        rd.band = 0;
        rd.style = 0;
        rd.left = rd.right = 0;
        int fog = std::clamp(int((90.0f - row) / 18.0f), 0, 5);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(3, 2, 2);
    }

    auto place = [&](float x, float z, float& sx, float& sy, float& ppm, int& fog) {
        project(x, z, sx, sy, ppm);
        sx += shx;
        sy += shy;
        fog = std::clamp(int((z - 1.5f) * 1.3f), 0, 7);
    };

    for (int i = 0; i < 3; i++) spr(art_.lamp, 70.0f + i * 90.0f, 16, 18, PAL_LAMP, false);

    if (mode_ == Mode::Title) word(art_.title, 160, 38, PAL_BRASS);
    else if (win) {
        word(art_.finished, 160, 30, PAL_BRASS);
        word(art_.leave, 160, 52, PAL_OK);
    } else if (mode_ == Mode::Over) {
        word(art_.open, 160, 36, PAL_BAD);
    } else if (marked_ && (mode_ == Mode::Aim || mode_ == Mode::Hold || mode_ == Mode::Swing)) {
        word(art_.markOpen, 160, 34, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        word(art_.title, 160, 36, PAL_PAPER);
    }

    float csx, csy, cppm;
    int cfog;
    float feetX = 160, feetY = 206;
    project(stance_, 1.32f, feetX, feetY, cppm);
    feetX += shx;
    feetY = 208 + shy;
    int bp = pose();
    if (mode_ == Mode::Title) {
        feetX = 118;
        feetY = 200;
        bp = 0;
    }
    bool hand = !ballLive_ && mode_ != Mode::Hold && mode_ != Mode::Over && mode_ != Mode::Roll;
    if (mode_ == Mode::Title) hand = true;
    if (hand) {
        float bx = feetX + 22, by = feetY - 46;
        if (bp == 1) {
            bx = feetX + 26;
            by = feetY - 78;
        }
        spr(art_.ball[int(clock_ * 4.0f) % 3], bx, by, 16, PAL_BALL, false);
    }

    struct Item {
        float z;
        int kind;
        int index;
    };
    Item item[16];
    int nitem = 0;
    if (ballLive_) item[nitem++] = {ballZ_, 1, 0};
    for (int i = 0; i < 10; i++) {
        if (pin_[i].swept) continue;
        item[nitem++] = {pin_[i].z, 0, i};
    }
    std::sort(item, item + nitem, [](const Item& a, const Item& b) { return a.z < b.z; });
    spr(art_.bowler[bp], feetX, feetY, mode_ == Mode::Title ? 78.0f : 86.0f, PAL_BOWLER, false, 0, true);
    stamp(art_.shadow, feetX, feetY, 34, 8, PAL_HUD, 0, true);
    for (int k = 0; k < nitem; k++) {
        if (item[k].kind == 1) {
            place(ballX_, std::max(Z_BALL, ballZ_), csx, csy, cppm, cfog);
            float dia = std::max(7.0f, cppm * 0.40f);
            if (gutter_) csy += 5;
            spr(art_.ball[int(ballZ_ * 3.0f) % 3], csx, csy - dia * 0.2f, dia, PAL_BALL, false, cfog);
            stamp(art_.shadow, csx, csy + dia * 0.25f, dia * 0.9f, dia * 0.28f, PAL_HUD, cfog, true);
        } else {
            const Pin& p = pin_[item[k].index];
            place(p.x, p.z, csx, csy, cppm, cfog);
            float ph = std::max(10.0f, cppm * 1.15f);
            if (p.fall <= 0.45f) spr(art_.pin, csx, csy, ph * (1.0f - 0.22f * p.fall), PAL_PIN, false, cfog, true);
            else spr(art_.pinFlat, csx, csy, ph * 0.42f, PAL_PIN, p.vx < 0, cfog, true);
            stamp(art_.shadow, csx, csy, ph * 0.55f, ph * 0.16f, PAL_HUD, cfog, true);
        }
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Swing) {
        bool hot = lined();
        for (int i = 0; i < 6; i++) {
            float z = 1.85f + i * 0.28f;
            place(aimX(), z, csx, csy, cppm, cfog);
            spr(art_.dot, csx, csy, hot ? 7.0f : 5.0f, hot ? PAL_OK : PAL_BRASS, false, cfog);
        }
    }
    for (int i = 0; i < 7; i++) {
        float ax = (i - 3) * 0.15f;
        place(ax, 2.35f, csx, csy, cppm, cfog);
        bool pocketArrow = std::fabs(ax - POCKET_X) < 0.08f || std::fabs(ax + POCKET_X) < 0.08f;
        int pal = pocketArrow && lined() && mode_ != Mode::Title ? PAL_OK : PAL_ARROW;
        spr(art_.arrow, csx, csy, std::max(7.0f, cppm * 0.20f), pal, false, cfog, true);
    }
    place(0, 1.70f, csx, csy, cppm, cfog);
    stamp(art_.foul, csx, csy, std::max(12.0f, cppm * 2.05f), 3, PAL_HUD, 0, false);
    for (float side : {-1.55f, 1.55f}) {
        place(side, 2.7f, csx, csy, cppm, cfog);
        spr(art_.machine, csx, csy, std::max(16.0f, cppm * 0.85f), PAL_DECK, side < 0, cfog, true);
    }
    place(0, Z_BACK + 0.15f, csx, csy, cppm, cfog);
    stamp(art_.backstop, csx, csy - 8, std::max(20.0f, cppm * 2.3f), 16, PAL_DECK, cfog, false);
    for (int i = 0; i < 10; i++) {
        if (pin_[i].swept) continue;
        place(pin_[i].homeX, pin_[i].homeZ, csx, csy, cppm, cfog);
        spr(art_.spot, csx, csy, std::max(3.0f, cppm * 0.12f), PAL_ARROW, false, cfog);
    }

    char count[24];
    writeCount(count, int(sizeof count));
    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(24, "A STRIKE OR A SPARE OPENS A MARK", PAL_PAPER);
        hudC(25, "THE MARK ENDS IT WHEN THE COUNT CLOSES", PAL_BRASS);
        hudC(26, "ARROWS SLIDE    C RELEASES", PAL_HUD);
        hudC(27, std::string("RETURN    ") + S3_VERSION_STRING, PAL_PAPER);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "PAUSE", PAL_BRASS);
        hudC(25, "RETURN RESUMES", PAL_HUD);
        hudC(26, "ESC TO THE TITLE", PAL_PAPER);
    } else if (win) {
        std::snprintf(buf, sizeof buf, "%s  %s", strike_ ? "STRIKE" : "SPARE", count);
        hudC(24, buf, PAL_BRASS);
        hudC(25, "THE COUNT IS CLOSED", PAL_OK);
        hudC(26, "LEAVE", PAL_OK);
        hudC(27, "RETURN BOWLS AGAIN", PAL_PAPER);
    } else if (mode_ == Mode::Over) {
        hudC(24, "NO MARK CLOSED IN TEN", PAL_BAD);
        hudC(25, "THE SHEET STAYS OPEN", PAL_PAPER);
        hudC(27, "RETURN BOWLS AGAIN", PAL_HUD);
    } else {
        hud(1, 24, "S3 PINSMARK", PAL_BRASS);
        if (marked_) std::snprintf(buf, sizeof buf, "MARK F%d", markFrame_);
        else std::snprintf(buf, sizeof buf, "FRAME %d", frame_ + 1);
        hud(30, 24, buf, PAL_PAPER);
        int col = 5;
        for (int i = 0; i < 10; i++) {
            char lab[4];
            if (i < 9) std::snprintf(lab, sizeof lab, " %d ", i + 1);
            else std::snprintf(lab, sizeof lab, "10 ");
            char sym = board_[i];
            bool cur = !marked_ && i == frame_ && mode_ != Mode::Over;
            if (!sym) sym = cur ? ((int(clock_ * 3) & 1) ? '>' : '-') : '.';
            char cell[4];
            std::snprintf(cell, sizeof cell, " %c ", sym);
            int pal = (sym == 'X' || sym == '/') ? PAL_BRASS : PAL_HUD;
            if (cur) pal = PAL_PAPER;
            hud(col, 25, lab, cur ? PAL_PAPER : PAL_HUD);
            hud(col, 26, cell, pal);
            col += 3;
        }
        int need = strike_ ? 2 : 1;
        if (marked_ && !won_) {
            int shown = std::min(need, bonusN_ + 1);
            std::snprintf(buf, sizeof buf, "FILL %d OF %d", shown, need);
            hudC(7, "MARK OPEN", PAL_BRASS);
            hudC(8, std::string(count) + "   " + buf, PAL_PAPER);
            if (mode_ == Mode::Aim) hudC(9, "ANY ROLL CLOSES A BOX", PAL_OK);
        } else if (mode_ == Mode::Swing) {
            const int cells = 17;
            int mid = cells / 2;
            int pos = clampI(meter() * float(cells - 1), 0, cells - 1);
            std::string bar(size_t(cells), '-');
            for (int i = mid - 1; i <= mid + 1; i++) bar[size_t(i)] = '=';
            bar[size_t(pos)] = '*';
            hudC(7, bar, lined() ? PAL_OK : PAL_BRASS);
            hudC(8, lined() ? (ball_ == 0 ? "POCKET" : "ON THE LEAVE") : "RELEASE ON THE LINE", lined() ? PAL_OK : PAL_PAPER);
        } else if (mode_ == Mode::Aim) {
            hudC(7, lined() ? (ball_ == 0 ? "POCKET" : "ON THE LEAVE") : (ball_ == 0 ? "SLIDE ONTO A POCKET" : "SLIDE ONTO THE LEAVE"),
                 lined() ? PAL_OK : PAL_PAPER);
            hudC(8, "C STARTS THE SWING", PAL_HUD);
        } else if (mode_ == Mode::Roll) {
            int down = std::max(0, armed_ - standing());
            std::snprintf(buf, sizeof buf, "DOWN %d", down);
            hudC(7, gutter_ ? "GUTTER" : (pocket_ ? "POCKET" : buf), gutter_ ? PAL_BAD : PAL_OK);
            hudC(8, "HOLD A SIDE TO HOOK", PAL_HUD);
        } else if (mode_ == Mode::Hold) {
            if (ball_ == 1 && !marked_) {
                std::snprintf(buf, sizeof buf, "LEAVE %d", standing());
                hudC(7, "SECOND BALL", PAL_PAPER);
                hudC(8, buf, PAL_HUD);
            } else hudC(7, "NEXT RACK", PAL_PAPER);
        }
        hud(1, 27, "C", PAL_BRASS);
        hud(3, 27, mode_ == Mode::Swing ? "RELEASE" : "SWING", PAL_HUD);
        hud(24, 27, "RETURN PAUSE", PAL_PAPER);
    }
}

}  // namespace pinsmark
