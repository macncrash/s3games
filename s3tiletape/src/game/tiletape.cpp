#include "game/tiletape.h"

#include <cstdio>
#include <cstring>

namespace tiletape {
namespace {

constexpr int PITCH = 44;
constexpr int OX = (gs::SCREEN_W - N * PITCH) / 2;
constexpr int TAPE_Y = 58;
constexpr int DRAWER_Y = 128;

}  // namespace

bool Game::prove() {
    bool seen[N] = {};
    for (int i = 0; i < N; i++) {
        int id = art_.order[i];
        if (id < 0 || id >= N || seen[id]) return false;
        seen[id] = true;
    }
    bool same = true;
    for (int i = 0; i < N; i++)
        if (drawer_[i] != art_.order[i]) same = false;
    return !same;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.7f);
    phase_ = Phase::Title;
    t_ = 0;
    cx_ = 0;
    hold_ = -1;
    moves_ = 0;
    won_ = false;
    over_ = false;
    left_ = false;
    paused_ = false;
    reason_ = "OPEN";
    for (int i = 0; i < N; i++) drawer_[i] = i;
    rules_ = prove();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (beep_ > 0 && --beep_ == 0) sys.apu.tone(0, 0, 0);
    if (paused_) {
        if (sys.pad.pressed(gs::BTN_START)) paused_ = false;
        draw();
        return;
    }
    update();
    draw();
}

int Game::marker() const {
    if (phase_ == Phase::Title) return 0;
    if (phase_ == Phase::Set) return 1;
    if (phase_ == Phase::Seal) return 2;
    return 3;
}

bool Game::matched() const {
    if (hold_ >= 0) return false;
    for (int i = 0; i < N; i++)
        if (drawer_[i] != art_.order[i]) return false;
    return true;
}

int Game::indexOf(int id) const {
    for (int i = 0; i < N; i++)
        if (drawer_[i] == id) return i;
    return -1;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.16f);
    beep_ = 6;
}

void Game::pickOrDrop() {
    int here = drawer_[cx_];
    if (hold_ < 0) {
        if (here < 0) return;
        hold_ = here;
        drawer_[cx_] = -1;
        blip(180.f + float(cx_) * 40.f);
        return;
    }
    drawer_[cx_] = hold_;
    hold_ = here;
    moves_++;
    blip(hold_ < 0 ? 392.f : 260.f);
}

void Game::update() {
    const gs::Pad& p = sys_->pad;
    t_++;
    if (phase_ == Phase::Title) {
        if (bot_ ? t_ > 18 : (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C))) {
            if (!rules_) {
                reason_ = "RULES";
                return;
            }
            phase_ = Phase::Set;
            t_ = 0;
            reason_ = "OPEN";
        }
        return;
    }
    if (phase_ == Phase::Seal) {
        if (t_ > (bot_ ? 16 : 40)) {
            if (!matched()) {
                reason_ = "DOES NOT MATCH";
                phase_ = Phase::Set;
                t_ = 0;
                won_ = false;
                return;
            }
            phase_ = Phase::Leave;
            t_ = 0;
            won_ = true;
            left_ = true;
            reason_ = "LEAVE";
            sys_->setLight(90, 220, 120);
            blip(523.f);
        }
        return;
    }
    if (phase_ == Phase::Leave) {
        if (t_ > (bot_ ? 12 : 80)) over_ = true;
        return;
    }

    if (!bot_ && p.pressed(gs::BTN_START)) {
        paused_ = true;
        return;
    }

    if (bot_) {
        if (matched()) {
            phase_ = Phase::Seal;
            t_ = 0;
            reason_ = "SEAL";
            return;
        }
        if (botWait_ > 0) {
            botWait_--;
            return;
        }
        botWait_ = 5;
        if (hold_ < 0) {
            int want = -1;
            for (int i = 0; i < N; i++) {
                if (drawer_[i] != art_.order[i]) {
                    want = i;
                    break;
                }
            }
            if (want < 0) return;
            if (cx_ < want) cx_++;
            else if (cx_ > want) cx_--;
            else pickOrDrop();
        } else {
            int home = -1;
            for (int i = 0; i < N; i++)
                if (art_.order[i] == hold_) home = i;
            if (home < 0) return;
            if (cx_ < home) cx_++;
            else if (cx_ > home) cx_--;
            else pickOrDrop();
        }
        if (matched()) {
            phase_ = Phase::Seal;
            t_ = 0;
            reason_ = "SEAL";
        }
        return;
    }

    if (p.pressed(gs::BTN_LEFT) && cx_ > 0) cx_--;
    if (p.pressed(gs::BTN_RIGHT) && cx_ < N - 1) cx_++;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C)) pickOrDrop();
    if (matched()) {
        phase_ = Phase::Seal;
        t_ = 0;
        reason_ = "SEAL";
    } else {
        reason_ = "OPEN";
    }
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Image& img, int x, int y, int w, int h, int pal) {
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(x);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int g = 2 + (y < 100 ? 0 : 1);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(2, g, 3);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.A.clear();
    vdp.B.clear();
    vdp.HUD.clear();
    backdrop();

    if (phase_ == Phase::Title) {
        hudC(6, "S3 TILETAPE", PAL_GOLD);
        hudC(9, "A SHORT TILE", PAL_CREAM);
        hudC(11, "THE DRAWER HAS TO", PAL_CREAM);
        hudC(13, "MATCH THE TAPE", PAL_CREAM);
        hudC(17, bot_ ? "SETTING" : "PRESS START", PAL_LEAF);
        hudC(22, "ARROWS MOVE   A LIFTS", PAL_DIM);
        vdp.render(sys_->fb);
        return;
    }

    hudC(1, "TAPE", PAL_GOLD);
    spr(art_.tape, OX - 8, TAPE_Y - 6, N * PITCH + 16, 28, PAL_TILE);
    for (int i = 0; i < N; i++) {
        int id = art_.order[i];
        spr(art_.tile[id], OX + i * PITCH + 4, TAPE_Y, 36, 16, PAL_TILE);
    }

    hudC(12, "DRAWER", PAL_CREAM);
    spr(art_.drawer, OX - 10, DRAWER_Y - 10, N * PITCH + 20, 40, PAL_TILE);
    for (int i = 0; i < N; i++) {
        int id = drawer_[i];
        if (id < 0) continue;
        int bob = (phase_ == Phase::Set && i == cx_ && hold_ < 0) ? -1 : 0;
        spr(art_.tile[id], OX + i * PITCH + 4, DRAWER_Y + 4 + bob, 36, 16, PAL_TILE);
    }
    if (hold_ >= 0) {
        spr(art_.tile[hold_], OX + cx_ * PITCH + 4, DRAWER_Y - 22, 36, 16, PAL_TILE);
        spr(art_.lift, OX + cx_ * PITCH + 18, DRAWER_Y - 30, 8, 8, PAL_TILE);
    }
    if (phase_ == Phase::Set && !paused_)
        spr(art_.cursor, OX + cx_ * PITCH + 4, DRAWER_Y + 24, 36, 6, PAL_TILE);

    int same = 0;
    for (int i = 0; i < N; i++)
        if (drawer_[i] == art_.order[i]) same++;
    char line[40];
    std::snprintf(line, sizeof line, "IN PLACE %d/%d   MOVES %d", same, N, moves_);
    hudC(23, line, same == N && hold_ < 0 ? PAL_LEAF : PAL_DIM);

    if (paused_) hudC(20, "PAUSED", PAL_GOLD);
    else if (phase_ == Phase::Seal) hudC(20, "THE DRAWER MATCHES", PAL_LEAF);
    else if (phase_ == Phase::Leave) hudC(20, "LEAVE", PAL_GOLD);
    else if (same == N - 1) hudC(25, "CLOSE IS STILL OPEN", PAL_DIM);
    else hudC(25, "A LIFT OR SET", PAL_DIM);

    vdp.render(sys_->fb);
}

}  // namespace tiletape
