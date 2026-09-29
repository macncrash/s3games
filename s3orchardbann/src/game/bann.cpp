#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace orchardbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kWorld = 2280.0f;
constexpr float kFloor = 188.0f;
constexpr float kHome = 168.0f;
constexpr float kSpawn = 250.0f;
constexpr float kBanner0 = 2060.0f;
constexpr float kMinX = 80.0f;
constexpr float kMaxX = 2160.0f;
constexpr float kRun = 176.0f;
constexpr float kCarry = 138.0f;
constexpr float kAccel = 2600.0f;
constexpr float kBody = 22.0f;
constexpr float kStrike = 60.0f;
constexpr float kGrab = 32.0f;
constexpr float kTrees[] = {520.0f, 900.0f, 1280.0f, 1660.0f, 1980.0f};

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (swing_ > 0) return art_.swing;
    if (std::abs(vx_) > 18.0f) return (int(step_ / 7.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1200.0f) return 2;
    return 3;
}

void Game::resetRun() {
    auto set = [&](int i, float a, float b, float sp) {
        Keeper& w = keep_[i];
        w.minX = a;
        w.maxX = b;
        w.x = (a + b) * 0.5f;
        w.speed = sp;
        w.dir = (i & 1) ? -1.0f : 1.0f;
        w.stun = 0;
    };
    set(0, 430.0f, 760.0f, 34.0f);
    set(1, 980.0f, 1320.0f, 36.0f);
    set(2, 1480.0f, 1840.0f, 32.0f);
    px_ = kSpawn;
    vx_ = 0;
    face_ = 1;
    has_ = false;
    bannerX_ = kBanner0;
    lives_ = 5;
    step_ = strikeCd_ = swing_ = inv_ = stun_ = dropLock_ = shake_ = 0;
    playT_ = 0;
    appleOn_ = false;
    appleN_ = 0;
    appleX_ = appleY_ = appleVy_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(520.0f, 0.05f, 0.07f);
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = 0;
    shake_ = 0.3f;
    sys_->rumble(0.25f, 0.55f, 180);
    sys_->setLight(40, 150, 40);
    blip(698.0f, 0.08f, 0.22f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    sys_->rumble(0.6f, 0.2f, 160);
    sys_->setLight(140, 30, 16);
    sys_->apu.noiseBurst(0.32f, 120.0f, 0.22f);
}

void Game::stumble(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.1f;
    stun_ = 0.22f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 140.0f;
    px_ = std::clamp(px_ + away * 10.0f, kMinX, kMaxX);
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.3f, 360.0f, 0.1f);
    sys_->rumble(0.5f, 0.2f, 80);
    if (has_) {
        has_ = false;
        bannerX_ = std::clamp(px_, 360.0f, 1900.0f);
        dropLock_ = 0.5f;
    }
    if (lives_ <= 0) lose();
}

void Game::spawnApple() {
    appleOn_ = true;
    appleX_ = kTrees[appleN_ % 5];
    appleN_++;
    appleY_ = 48.0f;
    appleVy_ = 20.0f;
}

void Game::bot(bool& left, bool& right, bool& strike) {
    left = right = strike = false;
    const float goal = has_ ? (kHome - 20.0f) : bannerX_;
    int travel = 0;
    if (goal > px_ + 6.0f) travel = 1;
    else if (goal < px_ - 6.0f) travel = -1;

    if (appleOn_ && appleY_ > 70.0f && appleY_ < kFloor - 6.0f) {
        float rel = appleX_ - px_;
        if (std::abs(rel) < 30.0f) {
            if (rel >= 0.0f) left = true;
            else right = true;
            face_ = left ? -1 : 1;
            return;
        }
        float ahead = travel == 0 ? 99.0f : (travel > 0 ? rel : -rel);
        if (ahead > 0.0f && ahead < 48.0f && appleY_ > 110.0f) return;
    }

    int threat = -1;
    float best = 1.0e9f;
    for (int i = 0; i < 3; i++) {
        const Keeper& w = keep_[i];
        if (w.stun > 0) continue;
        float rel = w.x - px_;
        float ahead = travel == 0 ? std::abs(rel) : (travel > 0 ? rel : -rel);
        if (ahead < -18.0f || ahead > 78.0f) continue;
        if (std::abs(rel) < best) {
            best = std::abs(rel);
            threat = i;
        }
    }
    if (threat >= 0 && strikeCd_ <= 0.0f) {
        float rel = keep_[threat].x - px_;
        face_ = rel >= 0.0f ? 1 : -1;
        strike = true;
        return;
    }
    if (threat >= 0 && best < 46.0f) {
        if (keep_[threat].x >= px_) left = true;
        else right = true;
        face_ = left ? -1 : 1;
        return;
    }
    if (travel > 0) {
        right = true;
        face_ = 1;
    } else if (travel < 0) {
        left = true;
        face_ = -1;
    }
}

void Game::stepPlay(bool left, bool right, bool strike) {
    playT_ += DT;
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (dropLock_ > 0) dropLock_ -= DT;
    if (strikeCd_ > 0) strikeCd_ -= DT;
    if (swing_ > 0) swing_ -= DT;

    if (!appleOn_) {
        if (playT_ > 0.8f && std::fmod(playT_, 1.35f) < DT) spawnApple();
    } else {
        appleVy_ += 520.0f * DT;
        appleY_ += appleVy_ * DT;
        if (appleY_ >= kFloor - 4.0f) appleOn_ = false;
        else if (inv_ <= 0 && stun_ <= 0 && std::abs(appleX_ - px_) < 16.0f && appleY_ > kFloor - 36.0f)
            stumble(appleX_);
    }

    for (Keeper& w : keep_) {
        if (w.stun > 0) {
            w.stun -= DT;
            continue;
        }
        w.x += w.dir * w.speed * DT;
        if (w.x >= w.maxX) {
            w.x = w.maxX;
            w.dir = -1.0f;
        } else if (w.x <= w.minX) {
            w.x = w.minX;
            w.dir = 1.0f;
        }
    }

    if (stun_ > 0) {
        vx_ = approach(vx_, 0.0f, 480.0f * DT);
    } else {
        float target = 0;
        if (right && !left) target = has_ ? kCarry : kRun;
        if (left && !right) target = has_ ? -kCarry : -kRun;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    }
    px_ += vx_ * DT;
    px_ = std::clamp(px_, kMinX, kMaxX);
    if (std::abs(vx_) > 24.0f) step_ += std::abs(vx_) * DT * 0.18f;

    if (has_ && px_ <= kHome) {
        win();
        return;
    }

    if (strike && strikeCd_ <= 0 && stun_ <= 0) {
        strikeCd_ = 0.26f;
        swing_ = 0.12f;
        bool hit = false;
        for (Keeper& w : keep_) {
            if (w.stun > 0) continue;
            float rel = w.x - px_;
            bool struck = face_ > 0 ? (rel > -12.0f && rel < kStrike) : (rel < 12.0f && rel > -kStrike);
            if (!struck) continue;
            w.stun = 1.85f;
            float shove = face_ > 0 ? 90.0f : -90.0f;
            w.x = std::clamp(w.x + shove, w.minX, w.maxX);
            hit = true;
        }
        if (hit) {
            blip(170.0f, 0.06f, 0.05f);
            sys_->apu.noiseBurst(0.14f, 700.0f, 0.05f);
        } else {
            blip(240.0f, 0.03f, 0.03f);
        }
    }

    if (inv_ <= 0 && stun_ <= 0) {
        for (const Keeper& w : keep_) {
            if (w.stun > 0) continue;
            if (std::abs(w.x - px_) < kBody) {
                stumble(w.x);
                break;
            }
        }
    }

    if (!has_ && dropLock_ <= 0 && stun_ <= 0 && std::abs(px_ - bannerX_) < kGrab) {
        has_ = true;
        blip(740.0f, 0.07f, 0.1f);
        sys_->rumble(0.2f, 0.4f, 80);
        sys_->setLight(160, 90, 20);
    }
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.y > gs::SCREEN_H + 40 || s.x + s.w < -80 || s.y + s.h < -40) return;
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
        spr(g, x + g.w * scale * 0.5f, y, float(g.h) * scale, pal, false, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float view = cam_;
    if (shake_ > 0.02f) view += std::sin(t_ * 46.0f) * shake_ * 3.0f;
    vdp.A.scroll(int(view * 0.22f), 0);
    vdp.B.scroll(int(view * 0.9f), 0);

    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true);
    };

    for (float tx : kTrees) world(art_.tree, tx, kFloor + 6.0f, 108, PAL_LEAF, false);
    world(art_.gate, 120.0f, kFloor + 4.0f, 92, PAL_WOOD, false);

    if ((inv_ <= 0) || (int(t_ * 16.0f) & 1)) {
        world(art_.shadow, px_, kFloor + 2.0f, 8, PAL_EARTH, false);
        world(hero(), px_, kFloor, 64, PAL_PICK, face_ < 0);
        world(art_.basket, px_ + face_ * 14.0f, kFloor - 18.0f, 14, PAL_WOOD, face_ < 0);
        if (has_) world(art_.banner, px_ + face_ * 18.0f, kFloor - 30.0f, 36, PAL_BANNER, face_ < 0);
    }
    for (const Keeper& w : keep_) {
        bool step = int(playT_ * 5.0f + w.x * 0.02f) & 1;
        world(art_.shadow, w.x, kFloor + 2.0f, 8, PAL_EARTH, false);
        world(step && w.stun <= 0 ? art_.scare[1] : art_.scare[0], w.x, w.stun > 0 ? kFloor - 6.0f : kFloor, 70,
              PAL_SCARE, w.dir < 0);
    }
    if (!has_) {
        float wave = std::sin(t_ * 2.6f) * 3.0f;
        world(art_.banner, bannerX_ + wave, kFloor - 52.0f, 42, PAL_BANNER, false);
    }
    if (appleOn_) spr(art_.apple, appleX_ - view, appleY_, 16, PAL_APPLE, false, false);

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    if (has_) hud(26, 1, "BANNER", PAL_BANNER);
    else hud(27, 1, "ORCHARD", PAL_HUD);

    if (mode_ == Mode::Title) {
        text("ORCHARD BANN", 160, 58, 1.05f, PAL_HUD, 0);
        text("BRING THE BANNER BACK", 160, 88, 0.5f, PAL_BANNER, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 132, 0.65f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 90, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 72, 0.65f, PAL_BANNER, 0);
    } else if (mode_ == Mode::Over) {
        text("THE ORCHARD KEPT IT", 160, 72, 0.6f, PAL_HUD, 0);
        text("START TO TRY AGAIN", 160, 108, 0.48f, PAL_LEAF, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleHold_ = 0;
    t_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
    bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
    bool strike = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, strike);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, strike);
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && start) {
            mode_ = Mode::Title;
            titleHold_ = 0;
            over_ = false;
            won_ = false;
            resetRun();
        }
    }

    float want = std::clamp(px_ - 150.0f, 0.0f, kWorld - float(gs::SCREEN_W));
    cam_ = approach(cam_, want, 480.0f * DT);
    draw();
}

}  // namespace orchardbann
