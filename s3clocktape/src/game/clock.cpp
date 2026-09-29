#include "game/clock.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace clocktape {
namespace {

int wrap12(int h) {
    int m = h % 12;
    if (m <= 0) m += 12;
    return m;
}

int stepToward(int cur, int want, int mod) {
    int cw = (want - cur + mod) % mod;
    int ccw = (cur - want + mod) % mod;
    if (cw == 0) return 0;
    return cw <= ccw ? 1 : -1;
}

void mixLine(uint16_t* row, int y0, int y1, uint16_t a, uint16_t b) {
    for (int y = y0; y < y1; y++) {
        float u = float(y - y0) / float(std::max(1, y1 - y0 - 1));
        int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
        int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
        int r = int(ar + (br - ar) * u);
        int g = int(ag + (bg - ag) * u);
        int bl = int(ab + (bb - ab) * u);
        row[y] = gs::rgb4(r, g, bl);
    }
}

}  // namespace

bool Game::audit() {
    bool seen[12][12] = {};
    for (int i = 0; i < kTapeN; i++) {
        if (kTape[i].hour < 1 || kTape[i].hour > 12) return false;
        if (kTape[i].minute % 5 != 0 || kTape[i].minute < 0 || kTape[i].minute > 55) return false;
        int hi = kTape[i].hour - 1;
        int mi = kTape[i].minute / 5;
        if (seen[hi][mi]) return false;
        seen[hi][mi] = true;
        if (!kTape[i].label || !kTape[i].label[0]) return false;
    }
    return kTapeN == 3;
}

bool Game::matched() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return false;
    return true;
}

int Game::heldCount() const {
    int n = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) n++;
    return n;
}

int Game::faceLine() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i] && hour_ == kTape[i].hour && minute_ == kTape[i].minute) return i;
    return -1;
}

int Game::openLine() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return -1;
}

int Game::hourStep() const {
    int h = hour_ % 12;
    int degTimes2 = h * 60 + minute_;  // degrees * 2
    int s = degTimes2 / 12;            // 6 degrees per step
    if (s < 0) s = 0;
    return s % kSteps;
}

int Game::minuteStep() const { return ((minute_ / 5) * 5) % kSteps; }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.65f);
    mode_ = Mode::Title;
    std::snprintf(note_, sizeof note_, rules_ ? "tape posted" : "rules failed");
}

void Game::begin() {
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    hour_ = 1;
    minute_ = 10;
    grip_ = 0;
    faults_ = 0;
    won_ = false;
    over_ = false;
    leaveT_ = 0;
    pause_ = false;
    mode_ = Mode::Play;
    std::snprintf(note_, sizeof note_, "set the face");
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    blipLeft_ = 6;
}

void Game::turn(int dir) {
    if (dir == 0) return;
    if (grip_ == 0) {
        hour_ = wrap12(hour_ + dir);
    } else {
        int m = minute_ + dir * 5;
        if (m >= 60) {
            m -= 60;
            hour_ = wrap12(hour_ + 1);
        } else if (m < 0) {
            m += 60;
            hour_ = wrap12(hour_ - 1);
        }
        minute_ = m;
    }
    blip(grip_ == 0 ? 220.f : 330.f);
}

void Game::fileFace() {
    int line = faceLine();
    if (line >= 0) {
        held_[line] = true;
        std::snprintf(note_, sizeof note_, "%s in the drawer", kTape[line].label);
        blip(620.f);
        if (matched()) {
            mode_ = Mode::Leave;
            leaveT_ = 0;
            std::snprintf(note_, sizeof note_, "drawer matches");
            blip(784.f);
        }
        return;
    }
    bool already = false;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i] && hour_ == kTape[i].hour && minute_ == kTape[i].minute) already = true;
    faults_++;
    if (already)
        std::snprintf(note_, sizeof note_, "already filed");
    else
        std::snprintf(note_, sizeof note_, "not on the tape");
    blip(110.f);
    if (faults_ >= kFaults) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        std::snprintf(note_, sizeof note_, "drawer jammed");
    }
}

void Game::botAct() {
    if (mode_ == Mode::Title) {
        if (++titleT_ > 8) begin();
        return;
    }
    if (mode_ != Mode::Play) return;
    int line = openLine();
    if (line < 0) return;
    const Slip& s = kTape[line];
    if (hour_ == s.hour && minute_ == s.minute) {
        fileFace();
        return;
    }
    int hd = stepToward(hour_, s.hour, 12);
    int md = stepToward(minute_ / 5, s.minute / 5, 12);
    if (hd != 0) {
        grip_ = 0;
        // Hour steps do not carry minutes, so aim the hour first.
        hour_ = wrap12(hour_ + hd);
        blip(200.f);
        return;
    }
    grip_ = 1;
    if (md != 0) turn(md > 0 ? 1 : -1);
}

void Game::human(const gs::Pad& pad) {
    if (pad.pressed(gs::BTN_START)) pause_ = !pause_;
    if (pause_) return;
    if (pad.pressed(gs::BTN_LEFT) || pad.pressed(gs::BTN_RIGHT)) {
        grip_ ^= 1;
        blip(160.f);
    }
    int dir = 0;
    if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_X)) dir = 1;
    if (pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_Y)) dir = -1;
    if (dir) turn(dir);
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C)) fileFace();
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0 || img.h == 0) return;
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
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    uint16_t top = gs::rgb4(2, 2, 4);
    uint16_t mid = gs::rgb4(5, 4, 5);
    uint16_t bot = gs::rgb4(3, 2, 2);
    mixLine(sys_->vdp.lineBackdrop, 0, 90, top, mid);
    mixLine(sys_->vdp.lineBackdrop, 90, gs::SCREEN_H, mid, bot);
    for (int y = 0; y < gs::SCREEN_H; y++) sys_->vdp.lineFog[y] = 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.cabinet, 112, 112, PAL_CAB);
    spr(art_.dial, float(kCx), float(kCy), PAL_DIAL);
    spr(art_.minute[minuteStep()], float(kCx), float(kCy), grip_ == 1 ? PAL_LIT : PAL_MIN);
    spr(art_.hour[hourStep()], float(kCx), float(kCy), grip_ == 0 ? PAL_LIT : PAL_HOUR);
    spr(art_.cap, float(kCx), float(kCy), PAL_WOOD);

    spr(art_.tape, 250, 78, PAL_TAPE);
    spr(art_.drawer, 160, 198, PAL_WOOD);
    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        spr(art_.slip, 78.f + i * 54.f, 198.f, PAL_SLIP);
    }

    hud(1, 1, "S3 CLOCKTAPE", PAL_GOLD);
    char face[16];
    std::snprintf(face, sizeof face, "%d:%02d", hour_, minute_);
    hud(1, 3, grip_ == 0 ? "GRIP HOUR" : "GRIP MINUTE", PAL_HUD);
    hud(14, 3, face, PAL_GOLD);

    hud(28, 2, "TAPE", PAL_INK);
    for (int i = 0; i < kTapeN; i++) {
        char line[24];
        std::snprintf(line, sizeof line, "%s %s", held_[i] ? "IN" : "  ", kTape[i].label);
        hud(28, 4 + i * 2, line, held_[i] ? PAL_GOLD : PAL_HUD);
    }

    char faults[24];
    std::snprintf(faults, sizeof faults, "FAULTS %d/%d", faults_, kFaults);
    hud(1, 24, faults, faults_ ? PAL_BAD : PAL_DIM);
    hud(1, 26, note_, mode_ == Mode::Lose ? PAL_BAD : PAL_HUD);

    if (mode_ == Mode::Title) {
        hudC(12, "THE DRAWER HAS TO", PAL_GOLD);
        hudC(14, "MATCH THE TAPE", PAL_GOLD);
        hudC(17, "START", PAL_HUD);
    } else if (mode_ == Mode::Leave || mode_ == Mode::Win) {
        hudC(12, "DRAWER MATCHES", PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        hudC(12, "STILL OPEN", PAL_BAD);
    } else if (pause_) {
        hudC(12, "PAUSED", PAL_DIM);
    } else {
        hud(1, 22, "ARROWS TURN   A FILES", PAL_DIM);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (blipLeft_ > 0) {
        if (--blipLeft_ == 0) sys.apu.tone(0, 0, 0);
    }
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_)
            botAct();
        else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))
            begin();
    } else if (mode_ == Mode::Play) {
        if (bot_)
            botAct();
        else
            human(pad);
    } else if (mode_ == Mode::Leave) {
        if (++leaveT_ > 36) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            std::snprintf(note_, sizeof note_, "left");
        }
    }
    draw();
}

}  // namespace clocktape
