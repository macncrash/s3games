#include "game/archchime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace archchime {
namespace {

constexpr int kFullDraw = 18;
constexpr int kFirm = 8;
constexpr int kFlight = 16;
constexpr int kStartSec = kHourSec - 36;
constexpr float kDrop = 78.f;
constexpr float kPi = 3.14159265f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

bool Game::audit() const {
    auto at = [](float x, float y, Bed bed, const char* name) {
        Mark m = classify(x, y);
        if (m.bed != bed || std::strcmp(m.name, name) != 0) {
            std::fprintf(stderr, "s3archchime %s scored %s at %.2f %.2f\n", name, m.name, x, y);
            return false;
        }
        return true;
    };
    if (!at(kCx, kCy, Bed::Gold, "GOLD")) return false;
    if (!at(kCx, kCy - (kGold + kRed) * 0.5f, Bed::Red, "RED")) return false;
    if (!at(kCx, kCy - (kRed + kBlue) * 0.5f, Bed::Blue, "BLUE")) return false;
    if (!at(kCx, kCy - (kBlue + kBlack) * 0.5f, Bed::Black, "BLACK")) return false;
    if (!at(kCx, kCy - (kBlack + kWhite) * 0.5f, Bed::White, "WHITE")) return false;
    if (!at(kCx, kCy - (kWhite + kStraw) * 0.5f, Bed::Straw, "STRAW")) return false;
    if (classify(kCx, kCy - (kStraw + 6.f)).bed != Bed::Miss) {
        std::fprintf(stderr, "s3archchime grass scored\n");
        return false;
    }
    Mark lip = classify(kCx, kCy - (kGold + 1.6f));
    if (lip.bed == Bed::Gold) {
        std::fprintf(stderr, "s3archchime red still gold\n");
        return false;
    }
    return true;
}

int Game::wind() const {
    static const int kW[kArrows] = {kWind0, -2, 4};
    int i = arrows_;
    if (i < 0) i = 0;
    if (i >= kArrows) i = kArrows - 1;
    return kW[i];
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

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3archchime rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(8, 10, 12));
    sys.apu.setMaster(0.7f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    reason_ = "";
    bed_[0] = 0;
    arrows_ = 0;
    playFrames_ = 0;
    drawTick_ = 0;
    steady_ = 0;
    flightT_ = 0;
    hold_ = 0;
    strikes_ = 0;
    firm_ = false;
    aimX_ = kCx;
    aimY_ = kCy;
    archX_ = kArchX;
    walk_ = 0;
}

void Game::beginAim() {
    drawTick_ = 0;
    steady_ = 0;
    firm_ = false;
    skipHold_ = !bot_;
    flightT_ = 0;
    if (bot_) {
        aimX_ = kCx - float(wind()) * kWindPx;
        aimY_ = kCy;
    }
    mode_ = Mode::Aim;
}

void Game::beginFail(const char* why) {
    if (!won_) reason_ = why;
    won_ = false;
    hold_ = 0;
    mode_ = Mode::Fail;
    if (sys_) sys_->apu.tone(0, 90.f, 0.06f);
}

void Game::beginEarly() {
    reason_ = "EARLY";
    hold_ = 0;
    mode_ = Mode::Early;
    if (sys_) sys_->apu.tone(0, 180.f, 0.05f);
}

void Game::beginChime() {
    won_ = true;
    reason_ = "CHIME";
    std::snprintf(bed_, sizeof bed_, "GOLD");
    hold_ = 0;
    strikes_ = 0;
    mode_ = Mode::Chime;
    if (!sys_) return;
    sys_->apu.tone(0, 523.f, 0.1f);
    sys_->apu.tone(1, 784.f, 0.06f);
    sys_->rumble(0.2f, 0.4f, 80);
}

void Game::enterNock() {
    mode_ = Mode::Nock;
    drawTick_ = 0;
    steady_ = 0;
    if (sys_) sys_->apu.tone(1, 140.f, 0.03f);
}

bool Game::drawHeld() const {
    if (!sys_) return false;
    if (sys_->pad.down(gs::BTN_A) || sys_->pad.down(gs::BTN_C)) return true;
    return sys_->pad.accel > 0.45f;
}

void Game::steer() {
    if (!sys_) return;
    float x = sys_->pad.axisX;
    float y = -sys_->pad.axisY;
    if (sys_->pad.down(gs::BTN_LEFT)) x = -1.f;
    if (sys_->pad.down(gs::BTN_RIGHT)) x = 1.f;
    if (sys_->pad.down(gs::BTN_UP)) y = -1.f;
    if (sys_->pad.down(gs::BTN_DOWN)) y = 1.f;
    float mag = std::sqrt(x * x + y * y);
    if (mag > 1.f) {
        x /= mag;
        y /= mag;
    }
    float sp = sys_->pad.down(gs::BTN_TURBO) ? 4.2f : 2.4f;
    aimX_ = clampf(aimX_ + x * sp, kCx - 90.f, kCx + 70.f);
    aimY_ = clampf(aimY_ + y * sp, kCy - 70.f, kCy + 70.f);
    if (sys_->pad.pressed(gs::BTN_B)) {
        aimX_ = kCx - float(wind()) * kWindPx;
        aimY_ = kCy;
    }
}

void Game::loose(bool firm) {
    firm_ = firm;
    float power = firm ? 1.f : std::min(1.f, float(drawTick_) / float(kFullDraw));
    float ox = 0.f;
    float oy = 0.f;
    if (!firm && steady_ > kFirm) {
        float mag = std::min(26.f, float(steady_ - kFirm) * 1.4f);
        oy = std::sin(float(steady_) * 0.7f) * mag;
        ox = oy * 0.35f;
    }
    if (!firm && power < 1.f) oy += (1.f - power) * kDrop;
    landX_ = aimX_ + float(wind()) * kWindPx + ox;
    landY_ = aimY_ + oy;
    arrows_++;
    flightT_ = 0;
    mode_ = Mode::Flight;
    if (!sys_) return;
    sys_->apu.tone(0, 240.f, 0.05f);
    sys_->rumble(0.1f, 0.2f, 24);
}

void Game::arrive() {
    Mark m = classify(landX_, landY_);
    std::snprintf(bed_, sizeof bed_, "%s", m.name);
    if (m.bed == Bed::Gold && firm_) {
        if (onHour()) beginChime();
        else if (pastHour()) beginFail("HOUR");
        else if (arrows_ >= kArrows) beginFail("EARLY");
        else beginEarly();
        return;
    }
    if (arrows_ >= kArrows || pastHour()) beginFail(pastHour() ? "HOUR" : m.name);
    else {
        reason_ = m.name;
        hold_ = 0;
        mode_ = Mode::Early;
        if (sys_) sys_->apu.tone(0, 110.f, 0.04f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool clockOn = mode_ == Mode::Aim || mode_ == Mode::Nock || mode_ == Mode::Flight || mode_ == Mode::Early;
    if (clockOn) playFrames_++;

    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_) go = hold_++ > 16;
        if (go) {
            if (!rules_) beginFail("RULES");
            else {
                arrows_ = 0;
                playFrames_ = 0;
                beginAim();
            }
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Aim) {
        if (pastHour()) beginFail("HOUR");
        else if (bot_) {
            aimX_ = kCx - float(wind()) * kWindPx;
            aimY_ = kCy;
            if (framesUntilHour() == kFlight) loose(true);
        } else {
            steer();
            if (skipHold_) {
                if (!drawHeld()) skipHold_ = false;
            } else if (drawHeld()) {
                enterNock();
            }
        }
    } else if (mode_ == Mode::Nock) {
        if (pastHour() && !drawHeld()) {
            loose(false);
        } else if (!drawHeld()) {
            bool firm = drawTick_ >= kFullDraw && steady_ <= kFirm;
            loose(firm);
        } else if (drawTick_ < kFullDraw) {
            drawTick_++;
            if (drawTick_ == kFullDraw && sys_) sys_->apu.tone(0, 880.f, 0.04f);
        } else {
            steady_++;
        }
    } else if (mode_ == Mode::Flight) {
        if (++flightT_ >= kFlight) arrive();
    } else if (mode_ == Mode::Early) {
        if (pastHour()) beginFail("HOUR");
        else if (++hold_ > 28) beginAim();
    } else if (mode_ == Mode::Chime) {
        if (hold_ % 6 == 0 && strikes_ < 12) {
            float hz = (strikes_ % 2) ? 659.f : 523.f;
            if (sys_) sys_->apu.tone(0, hz, 0.08f);
            strikes_++;
        }
        if (++hold_ > 12 * 6 + 10) {
            mode_ = Mode::Leave;
            hold_ = 0;
            walk_ = 0;
        }
    } else if (mode_ == Mode::Leave) {
        walk_ += 3.4f;
        archX_ = kArchX + walk_;
        if (++hold_ > 48) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless) sys.quit();
        }
    } else if (mode_ == Mode::Fail) {
        if (++hold_ > 70) {
            over_ = true;
            mode_ = Mode::Over;
            if (!sys.headless && !bot_) sys.quit();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) toTitle();
    }
    draw();
}

void Game::stamp(const gs::Image& img, float x, float y, float w, float h, int pal) {
    if (!sys_ || img.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::at(const gs::Image& img, float cx, float cy, int pal) {
    stamp(img, cx - float(img.w) * 0.5f, cy - float(img.h) * 0.5f, float(img.w), float(img.h), pal);
}

void Game::hand(float cx, float cy, float ang, float len, int pal) {
    int n = std::max(2, int(len / 3.f));
    for (int i = 1; i <= n; i++) {
        float t = len * (float(i) / float(n));
        stamp(art_.pip, cx + std::sin(ang) * t - 1.f, cy - std::cos(ang) * t - 1.f, 3.f, 3.f, pal);
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
    int n = s ? int(std::strlen(s)) : 0;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c = gs::rgb4(6, 8, 12);
        if (y > 36) c = gs::rgb4(8, 11, 14);
        if (y > 96) c = gs::rgb4(10, 12, 13);
        if (y > 150) c = gs::rgb4(4, 9, 4);
        if (y > 190) c = gs::rgb4(3, 7, 3);
        v.lineBackdrop[y] = c;
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    bool chiming = mode_ == Mode::Chime || mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    if (mode_ == Mode::Title) {
        stamp(art_.word, 118.f, 36.f, float(art_.word.w), float(art_.word.h), PAL_WORD);
        hudC(10, "UNTIL THE HOUR CHIMES", PAL_INK);
        hudC(12, "THEN LEAVE", PAL_GOLD);
        hudC(20, "HOLD A    LOOSE ON GOLD AT XII", PAL_INK);
        hudC(22, "THREE ARROWS", PAL_GOLD);
    }

    if (mode_ == Mode::Aim || mode_ == Mode::Nock) at(art_.sight, aimX_, aimY_, PAL_SIGHT);

    if (mode_ == Mode::Flight) {
        float u = std::min(1.f, float(flightT_) / float(kFlight));
        float x = kLooseX + (landX_ - kLooseX) * u;
        float y = kLooseY + (landY_ - kLooseY) * u;
        y -= std::sin(u * kPi) * 12.f;
        stamp(art_.arrow, x - float(art_.arrow.w), y - 3.f, float(art_.arrow.w), float(art_.arrow.h), PAL_ARROW);
    } else if (mode_ == Mode::Nock) {
        float pull = std::min(1.f, float(drawTick_) / float(kFullDraw)) * 10.f;
        stamp(art_.arrow, kLooseX - float(art_.arrow.w) - pull, kLooseY - 3.f, float(art_.arrow.w), float(art_.arrow.h),
              PAL_ARROW);
    }

    int h = 11, m = 58, s = 0;
    if (mode_ != Mode::Title) face(h, m, s);
    float minuteAng = (float(m) / 60.f) * 2.f * kPi;
    float hourAng = ((float(h % 12) + float(m) / 60.f) / 12.f) * 2.f * kPi;
    hand(36.f, 40.f, minuteAng, 9.f, PAL_CLOCK);
    hand(36.f, 40.f, hourAng, 6.f, PAL_GOLD);

    stamp(art_.archer, archX_, kArchY, float(art_.archer.w), float(art_.archer.h), PAL_ARCH);
    stamp(art_.face, kCx - kFaceMid, kCy - kFaceMid, float(kFace), float(kFace), PAL_FACE);
    stamp(art_.clock, 18.f, 22.f, float(art_.clock.w), float(art_.clock.h), PAL_CLOCK);
    stamp(art_.belfry, 48.f, 8.f, float(art_.belfry.w), float(art_.belfry.h), PAL_WORLD);
    stamp(art_.stand, kCx - 10.f, kCy + kFaceMid - 16.f, float(art_.stand.w), float(art_.stand.h), PAL_WORLD);
    stamp(art_.tree, 250.f, 40.f, float(art_.tree.w), float(art_.tree.h), PAL_WORLD);
    stamp(art_.tree, 286.f, 52.f, float(art_.tree.w) * 0.8f, float(art_.tree.h) * 0.8f, PAL_WORLD);

    if (mode_ != Mode::Title) {
        int shown = arrows_ + ((mode_ == Mode::Aim || mode_ == Mode::Nock) ? 1 : 0);
        if (shown < 1) shown = 1;
        if (shown > kArrows) shown = kArrows;
        char line[48];
        std::snprintf(line, sizeof line, "ARROW %d/%d", shown, kArrows);
        hud(1, 1, line, PAL_INK);
        int hh, mm, ss;
        face(hh, mm, ss);
        std::snprintf(line, sizeof line, "%d:%02d:%02d", hh, mm, ss);
        hud(30, 1, line, chiming || onHour() ? PAL_GREEN : PAL_GOLD);
        int w = wind();
        if (w == 0) std::snprintf(line, sizeof line, "WIND CALM");
        else if (w > 0) std::snprintf(line, sizeof line, "WIND +%d", w);
        else std::snprintf(line, sizeof line, "WIND %d", w);
        hud(1, 3, line, PAL_INK);
        if (bed_[0] && mode_ != Mode::Aim && mode_ != Mode::Nock) hud(30, 3, bed_, PAL_GOLD);
        int until = framesUntilHour();
        if (chiming) hudC(25, mode_ == Mode::Leave || (mode_ == Mode::Over && won_) ? "LEAVE" : "THE HOUR CHIMES", PAL_GREEN);
        else if (onHour()) hudC(25, "THE HOUR", PAL_GREEN);
        else if (until > 0) {
            std::snprintf(line, sizeof line, "HOUR IN %d", (until + kFpc - 1) / kFpc);
            hudC(25, line, PAL_INK);
        }
    }
    if (mode_ == Mode::Nock && steady_ > kFirm) hudC(23, "HOT", PAL_ALERT);
    else if (mode_ == Mode::Nock && drawTick_ >= kFullDraw) hudC(23, "LOOSE", PAL_GREEN);
    else if (mode_ == Mode::Early) hudC(23, reason_[0] ? reason_ : "EARLY", PAL_ALERT);
    if (mode_ == Mode::Fail || (mode_ == Mode::Over && !won_)) hudC(25, "THE HOUR IS GONE", PAL_ALERT);
}

}  // namespace archchime
