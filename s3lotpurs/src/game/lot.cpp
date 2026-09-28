#include "game/lot.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace lotp {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LOT_L = 22.f;
constexpr float LOT_R = 298.f;
constexpr float LOT_T = 36.f;
constexpr float LOT_B = 208.f;
constexpr float HIT_R = 16.f;

const float kWx[4] = {58.f, 262.f, 262.f, 58.f};
const float kWy[4] = {58.f, 58.f, 186.f, 186.f};

float wrap(float a) {
    const float pi = 3.14159265f;
    while (a > pi) a -= pi * 2.f;
    while (a < -pi) a += pi * 2.f;
    return a;
}

float toward(float from, float to, float step) {
    float d = wrap(to - from);
    if (d > step) d = step;
    if (d < -step) d = -step;
    return from + d;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.25f, 0.12f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
}

void Game::beginLot() {
    machs_.clear();
    sparks_.clear();
    score_ = 0;
    stalled_ = 0;
    fleet_ = 4;
    hull_ = 6;
    t_ = 0;
    modeT_ = 0;
    shake_ = 0;
    stuckT_ = 0;
    won_ = false;
    over_ = false;
    reason_ = "YOU STALLED";

    Mach you;
    you.you = true;
    you.kind = 0;
    you.hp = hull_;
    you.x = 160.f;
    you.y = 176.f;
    you.ang = 0.f;
    machs_.push_back(you);

    const int kinds[4] = {1, 2, 3, 4};
    const int pts[4] = {200, 260, 320, 240};
    const int hp[4] = {2, 2, 3, 2};
    for (int i = 0; i < 4; ++i) {
        Mach m;
        m.kind = kinds[i];
        m.hp = hp[i];
        m.points = pts[i];
        m.wp = i;
        m.x = kWx[i];
        m.y = kWy[i];
        m.ang = 1.5708f * float(i);
        machs_.push_back(m);
    }
    mode_ = Mode::Drive;
}

void Game::stall(Mach& m) {
    if (!m.live || m.you) return;
    m.live = false;
    m.spd = 0;
    stalled_++;
    score_ += m.points;
    sparks_.push_back({m.x, m.y, 0.45f});
    beep_ = 0.12f;
    if (others() == 0 && hull_ > 0) winLot();
}

void Game::winLot() {
    if (won_ || hull_ <= 0) return;
    won_ = true;
    reason_ = "LAST MACHINE";
    mode_ = Mode::Victory;
    modeT_ = 0;
}

void Game::loseLot() {
    if (mode_ == Mode::Over || mode_ == Mode::Victory) return;
    won_ = false;
    reason_ = "YOU STALLED";
    mode_ = Mode::Over;
    modeT_ = 0;
    hull_ = 0;
    if (!machs_.empty()) machs_[0].live = false;
}

void Game::collide(Mach& a, Mach& b) {
    if (!a.live || !b.live) return;
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float d2 = dx * dx + dy * dy;
    float reach = HIT_R * 2.f;
    if (d2 > reach * reach || d2 < 0.01f) return;
    float d = std::sqrt(d2);
    float nx = dx / d;
    float ny = dy / d;
    float push = (reach - d) * 0.5f;
    a.x -= nx * push;
    a.y -= ny * push;
    b.x += nx * push;
    b.y += ny * push;
    if (a.cd > 0.f || b.cd > 0.f) return;

    Mach* you = a.you ? &a : (b.you ? &b : nullptr);
    Mach* foe = a.you ? &b : (b.you ? &a : nullptr);
    if (!you || !foe) {
        if (a.spd + b.spd > 1.4f) {
            a.cd = b.cd = 0.25f;
            if (a.hp > 0 && --a.hp == 0) stall(a);
            if (b.live && b.hp > 0 && --b.hp == 0) stall(b);
        }
        return;
    }
    bool ram = ramming_ && you->spd > 0.45f;
    you->cd = 0.28f;
    foe->cd = 0.4f;
    shake_ = 0.18f;
    sparks_.push_back({(you->x + foe->x) * 0.5f, (you->y + foe->y) * 0.5f, 0.28f});
    if (ram) {
        foe->hp -= 1;
        foe->x += std::sin(you->ang) * 10.f;
        foe->y -= std::cos(you->ang) * 10.f;
        beep_ = 0.08f;
        if (foe->hp <= 0) stall(*foe);
    } else {
        hull_ -= 1;
        you->hp = hull_;
        you->spd *= 0.3f;
        if (hull_ <= 0) loseLot();
    }
}

void Game::update(float dt) {
    gs::Pad& pad = sys_->pad;
    Mach& you = machs_[0];
    bool gas = false;
    bool ram = false;
    float steer = 0.f;

    if (bot_) {
        int best = -1;
        float bestD = 1e9f;
        for (int i = 1; i < int(machs_.size()); ++i) {
            if (!machs_[i].live) continue;
            float dx = machs_[i].x - you.x;
            float dy = machs_[i].y - you.y;
            float d = dx * dx + dy * dy;
            if (d < bestD) {
                bestD = d;
                best = i;
            }
        }
        if (best > 0) {
            float dx = machs_[best].x - you.x;
            float dy = machs_[best].y - you.y;
            float aim = std::atan2(dx, -dy);
            float err = wrap(aim - you.ang);
            steer = err > 0.08f ? 1.f : (err < -0.08f ? -1.f : err * 8.f);
            gas = true;
            ram = true;
            float moved = std::fabs(you.x - prevX_) + std::fabs(you.y - prevY_);
            if (moved < 0.35f) stuckT_ += dt;
            else stuckT_ = 0.f;
            if (stuckT_ > 0.35f) {
                steer = 1.f;
                ram = true;
            }
        }
        prevX_ = you.x;
        prevY_ = you.y;
    } else if (you.live) {
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        steer += pad.axisX;
        gas = pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.accel > 0.2f;
        ram = pad.down(gs::BTN_C) || pad.down(gs::BTN_B);
        if (pad.down(gs::BTN_DOWN)) you.spd *= 0.9f;
    }
    ramming_ = ram;

    if (you.live) {
        float turn = (2.6f + std::max(0.f, 2.4f - you.spd)) * dt;
        you.ang = wrap(you.ang + std::clamp(steer, -1.f, 1.f) * turn);
        float want = 0.f;
        if (gas) want = ram ? 2.85f : 1.65f;
        you.spd += (want - you.spd) * std::min(1.f, dt * (ram ? 6.f : 3.f));
    } else {
        you.spd *= 0.92f;
    }

    for (int i = 1; i < int(machs_.size()); ++i) {
        Mach& m = machs_[i];
        if (!m.live) continue;
        float tx = kWx[m.wp];
        float ty = kWy[m.wp];
        float dx = you.x - m.x;
        float dy = you.y - m.y;
        float near = dx * dx + dy * dy;
        int chase = 0;
        float chaseD = 1e9f;
        for (int k = 1; k < int(machs_.size()); ++k) {
            if (!machs_[k].live) continue;
            float qx = machs_[k].x - you.x;
            float qy = machs_[k].y - you.y;
            float qd = qx * qx + qy * qy;
            if (qd < chaseD) {
                chaseD = qd;
                chase = k;
            }
        }
        if (i == chase && near < 110.f * 110.f && you.live) {
            tx = you.x;
            ty = you.y;
        } else {
            float wx = kWx[m.wp] - m.x;
            float wy = kWy[m.wp] - m.y;
            if (wx * wx + wy * wy < 18.f * 18.f) m.wp = (m.wp + 1) & 3;
        }
        float aim = std::atan2(tx - m.x, -(ty - m.y));
        m.ang = toward(m.ang, aim, 2.1f * dt);
        float want = (near < 78.f * 78.f) ? 1.25f : 1.05f;
        m.spd += (want - m.spd) * std::min(1.f, dt * 2.f);
    }

    for (Mach& m : machs_) {
        if (m.cd > 0.f) m.cd -= dt;
        if (!m.live && !m.you) continue;
        m.x += std::sin(m.ang) * m.spd;
        m.y -= std::cos(m.ang) * m.spd;
        if (m.x < LOT_L) {
            m.x = LOT_L;
            m.ang = wrap(-m.ang);
            m.spd *= 0.85f;
        }
        if (m.x > LOT_R) {
            m.x = LOT_R;
            m.ang = wrap(-m.ang);
            m.spd *= 0.85f;
        }
        if (m.y < LOT_T) {
            m.y = LOT_T;
            m.ang = wrap(3.14159265f - m.ang);
            m.spd *= 0.85f;
        }
        if (m.y > LOT_B) {
            m.y = LOT_B;
            m.ang = wrap(3.14159265f - m.ang);
            m.spd *= 0.85f;
        }
    }

    for (int i = 0; i < int(machs_.size()); ++i) {
        for (int j = i + 1; j < int(machs_.size()); ++j) collide(machs_[i], machs_[j]);
    }

    for (Spark& s : sparks_) s.t -= dt;
    sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& s) { return s.t <= 0.f; }),
                  sparks_.end());
    if (shake_ > 0.f) shake_ -= dt;
    if (beep_ > 0.f) beep_ -= dt;
}

void Game::audio() {
    float spd = machs_.empty() ? 0.f : machs_[0].spd;
    if (mode_ == Mode::Drive && spd > 0.2f) sys_->apu.tone(0, 48.f + spd * 36.f, 0.04f + spd * 0.015f);
    else sys_->apu.tone(0, 0, 0);
    if (beep_ > 0.f) sys_->apu.tone(1, 220.f, 0.08f);
    else sys_->apu.tone(1, 0, 0);
    if (mode_ == Mode::Victory) sys_->apu.tone(2, 392.f, 0.05f);
    else if (mode_ == Mode::Over) sys_->apu.noise(0.04f, 900.f, false);
    else sys_->apu.tone(2, 0, 0);
}

const gs::Mipped& Game::bodyOf(const Mach& m) const {
    if (!m.live && !m.you) return art_.wreck;
    if (m.kind == 2) return art_.van;
    if (m.kind == 3) return art_.truck;
    if (m.kind == 4) return art_.wagon;
    return art_.sedan;
}

int Game::palOf(const Mach& m) const {
    if (!m.live && !m.you) return PAL_WRECK;
    if (m.kind == 1) return PAL_RED;
    if (m.kind == 2) return PAL_VAN;
    if (m.kind == 3) return PAL_TRUCK;
    if (m.kind == 4) return PAL_WAGON;
    return PAL_YOU;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (!(h > 1.5f) || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c <= 32 || c >= 128) {
            x += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false);
        x += gw;
    }
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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float sh = shake_ > 0.f ? std::sin(t_ * 40.f) * 2.f : 0.f;
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        int g = 3 + ((y / 16) & 1);
        v.lineBackdrop[y] = gs::rgb4(g, g, g + 1);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(2, 2, 3));

    if (mode_ == Mode::Title) {
        text("ONE LOT", 160.f, 70.f, 1.05f, PAL_YOU);
        text("BE THE LAST MACHINE", 160.f, 98.f, 0.58f, PAL_HUD);
        text("STILL RUNNING", 160.f, 118.f, 0.58f, PAL_HUD);
        text("ARROWS DRIVE   C RAM", 160.f, 156.f, 0.4f, PAL_LOT);
        text("ENTER STARTS", 160.f, 176.f, 0.42f, PAL_YOU);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 112.f, 0.9f, PAL_YOU);
    } else if (mode_ == Mode::Drive && others() == 1) {
        text("ONE LEFT", 160.f, 28.f, 0.5f, PAL_YOU);
    } else if (mode_ == Mode::Victory) {
        text("LAST MACHINE", 160.f, 96.f, 0.85f, PAL_YOU);
        text("THE LOT IS YOURS", 160.f, 122.f, 0.5f, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        text("STALLED", 160.f, 96.f, 1.0f, PAL_RED);
        text("NOT THE LAST", 160.f, 124.f, 0.55f, PAL_HUD);
    } else {
        text("LAST MACHINE", 160.f + sh, 18.f, 0.55f, PAL_HUD);
    }

    for (int i = 0; i < 6; ++i) {
        float x = 48.f + float(i) * 44.f;
        spr(art_.stripe, x, 100.f, 70.f, PAL_LOT, false);
        spr(art_.stripe, x, 168.f, 40.f, PAL_LOT, false);
    }
    const float lamps[4][2] = {{28, 42}, {292, 42}, {28, 200}, {292, 200}};
    for (auto& L : lamps) spr(art_.lamp, L[0], L[1], 36.f, PAL_LOT, false);

    auto paint = [&](const Mach& m) {
        float h = (!m.live && !m.you) ? 18.f : (m.kind == 3 ? 34.f : m.kind == 2 ? 30.f : 28.f);
        bool flip = std::sin(m.ang) < -0.15f;
        spr(art_.shadow, m.x + 3.f, m.y + 8.f, 8.f, 0, false);
        gs::Sprite s;
        const gs::Mipped& body = bodyOf(m);
        float w = h * float(body.w) / float(body.h);
        s.w = int16_t(std::lround(w));
        s.h = int16_t(std::lround(h));
        s.x = int16_t(std::lround(m.x - s.w * 0.5f));
        s.y = int16_t(std::lround(m.y - s.h * 0.5f));
        s.img = body.pick(h);
        s.pal = uint8_t(palOf(m));
        s.hflip = flip;
        s.shadow = false;
        v.sprite(s);
    };
    for (int i = int(machs_.size()) - 1; i >= 0; --i) paint(machs_[i]);
    for (const Spark& s : sparks_) spr(art_.spark, s.x, s.y, 10.f + s.t * 20.f, PAL_FX, false);

    if (mode_ == Mode::Drive || mode_ == Mode::Pause) {
        hud(1, 26, "HULL " + std::to_string(std::max(0, hull_)), PAL_HUD);
        hud(28, 26, "OUT " + std::to_string(stalled_) + "/4", PAL_HUD);
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Drive && others() == 1) return 2;
    return 1;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    modeT_ += DT;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
        if (bot_ && modeT_ > 0.4f) go = true;
        if (go) beginLot();
    } else if (mode_ == Mode::Drive) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            modeT_ = 0;
        } else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Drive;
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (modeT_ > 1.2f) over_ = true;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            mode_ = Mode::Title;
            modeT_ = 0;
            machs_.clear();
            over_ = false;
        }
    }

    if (mode_ == Mode::Title && machs_.empty() && std::fmod(t_, 2.f) < 0.02f) {
        // parked picture on the title, rebuilt lightly so the lot is visible at boot
    }
    if (mode_ == Mode::Title && machs_.empty()) {
        beginLot();
        mode_ = Mode::Title;
        modeT_ = t_;
        for (Mach& m : machs_) m.spd = 0;
    }

    audio();
    draw();
    if (won_) sys.setLight(40, 160, 50);
    else if (mode_ == Mode::Over) sys.setLight(140, 20, 20);
    else sys.setLight(40, 40, 30);
}

}  // namespace lotp
