#include "game/archbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace archbell {
namespace {

constexpr int kFullDraw = 22;
constexpr int kFirm = 10;
constexpr int kFlight = 14;
constexpr float kAim = 2.4f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

bool Game::audit() const {
    auto at = [](float x, float y, Bed bed, const char* name) {
        Mark m = classify(x, y);
        if (m.bed != bed || std::strcmp(m.name, name) != 0) {
            std::fprintf(stderr, "s3archbell %s scored %s at %.2f %.2f\n", name, m.name, x, y);
            return false;
        }
        return true;
    };
    if (!at(kCx, kCy, Bed::Bell, "BELL")) return false;
    if (!at(kCx, kCy - (kBellR + kGold) * 0.5f, Bed::Gold, "GOLD")) return false;
    if (!at(kCx, kCy - (kGold + kRed) * 0.5f, Bed::Red, "RED")) return false;
    if (!at(kCx, kCy - (kRed + kBlue) * 0.5f, Bed::Blue, "BLUE")) return false;
    if (!at(kCx, kCy - (kBlue + kBlack) * 0.5f, Bed::Black, "BLACK")) return false;
    if (!at(kCx, kCy - (kBlack + kWhite) * 0.5f, Bed::White, "WHITE")) return false;
    if (!at(kCx, kCy - (kWhite + kBoss) * 0.5f, Bed::Straw, "STRAW")) return false;
    if (classify(kCx, kCy - (kBoss + 6.f)).bed != Bed::Miss) {
        std::fprintf(stderr, "s3archbell grass scored\n");
        return false;
    }
    Mark lip = classify(kCx, kCy - (kBellR + 1.5f));
    if (lip.bed == Bed::Bell) {
        std::fprintf(stderr, "s3archbell gold still on the bell\n");
        return false;
    }
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3archbell rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.72f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    why_ = Death::None;
    last_[0] = 0;
    bellAmp_ = 0.15f;
    bellPh_ = 0;
    t_ = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    drawTick_ = 0;
    steady_ = 0;
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    why_ = Death::None;
    last_[0] = 0;
    bellAmp_ = 0.15f;
    aimX_ = kCx;
    aimY_ = kCy;
    beginAim();
}

void Game::beginAim() {
    drawTick_ = 0;
    steady_ = 0;
    skipHold_ = !bot_;
    if (bot_) {
        aimX_ = kCx;
        aimY_ = kCy;
    }
    mode_ = Mode::Aim;
}

void Game::enterNock() {
    mode_ = Mode::Nock;
    drawTick_ = 0;
    steady_ = 0;
    blip(0, 180.f, 0.03f, 4);
}

bool Game::drawHeld() const {
    if (!sys_) return false;
    if (sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_C)) return true;
    return sys_->pad.accel > 0.45f;
}

void Game::steer() {
    if (!sys_) return;
    float x = 0.f;
    float y = 0.f;
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) x -= 1.f;
    if (p.down(gs::BTN_RIGHT)) x += 1.f;
    if (p.down(gs::BTN_UP)) y -= 1.f;
    if (p.down(gs::BTN_DOWN)) y += 1.f;
    if (std::fabs(p.axisX) > 0.2f) x = p.axisX;
    if (std::fabs(p.axisY) > 0.2f) y = -p.axisY;
    float m = std::hypot(x, y);
    if (m > 1.f) {
        x /= m;
        y /= m;
    }
    aimX_ = clampf(aimX_ + x * kAim, kCx - 96.f, kCx + 80.f);
    aimY_ = clampf(aimY_ + y * kAim, kCy - 88.f, kCy + 88.f);
    if (p.pressed(gs::BTN_B)) {
        aimX_ = kCx;
        aimY_ = kCy;
    }
}

void Game::nockTick() {
    if (bot_) {
        if (drawTick_ >= kFullDraw) {
            loose();
            return;
        }
        drawTick_++;
        if (drawTick_ == kFullDraw) blip(0, 880.f, 0.05f, 5);
        return;
    }
    if (!drawHeld()) {
        loose();
        return;
    }
    if (drawTick_ < kFullDraw) {
        drawTick_++;
        if (drawTick_ == kFullDraw) blip(0, 880.f, 0.05f, 5);
    } else if (steady_ < 48) {
        steady_++;
    }
}

void Game::loose() {
    if (drawTick_ < kFullDraw) fate_ = Fate::Short;
    else if (steady_ <= kFirm) fate_ = Fate::True;
    else fate_ = Fate::Hot;

    if (fate_ == Fate::Short) {
        landX_ = kCx + (aimX_ - kCx) * 0.15f;
        landY_ = kCy + kFaceMid + 16.f;
    } else if (fate_ == Fate::True) {
        landX_ = aimX_;
        landY_ = aimY_;
    } else {
        float dx = aimX_ - kCx;
        float dy = aimY_ - kCy;
        float d = std::hypot(dx, dy);
        float dirx = 0.f;
        float diry = -1.f;
        if (d > 0.5f) {
            dirx = dx / d;
            diry = dy / d;
        }
        float push = (kGold + 8.f) + float(steady_ - kFirm) * 1.4f;
        landX_ = aimX_ + dirx * push;
        landY_ = aimY_ + diry * push;
    }
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (sys_) {
        sys_->apu.tone(2, 0, 0);
        sys_->apu.noiseBurst(0.12f, 1800.f, 0.04f);
        sys_->rumble(0.1f, 0.18f, 24);
    }
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Flight) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    bellAmp_ = 1.f;
    bellTick_ = 1;
    mode_ = Mode::Ring;
    std::snprintf(last_, sizeof last_, "BELL");
    if (!sys_) return;
    sys_->rumble(0.35f, 0.8f, 160);
    sys_->setLight(255, 196, 48);
}

void Game::dieTry(Death why) {
    if (rung_ || mode_ != Mode::Flight) return;
    why_ = why;
    dead_++;
    if (why == Death::Short) std::snprintf(last_, sizeof last_, "SHORT");
    else if (why == Death::Hot) std::snprintf(last_, sizeof last_, "HOT");
    else {
        Mark m = classify(landX_, landY_);
        std::snprintf(last_, sizeof last_, "%s", m.name);
    }
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        if (sys_) {
            blip(0, 90.f, 0.1f, 24);
            sys_->setLight(150, 28, 28);
        }
    } else {
        mode_ = Mode::Dead;
        if (sys_) {
            blip(0, 140.f, 0.08f, 12);
            sys_->setLight(120, 48, 28);
        }
    }
}

void Game::arrive() {
    if (mode_ != Mode::Flight) return;
    Mark m = classify(landX_, landY_);
    if (fate_ == Fate::True && rules_ && m.bed == Bed::Bell) {
        ring();
        return;
    }
    Death why = Death::Wide;
    if (fate_ == Fate::Short) why = Death::Short;
    else if (fate_ == Fate::Hot) why = Death::Hot;
    if (why == Death::Wide && sys_) sys_->apu.noiseBurst(0.1f, 220.f, 0.05f);
    dieTry(why);
}

void Game::blip(int ch, float freq, float vol, int frames) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 0) blipLeft_ = frames;
    else tone2Left_ = frames;
}

void Game::pumpAudio() {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming) {
        if (--bellTick_ <= 0 && bellAmp_ > 0.4f) {
            float vol = 0.05f + 0.08f * bellAmp_;
            sys_->apu.tone(0, 880.f, vol);
            sys_->apu.tone(1, 1318.f, vol * 0.65f);
            blipLeft_ = 8;
            bellTick_ = 14;
        }
    }
    if (blipLeft_ > 0 && --blipLeft_ == 0) {
        sys_->apu.tone(0, 0, 0);
        if (!chiming) sys_->apu.tone(1, 0, 0);
    }
    if (tone2Left_ > 0 && --tone2Left_ == 0) sys_->apu.tone(2, 0, 0);
    if (mode_ == Mode::Nock) {
        float p = std::min(1.f, float(drawTick_) / float(kFullDraw));
        sys_->apu.tone(2, 90.f + p * 160.f, 0.02f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    bellPh_ += rung_ ? 0.42f : 0.04f;
    if (rung_) bellAmp_ = std::max(0.16f, bellAmp_ - 0.008f);

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
    if (bot_) start = false;

    if (mode_ == Mode::Title) {
        if (bot_ && t_ > 18) newGame();
        else if (!bot_ && p.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (!bot_ && start) {
            newGame();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) newGame();
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            float dx = kCx - aimX_;
            float dy = kCy - aimY_;
            float d = std::hypot(dx, dy);
            if (d > 0.4f) {
                float step = std::min(4.f, d);
                aimX_ += dx / d * step;
                aimY_ += dy / d * step;
            } else {
                aimX_ = kCx;
                aimY_ = kCy;
                enterNock();
            }
        } else {
            steer();
            if (skipHold_) {
                if (!drawHeld()) skipHold_ = false;
            } else if (drawHeld()) {
                enterNock();
            }
        }
    } else if (mode_ == Mode::Nock) {
        nockTick();
    } else if (mode_ == Mode::Flight) {
        if (++flightT_ >= kFlight) arrive();
    } else if (mode_ == Mode::Dead) {
        if (t_ % 48 == 0) beginAim();
    } else if (mode_ == Mode::Ring) {
        if (t_ > 0 && (t_ % 50) == 0) mode_ = Mode::Leave;
    } else if (mode_ == Mode::Leave) {
        if (t_ > 0 && (t_ % 40) == 0) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }

    // Dead/Ring/Leave used t_ modulo, which can fire the same frame they start.
    // Gate those waits with dedicated counters stored in flightT_ after the shot.
    pumpAudio();
    draw();
}

void Game::stamp(const gs::Image& img, float x, float y, float w, float h, int pal, bool shadow) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = img;
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::at(const gs::Image& img, float cx, float cy, int pal) {
    stamp(img, cx - float(img.w) * 0.5f, cy - float(img.h) * 0.5f, float(img.w), float(img.h), pal);
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
    int n = 0;
    if (s)
        while (s[n]) n++;
    int col = (40 - n) / 2;
    if (col < 0) col = 0;
    hud(col, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 128) {
            float u = float(y) / 128.f;
            v.lineBackdrop[y] = gs::rgb4(3 + int(u * 6.f), 5 + int(u * 5.f), 12 - int(u * 3.f));
        } else if (y < 168) {
            v.lineBackdrop[y] = gs::rgb4(3, 8, 3);
        } else {
            v.lineBackdrop[y] = (y & 1) ? gs::rgb4(2, 6, 2) : gs::rgb4(3, 8, 3);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) {
        stamp(art_.word, 78.f, 28.f, float(art_.word.w), float(art_.word.h), PAL_WORD);
        hudC(8, "A SHORT ARCH", PAL_INK);
        hudC(20, "HOLD A   LOOSE ON THE BELL", PAL_GOLD);
        hudC(22, "THREE TRIES", PAL_INK);
    }

    stamp(art_.stand, kCx - 8.f, kCy + kFaceMid - 18.f, float(art_.stand.w), float(art_.stand.h), PAL_WORLD);
    stamp(art_.face, kCx - kFaceMid, kCy - kFaceMid, float(kFace), float(kFace), PAL_FACE);

    float swing = std::sin(bellPh_) * bellAmp_ * 7.f;
    at(art_.bell, kCx + swing, kCy + 2.f, PAL_BELL);

    stamp(art_.archer, kArchX, kArchY, float(art_.archer.w), float(art_.archer.h), PAL_ARCH);

    if (mode_ == Mode::Flight) {
        float u = std::min(1.f, float(flightT_) / float(kFlight));
        float x = kLooseX + (landX_ - kLooseX) * u;
        float y = kLooseY + (landY_ - kLooseY) * u;
        y -= std::sin(u * 3.14159265f) * 10.f;
        stamp(art_.arrow, x - float(art_.arrow.w), y - 3.f, float(art_.arrow.w), float(art_.arrow.h), PAL_ARROW);
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Nock) at(art_.sight, aimX_, aimY_, PAL_SIGHT);

    stamp(art_.quiver, 8.f, 150.f, float(art_.quiver.w), float(art_.quiver.h), PAL_WORLD);
    for (int i = 0; i < 3; i++) {
        bool live = i >= dead_ && !(mode_ == Mode::Over && !won_);
        if (!live) continue;
        stamp(art_.shaft, 14.f, 146.f - float(i) * 6.f, float(art_.shaft.w), float(art_.shaft.h), PAL_ARROW);
    }

    if (mode_ == Mode::Nock) {
        float power = std::min(1.f, float(drawTick_) / float(kFullDraw));
        uint16_t col = steady_ > kFirm ? gs::rgb4(14, 3, 2) : (power >= 1.f ? gs::rgb4(6, 14, 4) : gs::rgb4(15, 13, 6));
        v.setColor(PAL_GOLD * 16 + 2, col);
        gs::Sprite bar;
        bar.x = 8;
        bar.y = 196;
        bar.w = int16_t(std::lround(96.f * power));
        bar.h = 6;
        bar.img = art_.shaft;
        bar.pal = PAL_GOLD;
        if (bar.w > 0) v.sprite(bar);
    }

    if (mode_ != Mode::Title) {
        int shown = dead_ + (rung_ ? 1 : 0);
        if (shown < 1) shown = (mode_ == Mode::Aim || mode_ == Mode::Nock || mode_ == Mode::Flight) ? dead_ + 1 : dead_;
        if (shown > 3) shown = 3;
        char line[40];
        std::snprintf(line, sizeof line, "TRY %d/3", shown);
        hud(1, 1, line, PAL_INK);
        if (last_[0]) {
            int pal = rung_ ? PAL_GOLD : PAL_ALERT;
            hud(28, 1, last_, pal);
        }
    }
    if (mode_ == Mode::Ring || (mode_ == Mode::Over && won_) || mode_ == Mode::Leave)
        hudC(24, "THE BELL RINGS", PAL_GOLD);
    if (mode_ == Mode::Over && !won_) hudC(24, "THIRD TRY DIED", PAL_ALERT);
    if (mode_ == Mode::Nock && steady_ > kFirm) hudC(24, "HOT", PAL_ALERT);
    else if (mode_ == Mode::Nock && drawTick_ >= kFullDraw) hudC(24, "LOOSE", PAL_GREEN);
}

}  // namespace archbell
