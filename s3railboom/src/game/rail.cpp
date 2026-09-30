#include "game/rail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace rail {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float RAIL_Y = 168.0f;
constexpr float GRAV = 980.0f;
constexpr float JUMP = 360.0f;
constexpr float MAX_V = 250.0f;
constexpr float ACCEL = 230.0f;
constexpr float FINISH = 8400.0f;
constexpr float CREW = 54.0f;
constexpr float CAM = 108.0f;

struct Span {
    float a, b;
};
constexpr Span GAPS[] = {{1400, 1490}, {3100, 3200}, {5000, 5110}, {6900, 7000}};
constexpr float BEAMS[] = {2200, 4100, 6000, 7600};

gs::FMPatch motorPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.4f;
    p.op[0] = {1, 0.8f, 0.02f, 0.3f, 0.8f, 0.2f};
    p.op[1] = {1, 1, 0.01f, 0.25f, 0.7f, 0.15f};
    p.op[2] = {2, 0.35f, 0.02f, 0.3f, 0.4f, 0.2f};
    p.op[3] = {3, 0.2f, 0.03f, 0.4f, 0.3f, 0.25f};
    p.vol = 0.12f;
    p.drive = 0.2f;
    p.tone = 1400;
    return p;
}

gs::FMPatch brassPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.3f;
    p.op[0] = {1, 1, 0.01f, 0.18f, 0.7f, 0.12f};
    p.op[1] = {2, 0.5f, 0.01f, 0.2f, 0.5f, 0.12f};
    p.op[2] = {3, 0.3f, 0.02f, 0.25f, 0.4f, 0.15f};
    p.op[3] = {1, 0.35f, 0.01f, 0.2f, 0.55f, 0.12f};
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
    sys_->apu.tone(0, freq, 0.08f);
    beep_ = 0.06f;
}

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
}

bool Game::gapAt(float x) const {
    for (const Span& g : GAPS)
        if (x > g.a && x < g.b) return true;
    return false;
}

float Game::respawnBefore(float x) const {
    float s = 80;
    for (const Span& g : GAPS)
        if (g.b < x - 40) s = g.b + 40;
    return s;
}

void Game::beginRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    onRail_ = true;
    coupled_ = true;
    ducked_ = false;
    lives_ = 3;
    strikes_ = 0;
    t_ = 0;
    x_ = 80;
    y_ = 0;
    vx_ = 0;
    vy_ = 0;
    clock_ = 0;
    crewLeft_ = CREW;
    hitCd_ = 0;
    recouple_ = 0;
    shake_ = 0;
    deliver_ = 0;
    if (!engineOn_) {
        sys_->apu.setPatch(0, motorPatch());
        sys_->apu.setPatch(1, brassPatch());
        sys_->apu.keyOn(0, 70, 0.08f);
        engineOn_ = true;
    }
}

void Game::physics(float dt, bool gas, bool brake, bool jump, bool duck) {
    ducked_ = duck && y_ <= 0.5f;
    float cap = ducked_ ? 150.0f : MAX_V;
    if (gas) vx_ += ACCEL * dt;
    else if (brake) vx_ -= ACCEL * 1.3f * dt;
    else vx_ -= 50.0f * dt;
    vx_ = std::clamp(vx_, 0.0f, cap);
    if (jump && y_ <= 0.5f && onRail_ && !ducked_) {
        vy_ = JUMP;
        onRail_ = false;
        blip(620);
    }
    x_ += vx_ * dt;
    y_ += vy_ * dt;
    if (y_ > 0.5f) {
        vy_ -= GRAV * dt;
        onRail_ = false;
    }
    if (y_ <= 0) {
        y_ = 0;
        vy_ = 0;
        float nose = x_ + 16;
        float tail = x_ - 16;
        if (gapAt(nose) || gapAt(tail) || gapAt(x_)) {
            lives_--;
            shake_ = 0.35f;
            sys_->apu.noiseBurst(0.4f, 700, 0.2f);
            if (lives_ <= 0) {
                mode_ = Mode::Over;
                over_ = true;
                won_ = false;
                sys_->apu.keyOff(0);
                engineOn_ = false;
                return;
            }
            x_ = respawnBefore(x_);
            vx_ = 40;
            onRail_ = true;
            coupled_ = true;
        } else {
            onRail_ = true;
        }
    }

    if (hitCd_ > 0) hitCd_ -= dt;
    if (onRail_ && y_ < 8 && !ducked_ && hitCd_ <= 0) {
        for (float b : BEAMS) {
            if (std::fabs(x_ - b) < 28) {
                vx_ *= 0.35f;
                strikes_++;
                hitCd_ = 0.45f;
                shake_ = 0.25f;
                blip(180);
                if (strikes_ >= 3) {
                    coupled_ = false;
                    strikes_ = 0;
                }
            }
        }
    }

    if (!coupled_ && onRail_ && duck) {
        recouple_ += dt;
        if (recouple_ > 0.55f) {
            coupled_ = true;
            recouple_ = 0;
            blip(440);
        }
    } else if (!duck) {
        recouple_ = 0;
    }

    clock_ += dt;
    float crewX = FINISH * std::min(clock_ / CREW, 1.0f);
    crewLeft_ = std::max(0.0f, CREW - clock_);
    if (clock_ >= CREW && mode_ == Mode::Run) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
        sys_->apu.keyOff(0);
        engineOn_ = false;
        return;
    }
    if (mode_ == Mode::Run && coupled_ && onRail_ && x_ >= FINISH && crewX < FINISH - 1) {
        mode_ = Mode::Deliver;
        deliver_ = 0;
        vx_ = 0;
        fanfare();
        blip(880);
    }
    (void)crewX;
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
    if (mode_ == Mode::Deliver) {
        deliver_ += dt;
        if (deliver_ > 1.1f) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Over;
            crewLeft_ = std::max(0.0f, CREW - clock_);
        }
        return;
    }

    bool gas = false, brake = false, jump = false, duck = false;
    if (bot_) {
        gas = true;
        float nextGap = 1e9f;
        for (const Span& g : GAPS)
            if (g.a > x_ - 8) nextGap = std::min(nextGap, g.a);
        if (y_ <= 0.5f && nextGap < 1e8f && nextGap - x_ < 62.0f && nextGap - x_ > 18.0f) jump = true;
        duck = false;
        for (float b : BEAMS)
            if (b - x_ < 78.0f && b - x_ > -36.0f) duck = true;
        if (!coupled_) duck = true;
    } else {
        gas = pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.accel > 0.4f ||
              pad.axisX > 0.25f;
        brake = pad.down(gs::BTN_LEFT) || pad.axisX < -0.25f || pad.brake > 0.4f;
        jump = pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_B) || pad.axisY > 0.55f;
        duck = pad.down(gs::BTN_DOWN) || pad.axisY < -0.45f;
    }
    physics(dt, gas, brake, jump, duck);
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
        if (y < 120) v.lineBackdrop[y] = gs::rgb4(2 + int(u * 6), 4 + int(u * 8), 10 + int((1 - u) * 4));
        else v.lineBackdrop[y] = gs::rgb4(3, 5, 3);
    }
    float cam = x_ - CAM;
    float sh = 0;
    if (shake_ > 0) sh = std::sin(t_ * 90.0f) * 4.0f * shake_;

    for (int i = -1; i < 6; i++) {
        float hx = std::floor((cam + i * 90) / 90.0f) * 90.0f;
        spr(art_.hill, hx - cam + 40, 132 + sh, 36, PAL_HILL);
    }

    float rivalX = FINISH * std::min(clock_ / CREW, 1.0f);
    spr(art_.rival, rivalX - cam, RAIL_Y - 18 + sh, 26, PAL_RIVAL);

    for (float s = std::floor((cam - 20) / 24.0f) * 24.0f; s < cam + gs::SCREEN_W + 24; s += 24.0f) {
        if (gapAt(s)) continue;
        spr(art_.sleeper, s - cam, RAIL_Y + 6 + sh, 7, PAL_IRON);
        spr(art_.rail, s - cam, RAIL_Y + sh, 5, PAL_IRON);
    }
    for (const Span& g : GAPS) {
        // open the rail: already skipped sleepers
        (void)g;
    }
    for (float b : BEAMS) spr(art_.beam, b - cam, RAIL_Y + sh, 52, PAL_BOOM);

    float boomX = FINISH - cam;
    spr(art_.boom, boomX + 20, RAIL_Y + sh, 78, PAL_BOOM);

    float cartH = ducked_ ? 22.0f : 36.0f;
    float cy = RAIL_Y - y_ + sh;
    spr(ducked_ ? art_.cartDuck : art_.cart, CAM + sh * 0.2f, cy, cartH, PAL_CART);
    if (coupled_) {
        float bob = std::sin(t_ * 18.0f) * (vx_ > 20 ? 1.5f : 0.3f);
        spr(art_.drive, CAM + 6, cy - cartH + 6 + bob, 16, PAL_DRIVE);
    } else {
        spr(art_.drive, CAM + 6, cy - 28 - std::sin(t_ * 8) * 3, 16, PAL_RED);
    }
    if (shake_ > 0.05f) spr(art_.spark, CAM - 16, cy, 10, PAL_AMBER);

    char buf[64];
    if (mode_ == Mode::Title) {
        text("S3 RAILBOOM", 160, 48, 1.15f, PAL_AMBER);
        hudC(12, "DELIVER THE DRIVE", PAL_HUD);
        hudC(14, "TO THE BOOM", PAL_HUD);
        hudC(17, "THE CLOCK IS THE OTHER CREW", PAL_RED);
        if (int(t_ * 2) % 2 == 0) hudC(21, "PRESS START", PAL_GREEN);
        hudC(24, "RIGHT RUN   UP JUMP   DOWN DUCK", PAL_HUD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_HUD);
    } else {
        int sec = int(std::ceil(std::max(0.0f, crewLeft_)));
        std::snprintf(buf, sizeof buf, "CREW %02d", sec);
        hud(1, 1, buf, sec < 10 ? PAL_RED : PAL_AMBER);
        std::snprintf(buf, sizeof buf, "X%d", std::max(0, lives_));
        hud(33, 1, buf, lives_ > 1 ? PAL_HUD : PAL_RED);
        int bars = int(std::clamp(x_ / FINISH, 0.0f, 1.0f) * 16);
        int crewBars = int(std::clamp(rivalX / FINISH, 0.0f, 1.0f) * 16);
        std::string mark(16, '.');
        for (int i = 0; i < 16; i++) {
            if (i == bars) mark[i] = 'D';
            else if (i == crewBars) mark[i] = 'C';
            else if (i < bars) mark[i] = '=';
        }
        hud(12, 1, mark, PAL_GREEN);
        if (!coupled_) hudC(4, "DRIVE LOOSE  HOLD DOWN", PAL_RED);
        if (mode_ == Mode::Deliver || (mode_ == Mode::Over && won_)) {
            text("DRIVE ON THE BOOM", 160, 64, 0.85f, PAL_GREEN);
            std::snprintf(buf, sizeof buf, "CREW %.1f S BEHIND", crewLeft_);
            hudC(16, buf, PAL_AMBER);
        } else if (mode_ == Mode::Over) {
            text(lives_ <= 0 ? "CART IN THE GAP" : "CREW TOOK THE BOOM", 160, 64, 0.75f, PAL_RED);
            hudC(18, "START TO RUN AGAIN", PAL_HUD);
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
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }
    if (engineOn_) {
        float burble = 1.0f + 0.04f * std::sin(t_ * 40.0f);
        float spd = 60.0f + vx_ * 0.55f;
        sys.apu.setFreq(0, spd * burble);
        sys.apu.setVol(0, mode_ == Mode::Run ? 0.12f : 0.05f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {392.0f, 523.0f, 659.0f, 784.0f};
        fanT_ += dt;
        if (fanT_ > 0.14f) {
            if (fanStep_ < 4) sys.apu.keyOn(1, notes[fanStep_], 0.22f);
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
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    mode_ = Mode::Title;
    if (bot_) beginRun();
}

}  // namespace rail
