#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace viaductbann {
namespace {

constexpr float kDeck = 148.f;
constexpr float kJump = -6.55f;
constexpr float kGrav = 0.2f;
constexpr float kWalk = 2.95f;
constexpr float kCarry = 2.45f;
constexpr float kWatch = 100.f;
constexpr float kHome = 100.f;
constexpr float kBanner = 2320.f;
constexpr float kWorld = 2500.f;
constexpr int kLives = 4;

struct Span {
    float a, b;
};
constexpr Span kSpans[] = {{0.f, 540.f}, {640.f, 1200.f}, {1300.f, 1860.f}, {1960.f, 2500.f}};

}  // namespace

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    toneT_ = hold;
}

const gs::Mipped& Game::hero() const {
    if (!grounded_) return art_.leap;
    if (std::fabs(vx_) < 0.2f) return art_.stand;
    return (int(playT_ * 8.f) & 1) ? art_.walkA : art_.walkB;
}

bool Game::solidAt(float x) const {
    for (const Span& s : kSpans)
        if (x >= s.a + 8.f && x <= s.b - 8.f) return true;
    return false;
}

bool Game::spanOf(float x, float& a, float& b) const {
    for (const Span& s : kSpans)
        if (x >= s.a && x <= s.b) {
            a = s.a;
            b = s.b;
            return true;
        }
    return false;
}

void Game::resetRun() {
    px_ = 84.f;
    py_ = kDeck;
    vx_ = vy_ = 0;
    bannerX_ = kBanner;
    has_ = false;
    lives_ = kLives;
    watch_ = kWatch;
    inv_ = 0;
    safeX_ = px_;
    faceRight_ = true;
    grounded_ = true;
    playT_ = 0;
    looks_[0] = {280.f, 180.f, 460.f, 1.05f, 1.f};
    looks_[1] = {1560.f, 1420.f, 1720.f, 1.0f, -1.f};
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    blip(392.f, 0.05f, 0.08f);
}

void Game::win() {
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = 0;
    blip(698.f, 0.07f, 0.25f);
    sys_->setLight(90, 40, 20);
}

void Game::lose() {
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    blip(80.f, 0.06f, 0.3f);
    sys_->setLight(40, 8, 12);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleHold_ = 0;
    t_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) return 4;
    if (has_ && px_ < 980.f) return 3;
    if (has_) return 2;
    return 1;
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    const float goal = has_ ? 72.f : bannerX_;
    if (px_ + 6.f < goal) right = true;
    else if (px_ - 6.f > goal) left = true;
    if (!grounded_) return;
    float a = 0, b = 0;
    if (spanOf(px_, a, b)) {
        if (right && px_ > b - 40.f && px_ < b - 14.f) jump = true;
        if (left && px_ < a + 40.f && px_ > a + 14.f) jump = true;
    }
    for (const Lookout& w : looks_) {
        float d = w.x - px_;
        if (right && d > 10.f && d < 48.f) jump = true;
        if (left && d < -10.f && d > -48.f) jump = true;
    }
}

void Game::stepPlay(bool left, bool right, bool jump) {
    playT_ += 1.f / 60.f;
    watch_ -= 1.f / 60.f;
    if (watch_ <= 0.f) {
        watch_ = 0;
        lose();
        return;
    }
    if (inv_ > 0) inv_ -= 1.f / 60.f;

    for (Lookout& w : looks_) {
        w.x += w.dir * w.speed;
        if (w.x < w.minX) {
            w.x = w.minX;
            w.dir = 1;
        }
        if (w.x > w.maxX) {
            w.x = w.maxX;
            w.dir = -1;
        }
    }

    float spd = has_ ? kCarry : kWalk;
    if (left) {
        vx_ = -spd;
        faceRight_ = false;
    } else if (right) {
        vx_ = spd;
        faceRight_ = true;
    } else {
        vx_ = 0;
    }
    if (jump && grounded_) {
        vy_ = kJump;
        grounded_ = false;
        blip(480.f, 0.04f, 0.05f);
    }

    vy_ = std::min(vy_ + kGrav, 7.f);
    px_ += vx_;
    py_ += vy_;
    px_ = std::clamp(px_, 24.f, kWorld - 24.f);

    bool on = solidAt(px_) && vy_ >= 0.f && py_ >= kDeck - 0.5f && py_ <= kDeck + 14.f;
    if (on) {
        py_ = kDeck;
        vy_ = 0;
        grounded_ = true;
        safeX_ = px_;
    } else if (py_ >= kDeck && !solidAt(px_)) {
        grounded_ = false;
        if (py_ > kDeck + 40.f) {
            lives_--;
            if (has_) {
                has_ = false;
                bannerX_ = std::clamp(safeX_, 40.f, kWorld - 40.f);
            }
            px_ = safeX_;
            py_ = kDeck;
            vy_ = 0;
            vx_ = 0;
            grounded_ = true;
            inv_ = 1.1f;
            blip(120.f, 0.06f, 0.12f);
            sys_->rumble(0.4f, 0.2f, 90);
            if (lives_ <= 0) {
                lose();
                return;
            }
        }
    } else {
        grounded_ = false;
    }

    if (inv_ <= 0.f && grounded_ && py_ > kDeck - 30.f) {
        for (const Lookout& w : looks_) {
            if (std::fabs(w.x - px_) < 16.f) {
                lives_--;
                if (has_) {
                    has_ = false;
                    bannerX_ = px_ + (faceRight_ ? -28.f : 28.f);
                    if (!solidAt(bannerX_)) bannerX_ = safeX_;
                }
                vx_ = faceRight_ ? -3.f : 3.f;
                vy_ = -2.4f;
                grounded_ = false;
                inv_ = 1.3f;
                blip(150.f, 0.06f, 0.1f);
                sys_->rumble(0.5f, 0.3f, 100);
                if (lives_ <= 0) lose();
                return;
            }
        }
    }

    if (!has_ && grounded_ && std::fabs(px_ - bannerX_) < 22.f) {
        has_ = true;
        blip(740.f, 0.06f, 0.1f);
        sys_->rumble(0.2f, 0.4f, 70);
        sys_->setLight(140, 50, 20);
    }

    if (has_ && px_ <= kHome && grounded_) win();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.y > gs::SCREEN_H + 48 || s.x + s.w < -48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 100) {
            int t = y * 6 / 100;
            vdp.lineBackdrop[y] = gs::rgb4(4 + t / 3, 2 + t / 4, 6 + t / 2);
        } else if (y < 160) {
            vdp.lineBackdrop[y] = gs::rgb4(5, 4, 6);
        } else {
            int d = (y - 160) * 8 / 64;
            vdp.lineBackdrop[y] = gs::rgb4(2, 1 + d / 6, 3);
        }
        vdp.lineFog[y] = uint8_t(y > 180 ? (y - 180) / 6 : 0);
        vdp.road[y].on = false;
    }

    auto world = [&](float wx) { return wx - cam_; };

    float drift = std::fmod(t_ * 10.f, 56.f);
    for (int i = 0; i < 8; i++) {
        float wx = cam_ - 30.f + i * 56.f - drift;
        spr(art_.mist, world(wx), 188.f + std::sin(t_ * 1.4f + i) * 3.f, 14.f, PAL_MIST, false, false);
    }

    for (const Span& s : kSpans) {
        for (float x = s.a + 28.f; x < s.b - 8.f; x += 48.f) {
            spr(art_.pier, world(x), kDeck + 10.f, 70.f, PAL_STONE, false, false);
            spr(art_.arch, world(x + 22.f), kDeck + 16.f, 36.f, PAL_GORGE, false, false);
            spr(art_.ashlar, world(x), kDeck + 2.f, 16.f, PAL_STONE, false, false);
        }
        if (s.b - s.a > 240.f) spr(art_.lamp, world((s.a + s.b) * 0.5f), kDeck, 34.f, PAL_LAMP, false, true);
    }
    spr(art_.abutment, world(46.f), kDeck + 6.f, 78.f, PAL_STONE, false, true);
    spr(art_.abutment, world(2448.f), kDeck + 6.f, 78.f, PAL_STONE, true, true);

    if (!has_) {
        const gs::Mipped& cloth = (int(t_ * 4.f) & 1) ? art_.bannerB : art_.banner;
        spr(cloth, world(bannerX_), kDeck - 2.f, 48.f, PAL_BANNER, false, true);
    }

    for (const Lookout& w : looks_) {
        bool step = int(t_ * 6.f + w.x) & 1;
        spr(art_.look[step ? 1 : 0], world(w.x), kDeck, 50.f, PAL_LOOK, w.dir < 0, true);
    }

    int flick = inv_ > 0.f && (int(t_ * 20.f) & 1);
    if (!flick) {
        spr(hero(), world(px_), py_, 52.f, PAL_COAT, !faceRight_, true);
        if (has_) {
            const gs::Mipped& cloth = (int(t_ * 6.f) & 1) ? art_.bannerB : art_.banner;
            spr(cloth, world(px_ + (faceRight_ ? 12.f : -12.f)), py_ - 34.f, 32.f, PAL_BANNER, !faceRight_, true);
        }
    }

    char buf[64];
    std::snprintf(buf, sizeof(buf), "WATCH %02d", int(std::ceil(watch_)));
    hud(1, 1, buf, PAL_HUD);
    std::snprintf(buf, sizeof(buf), "LIVES %d", lives_);
    hud(30, 1, buf, PAL_HUD);
    if (has_) hud(15, 1, "BANNER", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("VIADUCT", 160, 46, 1.4f, PAL_HUD, 0);
        hudC(12, "BRING THE BANNER BACK", PAL_HUD);
        hudC(14, "ANYTHING ELSE IS A LOSS", PAL_HUD);
        hudC(18, "A JUMP    ARROWS RUN", PAL_HUD);
        hudC(22, "START", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("BANNER HOME", 160, 40, 1.3f, PAL_HUD, 0);
        hudC(12, "THE VIADUCT HOLDS IT", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        text("LOSS", 160, 40, 1.4f, PAL_HUD, 0);
        hudC(12, "THE BANNER IS NOT BACK", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    if (toneT_ > 0) {
        toneT_ -= 1.f / 60.f;
        if (toneT_ <= 0) sys.apu.tone(0, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
    bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, jump);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump);
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && start) {
            mode_ = Mode::Title;
            titleHold_ = 0;
            over_ = false;
            won_ = false;
            resetRun();
        }
    }

    float want = std::clamp(px_ - 150.f, 0.f, kWorld - float(gs::SCREEN_W));
    cam_ += (want - cam_) * 0.12f;
    draw();
}

}  // namespace viaductbann
