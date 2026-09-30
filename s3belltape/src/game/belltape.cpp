#include "game/belltape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace belltape {

bool Game::matched() const {
    if (filled_ != kTapeN) return false;
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i] || drawer_[i] != kTape[i]) return false;
    return true;
}

int Game::drawerScore() const {
    int s = 0;
    for (int i = 0; i < filled_; i++) {
        int id = drawer_[i];
        if (id >= 0 && id < kNotes) s += kNote[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kNote[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kNote[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Swing || mode_ == Mode::Ring || mode_ == Mode::Gap) return 1;
    if (mode_ == Mode::Pocket) return 2;
    if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) return 3;
    return 4;
}

int Game::nextWant() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return kTape[i];
    return -1;
}

const gs::Image& Game::noteImg(int id) const {
    if (id == 1) return art_.knock;
    if (id == 2) return art_.toll;
    if (id == 3) return art_.clang;
    return art_.peal;
}

bool Game::prove() {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        const Note& n = kNote[kTape[i]];
        if (!n.name || !n.name[0] || n.pay <= 0 || n.decoy) {
            why_ = "TAPE";
            return false;
        }
        for (int j = 0; j < i; j++)
            if (std::strcmp(kNote[kTape[i]].name, kNote[kTape[j]].name) == 0) {
                why_ = "DUP";
                return false;
            }
        sum += n.pay;
    }
    if (sum != 15) {
        why_ = "SUM";
        return false;
    }
    const Note& decoy = kNote[3];
    if (!decoy.decoy || decoy.pay != kNote[1].pay || std::strcmp(decoy.name, kNote[1].name) == 0) {
        why_ = "DECOY";
        return false;
    }
    int seen[kNotes] = {};
    for (int i = 0; i < kDeckN; i++) {
        int id = kDeck[i];
        if (id < 0 || id >= kNotes) {
            why_ = "DECK";
            return false;
        }
        seen[id]++;
    }
    if (seen[0] != 1 || seen[1] != 1 || seen[2] != 1 || seen[3] != 2) {
        why_ = "DECK";
        return false;
    }
    why_ = "OPEN";
    return true;
}

void Game::begin() {
    filled_ = faults_ = rings_ = offer_ = age_ = anim_ = 0;
    over_ = won_ = left_ = false;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    why_ = rules_ ? "OPEN" : why_;
    sys_->apu.silence();
    if (rules_) arm();
    else fail("RULES");
}

void Game::arm() {
    if (offer_ >= kDeckN) {
        fail("SHORT");
        return;
    }
    age_ = anim_ = 0;
    mode_ = Mode::Swing;
    why_ = "SWING";
}

void Game::fileNote() {
    int id = kDeck[offer_];
    int slot = filled_;
    drawer_[slot] = id;
    held_[slot] = true;
    filled_++;
    rings_++;
    offer_++;
    anim_ = 0;
    mode_ = Mode::Ring;
    why_ = "FILED";
    sys_->apu.keyOn(0, 330.f + slot * 55.f, 0.32f);
    sys_->apu.tone(1, 660.f + slot * 30.f, 0.12f);
}

void Game::strike(bool inWindow) {
    int id = kDeck[offer_];
    const Note& n = kNote[id];
    int want = nextWant();
    if (!inWindow) {
        faults_++;
        fail("MISSED THE LIP");
        return;
    }
    if (n.decoy || id != want) {
        faults_++;
        fail(n.decoy ? "CLANG STAYS OUT" : "OUT OF ORDER");
        return;
    }
    fileNote();
}

void Game::expire() {
    int id = kDeck[offer_];
    if (kNote[id].decoy) {
        offer_++;
        anim_ = 0;
        mode_ = Mode::Gap;
        why_ = "CLANG FADED";
        sys_->apu.noiseBurst(0.08f, 80.f, 0.05f);
        return;
    }
    faults_++;
    fail("THE NOTE DIED");
}

void Game::fail(const char* why) {
    why_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    mode_ = Mode::Lose;
    anim_ = 0;
    sys_->apu.noiseBurst(0.18f, 40.f, 0.12f);
}

void Game::depart() {
    if (!matched() || faults_ != 0 || drawerScore() != 15) {
        fail("DRAWER");
        return;
    }
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    why_ = "the drawer matches the tape";
    anim_ = 0;
    sys_->apu.keyOn(0, 392.f, 0.28f);
    sys_->apu.keyOn(1, 494.f, 0.22f);
    sys_->apu.keyOn(2, 587.f, 0.18f);
}

void Game::spr(const gs::Image& img, float cx, float cy, int pal) {
    if (img.w == 0) return;
    gs::Sprite s;
    s.img = img;
    s.w = img.w;
    s.h = img.h;
    s.x = int16_t(std::lround(cx - img.w * 0.5f));
    s.y = int16_t(std::lround(cy - img.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    uint16_t top = gs::rgb4(1, 2, 5);
    uint16_t mid = gs::rgb4(3, 5, 9);
    uint16_t bot = gs::rgb4(2, 2, 3);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(6, 8, 4);
    if (mode_ == Mode::Lose) mid = gs::rgb4(7, 2, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.6f ? mix(top, mid, u / 0.6f) : mix(mid, bot, (u - 0.6f) / 0.4f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float bx = 168.f;
    spr(art_.belfry, bx, 36.f, PAL_WOOD);
    spr(art_.bell, bx, 92.f, PAL_BELL);

    float sway = 0.f;
    if (mode_ == Mode::Swing) sway = std::sin(age_ * 0.18f) * 10.f;
    else if (mode_ == Mode::Ring) sway = std::sin(anim_ * 0.7f) * (12.f - anim_ * 0.3f);
    spr(art_.clapper, bx + sway, 108.f, PAL_BELL);
    spr(art_.rope, bx + sway * 0.4f, 156.f, PAL_ROPE);

    int show = -1;
    if ((mode_ == Mode::Swing || mode_ == Mode::Ring) && offer_ < kDeckN) show = kDeck[offer_];
    if (mode_ == Mode::Ring && offer_ > 0) show = kDeck[offer_ - 1];
    if (show >= 0 && mode_ != Mode::Leave && mode_ != Mode::Over && mode_ != Mode::Pocket) {
        float ny = 92.f;
        if (mode_ == Mode::Ring) ny = 70.f - anim_ * 0.6f;
        spr(noteImg(show), bx - 46.f, ny, kNote[show].pal);
    }

    for (int i = 0; i < kTapeN; i++) {
        float dx = 64.f + i * 58.f;
        spr(art_.drawer, dx, 204.f, held_[i] ? PAL_WOOD : PAL_DIM);
        if (held_[i]) spr(noteImg(drawer_[i]), dx, 198.f, kNote[drawer_[i]].pal);
    }

    for (int i = 0; i < kTapeN; i++) {
        int pal = held_[i] ? PAL_GOLD : PAL_DIM;
        spr(art_.lamp, 24.f + i * 14.f, 18.f, pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 BELLTAPE", PAL_GOLD);
        hudC(17, "PLAY THE BELL", PAL_HUD);
        hudC(19, "UNTIL THE DRAWER", PAL_HUD);
        hudC(20, "MATCHES THE TAPE", PAL_GOLD);
        hudC(22, "PEAL  KNOCK  TOLL", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Over && won_) {
        hudC(2, "THE DRAWER MATCHES", PAL_GOLD);
        hudC(4, "YOU LEAVE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%s %d   %s %d   %s %d", tapeLabel(0), tapeScore(0), tapeLabel(1),
                      tapeScore(1), tapeLabel(2), tapeScore(2));
        hudC(21, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Lose) {
        hudC(2, "THE BELL FAILED", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pocket) {
        hudC(2, "DRAWER FULL", PAL_GOLD);
        hudC(21, "THE TAPE IS IN THE DRAWER", PAL_HUD);
        hudC(23, "LEAVE", PAL_GOLD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START LEAVES", PAL_DIM);
    } else {
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(30, 1, buf, PAL_HUD);
        if (mode_ == Mode::Swing && offer_ < kDeckN) {
            int id = kDeck[offer_];
            std::snprintf(buf, sizeof buf, "%s %d", kNote[id].name, kNote[id].pay);
            hudC(22, buf, kNote[id].decoy ? PAL_BAD : PAL_GOLD);
            int mark = age_ * 20 / kDieAt;
            if (mark > 19) mark = 19;
            char bar[24];
            for (int i = 0; i < 20; i++) {
                int at = i * kDieAt / 20;
                bool sweet = at >= kSweetLo && at <= kSweetHi;
                bar[i] = (i == mark) ? '|' : (sweet ? '=' : '-');
            }
            bar[20] = 0;
            hudC(24, bar, PAL_GOLD);
            hudC(26, "C RINGS THE LIP", PAL_DIM);
        } else if (mode_ == Mode::Ring) {
            hudC(24, "INTO THE DRAWER", PAL_GOLD);
        } else if (mode_ == Mode::Gap) {
            hudC(24, "CLANG STAYS OUT", PAL_DIM);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    rules_ = prove();
    mode_ = Mode::Title;
    if (bot_) begin();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Swing) {
        bool tap = false;
        int id = kDeck[offer_];
        if (bot_) {
            if (!kNote[id].decoy && id == nextWant()) tap = age_ == (kSweetLo + kSweetHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
        }
        if (tap) {
            strike(age_ >= kSweetLo && age_ <= kSweetHi);
        } else if (++age_ >= kDieAt) {
            expire();
        }
    } else if (mode_ == Mode::Ring) {
        anim_++;
        if (anim_ >= 26) {
            if (matched()) {
                mode_ = Mode::Pocket;
                anim_ = 0;
                why_ = "FULL";
            } else {
                arm();
            }
        }
    } else if (mode_ == Mode::Gap) {
        anim_++;
        if (anim_ >= 16) arm();
    } else if (mode_ == Mode::Pocket) {
        anim_++;
        if (bot_) {
            if (anim_ >= 20) depart();
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            depart();
        }
    } else if (mode_ == Mode::Lose) {
        anim_++;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Over) {
        anim_++;
    }

    draw();
}

}  // namespace belltape
