#include "game/purs.h"

#include <algorithm>
#include <cstdio>

namespace orchardpurs {
namespace {

struct Spawn {
    int frame;
    int row;
    int kind;
};

constexpr Spawn kSpawns[] = {
    {30, 1, 0}, {140, 0, 0}, {250, 2, 0}, {380, 1, 1}, {540, 2, 0}, {680, 0, 1},
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Title) return 0;
    if (seize_ > 0) return 2;
    return 1;
}

float Game::rowY(int row) const {
    static const float y[3] = {72.f, 118.f, 168.f};
    if (row < 0) row = 0;
    if (row > 2) row = 2;
    return y[row];
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
            m.vx = (float)((i * 13) % 7 - 3) * 0.6f;
            m.vy = (float)((i * 5) % 5 - 4) * 0.4f;
            m.life = 0.4f + (i % 3) * 0.1f;
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

void Game::bootOrchard() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = 8 - y / 40;
        if (sky < 2) sky = 2;
        int g = y > 48 ? 2 + (y / 56) : 0;
        sys_->vdp.lineBackdrop[y] = y < 40 ? gs::rgb4(sky, sky + 1, sky + 4) : gs::rgb4(2, 4 + g, 1);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = false;
    sys_->vdp.setFogColor(gs::rgb4(6, 8, 4));
    sys_->apu.setMaster(0.35f);
    sys_->apu.setEcho(0.08f, 0.12f, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    bootOrchard();
    mode_ = Mode::Title;
    age_ = 0;
    over_ = false;
    won_ = false;
}

void Game::begin() {
    mode_ = Mode::Play;
    playF_ = 0;
    row_ = 1;
    engine_ = 10;
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
    shots_.clear();
    for (auto& m : motes_) m.life = 0;
}

void Game::stallRival(Rival& r) {
    if (!r.running) return;
    r.running = false;
    r.hp = 0;
    stalled_++;
    seize_ = 36;
    puff(r.x, rowY(r.row), 7);
    sys_->apu.noiseBurst(0.32f, 900.f, 0.1f);
    bool any = false;
    for (const auto& o : rivals_)
        if (o.running) any = true;
    if (spawnIx_ >= kFleet && !any) winWatch();
}

void Game::hurtEngine() {
    if (mode_ != Mode::Play) return;
    engine_--;
    flash_ = 8;
    puff(58.f, rowY(row_), 4);
    blip(1, 110.f, 0.18f);
    if (engine_ <= 0) {
        engine_ = 0;
        loseWatch("THE MACHINE STOPPED");
    }
}

void Game::winWatch() {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    hold_ = 0;
    blip(0, 523.f, 0.16f);
}

void Game::loseWatch(const char* why) {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 0;
    sys_->apu.noiseBurst(0.4f, 280.f, 0.22f);
}

void Game::firePlayer() {
    if (cool_ > 0 || mode_ != Mode::Play) return;
    Shot s;
    s.row = row_;
    s.x = 86.f;
    s.vx = 6.2f;
    s.player = true;
    s.live = true;
    shots_.push_back(s);
    cool_ = 7;
    blip(0, 480.f, 0.1f);
}

void Game::fireRival(Rival& r) {
    Shot s;
    s.row = r.row;
    s.x = r.x - 18.f;
    s.vx = r.kind ? -1.7f : -2.05f;
    s.player = false;
    s.live = true;
    shots_.push_back(s);
    r.cool = r.kind ? 1.55f : 1.25f;
}

void Game::spawnDue() {
    while (spawnIx_ < kFleet && playF_ >= kSpawns[spawnIx_].frame) {
        Rival r;
        r.row = kSpawns[spawnIx_].row;
        r.kind = kSpawns[spawnIx_].kind;
        r.hp = r.kind ? 2 : 1;
        r.x = 312.f;
        r.cool = 1.1f + r.kind * 0.2f;
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
    if (best >= 0) row_ = rivals_[best].row;
    bool threat = false;
    for (const auto& s : shots_) {
        if (!s.live || s.player || s.row != row_) continue;
        if (s.x < 140.f) threat = true;
    }
    if (best >= 0 && !threat) firePlayer();
    if (threat) {
        for (int alt = 0; alt < kRows; alt++) {
            if (alt == row_) continue;
            bool busy = false;
            for (const auto& s : shots_)
                if (s.live && !s.player && s.row == alt && s.x < 160.f) busy = true;
            if (!busy) {
                row_ = alt;
                break;
            }
        }
    }
}

void Game::readPad() {
    const gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_UP)) row_--;
    if (p.pressed(gs::BTN_DOWN)) row_++;
    if (row_ < 0) row_ = 0;
    if (row_ > 2) row_ = 2;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO))
        firePlayer();
}

void Game::update() {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        tickMotes();
        if (++hold_ > 45) over_ = true;
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
        r.x -= r.kind ? 0.38f : 0.58f;
        r.cool -= 1.f / 60.f;
        if (r.cool <= 0 && r.x < 250.f && r.x > 110.f) fireRival(r);
        if (r.x < 78.f) {
            loseWatch("THE ROW WAS TAKEN");
            return;
        }
    }

    for (auto& b : shots_) {
        if (!b.live) continue;
        float prev = b.x;
        b.x += b.vx;
        if (b.player) {
            for (auto& r : rivals_) {
                if (!b.live || !r.running || r.row != b.row) continue;
                float nose = r.x - (r.kind ? 22.f : 16.f);
                if (prev < nose && b.x >= nose - 6.f) {
                    b.live = false;
                    r.hp--;
                    puff(nose, rowY(r.row), 3);
                    if (r.hp <= 0) stallRival(r);
                    else blip(1, 220.f, 0.08f);
                }
            }
            for (auto& o : shots_) {
                if (!b.live || &o == &b || !o.live || o.player || o.row != b.row) continue;
                if (prev <= o.x && b.x >= o.x) {
                    b.live = false;
                    o.live = false;
                    puff(b.x, rowY(b.row), 2);
                }
            }
        } else if (b.row == row_ && b.x < 74.f && prev >= 74.f) {
            b.live = false;
            hurtEngine();
        }
        if (b.x < -12.f || b.x > 340.f) b.live = false;
    }
    std::vector<Shot> keep;
    keep.reserve(shots_.size());
    for (const auto& b : shots_)
        if (b.live) keep.push_back(b);
    shots_.swap(keep);
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
        if (ch != ' ') spr(art_.glyph[ch - 32], x + 2.f, y, 10.f, pal, false);
        x += 7.f;
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    int blink = (sys_->frame / 20) & 1;

    if (mode_ == Mode::Title) {
        for (int i = 0; i < 5; i++) spr(art_.tree, 28.f + i * 66.f, 150.f, 70.f, PAL_TREE, false);
        spr(art_.tractor, 70, 178, 28, PAL_TRACTOR, false);
        spr(art_.sprayer, 170, 176, 24, PAL_SPRAY, true);
        spr(art_.harvest, 250, 174, 30, PAL_HARVEST, true);
        spr(art_.titleA, 160, 42, float(art_.titleA.h), PAL_HUD, false);
        spr(art_.titleB, 160, 68, float(art_.titleB.h), PAL_HUD, false);
        spr(art_.sub, 160, 96, float(art_.sub.h), PAL_HUD, false);
        if (blink) text("START TO TAKE THE ROWS", 58, 196, PAL_HUD);
        text("UP DOWN ROW   Z THROW", 72, 210, PAL_HUD);
        return;
    }

    for (int i = 0; i < kRows; i++) {
        spr(art_.tree, 130, rowY(i) - 22, 36, PAL_TREE, false);
        spr(art_.tree, 210, rowY(i) - 26, 40, PAL_TREE, true);
        spr(art_.tree, 280, rowY(i) - 20, 34, PAL_TREE, false);
    }
    spr(art_.shed, 22, 118, 90, PAL_SHED, false);

    char line[64];
    std::snprintf(line, sizeof line, "ENGINE %d", engine_);
    text(line, 8, 10, PAL_HUD);
    std::snprintf(line, sizeof line, "STALLED %d/%d", stalled_, kFleet);
    text(line, 168, 10, PAL_HUD);
    text(row_ == 0 ? "NORTH" : row_ == 1 ? "CENTRE" : "SOUTH", 8, 22, PAL_HUD);

    if (mode_ == Mode::Victory) text("THE LAST MACHINE STILL RUNNING", 16, 40, PAL_HUD);
    else if (mode_ == Mode::Fail) text(reason_, 64, 40, PAL_HUD);
    else if (mode_ == Mode::Pause) text("PAUSED", 130, 40, PAL_HUD);

    for (const auto& b : shots_) {
        if (!b.live) continue;
        spr(b.player ? art_.apple : art_.mist, b.x, rowY(b.row) - 2, b.player ? 8.f : 7.f,
            b.player ? PAL_APPLE : PAL_SPRAY, !b.player);
    }
    for (const auto& m : motes_) {
        if (m.life <= 0) continue;
        spr(art_.leaf, m.x, m.y, 5.f + m.life * 6.f, PAL_LEAF, false);
    }

    int ppal = flash_ ? PAL_APPLE : PAL_TRACTOR;
    spr(art_.tractor, 62, rowY(row_), 26, ppal, false);
    if (engine_ > 0 && (blink || engine_ > 3)) spr(art_.spark, 48, rowY(row_) - 12, 6, PAL_APPLE, false);

    for (const auto& r : rivals_) {
        const gs::Mipped& body = r.kind ? art_.harvest : art_.sprayer;
        float h = r.kind ? 28.f : 22.f;
        int pal = r.running ? (r.kind ? PAL_HARVEST : PAL_SPRAY) : PAL_WRECK;
        float y = rowY(r.row) + (r.running ? 0.f : 5.f);
        spr(body, r.x, y, h, pal, true);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (mode_ == Mode::Title) {
        if (bot_ && age_ > 20) begin();
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

}  // namespace orchardpurs
