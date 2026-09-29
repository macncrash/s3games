#include "game/boardmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace boardmark {
namespace {

constexpr float DT = 1.0f / 60.0f;

gs::FMPatch bell() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.5f, 0.12f};
    p.op[1] = {2, 0.4f, 0.01f, 0.2f, 0.35f, 0.14f};
    p.op[2] = {3, 0.22f, 0.02f, 0.2f, 0.25f, 0.16f};
    p.op[3] = {1, 0.28f, 0.01f, 0.22f, 0.4f, 0.14f};
    p.vol = 0.15f;
    return p;
}

int linePal(int line, bool mark) {
    if (mark || line == MARK_LINE) return PAL_GOLD;
    return line % 2 == 0 ? PAL_TEAL : PAL_ROSE;
}

const char* lineName(int i) {
    static const char* n[] = {"PIER", "YARD", "MARK", "LOFT", "WIRE"};
    if (i < 0) i = 0;
    if (i >= JACKS) i = JACKS - 1;
    return n[i];
}

}  // namespace

int Game::irnd(int n) {
    rng_ = rng_ * 1664525u + 1013904223u;
    if (n <= 1) return 0;
    return int((rng_ >> 8) % uint32_t(n));
}

Game::Call* Game::liveTrunk(int i) {
    for (Call& c : calls_) {
        if (c.on && !c.clearing && c.trunk == i) return &c;
    }
    return nullptr;
}

const Game::Call* Game::markCall() const {
    for (const Call& c : calls_) {
        if (c.on && c.mark) return &c;
    }
    return nullptr;
}

void Game::blip(bool high) {
    sys_->apu.tone(2, high ? 920.0f : 480.0f, 0.05f);
    beep_ = 0.04f;
}

void Game::click() { sys_->apu.noiseBurst(0.2f, 2400.0f, 0.035f); }

void Game::buzz() {
    sys_->apu.tone(2, 90.0f, 0.07f);
    beep_ = 0.12f;
    sys_->apu.noiseBurst(0.14f, 420.0f, 0.08f);
}

void Game::begin() {
    for (Call& c : calls_) c = Call{};
    held_ = -1;
    bank_ = 0;
    row_ = 0;
    finished_ = false;
    left_ = false;
    hold_ = 0;
    opX_ = 46;
    int trunk = irnd(JACKS);
    calls_[0].on = true;
    calls_[0].mark = true;
    calls_[0].trunk = trunk;
    calls_[0].line = MARK_LINE;
    calls_[0].maxLife = 11.0f;
    calls_[0].life = 11.0f;
    int other = (trunk + 2) % JACKS;
    int otherLine = (MARK_LINE + 1 + irnd(JACKS - 1)) % JACKS;
    if (otherLine == MARK_LINE) otherLine = (MARK_LINE + 1) % JACKS;
    calls_[1].on = true;
    calls_[1].mark = false;
    calls_[1].trunk = other;
    calls_[1].line = otherLine;
    calls_[1].maxLife = 8.0f;
    calls_[1].life = 8.0f;
    mode_ = Mode::Brief;
    t_ = 0;
}

void Game::dropMark() {
    if (mode_ != Mode::Play || finished_) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    t_ = 0;
    held_ = -1;
    sys_->apu.noiseBurst(0.4f, 120.0f, 0.3f);
}

void Game::seat() {
    if (bank_ == 0) {
        Call* c = liveTrunk(row_);
        if (!c) {
            buzz();
            return;
        }
        held_ = (held_ == row_) ? -1 : row_;
        click();
        return;
    }
    if (held_ < 0) {
        buzz();
        return;
    }
    Call* c = liveTrunk(held_);
    if (!c) {
        held_ = -1;
        buzz();
        return;
    }
    if (c->line != row_) {
        c->life -= 1.4f;
        buzz();
        if (c->mark && c->life <= 0) dropMark();
        return;
    }
    c->clearing = true;
    held_ = -1;
    click();
    sys_->apu.keyOn(0, c->mark ? 659.0f : 392.0f, 0.12f);
    if (c->mark) {
        finished_ = true;
        hold_ = 0.85f;
        sys_->apu.keyOn(1, 988.0f, 0.1f);
    }
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
    if (pad.pressed(gs::BTN_LEFT)) {
        bank_ = 0;
        blip(false);
    } else if (pad.pressed(gs::BTN_RIGHT)) {
        bank_ = 1;
        blip(false);
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) seat();
    if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_X)) {
        if (held_ >= 0) click();
        held_ = -1;
    }
}

void Game::botAct() {
    if (finished_) return;
    const Call* mark = markCall();
    if (!mark || mark->clearing) return;
    int wantBank = 0;
    int wantRow = mark->trunk;
    if (held_ >= 0) {
        wantBank = 1;
        wantRow = mark->line;
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
        if (!c.on || c.clearing) continue;
        if (finished_ && !c.mark) continue;
        c.life -= dt;
        if (c.life > 0) continue;
        c.on = false;
        if (held_ == c.trunk) held_ = -1;
        if (c.mark) dropMark();
    }
    if (!finished_ || mode_ != Mode::Play) return;
    hold_ -= dt;
    if (hold_ <= 0) {
        mode_ = Mode::Leave;
        t_ = 0;
        opX_ = 56;
    }
}

void Game::play(float dt) {
    if (!bot_ && sys_->pad.pressed(gs::BTN_START)) return;
    if (bot_) botAct();
    else steer(dt);
    tick(dt);
}

void Game::audio(float dt) {
    bool live = mode_ == Mode::Play && !finished_ && markCall() != nullptr;
    ringT_ += dt;
    if (live) {
        float p = std::fmod(ringT_, 1.05f);
        bool on = p < 0.07f || (p >= 0.14f && p < 0.21f);
        sys_->apu.tone(0, 440.0f, on ? 0.04f : 0);
        sys_->apu.tone(1, 494.0f, on ? 0.028f : 0);
    } else if (mode_ != Mode::Leave) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
    }
    if (beep_ > 0) {
        beep_ -= dt;
        if (beep_ <= 0) sys_->apu.tone(2, 0, 0);
    }
}

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

void Game::hud(int col, int row, const std::string& s, int pal) {
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    v.B.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 1 + y / 40;
        if (g > 4) g = 4;
        v.lineBackdrop[y] = gs::rgb4(1, 1, g);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    spr(art_.door, 292, 150, 70, PAL_WOOD, mode_ == Mode::Leave ? 0 : 6);

    for (int i = 0; i < JACKS; i++) {
        bool mark = i == MARK_LINE;
        int pal = linePal(i, mark);
        float y = float(jackY(i));
        float pulse = mark ? 1.0f + 0.08f * std::sin(float(sys_->frame) * 0.12f) : 1.0f;
        spr(art_.lamp, float(LINE_X), y, 16 * pulse, pal, mark ? 0 : 4);
        if (mark) spr(art_.coin, float(LINE_X) - 18, y, 14, PAL_GOLD);
        text(lineName(i), float(LINE_X) + 22, y, 0.32f, pal, -1);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Leave || mode_ == Mode::Fail || mode_ == Mode::Brief) {
        for (const Call& c : calls_) {
            if (!c.on) continue;
            int pal = linePal(c.line, c.mark);
            float x = float(TRUNK_X);
            float y = float(jackY(c.trunk));
            bool blink = c.clearing || ((sys_->frame / (c.life < 3.0f ? 4 : 8)) % 2 == 0);
            if (blink) spr(art_.lamp, x, y, c.mark ? 18 : 14, pal);
            if (c.mark) spr(art_.coin, x + 16, y, 12, PAL_GOLD);
            if (!c.clearing && c.maxLife > 0) {
                float frac = std::clamp(c.life / c.maxLife, 0.0f, 1.0f);
                sprBox(art_.bar, x - 16, y + 10, std::max(2.0f, 32.0f * frac), 4, frac < 0.3f ? PAL_BAD : pal);
            }
            if (c.clearing || held_ == c.trunk) {
                float x1 = c.clearing ? float(LINE_X) : float(bank_ == 0 ? TRUNK_X : LINE_X);
                float y1 = c.clearing ? float(jackY(c.line)) : float(jackY(row_));
                for (int i = 0; i <= 9; i++) {
                    float u = float(i) / 9.0f;
                    float bx = x + (x1 - x) * u;
                    float by = y + (y1 - y) * u + std::sin(u * 3.14159f) * 14.0f;
                    spr(art_.bead, bx, by, 6, c.mark ? PAL_GOLD : PAL_CORD);
                }
            }
        }
    }

    if (mode_ == Mode::Play && !finished_) {
        float cx = float(bank_ == 0 ? TRUNK_X : LINE_X);
        spr(art_.ring, cx, float(jackY(row_)), 24, held_ >= 0 ? PAL_GOLD : PAL_INK);
        spr(art_.plug, cx, float(jackY(row_)) + 10, 22, PAL_CORD);
    }

    float ox = mode_ == Mode::Leave ? opX_ : 40.0f;
    spr(art_.op, ox, 176, 48, PAL_WOOD);

    if (mode_ == Mode::Title) {
        text("S3 BOARDMARK", 160, 18, 0.72f, PAL_GOLD);
        text("FINISH THE MARK", 160, 40, 0.42f, PAL_INK);
        text("THEN LEAVE", 160, 54, 0.42f, PAL_GOLD);
        if ((sys_->frame / 30) % 2 == 0) text("PRESS START", 160, 72, 0.4f, PAL_INK);
    } else if (mode_ == Mode::Brief) {
        text("THE GOLD LAMP", 160, 16, 0.5f, PAL_GOLD);
        text("IS THE MARK", 160, 32, 0.42f, PAL_INK);
        text("SEAT IT  THEN WALK OUT", 160, 48, 0.36f, PAL_INK);
    } else if (mode_ == Mode::Play) {
        hud(1, 0, "S3 BOARDMARK", PAL_GOLD);
        hud(1, 1, finished_ ? "MARK HELD" : "FIND THE MARK", finished_ ? PAL_OK : PAL_INK);
        hud(1, 26, "ARROWS MOVE   C PLUG", PAL_INK);
    } else if (mode_ == Mode::Leave) {
        text("MARK FINISHED", 150, 28, 0.5f, PAL_GOLD);
        text("LEAVING THE DESK", 150, 46, 0.4f, PAL_OK);
    } else if (mode_ == Mode::Fail) {
        text("THE MARK DIED", 160, 28, 0.52f, PAL_BAD);
        text("YOU STAY", 160, 48, 0.4f, PAL_INK);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.1f, 0.2f, 0.12f);
    sys.apu.setPatch(0, bell());
    sys.apu.setPatch(1, bell());
    rng_ = bot_ ? 0xB0ADu : (0xB0ADu ^ uint32_t(sys.frame * 17u + 3u));
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) {
            begin();
            blip(true);
        }
    } else if (mode_ == Mode::Brief) {
        if ((bot_ && t_ > 0.4f) || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Play;
            t_ = 0;
            blip(true);
        }
    } else if (mode_ == Mode::Play) {
        play(DT);
    } else if (mode_ == Mode::Leave) {
        opX_ += 78.0f * DT;
        if (opX_ > 300.0f) {
            left_ = true;
            won_ = true;
            over_ = true;
        }
    } else if (mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            over_ = false;
            won_ = false;
            mode_ = Mode::Title;
            t_ = 0;
        }
    }

    audio(DT);
    draw();
}

}  // namespace boardmark
