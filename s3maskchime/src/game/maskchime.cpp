#include "game/maskchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace maskchime {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kRail0 = 50.f;
constexpr float kRail1 = 270.f;

struct Groove {
    const char* name;
    float hx, hy;
};

const Groove kGroove[kCuts] = {
    {"BROW", 150.f, 86.f},
    {"LEFT", 132.f, 108.f},
    {"RIGHT", 168.f, 108.f},
    {"MOUTH", 150.f, 140.f},
};

}  // namespace

int Game::openCut() const {
    for (int i = 0; i < kCuts; i++)
        if (!cutOn_[i]) return i;
    return -1;
}

float Game::needle() const {
    int t = phase_ % kPeriod;
    float u = t / float(kPeriod);
    return u < 0.5f ? u * 2.f : (1.f - u) * 2.f;
}

bool Game::hot() const {
    if (openCut() < 0) return false;
    float n = needle();
    return n > 0.42f && n < 0.58f;
}

int Game::clockSec() const { return kHourSec - kLeadSec + playFrames_ / kFpc; }

void Game::face(int& h, int& m, int& s) const {
    int t = clockSec();
    if (t < 0) t = 0;
    s = t % 60;
    m = (t / 60) % 60;
    h = (t / 3600) % 12;
    if (h == 0) h = 12;
}

int Game::hour() const {
    int h, m, s;
    face(h, m, s);
    return h;
}

int Game::minute() const {
    int h, m, s;
    face(h, m, s);
    return m;
}

int Game::second() const {
    int h, m, s;
    face(h, m, s);
    return s;
}

bool Game::onHour() const {
    int sec = clockSec();
    return sec >= kHourSec && sec < kHourSec + kGraceSec;
}

bool Game::pastHour() const { return clockSec() >= kHourSec + kGraceSec; }

void Game::blip(float freq) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, 0.22f);
}

void Game::blank() {
    for (int i = 0; i < kCuts; i++) cutOn_[i] = false;
    locked_ = 0;
    phase_ = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    reason_ = "";
    won_ = false;
    over_ = false;
    dead_ = 0;
    tryNo_ = 1;
    playFrames_ = 0;
    hold_ = 0;
    flash_ = 0;
    strikes_ = 0;
    swing_ = 0;
    blank();
}

void Game::beginFail(const char* why) {
    if (won_) return;
    reason_ = why;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    hold_ = 0;
    blip(90.f);
}

void Game::dieTry() {
    dead_++;
    flash_ = 18;
    blip(110.f);
    if (dead_ >= kTries) {
        beginFail("DEAD");
        return;
    }
    tryNo_ = dead_ + 1;
    blank();
}

void Game::strike() {
    if (mode_ != Mode::Play) return;
    if (!hot()) {
        dieTry();
        return;
    }
    int i = openCut();
    if (i < 0) return;
    cutOn_[i] = true;
    locked_++;
    phase_ = 0;
    blip(392.f + float(locked_) * 48.f);
    if (sealed()) {
        mode_ = Mode::Hold;
        flash_ = 0;
    }
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    mode_ = Mode::Chime;
    hold_ = 0;
    strikes_ = 0;
    swing_ = 10;
    if (!sys_) return;
    sys_->apu.tone(0, 523.f, 0.28f);
    sys_->apu.tone(1, 784.f, 0.16f);
    sys_->rumble(0.25f, 0.45f, 90);
}

void Game::askHour() {
    if (mode_ != Mode::Hold || !sealed()) return;
    if (onHour()) beginChime();
    else if (pastHour()) beginFail("HOUR");
    else {
        flash_ = 16;
        reason_ = "EARLY";
        blip(160.f);
    }
}

void Game::botAct() {
    if (mode_ == Mode::Play) {
        float n = needle();
        if (n > 0.47f && n < 0.53f) strike();
        return;
    }
    if (mode_ == Mode::Hold && onHour()) askHour();
}

void Game::human(const gs::Pad& pad) {
    if (mode_ == Mode::Play) {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) strike();
        return;
    }
    if (mode_ == Mode::Hold) {
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START)) askHour();
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = kCuts == 4 && kTries == 3 && kGraceSec > 0 && kLeadSec > kGraceSec && kPeriod > 8;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.enabled = true;
    sys.apu.setMaster(0.7f);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    reason_ = "";
    titleWait_ = 0;
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        titleWait_++;
        bool go = bot_ ? titleWait_ > 12 : (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) && titleWait_ > 4;
        if (go) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play || mode_ == Mode::Hold) {
        if (bot_) botAct();
        else human(pad);
        if (mode_ == Mode::Play || mode_ == Mode::Hold) {
            playFrames_++;
            if (mode_ == Mode::Play) phase_++;
            if (pastHour()) beginFail(sealed() ? "LATE" : "HOUR");
        }
    } else if (mode_ == Mode::Chime) {
        hold_++;
        if (hold_ % 8 == 0 && strikes_ < 4) {
            strikes_++;
            swing_ = 12;
            float f = strikes_ & 1 ? 659.f : 784.f;
            sys.apu.tone(0, f, 0.2f);
        }
        if (hold_ > 48) {
            mode_ = Mode::Leave;
            hold_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        if (++hold_ > 16) over_ = true;
    } else if (mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
            titleWait_ = 0;
            sys.apu.silence();
        }
    }

    if (swing_ > 0) swing_--;
    if (flash_ > 0) flash_--;
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int(std::lround(cx - img.w * 0.5f));
    s.y = int(std::lround(cy - img.h * 0.5f));
    s.w = img.w;
    s.h = img.h;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hand(float cx, float cy, float ang, float len, int pal) {
    float dx = std::sin(ang);
    float dy = -std::cos(ang);
    int n = std::max(2, int(len));
    for (int i = 1; i <= n; i++) {
        float t = float(i) / float(n);
        spr(art_.pip, cx + dx * len * t * 1.6f, cy + dy * len * t * 1.6f, pal);
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

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 5);
    uint16_t mid = gs::rgb4(6, 4, 4);
    uint16_t bot = gs::rgb4(3, 2, 1);
    if (mode_ == Mode::Chime || mode_ == Mode::Leave) mid = gs::rgb4(12, 8, 3);
    if (mode_ == Mode::Fail) {
        top = gs::rgb4(2, 0, 1);
        mid = gs::rgb4(5, 1, 1);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
        auto mix = [&](uint16_t a, uint16_t b, float t) {
            t = std::clamp(t, 0.f, 1.f);
            auto L = [&](int sh) { return int(std::lround(ch(a, sh) + (ch(b, sh) - ch(a, sh)) * t)); };
            return gs::rgb4(L(8), L(4), L(0));
        };
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.mask, 150.f, 112.f, mode_ == Mode::Fail ? PAL_DIM : PAL_FACE);
    for (int i = 0; i < kCuts; i++) {
        if (!cutOn_[i] && mode_ != Mode::Chime && mode_ != Mode::Leave) continue;
        int pal = (mode_ == Mode::Chime || mode_ == Mode::Leave) ? PAL_LIT : PAL_HOLE;
        spr(art_.hole, kGroove[i].hx, kGroove[i].hy, pal);
    }

    if (mode_ == Mode::Play) {
        spr(art_.rail, 160.f, 186.f, PAL_WOOD);
        float n = needle();
        float tx = kRail0 + n * (kRail1 - kRail0);
        spr(art_.tick, tx, 176.f, hot() ? PAL_GOLD : PAL_TICK);
    }

    float ox = std::sin(float(sys_->frame) * 0.8f) * float(swing_) * 0.45f;
    int bp = PAL_BELL;
    if (mode_ == Mode::Chime || mode_ == Mode::Leave) bp = PAL_LIT;
    else if (mode_ == Mode::Hold) bp = PAL_GOLD;
    spr(art_.bell, 286.f + ox, 36.f, bp);

    int h = 11, m = 58, s = 48;
    if (mode_ != Mode::Title) face(h, m, s);
    spr(art_.face, 40.f, 36.f, PAL_CLOCK);
    float minuteAng = (float(m) / 60.f) * 2.f * kPi;
    float hourAng = ((float(h % 12) + float(m) / 60.f) / 12.f) * 2.f * kPi;
    hand(40.f, 36.f, minuteAng, 8.f, PAL_INK);
    hand(40.f, 36.f, hourAng, 5.f, PAL_GOLD);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(8, "S3 MASK CHIME", PAL_GOLD);
        hudC(11, "PLAY MASK UNTIL", PAL_HUD);
        hudC(12, "THE HOUR HAS TO CHIME", PAL_GOLD);
        hudC(14, "THEN LEAVE", PAL_HUD);
        hudC(20, "A CUTS THE OPEN GROOVE", PAL_DIM);
        hudC(21, "A LEAVES ON THE HOUR", PAL_DIM);
        hudC(23, "THREE TRIES", PAL_HUD);
        if ((sys_->frame & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_DIM);
    } else if (mode_ == Mode::Fail) {
        hudC(1, "THE HOUR IS GONE", PAL_BAD);
        hudC(25, reason_, PAL_HUD);
        if ((sys_->frame & 16) == 0) hudC(27, "START", PAL_GOLD);
    } else if (mode_ == Mode::Chime) {
        hudC(1, "THE HOUR CHIMES", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        hudC(25, buf, PAL_GOLD);
        hudC(26, "LEAVE", PAL_HUD);
    } else if (mode_ == Mode::Leave) {
        hudC(1, "LEAVE", PAL_GOLD);
        hudC(25, "THE HOUR CHIMED", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "TRY %d/%d", tryNo_, kTries);
        hud(1, 24, buf, dead_ ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "CUTS %d", locked_);
        hud(28, 24, buf, sealed() ? PAL_GOLD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d:%02d:%02d", h, m, s);
        hud(16, 1, buf, onHour() ? PAL_GOLD : PAL_HUD);
        if (flash_ > 0 && mode_ == Mode::Hold) hudC(25, "NOT YET", PAL_BAD);
        else if (mode_ == Mode::Hold) hudC(25, onHour() ? "THE HOUR CHIMES" : "HOLD FOR THE HOUR", PAL_GOLD);
        else if (hot()) {
            std::snprintf(buf, sizeof buf, "CUT %s", kGroove[openCut()].name);
            hudC(25, buf, PAL_GOLD);
        } else {
            int i = openCut();
            std::snprintf(buf, sizeof buf, "WAIT %s", i >= 0 ? kGroove[i].name : "MASK");
            hudC(25, buf, PAL_DIM);
        }
        hudC(27, "A STRIKE", PAL_DIM);
    }
}

}  // namespace maskchime
