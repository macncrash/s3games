#include "game/chefgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace chefgold {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int PANS = 3;
constexpr int ORDERS = 8;
constexpr float PX[PANS] = {64.f, 160.f, 256.f};
constexpr float GOLD_LO = 0.62f;
constexpr float GOLD_HI = 0.84f;
constexpr float GOLD_COOK = 5.6f;
constexpr float CREAM_COOK = 2.6f;
constexpr float DROP_GAP = 1.05f;

// Six gold plates. Cream is face value and is left on the pass.
constexpr int GOLD_ORDER[ORDERS] = {1, 0, 1, 1, 0, 1, 1, 1};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Win) return 4;
    if (mode_ == Mode::Lose) return 3;
    if (golds_ >= 3) return 3;
    if (golds_ >= 1) return 2;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::enterTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    goldOut_ = false;
    score_ = bare_ = golds_ = cream_ = 0;
    reason_[0] = 0;
    sel_ = 1;
    for (int i = 0; i < PANS; i++) pans_[i] = {};
    sys_->setLight(180, 90, 30);
}

void Game::beginService() {
    score_ = bare_ = golds_ = cream_ = 0;
    goldOut_ = false;
    won_ = false;
    over_ = false;
    next_ = 0;
    sel_ = 1;
    holdDir_ = 0;
    hold_ = 0;
    dropCd_ = 0;
    chefX_ = PX[1];
    reason_[0] = 0;
    for (int i = 0; i < PANS; i++) pans_[i] = {};
    mode_ = Mode::Play;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildPictures(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.apu.setMaster(0.85f);
    if (bot_) beginService();
    else enterTitle();
}

void Game::nudge(int dir) {
    int n = std::clamp(sel_ + dir, 0, PANS - 1);
    if (n == sel_) return;
    sel_ = n;
    sys_->apu.tone(0, 640.f, 0.03f);
}

void Game::humanAct(float dt) {
    const gs::Pad& pad = sys_->pad;
    int dir = 0;
    if (pad.down(gs::BTN_LEFT) || pad.axisX < -0.45f) dir = -1;
    else if (pad.down(gs::BTN_RIGHT) || pad.axisX > 0.45f) dir = 1;
    if (dir == 0) {
        hold_ = 0;
        holdDir_ = 0;
    } else if (dir != holdDir_) {
        nudge(dir);
        holdDir_ = dir;
        hold_ = 0;
    } else {
        hold_ += dt;
        if (hold_ > 0.18f) {
            nudge(dir);
            hold_ = 0.08f;
        }
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_Z) ||
        pad.pressed(gs::BTN_TURBO))
        plate(sel_);
}

void Game::botAct() {
    // Plate only inside the gold. Cream is left so it cannot buy the line.
    int urgent = -1;
    float best = -1.f;
    for (int i = 0; i < PANS; i++) {
        const Pan& p = pans_[i];
        if (p.order < 0 || !p.gold) continue;
        if (p.heat < GOLD_LO || p.heat > GOLD_HI) continue;
        if (p.heat > best) {
            best = p.heat;
            urgent = i;
        }
    }
    if (urgent < 0) return;
    if (sel_ < urgent) sel_++;
    else if (sel_ > urgent) sel_--;
    else plate(urgent);
}

void Game::fail(const char* why) {
    std::snprintf(reason_, sizeof reason_, "%s", why);
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    sys_->apu.noiseBurst(0.4f, 380.f, 0.35f);
    sys_->setLight(170, 24, 18);
}

void Game::leaveIf() {
    // Only the gold counts double. Cream face points cannot buy the line.
    if (goldOut_ && golds_ > 0 && cream_ == 0 && score_ >= kLine && bare_ < kLine) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        sys_->apu.tone(1, 880.f, 0.08f);
        sys_->setLight(255, 200, 80);
    }
}

bool Game::plate(int pan) {
    if (pan < 0 || pan >= PANS || mode_ != Mode::Play) return false;
    Pan& p = pans_[pan];
    if (p.order < 0) return false;
    if (!p.gold) {
        // Cream plated off the gold scores its face. It does not double.
        cream_++;
        score_ += 1;
        bare_ += 1;
        p = {};
        sys_->apu.tone(1, 320.f, 0.05f);
        return true;
    }
    if (p.heat < GOLD_LO) {
        sys_->apu.tone(0, 140.f, 0.05f);
        return false;
    }
    if (p.heat > GOLD_HI) {
        fail("LATE GOLD");
        return false;
    }
    score_ += 2;
    bare_ += 1;
    golds_++;
    goldOut_ = true;
    p = {};
    sys_->apu.tone(1, 988.f, 0.08f);
    sys_->apu.noiseBurst(0.12f, 2400.f, 0.03f);
    if (!sys_->headless) sys_->rumble(0.12f, 0.3f, 60);
    leaveIf();
    return true;
}

void Game::tryDrop() {
    if (next_ >= ORDERS || dropCd_ > 0 || mode_ != Mode::Play) return;
    int slot = -1;
    for (int i = 0; i < PANS; i++)
        if (pans_[i].order < 0) {
            slot = i;
            break;
        }
    if (slot < 0) return;
    pans_[slot].order = next_;
    pans_[slot].gold = GOLD_ORDER[next_] != 0;
    pans_[slot].heat = 0;
    next_++;
    dropCd_ = DROP_GAP;
}

void Game::advance(float dt) {
    for (int i = 0; i < PANS; i++) {
        Pan& p = pans_[i];
        if (p.order < 0) continue;
        if (p.gold) {
            p.heat += dt / GOLD_COOK;
            if (p.heat > GOLD_HI) {
                fail("PASSED THE GOLD");
                return;
            }
        } else {
            p.heat += dt / CREAM_COOK;
            if (p.heat >= 1.f) p = {};  // cream left the pass, unplated
        }
    }
    if (mode_ != Mode::Play) return;
    if (dropCd_ > 0) dropCd_ -= dt;
    tryDrop();
    bool busy = next_ < ORDERS;
    for (int i = 0; i < PANS; i++)
        if (pans_[i].order >= 0) busy = true;
    if (!busy && !won_) fail(cream_ > 0 ? "CREAM PLATED" : "SHORT");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z)) beginService();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            if (bot_) botAct();
            else humanAct(DT);
            if (mode_ == Mode::Play) advance(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    } else if (!bot_ && (mode_ == Mode::Win || mode_ == Mode::Lose)) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) beginService();
        else if (pad.pressed(gs::BTN_MODE)) enterTitle();
    }

    float goal = PX[std::clamp(sel_, 0, PANS - 1)];
    chefX_ += (goal - chefX_) * std::min(1.f, DT * 10.f);
    if (mode_ == Mode::Play) {
        bool go = false;
        for (int i = 0; i < PANS; i++)
            if (pans_[i].gold && pans_[i].heat >= GOLD_LO) go = true;
        sys.setLight(go ? 230 : 150, go ? 160 : 70, 30);
    }
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid.lv[0];
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 28) c = gs::rgb4(5, 2, 1);
        else if (y < 150) c = gs::rgb4(12, 8, 6);
        else c = gs::rgb4(4, 4, 5);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.clear();
    backdrop();

    const bool service = mode_ != Mode::Title;
    int flick = int(anim_ * 10.f) % 3;
    for (int i = 0; i < PANS; i++) {
        float x = PX[i];
        spr(art_.pan, x, 156, 22, PAL_STEEL);
        spr(art_.flame, x, 172 - float(flick == i), 16, PAL_FIRE);
        float heat = 0;
        bool show = false;
        bool gold = true;
        if (!service) {
            show = true;
            heat = 0.35f + 0.45f * (0.5f + 0.5f * std::sin(anim_ * 1.3f + float(i)));
            gold = i != 1;
        } else if (pans_[i].order >= 0) {
            show = true;
            heat = pans_[i].heat;
            gold = pans_[i].gold;
        }
        if (show) {
            spr(gold ? art_.goldDish : art_.creamDish, x, 118, 28, PAL_FOOD);
            spr(art_.ticket, x, 78, 22, gold ? PAL_GOLD : PAL_PAPER);
            float bw = 56.f;
            float bx = x - bw * 0.5f;
            solid(bx, 96, bw * std::clamp(heat, 0.f, 1.f), 6, heat >= GOLD_LO && heat <= GOLD_HI ? PAL_GOLD : PAL_ALERT);
            solid(bx + bw * GOLD_LO, 94, bw * (GOLD_HI - GOLD_LO), 2, PAL_GOLD);
        }
    }
    float bob = std::sin(anim_ * 6.f) * 1.2f;
    spr(art_.chef, chefX_, 196 + bob, 46, PAL_CHEF);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 CHEF GOLD", PAL_GOLD);
        hudC(4, "ONLY THE GOLD COUNTS DOUBLE", PAL_HUD);
        hudC(24, "ARROWS MOVE   Z PLATES THE GOLD", PAL_HUD);
        hudC(25, "LEAVE THE CREAM", PAL_PAPER);
        if ((int(anim_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
    } else {
        hud(1, 0, "S3 CHEF GOLD", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(40 - int(std::strlen(buf)), 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "GOLD %d  CREAM %d  LINE %d", golds_, cream_, kLine);
        hudC(1, buf, cream_ ? PAL_ALERT : PAL_HUD);
        std::snprintf(buf, sizeof buf, "BARE %d", bare_);
        hud(1, 2, buf, PAL_PAPER);
        for (int i = 0; i < PANS; i++) {
            if (pans_[i].order < 0) continue;
            int col = int(std::lround(PX[i] / 8.f)) - 2;
            if (!pans_[i].gold) hud(col, 8, "CREAM", PAL_PAPER);
            else if (pans_[i].heat >= GOLD_LO) hud(col, 8, "GOLD", PAL_GOLD);
            else hud(col, 8, "COOK", PAL_HUD);
        }
        if (mode_ == Mode::Pause) hudC(14, "PAUSED", PAL_GOLD);
        else if (mode_ == Mode::Win) hudC(14, "LEFT ON THE DOUBLE", PAL_GOLD);
        else if (mode_ == Mode::Lose) {
            hudC(14, reason_[0] ? reason_ : "SHORT", PAL_ALERT);
        } else if (golds_ == 0) hudC(26, "PLATE ONLY WHILE THE BAR IS GOLD", PAL_HUD);
    }
}

}  // namespace chefgold
