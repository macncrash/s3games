#include "game/horngold.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace horngold {
namespace {
constexpr int kWindowLo = 12;
constexpr int kWindowHi = 20;
constexpr int kMissAt = 34;
constexpr float kPitch[kCalls] = {392.f, 262.f, 330.f, 392.f, 294.f, 523.f};

static_assert(kGolds * kGoldFace * 2 + kCreams * kCreamFace >= kLine, "doubled gold clears the line");
static_assert(kGolds * kGoldFace + kCreams * kCreamFace < kLine, "bare faces stay short");
static_assert(kGolds * kGoldFace * 2 < kLine, "gold alone does not buy the line");
static_assert((kGolds - 1) * kGoldFace * 2 + kCreams * kCreamFace < kLine, "leave on the last gold");
static_assert(kCalls == kGolds + kCreams, "the short horn is six calls");
}  // namespace

bool Game::open() const {
    const int face = golds_ * kGoldFace * 2 + cream_ * kCreamFace;
    return !spoiled_ && finisherGold_ && calls_ == kCalls && golds_ == kGolds && cream_ == kCreams && score_ == face &&
           bare_ < kLine && score_ >= kLine;
}

void Game::begin() {
    score_ = bare_ = golds_ = cream_ = calls_ = 0;
    breath_ = anim_ = 0;
    over_ = won_ = finisherGold_ = spoiled_ = false;
    why_ = "";
    mode_ = Mode::Breath;
    sys_->apu.silence();
}

void Game::sound(bool hit) {
    const bool gold = callGold(calls_);
    const float hz = kPitch[calls_ < kCalls ? calls_ : kCalls - 1];
    if (hit) {
        if (gold) {
            bare_ += kGoldFace;
            score_ += kGoldFace * 2;
            golds_++;
            finisherGold_ = true;
            sys_->apu.keyOn(0, hz, 0.34f);
            sys_->apu.keyOn(1, hz * 2.f, 0.12f);
        } else {
            bare_ += kCreamFace;
            score_ += kCreamFace;
            cream_++;
            finisherGold_ = false;
            sys_->apu.keyOn(0, hz, 0.2f);
        }
    } else {
        spoiled_ = true;
        finisherGold_ = false;
        why_ = "the breath missed the call";
        sys_->apu.noiseBurst(0.2f, 80.f, 0.14f);
    }
    calls_++;
    anim_ = 0;
    mode_ = Mode::Sound;
}

void Game::leave() {
    won_ = open();
    over_ = true;
    mode_ = Mode::Over;
    if (!won_ && why_[0] == 0) why_ = "the double was not the leave";
    if (won_) {
        why_ = "only the gold counts double";
        sys_->apu.keyOn(0, 523.f, 0.28f);
        sys_->apu.keyOn(1, 659.f, 0.2f);
        sys_->apu.keyOn(2, 784.f, 0.14f);
    } else {
        sys_->apu.noiseBurst(0.24f, 60.f, 0.18f);
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool hflip) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(4, 2, 2);
    uint16_t bot = gs::rgb4(2, 2, 1);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(10, 7, 2);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(5, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    int show = calls_ < kCalls ? calls_ : kCalls - 1;
    if (mode_ == Mode::Title) show = int(sys_->frame / 20) % kCalls;
    const bool gold = callGold(show);
    const int hornPal = gold ? PAL_GOLD : PAL_CREAM;

    float lift = 0.f;
    if (mode_ == Mode::Breath) {
        float t = breath_ / float(kMissAt);
        lift = t;
        if (breath_ >= kWindowLo && breath_ <= kWindowHi) lift = 0.72f + 0.28f * std::sin((breath_ - kWindowLo) * 0.8f);
    } else if (mode_ == Mode::Sound) {
        lift = 0.85f - anim_ / 28.f;
        if (lift < 0.f) lift = 0.f;
    } else if (mode_ == Mode::Title) {
        lift = 0.35f + 0.15f * std::sin(sys_->frame * 0.06f);
    }

    const float hx = 118.f;
    const float hy = 118.f - lift * 28.f;
    spr(art_.coach, 250.f, 132.f, PAL_YARD);
    spr(art_.lamp, 228.f, 96.f, PAL_LAMP);
    spr(art_.player, 62.f, 140.f, PAL_COAT);
    spr(art_.horn, hx, hy, hornPal);
    spr(art_.bell, hx + 52.f, hy, hornPal);
    spr(art_.rail, 168.f, 58.f, PAL_YARD);

    for (int i = 0; i < kCalls; i++) {
        const bool lit = (mode_ != Mode::Title && i < calls_) || (mode_ == Mode::Title && i == show) ||
                         (mode_ != Mode::Title && i == show && mode_ != Mode::Over);
        int pal = callGold(i) ? PAL_GOLD : PAL_CREAM;
        if (!lit) pal = PAL_DIM;
        spr(art_.note, 80.f + i * 32.f, 48.f, pal);
    }

    if (mode_ == Mode::Breath) {
        int filled = breath_ * 36 / kMissAt;
        if (filled < 1) filled = 1;
        if (filled > 36) filled = 36;
        gs::Sprite bar;
        bar.img = art_.breath;
        bar.w = 8;
        bar.h = int16_t(filled);
        bar.x = 292;
        bar.y = int16_t(150 - filled);
        bar.pal = (breath_ >= kWindowLo && breath_ <= kWindowHi) ? PAL_GOLD : PAL_BRASS;
        v.sprite(bar);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 HORN GOLD", PAL_GOLD);
        hudC(22, "A SHORT HORN", PAL_HUD);
        hudC(23, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(24, "CREAM KEEPS ITS FACE", PAL_CREAM);
        hudC(25, "LEAVE WHEN THAT IS TRUE", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(1, "LEFT ON THE DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(23, buf, PAL_GOLD);
        hudC(24, "ONLY THE GOLD COUNTED DOUBLE", PAL_HUD);
        hudC(25, "THE SHORT HORN IS DONE", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        hudC(1, "STILL AT THE HORN", PAL_BAD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(23, buf, PAL_BAD);
        hudC(24, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BARE %d", bare_);
        hud(12, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "LINE %d", kLine);
        hud(28, 1, buf, PAL_DIM);
        std::snprintf(buf, sizeof buf, "CALL %d/%d", show + 1, kCalls);
        hudC(21, buf, gold ? PAL_GOLD : PAL_CREAM);
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(22, buf, PAL_HUD);
        if (mode_ == Mode::Pause) hudC(24, "PAUSED", PAL_GOLD);
        else if (open() || (calls_ == kCalls && !spoiled_)) hudC(24, "LEAVE  THE GOLD IS DOUBLE", PAL_GOLD);
        else if (gold) hudC(24, "SOUND THE GOLD", PAL_GOLD);
        else hudC(24, "CREAM IS NOT A DOUBLE", PAL_CREAM);
        hudC(26, "C SOUND  A LEAVE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Breath) {
        if (calls_ >= kCalls) {
            // The short call is sounded. Wait for the leave.
        } else if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Breath;
            mode_ = Mode::Pause;
        } else {
            const bool tap = bot_ ? (breath_ == (kWindowLo + kWindowHi) / 2) : pad.pressed(gs::BTN_C);
            if (tap) sound(breath_ >= kWindowLo && breath_ <= kWindowHi);
            else if (++breath_ >= kMissAt) sound(false);
        }
    } else if (mode_ == Mode::Sound) {
        anim_++;
        if (anim_ >= 12) {
            breath_ = 0;
            anim_ = 0;
            mode_ = Mode::Rest;
        }
    } else if (mode_ == Mode::Rest) {
        anim_++;
        if (anim_ >= 10) {
            anim_ = 0;
            if (calls_ >= kCalls) {
                if (bot_) leave();
                else mode_ = Mode::Breath;
            } else {
                mode_ = Mode::Breath;
            }
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (mode_ == Mode::Breath && calls_ >= kCalls) {
        if (bot_ || pad.pressed(gs::BTN_A)) leave();
    } else if (!bot_ && (mode_ == Mode::Breath || mode_ == Mode::Rest || mode_ == Mode::Sound) &&
               pad.pressed(gs::BTN_A)) {
        leave();
    }

    draw();
}

}  // namespace horngold
