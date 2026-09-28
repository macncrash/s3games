#include "game/lanterngold.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lanterngold {
namespace {

constexpr int LEAD = 16;
constexpr int SHOW = 18;
constexpr int GAP = 10;
constexpr int MISS_T = 36;
constexpr int WIN_T = 50;
constexpr float LAMP_H = 48.f;
constexpr float BEAM_Y = 46.f;

const float kX[kLamps] = {36, 88, 140, 192, 244, 292};
const float kDrop[kLamps] = {2, 10, 6, 12, 4, 8};
const float kPitch[kLamps] = {349.23f, 392.00f, 440.00f, 493.88f, 523.25f, 587.33f};
const bool kGoldLamp[kLamps] = {false, true, false, true, false, true};

uint16_t mix(uint16_t a, uint16_t b, float t) {
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto L = [&](int x, int y) { return int(std::lround(x + (y - x) * t)); };
    return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
}

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Watch: return "watch";
    case Mode::Play: return "play";
    case Mode::Miss: return "miss";
    case Mode::Refuse: return "refuse";
    case Mode::Win: return "win";
    }
    return "?";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    finisherGold_ = false;
    step_ = gold_ = cream_ = score_ = handed_ = refused_ = 0;
    cursor_ = 0;
    timer_ = 0;
    lock_ = 0;
    hot_ = -1;
    hotT_ = 0;
    toneT_ = 0;
    show_ = 0;
    mothX_ = 40;
    mothY_ = 70;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
}

bool Game::isGold(int lamp) const { return lamp >= 0 && lamp < kLamps && kGoldLamp[lamp]; }

void Game::quiet() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    toneT_ = 0;
}

void Game::chime(int lamp, float vol) {
    if (lamp < 0 || lamp >= kLamps) return;
    float f = kPitch[lamp];
    if (isGold(lamp)) f *= 1.0f;
    sys_->apu.tone(0, f, vol);
    sys_->apu.tone(1, f * (isGold(lamp) ? 1.5f : 2.f), vol * 0.32f);
    toneT_ = 12;
    if (isGold(lamp)) sys_->setLight(255, 180, 40);
    else sys_->setLight(255, 230, 190);
}

void Game::thud() {
    sys_->apu.tone(0, 90.f, 0.08f);
    sys_->apu.noiseBurst(0.35f, 600.f, 0.18f);
    toneT_ = 14;
    sys_->setLight(90, 20, 20);
    sys_->rumble(0.4f, 0.15f, 80);
}

void Game::begin() {
    step_ = gold_ = cream_ = score_ = handed_ = refused_ = 0;
    won_ = false;
    over_ = false;
    finisherGold_ = false;
    cursor_ = 0;
    hot_ = -1;
    hotT_ = 0;
    hotBad_ = false;
    quiet();
    enterWatch();
}

void Game::enterWatch() {
    mode_ = Mode::Watch;
    show_ = 0;
    timer_ = 0;
    lock_ = 0;
}

void Game::enterPlay() {
    mode_ = Mode::Play;
    timer_ = 0;
    lock_ = 8;
}

void Game::enterMiss() {
    mode_ = Mode::Miss;
    timer_ = 0;
    handed_++;
    hotBad_ = true;
    hotT_ = 18;
    thud();
}

void Game::enterRefuse() {
    mode_ = Mode::Refuse;
    timer_ = 0;
    refused_++;
    hotBad_ = true;
    hotT_ = 18;
    thud();
}

void Game::enterWin() {
    mode_ = Mode::Win;
    timer_ = 0;
    won_ = true;
    finisherGold_ = true;
    sys_->rumble(0.2f, 0.5f, 160);
    sys_->setLight(255, 190, 50);
}

void Game::undoLast() {
    if (step_ <= 0) return;
    int lamp = lit_[step_ - 1];
    step_--;
    if (isGold(lamp)) {
        gold_ = std::max(0, gold_ - 1);
        score_ = std::max(0, score_ - 2);
    } else {
        cream_ = std::max(0, cream_ - 1);
        score_ = std::max(0, score_ - 1);
    }
}

void Game::nudge(int dir) { cursor_ = (cursor_ + dir + kLamps) % kLamps; }

void Game::tryLight(int lamp) {
    if (mode_ != Mode::Play || lock_ > 0) return;
    if (lamp < 0 || lamp >= kLamps) return;
    hot_ = lamp;
    hotT_ = 14;
    int want = order_[step_];
    if (lamp != want) {
        hotBad_ = true;
        enterMiss();
        return;
    }
    hotBad_ = false;
    bool gold = isGold(lamp);
    if (!gold && score_ + 1 >= kLine) {
        enterRefuse();
        return;
    }
    lit_[step_] = lamp;
    step_++;
    if (gold) {
        gold_++;
        score_ += 2;
    } else {
        cream_++;
        score_ += 1;
    }
    chime(lamp, gold ? 0.22f : 0.14f);
    lock_ = 10;
    const bool math = score_ == gold_ * 2 + cream_;
    if (gold && gold_ >= 1 && score_ >= kLine && (gold_ + cream_) < kLine && math) {
        enterWin();
        return;
    }
    if (step_ >= kOrderN) {
        // The taught order is the double. If it did not pay, the night fails closed.
        enterMiss();
    }
}

void Game::readHuman() {
    const gs::Pad& p = sys_->pad;
    if (mode_ == Mode::Title) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) begin();
        return;
    }
    if (mode_ == Mode::Win) {
        if (timer_ <= 0 && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A))) begin();
        return;
    }
    if (mode_ != Mode::Play || lock_ > 0) return;
    if (p.pressed(gs::BTN_LEFT)) nudge(-1);
    if (p.pressed(gs::BTN_RIGHT)) nudge(1);
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Z)) tryLight(cursor_);
    if (p.pressed(gs::BTN_X)) tryLight(0);
    if (p.pressed(gs::BTN_Y)) tryLight(1);
    if (p.pressed(gs::BTN_B)) tryLight(2);
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (timer_ > 24) begin();
        return;
    }
    if (mode_ != Mode::Play || lock_ > 0 || step_ >= kOrderN) return;
    if ((sys_->frame % 8) != 0) return;
    int want = order_[step_];
    if (cursor_ != want) {
        int cw = (want - cursor_ + kLamps) % kLamps;
        int ccw = (cursor_ - want + kLamps) % kLamps;
        nudge(cw <= ccw ? 1 : -1);
        return;
    }
    tryLight(cursor_);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (toneT_ > 0 && --toneT_ == 0) quiet();
    if (hotT_ > 0) hotT_--;
    if (lock_ > 0) lock_--;
    timer_++;
    if (mode_ == Mode::Watch) {
        int span = SHOW + GAP;
        int t = timer_ - LEAD;
        if (timer_ == LEAD) chime(order_[0], 0.16f);
        if (t >= 0) {
            int s = t / span;
            int u = t % span;
            if (s >= kOrderN) {
                enterPlay();
            } else {
                if (s != show_) {
                    show_ = s;
                    chime(order_[show_], 0.16f);
                }
                if (u == SHOW) quiet();
            }
        }
    } else if (mode_ == Mode::Miss) {
        if (timer_ == 8) undoLast();
        if (timer_ >= MISS_T) enterPlay();
    } else if (mode_ == Mode::Refuse) {
        if (timer_ >= MISS_T) enterPlay();
    } else if (mode_ == Mode::Win) {
        if (timer_ >= WIN_T) over_ = true;
    }

    if (bot_) botAct();
    else readHuman();
    // A person who launched --sim still has the bot. A windowed game reads the pad.
    // Title timer must advance even before begin. frame() already increments timer_.
    draw();
}

bool Game::lampOn(int i, int& pal) const {
    pal = PAL_DARK;
    bool showing = false;
    if (mode_ == Mode::Watch && timer_ >= LEAD) {
        int span = SHOW + GAP;
        int t = timer_ - LEAD;
        int s = t / span;
        int u = t % span;
        if (s >= 0 && s < kOrderN && u < SHOW && order_[s] == i) showing = true;
    }
    bool chain = false;
    int n = step_;
    for (int k = 0; k < n; k++)
        if (lit_[k] == i) chain = true;
    if (showing || chain || (mode_ == Mode::Win)) {
        pal = isGold(i) ? PAL_GOLD : PAL_CREAM;
        if (mode_ == Mode::Win || chain || showing) return true;
    }
    if (mode_ == Mode::Win) {
        pal = isGold(i) ? PAL_GOLD : PAL_CREAM;
        return true;
    }
    return false;
}

void Game::sky() {
    const uint16_t top = gs::rgb4(1, 1, 4);
    const uint16_t mid = gs::rgb4(3, 2, 7);
    const uint16_t hor = gs::rgb4(8, 5, 4);
    const uint16_t ground = gs::rgb4(2, 2, 3);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 88) c = mix(top, mid, y / 88.f);
        else if (y < 150) c = mix(mid, hor, (y - 88) / 62.f);
        else c = mix(hor, ground, std::min(1.f, (y - 150) / 74.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    const uint64_t fr = sys_->frame;

    spr(art_.moon, 270, 28, 28, PAL_SCENE);
    spr(art_.post, 14, 110, 120, PAL_SCENE);
    spr(art_.post, 306, 110, 120, PAL_SCENE);
    spr(art_.beam, 160, BEAM_Y, 8, PAL_SCENE);

    int follow = cursor_;
    if (mode_ == Mode::Watch && timer_ >= LEAD) {
        int span = SHOW + GAP;
        int s = (timer_ - LEAD) / span;
        if (s >= 0 && s < kOrderN) follow = order_[s];
    }
    float tx = kX[std::clamp(follow, 0, kLamps - 1)];
    float nx = mothX_ + (tx - mothX_) * 0.08f;
    faceLeft_ = nx < mothX_ - 0.2f;
    mothX_ = nx;
    mothY_ += (58.f - mothY_) * 0.06f;
    spr(art_.moth[int((fr / 5) & 1)], mothX_, mothY_, 12, PAL_SCENE, faceLeft_);

    for (int i = 0; i < kLamps; i++) {
        float swing = std::sin((float(fr) + i * 17) * 0.045f);
        float x = kX[i] + swing * 2.2f;
        float top = 52.f + kDrop[i];
        float cy = top + LAMP_H * 0.5f;
        float cordH = std::max(4.f, top - (BEAM_Y + 3.f));
        int pal = PAL_DARK;
        bool on = lampOn(i, pal);
        spr(art_.cord, x, BEAM_Y + 2.f + cordH * 0.5f, cordH, PAL_SCENE);
        if (on) spr(art_.flame[int((fr / 6 + i) & 1)], x, cy - 10.f, 12, pal);
        spr(art_.lamp, x, cy, LAMP_H, on ? pal : PAL_DARK);
        if ((mode_ == Mode::Play || mode_ == Mode::Title) && i == cursor_)
            spr(art_.wick, x, top - 6.f, 8, isGold(i) ? PAL_GOLD : PAL_CREAM);
    }

    if (mode_ == Mode::Title) {
        hudC(3, "S3 LANTERN GOLD", PAL_GOLD);
        hudC(18, "LIGHT THEM IN ORDER", PAL_HUD);
        hudC(19, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(21, "ARROWS MOVE   A LIGHTS", PAL_DIM);
        hudC(22, "CREAM IS ONE   GOLD IS TWO", PAL_DIM);
        hudC(24, "ENTER BEGINS THE NIGHT", PAL_HUD);
        hudC(26, S3_VERSION_STRING, PAL_DIM);
    } else {
        char line[48];
        std::snprintf(line, sizeof line, "GOLD %d  CREAM %d  SCORE %d", gold_, cream_, score_);
        hud(1, 0, line, PAL_HUD);
        std::snprintf(line, sizeof line, "LINE %d  BARE %d", kLine, gold_ + cream_);
        hud(1, 1, line, PAL_DIM);
        const char* banner = "WATCH THE ORDER";
        int bpal = PAL_HUD;
        if (mode_ == Mode::Play) banner = "YOUR TURN";
        else if (mode_ == Mode::Miss) {
            banner = "HANDED BACK TO THE DARK";
            bpal = PAL_ROSE;
        } else if (mode_ == Mode::Refuse) {
            banner = "CREAM DOES NOT FINISH";
            bpal = PAL_ROSE;
        } else if (mode_ == Mode::Win) {
            banner = "ONLY THE GOLD COUNTS DOUBLE";
            bpal = PAL_GOLD;
        }
        hudC(25, banner, bpal);
        if (mode_ == Mode::Win) hudC(26, "THE DOUBLE IS IN", PAL_JADE);
        else hudC(26, "A GOLD LAMP IS TWO", PAL_DIM);
    }
}

}  // namespace lanterngold
