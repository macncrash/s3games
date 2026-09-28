#include "game/lanternmark.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace lanternmark {
namespace {

constexpr int kLead = 16;
constexpr int kShow = 18;
constexpr int kGap = 10;
constexpr int kHot = 10;
constexpr int kLock = 8;
constexpr int kMiss = 36;
constexpr int kWin = 48;
constexpr int kBotAt = 18;
constexpr int kLamps = 6;
constexpr float kLampH = 48.f;
constexpr float kBeamY = 46.f;
constexpr float kX[kLamps] = {36, 88, 140, 192, 244, 292};
constexpr float kDrop[kLamps] = {2, 8, 12, 14, 8, 2};
constexpr float kPitch[kLamps] = {349.23f, 392.f, 440.f, 523.25f, 493.88f, 587.33f};
constexpr int kPal[kLamps] = {PAL_L0, PAL_L1, PAL_L2, PAL_GOLD, PAL_L3, PAL_L1};

uint16_t mix(uint16_t a, uint16_t b, float t) {
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto L = [&](int x, int y) { return int(std::lround(x + (y - x) * t)); };
    return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    finished_ = false;
    lifted_ = false;
    onMark_ = false;
    handed_ = 0;
    closer_ = -1;
    cursor_ = kMark;
    mothX_ = 48;
    mothY_ = 68;
}

void Game::quiet() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    toneT_ = 0;
}

void Game::chime(int lamp, float vol) {
    if (lamp < 0 || lamp >= kLamps) return;
    float f = kPitch[lamp];
    sys_->apu.tone(0, f, vol);
    sys_->apu.tone(1, f * 2.f, vol * 0.32f);
    toneT_ = 12;
    if (lamp == kMark) sys_->setLight(255, 190, 40);
    else sys_->setLight(180, 120, 60);
}

void Game::thud() {
    sys_->apu.tone(0, 90.f, 0.08f);
    sys_->apu.noiseBurst(0.35f, 600.f, 0.2f);
    toneT_ = 14;
    sys_->setLight(90, 0, 0);
    sys_->rumble(0.4f, 0.15f, 80);
}

void Game::begin() {
    handed_ = 0;
    won_ = false;
    over_ = false;
    finished_ = false;
    lifted_ = false;
    onMark_ = false;
    closer_ = -1;
    step_ = 0;
    cursor_ = 0;
    hot_ = -1;
    hotT_ = 0;
    hotBad_ = false;
    pending_ = -1;
    quiet();
    enterWatch();
}

void Game::enterWatch() {
    mode_ = Mode::Watch;
    phase_ = Phase::Lead;
    show_ = 0;
    timer_ = kLead;
    pending_ = -1;
    lock_ = 0;
}

void Game::enterPlay() {
    mode_ = Mode::Play;
    step_ = 0;
    lock_ = 6;
    pending_ = -1;
    cursor_ = order_[0];
}

void Game::enterMiss() {
    mode_ = Mode::Miss;
    timer_ = kMiss;
    handed_++;
    hotBad_ = true;
    hotT_ = 16;
    pending_ = -1;
    if (step_ > 0) step_--;
    thud();
}

void Game::enterOpen() {
    mode_ = Mode::Open;
    openT_ = 0;
    onMark_ = true;
    closer_ = kMark;
    cursor_ = kMark;
    pending_ = -1;
    chime(kMark, 0.1f);
    sys_->rumble(0.1f, 0.35f, 80);
}

void Game::enterWin() {
    mode_ = Mode::Win;
    timer_ = kWin;
    finished_ = true;
    lifted_ = true;
    won_ = true;
    sys_->apu.tone(0, 523.25f, 0.09f);
    sys_->apu.tone(1, 784.f, 0.05f);
    toneT_ = 20;
    sys_->setLight(255, 210, 80);
    sys_->rumble(0.2f, 0.5f, 160);
}

void Game::nudge(int dir) { cursor_ = (cursor_ + dir + kLamps) % kLamps; }

void Game::lift() {
    if (mode_ != Mode::Open) return;
    if (cursor_ != kMark && !bot_) return;
    hot_ = kMark;
    hotT_ = kHot;
    hotBad_ = false;
    enterWin();
}

void Game::light(int lamp) {
    if (lamp < 0 || lamp >= kLamps) return;
    if (mode_ == Mode::Title) {
        hot_ = lamp;
        hotT_ = kHot;
        hotBad_ = false;
        chime(lamp, 0.04f);
        cursor_ = lamp;
        return;
    }
    if (mode_ == Mode::Open) {
        cursor_ = lamp;
        if (lamp == kMark) lift();
        return;
    }
    if (mode_ != Mode::Play) return;
    if (lock_ > 0 || hotT_ > 0) {
        pending_ = lamp;
        return;
    }
    cursor_ = lamp;
    hot_ = lamp;
    hotT_ = kHot;
    hotBad_ = false;
    if (lamp == order_[step_]) {
        chime(lamp, 0.08f);
        sys_->rumble(0.f, 0.14f, 24);
        step_++;
        if (step_ >= kOrderN) {
            if (order_[kOrderN - 1] == kMark) enterOpen();
            else enterMiss();
        } else {
            lock_ = kLock;
        }
    } else {
        enterMiss();
    }
}

void Game::readHuman() {
    gs::Pad& p = sys_->pad;
    if (mode_ == Mode::Title || mode_ == Mode::Play || mode_ == Mode::Open) {
        bool left = p.down(gs::BTN_LEFT) || p.down(gs::BTN_UP);
        bool right = p.down(gs::BTN_RIGHT) || p.down(gs::BTN_DOWN);
        if (left == right) {
            hold_ = 0;
        } else {
            int dir = left ? -1 : 1;
            bool edge = left ? (p.pressed(gs::BTN_LEFT) || p.pressed(gs::BTN_UP))
                             : (p.pressed(gs::BTN_RIGHT) || p.pressed(gs::BTN_DOWN));
            if (edge) {
                nudge(dir);
                hold_ = 0;
            } else if (++hold_ >= 14 && hold_ % 5 == 0) {
                nudge(dir);
            }
        }
    }
    if (mode_ == Mode::Title && p.pressed(gs::BTN_START)) {
        begin();
        return;
    }
    bool face = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_X) ||
                p.pressed(gs::BTN_Y) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_TURBO);
    if (face && mode_ != Mode::Win) light(cursor_);
    for (char c : sys_->typed) {
        if (c >= '1' && c <= '6') light(c - '1');
    }
    sys_->typed.clear();
    if (mode_ == Mode::Title && p.pressed(gs::BTN_MODE) && sys_->hasHome()) sys_->eject();
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    const uint64_t fr = sys_->frame;

    float lx[kLamps], cy[kLamps], cordH[kLamps];
    int onPal[kLamps];
    bool on[kLamps];
    for (int i = 0; i < kLamps; i++) {
        float swing = std::sin((float(fr) + i * 17) * 0.045f);
        lx[i] = kX[i] + swing * 2.2f;
        float top = 52.f + kDrop[i] + std::fabs(swing) - ((hotT_ > 0 && hot_ == i) ? 3.f : 0.f);
        cy[i] = top + kLampH * 0.5f;
        cordH[i] = std::max(2.f, top - (kBeamY + 3.f));
        onPal[i] = PAL_DARK;
        on[i] = lampOn(i, onPal[i]);
    }

    int follow = cursor_;
    if (mode_ == Mode::Watch && phase_ != Phase::Lead) follow = order_[std::min(show_, kOrderN - 1)];
    else if (mode_ == Mode::Win) follow = kMark;
    float tx = kX[std::clamp(follow, 0, kLamps - 1)];
    float nx = mothX_ + (tx - mothX_) * 0.06f;
    faceLeft_ = nx < mothX_ - 0.2f;
    mothX_ = nx;
    mothY_ += (64.f - mothY_) * 0.05f;

    spr(art_.moon, 278, 24, 32, PAL_SCENE);
    for (int i = 0; i < 8; i++) {
        int sx = 12 + i * 34;
        int sy = 8 + (i * 13) % 22;
        if (((fr + i * 5) % 24) < 4) continue;
        spr(art_.star, float(sx), float(sy), 5, PAL_SCENE);
    }
    spr(art_.post, 10, 100, 96, PAL_SCENE);
    spr(art_.post, 310, 100, 96, PAL_SCENE);
    spr(art_.beam, 160, kBeamY, 8, PAL_SCENE);
    spr(art_.moth[int((fr / 5) & 1)], mothX_, mothY_, 12, PAL_SCENE, faceLeft_);

    for (int i = 0; i < kLamps; i++)
        if (on[i]) spr(art_.glow, lx[i], cy[i], 70, i == kMark ? PAL_GOLD : onPal[i]);
    for (int i = 0; i < kLamps; i++) spr(art_.cord, lx[i], kBeamY + 2.f + cordH[i] * 0.5f, cordH[i], PAL_SCENE);
    for (int i = 0; i < kLamps; i++) {
        int pal = on[i] ? onPal[i] : PAL_DARK;
        if (i == kMark && !on[i] && mode_ != Mode::Miss) pal = PAL_GOLD;
        spr(art_.lamp, lx[i], cy[i], kLampH, pal);
    }
    spr(art_.ring, kX[kMark], cy[kMark] - 6.f, 22, PAL_GOLD);
    for (int i = 0; i < kLamps; i++)
        if (on[i]) spr(art_.flame[int((fr / 5 + i) & 1)], lx[i], cy[i] - 4.f, 14, i == kMark ? PAL_GOLD : onPal[i]);

    int wick = cursor_;
    if (mode_ == Mode::Watch && phase_ != Phase::Lead) wick = order_[std::min(show_, kOrderN - 1)];
    if (mode_ != Mode::Win) {
        float wx = (mode_ == Mode::Watch) ? kX[wick] : lx[std::clamp(wick, 0, kLamps - 1)];
        float bob = std::sin(float(fr) * 0.22f) * 1.4f;
        spr(art_.wick, wx, 30.f + bob, 11, wick == kMark ? PAL_GOLD : kPal[wick]);
    }

    if (mode_ == Mode::Title) {
        hudC(1, "S3 LANTERNMARK", PAL_GOLD);
        hudC(18, "LIGHT THEM IN ORDER", PAL_HUD);
        hudC(19, "THE GOLD LAMP IS THE MARK", PAL_GOLD);
        hudC(20, "A MISS HANDS ONE BACK", PAL_HUD);
        hudC(22, "ARROWS MOVE THE WICK", PAL_DIM);
        hudC(23, "Z OR C LIGHTS IT", PAL_DIM);
        hudC(24, "4 IS THE GOLD LAMP", PAL_GOLD);
        hudC(25, "ENTER BEGINS", PAL_HUD);
        hudC(27, S3_VERSION_STRING, PAL_DIM);
    } else {
        char line[48];
        std::snprintf(line, sizeof line, "ORDER %d", kOrderN);
        hud(1, 0, line, PAL_HUD);
        std::snprintf(line, sizeof line, "BACK %d", handed_);
        hud(1, 1, line, mode_ == Mode::Miss ? PAL_ROSE : PAL_DIM);
        char pips[8];
        int shown = step_;
        if (mode_ == Mode::Open || mode_ == Mode::Win) shown = kOrderN;
        for (int i = 0; i < kOrderN; i++) pips[i] = i < shown ? (order_[i] == kMark ? 'G' : '*') : '.';
        pips[kOrderN] = 0;
        hud(30, 1, pips, mode_ == Mode::Win ? PAL_JADE : PAL_GOLD);

        const char* banner = "WATCH THE ORDER";
        int bpal = PAL_HUD;
        if (mode_ == Mode::Play) banner = "YOUR TURN";
        else if (mode_ == Mode::Miss) {
            banner = "HANDED BACK TO THE DARK";
            bpal = PAL_ROSE;
        } else if (mode_ == Mode::Open) {
            banner = "LIFT THE GOLD MARK";
            bpal = PAL_GOLD;
        } else if (mode_ == Mode::Win) {
            banner = "FINISHED MARK";
            bpal = PAL_JADE;
        }
        hudC(25, banner, bpal);
        if (mode_ == Mode::Play || mode_ == Mode::Open) hudC(26, "ARROWS  Z  OR  1-6", PAL_DIM);
        else if (mode_ == Mode::Win) hudC(26, "THE NIGHT CAN REST", PAL_DIM);
    }
}

bool Game::lampOn(int i, int& pal) const {
    if (hotT_ > 0 && hot_ == i) {
        pal = hotBad_ ? PAL_ROSE : kPal[i];
        return true;
    }
    if (mode_ == Mode::Title) {
        float w = std::sin((float(sys_->frame) + i * 20) * 0.05f);
        if (i == kMark || w > 0.35f) {
            pal = kPal[i];
            return true;
        }
        return false;
    }
    if (mode_ == Mode::Win || (mode_ == Mode::Open && i == kMark)) {
        pal = kPal[i];
        return i == kMark || mode_ == Mode::Win;
    }
    if (mode_ == Mode::Watch && phase_ == Phase::Show && order_[std::min(show_, kOrderN - 1)] == i) {
        pal = kPal[i];
        return true;
    }
    if ((mode_ == Mode::Play || mode_ == Mode::Miss) && step_ > 0) {
        for (int s = 0; s < step_ && s < kOrderN; s++) {
            if (order_[s] == i) {
                pal = kPal[i];
                return true;
            }
        }
    }
    return false;
}

void Game::sky() {
    const uint16_t top = gs::rgb4(1, 1, 4);
    const uint16_t mid = gs::rgb4(2, 2, 7);
    const uint16_t hor = gs::rgb4(8, 4, 5);
    const uint16_t ground = gs::rgb4(2, 2, 3);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 80) c = mix(top, mid, y / 80.f);
        else if (y < 140) c = mix(mid, hor, (y - 80) / 60.f);
        else c = mix(hor, ground, std::min(1.f, (y - 140) / 84.f));
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(std::max(1.f, w)));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (hotT_ > 0) hotT_--;
    if (lock_ > 0 && mode_ == Mode::Play) lock_--;
    if (toneT_ > 0 && --toneT_ == 0) quiet();

    switch (mode_) {
        case Mode::Title:
            if (bot_) {
                if (sys.frame >= kBotAt) begin();
            } else {
                readHuman();
            }
            break;
        case Mode::Watch:
            if (timer_ > 0) timer_--;
            if (timer_ == 0) {
                if (phase_ == Phase::Lead) {
                    phase_ = Phase::Show;
                    show_ = 0;
                    timer_ = kShow;
                    chime(order_[0], 0.08f);
                } else if (phase_ == Phase::Show) {
                    phase_ = Phase::Gap;
                    timer_ = kGap;
                    quiet();
                } else {
                    show_++;
                    if (show_ >= kOrderN) enterPlay();
                    else {
                        phase_ = Phase::Show;
                        timer_ = kShow;
                        chime(order_[show_], 0.08f);
                    }
                }
            }
            break;
        case Mode::Play:
            if (!bot_) readHuman();
            else sys.typed.clear();
            if (mode_ == Mode::Play && lock_ == 0 && hotT_ == 0) {
                if (pending_ >= 0) {
                    int p = pending_;
                    pending_ = -1;
                    light(p);
                } else if (bot_ && step_ < kOrderN) {
                    light(order_[step_]);
                }
            }
            break;
        case Mode::Miss:
            if (timer_ > 0) timer_--;
            if (timer_ == 0) enterWatch();
            break;
        case Mode::Open:
            openT_++;
            if (!bot_) readHuman();
            else if (openT_ >= 12 && mode_ == Mode::Open) lift();
            break;
        case Mode::Win:
            sys.typed.clear();
            if (timer_ > 0) timer_--;
            if (timer_ == 0) over_ = true;
            if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
            break;
    }
    draw();
}

}  // namespace lanternmark
