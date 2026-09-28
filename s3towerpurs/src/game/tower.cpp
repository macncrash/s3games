#include "game/tower.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tpurs {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float TAU = 6.2831853f;
constexpr float RAM_ARC = 0.34f;
constexpr float HURT_ARC = 0.13f;
constexpr float STEER = 2.35f;
constexpr float WATCH_LIMIT = 70.f;

float wrap(float a) {
    while (a > 3.1415926f) a -= TAU;
    while (a < -3.1415926f) a += TAU;
    return a;
}

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int x, int y) { return int(x + (y - x) * t + 0.5f); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

}  // namespace

const gs::Mipped& Game::bodyOf(int kind) const {
    if (kind == 1) return art_.hauler;
    if (kind == 2) return art_.scout;
    return art_.crawler;
}

int Game::palOf(int kind) const {
    if (kind == 1) return PAL_TEAL;
    if (kind == 2) return PAL_LAMP;
    return PAL_RED;
}

const char* Game::whoOf(int kind) const {
    if (kind == 1) return "HAULER";
    if (kind == 2) return "SCOUT";
    return "CRAWLER";
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    beep_ = 0.08f;
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    reason_ = "THE WATCH IS OVER";
    score_ = 0;
    stalled_ = 0;
    grit_ = 3;
    next_ = 0;
    fanStep_ = -1;
    bearing_ = 0;
    steer_ = 0;
    ram_ = 0;
    ramCd_ = 0;
    gap_ = 0.15f;
    watch_ = 0;
    shake_ = 0;
    engineOn_ = true;
    machs_.clear();
    machs_.resize(size_t(fleet_));
    static const int kinds[] = {0, 1, 2, 0, 1};
    static const float speeds[] = {0.72f, 0.58f, 0.86f, 0.64f, 0.78f};
    static const int points[] = {100, 150, 120, 100, 180};
    for (int i = 0; i < fleet_; ++i) {
        Mach& m = machs_[size_t(i)];
        m.kind = kinds[i];
        m.hp = 2;
        m.points = points[i];
        m.speed = speeds[i];
        m.ang = 2.15f;
        m.live = true;
        m.out = false;
        m.flash = 0;
    }
    sys_->setLight(90, 70, 30);
}

void Game::stall(Mach& m) {
    m.hp -= 1;
    m.flash = 0.2f;
    m.ang = wrap(bearing_ + 0.95f);
    ram_ = 0.16f;
    shake_ = std::max(shake_, 0.14f);
    sys_->apu.noiseBurst(0.2f, 900.f, 0.06f);
    sys_->rumble(0.25f, 0.4f, 50);
    if (m.hp > 0) {
        blip(220.f, 0.05f);
        return;
    }
    m.live = false;
    ++stalled_;
    score_ += m.points;
    static const float notes[] = {196.f, 247.f, 330.f, 392.f, 494.f};
    int n = std::clamp(stalled_ - 1, 0, 4);
    sys_->apu.keyOn(1, notes[n], 0.16f);
    if (stalled_ >= fleet_) winWatch();
    else gap_ = 0.45f;
}

void Game::winWatch() {
    if (won_ || mode_ == Mode::Over) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    score_ += 400;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->setLight(40, 140, 60);
}

void Game::loseWatch(const char* why) {
    if (won_ || mode_ == Mode::Over || mode_ == Mode::Victory) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Over;
    engineOn_ = false;
    shake_ = 0.5f;
    sys_->apu.noiseBurst(0.4f, 90.f, 0.25f);
    sys_->setLight(150, 30, 20);
}

void Game::steerBot() {
    float best = 99.f;
    bool any = false;
    for (const Mach& m : machs_) {
        if (!m.live || !m.out) continue;
        float d = wrap(m.ang - bearing_);
        if (!any || std::fabs(d) < std::fabs(best)) {
            best = d;
            any = true;
        }
    }
    if (!any) {
        steer_ = 0;
        return;
    }
    steer_ = std::clamp(best * 3.f, -1.f, 1.f);
    if (std::fabs(best) < RAM_ARC * 0.85f) tryRam();
}

void Game::tryRam() {
    if (ramCd_ > 0.f || mode_ != Mode::Watch || won_) return;
    int hit = -1;
    float best = 99.f;
    for (int i = 0; i < int(machs_.size()); ++i) {
        const Mach& m = machs_[size_t(i)];
        if (!m.live || !m.out) continue;
        float d = std::fabs(wrap(m.ang - bearing_));
        if (d < RAM_ARC && d < best) {
            best = d;
            hit = i;
        }
    }
    ramCd_ = 0.28f;
    ram_ = 0.12f;
    if (hit < 0) {
        grit_ -= 1;
        shake_ = std::max(shake_, 0.22f);
        blip(80.f, 0.05f);
        if (grit_ <= 0) loseWatch("YOU STALLED YOURSELF");
        return;
    }
    stall(machs_[size_t(hit)]);
}

void Game::update(float dt) {
    watch_ += dt;
    if (ramCd_ > 0.f) ramCd_ -= dt;
    if (ram_ > 0.f) ram_ -= dt;
    if (gap_ > 0.f) gap_ -= dt;

    if (next_ < fleet_ && gap_ <= 0.f) {
        bool busy = false;
        for (const Mach& m : machs_)
            if (m.live && m.out) busy = true;
        if (!busy) {
            Mach& m = machs_[size_t(next_++)];
            m.out = true;
            m.ang = wrap(bearing_ + 2.2f);
            m.hp = 2;
        }
    }

    bearing_ = wrap(bearing_ + steer_ * STEER * dt);

    for (Mach& m : machs_) {
        if (!m.live || !m.out) continue;
        if (m.flash > 0.f) m.flash -= dt;
        float d = wrap(bearing_ - m.ang);
        float step = std::clamp(d, -m.speed * dt, m.speed * dt);
        m.ang = wrap(m.ang + step);
        float gap = std::fabs(wrap(m.ang - bearing_));
        if (gap < HURT_ARC && ram_ <= 0.f) {
            m.ang = wrap(bearing_ + (d > 0 ? -0.4f : 0.4f));
            grit_ -= 1;
            shake_ = std::max(shake_, 0.3f);
            blip(70.f, 0.06f);
            sys_->apu.noiseBurst(0.18f, 200.f, 0.08f);
            if (grit_ <= 0) {
                loseWatch("YOUR MACHINE SEIZED");
                return;
            }
        }
    }

    if (watch_ > WATCH_LIMIT && !won_) loseWatch("THE WATCH RAN OUT");
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 2.f) return;
    const gs::Image& img = m.pick(h);
    if (img.h <= 0) return;
    gs::Sprite s;
    s.img = img;
    s.h = int(h);
    s.w = int(h * float(img.w) / float(img.h));
    s.x = int(cx - s.w * 0.5f);
    s.y = int(feet ? cy - s.h : cy - s.h * 0.5f);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Watch && others() == 1 && next_ >= fleet_) return 2;
    if (mode_ == Mode::Watch) return 1;
    return 0;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0, shy = 0;
    if (shake_ > 0.f) {
        shake_ = std::max(0.f, shake_ - DT);
        shx = std::sin(t_ * 47.f) * shake_ * 10.f;
        shy = std::cos(t_ * 39.f) * shake_ * 6.f;
    }
    uint16_t skyTop = mode_ == Mode::Over ? gs::rgb4(4, 1, 1) : gs::rgb4(1, 1, 3);
    uint16_t skyHor = mode_ == Mode::Victory ? gs::rgb4(8, 6, 3) : gs::rgb4(5, 4, 4);
    const int horizon = 86;
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < horizon) {
            v.lineBackdrop[y] = mix(skyTop, skyHor, float(y) / float(horizon));
            v.lineFog[y] = 0;
            rd.on = false;
            continue;
        }
        float row = float(y - horizon) + 1.f;
        rd.on = true;
        rd.cx = 160.f + shx * 0.3f;
        rd.hw = 18.f + row * 1.15f;
        rd.v = 640.f / row + t_ * 6.f;
        rd.pal = uint8_t(PAL_YARD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(rd.v / 36.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        v.lineBackdrop[y] = gs::rgb4(2, 3, 2);
        v.lineFog[y] = uint8_t(std::clamp(int(10.f - row * 0.08f), 0, 8));
    }
    v.setFogColor(mode_ == Mode::Over ? gs::rgb4(5, 2, 2) : gs::rgb4(3, 3, 2));

    float tx = 160.f + shx;
    float ty = 118.f + shy;
    spr(art_.shadow, tx, ty + 8.f, 28.f, PAL_FX, false, 4);
    spr(art_.tower, tx, ty, mode_ == Mode::Victory ? 108.f : 96.f, PAL_STONE, false, 2, true);
    spr(art_.lamp, tx - 22.f, ty + 6.f, 22.f, PAL_LAMP, false, 1, true);
    spr(art_.lamp, tx + 22.f, ty + 10.f, 20.f, PAL_LAMP, true, 1, true);

    for (const Mach& m : machs_) {
        if (!m.out) continue;
        float rel = wrap(m.ang - bearing_);
        float depth = std::cos(rel);
        float sx = tx + std::sin(rel) * 92.f;
        float sy = ty + 18.f + depth * 28.f;
        float h = m.live ? (22.f + (depth + 1.f) * 10.f) : 14.f;
        int fog = m.live ? int((1.f - depth) * 4.f) : 8;
        int pal = m.live ? palOf(m.kind) : PAL_STONE;
        if (m.flash > 0.f) pal = PAL_SPARK;
        spr(art_.shadow, sx, sy + 4.f, h * 0.45f, PAL_FX, false, fog + 2);
        spr(bodyOf(m.kind), sx, sy, h, pal, rel < 0.f, fog, true);
        if (!m.live) spr(art_.puff, sx, sy - 8.f, 12.f, PAL_FX, false, fog);
    }

    float bob = std::sin(t_ * 8.f) * (engineOn_ ? 1.2f : 0.f);
    float lift = ram_ > 0.f ? 10.f : 0.f;
    float feet = 200.f + shy + bob - lift;
    spr(art_.shadow, 160.f + shx, feet - 2.f, 50.f, PAL_FX, false);
    spr(art_.cab, 160.f + shx + steer_ * 6.f, feet, mode_ == Mode::Over ? 52.f : 64.f,
        mode_ == Mode::Over ? PAL_STONE : PAL_YOU, steer_ < 0.f, 0, true);
    if (engineOn_ && (int(t_ * 8.f) & 1) == 0) spr(art_.puff, 148.f + shx, feet - 28.f, 10.f, PAL_FX, false);
    if (ram_ > 0.f) spr(art_.spark, 160.f + shx, feet - 40.f, 18.f, PAL_SPARK, false);

    char buf[80];
    if (mode_ == Mode::Title) {
        hudC(4, "S3 TOWER PURSE", PAL_LAMP);
        hudC(16, "BE THE LAST MACHINE STILL RUNNING", PAL_YOU);
        hudC(18, "STALL EVERY OTHER MACHINE", PAL_HUD);
        hudC(21, "ARROWS SWING THE RING", PAL_HUD);
        hudC(22, "A OR UP RAMS", PAL_LAMP);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "PRESS START", PAL_LAMP);
        std::string ver = S3_VERSION_STRING;
        hud(39 - int(ver.size()), 1, ver, PAL_HUD);
    } else if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "OTHERS %d", others());
        hud(1, 1, buf, others() <= 1 ? PAL_LAMP : PAL_HUD);
        std::snprintf(buf, sizeof buf, "GRIT %d", grit_);
        hud(39 - int(std::strlen(buf)), 1, buf, grit_ > 1 ? PAL_YOU : PAL_RED);
        hud(1, 2, engineOn_ ? "YOU RUNNING" : "YOU SEIZED", PAL_YOU);
        const Mach* near = nullptr;
        float nd = 99.f;
        for (const Mach& m : machs_) {
            if (!m.live || !m.out) continue;
            float d = std::fabs(wrap(m.ang - bearing_));
            if (!near || d < nd) {
                near = &m;
                nd = d;
            }
        }
        if (near) {
            std::string line = std::string(whoOf(near->kind));
            hudC(21, line, palOf(near->kind));
            if (nd < RAM_ARC) hudC(23, "RAM NOW", PAL_LAMP);
            else hudC(23, "SWING ONTO IT", PAL_HUD);
        } else if (next_ < fleet_) {
            hudC(21, "THE NEXT MACHINE IS OUT", PAL_HUD);
        }
        std::snprintf(buf, sizeof buf, "STALLED %d", stalled_);
        hud(1, 26, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 26, buf, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(25, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(16, "THE LAST MACHINE STILL RUNNING", PAL_LAMP);
        hudC(18, "THE TOWER WATCH HOLDS", PAL_YOU);
        std::snprintf(buf, sizeof buf, "STALLED %d   SCORE %d", stalled_, score_);
        hudC(20, buf, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(16, "THE WATCH IS OVER", PAL_RED);
        hudC(18, reason_, PAL_HUD);
        std::snprintf(buf, sizeof buf, "STALLED %d   SCORE %d", stalled_, score_);
        hudC(20, buf, PAL_HUD);
        hudC(24, "START TRIES AGAIN", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    if (bot_) beginWatch();
    else {
        mode_ = Mode::Title;
        engineOn_ = true;
        sys.setLight(90, 70, 40);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        bearing_ = wrap(bearing_ + 0.25f * DT);
        if (!bot_ && pad.pressed(gs::BTN_START)) beginWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Watch) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        } else {
            steer_ = 0;
            if (bot_) steerBot();
            else {
                if (pad.down(gs::BTN_LEFT)) steer_ -= 1.f;
                if (pad.down(gs::BTN_RIGHT)) steer_ += 1.f;
                float ax = pad.axisX;
                if (std::fabs(ax) > 0.2f) steer_ = std::clamp(ax, -1.f, 1.f);
                if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_C)) tryRam();
            }
            update(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Watch;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        if (mode_ == Mode::Over) beginWatch();
        else {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }

    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(0, 0, 0);
    }
    if (engineOn_ && mode_ != Mode::Title) {
        sys.apu.tone(1, 48.f + (ram_ > 0.f ? 18.f : 0.f), mode_ == Mode::Watch ? 0.04f : 0.02f);
    }
    if (fanStep_ >= 0) {
        static const float notes[] = {262.f, 330.f, 392.f, 523.f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            if (fanStep_ < 4) sys.apu.keyOn(2, notes[fanStep_], 0.14f);
            ++fanStep_;
            fanT_ = 0;
            if (fanStep_ > 8) fanStep_ = -1;
        }
    }

    draw();
}

}  // namespace tpurs
