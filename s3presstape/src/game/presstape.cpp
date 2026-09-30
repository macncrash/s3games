#include "game/presstape.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace presstape {

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
        if (id >= 0 && id < kBlanks) s += kBlank[id].pay;
    }
    return s;
}

const char* Game::tapeLabel(int i) const {
    if (i < 0 || i >= kTapeN) return "";
    return kBlank[kTape[i]].name;
}

int Game::tapeScore(int i) const {
    if (i < 0 || i >= kTapeN) return 0;
    return kBlank[kTape[i]].pay;
}

int Game::phase() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Bed || mode_ == Mode::Stroke || mode_ == Mode::Clear) return 1;
    if (mode_ == Mode::Pocket) return 2;
    if (mode_ == Mode::Over && won_) return 3;
    return 4;
}

int Game::nextWant() const {
    for (int i = 0; i < kTapeN; i++)
        if (!held_[i]) return kTape[i];
    return -1;
}

const gs::Image& Game::blankImg(int id) const {
    if (id == 1) return art_.web;
    if (id == 2) return art_.cap;
    if (id == 3) return art_.flash;
    if (id == 4) return art_.shim;
    return art_.rib;
}

bool Game::prove() {
    int sum = 0;
    for (int i = 0; i < kTapeN; i++) {
        const Blank& b = kBlank[kTape[i]];
        if (!b.name || !b.name[0] || b.pay <= 0 || b.reject) {
            why_ = "TAPE";
            return false;
        }
        for (int j = 0; j < i; j++)
            if (std::strcmp(kBlank[kTape[i]].name, kBlank[kTape[j]].name) == 0) {
                why_ = "DUP";
                return false;
            }
        sum += b.pay;
    }
    if (sum != 18) {
        why_ = "SUM";
        return false;
    }
    const Blank& flash = kBlank[3];
    const Blank& shim = kBlank[4];
    if (!flash.reject || flash.pay != kBlank[1].pay || std::strcmp(flash.name, kBlank[1].name) == 0) {
        why_ = "FLASH";
        return false;
    }
    if (!shim.reject || shim.pay == flash.pay || std::strcmp(shim.name, flash.name) == 0) {
        why_ = "SHIM";
        return false;
    }
    int seen[kBlanks] = {};
    for (int i = 0; i < kDeckN; i++) {
        int id = kDeck[i];
        if (id < 0 || id >= kBlanks) {
            why_ = "DECK";
            return false;
        }
        seen[id]++;
    }
    if (seen[0] != 1 || seen[1] != 1 || seen[2] != 1 || seen[3] != 1 || seen[4] != 2) {
        why_ = "DECK";
        return false;
    }
    why_ = "OPEN";
    return true;
}

void Game::begin() {
    filled_ = faults_ = strokes_ = offer_ = age_ = anim_ = 0;
    over_ = won_ = left_ = holding_ = false;
    for (int i = 0; i < kTapeN; i++) {
        held_[i] = false;
        drawer_[i] = -1;
    }
    why_ = rules_ ? "OPEN" : why_;
    sys_->apu.silence();
    if (rules_) feed();
    else refuse("RULES");
}

void Game::feed() {
    if (offer_ >= kDeckN) {
        refuse("SHORT");
        return;
    }
    age_ = anim_ = 0;
    holding_ = false;
    mode_ = Mode::Bed;
    why_ = "BED";
}

void Game::stamp() {
    int id = kDeck[offer_];
    int slot = filled_;
    drawer_[slot] = id;
    held_[slot] = true;
    filled_++;
    strokes_++;
    offer_++;
    anim_ = 0;
    holding_ = false;
    mode_ = Mode::Clear;
    why_ = "STAMPED";
    sys_->apu.keyOn(0, 180.f + slot * 30.f, 0.26f);
    sys_->apu.noiseBurst(0.16f, 90.f, 0.1f);
}

void Game::refuse(const char* why) {
    why_ = why;
    won_ = false;
    left_ = false;
    over_ = true;
    holding_ = false;
    mode_ = Mode::Lose;
    anim_ = 0;
    faults_++;
    sys_->apu.noiseBurst(0.22f, 40.f, 0.16f);
}

void Game::passBlank() {
    int id = kDeck[offer_];
    if (kBlank[id].reject || id != nextWant()) {
        offer_++;
        anim_ = 0;
        holding_ = false;
        mode_ = Mode::Clear;
        why_ = "CLEARED";
        sys_->apu.noiseBurst(0.08f, 70.f, 0.05f);
        return;
    }
    refuse("THE BLANK LEFT");
}

void Game::depart() {
    if (!matched() || faults_ != 0 || drawerScore() != 18 || strokes_ != kTapeN) {
        refuse("DRAWER");
        return;
    }
    left_ = true;
    won_ = true;
    over_ = true;
    mode_ = Mode::Over;
    why_ = "the drawer matches the tape";
    anim_ = 0;
    sys_->apu.keyOn(0, 330.f, 0.28f);
    sys_->apu.keyOn(1, 440.f, 0.2f);
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
    uint16_t top = gs::rgb4(2, 2, 3);
    uint16_t mid = gs::rgb4(5, 6, 7);
    uint16_t bot = gs::rgb4(2, 3, 3);
    if (mode_ == Mode::Over && won_) mid = gs::rgb4(4, 8, 6);
    if (mode_ == Mode::Lose) mid = gs::rgb4(7, 2, 2);
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

    const float px = 168.f;
    const float bedY = 156.f;
    spr(art_.frame, px, 108.f, PAL_STEEL);
    spr(art_.bed, px, bedY, PAL_OIL);

    int show = -1;
    if ((mode_ == Mode::Bed || mode_ == Mode::Stroke) && offer_ < kDeckN) show = kDeck[offer_];
    if (show >= 0) spr(blankImg(show), px, bedY - 14.f, kBlank[show].pal);

    float ramY = 62.f;
    if (mode_ == Mode::Stroke) ramY = 62.f + (anim_ / float(kStroke)) * 62.f;
    else if (mode_ == Mode::Clear) ramY = 62.f + (1.f - anim_ / 20.f) * 20.f;
    if (ramY < 62.f) ramY = 62.f;
    spr(art_.ram, px, ramY, PAL_STEEL);

    for (int i = 0; i < kTapeN; i++) {
        float dx = 36.f + i * 52.f;
        spr(art_.drawer, dx, 200.f, held_[i] ? PAL_GOLD : PAL_DIM);
        if (held_[i]) spr(blankImg(drawer_[i]), dx, 194.f, kBlank[drawer_[i]].pal);
    }

    bool open = mode_ == Mode::Pocket || (mode_ == Mode::Over && won_);
    spr(art_.door, 292.f, 168.f, open ? PAL_DOOR : PAL_DIM);

    char buf[80];
    int f = int(sys_->frame);
    if (mode_ == Mode::Title) {
        hudC(2, "S3 PRESSTAPE", PAL_GOLD);
        hudC(17, "PLAY THE PRESS", PAL_HUD);
        hudC(19, "UNTIL THE DRAWER", PAL_HUD);
        hudC(20, "HAS TO MATCH THE TAPE", PAL_GOLD);
        hudC(22, "RIB   WEB   CAP", PAL_HUD);
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
        hudC(2, "THE PRESS STOPPED", PAL_BAD);
        hudC(22, why_, PAL_HUD);
        if ((f & 16) == 0) hudC(26, "START", PAL_GOLD);
    } else if (mode_ == Mode::Pocket) {
        hudC(2, "DRAWER MATCHES", PAL_GOLD);
        hudC(20, "THE TAPE IS IN THE DRAWER", PAL_HUD);
        hudC(22, "LEAVE", PAL_GOLD);
        if (!bot_ && (f & 16) == 0) hudC(26, "START LEAVES", PAL_DIM);
    } else {
        std::snprintf(buf, sizeof buf, "TAPE %s %s %s", tapeLabel(0), tapeLabel(1), tapeLabel(2));
        hud(1, 1, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "TILL %d", drawerScore());
        hud(30, 1, buf, PAL_HUD);
        if (mode_ == Mode::Bed && offer_ < kDeckN) {
            int id = kDeck[offer_];
            std::snprintf(buf, sizeof buf, "%s %d", kBlank[id].name, kBlank[id].pay);
            hudC(21, buf, kBlank[id].reject ? PAL_BAD : PAL_GOLD);
            hudC(23, kBlank[id].reject ? "LET IT CLEAR" : "HOLD THE PRESS", PAL_DIM);
            int mark = age_ * 16 / kDwell;
            if (mark > 15) mark = 15;
            char bar[20];
            for (int i = 0; i < 16; i++) bar[i] = (i == mark) ? '|' : '-';
            bar[16] = 0;
            hudC(25, bar, PAL_GOLD);
        } else if (mode_ == Mode::Stroke) {
            hudC(23, "FULL STROKE", PAL_GOLD);
        } else if (mode_ == Mode::Clear) {
            hudC(23, why_, PAL_DIM);
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
    } else if (mode_ == Mode::Bed) {
        int id = kDeck[offer_];
        bool want = !kBlank[id].reject && id == nextWant();
        bool down = false;
        if (bot_) {
            down = want && age_ >= 10;
        } else {
            down = pad.down(gs::BTN_A) || pad.down(gs::BTN_C);
        }
        if (down && !holding_) {
            holding_ = true;
            if (!want) {
                refuse(kBlank[id].reject ? "SCRAP STAYS OUT" : "OUT OF ORDER");
            } else {
                mode_ = Mode::Stroke;
                anim_ = 0;
                why_ = "PRESS";
                sys_->apu.keyOn(1, 90.f, 0.18f);
            }
        } else if (!down) {
            holding_ = false;
            if (++age_ >= kDwell) passBlank();
        } else if (++age_ >= kDwell) {
            passBlank();
        }
    } else if (mode_ == Mode::Stroke) {
        bool down = bot_ || pad.down(gs::BTN_A) || pad.down(gs::BTN_C);
        if (!down) {
            refuse("SHORT STROKE");
        } else if (++anim_ >= kStroke) {
            stamp();
        }
    } else if (mode_ == Mode::Clear) {
        anim_++;
        if (anim_ >= 16) {
            if (matched()) {
                mode_ = Mode::Pocket;
                anim_ = 0;
                why_ = "FULL";
            } else {
                feed();
            }
        }
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

}  // namespace presstape
