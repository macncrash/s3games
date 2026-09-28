#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace millp {
namespace {

constexpr float kDt = 1.f / 60.f;

struct Spawn {
    int frame;
    int lane;
    int kind;
};

constexpr Spawn kSpawns[] = {
    {36, 1, 0}, {150, 0, 0}, {270, 2, 0}, {400, 1, 1}, {560, 0, 0}, {700, 2, 1},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Title) return 0;
    if (seize_ > 0) return 2;
    return 1;
}

float Game::laneY(int lane) const {
    static const float y[3] = {96.f, 136.f, 176.f};
    if (lane < 0) lane = 0;
    if (lane > 2) lane = 2;
    return y[lane];
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
            m.vx = float((i * 13) % 7 - 3);
            m.vy = float((i * 5) % 5 - 4);
            m.life = 0.32f + (i % 3) * 0.08f;
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
        m.vy += 0.05f;
    }
}

void Game::bootYard() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& rd = sys_->vdp.road[y];
        rd.on = false;
        sys_->vdp.lineFog[y] = 0;
        if (y < 58) {
            int u = y / 12;
            sys_->vdp.lineBackdrop[y] = gs::rgb4(4 + u / 3, 6 + u / 4, 11 - u / 5);
        } else {
            int g = 3 + ((y / 8) & 1);
            bool band = false;
            for (int lane = 0; lane < 3; lane++) {
                float cy = laneY(lane);
                if (std::fabs(float(y) - cy) < 14.f) band = true;
            }
            sys_->vdp.lineBackdrop[y] = band ? gs::rgb4(6, 5, 2) : gs::rgb4(3, g + 2, 1);
        }
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
    sys_->apu.setMaster(0.35f);
    sys_->apu.setEcho(0.06f, 0.12f, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootYard();
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    millHp_ = 9;
    reason_ = "THE MILL IS STILL";
}

void Game::begin() {
    mode_ = Mode::Play;
    playF_ = 0;
    lane_ = 1;
    millHp_ = 9;
    stalled_ = 0;
    spawnIx_ = 0;
    cool_ = 0;
    seize_ = 0;
    hold_ = 0;
    won_ = false;
    over_ = false;
    reason_ = "";
    rivals_.clear();
    bolts_.clear();
    for (auto& m : motes_) m.life = 0;
}

void Game::stopRival(Rival& r) {
    if (!r.running) return;
    r.running = false;
    r.hp = 0;
    stalled_++;
    seize_ = 28;
    puff(r.x, laneY(r.lane), 6);
    sys_->apu.noiseBurst(0.32f, 1200.f, 0.08f);
    bool any = false;
    for (const auto& o : rivals_)
        if (o.running) any = true;
    if (spawnIx_ >= kFleet && !any) winMill();
}

void Game::hurtMill() {
    if (mode_ != Mode::Play) return;
    millHp_--;
    puff(58.f, laneY(lane_), 4);
    blip(1, 110.f, 0.16f);
    if (millHp_ <= 0) {
        millHp_ = 0;
        loseMill("THE MILL STOPPED");
    }
}

void Game::winMill() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    hold_ = 1.4f;
    blip(0, 523.f, 0.16f);
}

void Game::loseMill(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.4f;
    sys_->apu.noiseBurst(0.4f, 380.f, 0.18f);
}

void Game::firePlayer() {
    if (cool_ > 0 || mode_ != Mode::Play) return;
    Bolt b;
    b.lane = lane_;
    b.x = 78.f;
    b.vx = 6.4f;
    b.player = true;
    b.live = true;
    bolts_.push_back(b);
    cool_ = 9;
    blip(0, 540.f, 0.1f);
}

void Game::fireRival(Rival& r) {
    Bolt b;
    b.lane = r.lane;
    b.x = r.x - 18.f;
    b.vx = r.kind ? -2.4f : -2.9f;
    b.player = false;
    b.live = true;
    bolts_.push_back(b);
    r.cool = r.kind ? 1.45f : 1.05f;
}

void Game::spawnDue() {
    while (spawnIx_ < kFleet && playF_ >= kSpawns[spawnIx_].frame) {
        Rival r;
        r.lane = kSpawns[spawnIx_].lane;
        r.kind = kSpawns[spawnIx_].kind;
        r.hp = r.kind ? 2 : 1;
        r.x = 332.f;
        r.cool = 0.55f + r.kind * 0.25f;
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
    lane_ = rivals_[best].lane;
    if (cool_ == 0 && near < 318.f) firePlayer();
}

void Game::update() {
    playF_++;
    sail_++;
    if (seize_ > 0) seize_--;
    if (cool_ > 0) cool_--;
    spawnDue();
    if (bot_) botAct();
    else {
        const gs::Pad& pad = sys_->pad;
        if (pad.pressed(gs::BTN_UP)) lane_ = std::max(0, lane_ - 1);
        if (pad.pressed(gs::BTN_DOWN)) lane_ = std::min(2, lane_ + 1);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_C)) firePlayer();
    }
    for (auto& r : rivals_) {
        if (!r.running) continue;
        r.x -= r.kind ? 0.52f : 0.78f;
        r.cool -= kDt;
        if (r.cool <= 0.f && r.x < 250.f && r.x > 110.f) fireRival(r);
        if (r.x <= 74.f) {
            loseMill("A MACHINE GOT THROUGH");
            return;
        }
    }
    for (auto& b : bolts_) {
        if (!b.live) continue;
        b.x += b.vx;
        if (b.x < -20.f || b.x > 340.f) {
            b.live = false;
            continue;
        }
        if (b.player) {
            for (auto& r : rivals_) {
                if (!r.running || r.lane != b.lane) continue;
                if (std::fabs(b.x - r.x) < 16.f) {
                    b.live = false;
                    r.hp--;
                    puff(r.x, laneY(r.lane), 3);
                    if (r.hp <= 0) stopRival(r);
                    break;
                }
            }
        } else if (b.lane == lane_ && b.x < 72.f && b.x > 48.f) {
            b.live = false;
            hurtMill();
        }
    }
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return !b.live; }), bolts_.end());
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
        return;
    }
    if (mode_ == Mode::Play && millHp_ > 0) sys_->apu.tone(2, 64.f + millHp_ * 6.f, 0.028f);
    else if (mode_ == Mode::Title) sys_->apu.tone(2, 90.f, 0.012f);
    else sys_->apu.tone(2, 0.f, 0.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        sail_++;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || (bot_ && age_ > 24))
            begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else {
        if (won_) sail_++;
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
    bootYard();

    for (int i = 0; i < 9; i++) {
        float x = 20.f + float((i * 37) % 300);
        float y = 70.f + float((i * 19) % 28);
        spr(art_.stalk, x, y, 16.f, PAL_WHEAT, i & 1, 2);
    }

    for (const auto& m : motes_) {
        if (m.life <= 0) continue;
        spr(art_.dust, m.x, m.y, 8.f + m.life * 8.f, PAL_DUST, false, 0);
    }

    for (const auto& r : rivals_) {
        int fog = r.running ? 0 : 9;
        int pal = r.kind ? PAL_TRACTOR : PAL_THRESH;
        float h = r.kind ? 30.f : 26.f;
        spr(r.kind ? art_.tractor : art_.thresher, r.x, laneY(r.lane), h, pal, true, fog);
    }
    for (const auto& b : bolts_) {
        if (!b.live) continue;
        spr(b.player ? art_.stone : art_.dust, b.x, laneY(b.lane), b.player ? 10.f : 8.f,
            b.player ? PAL_STONE : PAL_IRON, false, 0);
    }

    float my = laneY(lane_);
    bool turning = millHp_ > 0 && mode_ != Mode::Fail;
    spr(art_.mill, 46.f, 118.f, 86.f, PAL_MILL, false, mode_ == Mode::Fail ? 6 : 0);
    spr((sail_ / 7) & 1 ? art_.sailA : art_.sailB, 46.f, 78.f, turning ? 52.f : 48.f, PAL_SAIL, false,
        turning ? 0 : 8);
    spr(art_.stone, 70.f, my, 8.f, PAL_GOOD, false, 0);

    if (mode_ == Mode::Title) {
        text("MILL PURSUIT", 188.f, 28.f, 0.72f, PAL_WHEAT);
        text("LAST MACHINE RUNNING", 188.f, 52.f, 0.38f, PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        text("STILL RUNNING", 190.f, 28.f, 0.7f, PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        text(reason_, 190.f, 26.f, std::char_traits<char>::length(reason_) > 18 ? 0.4f : 0.58f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 190.f, 28.f, 0.8f, PAL_WHEAT);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char line[40];
        std::snprintf(line, sizeof line, "STALLED %d/%d", stalled_, kFleet);
        hud(1, 1, line, PAL_TEXT);
        std::snprintf(line, sizeof line, "MILL %d", millHp_);
        hud(30, 1, line, millHp_ > 3 ? PAL_GOOD : PAL_ALERT);
        hudC(26, "UP DOWN LANE   A STONE", PAL_TEXT);
    } else if (mode_ == Mode::Title) {
        hudC(24, "START", PAL_WHEAT);
        hudC(26, "BE THE LAST MACHINE STILL RUNNING", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(25, "THE LAST MACHINE STILL RUNNING", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(25, reason_, PAL_ALERT);
    }
}

}  // namespace millp
