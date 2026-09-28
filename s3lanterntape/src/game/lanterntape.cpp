#include "game/lanterntape.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace lanterntape {
namespace {

constexpr float kFeet = 176.f;
constexpr float kReach = 16.f;

}  // namespace

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < kTapeN; i++)
        if (held_[i]) s += kPost[i].pay;
    if (junk_) s += kPost[3].pay;
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kPost[i].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kPost[i].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return 0;
    if (mode_ == Mode::Walk) return 1;
    if (mode_ == Mode::Pocket) return 2;
    if (mode_ == Mode::Drawer) return 3;
    if (mode_ == Mode::Shut) return 4;
    return 1;
}

bool Game::prove() {
    if (std::strcmp(kPost[0].name, "WICK") || kPost[0].pay != 5 || kPost[0].decoy) return false;
    if (std::strcmp(kPost[1].name, "GLASS") || kPost[1].pay != 3 || kPost[1].decoy) return false;
    if (std::strcmp(kPost[2].name, "HOOK") || kPost[2].pay != 4 || kPost[2].decoy) return false;
    if (std::strcmp(kPost[3].name, "SNUFF") || kPost[3].pay != 5 || !kPost[3].decoy) return false;
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) sum += kPost[i].pay;
    return sum == 12;
}

int Game::nextOpen() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return i;
    return -1;
}

int Game::nearest() const {
    int best = -1;
    float d = kReach;
    for (int i = 0; i < 4; i++) {
        float a = std::fabs(px_ - kPost[i].x);
        if (a <= d) {
            d = a;
            best = i;
        }
    }
    return best;
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 0.5f, vol * 0.35f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = prove();
    reason_ = rules_ ? "OPEN" : "RULES";
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    shut_ = false;
    junk_ = false;
    sets_ = 0;
    traps_ = 0;
    timer_ = 0;
    pocket_ = -1;
    px_ = 20.f;
    faceL_ = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    if (!rules_) reason_ = "RULES";
}

void Game::begin() {
    if (!rules_) return;
    junk_ = false;
    sets_ = 0;
    traps_ = 0;
    pocket_ = -1;
    px_ = 20.f;
    faceL_ = false;
    won_ = false;
    shut_ = false;
    over_ = false;
    for (int i = 0; i < kTapeN; i++) held_[i] = false;
    mode_ = Mode::Walk;
    timer_ = 0;
    reason_ = "OPEN";
    blip(392.f, 0.08f);
}

void Game::take(int line) {
    held_[line] = true;
    sets_++;
    pocket_ = line;
    mode_ = Mode::Pocket;
    timer_ = 28;
    reason_ = kPost[line].name;
    blip(440.f + line * 40.f, 0.12f);
    if (sys_) sys_->setLight(220, 160, 40);
}

void Game::miss(const char* why) {
    traps_++;
    junk_ = true;
    reason_ = why;
    beginLose();
}

void Game::setHere() {
    int n = nearest();
    if (n < 0) return;
    int want = nextOpen();
    if (kPost[n].decoy) {
        miss("SNUFF STAYS OUT");
        return;
    }
    if (n != want) {
        miss("OUT OF ORDER");
        return;
    }
    take(n);
    if (sets_ >= kTapeN && matched()) beginDrawer();
}

void Game::beginDrawer() {
    mode_ = Mode::Drawer;
    timer_ = 0;
    reason_ = matched() ? "MATCH" : "DOES NOT MATCH";
    if (sys_) sys_->setLight(180, 140, 60);
}

void Game::beginShut() {
    if (!matched() || sets_ != kTapeN || drawerScore() != 12 || traps_ != 0) {
        miss("DOES NOT MATCH");
        return;
    }
    mode_ = Mode::Shut;
    timer_ = 50;
    shut_ = true;
    won_ = true;
    reason_ = "SHUT";
    blip(523.f, 0.14f);
    if (sys_) {
        sys_->rumble(0.2f, 0.45f, 140);
        sys_->setLight(255, 190, 70);
    }
}

void Game::beginLose() {
    mode_ = Mode::Lose;
    timer_ = 70;
    won_ = false;
    shut_ = false;
    blip(110.f, 0.1f);
    if (sys_) sys_->setLight(80, 0, 0);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < 132) v.lineBackdrop[y] = gs::rgb4(1, 1, 4);
        else if (y < 156) v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        else if (y < 176) v.lineBackdrop[y] = gs::rgb4(2, 2, 2);
        else v.lineBackdrop[y] = gs::rgb4(1, 2, 1);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s{};
    s.w = int16_t(std::max(1, std::min(2000, int(std::lround(w)))));
    s.h = int16_t(std::max(1, std::min(2000, int(std::lround(h)))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
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
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    backdrop();
    spr(art_.moon, 274, 28, 26, PAL_MOON);
    const float stars[][2] = {{18, 18}, {48, 36}, {90, 14}, {140, 28}, {200, 16}, {230, 40}};
    for (auto& st : stars) spr(art_.star, st[0], st[1], 7, PAL_MOON);

    for (int i = 3; i >= 0; i--) {
        bool lit = (i < kTapeN && held_[i]) || (mode_ == Mode::Pocket && pocket_ == i);
        int pal = lit ? PAL_LIT : PAL_POST;
        float x = kPost[i].x;
        spr(art_.post, x, kFeet - 22, 52, pal);
        if (lit) spr(art_.flame, x, kFeet - 50, 12, PAL_LAMP);
        hud(int(x) / 8 - 2, 23, kPost[i].name, lit ? PAL_HUD : PAL_HUD);
    }

    if (mode_ != Mode::Title && mode_ != Mode::Shut) {
        spr(art_.walker, px_, kFeet - 16, 40, PAL_WALK);
        spr(art_.lamp, px_ + (faceL_ ? -12.f : 12.f), kFeet - 28, 22, PAL_LAMP);
        spr(art_.flame, px_ + (faceL_ ? -12.f : 12.f), kFeet - 40, 10, PAL_LAMP);
    }
    if (mode_ == Mode::Shut) {
        spr(art_.lamp, 160, 108, 48, PAL_LAMP);
        spr(art_.flame, 160, 86, 0, PAL_LAMP);
    }

    if (mode_ == Mode::Drawer || mode_ == Mode::Shut || mode_ == Mode::Pocket) {
        for (int i = 0; i < kTapeN; i++) {
            if (!held_[i] && pocket_ != i) continue;
            spr(art_.slip, 70.f + i * 70.f, 78, 16, PAL_SLIP);
        }
    }

    if (mode_ == Mode::Title) {
        hudC(4, "S3 LANTERNTAPE", PAL_HUD);
        hudC(6, "A SHORT LANTERN", PAL_HUD);
        hudC(10, "SET WICK  GLASS  HOOK", PAL_HUD);
        hudC(12, "SNUFF PAYS 5 AND STAYS OUT", PAL_HUD);
        hudC(16, "ARROWS MOVE   A SETS", PAL_HUD);
        hudC(18, "START SHUTS A MATCHED TILL", PAL_HUD);
        hudC(22, "PRESS START", PAL_HUD);
    } else if (mode_ == Mode::Walk) {
        hud(1, 1, "TAPE", PAL_HUD);
        hud(6, 1, "WICK 5  GLASS 3  HOOK 4", PAL_HUD);
        char till[24];
        std::snprintf(till, sizeof till, "TILL %d", drawerScore());
        hud(30, 1, till, PAL_HUD);
        hudC(25, "A SETS THE SHORT LANTERN", PAL_HUD);
    } else if (mode_ == Mode::Pocket) {
        char line[32];
        std::snprintf(line, sizeof line, "%s  %d", pocket_ >= 0 ? kPost[pocket_].name : "",
                      pocket_ >= 0 ? kPost[pocket_].pay : 0);
        hudC(3, line, PAL_HUD);
    } else if (mode_ == Mode::Drawer) {
        hudC(2, "THE DRAWER", PAL_HUD);
        hud(6, 12, "WICK 5", held_[0] ? PAL_HUD : PAL_HUD);
        hud(16, 12, "GLASS 3", PAL_HUD);
        hud(27, 12, "HOOK 4", PAL_HUD);
        char till[32];
        std::snprintf(till, sizeof till, "TILL %d   TAPE 12", drawerScore());
        hudC(16, till, matched() ? PAL_HUD : PAL_HUD);
        hudC(20, matched() ? "START SHUTS IT" : "DOES NOT MATCH", PAL_HUD);
    } else if (mode_ == Mode::Shut) {
        hudC(16, "THE SHORT LANTERN IS SHUT", PAL_HUD);
        hudC(18, "THE DRAWER MATCHES THE TAPE", PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        hudC(10, reason_, PAL_HUD);
        hudC(14, "A TILL THAT IS CLOSE IS STILL OPEN", PAL_HUD);
        hudC(18, "START TRIES THE LANE AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT);
    bool right = pad.down(gs::BTN_RIGHT);
    bool a = pad.pressed(gs::BTN_A);
    bool start = pad.pressed(gs::BTN_START);

    if (bot_) {
        left = right = a = start = false;
        if (mode_ == Mode::Title && timer_ > 10) start = true;
        if (mode_ == Mode::Walk) {
            int n = nextOpen();
            if (n >= 0) {
                float tx = kPost[n].x;
                if (px_ < tx - 2.f) right = true;
                else if (px_ > tx + 2.f) left = true;
                else a = true;
            }
        }
        if (mode_ == Mode::Drawer && timer_ > 20 && matched()) start = true;
        if (mode_ == Mode::Lose && timer_ > 30) start = true;
    }

    if (mode_ == Mode::Title) {
        timer_++;
        if (start || a) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = heldMode_;
    } else if (mode_ == Mode::Walk) {
        if (start && !bot_) {
            heldMode_ = mode_;
            mode_ = Mode::Pause;
        } else {
            if (left) {
                px_ -= 1.7f;
                faceL_ = true;
            }
            if (right) {
                px_ += 1.7f;
                faceL_ = false;
            }
            px_ = std::max(12.f, std::min(308.f, px_));
            if (a) setHere();
        }
    } else if (mode_ == Mode::Pocket) {
        if (--timer_ <= 0) {
            if (sets_ >= kTapeN && matched()) beginDrawer();
            else mode_ = Mode::Walk;
        }
    } else if (mode_ == Mode::Drawer) {
        timer_++;
        if (start) beginShut();
    } else if (mode_ == Mode::Shut) {
        if (--timer_ <= 0) {
            over_ = true;
            won_ = true;
            if (!bot_) {
                sys.apu.tone(0, 0, 0);
                sys.apu.tone(1, 0, 0);
            }
        }
    } else if (mode_ == Mode::Lose) {
        timer_++;
        if (start || (bot_ && timer_ > 40)) toTitle();
    }

    if (timer_ % 18 == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    draw();
}

}  // namespace lanterntape
