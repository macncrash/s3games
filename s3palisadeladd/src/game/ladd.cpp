#include "game/ladd.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace palisadeladd {
namespace {

constexpr float kDt = 1.0f / 60.0f;
constexpr float kRun = 140.0f;
constexpr float kJump = -340.0f;
constexpr float kGrav = 920.0f;
constexpr float kWatch = 48.0f;
constexpr float kLossY = 214.0f;
constexpr float kLadderX = 1220.0f;
constexpr float kLadderTop = 52.0f;
constexpr float kFeet = 168.0f;

struct Plat {
    float x, w, y;
};

constexpr Plat kPlats[] = {
    {0, 320, kFeet},
    {378, 300, kFeet},
    {736, 240, kFeet},
    {1034, 280, kFeet},
};
constexpr int kPlatN = 4;
constexpr int kLogPlat = 1;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paint();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    if (bot_) begin();
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (px_ > 1000.0f) return 3;
    if (px_ > 390.0f) return 2;
    return 1;
}

float Game::logX() const {
    const float span = 130.0f;
    const float speed = 72.0f;
    float period = 2.0f * span / speed;
    float u = std::fmod(std::max(0.0f, t_), period) / period;
    float tri = u < 0.5f ? u * 2.0f : 2.0f - u * 2.0f;
    return 500.0f + tri * span;
}

int Game::platAt(float x, float y) const {
    int found = -1;
    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        if (x <= p.x + 2.0f || x >= p.x + p.w - 2.0f) continue;
        if (std::abs(y - p.y) < 3.0f) found = i;
    }
    return found;
}

void Game::win() {
    if (won_) return;
    won_ = true;
    over_ = true;
    mode_ = Mode::Won;
    reason_ = "THE FAR LADDER";
    vx_ = 0;
    vy_ = 0;
    onLadder_ = true;
    py_ = kLadderTop;
    px_ = kLadderX;
    fan_ = 0;
    shake_ = 0.25f;
    sys_->rumble(0.25f, 0.7f, 200);
    sys_->setLight(40, 170, 80);
    sys_->apu.tone(0, 523.0f, 0.08f);
    sys_->apu.tone(1, 659.0f, 0.06f);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    reason_ = why;
    vx_ = 0;
    vy_ = 0;
    onLadder_ = false;
    fan_ = 0;
    shake_ = 0.8f;
    sys_->apu.noiseBurst(0.4f, 380.0f, 0.24f);
    sys_->rumble(0.8f, 0.25f, 160);
    sys_->setLight(170, 30, 20);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "";
    grounded_ = true;
    onLadder_ = false;
    onPlat_ = 0;
    face_ = 1;
    for (int i = 0; i < 4; i++) gapJumped_[i] = false;
    logHop_ = false;
    px_ = 72.0f;
    py_ = kFeet;
    vx_ = vy_ = 0;
    coyote_ = 0.12f;
    foot_ = 0;
    shake_ = 0;
    fan_ = -1;
    t_ = 0;
    camX_ = 0;
    sys_->apu.tone(0, 392.0f, 0.05f);
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 32);
    v.B.resize(64, 32);
    v.A.clear();
    v.B.clear();
    v.A.enabled = true;
    v.B.enabled = true;
    v.HUD.enabled = true;
    auto put = [&](gs::Plane& p, int tx, int ty, int tile, int pal) {
        if (tx < 0 || ty < 0 || tx >= p.w || ty >= p.h || tile <= 0) return;
        p.set(tx, ty, gs::entry(tile, pal));
    };
    for (int ty = 0; ty < 8; ty++) {
        for (int tx = 0; tx < v.B.w; tx++) {
            uint32_t h = uint32_t(tx) * 2246822519u ^ uint32_t(ty) * 3266489917u;
            if ((h % 19u) == 0) put(v.B, tx, ty, art_.star, PAL_SKY);
        }
    }
    for (int tx = 0; tx < v.B.w; tx++) {
        int top = 12 + int(tx % 3);
        put(v.B, tx, top, art_.ridgeTip, PAL_FAR);
        for (int ty = top + 1; ty < 20; ty++) put(v.B, tx, ty, art_.ridge, PAL_FAR);
    }
    for (int i = 0; i < kPlatN; i++) {
        const Plat& p = kPlats[i];
        int x0 = int(p.x) / 8;
        int n = int(p.w) / 8;
        int row = int(p.y) / 8;
        for (int k = 0; k < n; k++) {
            int tx = x0 + k;
            bool end = k == 0 || k == n - 1;
            put(v.A, tx, row, end ? art_.plankEnd : art_.plank, PAL_STAKE);
            if ((k % 2) == 0) {
                put(v.A, tx, row - 2, art_.tip, PAL_STAKE);
                put(v.A, tx, row - 1, art_.stake, PAL_STAKE);
            }
            int depth = 4 + (k % 3);
            for (int d = 1; d <= depth; d++) put(v.A, tx, row + d, art_.stake, PAL_STAKE);
        }
    }
}

void Game::bot(bool& left, bool& right, bool& up, bool& jump) {
    left = right = up = jump = false;
    if (mode_ != Mode::Play) return;
    const float reach = 16.0f;
    if (onLadder_ || (px_ > kLadderX - reach && px_ < kLadderX + reach && py_ <= kFeet + 2.0f && py_ > kLadderTop + 4.0f)) {
        up = true;
        if (px_ < kLadderX - 3.0f) right = true;
        else if (px_ > kLadderX + 3.0f) left = true;
        return;
    }
    if (px_ < kLadderX - 4.0f) right = true;
    else left = px_ > kLadderX + 4.0f;
    if (!grounded_) return;
    int plat = onPlat_;
    if (plat >= 0 && plat < kPlatN - 1) {
        const Plat& p = kPlats[plat];
        if (px_ >= p.x + p.w - 12.0f && !gapJumped_[plat]) {
            jump = true;
            gapJumped_[plat] = true;
        }
    }
    if (plat == kLogPlat) {
        float lx = logX();
        bool near = lx > px_ - 8.0f && lx < px_ + 42.0f;
        if (near && !logHop_) {
            jump = true;
            logHop_ = true;
        }
        if (!near) logHop_ = false;
    }
}

void Game::play(float dt, bool left, bool right, bool up, bool jump) {
    if (mode_ != Mode::Play) return;
    t_ += dt;
    if (t_ > kWatch) {
        lose("THE WATCH IS OVER");
        return;
    }

    const bool grab = up && std::abs(px_ - kLadderX) < 18.0f && py_ <= kFeet + 4.0f && py_ > kLadderTop + 2.0f;
    if (!onLadder_ && grab) {
        onLadder_ = true;
        grounded_ = false;
        vx_ = 0;
        vy_ = 0;
        px_ = kLadderX;
    }

    if (onLadder_) {
        px_ = kLadderX;
        vx_ = 0;
        if (up) py_ -= 78.0f * dt;
        if (!up && left == false && right) {
            /* stay until the top; the bot only climbs */
        }
        if (py_ <= kLadderTop) {
            win();
            return;
        }
        if (py_ > kFeet) py_ = kFeet;
        face_ = 1;
        return;
    }

    float accel = grounded_ ? 980.0f : 640.0f;
    if (left) {
        vx_ = std::max(-kRun, vx_ - accel * dt);
        face_ = -1;
    } else if (right) {
        vx_ = std::min(kRun, vx_ + accel * dt);
        face_ = 1;
    } else {
        vx_ *= grounded_ ? 0.55f : 0.992f;
        if (std::abs(vx_) < 4.0f) vx_ = 0;
    }

    if (jump && coyote_ > 0) {
        vy_ = kJump;
        grounded_ = false;
        coyote_ = 0;
        onPlat_ = -1;
        sys_->apu.tone(0, 620.0f, 0.04f);
    }

    float pyPrev = py_;
    vy_ += kGrav * dt;
    px_ += vx_ * dt;
    py_ += vy_ * dt;

    grounded_ = false;
    onPlat_ = -1;
    if (vy_ >= 0) {
        for (int i = 0; i < kPlatN; i++) {
            const Plat& p = kPlats[i];
            if (px_ <= p.x + 4.0f || px_ >= p.x + p.w - 4.0f) continue;
            if (pyPrev <= p.y + 0.5f && py_ >= p.y) {
                py_ = p.y;
                vy_ = 0;
                grounded_ = true;
                onPlat_ = i;
                break;
            }
        }
    }
    if (grounded_) coyote_ = 0.10f;
    else coyote_ = std::max(0.0f, coyote_ - dt);

    if (onPlat_ == kLogPlat && py_ > kFeet - 18.0f) {
        if (std::abs(px_ - logX()) < 24.0f) {
            lose("THE LOG");
            return;
        }
    }
    if (py_ > kLossY || px_ < -8.0f || px_ > 1360.0f) {
        lose(px_ > 1180.0f ? "MISSED THE LADDER" : "OFF THE PALISADE");
        return;
    }

    if (grounded_ && std::abs(vx_) > 20.0f) {
        foot_ -= dt;
        if (foot_ <= 0) {
            foot_ = 0.22f;
            sys_->apu.tone(2, 140.0f, 0.03f);
        }
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(clampf(cx - s.w * 0.5f, -2000.0f, 2000.0f)));
    s.y = int16_t(std::lround(clampf(feet ? cy - s.h : cy - s.h * 0.5f, -2000.0f, 2000.0f)));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::world(const gs::Mipped& m, float wx, float wy, float h, int pal, bool flip, bool feet) {
    spr(m, wx - camX_, wy, h, pal, flip, feet);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    float width = 0;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float pulse = 0.55f + 0.45f * std::sin((t_ + float(sys_->frame) * kDt) * 9.0f);
    v.setColor(PAL_FIRE * 16 + 4, gs::rgb4(15, 11 + int(3 * pulse), 4));

    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 90) c = gs::rgb4(1, 1, 3 + y / 45);
        else if (y < 150) c = gs::rgb4(2, 2, 5);
        else c = gs::rgb4(2 + (y - 150) / 30, 2, 3);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    float view = camX_;
    if (shake_ > 0) view += std::sin(t_ * 70.0f) * shake_ * 3.0f;
    v.A.scroll(int(std::lround(-view)), 0);
    v.B.scroll(int(std::lround(-view * 0.22f)), 0);

    float showT = mode_ == Mode::Title ? float(sys_->frame) * kDt : t_;
    world(art_.banner, 120, kFeet - 36, 18, PAL_ALERT, false, false);
    for (int i = 0; i < 4; i++) {
        float tx = 48.0f + float(i) * 300.0f;
        float fh = 14.0f + 3.0f * std::sin(showT * 11.0f + float(i));
        world(art_.flame, tx, kFeet - 18, fh, PAL_FIRE, false, true);
    }
    world(art_.ladder, kLadderX, kFeet, kFeet - kLadderTop, PAL_LADDER, false, true);
    world(art_.bell, kLadderX, kLadderTop - 6, 16, PAL_BELL, false, true);
    if (mode_ != Mode::Title) world(art_.log, logX(), kFeet + 2, 26, PAL_LOG, false, true);

    const gs::Mipped* body = &art_.stand;
    if (onLadder_) body = (int(t_ * 6.0f) & 1) ? &art_.climbA : &art_.climbB;
    else if (!grounded_) body = &art_.jump;
    else if (std::abs(vx_) > 12.0f) body = (int(t_ * 8.0f) & 1) ? &art_.walkA : &art_.walkB;
    world(*body, px_, py_, 40, PAL_HERO, face_ < 0, true);

    if (mode_ == Mode::Title) {
        text("S3 PALISADE LADD", 160, 36, 1.05f, PAL_GOLD);
        text("REACH THE FAR LADDER", 160, 62, 0.72f, PAL_HUD);
        text("MISS IT AND THE WATCH IS OVER", 160, 82, 0.55f, PAL_ALERT);
        hudC(24, "ENTER TO TAKE THE WATCH", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "WATCH HELD", PAL_GOLD);
        hudC(14, "ENTER TO RETURN", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudC(3, "FAR LADDER", PAL_OK);
        hudC(5, "THE WATCH HOLDS", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(3, "WATCH OVER", PAL_ALERT);
        hudC(5, reason_, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(kWatch - t_)));
        char buf[32];
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 1, buf, left < 10 ? PAL_ALERT : PAL_HUD);
        hud(28, 1, "FAR LADDER", PAL_GOLD);
    }
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - kDt);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bool left = false, right = false, up = false, down = false, jump = false;
    const gs::Pad& pad = sys.pad;
    if (!bot_) {
        left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
        right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
        up = pad.down(gs::BTN_UP) || pad.axisY > 0.35f;
        down = pad.down(gs::BTN_DOWN) || pad.axisY < -0.35f;
        jump = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    }
    (void)down;

    if (mode_ == Mode::Title) {
        camX_ = 0;
        px_ = 96;
        py_ = kFeet;
        grounded_ = true;
        onLadder_ = false;
        face_ = 1;
        vx_ = 0;
        if (bot_ || pad.pressed(gs::BTN_START) || jump) begin();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        draw();
        return;
    }
    if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || jump)) {
            mode_ = Mode::Title;
            over_ = false;
        }
        if (fan_ >= 0 && fan_ < 6) {
            float notes[] = {523, 659, 784, 1046};
            if (mode_ == Mode::Won && (sys.frame % 8) == 0) {
                sys.apu.tone(1, notes[fan_ % 4], 0.05f);
                fan_++;
            }
        }
        draw();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    if (bot_) bot(left, right, up, jump);
    float target = clampf(px_ - 110.0f, 0.0f, 1314.0f + 40.0f - gs::SCREEN_W);
    camX_ += (target - camX_) * 0.12f;
    play(kDt, left, right, up, jump);
    draw();
}

}  // namespace palisadeladd
