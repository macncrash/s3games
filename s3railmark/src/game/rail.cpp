#include "game/rail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace rail {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float RAIL_Y = 170.0f;
constexpr float MARK = 720.0f;
constexpr float TOL = 4.5f;
constexpr float ACCEL = 86.0f;
constexpr float BRAKE = 168.0f;
constexpr float COAST = 22.0f;
constexpr float MAX_V = 150.0f;
constexpr float SETTLE = 6.0f;
constexpr float HOLD = 0.32f;
constexpr float CREW = 16.5f;
constexpr float CAM = 96.0f;
constexpr float START = 48.0f;

gs::FMPatch motorPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.35f;
    p.op[0] = {1, 0.75f, 0.02f, 0.28f, 0.75f, 0.18f};
    p.op[1] = {1, 1, 0.01f, 0.22f, 0.65f, 0.14f};
    p.op[2] = {2, 0.3f, 0.02f, 0.3f, 0.35f, 0.18f};
    p.op[3] = {3, 0.18f, 0.03f, 0.35f, 0.28f, 0.2f};
    p.vol = 0.11f;
    p.drive = 0.16f;
    p.tone = 1200;
    return p;
}

gs::FMPatch brassPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.28f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.7f, 0.1f};
    p.op[1] = {2, 0.45f, 0.01f, 0.18f, 0.5f, 0.1f};
    p.op[2] = {3, 0.28f, 0.02f, 0.22f, 0.4f, 0.12f};
    p.op[3] = {1, 0.3f, 0.01f, 0.18f, 0.5f, 0.1f};
    p.vol = 0.2f;
    return p;
}

}  // namespace

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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    x -= w * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y + g.h * scale, g.h * scale, pal, false);
    }
}

void Game::blip(float freq) {
    sys_->apu.tone(2, freq, 0.09f);
    beep_ = 0.07f;
}

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
}

float Game::brakeStop() const {
    float pred = x_;
    float v = vx_;
    for (int i = 0; i < 500; i++) {
        v -= BRAKE * DT;
        if (v < 0) v = 0;
        pred += v * DT;
        if (v <= 0) break;
    }
    return pred;
}

void Game::beginRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    t_ = 0;
    x_ = START;
    vx_ = 0;
    clock_ = 0;
    crewLeft_ = CREW;
    still_ = 0;
    shake_ = 0;
    setT_ = 0;
    fail_ = "MISSED THE MARK";
    if (!engineOn_) {
        sys_->apu.setPatch(0, motorPatch());
        sys_->apu.setPatch(1, brassPatch());
        sys_->apu.keyOn(0, 64, 0.07f);
        engineOn_ = true;
    }
}

void Game::physics(float dt, bool gas, bool brake) {
    if (gas && !brake) vx_ += ACCEL * dt;
    else if (brake) vx_ -= BRAKE * dt;
    else vx_ -= COAST * dt;
    vx_ = std::clamp(vx_, 0.0f, MAX_V);
    x_ += vx_ * dt;

    clock_ += dt;
    crewLeft_ = std::max(0.0f, CREW - clock_);
    if (clock_ >= CREW) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        fail_ = "CREW TOOK THE MARK";
        sys_->apu.keyOff(0);
        engineOn_ = false;
        return;
    }
    if (x_ > MARK + TOL) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        fail_ = "PAST THE MARK";
        shake_ = 0.3f;
        sys_->apu.noiseBurst(0.35f, 500, 0.18f);
        sys_->apu.keyOff(0);
        engineOn_ = false;
        return;
    }
    if (vx_ < SETTLE) {
        still_ += dt;
        if (still_ >= HOLD) {
            if (std::fabs(x_ - MARK) <= TOL) {
                mode_ = Mode::Set;
                setT_ = 0;
                vx_ = 0;
                fanfare();
                blip(880);
            } else {
                mode_ = Mode::Over;
                over_ = true;
                won_ = false;
                fail_ = x_ < MARK ? "SHORT OF THE MARK" : "WIDE OF THE MARK";
                sys_->apu.keyOff(0);
                engineOn_ = false;
            }
        }
    } else {
        still_ = 0;
    }
}

void Game::update(float dt) {
    gs::Pad& pad = sys_->pad;
    if (mode_ == Mode::Title) {
        bool go = bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        if (go) beginRun();
        return;
    }
    if (mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            over_ = false;
            beginRun();
        }
        return;
    }
    if (mode_ == Mode::Set) {
        setT_ += dt;
        if (setT_ > 0.85f) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Over;
            crewLeft_ = std::max(0.0f, CREW - clock_);
            sys_->apu.keyOff(0);
            engineOn_ = false;
        }
        return;
    }

    bool gas = false, brake = false;
    if (bot_) {
        float stop = brakeStop();
        if (stop >= MARK - 0.2f) brake = true;
        else gas = true;
    } else {
        gas = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.35f ||
              pad.axisX > 0.25f;
        brake = pad.down(gs::BTN_LEFT) || pad.axisX < -0.25f || pad.brake > 0.35f;
    }
    physics(dt, gas, brake);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float u = y / float(gs::SCREEN_H);
        if (y < 128) v.lineBackdrop[y] = gs::rgb4(3 + int(u * 4), 5 + int(u * 6), 11);
        else v.lineBackdrop[y] = gs::rgb4(4, 4, 3);
    }
    float cam = x_ - CAM;
    float sh = 0;
    if (shake_ > 0) sh = std::sin(t_ * 80.0f) * 3.0f * shake_;

    for (int i = -1; i < 6; i++) {
        float hx = std::floor((cam + i * 110) / 110.0f) * 110.0f;
        spr(art_.hill, hx - cam + 50, 128 + sh, 32, PAL_HILL);
    }

    float rivalX = MARK * std::min(clock_ / CREW, 1.0f);
    if (mode_ != Mode::Title) spr(art_.rival, rivalX - cam, RAIL_Y - 6 + sh, 24, PAL_RIVAL);

    for (float s = std::floor((cam - 30) / 28.0f) * 28.0f; s < cam + gs::SCREEN_W + 30; s += 28.0f) {
        bool painted = std::fabs(s - MARK) < 10.0f;
        spr(art_.sleeper, s - cam, RAIL_Y + 8 + sh, 7, painted ? PAL_MARK : PAL_IRON);
        spr(art_.rail, s - cam, RAIL_Y + sh, 5, PAL_IRON);
    }
    spr(art_.mark, MARK - cam, RAIL_Y + 4 + sh, 22, PAL_MARK);
    spr(art_.post, MARK - cam + 28, RAIL_Y + sh, 58, PAL_POST);

    float cy = RAIL_Y + sh;
    spr(art_.cart, CAM, cy, 34, PAL_CART);
    if (brakeStop() > 0 && vx_ > 40 && mode_ == Mode::Run) {
        float bob = std::sin(t_ * 30.0f) * 2.0f;
        spr(art_.spark, CAM - 22, cy - 2 + bob, 8, PAL_AMBER);
    }

    char buf[72];
    if (mode_ == Mode::Title) {
        text("S3 RAILMARK", 160, 46, 1.05f, PAL_AMBER);
        hudC(12, "SET DOWN ON THE MARK", PAL_HUD);
        hudC(14, "CLOSE STILL FAILS THE LEG", PAL_GREEN);
        hudC(17, "THE CLOCK IS THE OTHER CREW", PAL_RED);
        if (int(t_ * 2) % 2 == 0) hudC(21, "PRESS START", PAL_GREEN);
        hudC(24, "RIGHT RUN    LEFT BRAKE", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
    } else {
        int sec = int(std::ceil(std::max(0.0f, crewLeft_)));
        std::snprintf(buf, sizeof buf, "CREW %02d", sec);
        hud(1, 1, buf, sec < 6 ? PAL_RED : PAL_AMBER);
        float gap = MARK - x_;
        if (std::fabs(gap) <= TOL && vx_ < SETTLE) std::snprintf(buf, sizeof buf, "ON MARK");
        else if (gap > 0) std::snprintf(buf, sizeof buf, "MARK %3.0f", gap);
        else std::snprintf(buf, sizeof buf, "PAST");
        hud(28, 1, buf, std::fabs(x_ - MARK) <= TOL ? PAL_GREEN : PAL_HUD);
        if (mode_ == Mode::Set || (mode_ == Mode::Over && won_)) {
            text("SET ON THE MARK", 160, 58, 0.8f, PAL_GREEN);
            std::snprintf(buf, sizeof buf, "CREW %.1f S BEHIND", crewLeft_);
            hudC(16, buf, PAL_AMBER);
        } else if (mode_ == Mode::Over) {
            text(fail_, 160, 58, 0.7f, PAL_RED);
            hudC(18, "START TO TAKE THE RAIL", PAL_HUD);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    float dt = DT;
    t_ += dt;
    if (shake_ > 0) shake_ -= dt;
    update(dt);
    draw();
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys.apu.tone(2, 0, 0);
    }
    if (engineOn_) {
        float burble = 1.0f + 0.05f * std::sin(t_ * 36.0f);
        sys.apu.setFreq(0, (58.0f + vx_ * 0.7f) * burble);
        sys.apu.setVol(0, mode_ == Mode::Run ? 0.11f : 0.04f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.0f, 494.0f, 587.0f, 784.0f};
        fanT_ += dt;
        if (fanT_ > 0.13f) {
            if (fanStep_ < 4) sys.apu.keyOn(1, notes[fanStep_], 0.2f);
            else sys.apu.keyOff(1);
            fanStep_++;
            fanT_ = 0;
            if (fanStep_ > 6) fanStep_ = -1;
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 11));
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.18f, 0.1f);
    mode_ = Mode::Title;
    if (bot_) beginRun();
}

}  // namespace rail
