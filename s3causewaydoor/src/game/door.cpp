#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace causewaydoor {
namespace {
constexpr int HOLD = 180 * 60;
constexpr float FAIL = 1.f;
}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.45f);
    sys.apu.setEcho(0.18f, 0.25f, 0.15f);
    if (bot_) begin();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    age_ = 0;
    gap_ = 0.18f;
    shake_ = 0;
    beat_ = 0;
}

void Game::layBeats() {
    static const int kPat[] = {LEAN_L, LEAN_R, BRACE, LEAN_L, CHAIN, LEAN_R, SHOULDER, BRACE};
    int t = 70;
    int n = 0;
    while (t < HOLD - 90 && n < 72) {
        int kind = kPat[n % 8];
        int dur = (kind == SHOULDER) ? 34 : (kind == CHAIN) ? 26 : 46;
        beats_[n] = {t, dur, kind};
        t += dur + 64 + (n % 5) * 16;
        n++;
    }
    beatCount_ = n;
    beat_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    age_ = 0;
    gap_ = 0.16f;
    shake_ = 0;
    lamp_ = 0;
    layBeats();
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    if (held) {
        sys_->apu.keyOn(0, 220.f, 0.22f);
        sys_->apu.keyOn(1, 277.f, 0.16f);
        sys_->apu.keyOn(2, 330.f, 0.14f);
    } else {
        sys_->apu.noiseBurst(0.4f, 90.f, 0.25f);
        sys_->apu.keyOn(0, 70.f, 0.2f);
    }
}

bool Game::holdLeft() const {
    if (bot_)
        return beat_ < beatCount_ && age_ >= beats_[beat_].at && age_ < beats_[beat_].at + beats_[beat_].dur &&
               beats_[beat_].kind == LEAN_L;
    const gs::Pad& p = sys_->pad;
    return p.down(gs::BTN_LEFT) || p.axisX < -0.35f;
}

bool Game::holdRight() const {
    if (bot_) {
        if (beat_ < beatCount_ && age_ >= beats_[beat_].at && age_ < beats_[beat_].at + beats_[beat_].dur)
            return beats_[beat_].kind == LEAN_R;
        return false;
    }
    const gs::Pad& p = sys_->pad;
    return p.down(gs::BTN_RIGHT) || p.axisX > 0.35f;
}

bool Game::holdA() const {
    if (bot_) {
        if (beat_ < beatCount_ && age_ >= beats_[beat_].at && age_ < beats_[beat_].at + beats_[beat_].dur)
            return beats_[beat_].kind == BRACE;
        return true;
    }
    return sys_->pad.down(gs::BTN_A);
}

bool Game::holdB() const {
    if (bot_)
        return beat_ < beatCount_ && age_ >= beats_[beat_].at && age_ < beats_[beat_].at + beats_[beat_].dur &&
               beats_[beat_].kind == CHAIN;
    return sys_->pad.down(gs::BTN_B);
}

bool Game::holdC() const {
    if (bot_)
        return beat_ < beatCount_ && age_ >= beats_[beat_].at && age_ < beats_[beat_].at + beats_[beat_].dur &&
               beats_[beat_].kind == SHOULDER;
    const gs::Pad& p = sys_->pad;
    return p.down(gs::BTN_C) || p.down(gs::BTN_DOWN) || p.axisY < -0.45f;
}

bool Game::correct() const {
    if (beat_ >= beatCount_) return holdA();
    const Beat& b = beats_[beat_];
    if (age_ < b.at || age_ >= b.at + b.dur) return holdA() || holdLeft() || holdRight();
    switch (b.kind) {
        case LEAN_L: return holdLeft();
        case LEAN_R: return holdRight();
        case BRACE: return holdA();
        case CHAIN: return holdB();
        case SHOULDER: return holdC();
        default: return false;
    }
}

const char* Game::cue() const {
    if (beat_ >= beatCount_ || age_ < beats_[beat_].at) return "HOLD";
    switch (beats_[beat_].kind) {
        case LEAN_L: return "LEAN LEFT";
        case LEAN_R: return "LEAN RIGHT";
        case BRACE: return "BRACE";
        case CHAIN: return "CATCH CHAIN";
        case SHOULDER: return "SHOULDER";
        default: return "HOLD";
    }
}

void Game::update() {
    while (beat_ < beatCount_ && age_ >= beats_[beat_].at + beats_[beat_].dur) beat_++;
    const bool live = beat_ < beatCount_ && age_ >= beats_[beat_].at;
    float push = 0.00048f;
    if (live) {
        const int kind = beats_[beat_].kind;
        float miss = 0.009f;
        if (kind == CHAIN) miss = 0.020f;
        else if (kind == SHOULDER) miss = 0.024f;
        else if (kind == BRACE) miss = 0.011f;
        push = correct() ? -0.0065f : miss;
        if (!correct()) shake_ = std::min(6.f, shake_ + 0.45f);
    } else if (holdA() || holdLeft() || holdRight()) {
        push = -0.0014f;
    }
    gap_ = std::clamp(gap_ + push, 0.f, 1.25f);
    if (shake_ > 0) shake_ *= 0.86f;
    lamp_ += 0.07f;
    age_++;
    if (gap_ >= FAIL) finish(false);
    else if (age_ >= HOLD) finish(true);
}

void Game::audio() {
    if (!sys_) return;
    float wind = 0.04f + gap_ * 0.08f;
    sys_->apu.noise(wind, 0.35f + gap_ * 0.4f, false);
    if (mode_ == Mode::Play && (age_ % 60) == 0) sys_->apu.tone(0, 90.f + (1.f - gap_) * 40.f, 0.05f);
    else if (mode_ != Mode::Play) sys_->apu.tone(0, 0, 0);
    if (blip_ > 0) {
        blip_ -= 1.f;
        sys_->apu.tone(1, 480.f, 0.06f);
    } else {
        sys_->apu.tone(1, 0, 0);
    }
    if (mode_ == Mode::Play && beat_ < beatCount_ && age_ == beats_[beat_].at) blip_ = 6;
}

void Game::sky() {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r, g, b;
        if (y < 108) {
            float t = y / 108.f;
            r = int(2 + t * 4);
            g = int(3 + t * 5);
            b = int(6 + t * 4);
        } else if (y < 150) {
            r = 2;
            g = 4;
            b = 6;
        } else {
            float t = (y - 150) / 74.f;
            r = int(1 + t);
            g = int(3 + t * 2);
            b = int(5 + t);
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = uint8_t(y < 70 ? (70 - y) / 14 : 0);
        vdp.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float width = 0;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, 0);
        x += g.w * scale + scale;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    sky();
    float sh = (shake_ > 0.4f) ? std::sin(age_ * 1.7f) * shake_ : 0.f;
    float open = gap_ * 52.f;
    int scroll = age_ / 2;

    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(40.f + i * 90.f + age_ * 0.15f, 360.f) - 20.f;
        spr(art_.gull, gx, 28.f + (i * 7 % 11), 10.f, PAL_SKY, i & 1, 6);
    }
    for (int i = 0; i < 6; i++) {
        float x = 18.f + i * 54.f;
        int fog = i < 2 ? 8 : 2;
        spr(art_.pile, x, 132.f, 70.f - i * 2.f, PAL_STONE, false, fog);
    }
    for (int i = 0; i < 8; i++) {
        float x = std::fmod(float((i * 48 - scroll) & 511), 380.f) - 30.f;
        spr(art_.wave, x, 176.f + (i % 3) * 6.f, 16.f, PAL_SEA, i & 1, 0);
    }
    for (int i = 0; i < 9; i++) spr(art_.slab, 20.f + i * 34.f, 158.f, 22.f, PAL_STONE, false, 0);

    float doorX = 248.f + open + sh;
    spr(art_.door, doorX, 108.f, 128.f, PAL_IRON, false, 0);
    float swing = std::sin(age_ * 0.08f) * (beat_ < beatCount_ && beats_[beat_].kind == CHAIN && age_ >= beats_[beat_].at ? 10.f : 3.f);
    spr(art_.chain, doorX - 8.f + swing, 46.f, 36.f, PAL_CHAIN, false, 0);
    spr(art_.bolt, doorX + 6.f, 70.f, 18.f, PAL_CHAIN, false, 0);

    bool chainNow = beat_ < beatCount_ && age_ >= beats_[beat_].at && beats_[beat_].kind == CHAIN;
    bool shoulder = holdC() || (mode_ == Mode::Play && beat_ < beatCount_ && age_ >= beats_[beat_].at && beats_[beat_].kind == SHOULDER && correct());
    bool bracing = holdA();
    const gs::Mipped* body = &art_.leanL;
    if (shoulder) body = &art_.shoulder;
    else if (bracing) body = &art_.brace;
    else if (holdRight()) body = &art_.leanR;
    else if (holdLeft()) body = &art_.leanL;
    spr(*body, 196.f + sh * 0.4f - open * 0.15f, 124.f, 78.f, PAL_COAT, false, 0);

    float glow = 36.f + std::sin(lamp_) * 2.f;
    spr(art_.lamp, 70.f, 92.f, glow, PAL_LAMP, false, 0);
    spr(art_.lamp, 150.f, 86.f, glow - 2.f, PAL_LAMP, false, 0);

    if (mode_ == Mode::Title) {
        text("CAUSEWAY DOOR", 160.f, 28.f, 1.15f, PAL_LAMP);
        hudC(22, "HOLD THE DOOR", PAL_HUD);
        hudC(24, "THREE MINUTES", PAL_HUD);
        hudC(26, "START", PAL_LAMP);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = std::max(0, HOLD - age_);
        int sec = left / 60;
        char clock[16];
        std::snprintf(clock, sizeof(clock), "%d:%02d", sec / 60, sec % 60);
        hud(1, 1, clock, PAL_HUD);
        char bar[22];
        int filled = std::clamp(int(gap_ * 16.f + 0.5f), 0, 16);
        for (int i = 0; i < 16; i++) bar[i] = i < filled ? '#' : '-';
        bar[16] = 0;
        hud(22, 1, bar, gap_ > 0.72f ? PAL_HUD : PAL_LAMP);
        if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_LAMP);
        else hudC(26, cue(), chainNow || (beat_ < beatCount_ && age_ >= beats_[beat_].at) ? PAL_LAMP : PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160.f, 36.f, 1.1f, PAL_LAMP);
        hudC(24, "THREE MINUTES", PAL_HUD);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        text("WATCH IS OVER", 160.f, 36.f, 1.1f, PAL_HUD);
        hudC(24, "THE DOOR OPENED", PAL_HUD);
        hudC(26, "START", PAL_LAMP);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        age_++;
        lamp_ += 0.05f;
        bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
        if (start) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    }
    audio();
    draw();
}

}  // namespace causewaydoor
