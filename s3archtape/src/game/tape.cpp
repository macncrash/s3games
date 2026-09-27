#include "game/tape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace archtape {
namespace {

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

bool Game::audit() {
    auto at = [](float r, int pay, int line, const char* name) {
        Hit h = hitAt(r);
        if (h.pay != pay || h.line != line || std::strcmp(h.name, name) != 0) {
            std::fprintf(stderr, "s3archtape ring %s at r %.2f read %s pay %d line %d\n", name, r, h.name, h.pay, h.line);
            return false;
        }
        return true;
    };
    if (!at(0.f, 10, 0, "PIN")) return false;
    if (!at(kBand[0].rOut, 10, 0, "PIN")) return false;
    if (!at(kBand[0].rOut + 0.01f, 10, -1, "DOT")) return false;
    if (!at(kBand[1].rOut, 10, -1, "DOT")) return false;
    if (!at(kBand[1].rOut + 0.01f, 9, 1, "GOLD")) return false;
    if (!at(kBand[2].rOut, 9, 1, "GOLD")) return false;
    if (!at(kBand[2].rOut + 0.01f, 9, -1, "PALE")) return false;
    if (!at(kBand[3].rOut, 9, -1, "PALE")) return false;
    if (!at(kBand[3].rOut + 0.01f, 7, 2, "RED")) return false;
    if (!at(kBand[4].rOut, 7, 2, "RED")) return false;
    if (!at(kBand[4].rOut + 0.01f, 7, -1, "RUST")) return false;
    if (!at(kBand[5].rOut, 7, -1, "RUST")) return false;
    if (!at(kBand[5].rOut + 0.01f, 0, -1, "MISS")) return false;
    if (tapeSum() != 26) return false;
    for (int i = 0; i < kTapeN; i++) {
        float x, y;
        bedPoint(i, x, y);
        float r = std::hypot(x - kCx, y - kCy);
        Hit h = hitAt(r);
        if (h.line != i) {
            std::fprintf(stderr, "s3archtape bed %d landed on %s\n", i, h.name);
            return false;
        }
    }
    std::snprintf(reason_, sizeof reason_, "rules hold");
    return true;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return tapeName(i);
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return tapePay(i);
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += tapePay(i);
    return s;
}

const char* Game::modeName() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Aim: return "aim";
    case Mode::Nock: return "nock";
    case Mode::Flight: return "flight";
    case Mode::Call: return "call";
    case Mode::Leave: return "leave";
    case Mode::Win: return "win";
    case Mode::Lose: return "lose";
    }
    return "?";
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Flight) return 2;
    if (mode_ == Mode::Call && matched()) return 3;
    if (mode_ == Mode::Leave || mode_ == Mode::Win) return 4;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) {
        std::snprintf(reason_, sizeof reason_, "rules failed");
        std::fprintf(stderr, "s3archtape rules failed\n");
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.7f);
    mode_ = Mode::Title;
    aimX_ = kCx;
    aimY_ = kCy;
}

void Game::begin() {
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    arrows_ = 0;
    traps_ = 0;
    marks_ = 0;
    won_ = false;
    over_ = false;
    left_ = false;
    call_[0] = 0;
    aimX_ = kCx;
    aimY_ = kCy;
    drawTick_ = 0;
    steady_ = 0;
    mode_ = Mode::Aim;
    skipHold_ = !bot_;
    std::snprintf(reason_, sizeof reason_, "open");
}

void Game::steer() {
    if (!sys_) return;
    const gs::Pad& p = sys_->pad;
    float mx = 0, my = 0;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    mx += p.axisX;
    my -= p.axisY;
    aimX_ = clampf(aimX_ + mx * 1.7f, 8.f, float(gs::SCREEN_W - 8));
    aimY_ = clampf(aimY_ + my * 1.7f, 16.f, 168.f);
}

bool Game::drawHeld() const {
    if (!sys_) return false;
    return sys_->pad.down(gs::BTN_Z) || sys_->pad.down(gs::BTN_A) || sys_->pad.accel > 0.4f;
}

void Game::loose() {
    float power = std::min(1.f, float(drawTick_) / float(kFullDraw));
    float wob = steady_ > 6 ? float(steady_ - 6) * 1.5f : 0.f;
    landX_ = aimX_ + wob;
    landY_ = aimY_ + (1.f - power) * 72.f;
    flightT_ = 0;
    mode_ = Mode::Flight;
    arrows_++;
    blip(0, 520.f + power * 180.f, 0.08f, 6);
}

void Game::arrive() {
    float r = std::hypot(landX_ - kCx, landY_ - kCy);
    last_ = hitAt(r);
    if (marks_ < 12) {
        markX_[marks_] = landX_;
        markY_[marks_] = landY_;
        marks_++;
    }
    if (last_.line >= 0 && !held_[last_.line]) {
        held_[last_.line] = true;
        std::snprintf(call_, sizeof call_, "%s IN THE DRAWER", last_.name);
        std::snprintf(reason_, sizeof reason_, "%s held", last_.name);
        blip(1, 660.f, 0.1f, 8);
    } else if (last_.line >= 0) {
        std::snprintf(call_, sizeof call_, "%s ALREADY IN", last_.name);
        std::snprintf(reason_, sizeof reason_, "repeat");
    } else if (last_.face && last_.pay > 0) {
        traps_++;
        std::snprintf(call_, sizeof call_, "%s STAYS OUT", last_.name);
        std::snprintf(reason_, sizeof reason_, "twin");
        blip(1, 180.f, 0.08f, 8);
    } else {
        std::snprintf(call_, sizeof call_, "MISS");
        std::snprintf(reason_, sizeof reason_, "miss");
        blip(1, 90.f, 0.06f, 6);
    }
    callT_ = 0;
    mode_ = Mode::Call;
    if (sys_) sys_->apu.noiseBurst(last_.line >= 0 ? 0.08f : 0.16f, last_.line >= 0 ? 2200.f : 700.f, 0.06f);
}

void Game::afterCall() {
    if (matched()) {
        leaveT_ = 0;
        left_ = false;
        mode_ = Mode::Leave;
        std::snprintf(reason_, sizeof reason_, "drawer matches");
        blip(0, 523.f, 0.1f, 10);
        return;
    }
    if (arrows_ >= kQuiver) {
        won_ = false;
        over_ = true;
        left_ = false;
        mode_ = Mode::Lose;
        std::snprintf(reason_, sizeof reason_, "quiver empty");
        return;
    }
    drawTick_ = 0;
    steady_ = 0;
    skipHold_ = !bot_;
    mode_ = Mode::Aim;
    if (bot_) {
        int line = 0;
        for (int i = 0; i < kTapeN; i++)
            if (!held_[i]) {
                line = i;
                break;
            }
        bedPoint(line, aimX_, aimY_);
    }
}

void Game::blip(int ch, float hz, float vol, int frames) {
    if (!sys_) return;
    sys_->apu.tone(ch, hz, vol);
    blipCh_ = ch;
    blipLeft_ = frames;
}

void Game::pump() {
    if (blipLeft_ > 0 && --blipLeft_ == 0 && sys_) sys_->apu.tone(blipCh_, 0, 0);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_) return;
    gs::Plane& h = sys_->vdp.HUD;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) c = ' ';
        int tile = art_.font[c - 32];
        h.set(col + i, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    int col = std::max(0, (40 - n) / 2);
    hud(col, row, s, pal);
}

void Game::stamp(const gs::Image& img, float x, float y, float w, float h, int pal) {
    if (!sys_ || img.w == 0) return;
    gs::Sprite s;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(std::max(1.f, w));
    s.h = int16_t(std::max(1.f, h));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t sky = y < 150 ? gs::rgb4(6, 9, 13) : gs::rgb4(3, 7, 3);
        if (y < 36) sky = gs::rgb4(8, 11, 14);
        if (y > 176) sky = gs::rgb4(4, 3, 2);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    bool aiming = mode_ == Mode::Aim || mode_ == Mode::Nock;
    int pose = 0;
    if (mode_ == Mode::Nock) pose = 1;
    else if (mode_ == Mode::Flight) pose = 2;

    if (mode_ == Mode::Title) {
        stamp(art_.wordArch, 18.f, 28.f, float(art_.wordArch.w), float(art_.wordArch.h), PAL_WORD);
        stamp(art_.wordTape, 18.f, 70.f, float(art_.wordTape.w), float(art_.wordTape.h), PAL_WORD);
    }
    if (mode_ == Mode::Leave || mode_ == Mode::Win) {
        float x = (gs::SCREEN_W - art_.wordLeave.w) * 0.5f;
        stamp(art_.wordLeave, x, 8.f, float(art_.wordLeave.w), float(art_.wordLeave.h), PAL_WORD);
    }
    if (aiming) stamp(art_.sight, aimX_ - 5.f, aimY_ - 5.f, 11.f, 11.f, PAL_SIGHT);
    if (mode_ == Mode::Flight) {
        float u = std::min(1.f, float(flightT_) / float(kFlight));
        float x = kLooseX + (landX_ - kLooseX) * u;
        float y = kLooseY + (landY_ - kLooseY) * u - std::sin(u * 3.14159265f) * 12.f;
        stamp(art_.arrow, x, y - 3.f, float(art_.arrow.w), float(art_.arrow.h), PAL_ARROW);
    }
    for (int i = marks_ - 1; i >= 0; i--) stamp(art_.pip, markX_[i] - 2.f, markY_[i] - 2.f, 5.f, 5.f, PAL_MARK);
    stamp(art_.archer[pose], 6.f, 36.f, float(art_.archer[pose].w), float(art_.archer[pose].h), PAL_ARCH);
    stamp(art_.face, kCx - kFaceMid, kCy - kFaceMid, float(kFace), float(kFace), PAL_FACE);
    stamp(art_.drawer, 76.f, 186.f, float(art_.drawer.w), float(art_.drawer.h), PAL_WOOD);

    if (aiming) {
        float power = mode_ == Mode::Nock ? std::min(1.f, float(drawTick_) / float(kFullDraw)) : 0.f;
        stamp(art_.px, 8.f, 172.f, 70.f, 5.f, PAL_FILL);
        if (power > 0.02f) stamp(art_.px, 8.f, 172.f, 70.f * power, 5.f, PAL_HUD_GOLD);
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 ARCHTAPE", PAL_HUD_GOLD);
        hudC(23, "THE DRAWER HAS TO MATCH THE TAPE", PAL_HUD);
        hudC(24, "PIN  GOLD  RED", PAL_HUD_GOLD);
        hudC(25, "DOT PALE RUST STAY OUT", PAL_HUD_ALERT);
        hudC(26, "Z DRAWS   LET GO LOOSES", PAL_HUD);
        hudC(27, "START", PAL_HUD);
        return;
    }

    hud(1, 0, "TAPE", PAL_HUD_GOLD);
    std::snprintf(line, sizeof line, "%s %s %s", held_[0] ? "PIN" : "...", held_[1] ? "GOLD" : "....",
                  held_[2] ? "RED" : "...");
    hud(8, 0, line, PAL_HUD);
    std::snprintf(line, sizeof line, "TILL %d", drawerScore());
    hud(28, 0, line, PAL_HUD_GOLD);

    if (mode_ == Mode::Win || mode_ == Mode::Leave) {
        hudC(24, "THE DRAWER MATCHES THE TAPE", PAL_HUD_GOLD);
        std::snprintf(line, sizeof line, "%s  %s  %s", tapeName(0), tapeName(1), tapeName(2));
        hudC(25, line, PAL_HUD);
        hudC(26, mode_ == Mode::Win ? "YOU LEAVE" : "LEAVING", PAL_HUD_GOLD);
        if (mode_ == Mode::Win) hudC(27, "START SHOOTS AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Lose) {
        hudC(24, "THE TAPE IS STILL OPEN", PAL_HUD_ALERT);
        hudC(25, reason_, PAL_HUD);
        hudC(27, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Call) {
        hudC(24, call_, last_.line >= 0 ? PAL_HUD_GOLD : PAL_HUD_ALERT);
    } else if (mode_ == Mode::Nock) {
        hudC(24, "LOOSE ON THE TAPE", PAL_HUD_GOLD);
    } else {
        hudC(24, "HOLD Z TO DRAW", PAL_HUD);
    }
    std::snprintf(line, sizeof line, "ARROWS %d", arrows_);
    hudC(26, line, PAL_HUD);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_++;
    if (!rules_) {
        draw();
        return;
    }

    if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
        if (mode_ == Mode::Title) {
            if (sys.hasHome()) sys.eject();
        } else {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            left_ = false;
        }
    }

    if (mode_ == Mode::Title) {
        bool go = bot_ ? (t_ >= 8) : (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C));
        if (go) begin();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Aim) {
        if (bot_) {
            int line = 0;
            for (int i = 0; i < kTapeN; i++)
                if (!held_[i]) {
                    line = i;
                    break;
                }
            bedPoint(line, aimX_, aimY_);
            mode_ = Mode::Nock;
            drawTick_ = 0;
            steady_ = 0;
        } else {
            steer();
            if (skipHold_) {
                if (!drawHeld()) skipHold_ = false;
            } else if (drawHeld()) {
                mode_ = Mode::Nock;
                drawTick_ = 0;
                steady_ = 0;
            }
        }
    } else if (mode_ == Mode::Nock) {
        if (!bot_) steer();
        bool release = bot_ ? (drawTick_ >= kFullDraw) : !drawHeld();
        if (release && (bot_ || drawTick_ > 0)) loose();
        else if (drawTick_ < kFullDraw) drawTick_++;
        else steady_++;
    } else if (mode_ == Mode::Flight) {
        if (++flightT_ > kFlight) arrive();
    } else if (mode_ == Mode::Call) {
        if (++callT_ > kCall) afterCall();
    } else if (mode_ == Mode::Leave) {
        if (++leaveT_ > kLeave) {
            left_ = true;
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            std::snprintf(reason_, sizeof reason_, "you leave");
        }
    }

    pump();
    draw();
}

}  // namespace archtape
