#include "cistern.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace purse {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float TWO = 6.2831853f;
constexpr float PI = 3.14159265f;
constexpr float CX = 160.f;
constexpr float CY = 112.f;
constexpr float RX = 108.f;
constexpr float RY = 52.f;
constexpr float STRIKE = 0.32f;
constexpr float BITE = 0.16f;
constexpr float WALK = 1.85f;

float wrap(float a) {
    while (a < 0) a += TWO;
    while (a >= TWO) a -= TWO;
    return a;
}

float angDiff(float from, float to) {
    float d = wrap(to) - wrap(from);
    if (d > PI) d -= TWO;
    if (d < -PI) d += TWO;
    return d;
}

float speedOf(int kind) {
    if (kind == 1) return 0.42f;
    if (kind == 2) return 1.05f;
    return 0.62f;
}
int hpOf(int kind) { return kind == 1 ? 2 : 1; }
int ptsOf(int kind) { return kind == 1 ? 250 : kind == 2 ? 180 : 100; }

struct Arr {
    float t;
    int kind;
    float ang;
};
const Arr kArr[] = {
    {0.5f, 0, PI},
    {3.6f, 0, 2.2f},
    {7.2f, 1, 4.4f},
    {11.4f, 2, 0.9f},
    {15.2f, 0, 5.4f},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Watch && others() == 1 && stalled_ > 0) return 2;
    return 1;
}

void Game::place(float ang, float& x, float& y) const {
    x = CX + std::cos(ang) * RX;
    y = CY + std::sin(ang) * RY;
}

int Game::nearest() const {
    int best = -1;
    float mind = 1.0e9f;
    for (int i = 0; i < int(machs_.size()); i++) {
        if (!machs_[i].alive) continue;
        float d = std::fabs(angDiff(you_, machs_[i].ang));
        if (d < mind) {
            mind = d;
            best = i;
        }
    }
    return best;
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    modeT_ = 0;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    fanStep_ = 0;
    fanT_ = 0;
    sys_->apu.noiseBurst(0.12f, 1400.f, 0.06f);
    sys_->setLight(40, 160, 80);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    modeT_ = 0;
    sys_->apu.noiseBurst(0.45f, 140.f, 0.32f);
    sys_->setLight(170, 20, 30);
    shake_ = 0.45f;
}

void Game::stall(Mach& m) {
    if (!m.alive) return;
    m.alive = false;
    stalled_++;
    score_ += m.points;
    sys_->apu.noiseBurst(0.16f, 700.f, 0.05f);
    sys_->apu.tone(0, 420.f, 0.05f);
    if (others() == 0) winWatch();
}

void Game::strike() {
    if (strikeCd_ > 0) return;
    strikeCd_ = 0.32f;
    flash_ = 5;
    sys_->apu.noiseBurst(0.18f, 1100.f, 0.04f);
    int best = nearest();
    if (best < 0) return;
    Mach& m = machs_[best];
    if (std::fabs(angDiff(you_, m.ang)) > STRIKE) return;
    m.hp -= 1;
    m.flash = 0.12f;
    float away = angDiff(you_, m.ang);
    if (std::fabs(away) < 0.04f) away = 0.2f;
    m.ang = wrap(m.ang + (away > 0 ? 0.38f : -0.38f));
    m.cool = 0.45f;
    if (m.hp <= 0) stall(m);
    else sys_->apu.tone(2, 180.f, 0.06f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    reason_ = "THE WATCH IS OVER";
    score_ = 0;
    stalled_ = 0;
    grit_ = 3;
    spawnAt_ = 0;
    fanStep_ = -1;
    flash_ = 0;
    you_ = 0.15f;
    watch_ = 0;
    modeT_ = 0;
    strikeCd_ = 0;
    shake_ = 0;
    machs_.clear();
    script_.clear();
    fleet_ = int(sizeof kArr / sizeof kArr[0]);
    for (const Arr& a : kArr) {
        Spawn s;
        s.t = a.t;
        s.kind = a.kind;
        s.ang = a.ang;
        script_.push_back(s);
    }
    if (sys_) sys_->setLight(20, 40, 50);
}

void Game::update(float dt) {
    while (spawnAt_ < int(script_.size()) && script_[spawnAt_].t <= watch_) {
        const Spawn& s = script_[spawnAt_++];
        Mach m;
        m.kind = s.kind;
        m.ang = s.ang;
        m.speed = speedOf(s.kind);
        m.hp = hpOf(s.kind);
        m.points = ptsOf(s.kind);
        m.cool = 0.4f;
        m.alive = true;
        machs_.push_back(m);
        sys_->apu.tone(1, 220.f + s.kind * 40.f, 0.04f);
    }

    bool wantStrike = false;
    float turn = 0;
    if (bot_) {
        int n = nearest();
        if (n >= 0) {
            float d = angDiff(you_, machs_[n].ang);
            if (std::fabs(d) <= STRIKE && strikeCd_ <= 0) wantStrike = true;
            else turn = d > 0 ? 1.f : -1.f;
            if (std::fabs(d) < BITE && strikeCd_ > 0.05f) turn = d > 0 ? -1.f : 1.f;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) turn -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) turn += 1.f;
        if (std::fabs(pad.axisX) > 0.25f) turn += pad.axisX > 0 ? 1.f : -1.f;
        if (turn > 1.f) turn = 1.f;
        if (turn < -1.f) turn = -1.f;
        wantStrike = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) || pad.accel > 0.55f;
    }

    you_ = wrap(you_ + turn * WALK * dt);
    if (wantStrike) strike();

    for (Mach& m : machs_) {
        if (!m.alive) continue;
        if (m.cool > 0) m.cool -= dt;
        if (m.flash > 0) m.flash -= dt;
        float d = angDiff(m.ang, you_);
        float dir = d > 0 ? 1.f : -1.f;
        if (std::fabs(d) > 0.05f) m.ang = wrap(m.ang + dir * m.speed * dt);
        if (m.cool <= 0 && std::fabs(angDiff(you_, m.ang)) < BITE && flash_ <= 0) {
            m.cool = 0.7f;
            grit_--;
            shake_ = 0.28f;
            sys_->rumble(0.4f, 0.6f, 90);
            sys_->apu.tone(2, 90.f, 0.08f);
            float away = angDiff(m.ang, you_);
            you_ = wrap(you_ + (away >= 0 ? 0.22f : -0.22f));
            if (grit_ <= 0) {
                loseWatch("YOU STALLED");
                return;
            }
        }
    }

    watch_ += dt;
    if (strikeCd_ > 0) strikeCd_ -= dt;
    if (flash_ > 0) flash_--;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt);
    if (spawnAt_ >= int(script_.size()) && others() == 0 && !won_) winWatch();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + shx_));
    s.y = int16_t(std::lround(cy - s.h * 0.5f + shy_));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 32 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 16.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, 0);
    }
}

void Game::draw() {
    shx_ = shy_ = 0;
    if (shake_ > 0.05f) {
        shx_ = std::sin(watch_ * 42.f) * 3.f;
        shy_ = std::cos(watch_ * 31.f) * 1.4f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int k = y < 64 ? 1 : 2;
        v.lineBackdrop[y] = gs::rgb4(k, k + 1, k + 2);
        v.lineFog[y] = uint8_t(y < 28 ? 2 : 0);
        v.road[y].on = false;
    }

    spr(art_.basin, CX, CY + 6.f, 86.f, PAL_WATER, false, 0);
    spr(art_.ring, CX, CY, 118.f, PAL_STONE, false, 0);
    for (int i = 0; i < 4; i++) {
        float a = i * 1.57f + 0.4f;
        float x = CX + std::cos(a) * (RX + 16.f);
        float y = CY + std::sin(a) * (RY + 10.f);
        spr(art_.lamp, x, y, 10.f, PAL_FX, false, 0);
    }

    struct Spot {
        float y;
        int id;
    };
    std::vector<Spot> order;
    for (int i = 0; i < int(machs_.size()); i++) order.push_back({0, i});
    for (Spot& s : order) {
        float x, y;
        place(machs_[s.id].ang, x, y);
        s.y = y;
    }
    std::sort(order.begin(), order.end(), [](const Spot& a, const Spot& b) { return a.y < b.y; });

    auto body = [&](const Mach& m) -> const gs::Mipped& {
        int step = int(watch_ * (m.kind == SKIT ? 10.f : 6.f)) & 1;
        if (!m.alive) return art_.wreck;
        if (m.kind == HEAVY) return art_.heavy[step];
        if (m.kind == SKIT) return art_.skit[step];
        return art_.cart[step];
    };
    auto palOf = [&](const Mach& m) {
        if (!m.alive) return PAL_STONE;
        if (m.kind == HEAVY) return PAL_HEAVY;
        if (m.kind == SKIT) return PAL_SKIT;
        return PAL_RIVAL;
    };

    bool drewYou = false;
    auto drawYou = [&](float yLine) {
        if (drewYou) return;
        float x, y;
        place(you_, x, y);
        if (y > yLine && mode_ != Mode::Title) return;
        drewYou = true;
        if (mode_ == Mode::Over && grit_ <= 0) {
            spr(art_.wreck, x, y, 16.f, PAL_YOU, false, 0);
            return;
        }
        int step = int(watch_ * 8.f) & 1;
        float h = flash_ > 0 ? 22.f : 18.f;
        spr(art_.cart[step], x, y, h, PAL_YOU, std::cos(you_) < 0, 0);
        if (flash_ > 0) {
            float fx = x + std::cos(you_) * 14.f;
            float fy = y + std::sin(you_) * 8.f;
            spr(art_.spark, fx, fy, 12.f, PAL_FX, false, 0);
        }
    };

    if (mode_ == Mode::Title) {
        Mach demo;
        demo.kind = LIGHT;
        demo.ang = 0.8f + std::sin(modeT_) * 0.15f;
        demo.alive = true;
        float x, y;
        place(demo.ang, x, y);
        spr(body(demo), x, y, 16.f, PAL_RIVAL, true, 0);
        demo.kind = HEAVY;
        demo.ang = 3.6f;
        place(demo.ang, x, y);
        spr(body(demo), x, y, 20.f, PAL_HEAVY, false, 1);
        demo.kind = SKIT;
        demo.ang = 5.2f;
        place(demo.ang, x, y);
        spr(body(demo), x, y, 14.f, PAL_SKIT, true, 0);
        you_ = std::sin(modeT_ * 0.7f) * 0.4f;
        drawYou(1.0e9f);
    } else {
        for (const Spot& s : order) {
            drawYou(s.y);
            const Mach& m = machs_[s.id];
            float x, y;
            place(m.ang, x, y);
            float h = m.kind == HEAVY ? 20.f : m.kind == SKIT ? 13.f : 16.f;
            if (!m.alive) h = 14.f;
            int fog = m.alive ? 0 : 4;
            spr(body(m), x, y, h, palOf(m), std::cos(m.ang) < 0, fog);
            if (m.flash > 0) spr(art_.spark, x, y - 8.f, 10.f, PAL_FX, false, 0);
        }
        drawYou(1.0e9f);
    }

    if (mode_ == Mode::Title) text("S3 CISTERN PURSE", 160, 18, 0.62f, PAL_HUD);
    else if (mode_ == Mode::Victory) text("LAST MACHINE", 160, 16, 0.7f, PAL_OK);
    else if (mode_ == Mode::Over) text("WATCH OVER", 160, 16, 0.75f, PAL_ALERT);
    else if (others() == 1 && stalled_ > 0) text("ONE LEFT", 160, 16, 0.7f, PAL_FX);

    char line[64];
    if (mode_ == Mode::Title) {
        hudC(22, "AT THE CISTERN.", PAL_HUD);
        hudC(23, "BE THE LAST MACHINE STILL RUNNING.", PAL_OK);
        hudC(24, "LEFT RIGHT THE RIM", PAL_HUD);
        hudC(25, "A STALLS A MACHINE THAT IS CLOSE", PAL_HUD);
        hudC(26, "STALLING YOURSELF LOSES THE WATCH", PAL_ALERT);
        hudC(27, "ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        std::snprintf(line, sizeof line, "GRIT %d", grit_);
        hud(1, 0, line, grit_ <= 1 ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "STALLED %d/%d", stalled_, fleet_);
        hud(28, 0, line, PAL_HUD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_HUD);
        else if (mode_ == Mode::Victory) hudC(26, "THE LAST MACHINE STILL RUNNING", PAL_OK);
        else if (mode_ == Mode::Over) hudC(26, reason_, PAL_ALERT);
        else if (others() == 1 && stalled_ > 0) hudC(26, "FINISH THE LAST ONE", PAL_FX);
        else hudC(26, "STAY RUNNING", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    modeT_ = 0;
    if (bot_) beginWatch();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    modeT_ += DT;
    const gs::Pad& pad = sys.pad;
    bool start = !bot_ && pad.pressed(gs::BTN_START);
    bool back = !bot_ && pad.pressed(gs::BTN_MODE);
    if (back && mode_ == Mode::Title) {
        if (sys.hasHome()) sys.eject();
        else sys.quit();
    } else if (back && mode_ == Mode::Watch) mode_ = Mode::Pause;
    else if (back && mode_ == Mode::Pause) mode_ = Mode::Title;
    else if (start && (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Over)) beginWatch();
    else if (start && mode_ == Mode::Watch) mode_ = Mode::Pause;
    else if (start && mode_ == Mode::Pause) mode_ = Mode::Watch;

    if (mode_ == Mode::Watch) update(DT);
    else if (mode_ == Mode::Victory && fanStep_ >= 0) {
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            fanT_ = 0;
            static const float notes[] = {392.f, 494.f, 587.f, 784.f};
            if (fanStep_ < 4) sys.apu.tone(0, notes[fanStep_], 0.06f);
            fanStep_++;
            if (fanStep_ > 6) {
                fanStep_ = -1;
                sys.apu.tone(0, 0, 0);
            }
        }
    }
    draw();
}

}  // namespace purse
