#include "game/seven.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pinsseven {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 90;
constexpr float FOCAL = 200.0f;
constexpr float LANE_K = 0.70f;
constexpr float Z_BALL = 1.58f;
constexpr float Z_HEAD = 2.45f;
constexpr float PIN_DZ = 0.42f;
constexpr float PIN_R = 0.10f;
constexpr float BALL_R = 0.16f;
constexpr float BALL_VZ = 2.35f;
constexpr float POCKET = 0.26f;
constexpr float POCKET_LO = 0.12f;
constexpr float POCKET_HI = 0.40f;
constexpr float HOUSE_X = -0.72f;
constexpr float MISS = 0.90f;
constexpr float SWING_PERIOD = 1.35f;
constexpr float BALL_MASS = 2.4f;
constexpr float PIN_MASS = 0.75f;

struct Lay {
    float x;
    int row;
};

constexpr Lay LAY[7] = {
    {0.00f, 0}, {-0.52f, 1}, {0.52f, 1}, {-0.78f, 2}, {-0.26f, 2}, {0.26f, 2}, {0.78f, 2},
};

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

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Roll) return 2;
    if (mode_ == Mode::Win || mode_ == Mode::Lose || over_) return 3;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 2, 4));
    resetRack();
    mode_ = Mode::Title;
    clock_ = 0;
    you_ = 0;
    them_ = 0;
    yours_ = true;
    won_ = false;
    over_ = false;
}

void Game::newGame() {
    you_ = 0;
    them_ = 0;
    knock_ = 0;
    yours_ = true;
    won_ = false;
    over_ = false;
    stance_ = 0;
    shake_ = 0;
    hook_ = 0;
    resetRack();
    mode_ = Mode::Approach;
}

void Game::resetRack() {
    for (int i = 0; i < 7; i++) {
        Pin& p = pin_[i];
        p.homeX = LAY[i].x;
        p.row = LAY[i].row;
        p.homeZ = Z_HEAD + float(p.row) * PIN_DZ;
        p.x = p.homeX;
        p.z = p.homeZ;
        p.vx = p.vz = 0;
        p.down = false;
        p.fall = 0;
    }
    ballLive_ = false;
    gutter_ = false;
    gutterSnd_ = false;
    committed_ = false;
    pocket_ = false;
    entryTaken_ = false;
    hook_ = 0;
    entry_ = 0;
    fresh_ = 0;
    ballX_ = ballZ_ = ballVx_ = ballVz_ = 0;
}

void Game::beginSwing() { mode_ = Mode::Swing; swingT_ = 0; }

void Game::release(float x, bool pocketShot) {
    ballX_ = clampf(x, -1.2f, 1.2f);
    ballZ_ = Z_BALL;
    ballVx_ = 0;
    ballVz_ = BALL_VZ;
    ballLive_ = true;
    gutter_ = std::fabs(ballX_) > 1.02f;
    gutterSnd_ = false;
    committed_ = pocketShot && !gutter_;
    pocket_ = false;
    entryTaken_ = false;
    entry_ = ballX_;
    hook_ = 0;
    rollT_ = 0;
    fresh_ = 0;
    mode_ = Mode::Roll;
    blip(committed_ ? 660.0f : 220.0f);
    sys_->apu.noiseBurst(0.18f, 1800, 0.05f);
}

void Game::sweep() {
    for (Pin& p : pin_) {
        if (p.down) continue;
        p.down = true;
        float side = (p.homeX >= entry_) ? 1.0f : -1.0f;
        if (std::fabs(p.homeX - entry_) < 0.08f) side = entry_ >= 0 ? 1.0f : -1.0f;
        p.vx += side * (1.5f + 0.35f * float(p.row));
        p.vz += 1.7f + 0.2f * float(p.row);
        fresh_++;
    }
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

float Game::standX() const { return yours_ ? POCKET : HOUSE_X; }

bool Game::inPocket(float x) const { return x >= POCKET_LO && x <= POCKET_HI; }

bool Game::onPocket() const { return inPocket(aimX()); }

int Game::pose() const {
    if (mode_ == Mode::Swing) return 1;
    if (mode_ == Mode::Roll && rollT_ < 0.14f) return 1;
    if (mode_ == Mode::Roll && rollT_ < 0.55f) return 2;
    if (mode_ == Mode::Win) return 2;
    return 0;
}

int Game::fallen() const {
    int n = 0;
    for (const Pin& p : pin_)
        if (p.down) n++;
    return n;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.08f);
}

void Game::physics(float dt) {
    const float h = dt / 4.0f;
    const float zBack = Z_HEAD + 2.0f * PIN_DZ;
    fresh_ = 0;
    for (int step = 0; step < 4; step++) {
        if (ballLive_) {
            if (!gutter_ && !committed_ && !entryTaken_) ballVx_ += hook_ * 2.4f * h;
            if (committed_) ballVx_ = 0;
            float px = ballX_, pz = ballZ_;
            ballX_ += ballVx_ * h;
            ballZ_ += ballVz_ * h;
            if (!gutter_ && ballZ_ < zBack + 0.25f && ballVz_ < 1.25f) ballVz_ = 1.25f;
            if (!gutter_ && std::fabs(ballX_) > 1.02f && ballZ_ < zBack) gutter_ = true;
            if (gutter_) {
                ballX_ = std::copysign(1.14f, ballX_);
                ballVz_ *= (1.0f - 1.5f * h);
                committed_ = false;
            }
            if (!entryTaken_ && pz < Z_HEAD && ballZ_ >= Z_HEAD) {
                float denom = ballZ_ - pz;
                float u = denom > 1e-5f ? (Z_HEAD - pz) / denom : 1.0f;
                entry_ = px + (ballX_ - px) * u;
                entryTaken_ = true;
                pocket_ = !gutter_ && (committed_ || inPocket(entry_)) && ballVz_ > 1.0f;
                if (pocket_) sweep();
            }
            if (!gutter_ && !pocket_) {
                for (Pin& p : pin_) {
                    bool was = p.down;
                    collide(ballX_, ballZ_, ballVx_, ballVz_, BALL_R, BALL_MASS, p.x, p.z, p.vx, p.vz, PIN_R, PIN_MASS,
                            0.32f);
                    float sp = std::hypot(p.vx, p.vz);
                    if (sp > 8.0f) {
                        p.vx *= 8.0f / sp;
                        p.vz *= 8.0f / sp;
                    }
                    if (!p.down && (sp > 0.9f || std::hypot(p.x - p.homeX, p.z - p.homeZ) > 0.24f)) p.down = true;
                    if (p.down && !was) fresh_++;
                }
            }
        }
        for (int a = 0; a < 7; a++) {
            for (int b = a + 1; b < 7; b++) {
                Pin& pa = pin_[a];
                Pin& pb = pin_[b];
                float sa = std::hypot(pa.vx, pa.vz), sb = std::hypot(pb.vx, pb.vz);
                if (!pa.down && !pb.down && sa < 0.05f && sb < 0.05f) continue;
                bool wa = pa.down, wb = pb.down;
                collide(pa.x, pa.z, pa.vx, pa.vz, PIN_R, PIN_MASS, pb.x, pb.z, pb.vx, pb.vz, PIN_R, PIN_MASS, 0.5f);
                for (Pin* p : {&pa, &pb}) {
                    float sp = std::hypot(p->vx, p->vz);
                    if (sp > 8.0f) {
                        p->vx *= 8.0f / sp;
                        p->vz *= 8.0f / sp;
                    }
                    if (!p->down && (sp > 0.85f || std::hypot(p->x - p->homeX, p->z - p->homeZ) > 0.22f)) p->down = true;
                }
                if (pa.down && !wa) fresh_++;
                if (pb.down && !wb) fresh_++;
            }
        }
        for (Pin& p : pin_) {
            float sp = std::hypot(p.vx, p.vz);
            if (sp < 0.04f && !p.down) continue;
            p.x += p.vx * h;
            p.z += p.vz * h;
            float drag = 1.0f - 3.1f * h;
            p.vx *= drag;
            p.vz *= drag;
            p.x = clampf(p.x, -2.2f, 2.2f);
            p.z = clampf(p.z, 0.6f, 7.5f);
            sp = std::hypot(p.vx, p.vz);
            if (!p.down && (sp > 0.85f || std::fabs(p.x) > 1.08f)) p.down = true;
            if (sp < 0.22f) p.vx = p.vz = 0;
        }
    }
    for (Pin& p : pin_)
        if (p.down) p.fall = std::min(1.0f, p.fall + dt * 3.4f);
    if (fresh_ > 0) {
        sys_->apu.noiseBurst(std::min(0.65f, 0.22f + fresh_ * 0.07f), 700.0f + fresh_ * 140.0f, 0.12f);
        sys_->apu.tone(2, 120.0f + fallen() * 36.0f, 0.07f);
        beep_ = std::max(beep_, 0.1f);
        shake_ = std::min(0.5f, shake_ + fresh_ * 0.05f);
        sys_->rumble(0.2f, 0.5f, 60);
    }
}

void Game::settle() {
    ballLive_ = false;
    sys_->apu.noise(0, 800, false);
    if (pocket_) {
        for (Pin& p : pin_) {
            if (!p.down) {
                p.down = true;
                p.fall = std::max(p.fall, 0.2f);
            }
        }
    }
    knock_ = fallen();
    if (yours_) you_ += knock_;
    else them_ += knock_;
    if (yours_ && you_ >= 7) {
        won_ = true;
        mode_ = Mode::Win;
        countT_ = 0;
        shake_ = 0.4f;
        sys_->apu.tone(0, 523, 0.12f);
        sys_->apu.tone(1, 659, 0.10f);
        sys_->apu.tone(2, 784, 0.09f);
        beep_ = 0.9f;
        sys_->rumble(0.45f, 0.85f, 200);
        sys_->setLight(255, 210, 70);
        return;
    }
    if (!yours_ && them_ >= 7 && you_ < 7) {
        won_ = false;
        mode_ = Mode::Lose;
        countT_ = 0;
        sys_->apu.tone(0, 110, 0.12f);
        sys_->apu.tone(1, 82, 0.10f);
        beep_ = 0.6f;
        sys_->setLight(160, 30, 30);
        return;
    }
    mode_ = Mode::Count;
    countT_ = 0;
    blip(yours_ ? 440.0f : 180.0f);
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

    const bool autoBowl = !human();
    if (bot_) {
        action = false;
        start = false;
        back = false;
        slide = 0;
    }
    if (autoBowl) {
        action = false;
        slide = 0;
        if (mode_ == Mode::Title && clock_ > 0.45f) start = true;
        else if (mode_ == Mode::Approach) {
            float d = standX() - stance_;
            if (std::fabs(d) <= 0.012f) {
                stance_ = standX();
                action = true;
            } else slide = d > 0 ? 1.0f : -1.0f;
        } else if (mode_ == Mode::Swing) {
            float m = meter();
            if (swingT_ > 0.08f && swingT_ < SWING_PERIOD * 0.5f && m >= 0.5f && m < 0.58f) action = true;
        }
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else if (mode_ == Mode::Title) {
        if (back && !bot_) sys.quit();
        else if (start || action) newGame();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        countT_ += DT;
        for (Pin& p : pin_)
            if (p.down) p.fall = std::min(1.0f, p.fall + DT * 3.0f);
        if (countT_ > 0.7f) over_ = true;
        if ((start || action) && !bot_) newGame();
        else if (back && !bot_) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else if (start && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Approach) {
        if (!action) stance_ = clampf(stance_ + slide * 1.25f * DT, -0.9f, 0.9f);
        if (action) beginSwing();
    } else if (mode_ == Mode::Swing) {
        swingT_ += DT;
        if (action) {
            if (autoBowl && yours_) release(POCKET, true);
            else if (autoBowl) release(HOUSE_X, false);
            else {
                float x = clampf(stance_ + (meter() - 0.5f) * MISS, -1.15f, 1.15f);
                release(x, inPocket(x));
            }
        }
    } else if (mode_ == Mode::Roll) {
        rollT_ += DT;
        if (ballLive_ && !committed_ && !entryTaken_ && human())
            hook_ = clampf(hook_ + slide * DT * 1.6f, -1.0f, 1.0f);
        physics(DT);
        if (gutter_ && !gutterSnd_) {
            gutterSnd_ = true;
            blip(90);
            sys.apu.noiseBurst(0.28f, 280, 0.16f);
            sys.setLight(170, 40, 40);
        }
        if (ballLive_ && !gutter_) sys.apu.noise(0.03f, 3600, false);
        bool quiet = true;
        for (const Pin& p : pin_)
            if (std::hypot(p.vx, p.vz) > 0.35f) quiet = false;
        bool far = ballZ_ > Z_HEAD + 2.0f * PIN_DZ + 0.55f;
        bool stopped = gutter_ && (ballVz_ < 0.45f || ballZ_ > Z_HEAD);
        bool ready = far || stopped || (pocket_ && rollT_ > 0.62f) || rollT_ > 2.6f;
        if (ready && (quiet || rollT_ > 1.05f || gutter_)) settle();
    } else if (mode_ == Mode::Count) {
        countT_ += DT;
        for (Pin& p : pin_)
            if (p.down) p.fall = std::min(1.0f, p.fall + DT * 3.0f);
        if (countT_ > 0.65f) {
            yours_ = !yours_;
            float keep = yours_ ? 0.0f : HOUSE_X * 0.35f;
            resetRack();
            stance_ = keep;
            mode_ = Mode::Approach;
        }
    }

    draw();
}

void Game::project(float x, float z, float& sx, float& sy, float& ppm) const {
    z = std::max(0.40f, z);
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

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    const float adv = 16.0f * scale;
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

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(clock_ * 90.0f) * 4.0f * shake_;
        shy = std::cos(clock_ * 70.0f) * 2.0f * shake_;
    }
    bool win = mode_ == Mode::Win;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            float u = y / float(HORIZON);
            int r = int(1 + u * (win ? 8 : 5));
            int g = int(1 + u * (win ? 6 : 3));
            int b = int(3 + u * (win ? 2 : 4));
            v.lineBackdrop[y] = gs::rgb4(std::min(r, 15), std::min(g, 15), std::min(b, 15));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - HORIZON);
        float z = FOCAL / std::max(1.0f, row);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.0f + shx;
        rd.hw = row * LANE_K;
        rd.v = z * 220.0f;
        rd.pal = PAL_LANE;
        rd.band = (int(z * 4.0f) & 1) ? 1 : 0;
        rd.style = 1;
        rd.left = rd.right = 0;
        int fog = std::clamp(int((78.0f - row) / 18.0f), 0, 5);
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(1, 2, 4);
    }

    auto place = [&](float x, float z, float& sx, float& sy, float& ppm, int& fog) {
        project(x, z, sx, sy, ppm);
        sx += shx;
        sy += shy;
        fog = std::clamp(int((z - 2.0f) * 1.6f), 0, 6);
    };

    if (mode_ == Mode::Title) {
        text("S3 PINS", 160, 14, 1.0f, PAL_HUD);
        text("SEVEN", 160, 42, 1.4f, PAL_GOLD);
    } else if (win) {
        text("SEVEN", 160, 42, 1.5f, PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        text("HOUSE", 160, 42, 1.45f, PAL_THEM);
    } else if (mode_ == Mode::Pause) text("PAUSE", 160, 42, 1.3f, PAL_AMBER);

    float feetX = 160.0f + stance_ * 78.0f + shx;
    float feetY = 202.0f + shy;
    int bp = pose();
    int bowPal = yours_ ? PAL_YOU : PAL_THEM;
    int ballPal = yours_ ? PAL_BALL : PAL_HOUSE;
    bool showHand = !ballLive_ && mode_ != Mode::Count && mode_ != Mode::Win && mode_ != Mode::Lose &&
                    !(mode_ == Mode::Roll && rollT_ >= 0.16f);
    if (mode_ == Mode::Title) {
        feetX = 72;
        feetY = 198;
        bp = 0;
        bowPal = PAL_YOU;
        ballPal = PAL_BALL;
        showHand = true;
    }
    if (showHand) {
        float bob = std::sin(clock_ * 3.0f) * 1.5f;
        float bx = feetX + 20.0f;
        float by = feetY - 30.0f + bob;
        if (bp == 1) {
            bx = feetX + 26.0f;
            by = feetY - 24.0f;
        } else if (bp == 2) {
            bx = feetX + 8.0f;
            by = feetY - 58.0f;
        }
        spr(art_.ball[int(clock_ * 2.5f) & 2], bx, by, 14, ballPal, false);
    }

    float csx, csy, cppm;
    int cfog;
    if (ballLive_) {
        place(ballX_, std::max(Z_BALL, ballZ_), csx, csy, cppm, cfog);
        float dia = std::max(7.0f, cppm * 0.34f);
        if (gutter_) csy += 5;
        spr(art_.ball[int(ballZ_ * 3.0f) & 2], csx, csy - dia * 0.2f, dia, ballPal, false, cfog, false);
        stamp(art_.shadow, csx, csy + dia * 0.25f, dia * 0.9f, dia * 0.28f, PAL_HUD, cfog, true);
    }

    spr(art_.bowler[bp], feetX, feetY, mode_ == Mode::Title ? 74.0f : 80.0f, bowPal, false, 0, true);
    stamp(art_.shadow, feetX, feetY + 1, 34, 8, PAL_HUD, 0, true);
    if (mode_ != Mode::Title) {
        float wx = yours_ ? 286.0f : 34.0f;
        int wp = yours_ ? PAL_THEM : PAL_YOU;
        spr(art_.bowler[0], wx, 198, 52, wp, !yours_, 0, true);
        stamp(art_.shadow, wx, 198, 22, 6, PAL_HUD, 0, true);
    } else {
        spr(art_.bowler[0], 268, 196, 56, PAL_THEM, true, 0, true);
        stamp(art_.shadow, 268, 196, 24, 6, PAL_HUD, 0, true);
    }

    int order[7];
    for (int i = 0; i < 7; i++) order[i] = i;
    std::sort(order, order + 7, [&](int a, int b) { return pin_[a].z < pin_[b].z; });
    for (int k = 0; k < 7; k++) {
        const Pin& p = pin_[order[k]];
        float sx, sy, ppm;
        int fog;
        place(p.x, p.z, sx, sy, ppm, fog);
        float ph = std::max(11.0f, ppm * (p.row == 0 ? 0.62f : 0.56f));
        bool flat = p.fall > 0.45f;
        if (!flat) spr(art_.pin, sx, sy, ph * (1.0f - 0.2f * p.fall), PAL_PIN, false, fog, true);
        else spr(art_.pinFlat, sx, sy - 1.0f, ph * 0.42f, PAL_PIN, p.homeX < 0, fog, true);
        stamp(art_.shadow, sx, sy, ph * 0.55f, ph * 0.16f, PAL_HUD, fog, true);
    }

    int lit = 0;
    if (mode_ == Mode::Title) lit = -1;
    else lit = std::min(you_, 7);
    for (int i = 0; i < 7; i++) {
        float lx = -0.78f + i * 0.26f;
        place(lx, Z_HEAD + 2.0f * PIN_DZ + 0.08f, csx, csy, cppm, cfog);
        bool on = lit < 0 ? (int(clock_ * 5.0f) % 7 == i) : i < lit;
        float lh = std::max(8.0f, cppm * 0.22f);
        spr(art_.lamp, csx, csy, lh, on ? PAL_LAMP : PAL_DIM, false, cfog, true);
    }

    bool hot = onPocket() && (mode_ == Mode::Approach || mode_ == Mode::Swing);
    if (mode_ == Mode::Approach || mode_ == Mode::Swing) {
        for (int i = 0; i < 6; i++) {
            float z = 1.62f + i * 0.16f;
            place(aimX(), z, csx, csy, cppm, cfog);
            spr(art_.dot, csx, csy, hot ? 6.5f : 4.5f, hot ? PAL_GREEN : PAL_GOLD, false, cfog, false);
        }
    }
    const float marks[4] = {-0.52f, -0.26f, POCKET, 0.52f};
    for (float mx : marks) {
        place(mx, 1.95f, csx, csy, cppm, cfog);
        bool pocketMark = std::fabs(mx - POCKET) < 0.02f;
        float ah = std::max(7.0f, cppm * (pocketMark ? 0.20f : 0.13f));
        if (pocketMark && hot) ah *= 1.12f;
        int pal = pocketMark && hot ? PAL_GREEN : (pocketMark ? PAL_GOLD : PAL_AMBER);
        spr(art_.arrow, csx, csy, ah, pal, false, cfog, true);
    }
    place(0, 1.55f, csx, csy, cppm, cfog);
    stamp(art_.foul, csx, csy, cppm * 2.05f, 3.0f, PAL_THEM, 0, false);
    place(0, Z_HEAD + 2.0f * PIN_DZ + 0.35f, csx, csy, cppm, cfog);
    stamp(art_.pit, csx, csy, cppm * 2.5f, cppm * 0.55f, PAL_PIT, cfog, false);
    for (float side : {-1.18f, 1.18f}) {
        place(side, 2.15f, csx, csy, cppm, cfog);
        spr(art_.post, csx, csy, std::max(16.0f, cppm * 0.7f), PAL_POST, side < 0, cfog, true);
        place(side, 3.15f, csx, csy, cppm, cfog);
        spr(art_.post, csx, csy, std::max(14.0f, cppm * 0.62f), PAL_POST, side < 0, cfog, true);
    }

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(8, "FIRST TO SEVEN", PAL_GOLD);
        hudC(9, "AIM THE ARROW", PAL_HUD);
        hudC(10, "C WHEN GREEN", PAL_GREEN);
        hud(1, 26, "START", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
    } else {
        hud(1, 0, "S3 PINS SEVEN", PAL_GOLD);
        hud(30, 0, "TO 7", PAL_AMBER);
        auto pips = [](char* out, int n) {
            int k = std::clamp(n, 0, 7);
            for (int i = 0; i < 7; i++) out[i] = i < k ? '#' : '.';
            out[7] = 0;
        };
        char yp[8], tp[8];
        pips(yp, you_);
        pips(tp, them_);
        std::snprintf(buf, sizeof buf, "YOU %d  %s", you_, yp);
        hud(1, 1, buf, yours_ && mode_ != Mode::Win ? PAL_GREEN : PAL_HUD);
        std::snprintf(buf, sizeof buf, "THEM %d  %s", them_, tp);
        hud(22, 1, buf, !yours_ && mode_ != Mode::Win && mode_ != Mode::Title ? PAL_AMBER : PAL_HUD);
        if (mode_ == Mode::Swing) {
            const int cells = 17;
            int mid = cells / 2;
            int pos = int(std::lround(meter() * (cells - 1)));
            char bar[18];
            for (int i = 0; i < cells; i++) bar[i] = (std::abs(i - mid) <= 1) ? '=' : '-';
            bar[std::clamp(pos, 0, cells - 1)] = '*';
            bar[cells] = 0;
            hudC(3, bar, hot ? PAL_GREEN : PAL_AMBER);
            hudC(4, hot ? "POCKET  LET GO" : "LET GO ON THE GREEN", hot ? PAL_GREEN : PAL_HUD);
        } else if (mode_ == Mode::Approach) {
            hudC(3, yours_ ? (hot ? "POCKET" : "SLIDE ONTO THE POCKET") : "HOUSE IS UP",
                 hot ? PAL_GREEN : PAL_AMBER);
            if (yours_) hudC(4, hot ? "C STARTS THE SWING" : "THE WIDE ARROW IS THE LINE", PAL_HUD);
        } else if (mode_ == Mode::Roll) {
            std::snprintf(buf, sizeof buf, "DOWN %d", fallen());
            const char* msg = gutter_ ? "GUTTER" : (pocket_ ? "POCKET" : buf);
            hudC(3, msg, gutter_ ? PAL_AMBER : PAL_GREEN);
        } else if (mode_ == Mode::Count) {
            std::snprintf(buf, sizeof buf, "%s  +%d", yours_ ? "YOU" : "HOUSE", knock_);
            hudC(3, buf, PAL_GOLD);
            int need = std::max(0, 7 - you_);
            std::snprintf(buf, sizeof buf, "NEED %d", need);
            hudC(4, buf, PAL_HUD);
        } else if (mode_ == Mode::Pause) {
            hudC(3, "START RESUMES", PAL_AMBER);
        }
        if (mode_ != Mode::Pause) hud(1, 27, "START PAUSE", PAL_HUD);
    }
}

}  // namespace pinsseven
