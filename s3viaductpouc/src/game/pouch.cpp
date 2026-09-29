#include "game/pouch.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace pouch {
namespace {

constexpr float kDeck = 112;
constexpr float kGrav = 0.2f;
constexpr float kJump = -4.35f;
constexpr float kSpeed = 1.75f;

struct Span {
    float a, b;
};
// Missing planks between the piers. Centres must stay on a span.
constexpr Span kDeckSpan[] = {{8, 78}, {106, 154}, {182, 222}, {250, 314}};
constexpr Span kGap[] = {{78, 106}, {154, 182}, {222, 250}};

float lampX(int frame) {
    return 160.0f + std::sin(frame * 0.045f) * 130.0f;
}

bool windOn(int frame) {
    int m = frame % 280;
    return m >= 190 && m < 250;
}

}  // namespace

bool Game::deckAt(float x) const {
    for (const Span& s : kDeckSpan)
        if (x >= s.a && x <= s.b) return true;
    return false;
}

void Game::spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip, bool shadow) {
    if (w < 1 || h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::drawSpan(float x0, float x1) {
    float w = x1 - x0;
    spr(art_.deck, x0, kDeck, w, 14, 1);
    spr(art_.pier, x0 - 4, kDeck + 8, 18, 72, 1);
    spr(art_.pier, x1 - 14, kDeck + 8, 18, 72, 1);
}

void Game::dropPouch(float kick) {
    if (!held_) return;
    held_ = false;
    px_ = x_ + 8;
    py_ = y_ - 18;
    pvx_ = kick;
    pvy_ = -1.2f;
    sys_->apu.noiseBurst(0.35f, 700.0f, 0.18f);
}

void Game::respawn() {
    if (mode_ != Mode::Play) return;
    lives_--;
    if (lives_ <= 0) {
        mode_ = Mode::Dead;
        over_ = true;
        won_ = false;
        sys_->apu.noiseBurst(0.5f, 200.0f, 0.4f);
        return;
    }
    x_ = 22;
    y_ = kDeck;
    vy_ = 0;
    held_ = true;
    px_ = x_;
    py_ = y_ - 16;
    inv_ = 50;
    sys_->apu.tone(1, 180.0f, 0.06f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    makeArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.road[y].on = false;
    if (bot_) mode_ = Mode::Play;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    frameN_++;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Play;
            frameN_ = 0;
            sys.apu.tone(0, 660.0f, 0.07f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if ((mode_ == Mode::Victory || mode_ == Mode::Dead) && !bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Play;
        over_ = false;
        won_ = false;
        lives_ = 3;
        held_ = true;
        x_ = 22;
        y_ = kDeck;
        vy_ = 0;
        frameN_ = 0;
        inv_ = 40;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;

        bool right = false, left = false, jump = false, clutch = false, duck = false;
        if (bot_) {
            right = true;
            clutch = true;
            float lx = lampX(frameN_);
            duck = std::fabs(x_ - lx) < 28.0f;
            for (const Span& g : kGap) {
                float d = g.a - x_;
                if (d > 4.0f && d < 18.0f && y_ >= kDeck - 0.5f && vy_ == 0) jump = true;
            }
        } else {
            right = pad.down(gs::BTN_RIGHT);
            left = pad.down(gs::BTN_LEFT);
            jump = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
            clutch = pad.down(gs::BTN_A) || pad.down(gs::BTN_Z);
            duck = pad.down(gs::BTN_DOWN);
        }

        if (inv_ > 0) inv_ -= 1;
        const bool gust = windOn(frameN_);
        if (right) {
            x_ += kSpeed;
            faceR_ = true;
        }
        if (left) {
            x_ -= kSpeed;
            faceR_ = false;
        }
        if (x_ < 12) x_ = 12;
        if (x_ > 308) x_ = 308;

        bool grounded = y_ >= kDeck - 0.01f && vy_ >= 0 && deckAt(x_);
        if (grounded) {
            y_ = kDeck;
            vy_ = 0;
            if (jump) {
                vy_ = kJump;
                grounded = false;
                sys.apu.tone(0, 420.0f, 0.05f);
            }
        } else {
            vy_ += kGrav;
            y_ += vy_;
        }

        if (y_ > 200) respawn();

        if (mode_ == Mode::Play && held_ && inv_ <= 0) {
            if (gust && !clutch) dropPouch(-1.4f);
            float lx = lampX(frameN_);
            bool lit = std::fabs(x_ - lx) < 16.0f && y_ < kDeck + 2;
            if (lit && !duck) dropPouch(faceR_ ? 1.1f : -1.1f);
        }

        if (!held_) {
            pvy_ += kGrav * 0.85f;
            px_ += pvx_;
            py_ += pvy_;
            if (py_ > kDeck - 8 && deckAt(px_ + 6) && pvy_ > 0) {
                py_ = kDeck - 8;
                pvy_ = 0;
                pvx_ *= 0.4f;
            }
            if (py_ > 190) respawn();
            float dx = (x_ + 4) - (px_ + 6);
            float dy = (y_ - 14) - py_;
            if (mode_ == Mode::Play && dx * dx + dy * dy < 16 * 16) {
                held_ = true;
                inv_ = 30;
                sys.apu.tone(0, 520.0f, 0.06f);
            }
        } else {
            px_ = x_ + (faceR_ ? 8 : -6);
            py_ = y_ - (duck ? 8 : 16);
        }

        if (mode_ == Mode::Play && held_ && x_ >= 298 && deckAt(x_) && y_ >= kDeck - 1) {
            mode_ = Mode::Victory;
            won_ = true;
            over_ = true;
            sys.apu.tone(0, 523.0f, 0.08f);
            sys.apu.tone(1, 659.0f, 0.07f);
            sys.apu.tone(2, 784.0f, 0.06f);
        }

        if (grounded && (right || left)) {
            if (++stepSnd_ >= 14) {
                stepSnd_ = 0;
                sys.apu.noise(0.03f, 1800.0f, true);
            }
        } else if (stepSnd_ == 0) {
            sys.apu.noise(0, 0, false);
        }
        windT_ = gust ? windT_ + 1 : 0;
        if (gust && (frameN_ % 12) == 0) sys.apu.noiseBurst(0.08f, 400.0f, 0.12f);
    }

    // Picture.
    gs::VDP& v = sys.vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = y / float(gs::SCREEN_H);
        uint16_t sky;
        if (y < 156) {
            int r = int(2 + u * 6);
            int g = int(3 + u * 5);
            int b = int(8 + (1.0f - u) * 6);
            sky = gs::rgb4(r, g, b);
        } else {
            int wob = int(2 + std::sin((y + frameN_) * 0.18f) * 1.2f);
            sky = gs::rgb4(1, 3 + wob, 7 + (y & 1));
        }
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
    }

    bool duck = !bot_ && pad.down(gs::BTN_DOWN) && mode_ == Mode::Play;
    if (bot_ && mode_ == Mode::Play) duck = std::fabs(x_ - lampX(frameN_)) < 28.0f;
    int fr = (int(std::fabs(x_)) / 6) & 1;
    float ph = duck ? 18.0f : 28.0f;
    if (mode_ == Mode::Title) {
        spr(art_.runner[(frameN_ / 10) & 1], 36, 112, 32, 56, 2);
        spr(art_.pouch, 70, 128, 28, 24, 3);
    } else {
        gs::Sprite sh;
        sh.img = art_.deck;
        sh.x = int16_t(x_ - 6);
        sh.y = int16_t(kDeck + 10);
        sh.w = 16;
        sh.h = 4;
        sh.shadow = true;
        sh.pal = 1;
        v.sprite(sh);
        spr(art_.runner[fr], x_ - 8, y_ - ph, 16, ph, 2, !faceR_);
        spr(art_.pouch, px_, py_, 14, 12, 3);
    }

    float lx = lampX(frameN_);
    {
        gs::Sprite beam;
        beam.img = art_.deck;
        beam.x = int16_t(lx - 10);
        beam.y = 94;
        beam.w = 20;
        beam.h = 18;
        beam.pal = 6;
        beam.fog = 8;
        v.sprite(beam);
    }
    spr(art_.lamp, lx - 5, 78, 10, 16, 6);
    spr(art_.flag, 292, kDeck - 16, 18, 14, 4);
    for (const Span& s : kDeckSpan) drawSpan(s.a, s.b);
    spr(art_.pier, -8, 168, 340, 14, 1);

    float gy = 70 + std::sin(frameN_ * 0.05f) * 6;
    spr(art_.gull, std::fmod(frameN_ * 0.4f, 340.0f) - 10, gy, 12, 6, 5);
    float c1 = std::fmod(frameN_ * 0.15f, 360.0f);
    spr(art_.cloud, c1 - 40, 28, 40, 16, 5);
    spr(art_.cloud, std::fmod(c1 * 0.6f + 80, 360.0f) - 40, 48, 28, 12, 5);
    spr(art_.moon, 250, 16, 22, 22, 5);

    if (mode_ == Mode::Title) {
        hudC(4, "VIADUCT", 7);
        hudC(6, "CARRY THE POUCH ACROSS", 7);
        hudC(18, "ARROWS RUN   B JUMP", 7);
        hudC(20, "A CLUTCH   DOWN DUCK", 7);
        hudC(23, "START", 9);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", 7);
    } else if (mode_ == Mode::Dead) {
        hudC(10, "THE POUCH IS LOST", 8);
        hudC(12, "WATCH OVER", 8);
    } else if (mode_ == Mode::Victory || won_) {
        hudC(8, "ACROSS", 9);
        hudC(10, "THE POUCH IS DELIVERED", 7);
    } else {
        char lives[24];
        std::snprintf(lives, sizeof lives, "LIVES %d", lives_);
        hud(1, 1, lives, 7);
        hud(28, 1, held_ ? "POUCH" : "DROPPED", held_ ? 9 : 8);
        if (windOn(frameN_)) hudC(25, held_ ? "WIND  CLUTCH" : "WIND", 8);
        if (std::fabs(x_ - lampX(frameN_)) < 36) hudC(24, "DUCK THE LAMP", 8);
        if (!held_) hudC(23, "PICK IT UP", 8);
    }
}

}  // namespace pouch
