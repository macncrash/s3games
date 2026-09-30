#include "game/tilechime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace tilechime {

int Game::clockSec() const { return kStartSec + playFrames_ / kFpc; }

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::faceTime(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    faceTime(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    faceTime(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    faceTime(h, m, s);
    return s;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.4f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    reason_ = "";
    tile_ = "";
    chimes_ = 0;
    pulls_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    socket_ = false;
    seated_ = Kind::Plain;
}

void Game::begin() {
    won_ = false;
    over_ = false;
    reason_ = "";
    tile_ = "";
    chimes_ = 0;
    pulls_ = 0;
    playFrames_ = 0;
    hold_ = 0;
    socket_ = false;
    seated_ = Kind::Plain;
    mode_ = Mode::Play;
    sys_->apu.tone(0, 392.f, 0.05f);
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    tile_ = "CHIME";
    chimes_ = 1;
    socket_ = true;
    seated_ = Kind::Chime;
    hold_ = 0;
    mode_ = Mode::Chime;
    sys_->apu.keyOn(0, 523.f, 0.32f);
    sys_->apu.keyOn(1, 784.f, 0.2f);
    sys_->apu.noiseBurst(0.1f, 180.f, 0.05f);
    if (!sys_->headless) sys_->rumble(0.25f, 0.5f, 100);
}

void Game::beginFail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    sys_->apu.tone(0, 110.f, 0.08f);
}

void Game::seat(Kind kind) {
    if (mode_ != Mode::Play || socket_) return;
    if (kind == Kind::Chime && onHour()) {
        beginChime();
        return;
    }
    if (kind == Kind::Plain && onHour()) {
        tile_ = "PLAIN";
        socket_ = true;
        seated_ = Kind::Plain;
        beginFail("MUTE");
        return;
    }
    if (pastHour()) {
        beginFail("HOUR");
        return;
    }
    pulls_++;
    tile_ = kind == Kind::Chime ? "CHIME" : "PLAIN";
    sys_->apu.tone(0, 180.f, 0.05f);
    sys_->apu.noiseBurst(0.08f, 400.f, 0.04f);
    if (pulls_ >= kPulls) beginFail("EARLY");
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        if (bot_) {
            if (onHour()) seat(Kind::Chime);
            else if (pastHour()) beginFail("HOUR");
        } else {
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_X)) seat(Kind::Chime);
            else if (pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C)) seat(Kind::Plain);
            else if (pastHour()) beginFail("HOUR");
        }
        if (mode_ == Mode::Play) playFrames_++;
    } else if (mode_ == Mode::Chime) {
        hold_++;
        if (hold_ == 12) sys.apu.tone(0, 659.f, 0.08f);
        if (hold_ == 24) sys.apu.tone(0, 784.f, 0.1f);
        if (hold_ == 40) sys.apu.tone(1, 1046.f, 0.1f);
        if (hold_ > 70) {
            over_ = true;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        if (bot_) {
            hold_++;
            if (hold_ > 24) over_ = true;
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            toTitle();
        }
    }
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hand(float cx, float cy, float ang, float len, int pal) {
    int n = std::max(2, int(len / 4.f));
    for (int i = 1; i <= n; i++) {
        float t = len * (float(i) / float(n));
        spr(art_.pip, cx + std::sin(ang) * t, cy - std::cos(ang) * t, 4.f, 4.f, pal);
    }
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

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int shade = 1 + y / 80;
        if (mode_ == Mode::Chime) shade += 2;
        if (mode_ == Mode::Fail) v.lineBackdrop[y] = gs::rgb4(shade + 1, shade, shade);
        else v.lineBackdrop[y] = gs::rgb4(shade, shade, shade + 2);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.wall, 160.f, 118.f, float(art_.wall.w), float(art_.wall.h), PAL_WALL);

    const float cx = 160.f;
    const float cy = 108.f;
    for (int i = 0; i < 12; i++) {
        if (i == 0 && socket_ && (mode_ == Mode::Chime || mode_ == Mode::Fail)) continue;
        float ang = float(i) * 0.523599f;
        float x = cx + std::sin(ang) * 78.f;
        float y = cy - std::cos(ang) * 62.f;
        int pal = (i == 0) ? PAL_CHIME : PAL_PLAIN;
        float s = (i == 0) ? 16.f : 12.f;
        if (i == 0 && mode_ == Mode::Play && !socket_) s = 10.f;
        spr(art_.tile, x, y, s, s * 0.8f, pal);
    }

    if (socket_) {
        int pal = seated_ == Kind::Chime ? PAL_CHIME : PAL_PLAIN;
        spr(art_.tile, cx, cy - 62.f, 22.f, 18.f, pal);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Chime || (mode_ == Mode::Fail && socket_)) {
        int sec = clockSec();
        float sang = (sec % 60) * 0.10472f;
        float mang = ((sec / 60) % 60) * 0.10472f + (sec % 60) * 0.001745f;
        hand(cx, cy, mang, 28.f, PAL_HAND);
        hand(cx, cy, sang, 44.f, PAL_HAND);
    }

    if (mode_ == Mode::Chime) {
        float bob = 4.f * std::sin(hold_ * 0.35f);
        spr(art_.bell, cx, 36.f + bob, 28.f, 32.f, PAL_BELL);
    } else if (mode_ == Mode::Play && !bot_) {
        spr(art_.tile, 64.f, 188.f, 22.f, 18.f, PAL_CHIME);
        spr(art_.tile, 256.f, 188.f, 22.f, 18.f, PAL_PLAIN);
    } else if (mode_ == Mode::Title) {
        spr(art_.bell, 160.f, 150.f, 22.f, 26.f, PAL_BELL);
        spr(art_.tile, 128.f, 168.f, 18.f, 14.f, PAL_CHIME);
        spr(art_.tile, 192.f, 168.f, 18.f, 14.f, PAL_PLAIN);
    }

    hudC(1, "S3 TILECHIME", PAL_TITLE);
    if (mode_ == Mode::Title) {
        hudC(4, "THE HOUR HAS TO CHIME", PAL_INK);
        hudC(6, "SET THE CHIME TILE ON TWELVE", PAL_HINT);
        hudC(20, "A  CHIME TILE", PAL_TITLE);
        hudC(21, "B  PLAIN TILE IS MUTE", PAL_BAD);
        hudC(24, "START", PAL_TITLE);
    } else {
        int h, m, s;
        faceTime(h, m, s);
        char buf[48];
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        hudC(3, buf, onHour() ? PAL_WIN : PAL_INK);
        if (mode_ == Mode::Play) {
            hudC(4, onHour() ? "ON THE HOUR" : "WAIT FOR THE HOUR", onHour() ? PAL_WIN : PAL_HINT);
            hudC(25, "CHIME TILE FOR THE SOCKET", PAL_TITLE);
            if (!bot_) {
                hud(2, 26, "A CHIME", PAL_TITLE);
                hud(28, 26, "B PLAIN", PAL_BAD);
            }
        } else if (mode_ == Mode::Chime) {
            hudC(4, "THE HOUR CHIMES", PAL_WIN);
            hudC(24, "THE TILE HELD", PAL_WIN);
        } else {
            const char* why = "THE HOUR DID NOT CHIME";
            if (reason_ && std::strcmp(reason_, "EARLY") == 0) why = "PULLED BEFORE THE HOUR";
            else if (reason_ && std::strcmp(reason_, "MUTE") == 0) why = "A PLAIN TILE IS MUTE";
            else if (reason_ && std::strcmp(reason_, "HOUR") == 0) why = "THE HOUR PASSED";
            hudC(4, why, PAL_BAD);
            hudC(24, "START", PAL_INK);
        }
    }
}

}  // namespace tilechime
