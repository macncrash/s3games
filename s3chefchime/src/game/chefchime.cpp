#include "game/chefchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace chefchime {

bool Game::audit() const {
    auto secAt = [](int frames) { return kStartSec + frames / kFpc; };
    int lead = kHourSec - kStartSec;
    if (lead < 24 || kGraceSec < 8 || kFpc < 2) return false;
    if (secAt(0) >= kHourSec) return false;
    if (secAt(lead * kFpc) != kHourSec) return false;
    if (secAt(lead * kFpc + (kGraceSec - 1) * kFpc) != kHourSec + kGraceSec - 1) return false;
    if (secAt((lead + kGraceSec) * kFpc) < kHourSec + kGraceSec) return false;
    if (kTarget != 12) return false;
    if (kPlates < 1) return false;
    if (!(kGoldLo < 0.7f && kGoldHi > kGoldLo && kGoldHi < 1.f)) return false;
    if (kRise <= 0.f || kRise > 0.05f) return false;
    return true;
}

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::wallFace(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    wallFace(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    wallFace(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    wallFace(h, m, s);
    return s;
}

void Game::begin() {
    plates_ = kPlates;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    swing_ = 0;
    heat_ = 0.35f;
    chefX_ = 96.f;
    faceR_ = true;
    pause_ = false;
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    if (sys_) sys_->apu.silence();
}

void Game::fail(const char* why) {
    reason_ = why;
    won_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    if (sys_) {
        sys_->apu.tone(0, 110.f, 0.08f);
        sys_->apu.noiseBurst(0.18f, 280.f, 0.08f);
    }
}

void Game::plate() {
    if (mode_ != Mode::Play || pause_) return;
    if (pastHour()) {
        fail("HOUR");
        return;
    }
    if (!onHour() || !inGold()) {
        plates_--;
        heat_ = std::max(0.2f, heat_ - 0.25f);
        if (sys_) sys_->apu.tone(0, 160.f, 0.05f);
        if (plates_ <= 0) {
            fail("EARLY");
            return;
        }
        reason_ = "EARLY";
        hold_ = 0;
        mode_ = Mode::Early;
        return;
    }
    won_ = true;
    reason_ = "CHIME";
    hold_ = 0;
    strikes_ = 0;
    swing_ = 8;
    mode_ = Mode::Chime;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->rumble(0.15f, 0.35f, 90);
    }
}

void Game::botAct() {
    if (heat_ > 0.73f) heat_ = std::max(kGoldLo, heat_ - 0.10f);
    if (onHour() && inGold()) plate();
}

void Game::human(const gs::Pad& pad) {
    if (pad.down(gs::BTN_LEFT)) heat_ = std::max(0.f, heat_ - 0.012f);
    if (pad.down(gs::BTN_RIGHT)) heat_ = std::min(1.f, heat_ + 0.01f);
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B)) heat_ = std::max(0.f, heat_ - 0.09f);
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_Z) || pad.pressed(gs::BTN_TURBO)) plate();
}

void Game::spr(const gs::Image& img, float cx, float cy, float h, int pal, bool flip) {
    if (!sys_ || img.w == 0 || h < 1.f) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.img = img;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (!sys_ || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t wall = gs::rgb4(7, 4, 3);
    uint16_t tile = gs::rgb4(11, 8, 6);
    uint16_t steel = gs::rgb4(4, 5, 6);
    uint16_t floor = gs::rgb4(3, 2, 2);
    if (mode_ == Mode::Chime || mode_ == Mode::Leave) tile = gs::rgb4(13, 10, 6);
    if (mode_ == Mode::Fail) {
        wall = gs::rgb4(4, 1, 1);
        tile = gs::rgb4(7, 2, 2);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c = wall;
        if (y >= 24 && y < 132) c = ((y / 10) & 1) ? tile : gs::rgb4(9, 6, 4);
        else if (y >= 132 && y < 168) c = steel;
        else if (y >= 168) c = floor;
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setFogColor(gs::rgb4(2, 1, 1));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    float bob = std::sin(anim_ * 5.f) * 1.1f;
    spr(art_.door, 286, 150, 78, PAL_WOOD);

    float swing = 0;
    if (swing_ > 0) swing = std::sin(anim_ * 18.f) * float(swing_) * 0.35f;
    int clockPal = (mode_ == Mode::Chime || mode_ == Mode::Leave) ? PAL_GOLD : PAL_CLOCK;
    spr(art_.clock, 168, 42, 36, clockPal);
    spr(art_.pendulum, 168 + swing, 68, 16, PAL_CLOCK);

    const gs::Image& pose = (mode_ == Mode::Leave) ? art_.chefWalk : art_.chef;
    spr(pose, chefX_, 164 + bob, 70, PAL_CHEF, !faceR_);

    int foodPal = PAL_FOOD;
    if (heat_ >= 0.92f || (mode_ == Mode::Fail && std::strcmp(reason_, "BURN") == 0)) foodPal = PAL_RED;
    else if (inGold() || mode_ == Mode::Chime || mode_ == Mode::Leave) foodPal = PAL_GOLD;
    float lift = inGold() ? std::sin(anim_ * 7.f) * 1.2f : 0.f;
    if (mode_ != Mode::Leave) {
        spr(art_.roast, 210, 124 + lift, 22, foodPal);
        spr(art_.pan, 206, 140, 22, PAL_STEEL);
        spr(art_.flame[int(anim_ * 10.f) & 1], 198, 156, 14, PAL_FIRE);
    }

    solid(78, 86, 120, 7, PAL_WOOD);
    float fw = 120.f * std::clamp(heat_, 0.f, 1.f);
    int fill = heat_ >= kGoldHi ? PAL_RED : (inGold() ? PAL_GOLD : PAL_FIRE);
    if (fw >= 1.f) solid(78, 86, fw, 7, fill);
    solid(78 + 120.f * kGoldLo, 84, 120.f * (kGoldHi - kGoldLo), 2, PAL_OK);

    for (int i = 0; i < kPlates; i++) {
        int pal = i < plates_ ? PAL_GOLD : PAL_RED;
        solid(16.f + float(i) * 14.f, 16, 10, 7, pal);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CHEFCHIME", PAL_GOLD);
        hudC(5, "A SHORT CHEF", PAL_HUD);
        hudC(22, "TEND THE ROAST", PAL_HUD);
        hudC(23, "LEAVE WHEN THE HOUR CHIMES", PAL_GOLD);
        hudC(25, "ARROWS TEND   A LEAVES", PAL_HUD);
        if ((int(anim_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        const char* line = "TOO SOON";
        if (reason_ && std::strcmp(reason_, "HOUR") == 0) line = "THE HOUR PASSED";
        else if (reason_ && std::strcmp(reason_, "BURN") == 0) line = "THE ROAST BURNED";
        hudC(3, line, PAL_RED);
        hudC(25, "THE HOUR HAD TO CHIME", PAL_HUD);
        if ((int(anim_ * 2.f) & 1) == 0) hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime || mode_ == Mode::Leave) {
        hudC(3, mode_ == Mode::Leave ? "LEAVE" : "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour(), minute(), second());
        hudC(24, buf, PAL_GOLD);
        hudC(25, "THE HOUR HAS TO CHIME", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "POST %d:00:00", kTarget);
        hud(1, 24, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "WALL %d:%02d:%02d", hour(), minute(), second());
        hudC(25, buf, onHour() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "PLATE %d", plates_);
        hud(31, 24, buf, plates_ < kPlates ? PAL_RED : PAL_HUD);
        if (mode_ == Mode::Early) hudC(26, "EARLY", PAL_RED);
        else if (onHour() && inGold()) hudC(26, "PLATE AND LEAVE", PAL_GOLD);
        else if (onHour()) hudC(26, "HOLD THE GOLD", PAL_GOLD);
        else if (inGold()) hudC(26, "WAIT FOR THE HOUR", PAL_OK);
        else if (heat_ < kGoldLo) hudC(26, "STILL RAW", PAL_HUD);
        else hudC(26, "TOO HOT", PAL_RED);
        hudC(27, "LR TEND  C STIR  A LEAVE", PAL_HUD);
        if (pause_) hudC(14, "PAUSED", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    heat_ = 0.62f;
    if (bot_) {
        if (!rules_) {
            reason_ = "RULES";
            won_ = false;
            mode_ = Mode::Fail;
        } else {
            begin();
        }
    }
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += 1.f / 60.f;
    const gs::Pad& pad = sys.pad;
    bool ticking = mode_ == Mode::Play || mode_ == Mode::Early;
    if (ticking && !pause_) playFrames_++;

    if (mode_ == Mode::Title) {
        heat_ = 0.62f + 0.08f * std::sin(anim_ * 1.6f);
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
        if (bot_) go = true;
        if (go) {
            if (!rules_) {
                reason_ = "RULES";
                won_ = false;
                mode_ = Mode::Fail;
            } else {
                begin();
            }
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) pause_ = !pause_;
        if (!pause_) {
            heat_ += kRise;
            if (heat_ >= 1.f) {
                heat_ = 1.f;
                fail("BURN");
            } else if (pastHour()) {
                fail("HOUR");
            } else if (bot_) {
                botAct();
            } else {
                human(pad);
                if (heat_ >= 1.f) {
                    heat_ = 1.f;
                    fail("BURN");
                }
            }
        }
        if (!bot_ && mode_ == Mode::Play && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            sys.apu.silence();
        }
    } else if (mode_ == Mode::Early) {
        if (pastHour()) fail("HOUR");
        else if (++hold_ > 24) {
            mode_ = Mode::Play;
            reason_ = "";
        }
    } else if (mode_ == Mode::Chime) {
        if (hold_ % 8 == 0 && strikes_ < 4) {
            sys.apu.tone(0, (strikes_ % 2) ? 659.f : 523.f, 0.09f);
            swing_ = 10;
            strikes_++;
        }
        if (swing_ > 0) swing_--;
        if (++hold_ > 48) {
            mode_ = Mode::Leave;
            hold_ = 0;
            faceR_ = true;
        }
    } else if (mode_ == Mode::Leave) {
        chefX_ += 2.8f;
        faceR_ = true;
        if (swing_ > 0) swing_--;
        if (++hold_ > 40 || chefX_ > 340.f) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        } else if (bot_ && ++hold_ > 8) {
            over_ = true;
        }
    }

    if (swing_ > 0 && (mode_ == Mode::Play || mode_ == Mode::Early)) swing_--;
    draw();
}

}  // namespace chefchime
