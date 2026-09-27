#include "game/golftape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace golftape {

namespace {

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += tapePay(i);
    return s;
}

const char* Game::tapeLabel(int i) const { return tapeName(i); }
int Game::tapeScore(int i) const { return tapePay(i); }

const char* Game::modeName() const {
    Mode m = mode_ == Mode::Pause ? heldMode_ : mode_;
    switch (m) {
    case Mode::Title: return "TITLE";
    case Mode::Aim: return "AIM";
    case Mode::Flight: return "FLIGHT";
    case Mode::Pocket: return "POCKET";
    case Mode::Judge: return "JUDGE";
    case Mode::Pause: return "PAUSE";
    case Mode::Over: return won_ ? "CLOSED" : "OPEN";
    }
    return "?";
}

bool Game::audit() const {
    if (kTapeN != 3) return false;
    if (std::strcmp(tapeName(0), "FADE") != 0 || std::strcmp(tapeName(1), "PITCH") != 0 ||
        std::strcmp(tapeName(2), "DROP") != 0)
        return false;
    if (tapePay(0) != 4 || tapePay(1) != 3 || tapePay(2) != 1) return false;
    if (tapeSum() != 8) return false;
    if (std::strcmp(twinName(0), "DRAW") != 0 || std::strcmp(twinName(1), "FLIP") != 0 ||
        std::strcmp(twinName(2), "LIP") != 0)
        return false;
    if (!inFade(0.40f) || !inDraw(-0.40f) || !inPitch(0.f) || !inFlip(0.50f) || !inDrop(0.f) || !inLip(0.32f))
        return false;
    if (inFade(-0.40f) || inPitch(0.50f) || inDrop(0.32f) || firmMeter(0.2f) || !firmMeter(0.50f)) return false;
    return true;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::snprintf(reason_, sizeof reason_, "tape");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.HUD.clear();
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    left_ = false;
    shots_ = 0;
    traps_ = 0;
    board_ = 0;
    stroke_ = 0;
    titleT_ = 0;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    ballX_ = 52.f;
    ballY_ = kGround;
    manX_ = 28.f;
    aim_ = 0.f;
    meter_ = 0.f;
    meterDir_ = 1.f;
    if (!rules_) std::snprintf(reason_, sizeof reason_, "tape");
    else reason_[0] = 0;
}

void Game::newRound() {
    shots_ = 0;
    traps_ = 0;
    board_ = 0;
    stroke_ = 0;
    won_ = false;
    left_ = false;
    over_ = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    ballX_ = 52.f;
    ballY_ = kGround;
    manX_ = 28.f;
    beginAim();
}

void Game::beginAim() {
    mode_ = Mode::Aim;
    aim_ = 0.f;
    meter_ = 0.f;
    meterDir_ = 1.f;
    read_ = Read::None;
    ballY_ = kGround;
}

Game::Read Game::readAim() const {
    if (stroke_ == 0) {
        if (inFade(aim_)) return Read::Fade;
        if (inDraw(aim_)) return Read::Draw;
    } else if (stroke_ == 1) {
        if (inPitch(aim_)) return Read::Pitch;
        if (inFlip(aim_)) return Read::Flip;
    } else if (stroke_ == 2) {
        if (inDrop(aim_)) return Read::Drop;
        if (inLip(aim_)) return Read::Lip;
    }
    return Read::None;
}

int Game::payOf(Read r) const {
    if (r == Read::Fade || r == Read::Draw) return 4;
    if (r == Read::Pitch || r == Read::Flip) return 3;
    if (r == Read::Drop || r == Read::Lip) return 1;
    return 0;
}

bool Game::tapeRead(Read r) const {
    return (stroke_ == 0 && r == Read::Fade) || (stroke_ == 1 && r == Read::Pitch) ||
           (stroke_ == 2 && r == Read::Drop);
}

float Game::landX(Read r) const {
    if (r == Read::Fade) return 156.f;
    if (r == Read::Draw) return 108.f;
    if (r == Read::Pitch) return 214.f;
    if (r == Read::Flip) return 248.f;
    if (r == Read::Drop) return kCupX;
    if (r == Read::Lip) return 248.f;
    float nudge = aim_ * 36.f;
    if (stroke_ == 0) return clampf(90.f + nudge, 70.f, 180.f);
    if (stroke_ == 1) return clampf(ballX_ + 40.f + nudge, ballX_ + 16.f, 280.f);
    return clampf(kCupX + nudge * 0.6f, 220.f, 290.f);
}

const char* Game::readName(Read r) const {
    switch (r) {
    case Read::Fade: return "FADE";
    case Read::Draw: return "DRAW";
    case Read::Pitch: return "PITCH";
    case Read::Flip: return "FLIP";
    case Read::Drop: return "DROP";
    case Read::Lip: return "LIP";
    case Read::None: return "SKID";
    }
    return "SKID";
}

const char* Game::clubName() const {
    if (stroke_ <= 0) return "DRIVER";
    if (stroke_ == 1) return "WEDGE";
    return "PUTTER";
}

void Game::blip(int ch, float freq, float vol) { sys_->apu.tone(ch, freq, vol); }

void Game::tickAudio() {
    if (toneKill_ > 0) {
        toneKill_--;
        if (toneKill_ == 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
            sys_->apu.tone(2, 0, 0);
        }
    }
}

void Game::swing() {
    read_ = readAim();
    bool sweet = firmMeter(meter_);
    int pay = sweet ? payOf(read_) : 0;
    if (!sweet) read_ = Read::None;
    shots_++;
    board_ += pay;
    if (tapeRead(read_)) held_[stroke_] = true;
    else traps_++;
    fromX_ = ballX_;
    toX_ = landX(read_);
    if (stroke_ == 2 && read_ == Read::Drop) toX_ = kCupX;
    apex_ = stroke_ == 2 ? 10.f : (stroke_ == 1 ? 34.f : 52.f);
    flightT_ = 0;
    mode_ = Mode::Flight;
    blip(0, sweet ? 520.f : 180.f, 0.12f);
    toneKill_ = 8;
}

void Game::finish() {
    bool closed = matched() && traps_ == 0 && shots_ == 3 && drawerScore() == tapeSum() && board_ == tapeSum();
    if (closed) {
        won_ = true;
        left_ = true;
        std::snprintf(reason_, sizeof reason_, "matched");
    } else {
        won_ = false;
        left_ = false;
        std::snprintf(reason_, sizeof reason_, "open");
    }
    over_ = true;
    mode_ = Mode::Over;
    if (won_) {
        blip(0, 523.f, 0.14f);
        blip(1, 659.f, 0.12f);
        blip(2, 784.f, 0.10f);
        toneKill_ = 30;
    } else {
        blip(0, 140.f, 0.12f);
        toneKill_ = 16;
    }
}

void Game::botAim() {
    if (stroke_ == 0) aim_ = 0.40f;
    else if (stroke_ == 1) aim_ = 0.f;
    else aim_ = 0.f;
    if (firmMeter(meter_)) swing();
}

void Game::humanAim(const gs::Pad& pad) {
    if (pad.down(gs::BTN_LEFT)) aim_ -= 0.018f;
    if (pad.down(gs::BTN_RIGHT)) aim_ += 0.018f;
    aim_ = clampf(aim_, -1.f, 1.f);
    if (pad.axisX != 0.f && !pad.down(gs::BTN_LEFT) && !pad.down(gs::BTN_RIGHT))
        aim_ = clampf(aim_ + pad.axisX * 0.02f, -1.f, 1.f);
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C)) swing();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause && mode_ != Mode::Title && mode_ != Mode::Over && pad.pressed(gs::BTN_START)) {
        heldMode_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Pause && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        mode_ = heldMode_;
    }

    Mode live = mode_ == Mode::Pause ? heldMode_ : mode_;
    if (mode_ != Mode::Pause) {
        if (live == Mode::Title) {
            titleT_++;
            if (bot_) {
                if (titleT_ > 24 && rules_) newRound();
            } else if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_B)) {
                if (rules_) newRound();
            }
        } else if (live == Mode::Aim) {
            meter_ += meterDir_ * 0.010f;
            if (meter_ >= 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            } else if (meter_ <= 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
            if (bot_) botAim();
            else humanAim(pad);
        } else if (live == Mode::Flight) {
            flightT_++;
            float u = clampf(flightT_ / 42.f, 0.f, 1.f);
            float e = u * u * (3.f - 2.f * u);
            ballX_ = fromX_ + (toX_ - fromX_) * e;
            ballY_ = kGround - std::sin(u * 3.14159265f) * apex_;
            manX_ += (fromX_ - 24.f - manX_) * 0.08f;
            if (flightT_ == 20) {
                blip(1, stroke_ == 2 ? 300.f : 220.f, 0.08f);
                toneKill_ = 6;
            }
            if (flightT_ >= 42) {
                ballX_ = toX_;
                ballY_ = kGround;
                if (tapeRead(read_)) {
                    pocketT_ = 0;
                    slipX_ = ballX_;
                    slipY_ = ballY_ - 20.f;
                    slipToX_ = 28.f + float(stroke_) * 100.f;
                    slipToY_ = 196.f;
                    mode_ = Mode::Pocket;
                    blip(0, 660.f, 0.10f);
                    toneKill_ = 10;
                } else if (stroke_ >= 2) {
                    judgeT_ = 0;
                    mode_ = Mode::Judge;
                } else {
                    stroke_++;
                    manX_ = ballX_ - 24.f;
                    beginAim();
                }
            }
        } else if (live == Mode::Pocket) {
            pocketT_++;
            float u = clampf(pocketT_ / 28.f, 0.f, 1.f);
            slipX_ = slipX_ + (slipToX_ - slipX_) * 0.18f;
            slipY_ = slipY_ + (slipToY_ - slipY_) * 0.18f;
            if (u >= 1.f) {
                if (stroke_ >= 2) {
                    judgeT_ = 0;
                    mode_ = Mode::Judge;
                } else {
                    stroke_++;
                    manX_ = ballX_ - 24.f;
                    beginAim();
                }
            }
        } else if (live == Mode::Judge) {
            judgeT_++;
            manX_ += 1.4f;
            if (manX_ > 300.f) left_ = matched();
            if (judgeT_ >= 36) finish();
        } else if (live == Mode::Over) {
            if (!bot_ && (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_START))) toTitle();
        }
    }
    tickAudio();
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!img.w || !img.h) return;
    gs::Sprite s;
    s.img = img;
    s.w = int(w);
    s.h = int(h);
    s.x = int(cx - w * 0.5f);
    s.y = int(cy - h * 0.5f);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 2.f) return;
    const gs::Image& img = m.pick(h);
    float w = img.h ? h * (float(img.w) / float(img.h)) : h;
    spr(img, cx, cy, w, h, pal, flip);
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c;
        if (y < 70) {
            int t = y / 14;
            c = gs::rgb4(4, 6 + (t & 1), 11);
        } else if (y < 120) c = gs::rgb4(5, 8, 12);
        else if (y < 176) c = gs::rgb4(3, 8, 3);
        else c = gs::rgb4(5, 3, 2);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    spr(art_.course, 160.f, 132.f, 320.f, 96.f, PAL_WORLD);
    int wave = (sys_->frame / 10) & 1;
    sprM(art_.flag[wave], 250.f, 118.f, 36.f, PAL_FLAG);
    int pose = (mode_ == Mode::Flight && flightT_ < 18) ? 1 : 0;
    sprM(art_.golfer[pose], manX_, 142.f, 52.f, PAL_MAN, false);
    float bh = (mode_ == Mode::Flight && flightT_ > 36) ? 10.f : 14.f;
    sprM(art_.ball, ballX_, ballY_ - 6.f, bh, PAL_BALL);

    spr(art_.paper, 62.f, 40.f, 108.f, 72.f, PAL_PAPER);
    spr(art_.drawer, 160.f, 206.f, 300.f, 28.f, PAL_WOOD);

    hud(2, 1, "TAPE", PAL_INK);
    for (int i = 0; i < kTapeN; i++) {
        char line[24];
        std::snprintf(line, sizeof line, "%s %d", tapeName(i), tapePay(i));
        hud(2, 3 + i, line, held_[i] ? PAL_GREEN : PAL_INK);
    }
    hud(16, 1, "TWINS STAY OUT", PAL_ALERT);
    hud(16, 2, "DRAW 4", PAL_INK);
    hud(16, 3, "FLIP 3", PAL_INK);
    hud(16, 4, "LIP  1", PAL_INK);

    if (mode_ == Mode::Title) {
        spr(art_.title, 200.f, 78.f, float(art_.title.w), float(art_.title.h), PAL_TITLE);
        hudC(12, "A SHORT GOLF", PAL_GOLD);
        hudC(14, "MATCH THE DRAWER TO THE TAPE", PAL_INK);
        hudC(16, "LEFT RIGHT AIM    A STRIKES", PAL_GREEN);
        hudC(18, rules_ ? "A TO TEE OFF" : "TAPE UNREADABLE", rules_ ? PAL_GOLD : PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        if (won_) {
            hudC(8, "DRAWER MATCHES THE TAPE", PAL_GOLD);
            hudC(10, "SHORT GOLF CLOSED", PAL_GREEN);
        } else {
            hudC(8, "STILL OPEN", PAL_ALERT);
            hudC(10, "A CLOSE CARD IS NOT THE TAPE", PAL_INK);
        }
        hudC(12, bot_ ? "" : "A TO PLAY AGAIN", PAL_INK);
    } else {
        char top[40];
        std::snprintf(top, sizeof top, "%s  STROKE %d/3", clubName(), stroke_ + 1);
        hud(14, 6, top, PAL_GOLD);
        Read live = (mode_ == Mode::Aim) ? readAim() : read_;
        bool sweet = mode_ == Mode::Aim ? firmMeter(meter_) : tapeRead(read_);
        int namePal = PAL_INK;
        if (mode_ == Mode::Aim && tapeRead(live)) namePal = PAL_GREEN;
        else if (mode_ == Mode::Aim && live != Read::None) namePal = PAL_ALERT;
        else if (tapeRead(read_)) namePal = PAL_GREEN;
        hud(14, 8, readName(mode_ == Mode::Aim ? live : read_), namePal);
        if (mode_ == Mode::Aim) hud(22, 8, sweet ? "FIRM" : "WAIT", sweet ? PAL_GOLD : PAL_INK);

        char bar[34];
        for (int i = 0; i < 32; i++) {
            float m = (float(i) + 0.5f) / 32.f;
            bar[i] = (m >= kFirmLo && m <= kFirmHi) ? '=' : '-';
        }
        int p = std::max(0, std::min(31, int(meter_ * 31.f)));
        bar[p] = 'O';
        bar[32] = 0;
        hud(4, 20, bar, PAL_GOLD);

        char aim[34];
        for (int i = 0; i < 32; i++) aim[i] = '.';
        aim[32] = 0;
        int ap = std::max(0, std::min(31, int((aim_ + 1.f) * 0.5f * 31.f)));
        aim[ap] = 'V';
        hud(4, 18, aim, PAL_GREEN);
        hud(1, 18, "AIM", PAL_INK);
        hud(1, 20, "PWR", PAL_INK);
    }

    for (int i = 0; i < kTapeN; i++) {
        if (!held_[i]) continue;
        if (mode_ == Mode::Pocket && i == stroke_) continue;
        float x = 28.f + float(i) * 100.f;
        spr(art_.slip[i], x, 196.f, 78.f, 16.f, PAL_SLIP);
        hud(2 + i * 12, 24, tapeName(i), PAL_INK);
    }
    if (mode_ == Mode::Pocket && stroke_ >= 0 && stroke_ < kTapeN) {
        spr(art_.slip[stroke_], slipX_, slipY_, 78.f, 16.f, PAL_SLIP);
    }

    char till[32];
    std::snprintf(till, sizeof till, "TILL %d  BOARD %d", drawerScore(), board_);
    hud(22, 26, till, won_ ? PAL_GREEN : PAL_INK);
    if (traps_ > 0) {
        char tr[16];
        std::snprintf(tr, sizeof tr, "TRAP %d", traps_);
        hud(2, 26, tr, PAL_ALERT);
    } else {
        hud(2, 26, "DRAWER", PAL_GOLD);
    }
}

}  // namespace golftape
