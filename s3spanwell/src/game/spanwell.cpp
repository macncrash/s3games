#include "game/spanwell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace spanwell {
namespace {

constexpr float DT = 1.0f / 60.0f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float spanX(float u) { return 160.0f + u * 118.0f; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(4, 7, 11));
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    wave_ = 0;
    clock_ = 0;
}

void Game::newGame() {
    mode_ = Mode::Play;
    phase_ = Phase::Calm;
    over_ = false;
    won_ = false;
    wave_ = 0;
    waveT_ = 0;
    calmT_ = 0;
    endT_ = 0;
    wx_ = 0;
    wv_ = 0;
    px_ = 0;
    shake_ = 0;
    bracing_ = false;
}

float Game::surge() const {
    if (phase_ != Phase::Wave) return 0;
    if (wave_ == 0) return 0.50f;
    if (wave_ == 1) return -0.64f;
    float half = 1.7f;
    return (std::fmod(waveT_, half * 2.0f) < half) ? 0.82f : -0.82f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) newGame();
        draw();
        return;
    }
    if (mode_ == Mode::Fell || mode_ == Mode::Stood) {
        endT_ += DT;
        shake_ = std::max(0.0f, shake_ - DT);
        if (endT_ > 1.15f) over_ = true;
        if (mode_ == Mode::Fell && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) newGame();
        sys.apu.tone(0, 0, 0);
        sys.apu.noise(0, 1000);
        draw();
        return;
    }

    left_ = pad.down(gs::BTN_LEFT);
    right_ = pad.down(gs::BTN_RIGHT);
    bool brace = pad.down(gs::BTN_A) || pad.down(gs::BTN_B);
    if (pad.axisX < -0.35f) left_ = true;
    if (pad.axisX > 0.35f) right_ = true;

    if (bot_) {
        left_ = right_ = brace = false;
        float look = wx_ + wv_ * 0.45f + surge() * 0.35f;
        float side = (look >= 0.0f) ? 1.0f : -1.0f;
        if (std::fabs(look) < 0.04f && std::fabs(surge()) < 0.05f) side = 0;
        if (side != 0.0f) {
            float goal = wx_ + side * 0.13f;
            if (px_ < goal - 0.025f) right_ = true;
            else if (px_ > goal + 0.025f) left_ = true;
            else brace = true;
        } else if (px_ > 0.05f) {
            left_ = true;
        } else if (px_ < -0.05f) {
            right_ = true;
        }
    }

    float dir = (right_ ? 1.0f : 0.0f) - (left_ ? 1.0f : 0.0f);
    px_ = clampf(px_ + dir * 2.05f * DT, -1.18f, 1.18f);

    float waveLen = (wave_ < 2) ? 5.4f : 6.9f;
    if (phase_ == Phase::Calm) {
        calmT_ += DT;
        wv_ += -wx_ * 1.4f * DT;
        wv_ *= 0.96f;
        wx_ += wv_ * DT;
        if (calmT_ > 1.15f) {
            phase_ = Phase::Wave;
            waveT_ = 0;
            sys.apu.noiseBurst(0.35f, 1800, 0.4f);
        }
    } else {
        waveT_ += DT;
        float acc = surge();
        bool beside = std::fabs(px_ - wx_) < 0.22f && std::fabs(px_ - wx_) > 0.035f;
        bracing_ = false;
        if (beside && brace) {
            float push = (px_ < wx_) ? 1.0f : -1.0f;
            acc += push * 2.55f;
            bracing_ = true;
        }
        wv_ += acc * DT;
        wv_ *= 0.992f;
        wx_ += wv_ * DT;
        if (waveT_ > waveLen) {
            if (wave_ >= 2) {
                mode_ = Mode::Stood;
                won_ = true;
                endT_ = 0;
                sys.apu.tone(0, 523.0f, 0.15f);
            } else {
                wave_++;
                phase_ = Phase::Calm;
                calmT_ = 0;
            }
        }
    }

    if (std::fabs(wx_) > 1.02f) {
        mode_ = Mode::Fell;
        won_ = false;
        endT_ = 0;
        shake_ = 1.0f;
        sys.apu.noiseBurst(0.6f, 900, 0.55f);
    }

    shake_ = std::max(0.0f, shake_ - DT * 0.4f);
    if (phase_ == Phase::Wave) {
        sys.apu.tone(0, 70.0f + std::fabs(surge()) * 40.0f, 0.05f + std::fabs(wv_) * 0.04f);
        sys.apu.noise(0.04f + std::fabs(surge()) * 0.03f, 2400, false);
        if (bracing_) sys.apu.tone(1, 180.0f, 0.06f);
        else sys.apu.tone(1, 0, 0);
    } else {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
        sys.apu.noise(0.015f, 1400, false);
    }

    if (pad.pressed(gs::BTN_START) && !bot_) {
        // leave the span; a fresh press on the title starts again
        mode_ = Mode::Title;
        sys.apu.tone(0, 0, 0);
        sys.apu.noise(0, 1000);
    }

    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.0f, 400.0f)));
    s.h = int16_t(std::lround(clampf(h, 1.0f, 400.0f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (w < 1.0f || h < 1.0f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.0f, 420.0f)));
    s.h = int16_t(std::lround(clampf(h, 1.0f, 240.0f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float sh = 0;
    if (shake_ > 0) sh = std::sin(clock_ * 70.0f) * 4.0f * shake_;
    float lean = clampf(wx_, -1.15f, 1.15f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 96) {
            float u = y / 96.0f;
            v.lineBackdrop[y] = gs::rgb4(4 + int(u * 4), 6 + int(u * 3), 11 + int((1.0f - u) * 3));
        } else {
            float u = (y - 96) / 128.0f;
            int drift = int(8.0f * std::sin(clock_ * 1.3f + y * 0.08f));
            v.lineBackdrop[y] = gs::rgb4(1, 3 + int(u * 4), 7 + int((1.0f - u) * 4));
            (void)drift;
        }
    }

    // Waves under and against the span. Earlier sprites sit on top, so the span is drawn last.
    for (int i = 0; i < 5; i++) {
        float sp = 40.0f + i * 18.0f;
        float x = std::fmod(clock_ * sp + i * 70.0f, 400.0f) - 40.0f;
        int fog = 2 + i;
        float crest = (phase_ == Phase::Wave) ? 6.0f + std::fabs(surge()) * 10.0f : 2.0f;
        stamp(art_.wave, x + sh, 150.0f + i * 8.0f - crest * 0.2f, 90.0f + i * 6.0f, 22.0f + crest, PAL_WAVE, fog);
    }

    const float deck = 132.0f + lean * lean * 6.0f;
    for (int i = 0; i < 5; i++) {
        float u = -1.0f + i * 0.5f;
        spr(art_.post, spanX(u) + sh, deck + 52.0f, 52.0f, PAL_WOOD, false, i == 0 || i == 4 ? 2 : 0);
    }
    for (int i = 0; i < 8; i++) {
        float u = -1.05f + i * 0.30f;
        float sag = std::fabs(u) * 4.0f;
        stamp(art_.plank, spanX(u) + sh, deck + sag, 46.0f, 16.0f, PAL_WOOD, 0);
    }

    float wellX = spanX(lean) + sh;
    float drop = (mode_ == Mode::Fell) ? std::min(80.0f, endT_ * 70.0f) : 0.0f;
    float wellY = deck - 4.0f + drop + std::fabs(lean) * 4.0f;
    // Crown shifts with the lean so the well is seen to tip.
    spr(art_.bucket, wellX - 10.0f + lean * 6.0f, wellY - 18.0f, 16.0f, PAL_ROPE);
    spr(art_.well, wellX, wellY, 62.0f, PAL_STONE);
    spr(art_.roof, wellX + lean * 14.0f, wellY - 52.0f, 26.0f, PAL_WELL);

    bool faceL = left_ && !right_;
    float kx = spanX(px_) + sh;
    float ky = deck + 2.0f;
    if (bracing_) stamp(art_.brace, (kx + wellX) * 0.5f, ky - 28.0f, 36.0f, 8.0f, PAL_ROPE);
    spr(art_.keeper, kx, ky, 48.0f, PAL_KEEPER, faceL);

    for (int i = 0; i < 4; i++) {
        float x = std::fmod(clock_ * 30.0f + i * 80.0f, 340.0f) - 10.0f;
        stamp(art_.foam, x, 168.0f + (i & 1) * 14.0f, 16.0f, 8.0f, PAL_FOAM, 1);
    }

    char buf[40];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 SPANWELL", PAL_GOLD);
        hudC(6, "ONE SPAN", PAL_HUD);
        hudC(8, "KEEP THE WELL STANDING", PAL_HUD);
        hudC(10, "THROUGH THREE WAVES", PAL_HUD);
        hudC(16, "A BRACE   ARROWS MOVE", PAL_WOOD);
        hudC(24, "START", PAL_GOLD);
    } else if (mode_ == Mode::Stood) {
        hudC(2, "THE WELL STOOD", PAL_GOLD);
        hudC(4, "THREE WAVES", PAL_HUD);
        std::snprintf(buf, sizeof(buf), "SPAN CLEAR");
        hudC(22, buf, PAL_GOLD);
    } else if (mode_ == Mode::Fell) {
        hudC(2, "THE WELL FELL", PAL_WARN);
        hudC(24, "START", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof(buf), "WAVE %d OF 3", wave_ + 1);
        hud(1, 0, buf, phase_ == Phase::Wave ? PAL_WARN : PAL_GOLD);
        hud(28, 0, phase_ == Phase::Wave ? "SURGE" : "CALM", phase_ == Phase::Wave ? PAL_WARN : PAL_HUD);
        hud(1, 27, "A BRACE", bracing_ ? PAL_GOLD : PAL_HUD);
        // Mark where the well sits on the span.
        int col = int(std::lround(clampf(20.0f + wx_ * 16.0f, 1.0f, 38.0f)));
        hud(col, 26, "W", std::fabs(wx_) > 0.72f ? PAL_WARN : PAL_STONE);
        hud(20, 26, ".", PAL_HUD);
    }
}

}  // namespace spanwell
