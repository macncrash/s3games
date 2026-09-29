#include "game/purs.h"

#include <algorithm>
#include <cstdio>

namespace palisadepurs {
namespace {

struct Spawn {
    int frame;
    int gap;
    int kind;
};

// kind 0 light cart, kind 1 ram. Staggered so the nearest can be stopped first.
constexpr Spawn kSpawns[] = {
    {20, 1, 0}, {180, 0, 0}, {320, 2, 0}, {480, 1, 1}, {700, 0, 1},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Title) return 0;
    if (seize_ > 0) return 2;
    return 1;
}

float Game::gapY(int gap) const {
    static const float y[3] = {72.f, 118.f, 164.f};
    if (gap < 0) gap = 0;
    if (gap > 2) gap = 2;
    return y[gap];
}

void Game::blip(int ch, float freq, float vol) {
    sys_->apu.tone(ch, freq, vol);
    blipT_ = 6;
}

void Game::puff(float x, float y, int n) {
    for (int i = 0; i < n; i++) {
        for (auto& m : motes_) {
            if (m.life > 0) continue;
            m.x = x;
            m.y = y;
            m.vx = float((i * 13) % 7 - 3);
            m.vy = float((i * 5) % 5 - 2);
            m.life = 0.32f + float(i % 3) * 0.08f;
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
        m.vy += 0.05f;
    }
}

void Game::bootField() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) {
            int t = y / 16;
            c = gs::rgb4(3 + t / 2, 4 + t / 3, 8 - t / 4);
        } else {
            int g = 3 + (y - 78) / 36;
            if (g > 6) g = 6;
            c = gs::rgb4(2, g, 1);
        }
        sys_->vdp.lineBackdrop[y] = c;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = false;
    sys_->vdp.setFogColor(gs::rgb4(4, 5, 6));
    sys_->apu.setMaster(0.35f);
    sys_->apu.setEcho(0.06f, 0.12f, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootField();
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Play;
    playF_ = 0;
    gap_ = 1;
    fire_ = 6;
    stopped_ = 0;
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
    if (sys_) sys_->setLight(90, 70, 30);
}

void Game::stopRival(Rival& r) {
    if (!r.running) return;
    r.running = false;
    r.hp = 0;
    stopped_++;
    seize_ = 36;
    puff(r.x, gapY(r.gap), 6);
    sys_->apu.noiseBurst(0.32f, 900.f, 0.08f);
    bool any = false;
    for (const auto& o : rivals_)
        if (o.running) any = true;
    if (spawnIx_ >= kFleet && !any) winWall();
}

void Game::hurtEngine() {
    if (mode_ != Mode::Play) return;
    fire_--;
    flash_ = 8;
    puff(70.f, gapY(gap_), 4);
    blip(1, 80.f, 0.18f);
    if (fire_ <= 0) {
        fire_ = 0;
        loseWall("YOUR ENGINE STALLED");
    }
}

void Game::winWall() {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    hold_ = 0;
    blip(0, 520.f, 0.16f);
}

void Game::loseWall(const char* why) {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 0;
    sys_->apu.noiseBurst(0.4f, 280.f, 0.22f);
}

void Game::firePlayer() {
    if (cool_ > 0 || mode_ != Mode::Play) return;
    Bolt b;
    b.gap = gap_;
    b.x = 86.f;
    b.vx = 6.4f;
    b.player = true;
    b.live = true;
    bolts_.push_back(b);
    cool_ = 7;
    blip(0, 540.f, 0.1f);
}

void Game::fireRival(Rival& r) {
    Bolt b;
    b.gap = r.gap;
    b.x = r.x - 18.f;
    b.vx = -1.7f;
    b.player = false;
    b.live = true;
    bolts_.push_back(b);
    r.cool = r.kind ? 1.55f : 1.25f;
}

void Game::spawnDue() {
    while (spawnIx_ < kFleet && playF_ >= kSpawns[spawnIx_].frame) {
        Rival r;
        r.gap = kSpawns[spawnIx_].gap;
        r.kind = kSpawns[spawnIx_].kind;
        r.hp = r.kind ? 3 : 1;
        r.x = 308.f;
        r.cool = 1.1f + float(r.kind) * 0.4f;
        r.running = true;
        rivals_.push_back(r);
        spawnIx_++;
    }
}

void Game::botAct() {
    int best = -1;
    float near = 1e9f;
    for (int i = 0; i < int(rivals_.size()); i++) {
        if (!rivals_[i].running) continue;
        if (rivals_[i].x < near) {
            near = rivals_[i].x;
            best = i;
        }
    }
    if (best >= 0) gap_ = rivals_[best].gap;
    if (best >= 0) firePlayer();
}

void Game::readPad() {
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_UP)) gap_--;
    if (p.pressed(gs::BTN_DOWN)) gap_++;
    if (gap_ < 0) gap_ = 0;
    if (gap_ > 2) gap_ = 2;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO))
        firePlayer();
}

void Game::update() {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        tickMotes();
        if (++hold_ > 48) over_ = true;
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
        r.x -= r.kind ? 0.34f : 0.52f;
        r.cool -= 1.f / 60.f;
        if (r.cool <= 0.f && r.x < 230.f && r.x > 120.f) fireRival(r);
        if (r.x < 98.f) {
            loseWall("A MACHINE TOOK THE WALL");
            return;
        }
    }

    for (auto& b : bolts_) {
        if (!b.live) continue;
        float prev = b.x;
        b.x += b.vx;
        if (b.player) {
            for (auto& r : rivals_) {
                if (!b.live || !r.running || r.gap != b.gap) continue;
                float nose = r.x - (r.kind ? 20.f : 14.f);
                if (prev < nose && b.x >= nose - 6.f) {
                    b.live = false;
                    r.hp--;
                    puff(nose, gapY(r.gap), 3);
                    if (r.hp <= 0) stopRival(r);
                    else blip(1, 200.f, 0.08f);
                }
            }
        } else if (b.gap == gap_ && prev >= 78.f && b.x < 78.f) {
            b.live = false;
            hurtEngine();
        }
        if (b.x < -12.f || b.x > 340.f) b.live = false;
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
        text("PALISADE", 112, 48, PAL_HUD);
        text("PURSUIT", 118, 64, PAL_HUD);
        text("ONE WALL. LAST MACHINE.", 70, 92, PAL_HUD);
        for (int i = 0; i < 5; i++) spr(art_.stake, 36.f + float(i) * 14.f, 150, 52, PAL_STAKE, false);
        spr(art_.you, 150, 150, 22, PAL_YOU, false);
        spr(art_.ram, 230, 148, 22, PAL_RAM, true);
        if (blink) text("START TO HOLD THE WALL", 64, 188, PAL_HUD);
        text("UP DOWN GAP   Z FIRE", 76, 206, PAL_HUD);
        return;
    }

    for (int i = 0; i < 7; i++) {
        float y = 48.f + float(i) * 24.f;
        spr(art_.stake, 22, y, 36, PAL_STAKE, false);
    }
    for (int i = 0; i < kGaps; i++) {
        spr(art_.stake, 108, gapY(i) + 16, 18, PAL_FIELD, false);
    }

    char line[64];
    std::snprintf(line, sizeof line, "FIRE %d", fire_);
    text(line, 8, 10, fire_ > 2 ? PAL_HUD : PAL_FX);
    std::snprintf(line, sizeof line, "STOPPED %d/%d", stopped_, kFleet);
    text(line, 176, 10, PAL_HUD);
    text(gap_ == 0 ? "CREST" : gap_ == 1 ? "GATE" : "FOOT", 8, 22, PAL_HUD);

    if (mode_ == Mode::Victory) text("THE LAST MACHINE STILL RUNNING", 40, 40, PAL_HUD);
    else if (mode_ == Mode::Fail) text(reason_, 52, 40, PAL_FX);
    else if (mode_ == Mode::Pause) text("PAUSED", 136, 40, PAL_HUD);

    for (const auto& b : bolts_) {
        if (!b.live) continue;
        spr(b.player ? art_.bolt : art_.shot, b.x, gapY(b.gap), 4.f, PAL_BOLT, !b.player);
    }
    for (const auto& m : motes_) {
        if (m.life <= 0) continue;
        spr(art_.spark, m.x, m.y, 3.f + m.life * 8.f, PAL_FX, false);
    }

    int ppal = flash_ ? PAL_FX : PAL_YOU;
    spr(art_.you, 64, gapY(gap_), 22, ppal, false);
    if (blink || fire_ > 0) spr(art_.lamp, 78, gapY(gap_) - 6, 6, PAL_BOLT, false);

    for (const auto& r : rivals_) {
        const gs::Mipped& body = r.kind ? art_.ram : art_.cart;
        float h = r.kind ? 22.f : 16.f;
        int pal = r.running ? (r.kind ? PAL_RAM : PAL_CART) : PAL_WRECK;
        float y = gapY(r.gap) + (r.running ? 0.f : 3.f);
        spr(body, r.x, y, h, pal, true);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_Z)) begin();
        draw();
        return;
    }
    if ((mode_ == Mode::Victory || mode_ == Mode::Fail) && !bot_ && sys.pad.pressed(gs::BTN_START)) {
        begin();
        draw();
        return;
    }
    if (mode_ == Mode::Play && !bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
    else if (mode_ == Mode::Pause && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    update();
    draw();
}

}  // namespace palisadepurs
