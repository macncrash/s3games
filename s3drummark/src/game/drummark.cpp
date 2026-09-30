#include "game/drummark.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace drummark {
namespace {

constexpr float kLeft = 48.f;
constexpr float kRight = 272.f;
constexpr float kSpeed = 1.55f;
constexpr float kWindow = 8.f;
constexpr float kMarkX[Game::kMarks] = {86.f, 138.f, 190.f, 242.f};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

int Game::hits() const {
    int n = 0;
    for (int i = 0; i < kMarks; i++)
        if (did_[i]) n++;
    return n;
}

float Game::beaterX() const { return kLeft + phase_; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    misses_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    mode_ = Mode::Title;
    hold_ = 0;
    tick_ = 0;
    t_ = 0;
    phase_ = 0;
    swing_ = 0;
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    say_[0] = 0;
    sayT_ = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 0, 2));
    sys.apu.setMaster(0.85f);
}

void Game::beginPhrase(bool keepMisses) {
    if (!keepMisses) misses_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    phase_ = 0;
    swing_ = 0;
    shake_ = 0;
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    mode_ = Mode::Play;
    std::snprintf(say_, sizeof say_, "COUNT IN");
    sayT_ = 0.6f;
    blip(220.f, 0.06f, 4);
}

void Game::blip(float freq, float vol, int frames) {
    if (!sys_ || fanT_ > 0) return;
    sys_->apu.tone(0, freq, vol);
    blip_ = frames;
}

void Game::strike() {
    int n = hits();
    if (n >= kMarks) return;
    did_[n] = true;
    swing_ = 10;
    shake_ = 4;
    static const char* name[kMarks] = {"ONE", "TWO", "THREE", "FOUR"};
    std::snprintf(say_, sizeof say_, "%s", name[n]);
    sayT_ = 0.45f;
    if (sys_) {
        sys_->apu.noiseBurst(0.42f, 1800.f, 0.08f);
        sys_->apu.tone(1, 140.f, 0.08f);
        sys_->rumble(0.15f, 0.35f, 50);
        sys_->setLight(180, 40, 30);
    }
    blip(180.f + float(n) * 40.f, 0.05f, 4);
    if (n + 1 == kMarks) finishMark();
}

void Game::drop() {
    for (int i = 0; i < kMarks; i++) did_[i] = false;
    misses_++;
    phase_ = 0;
    swing_ = 0;
    shake_ = 6;
    missT_ = 36;
    mode_ = Mode::Miss;
    won_ = false;
    finished_ = false;
    std::snprintf(say_, sizeof say_, "MISS");
    sayT_ = 0.7f;
    if (sys_) {
        sys_->apu.noiseBurst(0.18f, 400.f, 0.2f);
        sys_->rumble(0.4f, 0.1f, 90);
        sys_->setLight(40, 20, 80);
    }
}

void Game::finishMark() {
    finished_ = true;
    won_ = true;
    mode_ = Mode::Win;
    hold_ = 0;
    fanT_ = 1;
    std::snprintf(say_, sizeof say_, "FINISHED MARK");
    sayT_ = 3.f;
    if (sys_) {
        sys_->rumble(0.3f, 0.6f, 220);
        sys_->setLight(40, 180, 90);
    }
}

bool Game::botTap() const {
    if (mode_ != Mode::Play) return false;
    int n = 0;
    for (; n < kMarks; n++)
        if (!did_[n]) break;
    if (n >= kMarks) return false;
    float d = beaterX() - kMarkX[n];
    return d >= -3.2f && d <= 3.2f;
}

void Game::stepPlay() {
    bool tap = false;
    if (bot_) tap = botTap();
    else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    }

    int n = hits();
    if (tap) {
        if (n < kMarks && std::fabs(beaterX() - kMarkX[n]) <= kWindow) strike();
        else drop();
        return;
    }

    phase_ += kSpeed;
    float x = beaterX();
    if (n < kMarks && x > kMarkX[n] + kWindow) {
        drop();
        return;
    }
    if (phase_ > (kRight - kLeft)) phase_ = 0;
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (fanT_ > 0) {
        fanT_++;
        if (fanT_ == 2 || fanT_ == 12 || fanT_ == 22 || fanT_ == 34) {
            static const float n[4] = {196.f, 247.f, 294.f, 392.f};
            int i = fanT_ < 12 ? 0 : fanT_ < 22 ? 1 : fanT_ < 34 ? 2 : 3;
            a.tone(0, n[i], 0.12f);
            a.tone(2, n[i] * 2.f, 0.04f);
        }
        if (fanT_ > 70) {
            a.tone(0, 0, 0);
            a.tone(2, 0, 0);
            fanT_ = 0;
        }
        return;
    }
    if (blip_ > 0) {
        blip_--;
        if (blip_ == 0) a.tone(0, 0, 0);
    }
    if (swing_ <= 0) a.tone(1, 0, 0);
}

void Game::image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip, int fog, bool shadow) {
    if (!sys_ || h < 1.5f || h > 420.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (left > 340.f || top > 250.f || left + w < -30.f || top + h < -30.f) return;
    gs::Sprite s;
    auto q = [](float v) { return int16_t(std::lround(clampf(v, -400.f, 800.f))); };
    s.x = q(left);
    s.y = q(top);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip, int fog) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    image(m, cx - w * 0.5f, bottom - h, h, pal, flip, fog, false);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = y > 170 ? uint8_t((y - 170) / 6) : 0;
        if (y < 128) {
            float u = float(y) / 128.f;
            v.lineBackdrop[y] = gs::rgb4(2 + int(u * 2.f), 0, 4 + int((1.f - u) * 3.f));
        } else {
            float u = float(y - 128) / 96.f;
            v.lineBackdrop[y] = gs::rgb4(4 - int(u * 2.f), 2, 2);
        }
    }
}

void Game::stage() {
    image(art_.curtain, 8.f, 18.f, 130.f, PAL_STAGE, false, 2, false);
    image(art_.curtain, 284.f, 18.f, 130.f, PAL_STAGE, true, 2, false);
    spr(art_.lamp, 40.f, 150.f, 52.f, PAL_LAMP, false, 0);
    spr(art_.lamp, 280.f, 150.f, 52.f, PAL_LAMP, false, 0);

    float jig = shake_ ? ((tick_ & 1) ? 1.5f : -1.5f) : 0.f;
    spr(art_.player, 168.f + jig, 168.f, 62.f, PAL_PLAYER, false, 0);
    spr(art_.stool, 168.f, 176.f, 16.f, PAL_FX, false, 0);
    float bounce = swing_ > 6 ? 3.f : 0.f;
    spr(art_.drum, 168.f + jig, 158.f + bounce, 42.f, PAL_DRUM, false, 0);
    const gs::Mipped& stick = swing_ > 0 ? art_.stickDown : art_.stickUp;
    image(stick, 176.f, swing_ > 0 ? 108.f : 96.f, 28.f, PAL_FX, false, 0, false);

    float barY = 186.f;
    for (int i = 0; i < kMarks; i++) {
        const gs::Mipped& pip = did_[i] ? art_.pipOn : art_.pip;
        int pal = did_[i] ? PAL_GREEN : PAL_GOLD;
        spr(pip, kMarkX[i], barY, 12.f, pal, false, 0);
    }
    if (mode_ == Mode::Play || mode_ == Mode::Title) {
        float bx = (mode_ == Mode::Title) ? (kLeft + std::fmod(t_ * 40.f, kRight - kLeft)) : beaterX();
        spr(art_.beater, bx, barY - 2.f, 14.f, PAL_RED, false, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    stage();

    if (mode_ == Mode::Win) {
        spr(art_.card, 160.f, 78.f, 40.f, PAL_GOLD, false, 0);
        spr(art_.stamp, 160.f, 70.f, 16.f, PAL_RED, false, 0);
    }

    if (mode_ == Mode::Title) {
        hudC(2, "S3 DRUMMARK", PAL_GOLD);
        hudC(4, "A SHORT DRUM", PAL_HUD);
        hudC(6, "FOUR STROKES FINISH IT", PAL_HUD);
        hudC(8, "C ON EACH MARK", PAL_HUD);
        hudC(20, "ENTER STARTS", PAL_GREEN);
        return;
    }

    hud(1, 0, "S3 DRUMMARK", PAL_GOLD);
    char pips[8];
    for (int i = 0; i < kMarks; i++) pips[i] = did_[i] ? '#' : '-';
    pips[kMarks] = 0;
    char buf[32];
    std::snprintf(buf, sizeof buf, "MARK %s", pips);
    hud(28, 0, buf, hits() == kMarks ? PAL_GREEN : PAL_HUD);

    if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_GOLD);
        hudC(6, "ENTER RESUMES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(10, "FINISHED MARK", PAL_GREEN);
        hudC(12, "THE CARD IS CLOSED", PAL_GOLD);
        hudC(14, "LEAVE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Miss) {
        hudC(4, "MISS", PAL_RED);
        hudC(6, "THE MARK IS OPEN", PAL_GOLD);
        return;
    }
    if (sayT_ > 0 && say_[0]) hudC(3, say_, PAL_GOLD);
    else hudC(3, "STRIKE THE MARK", PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    tick_++;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - 1.f / 60.f);
    if (shake_ > 0) shake_--;
    if (swing_ > 0) swing_--;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        hold_++;
        bool go = bot_ ? hold_ >= 24 : (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C));
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        if (go) beginPhrase(false);
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) mode_ = Mode::Play;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Miss) {
        missT_--;
        if (bot_) {
            if (missT_ <= 0) beginPhrase(true);
        } else if (missT_ <= 0 || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            beginPhrase(true);
        }
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Win) {
        hold_++;
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
        draw();
        audio();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        audio();
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        sys.quit();
        return;
    }

    stepPlay();
    draw();
    audio();
}

}  // namespace drummark
