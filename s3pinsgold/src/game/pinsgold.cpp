#include "game/pinsgold.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pinsgold {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 44;
constexpr float FOCAL = 206.0f;
constexpr float LANE_K = 0.60f;
constexpr float Z0 = 3.85f;
constexpr float PIN_X = 0.36f;
constexpr float PIN_Z = 0.33f;
constexpr float PIN_R = 0.09f;
constexpr float BALL_R = 0.16f;
constexpr float BALL_VZ = 4.8f;
constexpr float POCKET = 0.09f;
constexpr float MISS = 0.90f;
constexpr float SWING = 0.92f;
constexpr float DRIVE = 0.06f;
constexpr int NEED = 10;
constexpr float BALL_M = 6.0f;
constexpr float PIN_M = 0.8f;

const int GOLD[10] = {1, 0, 0, 0, 1, 0, 1, 0, 0, 1};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

void capSpeed(float& vx, float& vz, float lim) {
    float s = std::hypot(vx, vz);
    if (s > lim) {
        vx *= lim / s;
        vz *= lim / s;
    }
}

void bounce(float& ax, float& az, float& avx, float& avz, float ar, float am, float& bx, float& bz, float& bvx,
            float& bvz, float br, float bm, float e) {
    float dx = bx - ax, dz = bz - az;
    float d2 = dx * dx + dz * dz;
    float r = ar + br;
    if (d2 >= r * r || d2 < 1e-8f) return;
    float d = std::sqrt(d2);
    float nx = dx / d, nz = dz / d;
    float overlap = r - d;
    float ia = 1.0f / am, ib = 1.0f / bm, inv = ia + ib;
    ax -= nx * overlap * (ia / inv);
    az -= nz * overlap * (ia / inv);
    bx += nx * overlap * (ib / inv);
    bz += nz * overlap * (ib / inv);
    float rv = (bvx - avx) * nx + (bvz - avz) * nz;
    if (rv >= 0.0f) return;
    float j = -(1.0f + e) * rv / inv;
    j = std::min(j, 6.0f);
    avx -= j * nx * ia;
    avz -= j * nz * ia;
    bvx += j * nx * ib;
    bvz += j * nz * ib;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    clock_ = 0;
    popN_ = 0;
    showTitle();
    silence();
}

void Game::showTitle() {
    score_ = gold_ = cream_ = 0;
    ball_ = 0;
    popN_ = 0;
    stance_ = 0;
    won_ = false;
    over_ = false;
    verdict_ = "";
    shake_ = 0;
    bannerT_ = 0;
    sweepT_ = 0;
    resetRack();
    mode_ = Mode::Title;
}

void Game::newGame() {
    score_ = gold_ = cream_ = 0;
    ball_ = 0;
    popN_ = 0;
    stance_ = 0;
    won_ = false;
    over_ = false;
    verdict_ = "";
    shake_ = 0;
    resetRack();
    ballLive_ = false;
    gutter_ = false;
    driven_ = false;
    wasPocket_ = false;
    mode_ = Mode::Approach;
    sys_->setLight(200, 140, 40);
}

void Game::resetRack() {
    int n = 0;
    for (int row = 0; row < 4; row++) {
        for (int i = 0; i <= row; i++, n++) {
            Pin& p = pin_[n];
            p.homeX = (float(i) - row * 0.5f) * PIN_X;
            p.homeZ = Z0 + row * PIN_Z;
            p.x = p.homeX;
            p.z = p.homeZ;
            p.vx = p.vz = 0;
            p.gold = GOLD[n] != 0;
            p.down = false;
            p.hidden = false;
            p.side = 1;
            p.fall = 0;
        }
    }
    ballLive_ = false;
    gutter_ = false;
    gutterSnd_ = false;
    driven_ = false;
}

void Game::beginSwing() {
    mode_ = Mode::Swing;
    swingT_ = 0;
    wasPocket_ = false;
}

void Game::release() {
    float miss = (meter() - 0.5f) * MISS;
    ballX_ = clampf(stance_ + miss, -1.2f, 1.2f);
    ballZ_ = 1.22f;
    ballVx_ = 0;
    ballVz_ = BALL_VZ;
    ballLive_ = true;
    gutter_ = false;
    gutterSnd_ = false;
    driven_ = false;
    rollT_ = 0;
    mode_ = Mode::Roll;
    blip(190);
    sys_->apu.noiseBurst(0.18f, 2100, 0.05f);
}

void Game::topple(Pin& p) {
    if (p.down || p.hidden) return;
    p.down = true;
    p.side = p.vx < 0 ? -1 : 1;
    if (p.fall < 0.15f) p.fall = 0.15f;
    int pts = p.gold ? 2 : 1;
    score_ += pts;
    if (p.gold) gold_++;
    else cream_++;
    if (p.gold && popN_ < 8) {
        Pop& pop = pop_[popN_++];
        pop.x = p.x;
        pop.z = p.z;
        pop.t = 0;
        pop.pts = pts;
    }
}

void Game::tryDrive(float entry) {
    // A ball that enters a gold pocket mixes the whole rack. Anywhere else, only contact counts.
    driven_ = true;
    if (gutter_) return;
    if (std::fabs(std::fabs(entry) - POCKET) > DRIVE) return;
    for (Pin& p : pin_) {
        if (p.hidden || p.down) continue;
        float dx = p.homeX - entry;
        float dz = p.homeZ - Z0;
        float dist = std::sqrt(dx * dx + dz * dz);
        float side = std::fabs(dx) < 0.05f ? (entry >= 0 ? 1.0f : -1.0f) : (dx >= 0 ? 1.0f : -1.0f);
        float kick = std::max(1.2f, 3.0f - dist);
        p.vx += side * kick;
        p.vz += kick;
        topple(p);
    }
}

void Game::physics(float dt) {
    const float h = dt / 8.0f;
    const float back = Z0 + 3.0f * PIN_Z;
    for (int step = 0; step < 8; step++) {
        if (ballLive_) {
            float px = ballX_, pz = ballZ_;
            if (!gutter_ && ballZ_ < Z0 - 0.05f) ballVx_ += steer_ * 2.1f * h;
            ballX_ += ballVx_ * h;
            ballZ_ += ballVz_ * h;
            ballVx_ *= (1.0f - 0.3f * h);
            if (!gutter_ && ballZ_ < Z0 - 0.08f && std::fabs(ballX_) > 1.02f) {
                gutter_ = true;
                driven_ = true;
            }
            if (gutter_) {
                ballX_ = std::copysign(1.18f, ballX_);
                ballVz_ *= (1.0f - 1.7f * h);
            }
            if (!driven_ && pz < Z0 && ballZ_ >= Z0) {
                float denom = ballZ_ - pz;
                float u = denom > 1e-5f ? (Z0 - pz) / denom : 1.0f;
                tryDrive(px + (ballX_ - px) * u);
            }
            if (!gutter_) {
                for (Pin& p : pin_) {
                    if (p.hidden) continue;
                    bounce(ballX_, ballZ_, ballVx_, ballVz_, BALL_R, BALL_M, p.x, p.z, p.vx, p.vz, PIN_R, PIN_M, 0.3f);
                    capSpeed(p.vx, p.vz, 7.0f);
                    float sp = std::hypot(p.vx, p.vz);
                    if (!p.down && (sp > 1.0f || std::hypot(p.x - p.homeX, p.z - p.homeZ) > 0.26f)) topple(p);
                }
            }
            if (ballZ_ > back + 0.85f) ballLive_ = false;
        }
        for (int a = 0; a < 10; a++) {
            for (int b = a + 1; b < 10; b++) {
                Pin& pa = pin_[a];
                Pin& pb = pin_[b];
                if (pa.hidden || pb.hidden) continue;
                if (std::hypot(pa.vx, pa.vz) < 0.08f && std::hypot(pb.vx, pb.vz) < 0.08f) continue;
                bounce(pa.x, pa.z, pa.vx, pa.vz, PIN_R, PIN_M, pb.x, pb.z, pb.vx, pb.vz, PIN_R, PIN_M, 0.62f);
                for (Pin* p : {&pa, &pb}) {
                    capSpeed(p->vx, p->vz, 7.0f);
                    float sp = std::hypot(p->vx, p->vz);
                    if (!p->down && (sp > 0.95f || std::hypot(p->x - p->homeX, p->z - p->homeZ) > 0.26f)) topple(*p);
                }
            }
        }
        for (Pin& p : pin_) {
            if (p.hidden) continue;
            float sp = std::hypot(p.vx, p.vz);
            if (sp < 0.05f && !p.down) continue;
            p.x += p.vx * h;
            p.z += p.vz * h;
            float damp = std::exp((p.down ? -4.5f : -2.4f) * h);
            p.vx *= damp;
            p.vz *= damp;
            p.x = clampf(p.x, -2.2f, 2.2f);
            p.z = clampf(p.z, 0.6f, back + 1.8f);
            sp = std::hypot(p.vx, p.vz);
            if (sp < 0.18f) p.vx = p.vz = 0;
            if (!p.down && (sp > 0.95f || std::fabs(p.x) > 1.08f || p.z > back + 0.45f ||
                            std::hypot(p.x - p.homeX, p.z - p.homeZ) > 0.30f))
                topple(p);
        }
    }
    for (Pin& p : pin_)
        if (p.down) p.fall = std::min(1.0f, p.fall + dt * 3.4f);
}

void Game::beginSweep() {
    ballLive_ = false;
    for (Pin& p : pin_) {
        p.vx = p.vz = 0;
        if (!p.down) {
            p.x = p.homeX;
            p.z = p.homeZ;
            p.fall = 0;
        }
    }
    sweepT_ = 0;
    mode_ = Mode::Sweep;
    blip(160);
}

void Game::beginBanner() {
    ballLive_ = false;
    for (Pin& p : pin_) p.vx = p.vz = 0;
    bannerT_ = 0;
    won_ = score_ >= NEED;
    verdict_ = won_ ? "DOUBLE" : "SHORT";
    mode_ = Mode::Banner;
    cheered_ = true;
    if (won_) {
        sys_->apu.tone(0, 523, 0.12f);
        sys_->apu.tone(1, 659, 0.10f);
        sys_->apu.tone(2, 784, 0.11f);
        beep_ = 0.9f;
        shake_ = 0.35f;
        sys_->rumble(0.4f, 0.8f, 180);
        sys_->setLight(255, 196, 40);
    } else {
        sys_->apu.tone(0, 98, 0.10f);
        beep_ = 0.45f;
        sys_->setLight(170, 28, 28);
    }
}

void Game::judge() {
    ballLive_ = false;
    sys_->apu.noise(0, 1200, false);
    const float back = Z0 + 3.0f * PIN_Z;
    for (Pin& p : pin_) {
        if (!p.down && !p.hidden) {
            float sp = std::hypot(p.vx, p.vz);
            if (sp > 0.9f || std::hypot(p.x - p.homeX, p.z - p.homeZ) > 0.26f || std::fabs(p.x) > 1.05f ||
                p.z > back + 0.3f)
                topple(p);
        }
        p.vx = p.vz = 0;
    }
    if (score_ >= NEED || ball_ >= 1) beginBanner();
    else beginSweep();
}

float Game::meter() const {
    float u = std::fmod(std::max(0.0f, swingT_), SWING) / SWING;
    return u < 0.5f ? u * 2.0f : 2.0f - u * 2.0f;
}

float Game::aimX() const {
    if (mode_ == Mode::Swing || (mode_ == Mode::Pause && held_ == Mode::Swing))
        return clampf(stance_ + (meter() - 0.5f) * MISS, -1.15f, 1.15f);
    if (mode_ == Mode::Roll && ballLive_) return ballX_;
    return stance_;
}

float Game::aimTarget() const {
    if (ball_ == 0) return POCKET;
    const Pin* best = nullptr;
    for (const Pin& p : pin_) {
        if (p.down || p.hidden) continue;
        if (!best || (p.gold && !best->gold) ||
            (p.gold == best->gold && std::fabs(p.homeX) < std::fabs(best->homeX)))
            best = &p;
    }
    return best ? clampf(best->homeX, -0.72f, 0.72f) : 0.0f;
}

bool Game::onPocket() const { return std::fabs(std::fabs(aimX()) - POCKET) <= DRIVE; }

bool Game::starReady() const { return std::fabs(meter() - 0.5f) <= 0.08f; }

int Game::pose() const {
    Mode m = mode_ == Mode::Pause ? held_ : mode_;
    if (m == Mode::Swing) return 1;
    if (m == Mode::Roll && rollT_ < 0.12f) return 2;
    if (m == Mode::Roll && rollT_ < 0.46f) return 3;
    if ((m == Mode::Banner || m == Mode::Over) && won_) return 3;
    return 0;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = std::max(beep_, 0.08f);
}

void Game::silence() {
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noise(0, 1000, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) clock_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) silence();
    }
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    for (int i = 0; i < popN_;) {
        pop_[i].t += DT;
        if (pop_[i].t > 0.75f) pop_[i] = pop_[--popN_];
        else i++;
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
            float d = aimTarget() - stance_;
            if (std::fabs(d) < 0.008f) {
                stance_ = aimTarget();
                action = true;
            } else slide = clampf(d * 7.0f, -1.0f, 1.0f);
        } else if (mode_ == Mode::Swing && swingT_ < SWING * 0.5f && meter() >= 0.5f) action = true;
    }

    if (mode_ == Mode::Pause) {
        if (start) mode_ = held_;
        else if (back) showTitle();
        draw();
        return;
    }
    if (back && mode_ == Mode::Title) {
        if (!bot_) sys.quit();
        draw();
        return;
    }
    if (back && mode_ != Mode::Banner) {
        showTitle();
        draw();
        return;
    }
    if (start && mode_ != Mode::Title && mode_ != Mode::Over && mode_ != Mode::Banner && !bot_) {
        held_ = mode_;
        mode_ = Mode::Pause;
        draw();
        return;
    }

    if (mode_ == Mode::Title) {
        if (start || action) newGame();
    } else if (mode_ == Mode::Over) {
        if (start || action) newGame();
    } else if (mode_ == Mode::Approach) {
        stance_ = clampf(stance_ + slide * 0.95f * DT, -0.72f, 0.72f);
        bool lined = onPocket();
        if (lined && !wasPocket_) blip(860);
        wasPocket_ = lined;
        if (lined) sys.setLight(40, 180, 70);
        else sys.setLight(180, 120, 40);
        if (action) beginSwing();
    } else if (mode_ == Mode::Swing) {
        swingT_ += DT;
        bool lined = onPocket();
        if (lined && !wasPocket_) blip(980);
        wasPocket_ = lined;
        if (lined && starReady()) sys.setLight(80, 220, 90);
        if (action) release();
    } else if (mode_ == Mode::Roll) {
        rollT_ += DT;
        steer_ = (!gutter_ && ballLive_ && ballZ_ < Z0 - 0.1f) ? slide : 0;
        int g0 = gold_, c0 = cream_;
        physics(DT);
        int dg = gold_ - g0, dc = cream_ - c0;
        if (dg) {
            sys.apu.tone(1, 880, 0.09f);
            sys.apu.tone(2, 1318, 0.07f);
            beep_ = std::max(beep_, 0.16f);
            shake_ = std::min(0.55f, shake_ + dg * 0.08f);
            sys.rumble(0.35f, 0.75f, 90);
            sys.setLight(255, 200, 40);
        } else if (dc) {
            blip(280.0f + dc * 50.0f);
            shake_ = std::min(0.3f, shake_ + dc * 0.035f);
        }
        if (dg + dc) sys.apu.noiseBurst(std::min(0.7f, 0.22f + (dg + dc) * 0.06f), 700.0f + dg * 180.0f, 0.1f);
        if (ballLive_ && !gutter_) sys.apu.noise(0.03f, 3600, false);
        if (gutter_ && !gutterSnd_) {
            gutterSnd_ = true;
            blip(80);
            sys.apu.noiseBurst(0.3f, 240, 0.16f);
        }
        bool quiet = true;
        for (const Pin& p : pin_)
            if (!p.hidden && std::hypot(p.vx, p.vz) > 0.22f) quiet = false;
        const float back = Z0 + 3.0f * PIN_Z;
        bool ballDone = gutter_ ? (ballZ_ > Z0 - 0.15f || ballVz_ < 0.4f || rollT_ > 2.2f)
                                : (ballZ_ > back + 0.45f || !ballLive_ || rollT_ > 3.2f);
        if ((ballDone && (quiet || rollT_ > 2.6f)) || rollT_ > 3.2f) judge();
    } else if (mode_ == Mode::Sweep) {
        sweepT_ += DT;
        for (Pin& p : pin_) {
            if (!p.down || p.hidden) continue;
            p.fall = 1;
            p.x += (p.homeX >= 0 ? 1.0f : -1.0f) * 1.7f * DT;
        }
        if (sweepT_ > 0.55f) {
            for (Pin& p : pin_)
                if (p.down) p.hidden = true;
            ball_ = 1;
            ballLive_ = false;
            gutter_ = false;
            driven_ = false;
            steer_ = 0;
            wasPocket_ = false;
            mode_ = Mode::Approach;
        }
    } else if (mode_ == Mode::Banner) {
        bannerT_ += DT;
        if (bannerT_ > (won_ ? 1.05f : 0.75f)) {
            over_ = true;
            mode_ = Mode::Over;
        }
    }

    if (mode_ != Mode::Roll) {
        for (Pin& p : pin_)
            if (p.down) p.fall = std::min(1.0f, p.fall + DT * 3.0f);
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
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
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
    s.w = int16_t(std::max(1, std::min(2000, (int)std::lround(w))));
    s.h = int16_t(std::max(1, std::min(2000, (int)std::lround(h))));
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
    int n = (int)std::strlen(s);
    const float adv = 18.0f * scale;
    x -= n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + adv * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = (unsigned char)s[i];
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? (int)std::strlen(s) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    bool blink = (int(clock_ * 6.0f) & 1) != 0;
    v.setColor(PAL_GOLD * 16 + 3, gs::rgb4(15, blink ? 15 : 12, blink ? 11 : 3));

    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(clock_ * 90.0f) * 4.0f * shake_;
        shy = std::cos(clock_ * 70.0f) * 2.0f * shake_;
    }
    bool win = (mode_ == Mode::Banner || mode_ == Mode::Over) && won_;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= HORIZON) {
            float u = y / float(HORIZON);
            int r = 1 + int(u * (win ? 8 : 5));
            int g = 1 + int(u * (win ? 5 : 2));
            int b = 1 + int(u * 2);
            v.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - HORIZON);
        float z = FOCAL / std::max(8.0f, row);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.0f + shx;
        rd.hw = row * LANE_K;
        rd.v = z * 36.0f;
        rd.pal = PAL_LANE;
        rd.band = 0;
        rd.style = 1;
        rd.left = rd.right = 0;
        v.lineFog[y] = uint8_t(std::clamp(int((62.0f - row) / 18.0f), 0, 5));
        v.lineBackdrop[y] = gs::rgb4(2, 1, 1);
    }

    auto place = [&](float x, float z, float& sx, float& sy, float& ppm, int& fog) {
        project(x, z, sx, sy, ppm);
        sx += shx;
        sy += shy;
        fog = std::clamp(int((z - 3.5f) * 1.4f), 0, 3);
    };

    if (mode_ == Mode::Title) text("S3 PINS GOLD", 160, 16, 1.12f, PAL_GOLD);
    else if (win) text("DOUBLE", 160, 16, 1.55f, PAL_GOLD);
    else if (mode_ == Mode::Banner || mode_ == Mode::Over) text("SHORT", 160, 16, 1.55f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 18, 1.3f, PAL_AMBER);

    spr(art_.lamp, 22, 62, 26, PAL_LAMP, false);
    spr(art_.lamp, 298, 62, 26, PAL_LAMP, true);

    int bp = pose();
    float feetRow = 150.0f;
    float feetX = 160.0f + stance_ * feetRow * LANE_K + shx;
    float feetY = float(HORIZON) + feetRow + shy;
    bool hand = !ballLive_ && mode_ != Mode::Roll && mode_ != Mode::Sweep && mode_ != Mode::Banner;
    if (hand) {
        float bx = feetX + 16, by = feetY - 34;
        if (bp == 1) {
            bx = feetX + 22;
            by = feetY - 16;
        } else if (bp == 2) {
            bx = feetX + 14;
            by = feetY - 58;
        }
        spr(art_.ball[int(clock_ * 3.0f) & 3], bx, by, 15, PAL_BALL, false);
    }
    float csx, csy, cppm;
    int cfog;
    if (ballLive_) {
        place(ballX_, std::max(1.15f, ballZ_), csx, csy, cppm, cfog);
        float dia = std::max(8.0f, cppm * 0.38f);
        spr(art_.ball[int(ballZ_ * 3.0f) & 3], csx, csy, dia, PAL_BALL, false, cfog, true);
    }
    spr(art_.bowler[bp], feetX, feetY, 66, PAL_BOWLER, false, 0, true);

    int order[10];
    for (int i = 0; i < 10; i++) order[i] = i;
    std::sort(order, order + 10, [&](int a, int b) { return pin_[a].z < pin_[b].z; });
    for (int k = 0; k < 10; k++) {
        const Pin& p = pin_[order[k]];
        if (p.hidden) continue;
        float sx, sy, ppm;
        int fog;
        place(p.x, p.z, sx, sy, ppm, fog);
        float ph = std::max(12.0f, ppm * 1.18f);
        int pal = p.gold ? PAL_GOLD : PAL_CREAM;
        if (p.fall <= 0.48f)
            spr(art_.pin, sx + p.side * p.fall * 4.0f, sy, ph * (1.0f - 0.2f * p.fall), pal, false, fog, true);
        else spr(art_.pinFlat, sx + p.side * 5.0f, sy, ph * 0.4f, pal, p.side < 0, fog, true);
    }
    if (ballLive_) {
        place(ballX_, std::max(1.15f, ballZ_), csx, csy, cppm, cfog);
        float dia = std::max(8.0f, cppm * 0.38f);
        stamp(art_.shadow, csx, csy, dia * 0.9f, dia * 0.28f, PAL_INK, cfog, true);
    }
    stamp(art_.shadow, feetX, feetY, 34, 8, PAL_INK, 0, true);
    for (int k = 0; k < 10; k++) {
        const Pin& p = pin_[order[k]];
        if (p.hidden) continue;
        float sx, sy, ppm;
        int fog;
        place(p.x, p.z, sx, sy, ppm, fog);
        float ph = std::max(12.0f, ppm * 1.18f);
        stamp(art_.shadow, sx, sy, ph * 0.55f, ph * 0.16f, PAL_INK, fog, true);
    }

    for (int i = 0; i < popN_; i++) {
        float sx, sy, ppm;
        int fog;
        place(pop_[i].x, pop_[i].z, sx, sy, ppm, fog);
        text("+2", sx, sy - 18.0f - pop_[i].t * 26.0f, 0.55f, PAL_GOLD);
    }

    bool guide = mode_ == Mode::Title || mode_ == Mode::Approach || mode_ == Mode::Swing ||
                 (mode_ == Mode::Pause && (held_ == Mode::Approach || held_ == Mode::Swing));
    bool hot = onPocket() && mode_ != Mode::Title;
    if (guide) {
        for (int i = 0; i < 7; i++) {
            float z = 1.65f + i * 0.26f;
            place(aimX(), z, csx, csy, cppm, cfog);
            spr(art_.dot, csx, csy, hot ? 7.0f : 5.0f, hot ? PAL_GREEN : PAL_GOLD, false, cfog);
        }
    }
    float pulse = 1.0f + 0.12f * std::sin(clock_ * 7.0f);
    for (float side : {-1.0f, 1.0f}) {
        bool sideHot = hot && ((side > 0.0f) == (aimX() >= 0.0f));
        for (int i = 0; i < 3; i++) {
            place(side * POCKET, 2.05f + i * 0.18f, csx, csy, cppm, cfog);
            float ah = std::max(8.0f, cppm * 0.22f) * (sideHot ? pulse : 1.0f);
            spr(art_.arrow, csx, csy, ah, sideHot ? PAL_GREEN : PAL_AMBER, false, cfog, true);
        }
    }
    for (int i = -3; i <= 3; i++) {
        place(i * 0.18f, 1.78f, csx, csy, cppm, cfog);
        spr(art_.dot, csx, csy, 4.0f, PAL_INK, false, cfog);
    }
    place(0, 1.58f, csx, csy, cppm, cfog);
    stamp(art_.foul, csx, csy, cppm * 2.05f, 3, PAL_RED, 0, false);
    place(0, Z0 + 1.35f * PIN_Z, csx, csy, cppm, cfog);
    stamp(art_.glow, csx, csy, cppm * 2.3f, cppm * 0.55f, PAL_GLOW, 1, false);
    place(0, Z0 + 3.15f * PIN_Z, csx, csy, cppm, cfog);
    stamp(art_.curtain, csx, csy - 6, cppm * 2.8f, 24, PAL_DECK, cfog, false);
    for (float side : {-1.32f, 1.32f}) {
        place(side, 2.35f, csx, csy, cppm, cfog);
        spr(art_.machine, csx, csy, std::max(16.0f, cppm * 0.7f), PAL_DECK, side < 0, cfog, true);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(5, "CREAM 1   GOLD 2   TEN WINS", PAL_INK);
        hudC(26, "SLIDE   C RELEASE   L R HOOKS", PAL_AMBER);
        std::snprintf(line, sizeof line, "START  %s", S3_VERSION_STRING);
        hudC(27, line, PAL_AMBER);
    } else if (mode_ == Mode::Banner || mode_ == Mode::Over) {
        hudC(5, win ? "ONLY THE GOLD COUNTS DOUBLE" : "GOLD LEFT ON THE DECK", win ? PAL_GOLD : PAL_AMBER);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hudC(6, line, win ? PAL_GREEN : PAL_RED);
        if (mode_ == Mode::Over) hudC(27, win ? "START" : "START RETRY", PAL_AMBER);
    } else if (mode_ != Mode::Pause) {
        hud(1, 0, "S3 PINS GOLD", PAL_GOLD);
        std::snprintf(line, sizeof line, "BALL %d", ball_ + 1);
        hud(33, 0, line, PAL_AMBER);
        int x = 10;
        for (int i = 0; i < 10; i++) {
            const Pin& p = pin_[i];
            char cell[2] = {char(p.down || p.hidden ? (p.gold ? '2' : '1') : (p.gold ? 'G' : 'C')), 0};
            int pal = p.gold ? PAL_GOLD : (p.down || p.hidden ? PAL_INK : PAL_CREAM);
            hud(x, 1, cell, pal);
            x += 2;
        }
        std::snprintf(line, sizeof line, "SCORE %d  NEED %d  GOLD x2", score_, NEED);
        hudC(2, line, score_ >= NEED ? PAL_GREEN : PAL_GOLD);
        const char* hint = "SLIDE ONTO A GOLD POCKET";
        int hpal = PAL_INK;
        if (mode_ == Mode::Swing) {
            if (onPocket()) {
                hint = starReady() ? "RELEASE" : "HOLD FOR THE STAR";
                hpal = PAL_GREEN;
            } else hint = "DOTS OFF THE POCKET";
        } else if (mode_ == Mode::Approach && onPocket()) {
            hint = "POCKET   C SWINGS";
            hpal = PAL_GREEN;
        } else if (mode_ == Mode::Roll) {
            hint = gutter_ ? "GUTTER" : (score_ >= NEED ? "DOUBLE" : "GOLD COUNTS DOUBLE");
            hpal = gutter_ ? PAL_RED : (score_ >= NEED ? PAL_GREEN : PAL_GOLD);
        } else if (mode_ == Mode::Sweep) {
            hint = "DEAD WOOD CLEARS";
            hpal = PAL_AMBER;
        }
        hudC(3, hint, hpal);
        hud(1, 27, "C", PAL_AMBER);
        hud(3, 27, mode_ == Mode::Swing ? "RELEASE" : "SWING", PAL_INK);
        hud(28, 27, "START PAUSE", PAL_INK);
    } else {
        hudC(26, "START RESUME", PAL_AMBER);
        hudC(27, "ESC TITLE", PAL_INK);
    }
}

}  // namespace pinsgold
