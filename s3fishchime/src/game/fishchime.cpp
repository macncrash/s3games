#include "game/fishchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace fishchime {

bool Game::audit() const {
    auto secAt = [](int frames) { return kStartSec + frames / kFpc; };
    int lead = kHourSec - kStartSec;
    if (lead < 24 || kGraceSec < 8 || kFpc < 2) return false;
    if (secAt(0) >= kHourSec) return false;
    if (secAt(lead * kFpc) != kHourSec) return false;
    if (secAt(lead * kFpc + (kGraceSec - 1) * kFpc) != kHourSec + kGraceSec - 1) return false;
    if (secAt((lead + kGraceSec) * kFpc) < kHourSec + kGraceSec) return false;
    if (kTarget != 12) return false;
    if (kBreaths < 1) return false;
    if (kBellX < 80.f) return false;
    return true;
}

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

int Game::framesUntilHour() const {
    int sec = clockSec();
    int sub = playFrames_ % kFpc;
    if (sec > kHourSec) return -((sec - kHourSec) * kFpc + sub);
    if (sec == kHourSec) return -sub;
    return (kHourSec - sec) * kFpc - sub;
}

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

bool Game::nearBell() const { return std::fabs(fishX_ - kBellX) <= kReach; }

void Game::begin() {
    breaths_ = kBreaths;
    playFrames_ = 0;
    hold_ = 0;
    strikes_ = 0;
    swing_ = 0;
    fishX_ = 36.f;
    bob_ = 0.f;
    faceR_ = true;
    pause_ = false;
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    if (sys_) sys_->apu.silence();
}

void Game::strike() {
    if (mode_ != Mode::Play || pause_) return;
    swing_ = 8;
    if (!nearBell()) {
        if (sys_) sys_->apu.noiseBurst(0.08f, 400.f, 0.05f);
        return;
    }
    if (pastHour()) {
        reason_ = "HOUR";
        won_ = false;
        hold_ = 0;
        mode_ = Mode::Fail;
        return;
    }
    if (!onHour()) {
        breaths_--;
        fishX_ = std::max(24.f, fishX_ - 28.f);
        faceR_ = false;
        if (sys_) {
            sys_->apu.tone(0, 140.f, 0.05f);
            sys_->apu.noiseBurst(0.16f, 220.f, 0.08f);
        }
        if (breaths_ <= 0) {
            reason_ = "BREATH";
            won_ = false;
            hold_ = 0;
            mode_ = Mode::Fail;
        } else {
            reason_ = "EARLY";
            hold_ = 0;
            mode_ = Mode::Early;
        }
        return;
    }
    won_ = true;
    reason_ = "CHIME";
    hold_ = 0;
    strikes_ = 0;
    mode_ = Mode::Chime;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.12f);
        sys_->rumble(0.12f, 0.3f, 80);
    }
}

void Game::botAct() {
    float gap = kBellX - fishX_;
    int until = framesUntilHour();
    int swim = int(std::ceil(std::fabs(gap) / 3.4f));
    if (!nearBell() && until <= swim + 8) {
        faceR_ = gap >= 0;
        float step = gap > 0 ? std::min(3.4f, gap) : std::max(-3.4f, gap);
        fishX_ += step;
        return;
    }
    if (nearBell() && onHour()) strike();
}

void Game::human(const gs::Pad& pad) {
    float dir = 0;
    if (pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (pad.axisX > 0.25f || pad.axisX < -0.25f) dir = pad.axisX;
    if (dir != 0.f) {
        faceR_ = dir > 0;
        fishX_ = std::clamp(fishX_ + dir * 2.6f, 16.f, 304.f);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) strike();
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal, bool hflip) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = hflip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) c = ' ';
        int tile = art_.font[c - 32];
        h.set(col + i, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t sky = gs::rgb4(4, 8, 13);
    uint16_t haze = gs::rgb4(8, 12, 14);
    uint16_t sea = gs::rgb4(1, 4, 10);
    uint16_t deep = gs::rgb4(0, 1, 5);
    if (mode_ == Mode::Chime || mode_ == Mode::Leave) haze = gs::rgb4(14, 12, 6);
    if (mode_ == Mode::Fail) {
        sky = gs::rgb4(4, 2, 3);
        sea = gs::rgb4(3, 1, 4);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
        auto lerp = [&](uint16_t a, uint16_t b, float t) {
            auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
            return gs::rgb4(L(8), L(4), L(0));
        };
        if (u < 0.38f) v.lineBackdrop[y] = lerp(sky, haze, u / 0.38f);
        else if (u < 0.48f) v.lineBackdrop[y] = lerp(haze, sea, (u - 0.38f) / 0.10f);
        else v.lineBackdrop[y] = lerp(sea, deep, (u - 0.48f) / 0.52f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    sky();

    int f = int(sys_->frame);
    spr(art_.pier, 292.f, 78.f, PAL_FACE);
    spr(art_.face, 292.f, 42.f, PAL_FACE);
    int h, m, s;
    wallFace(h, m, s);
    int hi = (h % 12);
    int mi = (m / 5) % 12;
    spr(art_.hand[1][mi], 292.f, 42.f, PAL_GOLD);
    spr(art_.hand[0][hi], 292.f, 42.f, PAL_HUD);

    float ox = std::sin(f * 0.5f) * float(swing_) * 0.35f;
    spr(art_.buoy, kBellX, 118.f, PAL_BUOY);
    spr(art_.bell, kBellX + ox, 92.f, mode_ == Mode::Chime || mode_ == Mode::Leave ? PAL_GOLD : PAL_BELL);
    for (int i = 0; i < 5; i++) {
        float kx = 28.f + float(i) * 52.f;
        float ky = 196.f + std::sin((f + i * 11) * 0.08f) * 3.f;
        spr(art_.kelp, kx, ky, PAL_KELP);
    }
    for (int i = 0; i < 4; i++) {
        float by = 130.f - float((f * 2 + i * 37) % 90);
        float bx = 50.f + float(i) * 60.f + std::sin((f + i) * 0.1f) * 4.f;
        spr(art_.bubble, bx, by, PAL_BUB);
    }

    float fy = kFishY + std::sin(bob_) * 3.f;
    int wag = ((f / 8) & 1);
    spr(art_.fish[wag], fishX_, fy, PAL_FISH, !faceR_);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 FISHCHIME", PAL_GOLD);
        hudC(5, "A SHORT FISH", PAL_HUD);
        hudC(24, "SWIM TO THE BUOY", PAL_HUD);
        hudC(25, "STRIKE WHEN THE HOUR CHIMES", PAL_GOLD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, reason_ && std::strcmp(reason_, "HOUR") == 0 ? "THE HOUR PASSED" : "TOO SOON", PAL_BAD);
        hudC(25, "THE HOUR HAD TO CHIME", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
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
        std::snprintf(buf, sizeof buf, "BREATH %d", breaths_);
        hud(30, 24, buf, breaths_ < kBreaths ? PAL_BAD : PAL_HUD);
        if (mode_ == Mode::Early) hudC(26, "EARLY", PAL_BAD);
        else if (nearBell() && onHour()) hudC(26, "STRIKE AND LEAVE", PAL_GOLD);
        else if (nearBell()) hudC(26, "WAIT FOR THE HOUR", PAL_GOLD);
        else hudC(26, "SWIM TO THE BELL", PAL_HUD);
        hudC(27, "LR SWIM   C STRIKE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 2, 4));
    sys.apu.setMaster(0.75f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool ticking = mode_ == Mode::Play || mode_ == Mode::Early;
    if (ticking && !pause_) {
        playFrames_++;
        bob_ += 0.18f;
    }

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C);
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
            if (pastHour()) {
                reason_ = "HOUR";
                won_ = false;
                hold_ = 0;
                mode_ = Mode::Fail;
            } else if (bot_) {
                botAct();
            } else {
                human(pad);
            }
        }
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            sys.apu.silence();
        }
    } else if (mode_ == Mode::Early) {
        if (pastHour()) {
            reason_ = "HOUR";
            won_ = false;
            mode_ = Mode::Fail;
        } else if (++hold_ > 20) {
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
        bob_ += 0.12f;
        if (++hold_ > 48) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        fishX_ += 3.6f;
        faceR_ = true;
        bob_ += 0.2f;
        if (swing_ > 0) swing_--;
        if (++hold_ > 36) {
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

}  // namespace fishchime
