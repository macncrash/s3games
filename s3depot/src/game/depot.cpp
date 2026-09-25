#include "game/depot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace depot {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kMax = 2.05f;
constexpr float kReach = 16.f;
constexpr float kMast = 15.f;
constexpr float kXLo = 18.f;
constexpr float kXHi = 302.f;
constexpr float kYLo = 76.f;
constexpr float kYHi = 208.f;
constexpr int kShift = 90 * 60;
constexpr int kDoorRow = 6;
constexpr int kBayRow = 10;

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

const char* kindName(int k) {
    static const char* n[] = {"CRATE", "DRUM", "PALLET"};
    if (k < 0 || k > 2) return "";
    return n[k];
}

}  // namespace

int Game::shiftSeconds() const {
    int left = clock_ < 0 ? 0 : clock_;
    return (left + 59) / 60;
}

const gs::Mipped& Game::freight(int kind) const {
    if (kind == 1) return art_.drum;
    if (kind == 2) return art_.pallet;
    return art_.crate;
}

int Game::freightPal(int kind) const {
    if (kind == 1) return PAL_DRUM;
    if (kind == 2) return PAL_PALLET;
    return PAL_CRATE;
}

float Game::freightH(int kind) const {
    if (kind == 1) return 22.f;
    if (kind == 2) return 18.f;
    return 20.f;
}

void Game::tune() {
    gs::FMPatch p;
    p.alg = 7;
    p.vol = 0.2f;
    p.fb = 0.05f;
    p.glide = 0.02f;
    for (int i = 0; i < 4; i++) {
        p.op[i].mul = i == 0 ? 1.f : (i == 1 ? 3.f : 1.f);
        p.op[i].level = i == 0 ? 1.f : (i == 1 ? 0.22f : 0.f);
        p.op[i].ar = 0.003f;
        p.op[i].dr = 0.2f;
        p.op[i].sl = 0.15f;
        p.op[i].rr = 0.16f;
    }
    for (int ch = 0; ch < 3; ch++) sys_->apu.setPatch(ch, p);
    sys_->apu.setEcho(0.16f, 0.28f, 0.12f);
}

void Game::layWorld() {
    static const float sx[kGoal] = {54.f, 92.f, 158.f, 220.f, 268.f, 132.f};
    static const float sy[kGoal] = {146.f, 184.f, 134.f, 196.f, 156.f, 200.f};
    static const int sk[kGoal] = {0, 1, 2, 0, 1, 2};
    static const int bc[kGoal] = {2, 8, 14, 20, 26, 32};
    static const int bk[kGoal] = {0, 1, 2, 0, 1, 2};
    for (int i = 0; i < kGoal; i++) {
        load_[i].x = sx[i];
        load_[i].y = sy[i];
        load_[i].kind = sk[i];
        load_[i].live = true;
        bay_[i].col = bc[i];
        bay_[i].x = float((bc[i] + 1) * 8);
        bay_[i].y = 88.f;
        bay_[i].kind = bk[i];
        bay_[i].full = false;
    }
    cleared_ = 0;
    carry_ = -1;
    focus_ = -1;
    wrong_ = -1;
    pop_ = 0;
}

void Game::park() {
    x_ = 186.f;
    y_ = 188.f;
    heading_ = -kPi * 0.5f;
    speed_ = 0;
    anchorX_ = x_;
    anchorY_ = y_;
    stall_ = 0;
}

void Game::paintBay(int i) {
    const Bay& b = bay_[i];
    int top = b.full ? art_.shutTop : art_.doorTop;
    int bot = b.full ? art_.shutBot : art_.doorBot;
    int slab = b.full ? art_.bayFull : art_.bayOpen;
    for (int c = 0; c < 2; c++) {
        sys_->vdp.A.set(b.col + c, kDoorRow, gs::entry(top, PAL_BRICK));
        sys_->vdp.A.set(b.col + c, kDoorRow + 1, gs::entry(bot, PAL_BRICK));
        sys_->vdp.A.set(b.col + c, kBayRow, gs::entry(slab, PAL_DOCK));
        sys_->vdp.A.set(b.col + c, kBayRow + 1, gs::entry(slab, PAL_DOCK));
    }
}

void Game::paintYard() {
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.B.clear();
    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 40; c++) v.B.set(c, r, gs::entry(art_.sky[(c + r * 3) & 3], PAL_NIGHT));
    }
    for (int r = 4; r < 28; r++) {
        for (int c = 0; c < 40; c++) {
            bool edge = c == 0 || c == 39;
            int tile = art_.conc[(r * 3 + c) % 3];
            int pal = PAL_YARD;
            if (edge) {
                tile = art_.post;
                pal = PAL_BRICK;
            } else if (r == 4) {
                tile = art_.roof;
                pal = PAL_BRICK;
            } else if (r == 5) {
                tile = ((c * 5) % 7 == 0) ? art_.window : art_.brick;
                pal = PAL_BRICK;
            } else if (r == 6 || r == 7) {
                tile = art_.brick;
                pal = PAL_BRICK;
            } else if (r == 8) {
                tile = art_.hazard;
                pal = PAL_YARD;
            } else if (r == 9) {
                tile = art_.apron;
                pal = PAL_YARD;
            } else if (r == 12) {
                tile = art_.stripe;
                pal = PAL_YARD;
            } else if (r == 27) {
                tile = art_.rail;
                pal = PAL_RAIL;
            } else if (((r * 13 + c * 7) % 29) == 0) {
                tile = art_.stain;
            }
            v.A.set(c, r, gs::entry(tile, pal));
        }
    }
    for (int i = 0; i < kGoal; i++) paintBay(i);
}

void Game::begin() {
    layWorld();
    park();
    clock_ = kShift;
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    why_ = "shift";
    song_ = -1;
    songKind_ = 0;
    horn_ = false;
    for (auto& p : puff_) p.t = 0;
    paintYard();
    retarget();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    tune();
    layWorld();
    park();
    clock_ = kShift;
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "shift";
    paintYard();
    sys.setLight(80, 90, 120);
    if (bot_) begin();
}

bool Game::focusOk() const {
    if (focus_ < 0 || focus_ >= kGoal) return false;
    if (carry_ >= 0) {
        if (bay_[focus_].full) return false;
        if (bay_[focus_].kind != load_[carry_].kind) return false;
        return true;
    }
    return load_[focus_].live;
}

void Game::retarget() {
    focus_ = -1;
    stall_ = 0;
    anchorX_ = x_;
    anchorY_ = y_;
    float best = 1e9f;
    if (carry_ >= 0) {
        int kind = load_[carry_].kind;
        for (int i = 0; i < kGoal; i++) {
            if (bay_[i].full || bay_[i].kind != kind) continue;
            float d = std::hypot(bay_[i].x - x_, bay_[i].y - y_);
            if (d < best) {
                best = d;
                focus_ = i;
            }
        }
        return;
    }
    for (int i = 0; i < kGoal; i++) {
        if (!load_[i].live) continue;
        float d = std::hypot(load_[i].x - x_, load_[i].y - y_);
        if (d < best) {
            best = d;
            focus_ = i;
        }
    }
}

float Game::near(float px, float py) const {
    float d = std::hypot(px - x_, py - y_);
    float fx = x_ + std::cos(heading_) * kMast;
    float fy = y_ + std::sin(heading_) * kMast;
    return std::min(d, std::hypot(px - fx, py - fy));
}

void Game::chime(int kind) {
    if (songKind_ == 2 && kind != 2) return;
    songKind_ = kind;
    song_ = 0;
}

void Game::burst(float px, float py) {
    int n = 0;
    for (auto& p : puff_) {
        if (p.t > 0) continue;
        float a = float(n) * 1.05f;
        p.x = px;
        p.y = py;
        p.vx = std::cos(a) * 0.85f;
        p.vy = std::sin(a) * 0.85f;
        p.t = 16.f;
        if (++n == 6) break;
    }
}

void Game::hitch(int i) {
    carry_ = i;
    focus_ = -1;
    wrong_ = -1;
    sys_->apu.noiseBurst(0.07f, 1600.f, 0.05f);
    sys_->apu.keyOn(0, 196.f, 0.12f);
    retarget();
}

void Game::deliver(int i) {
    int held = carry_;
    bay_[i].full = true;
    if (held >= 0) load_[held].live = false;
    carry_ = -1;
    focus_ = -1;
    wrong_ = -1;
    cleared_++;
    paintBay(i);
    burst(bay_[i].x, bay_[i].y);
    pop_ = 42;
    popX_ = bay_[i].x;
    popY_ = bay_[i].y + 16.f;
    sys_->rumble(0.25f, 0.45f, 90);
    if (cleared_ >= kGoal) win();
    else chime(1);
}

void Game::hook() {
    if (carry_ >= 0) {
        int hit = -1;
        float best = kReach;
        int wrong = -1;
        for (int i = 0; i < kGoal; i++) {
            if (bay_[i].full) continue;
            float d = near(bay_[i].x, bay_[i].y);
            if (d > kReach) continue;
            if (bay_[i].kind == load_[carry_].kind) {
                if (d < best) {
                    best = d;
                    hit = i;
                }
            } else if (wrong < 0) {
                wrong = i;
            }
        }
        if (hit >= 0) deliver(hit);
        else if (wrong >= 0 && wrong != wrong_) {
            wrong_ = wrong;
            if (!bot_) {
                sys_->apu.noiseBurst(0.06f, 180.f, 0.1f);
                sys_->apu.keyOn(0, 110.f, 0.1f);
            }
        } else if (wrong < 0) {
            wrong_ = -1;
        }
        return;
    }
    int hit = -1;
    float best = kReach;
    for (int i = 0; i < kGoal; i++) {
        if (!load_[i].live) continue;
        float d = near(load_[i].x, load_[i].y);
        if (d < best) {
            best = d;
            hit = i;
        }
    }
    if (hit >= 0) hitch(hit);
}

void Game::botDrive(float& steer, float& gas) {
    if (!focusOk()) retarget();
    if (focus_ < 0) return;
    float tx = carry_ >= 0 ? bay_[focus_].x : load_[focus_].x;
    float ty = carry_ >= 0 ? bay_[focus_].y : load_[focus_].y;
    float dx = tx - x_;
    float dy = ty - y_;
    float dist = std::hypot(dx, dy);
    if (dist < 1.f) {
        gas = 0.1f;
        return;
    }
    float want = std::atan2(dy, dx);
    float err = wrap(want - heading_);
    float ae = std::fabs(err);
    steer = std::clamp(err / 0.5f, -1.f, 1.f);
    gas = 1.f;
    if (ae > 1.f) gas = 0.1f;
    else if (ae > 0.5f) gas = 0.32f;
    else if (ae > 0.25f) gas = 0.62f;
    if (dist < 34.f) gas = std::min(gas, 0.42f);
    if (dist < 18.f) gas = std::min(gas, 0.22f);
    if (dist > 18.f && ae < 0.35f) gas = std::max(gas, 0.55f);

    float moved = std::hypot(x_ - anchorX_, y_ - anchorY_);
    if (moved > 2.2f) {
        anchorX_ = x_;
        anchorY_ = y_;
        stall_ = 0;
    } else if (++stall_ > 50) {
        heading_ = want;
        speed_ = 1.25f;
        stall_ = 0;
        anchorX_ = x_;
        anchorY_ = y_;
    }
}

void Game::drive(float& steer, float& gas) {
    steer = 0;
    gas = 0;
    if (bot_) {
        botDrive(steer, gas);
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.15f) steer = p.axisX;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO)) gas = 1.f;
    if (p.accel > 0.12f) gas = std::max(gas, p.accel);
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) gas = -0.85f;
    if (p.brake > 0.12f) gas = -p.brake;
    if (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_Y)) horn_ = true;
    steer = std::clamp(steer, -1.f, 1.f);
}

void Game::move(float steer, float gas) {
    float want = std::clamp(gas, -1.f, 1.f);
    want = want >= 0.f ? want * kMax : want * 1.25f;
    float step = std::fabs(want) + 0.05f < std::fabs(speed_) ? 0.2f : 0.1f;
    if (speed_ < want) speed_ = std::min(want, speed_ + step);
    else speed_ = std::max(want, speed_ - step);
    float rate = 0.055f + 0.08f * (1.f - std::min(1.f, std::fabs(speed_) / kMax));
    float sign = speed_ < -0.08f ? -1.f : 1.f;
    heading_ = wrap(heading_ + steer * sign * rate);
    if (y_ < 114.f && speed_ > 0.95f) speed_ = 0.95f;
    x_ += std::cos(heading_) * speed_;
    y_ += std::sin(heading_) * speed_;
    if (x_ < kXLo) x_ = kXLo;
    if (x_ > kXHi) x_ = kXHi;
    if (y_ < kYLo) y_ = kYLo;
    if (y_ > kYHi) y_ = kYHi;
}

void Game::puff() {
    if (std::fabs(speed_) < 0.4f) return;
    if ((sys_->frame & 3) != 0) return;
    for (auto& p : puff_) {
        if (p.t > 0) continue;
        p.x = x_ - std::cos(heading_) * 10.f;
        p.y = y_ - std::sin(heading_) * 10.f;
        p.vx = -std::cos(heading_) * 0.35f;
        p.vy = -std::sin(heading_) * 0.35f;
        p.t = 14.f;
        break;
    }
}

void Game::win() {
    if (mode_ != Mode::Play || cleared_ < kGoal) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    speed_ = 0;
    why_ = "clear";
    chime(2);
    sys_->rumble(0.4f, 0.7f, 160);
    sys_->setLight(40, 180, 60);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Lose;
    won_ = false;
    over_ = true;
    speed_ = 0;
    why_ = "whistle";
    chime(3);
    sys_->apu.noiseBurst(0.14f, 520.f, 0.45f);
    sys_->setLight(180, 30, 20);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (!sys_ || h < 1.f || m.h <= 0) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = float(y) / 223.f;
        int r = int(1.f + u * 7.f);
        int g = int(2.f + u * 3.f);
        int b = int(7.f - u * 4.f);
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    if (mode_ == Mode::Title) {
        spr(art_.logo, 160, 50, float(art_.logo.h), PAL_TITLE);
        spr(art_.tag, 160, 74, float(art_.tag.h), PAL_TITLE);
        spr(art_.hint, 160, 92, float(art_.hint.h), PAL_TITLE);
        spr(art_.hint2, 160, 106, float(art_.hint2.h), PAL_TITLE);
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        const gs::Mipped& word = mode_ == Mode::Win ? art_.win : art_.lose;
        spr(word, 160, 118, float(word.h), PAL_TITLE);
        spr(art_.plaque, 160, 118, float(art_.plaque.h), PAL_HUD);
    }
    if (pop_ > 0) {
        spr(art_.stowed, popX_, popY_ - float(42 - pop_) * 0.35f, float(art_.stowed.h), PAL_TITLE);
        if (mode_ == Mode::Play || mode_ == Mode::Win) pop_--;
    }

    if ((sys_->frame / 8) % 2 == 0) {
        float bx = x_ - std::cos(heading_) * 6.f;
        float by = y_ - std::sin(heading_) * 6.f;
        spr(art_.beacon, bx, by, 5, PAL_TUG);
    }
    if (carry_ >= 0) {
        float fx = x_ + std::cos(heading_) * kMast;
        float fy = y_ + std::sin(heading_) * kMast;
        int k = load_[carry_].kind;
        float jig = std::sin(float(sys_->frame) * 0.7f) * std::min(1.f, std::fabs(speed_));
        spr(freight(k), fx, fy + jig, freightH(k), freightPal(k));
    }
    float turns = heading_ / kTau;
    turns -= std::floor(turns);
    int fr = int(turns * 16.f + 0.5f) & 15;
    float bob = std::sin(float(sys_->frame) * 0.45f) * std::min(1.f, std::fabs(speed_)) * 0.5f;
    spr(art_.tug[fr], x_, y_ + bob, 36, PAL_TUG);
    spr(art_.shadow, x_ + 3.f, y_ + 5.f, 12, PAL_TUG, true);

    for (auto& p : puff_) {
        if (p.t <= 0) continue;
        p.x += p.vx;
        p.y += p.vy;
        p.t -= 1.f;
        spr(art_.dust, p.x, p.y, 4.f + p.t * 0.2f, PAL_YARD);
    }

    for (int i = 0; i < kGoal; i++) {
        if (!load_[i].live || i == carry_) continue;
        float bob = std::sin(float(sys_->frame) * 0.05f + float(i)) * 0.6f;
        spr(freight(load_[i].kind), load_[i].x, load_[i].y + bob, freightH(load_[i].kind), freightPal(load_[i].kind));
    }
    for (int i = 0; i < kGoal; i++) {
        if (!load_[i].live || i == carry_) continue;
        spr(art_.shadow, load_[i].x + 2.f, load_[i].y + 7.f, 8, PAL_TUG, true);
    }
    for (int i = 0; i < kGoal; i++) {
        if (bay_[i].full) continue;
        spr(freight(bay_[i].kind), bay_[i].x, 54.f, 13.f, freightPal(bay_[i].kind));
    }

    spr(art_.lamp, 14, 62, 30, PAL_LAMP);
    spr(art_.lamp, 306, 62, 30, PAL_LAMP);
    spr(art_.lamp, 160, 30, 26, PAL_LAMP);
    static const float bollardX[] = {48.f, 96.f, 144.f, 192.f, 240.f, 288.f};
    for (float bx : bollardX) spr(art_.bollard, bx, 216.f, 14, PAL_LAMP);

    float c1 = std::fmod(20.f + float(sys_->frame) * 0.12f, 400.f) - 40.f;
    float c2 = std::fmod(180.f + float(sys_->frame) * 0.07f, 420.f) - 50.f;
    spr(art_.cloud, c1, 16.f, 12, PAL_NIGHT);
    spr(art_.cloud, c2, 22.f, 10, PAL_NIGHT);
    spr(art_.moon, 292, 18, 16, PAL_NIGHT);

    for (int x = 0; x < 40; x++) v.HUD.set(x, 0, gs::entry(art_.bar, PAL_HUD));
    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 DEPOT", PAL_HUD);
        const char* blink = ((sys_->frame / 30) & 1) ? "ENTER" : "CLEAR";
        hud(40 - int(std::strlen(blink)), 0, blink, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_HUD);
    } else {
        char left[20];
        std::snprintf(left, sizeof left, "STOW %d/%d", cleared_, kGoal);
        hud(0, 0, left, PAL_HUD);
        if (carry_ >= 0) hud(11, 0, kindName(load_[carry_].kind), PAL_HUD);
        else if (mode_ == Mode::Play) hud(11, 0, "EMPTY", PAL_HUD);
        int sec = shiftSeconds();
        char clock[16];
        std::snprintf(clock, sizeof clock, "SHIFT %d:%02d", sec / 60, sec % 60);
        int pal = (mode_ == Mode::Play && sec <= 10 && ((sys_->frame / 8) & 1)) ? PAL_ALERT : PAL_HUD;
        hud(40 - int(std::strlen(clock)), 0, clock, pal);
        if (mode_ == Mode::Win) hudC(16, "ENTER FOR THE NEXT SHIFT", PAL_HUD);
        if (mode_ == Mode::Lose) hudC(15, "THE YARD STANDS", PAL_HUD);
        if (mode_ == Mode::Lose) hudC(17, "ENTER TO TRY AGAIN", PAL_HUD);
    }

    if (mode_ == Mode::Win) sys_->setLight(40, 180, 60);
    else if (mode_ == Mode::Lose) sys_->setLight(180, 30, 20);
    else if (carry_ >= 0) sys_->setLight(240, 170, 40);
    else if (mode_ == Mode::Play) sys_->setLight(90, 110, 150);
}

void Game::audio() {
    float rpm = std::min(1.f, std::fabs(speed_) / kMax);
    bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    sys_->apu.tone(0, 42.f + rpm * 36.f, live ? 0.012f + rpm * 0.028f : 0.f);
    sys_->apu.tone(1, 84.f + rpm * 24.f, live ? 0.008f + rpm * 0.014f : 0.f);
    sys_->apu.noise(live ? 0.012f + rpm * 0.034f : 0.f, 340.f + rpm * 980.f, false);
    if (horn_) {
        horn_ = false;
        sys_->apu.noiseBurst(0.1f, 900.f, 0.08f);
        sys_->apu.keyOn(1, 180.f, 0.1f);
    }
    if (mode_ == Mode::Play && clock_ > 0 && clock_ <= 10 * 60 && (clock_ % 60) == 0) sys_->apu.keyOn(1, 988.f, 0.09f);

    if (song_ >= 0) {
        struct N {
            int t;
            float f;
        };
        const N* ns = nullptr;
        int count = 0;
        int end = 0;
        static const N dockN[] = {{0, 523.f}, {8, 784.f}};
        static const N winN[] = {{0, 392.f}, {8, 523.f}, {16, 659.f}, {24, 784.f}, {36, 1046.f}};
        static const N loseN[] = {{0, 440.f}, {12, 349.f}, {24, 262.f}, {40, 196.f}};
        if (songKind_ == 2) {
            ns = winN;
            count = 5;
            end = 64;
        } else if (songKind_ == 3) {
            ns = loseN;
            count = 4;
            end = 64;
        } else {
            ns = dockN;
            count = 2;
            end = 22;
        }
        float f = 0;
        for (int i = 0; i < count; i++)
            if (song_ >= ns[i].t) f = ns[i].f;
        sys_->apu.tone(2, f, f > 0.f ? 0.045f : 0.f);
        for (int i = 0; i < count; i++)
            if (song_ == ns[i].t) sys_->apu.keyOn(0, ns[i].f, 0.16f);
        if (++song_ > end) {
            song_ = -1;
            if (songKind_ != 2) songKind_ = 0;
            sys_->apu.tone(2, 0, 0);
        }
    } else {
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    bool began = false;
    if (mode_ == Mode::Title) {
        if (start || bot_) {
            begin();
            began = true;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && start) {
            begin();
            began = true;
        }
    }

    if (mode_ == Mode::Play) {
        if (!bot_ && !began && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else {
            float steer = 0, gas = 0;
            drive(steer, gas);
            move(steer, gas);
            hook();
            if (mode_ == Mode::Play) {
                if (cleared_ >= kGoal) win();
                else {
                    clock_--;
                    if (clock_ <= 0) lose();
                }
            }
            if (mode_ == Mode::Play) puff();
        }
    }
    draw();
    audio();
}

}  // namespace depot
