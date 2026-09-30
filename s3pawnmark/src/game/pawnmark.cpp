#include "game/pawnmark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace pawnmark {
namespace {

constexpr float DT = 1.0f / 60.0f;

gs::FMPatch bell() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.16f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.5f, 0.12f};
    p.op[1] = {2, 0.4f, 0.01f, 0.2f, 0.35f, 0.14f};
    p.op[2] = {3, 0.22f, 0.02f, 0.2f, 0.25f, 0.16f};
    p.op[3] = {1, 0.28f, 0.01f, 0.22f, 0.4f, 0.14f};
    p.vol = 0.16f;
    return p;
}

}  // namespace

bool Game::empty(int f, int r) const {
    if (f < 0 || r < 0 || f >= FILES || r >= RANKS) return false;
    if (r == 0 && f != MARK_FILE) return false;
    for (const Man& m : men_) {
        if (m.alive && m.f == f && m.r == r) return false;
    }
    return true;
}

bool Game::walk(Man& m, int df, int dr) {
    int f = m.f + df;
    int r = m.r + dr;
    if (!empty(f, r)) return false;
    m.f = f;
    m.r = r;
    return true;
}

void Game::finish() {
    if (finished_) return;
    finished_ = true;
    mode_ = Mode::Done;
    hold_ = 0.7f;
    sys_->apu.keyOn(0, 523.0f, 0.2f);
    sys_->apu.keyOn(1, 659.0f, 0.16f);
    sys_->apu.keyOn(2, 784.0f, 0.14f);
}

void Game::begin() {
    men_[0] = {1, 4, false, true};
    men_[1] = {0, 1, true, true};
    men_[2] = {3, 2, true, true};
    men_[3] = {1, 2, true, true};
    finished_ = false;
    won_ = false;
    over_ = false;
    clock_ = 14;
    step_ = 0.15f;
    foe_ = 1.8f;
    t_ = 0;
    mode_ = bot_ ? Mode::Play : Mode::Brief;
}

void Game::botAct() {
    Man& p = men_[0];
    if (p.f < MARK_FILE) walk(p, 1, 0);
    else if (p.f > MARK_FILE) walk(p, -1, 0);
    else if (p.r > 0) walk(p, 0, -1);
    blip(p.r == 0);
    if (p.f == MARK_FILE && p.r == 0) finish();
}

void Game::humanAct() {
    const gs::Pad& pad = sys_->pad;
    Man& p = men_[0];
    int df = 0, dr = 0;
    if (pad.pressed(gs::BTN_LEFT)) df = -1;
    else if (pad.pressed(gs::BTN_RIGHT)) df = 1;
    else if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_A)) dr = -1;
    else if (pad.pressed(gs::BTN_DOWN)) dr = 1;
    else return;
    if (!walk(p, df, dr)) {
        blip(false);
        return;
    }
    blip(true);
    if (p.f == MARK_FILE && p.r == 0) finish();
}

void Game::stepBlacks() {
    Man& p = men_[0];
    for (int i = 1; i < 4; i++) {
        Man& b = men_[i];
        if (!b.alive) continue;
        int nr = b.r + 1;
        if (nr >= RANKS) {
            b.alive = false;
            continue;
        }
        if (p.alive && p.f == b.f && p.r == nr) {
            p.alive = false;
            b.r = nr;
            mode_ = Mode::Fail;
            over_ = true;
            won_ = false;
            sys_->apu.keyOn(0, 110.0f, 0.2f);
            return;
        }
        bool blocked = false;
        for (int j = 1; j < 4; j++) {
            if (j != i && men_[j].alive && men_[j].f == b.f && men_[j].r == nr) blocked = true;
        }
        if (!blocked) b.r = nr;
    }
}

void Game::play(float dt) {
    if (!men_[0].alive || finished_) return;
    clock_ -= dt;
    step_ -= dt;
    foe_ -= dt;
    if (step_ <= 0) {
        step_ = bot_ ? 0.22f : 0.12f;
        if (bot_) botAct();
    }
    if (!bot_) humanAct();
    if (finished_ || !men_[0].alive) return;
    if (foe_ <= 0) {
        foe_ = 1.7f;
        stepBlacks();
    }
    if (men_[0].alive && clock_ <= 0) {
        mode_ = Mode::Fail;
        over_ = true;
        won_ = false;
        sys_->apu.keyOn(0, 98.0f, 0.18f);
    }
}

void Game::blip(bool high) {
    sys_->apu.tone(2, high ? 880.0f : 140.0f, 0.05f);
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
        int g = 2 + (y * 3) / gs::SCREEN_H;
        v.lineBackdrop[y] = gs::rgb4(1, g, 1);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) {
        text("S3 PAWNMARK", 160, 28, 0.7f, PAL_GOLD);
        text("A SHORT PAWN", 160, 54, 0.42f, PAL_INK);
        text("FINISH THE MARK", 160, 74, 0.4f, PAL_IVORY);
        if ((sys_->frame / 30) % 2 == 0) text("PRESS START", 160, 110, 0.42f, PAL_GOLD);
        spr(art_.pawn, 160, 168, 52, PAL_IVORY);
        spr(art_.mark, 160, 128, 22, PAL_GOLD);
    } else if (mode_ == Mode::Brief) {
        text("THE GOLD SQUARE", 160, 18, 0.46f, PAL_GOLD);
        text("IS THE MARK", 160, 36, 0.4f, PAL_INK);
        text("WALK THE PAWN ONTO IT", 160, 56, 0.34f, PAL_IVORY);
    }

    if (mode_ != Mode::Title) {
        if (finished_) {
            text("MARK FINISHED", 160, 22, 0.48f, PAL_GOLD);
            spr(art_.crown, float(sqX(MARK_FILE)), float(sqY(0)) - 18, 16, PAL_GOLD);
        } else if (mode_ == Mode::Fail) {
            text(men_[0].alive ? "THE CLOCK DIED" : "THE PAWN FELL", 160, 18, 0.42f, PAL_BAD);
        } else if (mode_ == Mode::Play) {
            hud(1, 0, "S3 PAWNMARK", PAL_GOLD);
            hud(22, 0, "MARK OPEN", PAL_INK);
            int sec = std::max(0, int(std::ceil(clock_)));
            char buf[16];
            std::snprintf(buf, sizeof buf, "TIME %d", sec);
            hud(1, 26, "ARROWS STEP", PAL_INK);
            hud(28, 26, buf, sec < 5 ? PAL_BAD : PAL_IVORY);
        }

        spr(art_.pawn, float(sqX(men_[0].f)), float(sqY(men_[0].r)) - 2, men_[0].alive ? 30 : 18,
            men_[0].alive ? PAL_IVORY : PAL_BAD, men_[0].alive ? 0 : 6);
        for (int i = 1; i < 4; i++) {
            if (!men_[i].alive) continue;
            spr(art_.foe, float(sqX(men_[i].f)), float(sqY(men_[i].r)) - 2, 28, PAL_NIGHT);
        }
        spr(art_.mark, float(sqX(MARK_FILE)), float(sqY(0)), finished_ ? 22 : 16, PAL_GOLD);

        for (int r = 0; r < RANKS; r++) {
            for (int f = 0; f < FILES; f++) {
                bool dark = ((f + r) & 1) != 0;
                bool mark = f == MARK_FILE && r == 0;
                int pal = mark ? PAL_GOLD : (dark ? PAL_WOOD : PAL_FELT);
                float pulse = mark && !finished_ ? 1.0f + 0.06f * std::sin(float(sys_->frame) * 0.14f) : 1.0f;
                spr(art_.square, float(sqX(f)), float(sqY(r)), float(SQ - 2) * pulse, pal, mark ? 0 : 2);
            }
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.08f, 0.18f, 0.1f);
    sys.apu.setPatch(0, bell());
    sys.apu.setPatch(1, bell());
    sys.apu.setPatch(2, bell());
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            begin();
            blip(true);
        }
    } else if (mode_ == Mode::Brief) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Play;
            t_ = 0;
            blip(true);
        }
    } else if (mode_ == Mode::Play) {
        play(DT);
    } else if (mode_ == Mode::Done) {
        hold_ -= DT;
        if (hold_ <= 0) {
            won_ = true;
            over_ = true;
        }
    } else if (mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            over_ = false;
            won_ = false;
            mode_ = Mode::Title;
        }
    }

    draw();
}

}  // namespace pawnmark
