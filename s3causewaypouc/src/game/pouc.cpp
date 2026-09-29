#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cwpouc {
namespace {

constexpr float kFloor = 168.f;
constexpr float kGrav = 1100.f;
constexpr float kJump = -460.f;
constexpr float kRun = 210.f;
constexpr float kGoal = 1580.f;
constexpr float kDt = 1.f / 60.f;
struct Deck {
    float a, b;
};
constexpr Deck kSpans[] = {{0.f, 400.f}, {520.f, 780.f}, {920.f, 1160.f}, {1300.f, 1760.f}};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0.f;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    held_ = false;
    onGround_ = true;
    face_ = 1;
    reason_ = "THE POUCH DID NOT CROSS";
    px_ = 48.f;
    py_ = kFloor;
    vx_ = 0.f;
    vy_ = 0.f;
    pouchX_ = 220.f;
    cam_ = 0.f;
    playT_ = 0.f;
}

void Game::finish(bool crossed, const char* why) {
    won_ = crossed;
    over_ = true;
    reason_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    vx_ = 0.f;
    vy_ = 0.f;
}

bool Game::solidAt(float x) const {
    for (const Deck& s : kSpans) {
        if (x >= s.a && x <= s.b) return true;
    }
    return false;
}

bool Game::jumpEdge(bool right) const {
    const Deck* here = nullptr;
    for (const Deck& s : kSpans) {
        if (px_ >= s.a && px_ <= s.b) here = &s;
    }
    if (!here || !right) return false;
    bool more = false;
    for (const Deck& n : kSpans)
        if (n.a > here->b + 1.f) more = true;
    if (!more) return false;
    float ahead = here->b - px_;
    return ahead < 18.f && ahead > 0.f;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 3;
    if (mode_ == Mode::Lost) return 4;
    if (held_) return 2;
    return 1;
}

void Game::botIntent(bool& left, bool& right, bool& jump, bool& go) {
    go = true;
    left = right = jump = false;
    if (mode_ != Mode::Play) return;
    if (!held_) {
        if (px_ < pouchX_ - 8.f) right = true;
        else if (px_ > pouchX_ + 8.f) left = true;
    } else {
        right = true;
    }
    if (onGround_) jump = jumpEdge(right);
}

void Game::stepPlay(bool left, bool right, bool jump) {
    playT_ += kDt;
    if (left && !right) {
        vx_ = -kRun;
        face_ = -1;
    } else if (right && !left) {
        vx_ = kRun;
        face_ = 1;
    } else {
        vx_ = 0.f;
    }
    if (jump && onGround_) {
        vy_ = kJump;
        onGround_ = false;
        sys_->apu.tone(1, 320.f, 0.06f);
    }
    px_ += vx_ * kDt;
    px_ = std::clamp(px_, 16.f, 1700.f);
    if (!onGround_) {
        vy_ += kGrav * kDt;
        py_ += vy_ * kDt;
        if (vy_ > 0.f && py_ >= kFloor && solidAt(px_)) {
            py_ = kFloor;
            vy_ = 0.f;
            onGround_ = true;
        } else if (py_ > kFloor + 48.f) {
            finish(false, "OFF THE CAUSEWAY");
            return;
        }
    } else if (!solidAt(px_)) {
        onGround_ = false;
        vy_ = 0.f;
    }
    if (!held_ && onGround_ && std::abs(px_ - pouchX_) < 22.f) {
        held_ = true;
        sys_->apu.tone(2, 520.f, 0.07f);
    }
    if (!held_) pouchX_ = 220.f;
    if (onGround_ && px_ >= kGoal) {
        if (held_) finish(true, "THE POUCH CROSSED");
        else finish(false, "WITHOUT THE POUCH");
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    bool left = false, right = false, jump = false, go = false;
    if (bot_) botIntent(left, right, jump, go);
    else {
        const gs::Pad& p = sys.pad;
        left = p.down(gs::BTN_LEFT);
        right = p.down(gs::BTN_RIGHT);
        jump = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_UP);
        go = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C);
    }
    if (mode_ == Mode::Title && go) begin();
    else if (mode_ == Mode::Play) stepPlay(left, right, jump);
    if (mode_ == Mode::Play && onGround_ && std::abs(vx_) > 1.f) {
        sys.apu.tone(0, (int(playT_ * 8.f) & 1) ? 90.f : 70.f, 0.03f);
    } else if (mode_ != Mode::Play) {
        sys.apu.tone(0, 0.f, 0.f);
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    float adv = 14.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, std::max(8.f, float(g.h) * scale), pal, false, false);
    }
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 96) {
            float k = float(y) / 96.f;
            v.lineBackdrop[y] = gs::rgb4(3 + int(k * 4), 5 + int(k * 3), 10 + int((1.f - k) * 4));
        } else {
            float k = float(y - 96) / float(gs::SCREEN_H - 96);
            v.lineBackdrop[y] = gs::rgb4(1, 3 + int(k * 3), 6 + int(k * 4));
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    backdrop();
    cam_ = std::clamp(px_ - 110.f, 0.f, 1440.f);

    for (int i = 0; i < 3; i++) {
        float gx = std::fmod(40.f + i * 140.f + t_ * 18.f, 420.f) - 40.f;
        float gy = 28.f + i * 16.f + std::sin(t_ * 1.4f + i) * 4.f;
        spr(art_.gull, gx, gy, 10.f, PAL_GULL, (i & 1) != 0, false);
    }

    auto worldX = [&](float x) { return x - cam_; };
    for (const Deck& s : kSpans) {
        spr(art_.post, worldX(s.a + 6.f), kFloor, 40.f, PAL_POST, false, true);
        spr(art_.post, worldX(s.b - 6.f), kFloor, 40.f, PAL_POST, false, true);
        for (float x = s.a + 16.f; x < s.b; x += 30.f) {
            spr(art_.plank, worldX(x), kFloor + 2.f, 16.f, PAL_PLANK, false, true);
        }
    }
    spr(art_.gate, worldX(kGoal), kFloor, 56.f, PAL_GATE, false, true);

    if (!held_) spr(art_.pouch, worldX(pouchX_), kFloor - 2.f, 18.f, PAL_POUCH, false, true);

    const gs::Mipped& body = !onGround_ ? art_.leap : (std::abs(vx_) > 1.f && (int(playT_ * 8.f) & 1) ? art_.runA : art_.stand);
    float bob = onGround_ && std::abs(vx_) > 1.f ? std::sin(playT_ * 16.f) * 1.5f : 0.f;
    spr(body, worldX(px_), kFloor + bob, 46.f, PAL_WALKER, face_ < 0, true);
    if (held_) spr(art_.pouch, worldX(px_ + face_ * 12.f), kFloor - 28.f + bob, 16.f, PAL_POUCH, face_ < 0, false);

    if (mode_ == Mode::Title) {
        text("CAUSEWAY", 160.f, 36.f, 1.3f, PAL_TITLE);
        text("CARRY THE POUCH", 160.f, 62.f, 0.7f, PAL_HUD);
        text("PRESS START", 160.f, 150.f, 0.65f, int(t_ * 2.f) & 1 ? PAL_TITLE : PAL_HUD);
        text("ARROWS RUN   A JUMP", 160.f, 176.f, 0.48f, PAL_HUD);
    } else if (mode_ == Mode::Play) {
        text(held_ ? "POUCH IN HAND" : "FETCH THE POUCH", 160.f, 10.f, 0.5f, held_ ? PAL_GOOD : PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160.f, 48.f, 0.7f, PAL_GOOD);
    } else {
        text("LOSS", 160.f, 44.f, 1.1f, PAL_ALERT);
        text(reason_, 160.f, 74.f, 0.5f, PAL_HUD);
    }
}

}  // namespace cwpouc
