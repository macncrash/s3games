#include "fish.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fishmark {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kMark = 0.72f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            uint32_t e = 0;
            if (cy >= 14) e = gs::entry(art_.waterTile, PAL_WATER);
            sys.vdp.B.set(cx, cy, e);
        }
    }
    begin();
}

void Game::begin() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    race_ = 0;
    power_ = 0;
    cast_ = 0.55f;
    phase_ = 0;
    fish_ = 0;
    rod_ = 0;
    progress_ = 0;
    strain_ = 0;
    lay_ = 0.12f;
    note_[0] = 0;
    report_[0] = 0;
    tone_ = 0;
    toneV_ = 0;
    charging_ = false;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Cast || mode_ == Mode::Wait) return 1;
    if (mode_ == Mode::Fight) return 2;
    if (mode_ == Mode::Measure) return 3;
    return 4;
}

void Game::controls() {
    const gs::Pad& p = sys_->pad;
    holdA_ = p.down(gs::BTN_A) || p.down(gs::BTN_C);
    tapA_ = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
    start_ = p.pressed(gs::BTN_START);
    left_ = p.down(gs::BTN_LEFT) || p.axisX < -0.35f;
    right_ = p.down(gs::BTN_RIGHT) || p.axisX > 0.35f;
}

void Game::pilot() {
    holdA_ = false;
    tapA_ = false;
    left_ = false;
    right_ = false;
    start_ = false;
    if (mode_ == Mode::Title) {
        if (t_ > 0.35f) start_ = true;
        return;
    }
    if (mode_ == Mode::Cast) {
        holdA_ = phase_ < 0.55f;
        return;
    }
    if (mode_ == Mode::Wait) {
        if (phase_ >= biteAt_ && phase_ < biteAt_ + 0.15f) tapA_ = true;
        return;
    }
    if (mode_ == Mode::Fight) {
        if (fish_ > rod_ + 0.04f) right_ = true;
        else if (fish_ < rod_ - 0.04f) left_ = true;
        holdA_ = true;
        return;
    }
    if (mode_ == Mode::Measure) {
        if (lay_ < kMark) right_ = true;
        else tapA_ = true;
    }
}

void Game::snapLine() {
    mode_ = Mode::Cast;
    phase_ = 0;
    power_ = 0;
    progress_ = 0;
    strain_ = 0;
    rod_ = 0;
    std::snprintf(note_, sizeof note_, "THE LINE WENT");
    tone_ = 90.f;
    toneV_ = 0.15f;
}

void Game::win() {
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    std::snprintf(note_, sizeof note_, "THE MARK IS FINISHED");
    std::snprintf(report_, sizeof report_, "S3 FISH MARK  FINISHED  the mark is covered  (%.1f s)", race_);
    tone_ = 523.f;
    toneV_ = 0.18f;
}

void Game::stepCast() {
    if (holdA_) power_ = clampf(power_ + kDt * 0.85f, 0.f, 1.f);
    bool released = charging_ && !holdA_;
    charging_ = holdA_;
    if (released && power_ > 0.18f) {
        cast_ = 0.35f + power_ * 0.55f;
        mode_ = Mode::Wait;
        phase_ = 0;
        biteAt_ = 1.35f + (1.f - power_) * 0.4f;
        std::snprintf(note_, sizeof note_, "WAIT ON THE BOBBER");
        tone_ = 220.f;
        toneV_ = 0.08f;
    } else if (released && power_ > 0.02f) {
        std::snprintf(note_, sizeof note_, "CAST IT FURTHER");
        power_ = 0;
    }
}

void Game::stepWait() {
    if (phase_ < biteAt_) {
        std::snprintf(note_, sizeof note_, "THE MARK IS OUT THERE");
        return;
    }
    if (phase_ > biteAt_ + 1.05f) {
        mode_ = Mode::Cast;
        phase_ = 0;
        power_ = 0;
        std::snprintf(note_, sizeof note_, "IT SLIPPED THE HOOK");
        tone_ = 110.f;
        toneV_ = 0.1f;
        return;
    }
    std::snprintf(note_, sizeof note_, "STRIKE");
    if (tapA_) {
        mode_ = Mode::Fight;
        phase_ = 0;
        progress_ = 0;
        strain_ = 0;
        rod_ = 0;
        fish_ = 0;
        std::snprintf(note_, sizeof note_, "WORK THE ROD");
        tone_ = 330.f;
        toneV_ = 0.1f;
    }
}

void Game::stepFight() {
    float target = std::sin(phase_ * 1.35f) * 0.82f;
    fish_ += (target - fish_) * (1.f - std::exp(-kDt * 2.4f));
    float steer = (right_ ? 1.f : 0.f) - (left_ ? 1.f : 0.f);
    rod_ = clampf(rod_ + steer * 2.6f * kDt, -1.f, 1.f);
    float err = std::fabs(rod_ - fish_);
    if (holdA_ && err < 0.38f) {
        progress_ += kDt * 0.42f;
        strain_ = std::max(0.f, strain_ - kDt * 0.7f);
        std::snprintf(note_, sizeof note_, "REEL");
    } else if (err > 0.72f) {
        strain_ += kDt * 0.55f;
        std::snprintf(note_, sizeof note_, "GIVE IT LINE");
    } else {
        strain_ = std::max(0.f, strain_ - kDt * 0.35f);
        std::snprintf(note_, sizeof note_, "FIND THE BAND");
    }
    if (strain_ >= 1.f) snapLine();
    else if (progress_ >= 1.f) {
        mode_ = Mode::Measure;
        phase_ = 0;
        lay_ = 0.12f;
        std::snprintf(note_, sizeof note_, "LAY IT ON THE MARK");
        tone_ = 392.f;
        toneV_ = 0.12f;
    }
}

void Game::stepMeasure() {
    float steer = (right_ ? 1.f : 0.f) - (left_ ? 1.f : 0.f);
    lay_ = clampf(lay_ + steer * 0.42f * kDt, 0.f, 1.f);
    if (lay_ >= kMark) std::snprintf(note_, sizeof note_, "THE MARK IS COVERED");
    else std::snprintf(note_, sizeof note_, "SLIDE ONTO THE MARK");
    if (tapA_) {
        if (lay_ >= kMark) win();
        else {
            std::snprintf(note_, sizeof note_, "SHORT OF THE MARK");
            tone_ = 140.f;
            toneV_ = 0.08f;
        }
    }
}

void Game::audio() {
    if (toneV_ > 0.f) {
        sys_->apu.tone(0, tone_, toneV_);
        toneV_ = std::max(0.f, toneV_ - kDt * 0.35f);
        if (mode_ == Mode::Win) {
            float n = std::fmod(t_, 0.4f);
            sys_->apu.tone(0, n < 0.2f ? 523.f : 659.f, 0.08f);
        }
    } else if (mode_ == Mode::Fight) {
        sys_->apu.tone(0, 80.f + progress_ * 40.f, 0.03f);
    } else if (mode_ == Mode::Wait && phase_ >= biteAt_) {
        sys_->apu.tone(0, ((int(phase_ * 8.f) & 1) ? 480.f : 240.f), 0.05f);
    } else {
        sys_->apu.tone(0, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (bot_) pilot();
    else controls();
    if (mode_ == Mode::Title) {
        if (start_ || tapA_) {
            mode_ = Mode::Cast;
            phase_ = 0;
            power_ = 0;
            race_ = 0;
            std::snprintf(note_, sizeof note_, "HOLD A  THEN LET GO");
        }
    } else if (mode_ != Mode::Win) {
        race_ += kDt;
        phase_ += kDt;
        if (mode_ == Mode::Cast) stepCast();
        else if (mode_ == Mode::Wait) stepWait();
        else if (mode_ == Mode::Fight) stepFight();
        else if (mode_ == Mode::Measure) stepMeasure();
    } else if (!bot_ && start_) {
        begin();
    }
    audio();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::drawHud() {
    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(20, "A FINISHED MARK ENDS IT", PAL_BANNER);
        hudC(22, "HOLD A TO CAST", PAL_HUD);
        hudC(23, "STRIKE THE BITE", PAL_HUD);
        hudC(24, "LEFT AND RIGHT WORK THE ROD", PAL_HUD);
        hudC(25, "A REELS IN THE BAND", PAL_HUD);
        hudC(26, "LAY THE FISH ON THE MARK", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "RETURN", PAL_WIN);
        return;
    }
    int sec = int(race_);
    int frac = int((race_ - sec) * 10.f);
    std::snprintf(buf, sizeof buf, "%02d.%d", sec, frac);
    hud(1, 0, "S3 FISH MARK", PAL_BANNER);
    hud(34, 0, buf, PAL_HUD);
    if (mode_ == Mode::Win) {
        hudC(16, "THE MARK IS FINISHED", PAL_WIN);
        if (!bot_) hudC(18, "RETURN FISHES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Cast) {
        int n = std::max(0, std::min(8, int(power_ * 8.f)));
        char pips[12];
        for (int i = 0; i < 8; i++) pips[i] = i < n ? '#' : '-';
        pips[8] = 0;
        std::snprintf(buf, sizeof buf, "CAST %s", pips);
        hudC(26, buf, PAL_BANNER);
    } else if (mode_ == Mode::Fight) {
        int n = std::max(0, std::min(8, int(progress_ * 8.f + 0.001f)));
        char pips[12];
        for (int i = 0; i < 8; i++) pips[i] = i < n ? '#' : '-';
        pips[8] = 0;
        std::snprintf(buf, sizeof buf, "FISH %s", pips);
        hudC(25, buf, strain_ > 0.55f ? PAL_ALERT : PAL_WIN);
        int band = int((rod_ + 1.f) * 4.f);
        band = std::max(0, std::min(7, band));
        char rod[12] = "--------";
        rod[band] = 'O';
        rod[8] = 0;
        std::snprintf(buf, sizeof buf, "ROD %s", rod);
        hudC(26, buf, PAL_HUD);
    } else if (mode_ == Mode::Measure) {
        hudC(26, lay_ >= kMark ? "A FINISHES THE MARK" : "RIGHT ONTO THE GOLD", lay_ >= kMark ? PAL_WIN : PAL_HUD);
    }
    if (note_[0]) hudC(mode_ == Mode::Win ? 17 : 27, note_, mode_ == Mode::Fight && strain_ > 0.55f ? PAL_ALERT : PAL_HUD);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool hflip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    long sw = std::lround(w);
    long sh = std::lround(h);
    if (sw < 1) sw = 1;
    if (sh < 1) sh = 1;
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::lround(cx - sw * 0.5f));
    s.y = int16_t(std::lround(cy - sh * 0.5f));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky;
        if (y < 108) {
            float u = y / 108.f;
            sky = gs::rgb4(int(4 + u * 7), int(7 + u * 3), int(12 - u * 3));
        } else {
            float u = (y - 108.f) / 116.f;
            sky = gs::rgb4(int(2 + u), int(5 + u * 2), int(8 - u));
        }
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.B.hscroll[y] = int16_t(std::sin((y * 0.15f) + t_ * 1.7f) * 6.f);
    }
    spr(art_.reed, 28, 118, 36, PAL_REED);
    spr(art_.reed, 48, 122, 30, PAL_REED);
    spr(art_.reed, 300, 120, 34, PAL_REED);
    float gy = 36 + std::sin(t_ * 0.8f) * 6.f;
    spr(art_.gull, 200 + std::sin(t_ * 0.4f) * 30.f, gy, 10, PAL_HUD);

    spr(art_.pier, 118, 168, 36, PAL_PIER);
    spr(art_.angler, 78, 142, 52, PAL_ANGLER);
    float tipX = 108, tipY = 108;
    spr(art_.rod, 96, 124, 62, PAL_ANGLER);

    float reach = 150.f + cast_ * 120.f;
    if (mode_ == Mode::Fight) reach = 150.f + cast_ * 120.f * (1.f - progress_);
    if (mode_ == Mode::Cast) reach = 130.f + power_ * 140.f;
    if (mode_ == Mode::Title || mode_ == Mode::Measure || mode_ == Mode::Win) reach = 168.f;
    float bobY = 132.f + std::sin(t_ * 3.f) * 2.f;
    if (mode_ == Mode::Wait && phase_ >= biteAt_) bobY += std::sin(phase_ * 28.f) * 4.f;
    int beads = 10;
    for (int i = 1; i < beads; i++) {
        float u = i / float(beads);
        float x = tipX + (reach - tipX) * u;
        float y = tipY + (bobY - tipY) * u + std::sin(u * 3.14f) * 10.f;
        spr(art_.bead, x, y, 3, PAL_LINE);
    }
    bool showFish = mode_ == Mode::Fight || mode_ == Mode::Wait;
    if (showFish && mode_ == Mode::Fight) {
        float fx = reach + fish_ * 18.f;
        spr(art_.fish, fx, bobY + 16.f, 22, PAL_FISH, fish_ < 0);
        spr(art_.splash, reach, bobY + 4.f, 12, PAL_WATER);
    }
    if (mode_ != Mode::Measure && mode_ != Mode::Win) spr(art_.bobber, reach, bobY, 16, PAL_BOBBER);

    spr(art_.board, 196, 158, 22, PAL_BOARD);
    if (mode_ == Mode::Measure || mode_ == Mode::Win) {
        float fx = 150.f + lay_ * 78.f;
        spr(art_.fish, fx, 150, 26, PAL_FISH, false);
    }

    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 48, 28, PAL_BANNER);
        spr(art_.wordMark, 160, 82, 28, PAL_BANNER);
    } else if (mode_ == Mode::Win) {
        spr(art_.finished, 160, 64, 26, PAL_WIN);
    }
    drawHud();
}

}  // namespace fishmark
