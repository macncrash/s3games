#include "game/drumchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace drumchime {

bool Game::audit() const {
    auto secAt = [](int frames) { return kStartSec + frames / kFpc; };
    int lead = kHourSec - kStartSec;
    if (lead < 30 || kGraceSec < 8 || kFpc < 2) return false;
    if (secAt(0) >= kHourSec) return false;
    if (secAt(lead * kFpc) != kHourSec) return false;
    if (secAt(lead * kFpc + (kGraceSec - 1) * kFpc) != kHourSec + kGraceSec - 1) return false;
    if (secAt((lead + kGraceSec) * kFpc) < kHourSec + kGraceSec) return false;
    if (kGold < 0 || kGold >= kPads || kCream == kGold) return false;
    if (kSticks < 1) return false;
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

void Game::place(int i, float& x, float& y) const {
    static const float xs[kPads] = {118, 168, 214, 78, 250};
    static const float ys[kPads] = {158, 128, 118, 78, 92};
    x = xs[i];
    y = ys[i];
}

void Game::begin() {
    pad_ = 0;
    sticks_ = kSticks;
    blows_ = 0;
    playFrames_ = 0;
    rep_ = 0;
    hold_ = 0;
    anim_ = 0;
    walk_ = 48.f;
    over_ = false;
    won_ = false;
    chimed_ = false;
    reason_ = "";
    face_ = "";
    mode_ = Mode::Play;
    if (sys_) sys_->apu.silence();
}

void Game::fail(const char* why) {
    reason_ = why;
    won_ = false;
    over_ = true;
    hold_ = 0;
    mode_ = Mode::Fail;
    if (sys_) sys_->apu.noiseBurst(0.32f, 80.f, 0.26f);
}

void Game::turn(int dir) {
    if (dir == 0 || mode_ != Mode::Play) return;
    pad_ = (pad_ + dir + kPads) % kPads;
    if (sys_) sys_->apu.tone(1, goldPad(pad_) ? 620.f : (creamPad(pad_) ? 280.f : 360.f), 0.04f);
}

void Game::strike() {
    if (mode_ != Mode::Play) return;
    anim_ = 0;
    if (goldPad(pad_)) {
        if (pastHour()) {
            face_ = "GOLD";
            fail("HOUR");
            return;
        }
        if (!onHour()) {
            sticks_--;
            face_ = "EARLY";
            reason_ = "EARLY";
            hold_ = 0;
            mode_ = Mode::Early;
            if (sys_) {
                sys_->apu.tone(0, 140.f, 0.06f);
                sys_->apu.noiseBurst(0.16f, 200.f, 0.08f);
            }
            if (sticks_ <= 0) fail("STICKS");
            return;
        }
        chimed_ = true;
        blows_++;
        face_ = "GOLD";
        reason_ = "";
        hold_ = 0;
        mode_ = Mode::Chime;
        if (sys_) {
            sys_->apu.tone(0, 196.f, 0.1f);
            sys_->apu.tone(1, 392.f, 0.08f);
            sys_->apu.noiseBurst(0.14f, 420.f, 0.06f);
            sys_->rumble(0.2f, 0.4f, 80);
        }
        return;
    }
    if (creamPad(pad_)) {
        sticks_--;
        face_ = "CREAM";
        reason_ = "CREAM";
        hold_ = 0;
        mode_ = Mode::Early;
        if (sys_) sys_->apu.tone(0, 160.f, 0.05f);
        if (sticks_ <= 0) fail("STICKS");
        return;
    }
    mode_ = Mode::Hit;
    if (sys_) {
        sys_->apu.tone(0, 110.f, 0.05f);
        sys_->apu.noiseBurst(0.12f, 90.f, 0.08f);
    }
}

void Game::leave() {
    if (mode_ != Mode::Play && mode_ != Mode::Chime && mode_ != Mode::Hit) return;
    if (!chimed_ || !onHour()) {
        fail(pastHour() ? "HOUR" : "EARLY");
        return;
    }
    won_ = true;
    reason_ = "CHIME";
    face_ = "GOLD";
    hold_ = 0;
    mode_ = Mode::Leave;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.08f);
        sys_->apu.tone(1, 659.f, 0.07f);
    }
}

void Game::botAct() {
    if (!chimed_) {
        if (pad_ != kGold) {
            turn(1);
            return;
        }
        if (onHour()) strike();
        return;
    }
    if (onHour()) leave();
}

void Game::human(const gs::Pad& pad) {
    int dir = 0;
    if (pad.down(gs::BTN_RIGHT)) dir += 1;
    if (pad.down(gs::BTN_LEFT)) dir -= 1;
    if (dir == 0) rep_ = 0;
    else if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_LEFT)) {
        rep_ = 0;
        turn(dir);
    } else if (++rep_ > 8 && (rep_ % 3) == 0) {
        turn(dir);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) strike();
    if (pad.pressed(gs::BTN_A)) leave();
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
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 5);
    uint16_t mid = gs::rgb4(5, 2, 4);
    uint16_t bot = gs::rgb4(2, 1, 2);
    if ((mode_ == Mode::Leave || mode_ == Mode::Chime) && won_) mid = gs::rgb4(10, 7, 2);
    if (mode_ == Mode::Leave && won_) mid = gs::rgb4(12, 8, 2);
    if (mode_ == Mode::Fail) {
        top = gs::rgb4(2, 0, 1);
        mid = gs::rgb4(5, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.face, 160.f, 46.f, PAL_FACE);
    int mins = minute();
    float ang = (mins / 60.f) * 6.28318f - 1.5708f;
    spr(art_.hand, 160.f + std::cos(ang) * 8.f, 46.f + std::sin(ang) * 8.f, PAL_FACE);
    float swing = (mode_ == Mode::Chime || mode_ == Mode::Leave) ? float((hold_ / 3) % 2 ? 4 : -4) : 0.f;
    spr(art_.bell, 226.f + swing, 40.f, PAL_BELL);

    int shown = pad_;
    if (mode_ == Mode::Title) shown = int(sys_->frame / 16) % kPads;
    for (int i = 0; i < kPads; i++) {
        float x, y;
        place(i, x, y);
        bool cym = i >= 3;
        int pal = goldPad(i) ? PAL_GOLD : (creamPad(i) ? PAL_CREAM : PAL_HEAD);
        if (cym) spr(art_.shell, x, y + 16.f, PAL_STAGE);
        else if (i != 0) spr(art_.shell, x, y + 12.f, PAL_SHELL);
        const gs::Image* img = &art_.headWood;
        if (i == 0) img = &art_.bass;
        else if (cym) img = &art_.cym;
        else if (goldPad(i)) img = &art_.headGold;
        else if (creamPad(i)) img = &art_.headCream;
        int use = (i == shown && mode_ != Mode::Title && goldPad(i)) ? PAL_LIT : pal;
        if (i == 0) use = (i == shown && mode_ != Mode::Title) ? PAL_LIT : PAL_SHELL;
        if (cym && i == shown && mode_ != Mode::Title) use = PAL_LIT;
        float bob = (anim_ < 8 && i == shown && (mode_ == Mode::Hit || mode_ == Mode::Chime))
                        ? float(anim_ < 4 ? anim_ : 8 - anim_)
                        : 0.f;
        spr(*img, x, y + bob, use);
    }

    float sx, sy;
    place(shown, sx, sy);
    float lift = (mode_ == Mode::Hit || mode_ == Mode::Chime) ? 8.f : 4.f;
    spr(art_.stick, sx - 10.f, sy - 18.f - lift, PAL_STICK);
    spr(art_.stick, sx + 12.f, sy - 16.f - lift * 0.5f, PAL_STICK, true);
    spr(art_.player, walk_, 176.f, PAL_STAGE);

    char buf[64];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(1, "S3 DRUMCHIME", PAL_GOLD);
        hudC(23, "PLAY DRUM UNTIL THE HOUR CHIMES", PAL_GOLD);
        hudC(24, "THE GOLD SNARE IS TWELVE", PAL_HUD);
        hudC(25, "LEAVE WHEN THAT IS TRUE", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Leave && won_) {
        hudC(1, "THE HOUR CHIMES", PAL_GOLD);
        int h, m, s;
        wallFace(h, m, s);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d  GOLD", h, m, s);
        hudC(24, buf, PAL_GOLD);
        hudC(25, "YOU LEFT ON THE CHIME", PAL_HUD);
        hudC(26, "THE DOOR IS OPEN", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(1, "STILL INSIDE", PAL_BAD);
        int h, m, s;
        wallFace(h, m, s);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d  %s", h, m, s, reason_[0] ? reason_ : "OPEN");
        hudC(24, buf, PAL_BAD);
        hudC(25, "THE HOUR DID NOT TAKE YOU", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else {
        int h, m, s;
        wallFace(h, m, s);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        hud(1, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "STICKS %d", sticks_);
        hud(28, 1, buf, sticks_ < kSticks ? PAL_BAD : PAL_DIM);
        const char* name = "WOOD";
        if (goldPad(shown)) name = "GOLD";
        else if (creamPad(shown)) name = "CREAM";
        else if (shown >= 3) name = "CYMBAL";
        std::snprintf(buf, sizeof buf, "PAD %s", name);
        hudC(24, buf, goldPad(shown) ? PAL_GOLD : (creamPad(shown) ? PAL_CREAM : PAL_HUD));
        if (mode_ == Mode::Pause) hudC(25, "PAUSED", PAL_GOLD);
        else if (mode_ == Mode::Early) hudC(25, reason_[0] ? reason_ : "EARLY", PAL_BAD);
        else if (mode_ == Mode::Chime) hudC(25, "THE HOUR HAS TO CHIME", PAL_GOLD);
        else if (chimed_ && onHour()) hudC(25, "LEAVE", PAL_GOLD);
        else if (onHour() && goldPad(shown)) hudC(25, "PLAY THE GOLD", PAL_GOLD);
        else if (onHour()) hudC(25, "FIND THE GOLD SNARE", PAL_GOLD);
        else hudC(25, "WAIT FOR THE HOUR", PAL_HUD);
        hudC(27, "LR MOVE  C PLAY  A LEAVE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    if (bot_) {
        if (!rules_) fail("RULES");
        else begin();
    }
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool ticking = mode_ == Mode::Play || mode_ == Mode::Early || mode_ == Mode::Hit;
    if (ticking) playFrames_++;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            if (!rules_) fail("RULES");
            else begin();
        } else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else if (pastHour() && !chimed_) {
            fail("HOUR");
        } else if (bot_) {
            botAct();
        } else {
            human(pad);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Hit) {
        anim_++;
        if (anim_ >= 10) mode_ = Mode::Play;
    } else if (mode_ == Mode::Early) {
        anim_++;
        if (pastHour()) fail("HOUR");
        else if (++hold_ > 18 && mode_ == Mode::Early) {
            mode_ = Mode::Play;
            reason_ = "";
        }
    } else if (mode_ == Mode::Chime) {
        anim_++;
        if (hold_ % 8 == 0) sys.apu.tone(0, (hold_ & 8) ? 659.f : 523.f, 0.08f);
        if (bot_) {
            if (onHour()) leave();
        } else if (pad.pressed(gs::BTN_A)) {
            leave();
        } else if (++hold_ > 90) {
            fail("HOUR");
        }
    } else if (mode_ == Mode::Leave) {
        walk_ += 3.4f;
        if (++hold_ > 28) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    }

    if (!bot_ && mode_ != Mode::Title && mode_ != Mode::Leave && pad.pressed(gs::BTN_MODE)) {
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
        sys.apu.silence();
    }
    draw();
}

}  // namespace drumchime
