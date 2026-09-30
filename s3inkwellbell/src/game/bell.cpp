#include "game/bell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace inkwellbell {
namespace {
constexpr float kDt = 1.f / 60.f;
constexpr float kLip = 168.f;
constexpr float kWellSpeed = 210.f;
}  // namespace

void Game::chime(float freq) {
    gs::FMPatch p;
    p.alg = 4;
    p.vol = 0.35f;
    p.op[0].mul = 1.f;
    p.op[0].level = 1.f;
    p.op[0].ar = 0.01f;
    p.op[0].dr = 0.4f;
    p.op[0].sl = 0.2f;
    p.op[0].rr = 0.6f;
    sys_->apu.setPatch(0, p);
    sys_->apu.keyOn(0, freq, 0.4f);
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip) {
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spawn() {
    rng_ = rng_ * 1664525u + 1013904223u;
    float u = (rng_ >> 8) * (1.f / 16777216.f);
    dropX_ = 56.f + u * 180.f;
    if (std::fabs(dropX_ - wellX_) < 40.f) dropX_ = dropX_ < 150.f ? dropX_ + 80.f : dropX_ - 80.f;
    dropX_ = std::clamp(dropX_, 48.f, 240.f);
    dropY_ = 18.f;
    dropVy_ = 46.f + marks_ * 7.f;
    drop_ = true;
}

void Game::begin() {
    marks_ = 0;
    tries_ = kTries;
    wellX_ = 160.f;
    over_ = false;
    won_ = false;
    left_ = false;
    anim_ = 0;
    swing_ = 0.f;
    mode_ = Mode::Play;
    spawn();
    chime(440.f);
}

void Game::catchInk() {
    drop_ = false;
    marks_++;
    chime(520.f + marks_ * 30.f);
    if (marks_ >= kMarks && tries_ > 0) {
        mode_ = Mode::Ring;
        anim_ = 0;
        swing_ = 0.f;
        chime(880.f);
        sys_->apu.noiseBurst(0.15f, 400.f, 0.2f);
    } else {
        spawn();
    }
}

void Game::miss() {
    drop_ = false;
    tries_--;
    anim_ = 0;
    sys_->apu.noiseBurst(0.4f, 700.f, 0.25f);
    if (tries_ <= 0) {
        mode_ = Mode::Over;
        over_ = true;
        won_ = false;
    } else {
        mode_ = Mode::Miss;
    }
}

void Game::leave() {
    won_ = true;
    left_ = true;
    over_ = true;
    mode_ = Mode::Over;
    sys_->quit();
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(2, 2, 3);
    uint16_t mid = gs::rgb4(6, 4, 3);
    uint16_t bot = gs::rgb4(3, 2, 1);
    if (mode_ == Mode::Ring || (mode_ == Mode::Over && won_)) mid = gs::rgb4(10, 8, 3);
    if (mode_ == Mode::Over && !won_) mid = gs::rgb4(6, 2, 2);
    if (mode_ == Mode::Miss) mid = gs::rgb4(7, 3, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.5f ? mix(top, mid, u / 0.5f) : mix(mid, bot, (u - 0.5f) / 0.5f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    spr(art_.page, 150.f, 108.f, float(art_.page.w), float(art_.page.h), PAL_PAGE);
    spr(art_.desk, 160.f, 198.f, float(art_.desk.w), float(art_.desk.h), PAL_DESK);

    float bellX = 286.f + std::sin(swing_) * (mode_ == Mode::Ring || won_ ? 6.f : 0.f);
    float bellH = (mode_ == Mode::Ring) ? 46.f : 40.f;
    spr(art_.bell, bellX, 36.f, 36.f, bellH, PAL_BELL);

    const float wellY = 176.f;
    spr(art_.well, wellX_, wellY, 58.f, 36.f, PAL_DESK);
    float qbob = (mode_ == Mode::Title) ? float((sys_->frame / 8) % 4) : 0.f;
    spr(art_.quill, wellX_ + 10.f, wellY - 34.f - qbob, 14.f, 52.f, PAL_QUILL);

    if (drop_ && (mode_ == Mode::Play || mode_ == Mode::Title || mode_ == Mode::Pause))
        spr(art_.drop, dropX_, dropY_, 12.f, 16.f, PAL_INK);

    for (int i = 0; i < marks_ && i < kMarks; i++) {
        float bx = 78.f + (i % 4) * 16.f;
        float by = 78.f + (i / 4) * 16.f;
        spr(art_.drop, bx, by, 8.f, 10.f, PAL_INK);
    }
    for (int i = 0; i < kTries; i++) {
        int pal = i < tries_ ? PAL_BRASS : PAL_DIM;
        spr(art_.nib, 24.f + i * 14.f, 28.f, 10.f, 16.f, pal);
    }

    char buf[80];
    if (mode_ == Mode::Title) {
        hudC(2, "S3 INKWELL BELL", PAL_BRASS);
        hudC(18, "CATCH THE INK", PAL_HUD);
        hudC(20, "EIGHT MARKS RING THE BELL", PAL_BRASS);
        hudC(21, "BEFORE THE THIRD TRY DIES", PAL_HUD);
        hudC(23, "ARROWS MOVE THE WELL", PAL_DIM);
        if ((sys_->frame & 16) == 0) hudC(25, "PRESS START", PAL_BRASS);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE BELL RANG", PAL_BRASS);
        std::snprintf(buf, sizeof buf, "MARKS %d  TRIES %d", marks_, tries_);
        hudC(20, buf, PAL_BRASS);
        hudC(22, "BEFORE THE THIRD TRY DIED", PAL_HUD);
        hudC(24, "LEAVE", PAL_BRASS);
    } else if (mode_ == Mode::Over) {
        hudC(2, "THE THIRD TRY DIED", PAL_BAD);
        std::snprintf(buf, sizeof buf, "MARKS %d", marks_);
        hudC(20, buf, PAL_BAD);
        hudC(22, "THE BELL STAYED QUIET", PAL_HUD);
        if ((sys_->frame & 16) == 0) hudC(25, "START", PAL_BRASS);
    } else if (mode_ == Mode::Ring) {
        hudC(2, "BELL", PAL_BRASS);
        std::snprintf(buf, sizeof buf, "MARKS %d  TRIES %d", marks_, tries_);
        hud(1, 26, buf, PAL_HUD);
        hudC(24, "LEAVE", PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        hudC(2, "PAUSED", PAL_BRASS);
        hudC(24, "START", PAL_DIM);
    } else if (mode_ == Mode::Miss) {
        hudC(2, "BLOT", PAL_BAD);
        std::snprintf(buf, sizeof buf, "TRY %d LEFT", tries_);
        hudC(22, buf, PAL_BAD);
    } else {
        std::snprintf(buf, sizeof buf, "MARKS %d/%d", marks_, kMarks);
        hud(1, 1, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "TRY %d", kTries - tries_ + 1);
        hud(28, 1, buf, tries_ == 1 ? PAL_BAD : PAL_BRASS);
        hudC(26, "MOVE", PAL_DIM);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.55f);
    mode_ = Mode::Title;
    drop_ = true;
    dropX_ = 200.f;
    dropY_ = 70.f;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        dropY_ += 18.f * kDt;
        if (dropY_ > 120.f) dropY_ = 40.f;
        wellX_ = 150.f + std::sin(sys.frame * 0.04f) * 50.f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = Mode::Play;
            mode_ = Mode::Pause;
        } else {
            float axis = std::fabs(pad.axisX) > 0.12f ? pad.axisX
                                                       : float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
            if (bot_) {
                float dx = dropX_ - wellX_;
                float cap = kWellSpeed * kDt;
                if (dx > cap) dx = cap;
                if (dx < -cap) dx = -cap;
                wellX_ += dx;
            } else {
                wellX_ += axis * kWellSpeed * kDt;
            }
            wellX_ = std::clamp(wellX_, 40.f, 248.f);
            if (drop_) {
                dropY_ += dropVy_ * kDt;
                if (dropY_ >= kLip - 6.f && dropY_ <= kLip + 14.f && std::fabs(dropX_ - wellX_) < 24.f) catchInk();
                else if (dropY_ > kLip + 22.f) miss();
            }
        }
    } else if (mode_ == Mode::Miss) {
        anim_++;
        if (anim_ > 28) {
            mode_ = Mode::Play;
            spawn();
        }
    } else if (mode_ == Mode::Ring) {
        anim_++;
        swing_ += 0.45f;
        if (anim_ == 12) chime(660.f);
        if (anim_ == 24) chime(880.f);
        if (anim_ > 48) leave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE)) mode_ = held_;
    } else if (mode_ == Mode::Over) {
        if (won_) swing_ += 0.2f;
        if (!bot_ && !won_ && pad.pressed(gs::BTN_START)) begin();
    }

    draw();
}

}  // namespace inkwellbell
