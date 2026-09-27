#include "game/skate.h"

#include <cstdio>
#include <cstring>

namespace skate {
namespace {
constexpr float kMinX = 40.f;
constexpr float kMaxX = 980.f;
constexpr float kCamLead = 96.f;
}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += tapePay(i);
    return s;
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 1.5f, vol * 0.4f);
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

void Game::spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    const gs::Image& img = m.pick(h);
    spr(img, cx - w * 0.5f, cy - h * 0.5f, w, h, pal, flip);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleT_ = 0;
    x_ = 80.f;
    vx_ = 0.f;
    hop_ = 0.f;
    meter_ = 0.2f;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    pops_ = 0;
    std::snprintf(note_, sizeof note_, "MATCH THE TAPE");
}

void Game::newRun() {
    mode_ = Mode::Ride;
    over_ = false;
    won_ = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    pops_ = 0;
    cool_ = 0;
    hop_ = 0.f;
    x_ = 80.f;
    meter_ = 0.15f;
    meterDir_ = 1.f;
    mark_ = -1;
    line_ = -1;
    std::snprintf(note_, sizeof note_, "POP ON THE TAPE");
    blip(220.f, 0.05f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.HUD.enabled = true;
    sys.vdp.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.road[y].on = false;
        float t = float(y) / float(gs::SCREEN_H);
        int sky = t < 0.62f ? 1 : 0;
        (void)sky;
        int r = 4 + int((1.f - t) * 4);
        int g = 6 + int((1.f - t) * 3);
        int b = 10 + int((1.f - t) * 4);
        if (y > 150) {
            r = 3;
            g = 3;
            b = 4;
        }
        sys.vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        sys.vdp.lineFog[y] = 0;
    }
    toTitle();
}

void Game::pop() {
    if (mode_ != Mode::Ride || cool_ > 0 || pops_ >= kMaxPops) return;
    firm_ = firmMeter(meter_);
    mark_ = markAt(x_);
    line_ = takenLine(firm_, x_);
    pops_++;
    hop_ = 18.f;
    cool_ = 28;
    mode_ = Mode::Judge;
    judgeT_ = 0;
    if (line_ >= 0 && !held_[line_]) {
        held_[line_] = true;
        std::snprintf(note_, sizeof note_, "%s IN THE DRAWER", kMark[mark_].name);
        blip(440.f, 0.06f);
    } else if (line_ >= 0) {
        std::snprintf(note_, sizeof note_, "%s ALREADY IN", kMark[mark_].name);
        blip(180.f, 0.04f);
    } else if (firm_ && mark_ >= 0) {
        std::snprintf(note_, sizeof note_, "%s STAYS OUT", kMark[mark_].name);
        blip(110.f, 0.05f);
    } else if (mark_ >= 0) {
        std::snprintf(note_, sizeof note_, "SLOPPY %s", kMark[mark_].name);
        blip(90.f, 0.04f);
    } else {
        std::snprintf(note_, sizeof note_, "NO MARK");
        blip(80.f, 0.03f);
    }
}

void Game::judge() {
    judgeT_++;
    if (hop_ > 0.f) hop_ -= 0.7f;
    if (judgeT_ < 36) return;
    if (matched() && drawerScore() == tapeSum()) {
        mode_ = Mode::Leave;
        leaveT_ = 0;
        std::snprintf(note_, sizeof note_, "DRAWER MATCHES THE TAPE");
        blip(523.f, 0.07f);
        return;
    }
    if (pops_ >= kMaxPops) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = true;
        std::snprintf(note_, sizeof note_, "DOES NOT MATCH");
        return;
    }
    mode_ = Mode::Ride;
}

void Game::botRide() {
    if (mode_ == Mode::Title) {
        if (++titleT_ > 12) newRun();
        return;
    }
    if (mode_ != Mode::Ride) return;
    int line = -1;
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) {
            line = i;
            break;
        }
    if (line < 0) return;
    float tx = kMark[markOfLine(line)].x;
    float d = tx - x_;
    if (std::fabs(d) > 4.f) {
        vx_ = (d > 0.f ? 1.f : -1.f) * 3.4f;
        return;
    }
    vx_ = 0.f;
    x_ = tx;
    if (cool_ <= 0 && firmMeter(meter_)) pop();
}

void Game::humanRide(const gs::Pad& pad) {
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) newRun();
        return;
    }
    if (mode_ == Mode::Over || mode_ == Mode::Lose) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) toTitle();
        return;
    }
    if (mode_ != Mode::Ride) return;
    float dir = 0.f;
    if (pad.down(gs::BTN_LEFT)) dir -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) dir += 1.f;
    if (std::fabs(pad.axisX) > 0.2f) dir = pad.axisX;
    vx_ = dir * 2.6f;
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C)) pop();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    meter_ += meterDir_ * 0.016f;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    if (cool_ > 0) cool_--;
    if (bot_) botRide();
    else humanRide(sys.pad);

    if (mode_ == Mode::Ride || mode_ == Mode::Judge || mode_ == Mode::Leave) {
        x_ += vx_;
        if (x_ < kMinX) x_ = kMinX;
        if (x_ > kMaxX) x_ = kMaxX;
    }
    if (mode_ == Mode::Judge) judge();
    if (mode_ == Mode::Leave) {
        leaveT_++;
        hop_ = 6.f + std::sin(leaveT_ * 0.35f) * 4.f;
        vx_ = 1.6f;
        if (leaveT_ > 70) {
            won_ = matched() && drawerScore() == tapeSum();
            over_ = true;
            mode_ = Mode::Over;
            if (won_) std::snprintf(note_, sizeof note_, "DRAWER MATCHES THE TAPE");
            else std::snprintf(note_, sizeof note_, "DOES NOT MATCH");
        }
    }
    if (mode_ == Mode::Ride && hop_ > 0.f) hop_ -= 1.f;

    float want = x_ - kCamLead;
    if (want < 0.f) want = 0.f;
    cam_ += (want - cam_) * 0.18f;

    sys.apu.tone(0, 0, 0);
    sys.apu.tone(1, 0, 0);
    if (mode_ == Mode::Ride && firmMeter(meter_)) sys.apu.tone(2, 660.f, 0.015f);
    else sys.apu.tone(2, 0, 0);

    draw();
}

void Game::backdrop() {
    int hs = int(cam_) & 7;
    for (int y = 0; y < gs::SCREEN_H; y++) sys_->vdp.B.hscroll[y] = int16_t(-int(cam_) + (y >= 152 ? hs : 0));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float sx = x_ - cam_;
    float gy = 168.f - hop_;
    bool faceL = vx_ < -0.2f;
    sprM(art_.board, sx, gy + 16.f, 10.f, PAL_DECK, faceL);
    sprM(art_.skater, sx, gy - 2.f, hop_ > 2.f ? 40.f : 36.f, PAL_YOU, faceL);

    for (int i = 0; i < kMarkN; i++) {
        float px = kMark[i].x - cam_;
        if (px < -20.f || px > 340.f) continue;
        int pal = kMark[i].line < 0 ? PAL_BAD : PAL_GOOD;
        spr(art_.post, px - 4.f, 128.f, 8.f, 28.f, pal);
    }

    spr(art_.cassette, 8.f, 6.f, 36.f, 22.f, PAL_TAPE);
    spr(art_.drawer, 76.f, 188.f, 168.f, 28.f, PAL_WOOD);
    for (int i = 0; i < kTapeN; i++) {
        float sx0 = 84.f + i * 54.f;
        spr(art_.slot, sx0, 193.f, 48.f, 18.f, PAL_PAPER);
        if (held_[i]) spr(art_.slip[i], sx0 + 2.f, 195.f, 44.f, 14.f, PAL_GOOD);
    }

    int barX = 250;
    int barW = 60;
    hud(1, 1, "TAPE", PAL_GOLD);
    char line[48];
    std::snprintf(line, sizeof line, "%s %d", tapeName(0), tapePay(0));
    hud(8, 1, line, held_[0] ? PAL_GOOD : PAL_INK);
    std::snprintf(line, sizeof line, "%s %d", tapeName(1), tapePay(1));
    hud(18, 1, line, held_[1] ? PAL_GOOD : PAL_INK);
    std::snprintf(line, sizeof line, "%s %d", tapeName(2), tapePay(2));
    hud(28, 1, line, held_[2] ? PAL_GOOD : PAL_INK);

    hud(31, 3, "POP", PAL_INK);
    int fill = int(meter_ * 7.f);
    if (fill > 7) fill = 7;
    char meter[8];
    for (int i = 0; i < 7; i++) meter[i] = i < fill ? '#' : '-';
    meter[7] = 0;
    hud(31, 4, meter, firmMeter(meter_) ? PAL_GOOD : PAL_GOLD);
    (void)barX;
    (void)barW;

    hud(1, 24, "DRAWER", PAL_WOOD);
    for (int i = 0; i < kTapeN; i++) {
        hud(10 + i * 7, 24, held_[i] ? tapeName(i) : "----", held_[i] ? PAL_GOOD : PAL_INK);
    }

    if (mode_ == Mode::Title) {
        hudC(8, "S3 SKATETAPE", PAL_GOLD);
        hudC(10, "A SHORT SKATE", PAL_INK);
        hudC(12, "THE DRAWER HAS TO MATCH THE TAPE", PAL_PAPER);
        hudC(14, "POP OLLIE  GRIND  KICK", PAL_GOOD);
        hudC(15, "POP  SLIDE  SHOVE STAY OUT", PAL_BAD);
        hudC(18, "LEFT RIGHT   A POPS", PAL_INK);
        hudC(20, "START", PAL_GOLD);
    } else {
        hudC(8, note_, mode_ == Mode::Over && won_ ? PAL_WIN : (mode_ == Mode::Lose ? PAL_BAD : PAL_INK));
        int near = markAt(x_);
        if (near >= 0 && mode_ == Mode::Ride) {
            std::snprintf(line, sizeof line, "%s %d", kMark[near].name, kMark[near].pay);
            hudC(10, line, kMark[near].line < 0 ? PAL_BAD : PAL_GOOD);
        }
        std::snprintf(line, sizeof line, "POPS %d/%d", pops_, kMaxPops);
        hud(1, 26, line, PAL_INK);
        if (mode_ == Mode::Over && won_) hudC(12, "YOU LEAVE", PAL_WIN);
        if (mode_ == Mode::Lose) hudC(12, "STILL OPEN", PAL_BAD);
    }
}

}  // namespace skate
