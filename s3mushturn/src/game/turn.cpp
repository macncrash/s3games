#include "game/turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mushturn {

namespace {
constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 92;
constexpr float FINISH = 176.0f;
constexpr float HALF = 2.75f;
constexpr float NEED = 1.05f;
constexpr float TIP_LEAN = 0.92f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float smooth(float a, float b, float z, float z0, float z1) {
    if (z <= z0) return a;
    if (z >= z1) return b;
    float u = (z - z0) / (z1 - z0);
    u = u * u * (3.0f - 2.0f * u);
    return a + (b - a) * u;
}
}  // namespace

float Game::heading(float z) const {
    if (z < 30.0f) return 0.0f;
    if (z < 52.0f) return smooth(0.0f, 0.82f, z, 30.0f, 52.0f);
    if (z < 80.0f) return 0.82f;
    if (z < 102.0f) return smooth(0.82f, -0.48f, z, 80.0f, 102.0f);
    if (z < 130.0f) return -0.48f;
    if (z < 152.0f) return smooth(-0.48f, 0.64f, z, 130.0f, 152.0f);
    return 0.64f;
}

void Game::bake() {
    float x = 0;
    curveTab_[0] = 0;
    for (int i = 0; i < 200; i++) {
        float h = heading(float(i) + 0.5f);
        x += std::sin(h) * 0.82f;
        curveTab_[i + 1] = x;
    }
    corners_[0] = {30.0f, 52.0f, 1.0f, 0, false};
    corners_[1] = {80.0f, 102.0f, -1.0f, 0, false};
    corners_[2] = {130.0f, 152.0f, 1.0f, 0, false};
}

float Game::curve(float z) const {
    if (z < 0) z = 0;
    if (z > 199.0f) z = 199.0f;
    int i = int(z);
    float f = z - float(i);
    return curveTab_[i] * (1.0f - f) + curveTab_[i + 1] * f;
}

int Game::cornerAt(float z) const {
    for (int i = 0; i < 3; i++)
        if (z >= corners_[i].z0 && z < corners_[i].z1) return i;
    return -1;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.14f);
    blip_ = 0.12f;
}

void Game::miss(const char* why) {
    note_ = why;
    flash_ = 0.32f;
    lives_--;
    blip(90);
    if (lives_ <= 0) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        hold_ = 0;
    } else {
        mode_ = Mode::Miss;
        hold_ = 1.15f;
    }
}

void Game::restartLeg() {
    pz_ = 2.0f;
    x_ = curve(pz_);
    speed_ = 9.0f;
    lean_ = 0;
    wrong_ = 0;
    flatLean_ = 0;
    turnsMade_ = 0;
    for (int i = 0; i < 3; i++) {
        corners_[i].credit = 0;
        corners_[i].made = false;
    }
    note_ = "HOLD THE LEAN";
}

void Game::begin() {
    lives_ = 3;
    over_ = false;
    won_ = false;
    mode_ = Mode::Run;
    restartLeg();
}

void Game::botInput(float& leanWant, float& throttle) const {
    throttle = 0;
    leanWant = 0;
    for (int i = 0; i < 3; i++) {
        const Corner& c = corners_[i];
        if (pz_ > c.z0 - 2.6f && pz_ < c.z1 - 0.15f) leanWant = c.dir * 0.64f;
    }
}

void Game::mush(float dt) {
    float leanWant = 0;
    float throttle = 0;
    if (bot_) {
        botInput(leanWant, throttle);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) leanWant = -0.70f;
        if (pad.down(gs::BTN_RIGHT)) leanWant = 0.70f;
        if (pad.down(gs::BTN_UP)) throttle = 1;
        if (pad.down(gs::BTN_DOWN)) throttle = -1;
        float aim = curve(pz_ + 3.0f);
        x_ += (aim - x_) * std::min(1.0f, 4.0f * dt);
        x_ += pad.axisX * 2.4f * dt;
        if (std::fabs(pad.axisX) > 0.45f && leanWant == 0) leanWant = clampf(pad.axisX, -1.0f, 1.0f) * 0.70f;
    }

    if (bot_) {
        float aim = curve(pz_) + (curve(pz_ + 3.0f) - curve(pz_)) * 0.28f;
        x_ += (aim - x_) * std::min(1.0f, 10.0f * dt);
    }

    float rate = 4.2f;
    lean_ += clampf(leanWant - lean_, -rate * dt, rate * dt);

    float spdTarget = 9.0f + throttle * 3.6f;
    speed_ += (spdTarget - speed_) * std::min(1.0f, 2.4f * dt);
    pz_ += speed_ * dt;
    stride_ += speed_ * dt * 0.22f;

    float off = x_ - curve(pz_);
    if (std::fabs(off) > HALF + 0.45f) {
        miss("MISSED THE END");
        return;
    }
    if (std::fabs(lean_) > TIP_LEAN) {
        miss("TIPPED");
        return;
    }

    int ci = cornerAt(pz_);
    if (ci >= 0) {
        Corner& c = corners_[ci];
        float side = lean_ * c.dir;
        if (side > 0.40f && side < 0.86f) c.credit += dt;
        if (side < -0.35f) wrong_ += dt;
        else wrong_ = 0;
        flatLean_ = 0;
        if (wrong_ > 0.28f || (speed_ > 12.15f && std::fabs(lean_) > 0.4f)) {
            miss("TIPPED");
            return;
        }
    } else {
        wrong_ = 0;
        if (std::fabs(lean_) > 0.38f) flatLean_ += dt;
        else flatLean_ = 0;
        if (flatLean_ > 0.55f) {
            miss("TIPPED");
            return;
        }
        for (int i = 0; i < 3; i++) {
            Corner& c = corners_[i];
            if (!c.made && pz_ >= c.z1) {
                if (c.credit >= NEED) {
                    c.made = true;
                    turnsMade_++;
                    note_ = turnsMade_ == 1 ? "TURN ONE" : turnsMade_ == 2 ? "TURN TWO" : "TURN THREE";
                    blip(640);
                } else {
                    miss("TURN MISSED");
                    return;
                }
            }
        }
    }

    if (pz_ >= FINISH) {
        if (turnsMade_ >= 3 && std::fabs(off) <= HALF + 0.2f) {
            mode_ = Mode::Win;
            over_ = true;
            won_ = true;
            note_ = "LEG MADE";
            blip(880);
        } else {
            miss("MISSED THE END");
        }
    }
}

void Game::project(float wx, float wz, float& sx, float& sy, float& ppm) const {
    float d = wz - pz_;
    if (d < 0.85f) d = 0.85f;
    float n = 1.0f - std::sqrt(clampf((d - 1.1f) / 46.0f, 0.0f, 1.0f));
    sy = float(HORIZON) + n * float(gs::SCREEN_H - 8 - HORIZON);
    ppm = 170.0f / d;
    sx = 160.0f + (wx - x_) * ppm;
}

void Game::spr(const gs::Mipped& m, float cx, float footY, float destH, int pal, bool flip) {
    if (destH < 2.0f) return;
    gs::Sprite s;
    s.img = m.pick(destH);
    s.h = std::max(1, int(destH));
    s.w = std::max(1, int(m.w * destH / float(m.h)));
    s.x = int(cx - s.w * 0.5f);
    s.y = int(footY - s.h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::glyphText(const char* s, int x, int y, int scale, int pal) {
    int cw = 6 * scale;
    for (const char* p = s; *p; p++) {
        unsigned c = unsigned(*p);
        if (c < 32 || c > 127) c = '?';
        gs::Sprite sp;
        sp.img = art_.glyph[c - 32];
        sp.x = int16_t(x);
        sp.y = int16_t(y);
        sp.w = int16_t(5 * scale);
        sp.h = int16_t(7 * scale);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        x += cw;
    }
}

void Game::paintSky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = y / float(HORIZON);
            v.lineBackdrop[y] = gs::rgb4(int(6 + 5 * u), int(8 + 4 * u), int(12 + 2 * u));
            v.lineFog[y] = uint8_t((HORIZON - y) / 18);
            v.road[y].on = false;
        } else {
            v.lineBackdrop[y] = gs::rgb4(6, 7, 9);
            float n = (y - HORIZON) / float(gs::SCREEN_H - HORIZON);
            v.lineFog[y] = uint8_t(clampf((1.0f - n) * 6.0f, 0.0f, 10.0f));
        }
    }
}

void Game::paintTrail() {
    gs::VDP& v = sys_->vdp;
    v.roadTime = int(t_ * 20);
    for (int y = HORIZON; y < gs::SCREEN_H; y++) {
        float n = (y - HORIZON) / float(gs::SCREEN_H - 1 - HORIZON);
        float d = 1.1f + 46.0f * (1.0f - n) * (1.0f - n);
        float ppm = 170.0f / d;
        float z = pz_ + d;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f + (curve(z) - x_) * ppm;
        r.hw = HALF * ppm;
        r.v = pz_ * 12.0f + d * 16.0f;
        r.pal = PAL_SNOW;
        r.band = (int(z * 0.5f) & 1);
        r.style = gs::ROAD_SNOW;
        r.left = gs::GROUND_SNOWWALL;
        r.right = gs::GROUND_SNOWWALL;
    }
}

void Game::paintWorld() {
    struct Item {
        float z;
        int kind;
        float wx;
        bool flip;
    };
    Item items[40];
    int n = 0;
    auto push = [&](float z, int kind, float wx, bool flip) {
        if (n < 40 && z > pz_ + 0.9f && z < pz_ + 48.0f) items[n++] = {z, kind, wx, flip};
    };
    float base = std::floor(pz_ / 9.0f) * 9.0f;
    for (int i = 0; i < 7; i++) {
        float z = base + i * 9.0f;
        float c = curve(z);
        push(z, 0, c - HALF - 1.15f, false);
        push(z + 4.5f, 0, c + HALF + 1.2f, true);
    }
    for (int i = 0; i < 3; i++) {
        const Corner& cnr = corners_[i];
        float z = (cnr.z0 + cnr.z1) * 0.5f;
        float c = curve(z);
        float side = cnr.dir > 0 ? 1.0f : -1.0f;
        push(z, 1, c + side * (HALF + 0.15f), cnr.dir < 0);
        push(cnr.z0 + 2.0f, 1, curve(cnr.z0 + 2.0f) + side * (HALF + 0.15f), cnr.dir < 0);
        push(cnr.z1 - 2.0f, 1, curve(cnr.z1 - 2.0f) + side * (HALF + 0.15f), cnr.dir < 0);
    }
    float fc = curve(FINISH);
    push(FINISH, 2, fc - HALF + 0.15f, false);
    push(FINISH, 2, fc + HALF - 0.15f, true);

    std::sort(items, items + n, [](const Item& a, const Item& b) { return a.z < b.z; });
    for (int i = 0; i < n; i++) {
        float sx, sy, ppm;
        project(items[i].wx, items[i].z, sx, sy, ppm);
        if (sx < -70 || sx > 390) continue;
        if (items[i].kind == 0) spr(art_.spruce, sx, sy, 4.6f * ppm, PAL_PINE, items[i].flip);
        else if (items[i].kind == 1) spr(art_.chevron, sx, sy, 1.8f * ppm, PAL_SIGN, items[i].flip);
        else spr(art_.arch, sx, sy, 3.4f * ppm, PAL_ARCH, items[i].flip);
    }
}

void Game::paintTeam() {
    float bob = std::sin(stride_ * 6.2832f) * 2.0f;
    int fr = int(std::fmod(stride_, 1.0f) * 3.0f);
    if (fr < 0) fr = 0;
    if (fr > 2) fr = 2;
    float foot = 196.0f + bob;
    float slide = lean_ * 36.0f;
    bool flip = lean_ < -0.05f;
    spr(art_.sled, 160 + slide, foot, 46, PAL_TEAM, flip);
    spr(art_.musher, 160 + slide * 0.85f, foot - 18, 34, PAL_TEAM, flip);
    spr(art_.dog[fr], 168 + slide * 0.4f, foot - 34, 18, PAL_TEAM, false);
    spr(art_.dog[(fr + 1) % 3], 148 + slide * 0.25f, foot - 40, 16, PAL_TEAM, false);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    bake();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.72f);
    sys.apu.silence();
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (blip_ > 0) {
        blip_ -= DT;
        if (blip_ <= 0) sys.apu.tone(1, 0, 0);
    }
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        pz_ = 6.0f;
        x_ = curve(pz_);
        lean_ = std::sin(t_ * 1.3f) * 0.25f;
        stride_ += DT * 1.4f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            blip(520);
        }
    } else if (mode_ == Mode::Run) {
        mush(DT);
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            blip(180);
        }
    } else if (mode_ == Mode::Miss) {
        hold_ -= DT;
        if (hold_ <= 0) {
            restartLeg();
            mode_ = Mode::Run;
        }
    } else {
        stride_ += DT * 0.4f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) begin();
    }

    if (flash_ > 0) flash_ -= DT;
    paintSky();
    if (flash_ > 0.1f) {
        for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(12, 12, 14);
    }
    paintTrail();
    sys.vdp.clearSprites();
    paintTeam();
    paintWorld();

    char buf[64];
    if (mode_ == Mode::Title) {
        glyphText("MUSH TURN", 86, 24, 2, PAL_GOLD);
        glyphText("THREE TURNS WITHOUT TIPPING", 58, 52, 1, PAL_INK);
        glyphText("MISS THE END AND THE LEG FAILS", 52, 66, 1, PAL_INK);
        glyphText("LEFT RIGHT LEAN   UP IS FAST", 64, 92, 1, PAL_GOLD);
        glyphText("PRESS START", 118, 150, 1, PAL_INK);
    } else {
        std::snprintf(buf, sizeof buf, "TURNS %d/3", turnsMade_);
        glyphText(buf, 8, 8, 1, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        glyphText(buf, 230, 8, 1, PAL_INK);
        int bars = int(std::fabs(lean_) * 8.0f);
        if (bars > 8) bars = 8;
        buf[0] = lean_ < -0.05f ? 'L' : lean_ > 0.05f ? 'R' : '-';
        for (int i = 0; i < bars; i++) buf[1 + i] = '|';
        buf[1 + bars] = 0;
        glyphText(buf, 8, 20, 1, std::fabs(lean_) > 0.78f ? PAL_ARCH : PAL_INK);
        if (note_[0]) glyphText(note_, 8, 34, 1, PAL_GOLD);
        if (mode_ == Mode::Win) glyphText("LEG MADE", 110, 78, 2, PAL_GOLD);
        if (mode_ == Mode::Lose) glyphText("LEG FAILED", 98, 78, 2, PAL_ARCH);
    }
}

}  // namespace mushturn
