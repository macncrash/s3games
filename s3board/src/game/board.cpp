#include "game/board.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace board {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int SHIFTS = 3;
constexpr int DROP_LIMIT = 3;

struct Def {
    const char* name;
    const char* blurb;
    int need;
    int live;
    float life;
    float gap;
};

const Def& shiftDef(bool practice, int shift) {
    static const Def k[SHIFTS] = {
        {"MORNING", "ONE LAMP AT A TIME", 6, 1, 12.0f, 0.40f},
        {"THE RUSH", "TWO ON THE BOARD", 8, 2, 9.5f, 0.32f},
        {"NIGHT WIRE", "THREE BEFORE THEY DROP", 10, 3, 8.5f, 0.28f},
    };
    static const Def practiceDef = {"PRACTICE", "THREE DROPS AND YOU ARE OFF", 100000, 2, 11.0f, 0.45f};
    if (practice) return practiceDef;
    if (shift < 0) shift = 0;
    if (shift >= SHIFTS) shift = SHIFTS - 1;
    return k[shift];
}

gs::FMPatch chimePatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1, 1, 0.01f, 0.18f, 0.55f, 0.12f};
    p.op[1] = {2, 0.45f, 0.01f, 0.22f, 0.4f, 0.14f};
    p.op[2] = {3, 0.25f, 0.02f, 0.2f, 0.3f, 0.16f};
    p.op[3] = {1, 0.3f, 0.01f, 0.24f, 0.45f, 0.14f};
    p.vol = 0.16f;
    return p;
}

gs::FMPatch humPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.15f;
    p.op[0] = {1, 0.35f, 0.08f, 0.5f, 0.8f, 0.4f};
    p.op[1] = {2, 0.2f, 0.05f, 0.4f, 0.7f, 0.35f};
    p.op[2] = {1, 0.15f, 0.08f, 0.5f, 0.6f, 0.4f};
    p.op[3] = {0.5f, 0.2f, 0.1f, 0.5f, 0.7f, 0.4f};
    p.vol = 0.04f;
    p.tone = 400;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Fail) return 4;
    if (mode_ == Mode::Clear) return 2;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::irnd(int n) {
    rng_ = rng_ * 1664525u + 1013904223u;
    if (n <= 1) return 0;
    return int((rng_ >> 8) % uint32_t(n));
}

Game::Call* Game::trunk(int i) {
    for (Call& c : calls_) {
        if (c.on && !c.clearing && c.trunk == i) return &c;
    }
    return nullptr;
}

int Game::liveCount() const {
    int n = 0;
    for (const Call& c : calls_)
        if (c.on && !c.clearing) n++;
    return n;
}

void Game::blip(bool high) {
    sys_->apu.tone(2, high ? 880.0f : 520.0f, 0.06f);
    beep_ = 0.045f;
}

void Game::buzz() {
    sys_->apu.tone(2, 92.0f, 0.08f);
    beep_ = 0.14f;
    sys_->apu.noiseBurst(0.16f, 500.0f, 0.09f);
}

void Game::click() { sys_->apu.noiseBurst(0.22f, 2200.0f, 0.04f); }

void Game::chime() {
    sys_->apu.keyOn(0, 523.0f, 0.12f);
    sys_->apu.keyOn(1, 784.0f, 0.1f);
}

void Game::fanTick(float dt) {
    if (fanStep_ < 0) return;
    fanT_ += dt;
    if (fanT_ < 0.11f) return;
    fanT_ = 0;
    static const float notes[] = {523.0f, 659.0f, 784.0f, 1046.0f};
    if (fanStep_ < 4) sys_->apu.keyOn(0, notes[fanStep_], 0.16f);
    else if (fanStep_ == 4) sys_->apu.keyOff(0);
    if (++fanStep_ > 6) fanStep_ = -1;
}

void Game::pop(float x, float y, const char* s, int pal) {
    for (Pop& p : pops_) {
        if (p.on) continue;
        p.on = true;
        p.x = x;
        p.y = y;
        p.t = 0.7f;
        p.pal = pal;
        std::snprintf(p.text, sizeof p.text, "%s", s);
        return;
    }
}

void Game::agePops(float dt) {
    for (Pop& p : pops_) {
        if (!p.on) continue;
        p.t -= dt;
        p.y -= 14.0f * dt;
        if (p.t <= 0) p.on = false;
    }
    if (sparkT_ > 0) sparkT_ -= dt;
}

void Game::newRun(bool practice) {
    practice_ = practice;
    shift_ = 0;
    made_ = 0;
    dropsAll_ = 0;
    won_ = false;
    over_ = false;
    held_ = -1;
    for (Pop& p : pops_) p.on = false;
    rng_ = bot_ ? 0xB0ADu : (0xB0ADu ^ uint32_t(sys_->frame) * 0x9E3779B9u);
    startShift();
}

void Game::startShift() {
    const Def& d = shiftDef(practice_, shift_);
    need_ = d.need;
    maxLive_ = d.live;
    life_ = d.life;
    spawnGap_ = d.gap;
    connected_ = 0;
    drops_ = 0;
    held_ = -1;
    bank_ = 0;
    row_ = 0;
    spawnWait_ = spawnGap_;
    for (Call& c : calls_) c = Call{};
    mode_ = Mode::Brief;
    t_ = 0;
}

void Game::beginPlay() {
    mode_ = Mode::Play;
    t_ = 0;
    held_ = -1;
    bank_ = 0;
    row_ = 0;
    for (int i = 0; i < maxLive_; i++) spawnOne();
    spawnWait_ = spawnGap_;
}

bool Game::spawnOne() {
    int trunks[JACKS], lines[JACKS];
    int nt = 0, nl = 0;
    bool usedT[JACKS] = {};
    bool usedL[JACKS] = {};
    for (const Call& c : calls_) {
        if (!c.on) continue;
        usedT[c.trunk] = true;
        usedL[c.line] = true;
    }
    for (int i = 0; i < JACKS; i++) {
        if (!usedT[i]) trunks[nt++] = i;
        if (!usedL[i]) lines[nl++] = i;
    }
    if (!nt || !nl) return false;
    for (Call& slot : calls_) {
        if (slot.on) continue;
        slot = Call{};
        slot.on = true;
        slot.trunk = trunks[irnd(nt)];
        slot.line = lines[irnd(nl)];
        slot.maxLife = life_;
        slot.life = life_;
        return true;
    }
    return false;
}

void Game::connectCall(Call& c) {
    c.clearing = true;
    c.flash = 0.32f;
    connected_++;
    made_++;
    held_ = -1;
    spawnWait_ = spawnGap_;
    pop((TRUNK_X + LINE_X) * 0.5f, (jackY(c.trunk) + jackY(c.line)) * 0.5f - 10, "ON", PAL_GREEN);
    click();
    chime();
}

void Game::dropCall(Call& c) {
    if (!c.on || c.clearing) return;
    int y = jackY(c.trunk);
    if (held_ == c.trunk) held_ = -1;
    c.on = false;
    drops_++;
    dropsAll_++;
    pop(float(TRUNK_X), float(y) - 8, "DROP", PAL_RED);
    sys_->apu.noiseBurst(0.42f, 140.0f, 0.32f);
    sys_->apu.keyOn(2, 110.0f, 0.14f);
    sparkT_ = 0.2f;
    sparkX_ = float(TRUNK_X);
    sparkY_ = float(y);
    if (drops_ >= DROP_LIMIT && mode_ == Mode::Play) {
        mode_ = Mode::Fail;
        t_ = 0;
        if (!practice_) over_ = true;
    }
}

void Game::seat() {
    if (bank_ == 0) {
        Call* c = trunk(row_);
        if (!c) {
            buzz();
            return;
        }
        if (held_ == row_) {
            held_ = -1;
            click();
            return;
        }
        held_ = row_;
        click();
        return;
    }
    if (held_ < 0) {
        buzz();
        return;
    }
    Call* c = trunk(held_);
    if (!c) {
        held_ = -1;
        buzz();
        return;
    }
    if (c->line != row_) {
        c->life -= 1.35f;
        sparkT_ = 0.28f;
        sparkX_ = float(LINE_X);
        sparkY_ = float(jackY(row_));
        pop(float(LINE_X), sparkY_ - 8, "NO", PAL_RED);
        buzz();
        if (c->life <= 0) dropCall(*c);
        return;
    }
    connectCall(*c);
}

void Game::steer(float dt) {
    const gs::Pad& pad = sys_->pad;
    int dy = (pad.down(gs::BTN_DOWN) ? 1 : 0) - (pad.down(gs::BTN_UP) ? 1 : 0);
    if (dy == 0) {
        rep_ = 0;
    } else if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_DOWN)) {
        row_ = std::clamp(row_ + dy, 0, JACKS - 1);
        rep_ = 0.18f;
        blip(false);
    } else {
        rep_ -= dt;
        if (rep_ <= 0) {
            int next = std::clamp(row_ + dy, 0, JACKS - 1);
            if (next != row_) {
                row_ = next;
                blip(false);
            }
            rep_ = 0.07f;
        }
    }
    if (pad.pressed(gs::BTN_LEFT) && bank_ != 0) {
        bank_ = 0;
        blip(false);
    } else if (pad.pressed(gs::BTN_RIGHT) && bank_ != 1) {
        bank_ = 1;
        blip(false);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) seat();
    if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X)) {
        if (held_ >= 0) click();
        held_ = -1;
    }
}

void Game::botAct() {
    if (held_ >= 0 && !trunk(held_)) held_ = -1;
    int best = -1;
    float soon = 1.0e9f;
    for (const Call& c : calls_) {
        if (!c.on || c.clearing || c.life >= soon) continue;
        soon = c.life;
        best = c.trunk;
    }
    if (best < 0) return;
    int wantBank = 0;
    int wantRow = best;
    if (held_ >= 0) {
        Call* c = trunk(held_);
        if (!c) {
            held_ = -1;
            return;
        }
        wantBank = 1;
        wantRow = c->line;
    }
    if (bank_ != wantBank) {
        bank_ = wantBank;
        return;
    }
    if (row_ != wantRow) {
        row_ += wantRow > row_ ? 1 : -1;
        return;
    }
    seat();
}

void Game::tick(float dt) {
    for (Call& c : calls_) {
        if (!c.on) continue;
        if (c.clearing) {
            c.flash -= dt;
            if (c.flash <= 0) c.on = false;
            continue;
        }
        if (connected_ >= need_) {
            c.clearing = true;
            c.flash = 0.22f;
            continue;
        }
        c.life -= dt;
        if (c.life <= 0) dropCall(c);
    }
}

void Game::maintain(float dt) {
    if (connected_ >= need_) {
        for (Call& c : calls_) {
            if (c.on && !c.clearing) {
                c.clearing = true;
                c.flash = 0.22f;
            }
        }
        held_ = -1;
        bool any = false;
        for (const Call& c : calls_)
            if (c.on) any = true;
        if (!any) finishShift();
        return;
    }
    if (liveCount() >= maxLive_) {
        spawnWait_ = spawnGap_;
        return;
    }
    spawnWait_ -= dt;
    if (spawnWait_ <= 0) {
        if (spawnOne()) spawnWait_ = spawnGap_;
        else spawnWait_ = 0.12f;
    }
}

void Game::finishShift() {
    if (!practice_ && shift_ + 1 >= SHIFTS) {
        mode_ = Mode::Victory;
        won_ = true;
        over_ = true;
        t_ = 0;
        fanStep_ = 0;
        fanT_ = 0;
        return;
    }
    if (!practice_) {
        mode_ = Mode::Clear;
        t_ = 0;
        fanStep_ = 0;
        fanT_ = 0;
    }
}

void Game::play(float dt) {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    if (bot_) botAct();
    else steer(dt);
    tick(dt);
    if (mode_ != Mode::Play) return;
    maintain(dt);
}

void Game::audio(float dt) {
    fanTick(dt);
    bool live = false;
    float minFrac = 1;
    if (mode_ == Mode::Play) {
        for (const Call& c : calls_) {
            if (!c.on || c.clearing || c.maxLife <= 0) continue;
            live = true;
            minFrac = std::min(minFrac, c.life / c.maxLife);
        }
    }
    ringT_ += dt;
    if (live) {
        float period = minFrac < 0.30f ? 0.52f : 1.15f;
        float p = std::fmod(ringT_, period);
        bool on = p < 0.08f || (p >= 0.15f && p < 0.23f);
        sys_->apu.tone(0, 440.0f, on ? 0.045f : 0);
        sys_->apu.tone(1, 480.0f, on ? 0.032f : 0);
    } else {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys_->apu.tone(2, 0, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1 || m.h < 1) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal);
    }
}

void Game::drawTop() {
    const gs::Pad& pad = sys_->pad;
    (void)pad;
    for (const Pop& p : pops_) {
        if (!p.on) continue;
        text(p.text, p.x, p.y, 0.55f, p.pal);
    }
    if (sparkT_ > 0) spr(art_.spark, sparkX_, sparkY_, 18, PAL_RED);

    if (mode_ == Mode::Title) {
        text("S3 BOARD", 160, 16, 1.05f, PAL_AMBER);
        text("CONNECT THE CALLS", 160, 36, 0.5f, PAL_WHITE);
        text("BEFORE THEY DROP", 160, 50, 0.5f, PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) text("PRESS START", 160, 66, 0.48f, PAL_WHITE);
    } else if (mode_ == Mode::Menu) {
        text("S3 BOARD", 160, 78, 0.85f, PAL_AMBER);
        const char* items[] = {"NIGHT SHIFT", "PRACTICE", "HOW TO PATCH"};
        for (int i = 0; i < 3; i++) {
            float y = 108 + i * 18;
            text(items[i], 168, y, 0.55f, i == menu_ ? PAL_AMBER : PAL_WHITE);
            if (i == menu_) spr(art_.lamp, 78, y, 14, PAL_INN);
        }
    } else if (mode_ == Mode::Help) {
        text("HOW TO PATCH", 160, 64, 0.7f, PAL_AMBER);
        const char* lines[] = {"MATCH THE MARK", "ARROWS MOVE THE PLUG", "C SEATS IT", "X PULLS THE CORD",
                               "3 DROPS ENDS THE SHIFT"};
        for (int i = 0; i < 5; i++) text(lines[i], 160, 88 + i * 14, 0.42f, PAL_WHITE);
        for (int i = 0; i < JACKS; i++) {
            float x = 58 + i * 40;
            spr(art_.badge[i], x, 168, 16, callPal(i));
            text(lineName(i), x, 184, 0.36f, PAL_WHITE);
        }
        text("START RETURNS", 160, 200, 0.4f, PAL_AMBER);
    } else if (mode_ == Mode::Brief) {
        const Def& d = shiftDef(practice_, shift_);
        text(d.name, 160, 86, 0.85f, PAL_AMBER);
        text(d.blurb, 160, 108, 0.42f, PAL_WHITE);
        char buf[40];
        if (!practice_) std::snprintf(buf, sizeof buf, "CONNECT %d", need_);
        else std::snprintf(buf, sizeof buf, "STAY ON THE BOARD");
        text(buf, 160, 128, 0.5f, PAL_GREEN);
        text("MATCH THE MARK", 160, 148, 0.42f, PAL_WHITE);
        text("PRESS START", 160, 170, 0.48f, PAL_AMBER);
    } else if (mode_ == Mode::Clear) {
        const Def& d = shiftDef(false, shift_);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%s CLEAR", d.name);
        text(buf, 160, 96, 0.62f, PAL_GREEN);
        std::snprintf(buf, sizeof buf, "%d CONNECTED", made_);
        text(buf, 160, 120, 0.5f, PAL_WHITE);
        text(shift_ + 1 < SHIFTS ? "NEXT WATCH" : "DONE", 160, 146, 0.48f, PAL_AMBER);
    } else if (mode_ == Mode::Fail) {
        text("THREE DROPS", 160, 96, 0.7f, PAL_RED);
        text("THE BOARD LET GO", 160, 120, 0.48f, PAL_WHITE);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%d CONNECTED", made_);
        text(buf, 160, 144, 0.48f, PAL_AMBER);
        text("START RETURNS", 160, 168, 0.42f, PAL_WHITE);
    } else if (mode_ == Mode::Victory) {
        text("THE LINES HELD", 160, 90, 0.62f, PAL_AMBER);
        text("EVERY CALL CONNECTED", 160, 114, 0.42f, PAL_WHITE);
        char buf[40];
        std::snprintf(buf, sizeof buf, "%d ON THE BOARD", made_);
        text(buf, 160, 138, 0.5f, PAL_GREEN);
        text("PRESS START", 160, 164, 0.42f, PAL_WHITE);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 108, 0.9f, PAL_AMBER);
        text("START RESUMES", 160, 132, 0.42f, PAL_WHITE);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        float cx = float(bank_ == 0 ? TRUNK_X : LINE_X);
        float cy = float(jackY(row_));
        spr(art_.ring, cx, cy, 26, held_ >= 0 ? PAL_AMBER : PAL_WHITE);
        if (held_ >= 0) spr(art_.ring, float(TRUNK_X), float(jackY(held_)), 30, PAL_GREEN);
        spr(art_.plug, cx, cy + 8, 26, PAL_CORD);
    }
}

void Game::drawWorld() {
    bool show = mode_ == Mode::Title || mode_ == Mode::Play || mode_ == Mode::Pause;
    if (!show) return;

    auto cord = [&](float x0, float y0, float x1, float y1) {
        const int beads = 10;
        for (int i = 0; i <= beads; i++) {
            float t = float(i) / float(beads);
            float x = x0 + (x1 - x0) * t;
            float y = y0 + (y1 - y0) * t + std::sin(t * 3.1415926f) * 16.0f;
            spr(art_.bead, x, y, 6, PAL_CORD);
        }
    };

    for (int i = 0; i < JACKS; i++) {
        bool hot = false;
        for (const Call& c : calls_) {
            if (c.on && c.line == i) hot = true;
        }
        int fog = 8;
        float h = 13;
        if (mode_ == Mode::Title) {
            fog = 3;
            h = 15 + std::sin(float(sys_->frame) * 0.08f + i) * 0.8f;
        } else if (hot) {
            fog = 0;
            h = 16;
        }
        float x = float(LINE_X);
        float y = float(jackY(i));
        spr(art_.badge[i], x - 16, y, hot || mode_ == Mode::Title ? 13 : 10, callPal(i), fog);
        spr(art_.lamp, x, y, h, callPal(i), fog);
    }

    if (mode_ != Mode::Play && mode_ != Mode::Pause) return;

    for (const Call& c : calls_) {
        if (!c.on) continue;
        float x = float(TRUNK_X);
        float y = float(jackY(c.trunk));
        float frac = c.maxLife > 0 ? std::clamp(c.life / c.maxLife, 0.0f, 1.0f) : 0;
        bool blink = true;
        if (!c.clearing) {
            int step = frac < 0.28f ? 4 : 9;
            blink = (int(sys_->frame / step) % 3) != 0;
        }
        float bh = held_ == c.trunk ? 15 : 12;
        spr(art_.badge[c.line], x + 16, y, bh, callPal(c.line));
        if (blink) spr(art_.lamp, x, y, c.clearing ? 17 : 15, callPal(c.line));
        if (!c.clearing) {
            int pal = frac < 0.30f ? PAL_RED : callPal(c.line);
            sprBox(art_.bar, x - 14, y + 10, std::max(2.0f, 28.0f * frac), 4, pal);
        }
        if (c.clearing) cord(x, y, float(LINE_X), float(jackY(c.line)));
    }
    if (held_ >= 0) {
        float x1 = float(bank_ == 0 ? TRUNK_X : LINE_X);
        float y1 = float(jackY(row_));
        cord(float(TRUNK_X), float(jackY(held_)), x1, y1);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    drawTop();
    if (mode_ == Mode::Title || mode_ == Mode::Menu) spr(art_.op, mode_ == Mode::Title ? 62 : 54, 150, 108, PAL_OP);
    if (mode_ == Mode::Menu || mode_ == Mode::Help || mode_ == Mode::Brief || mode_ == Mode::Clear || mode_ == Mode::Fail ||
        mode_ == Mode::Victory || mode_ == Mode::Pause)
        spr(art_.card, 160, 128, float(art_.card.h), PAL_BG);
    drawWorld();

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        const Def& d = shiftDef(practice_, shift_);
        char buf[40];
        hud(1, 0, "S3 BOARD", PAL_WHITE);
        std::snprintf(buf, sizeof buf, "%s", d.name);
        hud(1, 1, buf, PAL_AMBER);
        if (practice_) std::snprintf(buf, sizeof buf, "%d UP", connected_);
        else std::snprintf(buf, sizeof buf, "%d/%d", connected_, need_);
        hud(28, 1, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "DROPS %d/%d", drops_, DROP_LIMIT);
        hud(1, 2, buf, drops_ > 0 ? PAL_RED : PAL_WHITE);
        hudC(26, "ARROWS MOVE   C PLUG   X PULL", PAL_WHITE);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    sys.apu.setPatch(0, chimePatch());
    sys.apu.setPatch(1, chimePatch());
    sys.apu.setPatch(2, chimePatch());
    sys.apu.setPatch(7, humPatch());
    sys.apu.keyOn(7, 98.0f, 0.04f);
    if (bot_) newRun(false);
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = DT;
    t_ += dt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Menu;
            menu_ = 0;
            t_ = 0;
            blip(true);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Menu) {
        if (pad.pressed(gs::BTN_DOWN)) {
            menu_ = (menu_ + 1) % 3;
            blip(false);
        } else if (pad.pressed(gs::BTN_UP)) {
            menu_ = (menu_ + 2) % 3;
            blip(false);
        }
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            if (menu_ == 2) {
                mode_ = Mode::Help;
                t_ = 0;
            } else {
                newRun(menu_ == 1);
            }
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Help) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Menu;
            blip(false);
        }
    } else if (mode_ == Mode::Brief) {
        if ((bot_ && t_ > 0.35f) || pad.pressed(gs::BTN_START)) {
            beginPlay();
            blip(true);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Menu;
        }
    } else if (mode_ == Mode::Play) {
        play(dt);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Menu;
    } else if (mode_ == Mode::Clear) {
        if ((bot_ && t_ > 0.45f) || pad.pressed(gs::BTN_START) || t_ > 1.5f) {
            shift_++;
            startShift();
        }
    } else if (mode_ == Mode::Fail || mode_ == Mode::Victory) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = Mode::Menu;
            over_ = false;
            won_ = false;
        }
    }

    agePops(dt);
    audio(dt);
    draw();
}

}  // namespace board
