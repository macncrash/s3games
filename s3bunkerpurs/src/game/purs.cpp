#include "game/purs.h"

#include <algorithm>
#include <cstdio>

namespace bunkerpurs {
namespace {

struct Spawn {
    int frame;
    int bay;
    int kind;
};

constexpr Spawn kSpawns[] = {
    {24, 1, 0}, {130, 0, 0}, {230, 2, 0}, {360, 1, 1}, {520, 2, 0}, {640, 0, 1},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Title) return 0;
    if (seize_ > 0) return 2;
    return 1;
}

float Game::bayY(int bay) const {
    static const float y[3] = {78.f, 118.f, 162.f};
    if (bay < 0) bay = 0;
    if (bay > 2) bay = 2;
    return y[bay];
}

void Game::blip(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    blipT_ = 6;
}

void Game::puff(float x, float y, int n, int kind) {
    for (int i = 0; i < n; i++) {
        for (auto& m : motes_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y;
            m.vx = (float)((i * 17) % 7 - 3);
            m.vy = (float)((i * 5) % 5 - 3);
            m.life = 0.35f + (i % 3) * 0.08f;
            m.kind = kind;
            break;
        }
    }
}

void Game::tickMotes() {
    for (auto& m : motes_) {
        if (m.life <= 0) continue;
        m.life -= 1.f / 60.f;
        m.x += m.vx;
        m.y += m.vy;
        m.vy += 0.04f;
    }
}

void Game::bootRoom() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 1 + y / 40;
        if (g > 4) g = 4;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(g, g, g + 1);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = true;
    sys_->vdp.hudEnabled = false;
    sys_->apu.setMaster(0.35f);
    sys_->apu.setEcho(0.08f, 0.15f, 0.08f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootRoom();
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
}

void Game::begin() {
    mode_ = Mode::Play;
    playF_ = 0;
    bay_ = 1;
    boiler_ = 8;
    stalled_ = 0;
    spawnIx_ = 0;
    cool_ = 0;
    seize_ = 0;
    hold_ = 0;
    flash_ = 0;
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
    seize_ = 32;
    puff(r.x, bayY(r.bay), 6, 0);
    sys_->apu.noiseBurst(0.35f, 1400.f, 0.08f);
    bool any = false;
    for (const auto& o : rivals_)
        if (o.running) any = true;
    if (spawnIx_ >= kFleet && !any) winBunker();
}

void Game::hurtBoiler() {
    if (mode_ != Mode::Play) return;
    boiler_--;
    flash_ = 8;
    puff(52.f, bayY(bay_), 4, 1);
    blip(1, 90.f, 0.2f);
    if (boiler_ <= 0) {
        boiler_ = 0;
        loseBunker("THE MACHINE STOPPED");
    }
}

void Game::winBunker() {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    hold_ = 0;
    blip(0, 440.f, 0.18f);
}

void Game::loseBunker(const char* why) {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 0;
    sys_->apu.noiseBurst(0.4f, 400.f, 0.2f);
}

void Game::firePlayer() {
    if (cool_ > 0 || mode_ != Mode::Play) return;
    Bolt b;
    b.bay = bay_;
    b.x = 78.f;
    b.vx = 5.4f;
    b.player = true;
    b.live = true;
    bolts_.push_back(b);
    cool_ = 8;
    blip(0, 620.f, 0.12f);
    puff(84.f, bayY(bay_), 2, 0);
}

void Game::fireRival(Rival& r) {
    Bolt b;
    b.bay = r.bay;
    b.x = r.x - 16.f;
    b.vx = -2.15f;
    b.player = false;
    b.live = true;
    bolts_.push_back(b);
    r.cool = r.kind ? 1.35f : 1.05f;
}

void Game::spawnDue() {
    while (spawnIx_ < kFleet && playF_ >= kSpawns[spawnIx_].frame) {
        Rival r;
        r.bay = kSpawns[spawnIx_].bay;
        r.kind = kSpawns[spawnIx_].kind;
        r.hp = r.kind ? 3 : 1;
        r.x = 308.f;
        r.cool = 0.7f + r.kind * 0.3f;
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
    if (best >= 0) bay_ = rivals_[best].bay;
    if (best >= 0) firePlayer();
}

void Game::readPad() {
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_UP)) bay_--;
    if (p.pressed(gs::BTN_DOWN)) bay_++;
    if (bay_ < 0) bay_ = 0;
    if (bay_ > 2) bay_ = 2;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO))
        firePlayer();
}

void Game::update() {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        tickMotes();
        if (++hold_ > 40) over_ = true;
        return;
    }
    if (mode_ == Mode::Pause) return;
    playF_++;
    if (cool_ > 0) cool_--;
    if (seize_ > 0) seize_--;
    if (flash_ > 0) flash_--;
    if (blipT_ > 0 && --blipT_ == 0) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
    spawnDue();
    if (bot_) botAct();
    else readPad();

    for (auto& r : rivals_) {
        if (!r.running) continue;
        float spd = r.kind ? 0.46f : 0.70f;
        r.x -= spd;
        r.cool -= 1.f / 60.f;
        if (r.cool <= 0 && r.x < 240.f && r.x > 100.f) fireRival(r);
        if (r.x < 72.f) {
            loseBunker("THE PLANT SEIZED");
            return;
        }
    }

    for (auto& b : bolts_) {
        if (!b.live) continue;
        float prev = b.x;
        b.x += b.vx;
        if (b.player) {
            for (auto& r : rivals_) {
                if (!b.live || !r.running || r.bay != b.bay) continue;
                float nose = r.x - (r.kind ? 18.f : 12.f);
                if (prev < nose && b.x >= nose - 4.f) {
                    b.live = false;
                    r.hp--;
                    puff(nose, bayY(r.bay), 3, 0);
                    if (r.hp <= 0) stopRival(r);
                    else blip(1, 180.f, 0.1f);
                }
            }
            for (auto& o : bolts_) {
                if (!b.live || &o == &b || !o.live || o.player || o.bay != b.bay) continue;
                if ((prev <= o.x && b.x >= o.x) || (o.x >= prev && o.x <= b.x)) {
                    b.live = false;
                    o.live = false;
                    puff(b.x, bayY(b.bay), 2, 0);
                }
            }
        } else if (b.bay == bay_ && b.x < 70.f && prev >= 70.f) {
            b.live = false;
            hurtBoiler();
        }
        if (b.x < -8.f || b.x > 340.f) b.live = false;
    }
    std::vector<Bolt> keep;
    keep.reserve(bolts_.size());
    for (const auto& b : bolts_)
        if (b.live) keep.push_back(b);
    bolts_.swap(keep);
    tickMotes();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (m.h <= 0 || h < 1.f) return;
    float s = h / float(m.h);
    float w = float(m.w) * s;
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.x = int(cx - w * 0.5f);
    sp.y = int(cy - h * 0.5f);
    sp.w = std::max(1, int(w));
    sp.h = std::max(1, int(h));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const char* s, float x, float y, int pal) {
    for (int i = 0; s[i]; i++) {
        unsigned char ch = (unsigned char)s[i];
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        if (ch < 32 || ch > 127) ch = '?';
        if (ch != ' ') spr(art_.glyph[ch - 32], x + 2.f, y, 7.f, pal, false);
        x += 6.f;
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    int blink = (sys_->frame / 20) & 1;

    if (mode_ == Mode::Title) {
        spr(art_.titleA, 160, 48, float(art_.titleA.h), PAL_HUD, false);
        spr(art_.titleB, 160, 72, float(art_.titleB.h), PAL_HUD, false);
        spr(art_.sub, 160, 98, float(art_.sub.h), PAL_HUD, false);
        spr(art_.plant, 70, 150, 40, PAL_PLANT, false);
        spr(art_.lightM, 150, 146, 26, PAL_LIGHT, true);
        spr(art_.heavyM, 230, 148, 32, PAL_HEAVY, true);
        spr(art_.door, 300, 140, 70, PAL_DOOR, false);
        if (blink) text("ENTER TO HOLD THE BUNKER", 52, 188, PAL_HUD);
        text("UP DOWN SLEW   Z FIRE", 70, 204, PAL_HUD);
        return;
    }

    char line[64];
    std::snprintf(line, sizeof line, "BOILER %d", boiler_);
    text(line, 8, 10, boiler_ > 2 ? PAL_HUD : PAL_HUD);
    std::snprintf(line, sizeof line, "STOPPED %d/%d", stalled_, kFleet);
    text(line, 168, 10, PAL_HUD);
    text("BAY", 8, 22, PAL_HUD);
    text(bay_ == 0 ? "CATWALK" : bay_ == 1 ? "FLOOR" : "DRAIN", 32, 22, PAL_HUD);

    if (mode_ == Mode::Victory) text("THE LAST MACHINE STILL RUNNING", 28, 40, PAL_HUD);
    else if (mode_ == Mode::Fail) text(reason_, 70, 40, PAL_HUD);
    else if (mode_ == Mode::Pause) text("PAUSED", 136, 40, PAL_HUD);

    for (const auto& b : bolts_) {
        if (!b.live) continue;
        spr(b.player ? art_.bolt : art_.ebolt, b.x, bayY(b.bay), 5.f, PAL_BOLT, !b.player);
    }
    for (const auto& m : motes_) {
        if (m.life <= 0) continue;
        spr(art_.spark, m.x, m.y, 4.f + m.life * 6.f, PAL_FX, false);
    }

    int ppal = flash_ ? PAL_FX : PAL_PLANT;
    spr(art_.plant, 40, bayY(bay_), 34, ppal, false);
    if (blink || boiler_ > 0) spr(art_.lamp, 62, bayY(bay_) - 6, 8, PAL_BOLT, false);

    for (const auto& r : rivals_) {
        const gs::Mipped& body = r.kind ? art_.heavyM : art_.lightM;
        float h = r.kind ? 30.f : 24.f;
        int pal = r.running ? (r.kind ? PAL_HEAVY : PAL_LIGHT) : PAL_WRECK;
        float y = bayY(r.bay) + (r.running ? 0.f : 4.f);
        spr(body, r.x, y, h, pal, true);
    }

    for (int i = 0; i < kBays; i++) {
        spr(art_.bay, 180, bayY(i) + 8, 16, PAL_DOOR, false);
        if (i == bay_) spr(art_.lamp, 96, bayY(i) - 12, 6, PAL_BOLT, false);
    }
    spr(art_.door, 306, 120, 150, PAL_DOOR, false);
    if (blink) spr(art_.lamp, 298, 78, 8, PAL_FX, false);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (mode_ == Mode::Title) {
        if (bot_ && age_ > 24) begin();
        else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_Z))
            begin();
        draw();
        return;
    }
    if (mode_ == Mode::Play && !bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
    else if (mode_ == Mode::Pause && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    update();
    draw();
}

}  // namespace bunkerpurs
