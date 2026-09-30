#include "game/foundry.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace foundryp {
namespace {

constexpr float kDt = 1.f / 60.f;

struct Due {
    int frame;
    int rail;
    int kind;
};

constexpr Due kDue[] = {
    {30, 1, 0}, {140, 0, 0}, {250, 2, 0}, {380, 1, 1}, {530, 0, 1}, {680, 2, 0},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Title) return 0;
    if (seize_ > 0) return 2;
    return 1;
}

float Game::railY(int rail) const {
    static const float y[3] = {100.f, 142.f, 184.f};
    if (rail < 0) rail = 0;
    if (rail > 2) rail = 2;
    return y[rail];
}

void Game::blip(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    blip_ = 0.08f;
}

void Game::puff(float x, float y, int n) {
    for (int i = 0; i < n; i++) {
        for (auto& m : motes_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y;
            m.vx = float((i * 11) % 7 - 3);
            m.vy = float((i * 3) % 5 - 4);
            m.life = 0.28f + (i % 3) * 0.09f;
            break;
        }
    }
}

void Game::tickMotes() {
    for (auto& m : motes_) {
        if (m.life <= 0) continue;
        m.life -= kDt;
        m.x += m.vx;
        m.y += m.vy;
        m.vy -= 0.04f;
    }
}

void Game::bootFloor() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = sys_->vdp.road[y];
        rd.on = false;
        sys_->vdp.lineFog[y] = 0;
        if (y < 52) {
            int u = y / 10;
            sys_->vdp.lineBackdrop[y] = gs::rgb4(2 + u / 4, 1, 1 + (4 - u) / 6);
        } else {
            bool rail = false;
            for (int r = 0; r < 3; r++) {
                if (std::fabs(float(y) - railY(r)) < 10.f) rail = true;
            }
            int glow = (y > 160) ? 2 : 0;
            sys_->vdp.lineBackdrop[y] =
                rail ? gs::rgb4(8 + glow, 3, 1) : gs::rgb4(3 + glow, 2, 1 + ((y / 6) & 1));
        }
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
    sys_->apu.setMaster(0.35f);
    sys_->apu.setEcho(0.05f, 0.1f, 0.05f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootFloor();
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    heat_ = 8;
    reason_ = "THE FOUNDRY IS COLD";
}

void Game::begin() {
    mode_ = Mode::Play;
    playF_ = 0;
    rail_ = 1;
    heat_ = 8;
    stalled_ = 0;
    spawnIx_ = 0;
    cool_ = 0;
    seize_ = 0;
    hold_ = 0;
    won_ = false;
    over_ = false;
    reason_ = "";
    rivals_.clear();
    shots_.clear();
    for (auto& m : motes_) m.life = 0;
}

void Game::seize(Rival& r) {
    if (!r.running) return;
    r.running = false;
    r.hp = 0;
    stalled_++;
    seize_ = 30;
    puff(r.x, railY(r.rail) - 6.f, 7);
    sys_->apu.noiseBurst(0.3f, 900.f, 0.08f);
    bool any = false;
    for (const auto& o : rivals_)
        if (o.running) any = true;
    if (spawnIx_ >= kFleet && !any) winFloor();
}

void Game::chill() {
    if (mode_ != Mode::Play) return;
    heat_--;
    puff(62.f, railY(rail_), 4);
    blip(1, 90.f, 0.16f);
    if (heat_ <= 0) {
        heat_ = 0;
        loseFloor("THE FURNACE WENT COLD");
    }
}

void Game::winFloor() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    hold_ = 1.4f;
    blip(0, 494.f, 0.16f);
}

void Game::loseFloor(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.4f;
    sys_->apu.noiseBurst(0.42f, 240.f, 0.18f);
}

void Game::quench() {
    if (cool_ > 0 || mode_ != Mode::Play) return;
    Shot s;
    s.rail = rail_;
    s.x = 86.f;
    s.vx = 6.6f;
    s.ours = true;
    s.live = true;
    shots_.push_back(s);
    cool_ = 8;
    blip(0, 620.f, 0.09f);
}

void Game::rivalShot(Rival& r) {
    Shot s;
    s.rail = r.rail;
    s.x = r.x - 16.f;
    s.vx = r.kind ? -2.2f : -2.7f;
    s.ours = false;
    s.live = true;
    shots_.push_back(s);
    r.cool = r.kind ? 1.55f : 1.15f;
}

void Game::spawnDue() {
    while (spawnIx_ < kFleet && playF_ >= kDue[spawnIx_].frame) {
        Rival r;
        r.rail = kDue[spawnIx_].rail;
        r.kind = kDue[spawnIx_].kind;
        r.hp = r.kind ? 2 : 1;
        r.x = 336.f;
        r.cool = 0.6f + r.kind * 0.3f;
        r.running = true;
        rivals_.push_back(r);
        spawnIx_++;
    }
}

void Game::botAct() {
    int best = -1;
    float near = 1e9f;
    for (int i = 0; i < (int)rivals_.size(); i++) {
        if (!rivals_[i].running) continue;
        if (rivals_[i].x < near) {
            near = rivals_[i].x;
            best = i;
        }
    }
    if (best < 0) return;
    rail_ = rivals_[best].rail;
    if (cool_ == 0 && near < 320.f) quench();
}

void Game::update() {
    playF_++;
    flicker_++;
    if (seize_ > 0) seize_--;
    if (cool_ > 0) cool_--;
    spawnDue();
    if (bot_) botAct();
    else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_UP)) rail_ = std::max(0, rail_ - 1);
        if (pad.pressed(gs::BTN_DOWN)) rail_ = std::min(2, rail_ + 1);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) quench();
    }
    for (auto& r : rivals_) {
        if (!r.running) continue;
        r.x -= r.kind ? 0.46f : 0.72f;
        r.cool -= kDt;
        if (r.cool <= 0.f && r.x < 260.f && r.x > 120.f) rivalShot(r);
        if (r.x <= 84.f) {
            loseFloor("A MACHINE GOT THROUGH");
            return;
        }
    }
    for (auto& s : shots_) {
        if (!s.live) continue;
        s.x += s.vx;
        if (s.x < -16.f || s.x > 340.f) {
            s.live = false;
            continue;
        }
        if (s.ours) {
            for (auto& r : rivals_) {
                if (!r.running || r.rail != s.rail) continue;
                if (std::fabs(s.x - r.x) < 16.f) {
                    s.live = false;
                    r.hp--;
                    puff(r.x, railY(r.rail), 3);
                    if (r.hp <= 0) seize(r);
                    break;
                }
            }
        } else if (s.rail == rail_ && s.x < 78.f && s.x > 50.f) {
            s.live = false;
            chill();
        }
    }
    shots_.erase(std::remove_if(shots_.begin(), shots_.end(), [](const Shot& s) { return !s.live; }), shots_.end());
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
        return;
    }
    if (mode_ == Mode::Play && heat_ > 0) sys_->apu.tone(2, 48.f + heat_ * 5.f, 0.03f);
    else if (mode_ == Mode::Title) sys_->apu.tone(2, 72.f, 0.012f);
    else sys_->apu.tone(2, 0.f, 0.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        flicker_++;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || (bot_ && age_ > 24))
            begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else {
        if (won_) flicker_++;
        hold_ -= kDt;
        if (hold_ <= 0.f) {
            over_ = true;
            if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
                mode_ = Mode::Title;
                over_ = false;
                age_ = 0;
            }
        }
    }
    tickMotes();
    serviceAudio();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0);
        x += gw;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    bootFloor();

    for (int i = 0; i < 5; i++) {
        float x = 18.f + float(i) * 58.f;
        float bob = float((flicker_ + i * 5) % 12);
        spr(art_.stack, x, 36.f - bob * 0.4f, 18.f, PAL_SOOT, false, 3);
    }

    for (const auto& m : motes_) {
        if (m.life <= 0) continue;
        spr(art_.ember, m.x, m.y, 6.f + m.life * 10.f, PAL_EMBER, false, 0);
    }

    for (const auto& r : rivals_) {
        int fog = r.running ? 0 : 10;
        if (r.kind) spr(art_.crucible, r.x, railY(r.rail) - 4.f, 28.f, PAL_CRUC, true, fog);
        else spr(art_.slag, r.x, railY(r.rail), 22.f, PAL_SLAG, true, fog);
    }
    for (const auto& s : shots_) {
        if (!s.live) continue;
        spr(s.ours ? art_.spark : art_.ember, s.x, railY(s.rail), s.ours ? 9.f : 7.f,
            s.ours ? PAL_SPARK : PAL_HEAT, false, 0);
    }

    float my = railY(rail_);
    bool hot = heat_ > 0 && mode_ != Mode::Fail;
    spr(art_.furnace, 40.f, 128.f, 92.f, PAL_FURN, false, mode_ == Mode::Fail ? 7 : 0);
    float mouth = (flicker_ / 6) & 1 ? 14.f : 12.f;
    if (hot) spr(art_.spark, 58.f, 150.f, mouth, PAL_HEAT, false, 0);
    spr(art_.ladle, 78.f, my, 16.f, PAL_LADLE, false, hot ? 0 : 8);

    if (mode_ == Mode::Title) {
        text("FOUNDRY PURSUIT", 196.f, 28.f, 0.62f, PAL_HEAT);
        text("LAST MACHINE RUNNING", 196.f, 50.f, 0.36f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("STILL RUNNING", 196.f, 26.f, 0.68f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 196.f, 24.f, std::char_traits<char>::length(reason_) > 18 ? 0.38f : 0.52f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 196.f, 26.f, 0.8f, PAL_HEAT);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char line[40];
        std::snprintf(line, sizeof line, "SEIZED %d/%d", stalled_, kFleet);
        hud(1, 1, line, PAL_TEXT);
        std::snprintf(line, sizeof line, "HEAT %d", heat_);
        hud(31, 1, line, heat_ > 3 ? PAL_GOOD : PAL_ALERT);
        hudC(26, "UP DOWN RAIL   A QUENCH", PAL_TEXT);
    } else if (mode_ == Mode::Title) {
        hudC(24, "START", PAL_HEAT);
        hudC(26, "BE THE LAST MACHINE STILL RUNNING", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(25, "THE LAST MACHINE STILL RUNNING", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, reason_, PAL_ALERT);
    }
}

}  // namespace foundryp
