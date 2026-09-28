#include "game/safetape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace safetape {

bool Game::matched() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return false;
    return true;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kTape[i].name;
}

int Game::tapeDigit(int i) const {
    if (i < 0 || i >= kTapeN) return -1;
    return kTape[i].digit;
}

int Game::nearDigit(int i) const {
    if (i < 0 || i >= kTapeN) return -1;
    return kNear[i];
}

int Game::drawerSum() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) {
        if (!filed_[i] || drawer_[i] < 0) return -1;
        s += drawer_[i];
    }
    return s;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Dial) return matched() ? 2 : 1;
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) return 3;
    return 4;
}

bool Game::prove() {
    rules_ = false;
    reason_ = "RULES";
    if (std::strcmp(kTape[0].name, "LEFT") != 0 || std::strcmp(kTape[1].name, "SPINE") != 0 ||
        std::strcmp(kTape[2].name, "RIGHT") != 0)
        return false;
    int tapeSum = 0, nearSum = 0;
    bool same = true;
    int seen[10] = {};
    int seenNear[10] = {};
    for (int i = 0; i < kTapeN; i++) {
        int d = kTape[i].digit;
        int n = kNear[i];
        if (d < 0 || d > 9 || n < 0 || n > 9) return false;
        tapeSum += d;
        nearSum += n;
        if (d != n) same = false;
        seen[d]++;
        seenNear[n]++;
    }
    // The close plate must add up like the tape, and must not be the tape.
    if (tapeSum != 13 || nearSum != tapeSum || same) {
        reason_ = "SUM";
        return false;
    }
    bool multiset = true;
    for (int d = 0; d < 10; d++)
        if (seen[d] != seenNear[d]) multiset = false;
    // A shuffled copy would be a different trap. This plate is a same-sum stranger.
    if (multiset) {
        reason_ = "NEAR IS A SHUFFLE";
        return false;
    }
    auto holds = [](const int* d) {
        for (int i = 0; i < kTapeN; i++)
            if (d[i] != kTape[i].digit) return false;
        return true;
    };
    int tape[kTapeN], turned[kTapeN];
    for (int i = 0; i < kTapeN; i++) tape[i] = kTape[i].digit;
    turned[0] = tape[1];
    turned[1] = tape[2];
    turned[2] = tape[0];
    if (!holds(tape) || holds(kNear) || holds(turned)) {
        reason_ = "CLOSE COUNTS";
        return false;
    }
    rules_ = true;
    reason_ = "SHUT";
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    left_ = false;
    nearRejected_ = false;
    botNear_ = true;
    t_ = 0;
    walk_ = 0;
    if (!prove()) {
        reason_ = rules_ ? reason_ : (reason_ ? reason_ : "RULES");
        rules_ = false;
    }
}

void Game::begin() {
    for (int i = 0; i < kTapeN; i++) {
        dial_[i] = 0;
        drawer_[i] = -1;
        filed_[i] = false;
        held_[i] = false;
    }
    sel_ = 0;
    botNear_ = true;
    nearRejected_ = false;
    left_ = false;
    won_ = false;
    over_ = false;
    walk_ = 0;
    shake_ = 0;
    wrong_ = 0;
    reason_ = "SHUT";
    mode_ = Mode::Dial;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    beep_ = 4;
}

void Game::nudgeSel(int d) {
    sel_ = (sel_ + d + kTapeN) % kTapeN;
    blip(480);
}

void Game::nudgeDial(int d) {
    dial_[sel_] = (dial_[sel_] + d + 10) % 10;
    blip(160.0f + dial_[sel_] * 18.0f);
}

void Game::file() {
    drawer_[sel_] = dial_[sel_];
    filed_[sel_] = true;
    held_[sel_] = drawer_[sel_] == kTape[sel_].digit;
    blip(held_[sel_] ? 740.0f : 220.0f);
    if (!held_[sel_]) {
        wrong_ = 28;
        shake_ = 8;
        sys_->apu.noiseBurst(0.25f, 180, 10);
    }
    bool allFiled = true;
    bool nearNow = true;
    for (int i = 0; i < kTapeN; i++) {
        if (!filed_[i]) allFiled = false;
        if (drawer_[i] != kNear[i]) nearNow = false;
    }
    if (allFiled && nearNow && !matched()) nearRejected_ = true;
    if (allFiled && !matched()) reason_ = drawerSum() == 13 ? "A CLOSE SUM IS STILL SHUT" : "DOES NOT MATCH";
    if (matched()) reason_ = "THE TAPE IS MET";
}

void Game::clearSel() {
    drawer_[sel_] = -1;
    filed_[sel_] = false;
    held_[sel_] = false;
    reason_ = "SHUT";
    blip(300);
}

void Game::beginLeave() {
    if (!matched() || !rules_) return;
    mode_ = Mode::Leave;
    left_ = false;
    walk_ = 0;
    reason_ = "LEAVE";
    sys_->apu.tone(1, 523, 0.08f);
}

void Game::beginLose(const char* why) {
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    left_ = false;
    reason_ = why;
}

void Game::botAct() {
    if (!rules_) {
        beginLose("RULES");
        return;
    }
    if (mode_ == Mode::Title) {
        if (t_ > 16) begin();
        return;
    }
    if (mode_ == Mode::Leave || mode_ == Mode::Lose || mode_ == Mode::Over) return;
    const int* goal = botNear_ ? kNear : nullptr;
    int want = goal ? goal[sel_] : kTape[sel_].digit;
    if (dial_[sel_] != want) {
        int up = (want - dial_[sel_] + 10) % 10;
        int dn = (dial_[sel_] - want + 10) % 10;
        nudgeDial(up <= dn ? 1 : -1);
        return;
    }
    if (!filed_[sel_] || drawer_[sel_] != want) {
        file();
        return;
    }
    if (sel_ < kTapeN - 1) {
        nudgeSel(1);
        return;
    }
    if (botNear_) {
        if (matched() || !nearRejected_) {
            beginLose(matched() ? "NEAR OPENED" : "NEAR WAS NOT FILED");
            return;
        }
        botNear_ = false;
        sel_ = 0;
        return;
    }
    if (!matched()) {
        beginLose("TAPE MISSED");
        return;
    }
    beginLeave();
}

void Game::human() {
    if (mode_ == Mode::Title || mode_ == Mode::Lose || mode_ == Mode::Over) {
        if (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A)) {
            if (mode_ == Mode::Title && rules_) begin();
        }
        return;
    }
    if (mode_ == Mode::Leave) return;
    if (matched() && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_B))) {
        beginLeave();
        return;
    }
    auto tap = [&](gs::Button b, int slot, auto&& fn) {
        int& h = hold_[slot];
        if (sys_->pad.pressed(b)) {
            h = 0;
            fn();
        } else if (sys_->pad.down(b)) {
            if (++h >= 14 && (h % 5) == 0) fn();
        } else {
            h = 0;
        }
    };
    tap(gs::BTN_LEFT, 0, [&] { nudgeSel(-1); });
    tap(gs::BTN_RIGHT, 1, [&] { nudgeSel(1); });
    tap(gs::BTN_UP, 2, [&] { nudgeDial(1); });
    tap(gs::BTN_DOWN, 3, [&] { nudgeDial(-1); });
    if (sys_->pad.pressed(gs::BTN_A)) file();
    if (sys_->pad.pressed(gs::BTN_C)) clearSel();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (shake_ > 0) shake_--;
    if (wrong_ > 0) wrong_--;

    if (mode_ == Mode::Leave) {
        walk_ += 3.2f;
        if (int(t_) % 10 == 0) sys.apu.tone(2, 90, 0.03f);
        if (walk_ >= 210.f) {
            left_ = true;
            bool drawerOk = true;
            for (int i = 0; i < kTapeN; i++)
                if (drawer_[i] != kTape[i].digit) drawerOk = false;
            won_ = matched() && left_ && rules_ && drawerOk && drawerSum() == 13;
            over_ = true;
            mode_ = Mode::Over;
            reason_ = won_ ? "LEAVE" : "DOES NOT MATCH";
            if (won_) sys.setLight(80, 220, 120);
        }
    }

    if (bot_) botAct();
    else human();

    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::clamp(std::lround(w), 1L, 2000L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::box(float x, float y, float w, float h, int pal) {
    if (w < 1 || h < 1) return;
    gs::Sprite s{};
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = art_.solid.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float jx = (shake_ > 0) ? ((shake_ & 1) ? 2.0f : -2.0f) : 0.0f;
    float doorSlide = (matched() && mode_ != Mode::Title) ? -18.f : 0.f;

    if (mode_ != Mode::Title) {
        spr(art_.door, art_.doorX + art_.doorW * 0.5f + jx, art_.doorY + art_.doorH * 0.5f + doorSlide, art_.doorH,
            PAL_DOOR);
        int pal = (wrong_ > 0 && (wrong_ & 4)) ? PAL_RED : PAL_AMBER;
        if (mode_ == Mode::Dial) spr(art_.caret, art_.dialX[sel_] + jx, art_.dialY - 16, 7, pal);
        for (int i = 0; i < kTapeN; i++) {
            spr(art_.digit[dial_[i]], art_.dialX[i] + jx, art_.dialY, 16, PAL_WHEEL);
            spr(art_.digit[kTape[i].digit], art_.tapeX[i], art_.tapeY[i], 14, PAL_INK);
            spr(art_.digit[kNear[i]], art_.nearX[i], art_.nearY, 12, PAL_TAPE);
            if (filed_[i] && drawer_[i] >= 0) {
                int dp = held_[i] ? PAL_GREEN : PAL_RED;
                spr(art_.digit[drawer_[i]], art_.drawerX[i], art_.drawerY, 12, dp);
            }
        }
    }

    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        float x = 36.f + walk_;
        if (x > 300.f) x = 300.f;
        float bob = (int(walk_) / 8) & 1 ? 1.f : 0.f;
        spr(art_.walker, x, 178.f + bob, 26, PAL_WALK);
    }

    if (mode_ == Mode::Title) {
        box(36, 58, 248, 100, PAL_SHADE);
        hudC(9, "S3 SAFETAPE", PAL_AMBER);
        hudC(11, "A SHORT SAFE", PAL_TEXT);
        hudC(13, "THE DRAWER HAS TO MATCH THE TAPE", PAL_AMBER);
        hudC(15, "LEFT  SPINE  RIGHT", PAL_TEXT);
        hudC(17, "A CLOSE SUM IS STILL SHUT", PAL_TEXT);
        hudC(19, "ARROWS TURN   A FILES   B LEAVES", PAL_TEXT);
        if ((t_ / 30) % 2 == 0) hudC(21, "PRESS START", PAL_AMBER);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 8, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "TAPE", PAL_AMBER);
        hudC(25, "THE DRAWER MATCHES THE TAPE", PAL_GREEN);
        hudC(26, "YOU LEAVE", PAL_GREEN);
    } else if (mode_ == Mode::Lose) {
        hudC(25, reason_, PAL_RED);
    } else if (mode_ == Mode::Leave) {
        hudC(1, "TAPE", PAL_AMBER);
        hudC(25, "THE DRAWER MATCHES THE TAPE", PAL_GREEN);
        hudC(26, "WALK OUT", PAL_AMBER);
    } else {
        hud(1, 1, "TAPE", PAL_AMBER);
        hud(30, 1, "NEAR", PAL_RED);
        hudC(24, kTape[sel_].name, held_[sel_] ? PAL_GREEN : PAL_AMBER);
        hudC(26, reason_, matched() ? PAL_GREEN : (wrong_ > 0 ? PAL_RED : PAL_TEXT));
    }
}

}  // namespace safetape
