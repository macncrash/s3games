#include "game/horntape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace horntape {

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
        if (id >= 0 && id < kParts) s += kPart[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kPart[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kPart[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Rise || mode_ == Mode::Blow || mode_ == Mode::Rest) return 1;
    if (mode_ == Mode::Full) return 2;
    if (mode_ == Mode::Done && won_) return 3;
    return 4;
}

int Game::nextWant() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return kTape[i];
    return -1;
}

const gs::Image& Game::partImg(int id) const {
    if (id == 1) return art_.valve;
    if (id == 2) return art_.flare;
    if (id == 3) return art_.squeal;
    return art_.lip;
}

bool Game::prove() {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        const Part& p = kPart[kTape[i]];
        if (!p.name || !p.name[0] || p.pay <= 0 || p.decoy) {
            why_ = "TAPE";
            return false;
        }
        for (int j = 0; j < i; j++)
            if (std::strcmp(kPart[kTape[i]].name, kPart[kTape[j]].name) == 0) {
                why_ = "DUP";
                return false;
            }
        sum += p.pay;
    }
    if (sum != 19) {
        why_ = "SUM";
        return false;
    }
    const Part& decoy = kPart[3];
    if (!decoy.decoy || decoy.pay != kPart[1].pay || std::strcmp(decoy.name, kPart[1].name) == 0) {
        why_ = "DECOY";
        return false;
    }
    int seen[kParts] = {};
    for (int i = 0; i < kDeckN; i++) {
        int id = kDeck[i];
        if (id < 0 || id >= kParts) {
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
    filled_ = faults_ = blows_ = offer_ = age_ = anim_ = 0;
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
    mode_ = Mode::Rise;
    why_ = "BREATH";
}

void Game::filePart() {
    int id = kDeck[offer_];
    int slot = filled_;
    drawer_[slot] = id;
    held_[slot] = true;
    filled_++;
    blows_++;
    offer_++;
    anim_ = 0;
    mode_ = Mode::Blow;
    why_ = "FILED";
    sys_->apu.keyOn(0, 196.f + slot * 48.f, 0.34f);
    sys_->apu.keyOn(1, 294.f + slot * 24.f, 0.16f);
}

void Game::blow(bool inWindow) {
    int id = kDeck[offer_];
    const Part& p = kPart[id];
    int want = nextWant();
    if (!inWindow) {
        faults_++;
        fail("MISSED THE HORN");
        return;
    }
    if (p.decoy || id != want) {
        faults_++;
        fail(p.decoy ? "SQUEAL STAYS OUT" : "OUT OF ORDER");
        return;
    }
    filePart();
}

void Game::expire() {
    int id = kDeck[offer_];
    if (kPart[id].decoy) {
        offer_++;
        anim_ = 0;
        mode_ = Mode::Rest;
        why_ = "SQUEAL FELL";
        sys_->apu.noiseBurst(0.06f, 90.f, 0.04f);
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
    mode_ = Mode::Fail;
    anim_ = 0;
    sys_->apu.noiseBurst(0.2f, 55.f, 0.12f);
}

void Game::depart() {
    if (!matched() || faults_ != 0 || drawerScore() != 19) {
        fail("DRAWER");
        return;
    }
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Done;
    why_ = "the drawer matches the tape";
    anim_ = 0;
    sys_->apu.keyOn(0, 262.f, 0.3f);
    sys_->apu.keyOn(1, 392.f, 0.2f);
    sys_->apu.keyOn(2, 523.f, 0.14f);
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
    uint16_t top = gs::rgb4(1, 2, 4);
    uint16_t mid = gs::rgb4(3, 4, 7);
    uint16_t bot = gs::rgb4(2, 2, 3);
    if (mode_ == Mode::Done && won_) mid = gs::rgb4(2, 6, 4);
    if (mode_ == Mode::Fail) mid = gs::rgb4(6, 1, 2);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        auto mix = [](uint16_t a, uint16_t b, float t) {
            int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
            int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
            auto L = [&](int p, int q) { return int(p + (q - p) * t + 0.5f); };
            return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
        };
        v.lineBackdrop[y] = u < 0.55f ? mix(top, mid, u / 0.55f) : mix(mid, bot, (u - 0.55f) / 0.45f);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const float hx = 150.f;
    const float hy = 108.f;
    spr(art_.horn, hx, hy, PAL_BRASS);
    spr(art_.bell, hx + 62.f, hy - 28.f, PAL_FLARE);

    int show = -1;
    if (mode_ == Mode::Rise || mode_ == Mode::Blow) show = kDeck[offer_ < kDeckN ? offer_ : kDeckN - 1];
    if (mode_ == Mode::Blow && offer_ > 0) show = kDeck[offer_ - 1];
    if (show >= 0 && mode_ != Mode::Done) {
        float py = hy - 52.f;
        if (mode_ == Mode::Blow) py = hy - 52.f - anim_ * 1.4f;
        spr(partImg(show), hx - 54.f, py, kPart[show].pal);
    }

    float by = hy + 8.f;
    if (mode_ == Mode::Rise) {
        float u = age_ / float(kDieAt);
        by = hy + 24.f - 70.f * u;
    } else if (mode_ == Mode::Blow) {
        by = hy - 40.f - anim_ * 0.6f;
    }
    spr(art_.breath, hx - 8.f, by, PAL_AIR);

    for (int i = 0; i < kTapeN; i++) {
        float x = 72.f + i * 70.f;
        spr(art_.drawer, x, 198.f, held_[i] ? PAL_PAPER : PAL_DIM);
        if (held_[i]) spr(partImg(drawer_[i]), x, 192.f, kPart[drawer_[i]].pal);
    }

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 HORNTAPE", PAL_GOLD);
        hudC(16, "PLAY THE HORN", PAL_HUD);
        hudC(18, "UNTIL THE DRAWER", PAL_HUD);
        hudC(19, "MATCHES THE TAPE", PAL_GOLD);
        hudC(21, "LIP  VALVE  FLARE", PAL_HUD);
        if ((f & 16) == 0) hudC(26, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 0, ver, PAL_DIM);
    } else if (mode_ == Mode::Done && won_) {
        hudC(2, "THE DRAWER MATCHES", PAL_GOLD);
        hudC(4, "YOU LEAVE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "%s %d   %s %d   %s %d", tapeLabel(0), tapeScore(0), tapeLabel(1),
                      tapeScore(1), tapeLabel(2), tapeScore(2));
        hudC(21, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hudC(23, buf, PAL_GOLD);
    } else if (mode_ == Mode::Fail) {
        hudC(2, "THE HORN FAILED", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Full) {
        hudC(2, "DRAWER FULL", PAL_GOLD);
        hudC(20, "THE TAPE IS IN THE DRAWER", PAL_HUD);
        hudC(22, "LEAVE", PAL_GOLD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START LEAVES", PAL_DIM);
    } else {
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(30, 1, buf, PAL_HUD);
        if (mode_ == Mode::Rise && offer_ < kDeckN) {
            int id = kDeck[offer_];
            std::snprintf(buf, sizeof buf, "%s %d", kPart[id].name, kPart[id].pay);
            hudC(22, buf, kPart[id].decoy ? PAL_BAD : PAL_GOLD);
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
            hudC(26, "A PLAYS THE HORN", PAL_DIM);
        } else if (mode_ == Mode::Blow) {
            hudC(24, "INTO THE DRAWER", PAL_GOLD);
        } else if (mode_ == Mode::Rest) {
            hudC(24, "SQUEAL STAYS OUT", PAL_DIM);
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
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Rise) {
        bool tap = false;
        int id = kDeck[offer_];
        if (bot_) {
            if (!kPart[id].decoy && id == nextWant()) tap = age_ == (kSweetLo + kSweetHi) / 2;
        } else {
            tap = pad.pressed(gs::BTN_A);
        }
        if (tap) {
            blow(age_ >= kSweetLo && age_ <= kSweetHi);
        } else if (++age_ >= kDieAt) {
            expire();
        }
    } else if (mode_ == Mode::Blow) {
        anim_++;
        if (anim_ >= 24) {
            if (matched()) {
                mode_ = Mode::Full;
                anim_ = 0;
                why_ = "FULL";
            } else {
                arm();
            }
        }
    } else if (mode_ == Mode::Rest) {
        anim_++;
        if (anim_ >= 14) arm();
    } else if (mode_ == Mode::Full) {
        anim_++;
        if (bot_) {
            if (anim_ >= 18) depart();
        } else if (pad.pressed(gs::BTN_START)) {
            depart();
        }
    } else if (mode_ == Mode::Fail) {
        anim_++;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
    } else if (mode_ == Mode::Done) {
        anim_++;
    }

    draw();
}

}  // namespace horntape
