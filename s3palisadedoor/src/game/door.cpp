#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace palisade {
namespace {

constexpr int HOLD = 180 * 60;
constexpr float DOOR_X = 160;
constexpr float LINE_Y = 168;

int clampi(int v, int a, int b) { return v < a ? a : v > b ? b : v; }

}  // namespace

void Game::tone(float freq) {
    sys_->apu.tone(0, freq, 0.08f);
    beep_ = 0.06f;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 400L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 400L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    mode_ = Mode::Title;
    if (bot_) begin();
}

void Game::begin() {
    mode_ = Mode::Fight;
    over_ = false;
    won_ = false;
    door_ = 100;
    lives_ = 5;
    held_ = 0;
    px_ = DOOR_X;
    face_ = 1;
    thrust_ = 0;
    thrustCd_ = 0;
    hurt_ = 0;
    spawnIn_ = 70;
    spawnN_ = 0;
    foes_.clear();
}

void Game::bot(float& mx, bool& thrust) {
    const Foe* best = nullptr;
    float bestD = 1e9f;
    for (const Foe& f : foes_) {
        float d = std::fabs(f.x - DOOR_X);
        if (f.bash) d -= 40;  // already on the timber
        if (d < bestD) {
            bestD = d;
            best = &f;
        }
    }
    float goal = best ? best->x : DOOR_X;
    mx = std::clamp(goal - px_, -1.0f, 1.0f);
    if (best && std::fabs(best->x - px_) < 32.0f) thrust = true;
}

void Game::update() {
    float mx = 0;
    bool thrust = false;
    if (bot_) {
        bot(mx, thrust);
    } else {
        if (sys_->pad.down(gs::BTN_LEFT)) mx -= 1;
        if (sys_->pad.down(gs::BTN_RIGHT)) mx += 1;
        if (std::fabs(sys_->pad.axisX) > 0.25f) mx = sys_->pad.axisX;
        thrust = sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_B);
    }
    if (mx < -0.15f) face_ = -1;
    else if (mx > 0.15f) face_ = 1;
    px_ = std::clamp(px_ + mx * 2.35f, 18.0f, 302.0f);

    if (thrust_ > 0) thrust_--;
    if (thrustCd_ > 0) thrustCd_--;
    if (hurt_ > 0) hurt_--;
    bool swung = false;
    if (thrust && thrustCd_ == 0 && thrust_ == 0) {
        thrust_ = 8;
        thrustCd_ = 13;
        swung = true;
        tone(210);
    }

    float sec = held_ / 60.0f;
    float speed = 32.0f + sec * 0.22f;
    if (speed > 70) speed = 70;
    int hp = sec < 40 ? 1 : 2;

    if (--spawnIn_ <= 0) {
        float gap = sec < 25 ? 130 : sec < 70 ? 88 : sec < 120 ? 64 : 50;
        spawnIn_ = int(gap);
        int n = 1;
        if (sec > 50 && (spawnN_ % 5) == 4) n = 2;
        if (sec > 110 && (spawnN_ % 3) == 2) n = 2;
        for (int i = 0; i < n && int(foes_.size()) < 6; i++) {
            Foe f;
            bool left = ((spawnN_ + i) & 1) == 0;
            f.x = left ? -12.0f : 332.0f;
            f.vx = left ? speed / 60.0f : -speed / 60.0f;
            f.hp = hp;
            foes_.push_back(f);
        }
        spawnN_++;
    }

    bool onDoor = false;
    for (int i = int(foes_.size()) - 1; i >= 0; --i) {
        Foe& f = foes_[i];
        if (f.flash > 0) f.flash--;
        if (!f.bash) {
            f.x += f.vx;
            if (std::fabs(f.x - DOOR_X) < 16.0f) f.bash = true;
        } else {
            onDoor = true;
            f.wind++;
        }
        if (swung && f.hp > 0) {
            float dx = f.x - px_;
            bool ahead = face_ > 0 ? (dx > -8 && dx < 36) : (dx < 8 && dx > -36);
            if (ahead) {
                f.hp--;
                f.flash = 6;
                tone(520);
                if (f.hp <= 0) {
                    foes_.erase(foes_.begin() + i);
                    continue;
                }
            }
        }
        if (f.bash && f.wind > 28 && f.hp > 0 && std::fabs(f.x - px_) < 18.0f && hurt_ == 0) {
            lives_--;
            hurt_ = 100;
            tone(90);
            if (lives_ <= 0) {
                mode_ = Mode::Lost;
                over_ = true;
                won_ = false;
                return;
            }
        }
    }

    if (onDoor) {
        if ((bashTick_++ % 8) == 0) sys_->apu.noiseBurst(0.22f, 420, 0.1f);
        if ((bashTick_ % 9) == 0) door_ -= 1;
        if (door_ < 0) door_ = 0;
        if (door_ <= 0) {
            mode_ = Mode::Lost;
            over_ = true;
            won_ = false;
            return;
        }
    }

    held_++;
    if (held_ >= HOLD) {
        mode_ = Mode::Victory;
        over_ = true;
        won_ = true;
        tone(660);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) {
            float t = y / 78.0f;
            c = gs::rgb4(int(2 + 8 * t), int(3 + 4 * t), int(8 - 4 * t));
        } else if (y < 132) {
            c = gs::rgb4(5, 4, 3);
        } else if (y < 156) {
            c = gs::rgb4(4, 5, 2);
        } else {
            int band = ((y / 3) & 1);
            c = band ? gs::rgb4(3, 4, 2) : gs::rgb4(4, 5, 2);
        }
        v.lineBackdrop[y] = c;
    }

    spr(art_.moon, 262, 58, 26, PAL_MOON, false);

    for (int i = 0; i < 18; i++) {
        float x = 8 + i * 18.0f;
        if (std::fabs(x - DOOR_X) < 22) continue;
        float h = 86 + ((i * 17) % 5);
        spr(art_.stake, x, 154, h, PAL_WOOD, (i & 1) != 0);
    }
    for (int i = 0; i < 18; i += 3) {
        float x = 8 + i * 18.0f;
        if (std::fabs(x - DOOR_X) < 30) continue;
        spr(art_.banner, x + 6, 92, 14, PAL_IRON, false);
    }

    float crack = (100 - door_) * 0.08f;
    spr(art_.gate, DOOR_X - 16 - crack, 158, 78, PAL_WOOD, false);
    spr(art_.gate, DOOR_X + 16 + crack, 158, 78, PAL_WOOD, true);

    for (const Foe& f : foes_) {
        int pal = f.flash ? PAL_FX : PAL_RAIDER;
        bool flip = f.vx < 0 || (f.bash && f.x > DOOR_X);
        spr(art_.raider, f.x, LINE_Y, 46, pal, flip);
        float ax = f.x + (flip ? -10.0f : 10.0f);
        float ay = LINE_Y - (f.bash ? 18.0f : 28.0f) + ((held_ / 6) & 1 ? 2.0f : 0);
        spr(art_.axe, ax, ay, 16, PAL_IRON, flip);
    }

    bool show = hurt_ == 0 || ((hurt_ / 3) & 1) == 0;
    if (show) {
        const gs::Mipped& g = thrust_ > 0 ? art_.guardThrust : art_.guard;
        spr(g, px_, LINE_Y, 52, PAL_PLAYER, face_ < 0);
    }

    int left = HOLD - held_;
    if (left < 0) left = 0;
    int sec = (mode_ == Mode::Fight || mode_ == Mode::Pause) ? left / 60 : held_ / 60;
    char clock[16];
    std::snprintf(clock, sizeof(clock), "%d:%02d", sec / 60, sec % 60);
    hud(1, 1, "DOOR", PAL_HUD);
    int bars = clampi(door_ / 5, 0, 20);
    std::string bar(size_t(bars), '#');
    bar.append(size_t(20 - bars), '.');
    hud(6, 1, bar, door_ < 30 ? PAL_HUD : PAL_HUD);
    hud(28, 1, clock, PAL_HUD);
    hud(1, 2, "RANK", PAL_HUD);
    std::string hearts(size_t(std::max(lives_, 0)), '*');
    hud(6, 2, hearts, PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(8, "PALISADE", PAL_HUD);
        hudC(11, "HOLD THE DOOR", PAL_HUD);
        hudC(13, "THREE MINUTES", PAL_HUD);
        hudC(16, "LEFT RIGHT MOVE", PAL_HUD);
        hudC(17, "A  THRUST", PAL_HUD);
        hudC(20, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(10, "THE DOOR HELD", PAL_HUD);
        hudC(12, "THREE MINUTES", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(10, lives_ <= 0 ? "YOU FELL" : "THE DOOR FELL", PAL_HUD);
        hudC(12, "THE PALISADE IS LOST", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
    } else if (held_ < 90) {
        hudC(24, "KEEP THEM OFF THE GATE", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0) {
        beep_ -= 1.0f / 60.0f;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }

    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Fight;
    } else if (mode_ == Mode::Fight) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
            heldMode_ = mode_;
            mode_ = Mode::Pause;
        } else {
            update();
        }
    }

    draw();
}

}  // namespace palisade
