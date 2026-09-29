#include "game/drawerchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace drawerchime {
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
    if (lead < 24 || kGraceSec < 8 || kFpc < 2) return false;
    if (secAt(0) >= kHourSec) return false;
    if (secAt(lead * kFpc) != kHourSec) return false;
    if (secAt(lead * kFpc + (kGraceSec - 1) * kFpc) != kHourSec + kGraceSec - 1) return false;
    if (kTarget < 1 || kTarget > 12) return false;
    if (kDrawers != 4 || kTries < 2) return false;
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
    hourP_ = 9;
    minP_ = 14;
    secP_ = 33;
    sel_ = 0;
    tries_ = kTries;
    playFrames_ = 0;
    rep_ = 0;
    hold_ = 0;
    strikes_ = 0;
    swing_ = 0;
    clerkX_ = 28.f;
    for (int i = 0; i < kDrawers; i++) slide_[i] = 0;
    pause_ = false;
    over_ = false;
    won_ = false;
    chimed_ = false;
    reason_ = "";
    mode_ = Mode::Play;
    if (sys_) sys_->apu.silence();
}

void Game::nudge(int dir) {
    if (dir == 0 || mode_ != Mode::Play || sel_ > 2) return;
    if (sel_ == 0) hourP_ = wrap12(hourP_ + dir);
    else if (sel_ == 1) minP_ = (minP_ + dir + 60) % 60;
    else secP_ = (secP_ + dir + 60) % 60;
    slide_[sel_] = 10;
    if (sys_) sys_->apu.tone(1, 380.f + float(sel_) * 90.f, 0.03f);
}

void Game::pull() {
    if (mode_ != Mode::Play || pause_) return;
    slide_[3] = 16;
    if (!platesTrue()) {
        tries_--;
        minP_ = (minP_ + 13) % 60;
        if (minP_ == 0) minP_ = 7;
        swing_ = 4;
        if (sys_) sys_->apu.noiseBurst(0.18f, 140.f, 0.07f);
        reason_ = "FALSE";
        hold_ = 0;
        mode_ = tries_ <= 0 ? Mode::Fail : Mode::Jam;
        if (tries_ <= 0) reason_ = "JAM";
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
        tries_--;
        swing_ = 3;
        reason_ = "EARLY";
        hold_ = 0;
        if (sys_) sys_->apu.tone(0, 160.f, 0.04f);
        mode_ = tries_ <= 0 ? Mode::Fail : Mode::Jam;
        if (tries_ <= 0) reason_ = "JAM";
        return;
    }
    won_ = true;
    chimed_ = true;
    reason_ = "CHIME";
    hold_ = 0;
    strikes_ = 0;
    slide_[3] = 28;
    mode_ = Mode::Chime;
    if (sys_) {
        sys_->apu.tone(0, 523.f, 0.1f);
        sys_->rumble(0.12f, 0.3f, 60);
    }
}

void Game::botAct() {
    int curH = hourP_ % 12;
    int wantH = kTarget % 12;
    if (curH != wantH) {
        sel_ = 0;
        nudge(stepToward(curH, wantH, 12));
        return;
    }
    if (minP_ != 0) {
        sel_ = 1;
        nudge(stepToward(minP_, 0, 60));
        return;
    }
    if (secP_ != 0) {
        sel_ = 2;
        nudge(stepToward(secP_, 0, 60));
        return;
    }
    sel_ = 3;
    if (onHour()) pull();
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_UP)) sel_ = (sel_ + kDrawers - 1) % kDrawers;
    if (pad.pressed(gs::BTN_DOWN)) sel_ = (sel_ + 1) % kDrawers;
    int dir = 0;
    if (pad.down(gs::BTN_RIGHT)) dir += 1;
    if (pad.down(gs::BTN_LEFT)) dir -= 1;
    if (dir == 0) rep_ = 0;
    else if (pad.pressed(gs::BTN_RIGHT) || pad.pressed(gs::BTN_LEFT)) {
        rep_ = 0;
        nudge(dir);
    } else if (++rep_ > 8 && (rep_ % 2) == 0) {
        nudge(dir);
    }
    if (pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) pull();
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
    uint16_t top = gs::rgb4(1, 1, 4);
    uint16_t mid = gs::rgb4(4, 3, 6);
    uint16_t bot = gs::rgb4(2, 1, 2);
    if (mode_ == Mode::Chime || mode_ == Mode::Leave) mid = gs::rgb4(10, 7, 3);
    if (mode_ == Mode::Fail) {
        top = gs::rgb4(3, 0, 1);
        mid = gs::rgb4(5, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto lerp = [](uint16_t a, uint16_t b, float t) {
            auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
            auto L = [&](int sh) { return int(std::lround(ch(a, sh) + (ch(b, sh) - ch(a, sh)) * t)); };
            return gs::rgb4(L(8), L(4), L(0));
        };
        v.lineBackdrop[y] = u < 0.5f ? lerp(top, mid, u / 0.5f) : lerp(mid, bot, (u - 0.5f) / 0.5f);
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
    spr(art_.cabinet, 108.f, 112.f, PAL_WOOD);

    static const float kY[4] = {52.f, 88.f, 124.f, 160.f};
    static const char* kName[3] = {"HOUR", "MIN", "SEC"};
    bool lit = platesTrue() || mode_ == Mode::Chime || mode_ == Mode::Leave;
    for (int i = 0; i < 3; i++) {
        int pal = (mode_ == Mode::Play && sel_ == i && !lit) ? PAL_LIT : PAL_WOOD;
        if (lit) pal = PAL_LIT;
        spr(art_.drawer, 108.f + float(slide_[i]), kY[i], pal);
        spr(art_.knob, 146.f + float(slide_[i]), kY[i], PAL_BRASS);
    }
    int cpal = mode_ == Mode::Chime || mode_ == Mode::Leave ? PAL_LIT : (sel_ == 3 ? PAL_GOLD : PAL_BRASS);
    spr(art_.chimeBox, 108.f + float(slide_[3]), kY[3], cpal);
    spr(art_.knob, 146.f + float(slide_[3]), kY[3], PAL_BRASS);

    float ox = std::sin(f * 0.8f) * float(swing_) * 0.35f;
    spr(art_.bell, 250.f + ox, 36.f, mode_ == Mode::Chime ? PAL_LIT : PAL_BELL);
    spr(art_.clapper, 250.f + ox * 1.4f, 50.f, PAL_BELL);

    int wh, wm, ws;
    wallFace(wh, wm, ws);
    spr(art_.face, 250.f, 100.f, PAL_FACE);
    spr(art_.handH[(wh % 12)], 250.f, 100.f, PAL_FACE);
    spr(art_.handM[(wm / 5) % 12], 250.f, 100.f, PAL_FACE);
    spr(art_.clerk, clerkX_, 186.f, PAL_CLERK);

    char buf[56];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 DRAWERCHIME", PAL_GOLD);
        hudC(23, "SEAT THE DRAWERS", PAL_HUD);
        hudC(24, "THE HOUR HAS TO CHIME", PAL_GOLD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(2, reason_ && std::strcmp(reason_, "HOUR") == 0 ? "THE HOUR PASSED" : "THE DRAWER JAMS", PAL_BAD);
        hudC(24, "YOU DID NOT LEAVE ON THE HOUR", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime || mode_ == Mode::Leave) {
        hudC(2, mode_ == Mode::Leave ? "LEAVE" : "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:00:00", kTarget);
        hudC(23, buf, PAL_GOLD);
        hudC(24, "THE HOUR HAS TO CHIME", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "POST %d:00:00", kTarget);
        hud(1, 23, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", hourP_, minP_, secP_);
        hudC(23, buf, platesTrue() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "TRY %d", tries_);
        hud(32, 23, buf, tries_ < kTries ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "WALL %d:%02d:%02d", wh, wm, ws);
        hudC(24, buf, onHour() ? PAL_GOLD : PAL_HUD);
        if (mode_ == Mode::Jam) hudC(25, reason_ && reason_[0] ? reason_ : "JAM", PAL_BAD);
        else if (platesTrue() && onHour()) hudC(25, "PULL THE CHIME DRAWER", PAL_GOLD);
        else if (platesTrue()) hudC(25, "WAIT FOR THE HOUR", PAL_GOLD);
        else {
            std::snprintf(buf, sizeof buf, "SET %s", sel_ < 3 ? kName[sel_] : "CHIME");
            hudC(25, buf, PAL_HUD);
        }
        hudC(26, "UD DRAWER  LR PLATE  C PULL", PAL_DIM);
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
    bool ticking = mode_ == Mode::Play || mode_ == Mode::Jam;
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
    } else if (mode_ == Mode::Jam) {
        if (pastHour()) {
            reason_ = "HOUR";
            won_ = false;
            mode_ = Mode::Fail;
        } else if (++hold_ > 20) {
            mode_ = Mode::Play;
            reason_ = "";
        }
    } else if (mode_ == Mode::Chime) {
        if (hold_ % 8 == 0 && strikes_ < kTarget) {
            sys.apu.tone(0, (strikes_ % 2) ? 659.f : 523.f, 0.09f);
            swing_ = 10;
            strikes_++;
        }
        if (swing_ > 0) swing_--;
        if (++hold_ > kTarget * 8 + 12) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        clerkX_ += 3.4f;
        if (swing_ > 0) swing_--;
        if (slide_[3] > 8) slide_[3]--;
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

    for (int i = 0; i < 3; i++)
        if (slide_[i] > 0 && mode_ == Mode::Play) slide_[i]--;
    if (slide_[3] > 0 && mode_ == Mode::Play) slide_[3]--;
    if (swing_ > 0 && mode_ == Mode::Play) swing_--;
    draw();
}

}  // namespace drawerchime
