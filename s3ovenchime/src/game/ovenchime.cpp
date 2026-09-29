#include "game/ovenchime.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace ovenchime {
namespace {

constexpr int kStrikes = 3;
constexpr int kTick = 6;
constexpr float kGoldLo = 46.f;
constexpr float kGoldHi = 64.f;
constexpr float kRise = 0.42f;

}  // namespace

bool Game::inGold() const { return heat_ >= kGoldLo && heat_ <= kGoldHi; }

void Game::tone(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 1.5f, vol * 0.4f);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = won_ = false;
    door_ = false;
    hourHold_ = false;
    strikes_ = 0;
    hold_ = 0;
    tick_ = 0;
    hour_ = 11;
    minute_ = 59;
    second_ = 50;
    heat_ = 40.f;
    swing_ = 0.f;
    why_ = "";
    std::snprintf(note_, sizeof note_, "WAIT FOR THE HOUR");
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    buildArt(sys.vdp, art_);
    toTitle();
}

void Game::begin() {
    toTitle();
    mode_ = Mode::Tend;
    std::snprintf(note_, sizeof note_, "SHUT THE DOOR");
    tone(220.f, 0.08f);
}

void Game::chime() {
    mode_ = Mode::Chime;
    hold_ = 48;
    hour_ = 12;
    minute_ = 0;
    second_ = 0;
    why_ = "THE HOUR CHIMES";
    std::snprintf(note_, sizeof note_, "THE HOUR CHIMES");
    tone(523.f, 0.2f);
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    hold_ = 36;
    why_ = why;
    std::snprintf(note_, sizeof note_, "%s", why);
    tone(90.f, 0.12f);
}

void Game::finish(bool win) {
    won_ = win;
    over_ = true;
    mode_ = Mode::Over;
    if (win) {
        hour_ = 12;
        minute_ = 0;
        second_ = 0;
    }
}

void Game::miss(const char* why) {
    strikes_++;
    why_ = why;
    std::snprintf(note_, sizeof note_, "%s", why);
    tone(140.f, 0.1f);
    if (strikes_ >= kStrikes) fail("THREE STRIKES");
}

void Game::strike() {
    if (!hourHold_) {
        miss("TOO SOON");
        return;
    }
    if (!door_) {
        miss("DOOR OPEN");
        return;
    }
    if (!inGold()) {
        miss("OFF THE GOLD");
        return;
    }
    chime();
}

void Game::tickClock() {
    if (hourHold_ || mode_ != Mode::Tend) return;
    if (++tick_ < kTick) return;
    tick_ = 0;
    second_++;
    if (second_ < 60) return;
    second_ = 0;
    minute_++;
    if (minute_ < 60) return;
    minute_ = 0;
    hour_ = 12;
    hourHold_ = true;
    hold_ = 90;
    std::snprintf(note_, sizeof note_, "THE HOUR");
    tone(392.f, 0.1f);
}

void Game::botIntent(bool& left, bool& right, bool& doorTap, bool& strikeTap) {
    left = right = doorTap = strikeTap = false;
    if (mode_ != Mode::Tend) return;
    if (!door_) doorTap = true;
    if (heat_ > 55.f) left = true;
    else if (heat_ < 50.f) right = true;
    if (hourHold_ && door_ && inGold()) strikeTap = true;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT);
    bool right = pad.down(gs::BTN_RIGHT);
    bool doorTap = pad.pressed(gs::BTN_B);
    bool strikeTap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
    bool start = pad.pressed(gs::BTN_START);
    if (bot_) botIntent(left, right, doorTap, strikeTap);

    if (mode_ == Mode::Title) {
        if (bot_ || start || strikeTap) begin();
    } else if (mode_ == Mode::Tend) {
        heat_ += kRise;
        if (left) heat_ -= 1.15f;
        if (right) heat_ += 0.85f;
        if (heat_ < 0.f) heat_ = 0.f;
        if (heat_ > 100.f) heat_ = 100.f;
        if (doorTap) {
            door_ = !door_;
            std::snprintf(note_, sizeof note_, door_ ? "DOOR SHUT" : "DOOR OPEN");
        }
        tickClock();
        if (mode_ == Mode::Tend && strikeTap) strike();
        if (mode_ == Mode::Tend && hourHold_) {
            swing_ += 0.35f;
            if (--hold_ <= 0) fail("THE HOUR PASSED");
        }
        if (mode_ == Mode::Tend && heat_ >= 98.f) fail("CHARRED");
    } else if (mode_ == Mode::Chime) {
        swing_ += 0.7f;
        if (--hold_ <= 0) finish(true);
    } else if (mode_ == Mode::Fail) {
        if (--hold_ <= 0) finish(false);
    } else if (mode_ == Mode::Over) {
        if (!bot_ && start) toTitle();
    }

    if (mode_ != Mode::Chime && !(mode_ == Mode::Over && won_)) {
        if (sys.frame % 8 == 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
        }
    }
    draw();
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(2, 2, 4);
        if (y > 40) c = gs::rgb4(5, 3, 3);
        if (y > 150) c = gs::rgb4(3, 2, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.oven, 160.f, 128.f, float(art_.oven.w), float(art_.oven.h), PAL_OVEN);
    int loafPal = PAL_LOAF;
    if (inGold()) loafPal = PAL_GOLD;
    else if (heat_ > kGoldHi) loafPal = PAL_ALERT;
    spr(art_.loaf, 168.f, 132.f, float(art_.loaf.w), float(art_.loaf.h), loafPal);
    for (int i = 0; i < 4; i++) {
        float fh = 14.f + 4.f * std::sin(float(sys_->frame) * 0.4f + float(i));
        spr(art_.flame, 112.f + float(i) * 18.f, 176.f, 10.f, fh, PAL_FIRE);
    }
    float doorX = door_ ? 118.f : 78.f;
    spr(art_.door, doorX, 128.f, float(art_.door.w) * 0.7f, float(art_.door.h) * 0.7f, PAL_DOOR);

    spr(art_.face, 250.f, 52.f, float(art_.face.w), float(art_.face.h), PAL_CLOCK);
    float handY = hourHold_ || (mode_ == Mode::Over && won_) ? 40.f : 48.f;
    float handX = hourHold_ || (mode_ == Mode::Over && won_) ? 250.f : 258.f + std::sin(swing_) * 2.f;
    spr(art_.hand, handX, handY, float(art_.hand.w), float(art_.hand.h), PAL_HAND);

    int filled = int(heat_ / 100.f * 16.f);
    for (int i = 0; i < 16; i++) {
        int pal = PAL_INK;
        if (i >= 7 && i <= 10) pal = PAL_GOLD;
        if (i >= filled) pal = PAL_OVEN;
        if (i < filled && (i < 7 || i > 10)) pal = heat_ > kGoldHi ? PAL_ALERT : PAL_OK;
        spr(art_.pip, 96.f + float(i) * 8.f, 198.f, 6.f, 6.f, pal);
    }
    for (int i = 0; i < kStrikes; i++) {
        int pal = i < strikes_ ? PAL_ALERT : PAL_OK;
        spr(art_.pip, 200.f + float(i) * 10.f, 198.f, 6.f, 6.f, pal);
    }

    char clock[16];
    std::snprintf(clock, sizeof clock, "%d:%02d:%02d", hour_, minute_, second_);
    if (mode_ == Mode::Title) {
        hudC(3, "S3 OVENCHIME", PAL_GOLD);
        hudC(6, "THE HOUR HAS TO CHIME", PAL_INK);
        hudC(8, "SHUT THE DOOR IN THE GOLD", PAL_INK);
        hudC(22, "START", PAL_OK);
    } else if (mode_ == Mode::Over) {
        hudC(2, won_ ? "THE HOUR CHIMES" : note_, won_ ? PAL_GOLD : PAL_ALERT);
        hudC(4, clock, PAL_OK);
        if (!bot_) hudC(22, won_ ? "DONE" : "START", won_ ? PAL_OK : PAL_ALERT);
    } else {
        hudC(2, note_, mode_ == Mode::Chime ? PAL_GOLD : PAL_INK);
        hudC(4, clock, hourHold_ ? PAL_GOLD : PAL_OK);
        hudC(24, "ARROWS HEAT  B DOOR  A STRIKE", PAL_INK);
    }
}

}  // namespace ovenchime
