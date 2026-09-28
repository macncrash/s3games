#include "game/lanternbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lanternbell {
namespace {
constexpr float kDt = 1.f / 60.f;
}

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Watch: return "watch";
    case Mode::Play: return "play";
    case Mode::Dead: return "dead";
    case Mode::Ring: return "ring";
    case Mode::Leave: return "leave";
    case Mode::Over: return "over";
    }
    return "?";
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.4f);
    begin();
    audit();
}

void Game::begin() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    why_ = "";
    hold_ = 0;
    cursor_ = 0;
    step_ = 0;
    clock_ = 0.f;
    bellAmp_ = 0.2f;
    for (int i = 0; i < kLamps; i++) lit_[i] = false;
    mode_ = Mode::Title;
}

void Game::audit() {
    bool endsOnBell = kOrder[kOrderN - 1] == kBell;
    bool distinct = kOrder[0] != kOrder[1] && kOrder[1] != kOrder[2] && kOrder[0] != kOrder[2];
    bool inRange = true;
    for (int i = 0; i < kOrderN; i++)
        if (kOrder[i] < 0 || kOrder[i] >= kLamps) inRange = false;
    rules_ = endsOnBell && distinct && inRange && kOrderN == 3;
    if (!rules_) why_ = "rules";
}

void Game::enterWatch() {
    mode_ = Mode::Watch;
    show_ = 0;
    timer_ = 0;
    step_ = 0;
    cursor_ = 0;
    for (int i = 0; i < kLamps; i++) lit_[i] = false;
    tryNo_ = dead_ + 1;
}

void Game::enterPlay() {
    mode_ = Mode::Play;
    step_ = 0;
    timer_ = 0;
    botWait_ = 0;
    cursor_ = 0;
    for (int i = 0; i < kLamps; i++) lit_[i] = false;
}

void Game::light(int lamp) {
    if (mode_ != Mode::Play || lamp < 0 || lamp >= kLamps) return;
    cursor_ = lamp;
    if (lamp != kOrder[step_]) {
        dieTry("MISS");
        return;
    }
    lit_[lamp] = true;
    step_++;
    if (sys_) {
        float f = 392.f * std::pow(2.f, float(step_) / 6.f);
        sys_->apu.tone(0, f, 0.1f);
    }
    if (step_ >= kOrderN) ring();
}

void Game::ring() {
    if (rung_) return;
    rung_ = true;
    won_ = true;
    why_ = "";
    hold_ = 0;
    bellAmp_ = 1.f;
    mode_ = Mode::Ring;
    lit_[kBell] = true;
    if (sys_) {
        sys_->apu.tone(0, 740.f, 0.12f);
        sys_->apu.tone(1, 1110.f, 0.08f);
        if (!sys_->headless) sys_->rumble(0.3f, 0.6f, 140);
    }
}

void Game::dieTry(const char* why) {
    if (rung_) return;
    dead_++;
    why_ = why;
    for (int i = 0; i < kLamps; i++) lit_[i] = false;
    if (sys_) sys_->apu.tone(0, 98.f, 0.08f);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        if (sys_) sys_->apu.tone(1, 55.f, 0.06f);
        return;
    }
    hold_ = 0;
    mode_ = Mode::Dead;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    bellPh_ += kDt * (rung_ ? 13.f : 1.6f);
    if (rung_) bellAmp_ = std::max(0.22f, bellAmp_ - kDt * 0.35f);

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    if (bot_) start = mode_ == Mode::Title && clock_ > 0.3f;

    if (mode_ == Mode::Title) {
        if (start) enterWatch();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Watch) {
        timer_++;
        int slot = timer_ / 22;
        int phase = timer_ % 22;
        if (slot >= kOrderN) {
            enterPlay();
        } else {
            show_ = (phase < 14) ? kOrder[slot] : -1;
        }
    } else if (mode_ == Mode::Play) {
        timer_++;
        if (bot_) {
            if (!rules_) dieTry("rules");
            else if (++botWait_ >= 8) {
                botWait_ = 0;
                light(kOrder[step_]);
            }
        } else {
            if (pad.pressed(gs::BTN_LEFT)) cursor_ = (cursor_ + kLamps - 1) % kLamps;
            if (pad.pressed(gs::BTN_RIGHT)) cursor_ = (cursor_ + 1) % kLamps;
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) light(cursor_);
            else if (timer_ > 60 * 5) dieTry("DARK");
        }
    } else if (mode_ == Mode::Dead) {
        hold_++;
        if (hold_ > 40) enterWatch();
    } else if (mode_ == Mode::Ring) {
        hold_++;
        if (hold_ == 18 && sys_) sys_->apu.tone(0, 880.f, 0.09f);
        if (hold_ > 50) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        hold_++;
        if (hold_ > 36) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) begin();
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(1, 1, 4);
        if (y > 40) c = gs::rgb4(2, 2, 6);
        if (y > 100) c = gs::rgb4(2, 3, 7);
        if (y > 160) c = gs::rgb4(2, 3, 4);
        if (y > 188) c = gs::rgb4(2, 2, 3);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.moon, 268.f, 36.f, float(art_.moon.w), float(art_.moon.h), PAL_MOON);
    const float stars[][2] = {{24, 28}, {60, 48}, {110, 22}, {190, 40}, {220, 18}, {300, 70}};
    for (auto s : stars) spr(art_.star, s[0], s[1], 3.f, 3.f, PAL_MOON);

    for (int i = 0; i < kLamps; i++) {
        float x = kPostX[i];
        spr(art_.post, x, kGround - 34.f, float(art_.post.w), float(art_.post.h), PAL_YARD);
        spr(art_.lamp, x, kGround - 78.f, float(art_.lamp.w), float(art_.lamp.h), PAL_LAMP);
        bool on = lit_[i] || (mode_ == Mode::Watch && show_ == i);
        if (on) {
            float bob = (i == show_ || lit_[i]) ? std::sin(clock_ * 10.f + i) * 1.2f : 0.f;
            spr(art_.flame, x, kGround - 86.f + bob, float(art_.flame.w), float(art_.flame.h), PAL_FLAME);
        }
    }

    float swing = std::sin(bellPh_) * (rung_ ? 12.f * bellAmp_ : 1.6f);
    float bx = kPostX[kBell] + swing;
    float by = kGround - 118.f;
    spr(art_.clapper, bx + swing * 0.15f, by + 6.f, 5.f, 8.f, PAL_BELL);
    spr(art_.bell, bx, by, float(art_.bell.w), float(art_.bell.h), PAL_BELL);

    if (mode_ == Mode::Play || mode_ == Mode::Watch || mode_ == Mode::Title) {
        float cx = kPostX[std::clamp(cursor_, 0, kLamps - 1)];
        spr(art_.carry, cx + 16.f, kGround - 28.f, float(art_.carry.w), float(art_.carry.h), PAL_HAND);
        spr(art_.flame, cx + 16.f, kGround - 36.f, 6.f, 9.f, PAL_FLAME);
    }

    if (mode_ == Mode::Title) spr(art_.title, 160.f, 52.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
    if (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_))
        spr(art_.leave, 160.f, 78.f, float(art_.leave.w), float(art_.leave.h), PAL_LEAVE);

    hud(1, 1, "S3 LANTERNBELL", PAL_INK);
    char buf[48];
    int shownTry = tryNo_ > 0 ? tryNo_ : 1;
    std::snprintf(buf, sizeof buf, "TRY %d   DEAD %d", shownTry, dead_);
    hud(1, 2, buf, PAL_GOLD);
    hud(30, 2, rung_ ? "BELL" : "QUIET", rung_ ? PAL_LEAVE : PAL_INK);

    if (mode_ == Mode::Title) hudC(25, "START  LIGHT THE ORDER", PAL_GREEN);
    else if (mode_ == Mode::Watch) hudC(25, "WATCH THE LAMPS", PAL_GOLD);
    else if (mode_ == Mode::Play) hudC(25, "LEFT RIGHT  A LIGHTS", PAL_GREEN);
    else if (mode_ == Mode::Dead) {
        std::snprintf(buf, sizeof buf, "%s  TRY DIED", why_);
        hudC(25, buf, PAL_ALERT);
    } else if (mode_ == Mode::Ring || mode_ == Mode::Leave) hudC(23, "LEAVE  THE BELL RANG", PAL_LEAVE);
    else if (mode_ == Mode::Over && !won_) hudC(25, "THIRD TRY DIED  BELL SILENT", PAL_ALERT);

    if (mode_ == Mode::Play) {
        std::snprintf(buf, sizeof buf, "LAMP %d OF %d", step_ + 1, kOrderN);
        hud(1, 25, buf, PAL_GOLD);
    }
}

}  // namespace lanternbell
