#include "game/breach.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace breach {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float kFloor = 190.0f;
constexpr float kWorld = 1760.0f;
constexpr float kSpawn = 118.0f;
constexpr float kExit = 68.0f;
constexpr float kBanner0 = 1664.0f;
constexpr float kHalf = 5.0f;
constexpr float kTall = 36.0f;
constexpr float kDuckH = 15.0f;
constexpr float kGrav = 1100.0f;
constexpr float kRun = 150.0f;
constexpr float kCarry = 108.0f;
constexpr float kJump = -400.0f;
constexpr float kCarryJump = -360.0f;
constexpr float kMaxFall = 520.0f;
constexpr float kBeam0 = 650.0f;
constexpr float kBeam1 = 800.0f;
constexpr float kBeamTop = 148.0f;
constexpr float kBeamBot = 168.0f;
constexpr float kAlarm = 42.0f;
constexpr float kGHalf = 6.0f;
constexpr float kGTall = 28.0f;
constexpr float kAccel = 1500.0f;

struct Crate {
    float x, w, h;
};

constexpr Crate kCrates[] = {
    {460.0f, 52.0f, 26.0f},
    {1240.0f, 48.0f, 28.0f},
};

constexpr float kColumns[] = {200, 520, 860, 1180, 1500};
constexpr float kWindows[] = {300, 700, 1040, 1380};
constexpr float kTapestries[] = {380, 960, 1320};

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

bool overlapX(float px, float half, float left, float right) {
    return px + half > left && px - half < right;
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (ducked_) return art_.duck;
    if (!grounded_) return art_.air;
    if (std::abs(vx_) > 16.0f) return (int(step_ / 7.0f) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::noteOff() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
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

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 64 || s.y + s.h < -64) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::world(const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet) {
    spr(m, wx - cam_, foot, h, pal, flip, feet, false);
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
        float h = g.h * scale;
        spr(g, x + g.w * scale * 0.5f, y, h, pal, false, false, false);
        x += g.w * scale + scale;
    }
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (px_ > 1200.0f) return 2;
    return 3;
}

void Game::resetRun() {
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    face_ = 1;
    grounded_ = true;
    ducked_ = false;
    has_ = false;
    alarmOn_ = false;
    alarm_ = kAlarm;
    bannerX_ = kBanner0;
    lives_ = 3;
    hits_ = 0;
    inv_ = stun_ = dropLock_ = 0;
    coyote_ = 0.12f;
    jumpBuf_ = 0;
    step_ = 0;
    shake_ = 0;
    fan_ = -1;
    guards_[0] = {200.0f, 150.0f, 310.0f, 18.0f, 1.0f};
    guards_[1] = {1040.0f, 900.0f, 1080.0f, 20.0f, -1.0f};
    guards_[2] = {1460.0f, 1360.0f, 1540.0f, 18.0f, 1.0f};
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    cam_ = std::clamp(px_ - 140.0f, 0.0f, kWorld - gs::SCREEN_W);
    blip(520.0f, 0.05f, 0.06f);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = 0;
    vy_ = 0;
    fan_ = 0;
    fanT_ = 0;
    shake_ = 0;
    sys_->rumble(0.3f, 0.7f, 200);
    sys_->setLight(40, 180, 70);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = 0;
    fan_ = 0;
    fanT_ = 0;
    sys_->rumble(0.8f, 0.3f, 180);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.4f, 180.0f, 0.25f);
}

void Game::hurt(float fromX) {
    if (inv_ > 0 || mode_ != Mode::Play) return;
    hits_++;
    lives_--;
    inv_ = 1.2f;
    stun_ = 0.32f;
    float away = px_ < fromX ? -1.0f : 1.0f;
    vx_ = away * 170.0f;
    vy_ = -220.0f;
    grounded_ = false;
    shake_ = 1.0f;
    sys_->apu.noiseBurst(0.45f, 520.0f, 0.16f);
    sys_->rumble(0.7f, 0.4f, 120);
    sys_->setLight(200, 40, 30);
    if (has_) {
        has_ = false;
        bannerX_ = std::clamp(px_ + away * 28.0f, 96.0f, kWorld - 48.0f);
        dropLock_ = 0.45f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& down, bool& jump) {
    const float goal = has_ ? 40.0f : bannerX_;
    const float dir = goal > px_ + 4.0f ? 1.0f : goal < px_ - 4.0f ? -1.0f : 0.0f;
    if (dir > 0) right = true;
    if (dir < 0) left = true;
    face_ = dir != 0 ? int(dir) : face_;

    const bool duckZone = px_ > kBeam0 - 20.0f && px_ < kBeam1 + 20.0f && py_ > kBeamBot - 10.0f;
    if (duckZone) {
        down = true;
        return;
    }
    if (!grounded_ || dir == 0.0f) return;

    const float speed = has_ ? kCarry : kRun;
    const float travel = has_ ? 74.0f : 112.0f;
    bool want = false;
    for (const Crate& c : kCrates) {
        if (py_ <= kFloor - c.h + 3.0f && px_ > c.x - 2.0f && px_ < c.x + c.w + 2.0f) continue;
        const float near = dir > 0 ? c.x : c.x + c.w;
        const float gap = (near - px_) * dir;
        if (gap > 16.0f && gap < 46.0f) want = true;
        if (std::abs(vx_) < 12.0f && gap > -2.0f && gap < 14.0f && py_ > kFloor - c.h - 2.0f) want = true;
    }
    for (const Guard& g : guards_) {
        const float gap = (g.x - px_) * dir;
        if (gap > 18.0f && gap < 44.0f) want = true;
    }
    if (want) {
        const float land = px_ + dir * travel;
        if (land > kBeam0 - 8.0f && land < kBeam1 + 8.0f) want = false;
    }
    (void)speed;
    jump = want;
}

void Game::stepPlay(float dt, bool left, bool right, bool down, bool jump) {
    if (stun_ > 0) stun_ -= dt;
    if (inv_ > 0) inv_ -= dt;
    if (dropLock_ > 0) dropLock_ -= dt;

    for (Guard& g : guards_) {
        g.x += g.dir * g.speed * dt;
        if (g.x >= g.maxX) {
            g.x = g.maxX;
            g.dir = -1.0f;
        } else if (g.x <= g.minX) {
            g.x = g.minX;
            g.dir = 1.0f;
        }
    }

    if (!has_ && dropLock_ <= 0 && std::abs(px_ - bannerX_) < 24.0f && std::abs(py_ - kFloor) < 18.0f && stun_ <= 0) {
        has_ = true;
        if (!alarmOn_) {
            alarmOn_ = true;
            alarm_ = kAlarm;
        }
        blip(740.0f, 0.07f, 0.08f);
        sys_->rumble(0.2f, 0.45f, 80);
        sys_->apu.tone(2, 880.0f, 0.05f);
    }

    const bool canDuck = grounded_ || py_ >= kFloor - 2.0f;
    ducked_ = down && canDuck && stun_ <= 0;
    const bool inBeamX = px_ > kBeam0 && px_ < kBeam1;
    const bool wouldClip = inBeamX && (py_ - kTall) < kBeamBot && py_ > kBeamTop;
    if (wouldClip) ducked_ = true;
    const float height = ducked_ ? kDuckH : kTall;

    if (stun_ <= 0) {
        float target = 0;
        if (right && !left) target = has_ ? kCarry : kRun;
        if (left && !right) target = has_ ? -kCarry : -kRun;
        if ((right && !left) || (left && !right)) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * dt);
    } else {
        vx_ = approach(vx_, 0, 90.0f * dt);
    }

    if (jump && stun_ <= 0) jumpBuf_ = 0.12f;
    else jumpBuf_ = std::max(0.0f, jumpBuf_ - dt);

    vy_ = std::min(kMaxFall, vy_ + kGrav * dt);

    const float prevX = px_;
    px_ += vx_ * dt;

    auto pushX = [&](float left, float right, float top, float bot) {
        if (!overlapX(px_, kHalf, left, right)) return;
        const float head = py_ - height;
        if (head >= bot || py_ <= top + 1.0f) return;
        if (prevX + kHalf <= left + 0.4f) {
            px_ = left - kHalf;
            vx_ = std::min(vx_, 0.0f);
        } else if (prevX - kHalf >= right - 0.4f) {
            px_ = right + kHalf;
            vx_ = std::max(vx_, 0.0f);
        }
    };
    for (const Crate& c : kCrates) pushX(c.x, c.x + c.w, kFloor - c.h, kFloor);
    pushX(kBeam0, kBeam1, kBeamTop, kBeamBot);

    const float prevY = py_;
    py_ += vy_ * dt;
    grounded_ = false;
    for (const Crate& c : kCrates) {
        const float top = kFloor - c.h;
        if (!overlapX(px_, kHalf - 1.0f, c.x, c.x + c.w)) continue;
        if (prevY <= top + 1.0f && py_ >= top && vy_ >= 0) {
            py_ = top;
            vy_ = 0;
            grounded_ = true;
        }
    }
    if (py_ >= kFloor) {
        py_ = kFloor;
        if (vy_ > 0) vy_ = 0;
        grounded_ = true;
    }
    if (overlapX(px_, kHalf, kBeam0, kBeam1)) {
        const float head = py_ - height;
        const float prevHead = prevY - height;
        if (head < kBeamBot && py_ > kBeamTop && prevHead >= kBeamBot - 0.8f && vy_ < 0) {
            py_ = kBeamBot + height;
            vy_ = 0;
        }
    }

    px_ = std::clamp(px_, 28.0f, kWorld - 24.0f);

    if (grounded_) coyote_ = 0.12f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (jumpBuf_ > 0 && coyote_ > 0 && !ducked_ && stun_ <= 0) {
        vy_ = has_ ? kCarryJump : kJump;
        jumpBuf_ = 0;
        coyote_ = 0;
        grounded_ = false;
        blip(has_ ? 360.0f : 480.0f, 0.05f, 0.05f);
    }

    if (grounded_ && std::abs(vx_) > 30.0f) {
        step_ += std::abs(vx_) * dt * 0.18f;
        if (step_ > 1000.0f) step_ -= 1000.0f;
    }

    if (inv_ <= 0 && stun_ <= 0) {
        for (const Guard& g : guards_) {
            if (std::abs(px_ - g.x) > kHalf + kGHalf) continue;
            const float gTop = kFloor - kGTall;
            const float head = py_ - height;
            if (vy_ >= 0 && prevY <= gTop + 2.0f && py_ >= gTop - 1.0f && head < gTop) {
                py_ = gTop;
                vy_ = -140.0f;
                grounded_ = false;
                continue;
            }
            if (head < kFloor && py_ > gTop) {
                hurt(g.x);
                break;
            }
        }
    }

    if (alarmOn_ && mode_ == Mode::Play) {
        alarm_ -= dt;
        if (has_ && px_ <= kExit) win();
        else if (alarm_ <= 0) lose();
    } else if (has_ && px_ <= kExit) {
        win();
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    const float view = std::clamp(cam_, 0.0f, kWorld - gs::SCREEN_W);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 18) c = gs::rgb4(3, 2, 1);
        else if (y < 150) c = gs::rgb4(6, 5, 4);
        else if (y < 186) c = gs::rgb4(5, 3, 2);
        else c = gs::rgb4(4, 3, 3);
        vdp.lineBackdrop[y] = c;
        float par = y > 186 ? 1.0f : y > 160 ? 0.82f : 0.55f;
        vdp.B.hscroll[y] = int16_t(std::lround(view * par));
        vdp.B.vscroll[y] = 0;
    }

    auto camWorld = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip) {
        spr(m, wx - view, foot, h, pal, flip, true, false);
    };

    const int flutter = int(t_ * 7.0f) & 1;
    const bool showHero = inv_ <= 0 || (int(inv_ * 18.0f) & 1) == 0;

    if (mode_ == Mode::Title) {
        text("S3 BREACH", 160, 36, 2.0f, PAL_HUD, 0);
        text("THE HALL, THEN THE BANNER", 160, 78, 1.0f, PAL_CLOTH, 0);
        text("BRING IT BACK", 160, 98, 1.0f, PAL_HUD, 0);
        if (int(t_ * 2.0f) & 1) text("START", 160, 132, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 48, 1.4f, PAL_HUD, 0);
        text("THROUGH THE BREACH", 160, 74, 1.0f, PAL_CLOTH, 0);
    } else if (mode_ == Mode::Over) {
        text(lives_ <= 0 ? "CAUGHT IN THE HALL" : "THE GATE IS SHUT", 160, 52, 1.2f, PAL_HUD, 0);
        text("THE BANNER STAYS", 160, 80, 1.0f, PAL_CLOTH, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 48, 1.6f, PAL_HUD, 0);
    }

    if (showHero && mode_ != Mode::Title) {
        const gs::Mipped& body = hero();
        float hh = ducked_ ? 22.0f : 42.0f;
        spr(body, px_ - view, py_, hh, PAL_PLAYER, face_ < 0, true, false);
    } else if (mode_ == Mode::Title) {
        spr(art_.stand, px_ - view, py_, 42, PAL_PLAYER, false, true, false);
    }
    if (has_ && (showHero || mode_ == Mode::Title)) {
        int fr = flutter;
        camWorld(art_.banner[fr], px_ + face_ * 16.0f, py_ - 8.0f, 48, PAL_CLOTH, face_ < 0);
    }

    spr(art_.shadow, px_ - view, py_ + 2.0f, 8, PAL_FX, false, true, true);

    for (const Guard& g : guards_) {
        int pose = int(g.x / 8.0f) & 1 ? 1 : 2;
        if (std::abs(g.x - g.minX) < 1.0f || std::abs(g.x - g.maxX) < 1.0f) pose = 0;
        camWorld(art_.guard[pose], g.x, kFloor + 2.0f, 48, PAL_GUARD, g.dir < 0);
        spr(art_.shadow, g.x - view, kFloor + 2.0f, 8, PAL_FX, false, true, true);
    }

    if (!has_) {
        float bob = std::sin(t_ * 3.0f) * 1.5f;
        camWorld(art_.banner[flutter], bannerX_, kFloor - 6.0f + bob, 62, PAL_CLOTH, false);
        camWorld(art_.standBase, bannerX_, kFloor + 2.0f, 14, PAL_CLOTH, false);
        camWorld(art_.star, bannerX_ - 16.0f, kFloor - 70.0f + bob, 8, PAL_CLOTH, false);
        camWorld(art_.star, bannerX_ + 14.0f, kFloor - 58.0f - bob, 6, PAL_CLOTH, false);
        if (std::abs(bannerX_ - kBanner0) < 8.0f) camWorld(art_.dais, kBanner0, kFloor + 4.0f, 20, PAL_STONE, false);
        text("BANNER", bannerX_ - view, kFloor - 78.0f, 1.0f, PAL_HUD, 0);
    }

    for (const Crate& c : kCrates) camWorld(art_.crate, c.x + c.w * 0.5f, kFloor + 2.0f, c.h + 6.0f, PAL_WOOD, false);

    const float drop = alarmOn_ ? std::clamp(1.0f - alarm_ / kAlarm, 0.0f, 1.0f) : 0.0f;
    camWorld(art_.grate, 78.0f, 20.0f + drop * 150.0f, 86, PAL_WOOD, false);

    for (float x = kBeam0 + 20.0f; x < kBeam1; x += 36.0f) {
        camWorld(art_.beam, x, kBeamBot - 2.0f, 18, PAL_WOOD, false);
        camWorld(art_.chain, x - 10.0f, kBeamTop + 4.0f, 22, PAL_WOOD, false);
    }
    camWorld(art_.door, 36.0f, kFloor + 4.0f, 100, PAL_DOOR, false);
    text("OUT", 40.0f - view, 78.0f, 1.0f, PAL_NIGHT, 0);

    for (float x : kTapestries) camWorld(art_.tapestry, x, 118.0f, 28, PAL_CLOTH, false);
    for (float x : kWindows) camWorld(art_.window, x, 132.0f, 52, PAL_NIGHT, false);
    int fi = int(t_ * 8.0f) & 1;
    for (float x : kColumns) {
        camWorld(art_.column, x, kFloor + 2.0f, 156, PAL_STONE, false);
        camWorld(art_.sconce, x + 16.0f, 96.0f, 18, PAL_WOOD, false);
        camWorld(art_.flame[fi], x + 16.0f, 84.0f, 12, PAL_FIRE, false);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char lives[16];
        std::snprintf(lives, sizeof lives, "LIVES %d", lives_);
        hud(1, 0, lives, lives_ > 1 ? PAL_HUD : PAL_CLOTH);
        if (!alarmOn_) hud(28, 0, "GATE UP", PAL_HUD);
        else {
            int n = std::clamp(int(std::ceil(alarm_ / kAlarm * 8.0f)), 0, 8);
            std::string bar = "GATE ";
            for (int i = 0; i < 8; i++) bar += i < n ? '#' : '-';
            hud(23, 0, bar, alarm_ < 10.0f ? PAL_CLOTH : PAL_HUD);
        }
        if (!has_) hudC(1, "THE HALL, THEN THE BANNER", PAL_HUD);
        else hudC(1, "BRING IT BACK", alarm_ < 10.0f ? PAL_CLOTH : PAL_HUD);
        hud(1, 26, "ARROWS  Z JUMP  DOWN DUCK", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
    } else if (mode_ == Mode::Victory || mode_ == Mode::Over) {
        if (!bot_) hudC(26, "START", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.72f);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    cam_ = 24.0f;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float bob = std::sin(t_ * 0.6f) * 10.0f;
        cam_ = 36.0f + bob;
        for (Guard& g : guards_) {
            g.x += g.dir * g.speed * DT;
            if (g.x >= g.maxX) {
                g.x = g.maxX;
                g.dir = -1;
            } else if (g.x <= g.minX) {
                g.x = g.minX;
                g.dir = 1;
            }
        }
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(440.0f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            resetRun();
            mode_ = Mode::Title;
            t_ = 0;
        }
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, down = false, jump = false;
        if (bot_) {
            bot(left, right, down, jump);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            down = pad.down(gs::BTN_DOWN);
            jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO) || pad.pressed(gs::BTN_UP);
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Mode::Pause;
                blip(280.0f, 0.04f, 0.04f);
            }
        }
        if (mode_ == Mode::Play) stepPlay(DT, left, right, down, jump);
        float lead = face_ * 34.0f;
        float want = std::clamp(px_ + lead - 156.0f, 0.0f, kWorld - gs::SCREEN_W);
        cam_ += (want - cam_) * std::min(1.0f, DT * 7.0f);
        if (shake_ > 0) {
            cam_ += std::sin(t_ * 90.0f) * shake_ * 3.0f;
            shake_ = std::max(0.0f, shake_ - DT * 2.4f);
            cam_ = std::clamp(cam_, 0.0f, kWorld - gs::SCREEN_W);
        }
    }

    if (mode_ == Mode::Play && alarmOn_) {
        float tick = std::fmod(alarm_, 0.5f);
        if (tick < DT) sys.apu.tone(2, alarm_ < 10.0f ? 220.0f : 140.0f, 0.04f);
    } else if (fan_ < 0 && mode_ != Mode::Victory) {
        sys.apu.tone(1, mode_ == Mode::Title ? 90.0f : 64.0f, 0.018f);
    }

    if (fan_ >= 0) {
        static const float notes[] = {523.0f, 659.0f, 784.0f, 1046.0f};
        fanT_ += DT;
        if (fanT_ > 0.14f) {
            if (fan_ < 4) sys.apu.tone(2, notes[fan_], won_ ? 0.07f : 0.04f);
            else sys.apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0;
            if (fan_ > 8) fan_ = -1;
        }
    }

    if (mode_ == Mode::Play && !alarmOn_) sys.setLight(180, 120, 40);
    else if (mode_ == Mode::Play && alarm_ < 10.0f) sys.setLight(200, 40, 30);
    else if (mode_ == Mode::Title) sys.setLight(70, 80, 140);

    noteOff();
    draw();
}

}  // namespace breach
