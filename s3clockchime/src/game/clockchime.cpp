#include "game/clockchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace clockchime {
namespace {

int wrap12(int h) {
    h %= 12;
    if (h <= 0) h += 12;
    return h;
}

int stepToward(int cur, int want, int mod) {
    int cw = (want - cur + mod) % mod;
    int ccw = (cur - want + mod) % mod;
    if (cw == 0) return 0;
    return cw <= ccw ? 1 : -1;
}

}  // namespace

bool Game::audit() const {
    auto secAt = [](int frames) { return kStartSec + frames / kFpc; };
    int lead = kHourSec - kStartSec;
    if (lead < 30 || kGraceSec < 8 || kFpc < 2) return false;
    if (secAt(0) >= kHourSec) return false;
    if (secAt(lead * kFpc) != kHourSec) return false;
    if (secAt(lead * kFpc + (kGraceSec - 1) * kFpc) != kHourSec + kGraceSec - 1) return false;
    if (secAt((lead + kGraceSec) * kFpc) < kHourSec + kGraceSec) return false;
    if (kTarget < 1 || kTarget > 12) return false;
    if (kTarget == 10 && 47 == 0) return false;
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

int Game::hourStep() const {
    int h = hour_ % 12;
    int deg = h * 30 + minute_ / 2;
    int s = deg / 6;
    if (s < 0) s += 60;
    return s % 60;
}

void Game::begin() {
    hour_ = 10;
    minute_ = 47;
    second_ = 22;
    grip_ = 0;
    ropes_ = kRopes;
    playFrames_ = 0;
    rep_ = 0;
    hold_ = 0;
    strikes_ = 0;
    swing_ = 0;
    ropeY_ = 0;
    keepX_ = 196.f;
    pause_ = false;
    over_ = false;
    won_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    if (sys_) sys_->apu.silence();
}

void Game::turn(int dir) {
    if (dir == 0 || mode_ != Mode::Play) return;
    if (grip_ == 0) hour_ = wrap12(hour_ + dir);
    else if (grip_ == 1) minute_ = (minute_ + dir + 60) % 60;
    else second_ = (second_ + dir + 60) % 60;
    if (sys_) sys_->apu.tone(1, 520.f + float(grip_) * 140.f, 0.03f);
}

void Game::haul() {
    if (mode_ != Mode::Play || pause_) return;
    ropeY_ = 8;
    if (!handsTrue()) {
        ropes_--;
        minute_ = (minute_ + 17) % 60;
        if (minute_ == 0) minute_ = 11;
        swing_ = 6;
        if (sys_) {
            sys_->apu.tone(0, 90.f, 0.05f);
            sys_->apu.noiseBurst(0.2f, 160.f, 0.08f);
        }
        if (ropes_ <= 0) {
            reason_ = "ROPES";
            won_ = false;
            hold_ = 0;
            mode_ = Mode::Fail;
        } else {
            reason_ = "FALSE";
            hold_ = 0;
            mode_ = Mode::Early;
        }
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
        ropes_--;
        minute_ = 19;
        second_ = 7;
        swing_ = 4;
        reason_ = "EARLY";
        hold_ = 0;
        if (sys_) sys_->apu.tone(0, 180.f, 0.05f);
        if (ropes_ <= 0) {
            reason_ = "ROPES";
            won_ = false;
            mode_ = Mode::Fail;
        } else {
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
        sys_->apu.tone(0, 523.f, 0.1f);
        sys_->rumble(0.15f, 0.35f, 70);
    }
}

void Game::botAct() {
    int wantH = kTarget % 12;
    int curH = hour_ % 12;
    if (curH != wantH) {
        grip_ = 0;
        turn(stepToward(curH, wantH, 12));
        return;
    }
    if (minute_ != 0) {
        grip_ = 1;
        turn(stepToward(minute_, 0, 60));
        return;
    }
    if (second_ != 0) {
        grip_ = 2;
        turn(stepToward(second_, 0, 60));
        return;
    }
    if (onHour()) haul();
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_UP)) grip_ = (grip_ + 2) % 3;
    if (pad.pressed(gs::BTN_DOWN)) grip_ = (grip_ + 1) % 3;
    int dir = 0;
    if (pad.down(gs::BTN_RIGHT)) dir += 1;
    if (pad.down(gs::BTN_LEFT)) dir -= 1;
    if (dir == 0) rep_ = 0;
    else if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_LEFT)) {
        rep_ = 0;
        turn(dir);
    } else if (++rep_ > 8 && (rep_ % 2) == 0) {
        turn(dir);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) haul();
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::axle(const gs::Image& img, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(kCx - kPivot);
    s.y = int16_t(kCy - kPivot);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 1, 5);
    uint16_t mid = gs::rgb4(6, 3, 5);
    uint16_t bot = gs::rgb4(2, 1, 2);
    if (mode_ == Mode::Chime || mode_ == Mode::Leave) mid = gs::rgb4(12, 8, 3);
    if (mode_ == Mode::Fail) {
        top = gs::rgb4(3, 0, 1);
        mid = gs::rgb4(6, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto lerp = [](uint16_t a, uint16_t b, float t) {
            auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
            auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
            return gs::rgb4(L(8), L(4), L(0));
        };
        v.lineBackdrop[y] = u < 0.55f ? lerp(top, mid, u / 0.55f) : lerp(mid, bot, (u - 0.55f) / 0.45f);
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
    spr(art_.moon, 28, 22, PAL_NIGHT);
    static const int kStar[][2] = {{12, 12}, {48, 8}, {70, 18}, {200, 10}, {300, 14}, {280, 28}, {16, 36}};
    for (int i = 0; i < 7; i++) {
        if (((f + i * 3) % 17) == 0) continue;
        spr(art_.star, float(kStar[i][0]), float(kStar[i][1]), PAL_NIGHT);
    }

    spr(art_.face, float(kCx), float(kCy), PAL_FACE);
    int hs = hourStep();
    int mp = handsTrue() || mode_ == Mode::Chime || mode_ == Mode::Leave ? PAL_LIT : PAL_MIN;
    int hp = handsTrue() || mode_ == Mode::Chime || mode_ == Mode::Leave ? PAL_LIT : PAL_HOUR;
    int sp = (mode_ == Mode::Play && grip_ == 2) ? PAL_LIT : PAL_SEC;
    if (mode_ == Mode::Play && grip_ == 0 && !handsTrue()) hp = PAL_LIT;
    if (mode_ == Mode::Play && grip_ == 1 && !handsTrue()) mp = PAL_LIT;
    axle(art_.hand[2][second_ % 60], sp);
    axle(art_.hand[1][minute_ % 60], mp);
    axle(art_.hand[0][hs], hp);
    spr(art_.cap, float(kCx), float(kCy), PAL_HOUR);

    float ox = std::sin(f * 0.7f) * float(swing_) * 0.4f;
    spr(art_.bell, float(kBellX) + ox, float(kBellY), mode_ == Mode::Chime ? PAL_LIT : PAL_BELL);
    spr(art_.rope, float(kBellX + 22), 96.f + float(ropeY_), PAL_BELL);
    spr(art_.keeper, keepX_, 176.f, PAL_KEEP);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 CLOCKCHIME", PAL_GOLD);
        hudC(24, "PLAY CLOCK", PAL_GOLD);
        hudC(25, "LEAVE WHEN THE HOUR CHIMES", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "PRESS START", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(3, reason_ && std::strcmp(reason_, "HOUR") == 0 ? "THE HOUR PASSED" : "THE WORKS JAM", PAL_BAD);
        hudC(25, "YOU DID NOT LEAVE ON THE HOUR", PAL_HUD);
        if ((f & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime || mode_ == Mode::Leave) {
        hudC(3, mode_ == Mode::Leave ? "LEAVE" : "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:00:00", kTarget);
        hudC(24, buf, PAL_GOLD);
        hudC(25, "THE HOUR HAS TO CHIME", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "POST %d:00:00", kTarget);
        hud(1, 24, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hour_, minute_, second_);
        hudC(24, buf, handsTrue() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "ROPES %d", ropes_);
        hud(31, 24, buf, ropes_ < kRopes ? PAL_BAD : PAL_HUD);
        int wh, wm, ws;
        wallFace(wh, wm, ws);
        std::snprintf(buf, sizeof buf, "WALL %d:%02d:%02d", wh, wm, ws);
        hudC(25, buf, onHour() ? PAL_GOLD : PAL_HUD);
        if (mode_ == Mode::Early) hudC(26, reason_ && reason_[0] ? reason_ : "EARLY", PAL_BAD);
        else if (handsTrue() && onHour()) hudC(26, "HAUL AND LEAVE", PAL_GOLD);
        else if (handsTrue()) hudC(26, "WAIT FOR THE HOUR", PAL_GOLD);
        else hudC(26, "SET THE HANDS", PAL_HUD);
        hudC(27, "UD HAND  LR TURN  C ROPE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.apu.setMaster(0.75f);
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool ticking = mode_ == Mode::Play || mode_ == Mode::Early;
    if (ticking && !pause_) playFrames_++;

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
        } else if (++hold_ > 24) {
            mode_ = Mode::Play;
            reason_ = "";
        }
    } else if (mode_ == Mode::Chime) {
        if (hold_ % 8 == 0 && strikes_ < kTarget) {
            sys.apu.tone(0, (strikes_ % 2) ? 659.f : 523.f, 0.09f);
            swing_ = 12;
            strikes_++;
        }
        if (swing_ > 0) swing_--;
        if (ropeY_ > 0) ropeY_--;
        if (++hold_ > kTarget * 8 + 16) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        keepX_ += 3.2f;
        if (swing_ > 0) swing_--;
        if (++hold_ > 42) {
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

    if (ropeY_ > 0 && mode_ == Mode::Play) ropeY_--;
    if (swing_ > 0 && mode_ == Mode::Play) swing_--;
    draw();
}

}  // namespace clockchime
