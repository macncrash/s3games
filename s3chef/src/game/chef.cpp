#include "game/chef.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace chef {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float READY = 0.60f;
constexpr float TARGET = 0.78f;
constexpr float DROP_GAP = 1.10f;
constexpr float PX[PANS] = {64.f, 160.f, 256.f};
constexpr float TICKET_Y = 46.f;
constexpr float TICKET_H = 50.f;
constexpr float BAR_Y = 78.f;
constexpr float BAR_W = 52.f;
constexpr float DISH_Y = 118.f;
constexpr float DISH_H = 36.f;
constexpr float PAN_Y = 142.f;
constexpr float PAN_H = 28.f;
constexpr float FLAME_Y = 160.f;
constexpr float FLAME_H = 18.f;
constexpr float CHEF_Y = 196.f;
constexpr float CHEF_H = 48.f;
constexpr float LAMP_Y = 26.f;
constexpr float LAMP_H = 22.f;

struct Recipe {
    const char* name;
    float cook;
};

constexpr Recipe MENU[TICKETS] = {
    {"BURGER", 7.2f}, {"TROUT", 7.8f}, {"STEAK", 5.8f}, {"OMELET", 7.2f}, {"BISQUE", 8.4f},
    {"RIBS", 6.0f},   {"TACO", 6.8f},  {"NOODLE", 7.6f}, {"PIE", 5.6f},    {"CHOPS", 6.6f},
};

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 1, 1));
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    chefX_ = PX[1];
    if (bot_) beginService();
    else mode_ = Mode::Title;
}

void Game::beginService() {
    score_ = 0;
    plated_ = 0;
    streak_ = 0;
    next_ = 0;
    sel_ = 1;
    burned_ = -1;
    holdDir_ = 0;
    hold_ = 0;
    dropCd_ = 0;
    t_ = 0;
    reach_ = 0;
    shake_ = 0;
    fanT_ = 0;
    chefX_ = PX[1];
    won_ = false;
    over_ = false;
    fail_[0] = 0;
    pops_.clear();
    for (int i = 0; i < TICKETS; i++) done_[i] = false;
    for (int i = 0; i < PANS; i++) pans_[i] = {};
    mode_ = Mode::Play;
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    burned_ = -1;
    sel_ = 1;
    shake_ = 0;
    fail_[0] = 0;
    pops_.clear();
    for (int i = 0; i < PANS; i++) pans_[i] = {};
}

void Game::nudge(int dir) {
    int n = std::clamp(sel_ + dir, 0, PANS - 1);
    if (n == sel_) return;
    sel_ = n;
    chime(0, 720.f, 0.035f, 0.03f);
}

void Game::humanAct(float dt) {
    const gs::Pad& pad = sys_->pad;
    int dir = 0;
    if (pad.down(gs::BTN_LEFT)) dir = -1;
    else if (pad.down(gs::BTN_RIGHT)) dir = 1;
    if (dir == 0) {
        if (pad.axisX < -0.45f) dir = -1;
        else if (pad.axisX > 0.45f) dir = 1;
    }
    if (dir == 0) {
        hold_ = 0;
        holdDir_ = 0;
    } else if (dir != holdDir_) {
        nudge(dir);
        holdDir_ = dir;
        hold_ = 0;
    } else {
        hold_ += dt;
        if (hold_ > 0.22f) {
            nudge(dir);
            hold_ = 0.12f;
        }
    }
    if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) plate(sel_);
}

void Game::botAct() {
    // Plate the ticket closest to burning, once it is in the gold.
    int urgent = -1;
    float worst = 1e9f;
    for (int i = 0; i < PANS; i++) {
        const Pan& p = pans_[i];
        if (p.ticket < 0 || p.heat < READY) continue;
        float ttl = (1.f - p.heat) * MENU[p.ticket].cook;
        if (ttl < worst) {
            worst = ttl;
            urgent = i;
        }
    }
    if (urgent < 0) return;
    if (pans_[urgent].heat < TARGET && worst > 0.70f) return;
    if (sel_ < urgent) sel_++;
    else if (sel_ > urgent) sel_--;
    else plate(urgent);
}

bool Game::plate(int pan) {
    if (pan < 0 || pan >= PANS) return false;
    Pan& p = pans_[pan];
    if (p.ticket < 0) {
        if (!bot_) chime(0, 180.f, 0.03f, 0.03f);
        return false;
    }
    if (p.heat < READY) {
        p.heat = std::min(READY + 0.04f, p.heat + 0.05f);
        streak_ = 0;
        pop(pan, "RAW", PAL_RED);
        chime(0, 150.f, 0.07f, 0.08f);
        return false;
    }
    float d = std::fabs(p.heat - 0.80f);
    float acc = 1.f - std::min(1.f, d / 0.20f);
    int pts = 450 + int(std::lround(550.f * acc));
    streak_++;
    pts += 40 * (streak_ - 1);
    score_ += pts;
    plated_++;
    done_[p.ticket] = true;
    char buf[16];
    std::snprintf(buf, sizeof buf, "+%d", pts);
    pop(pan, buf, PAL_GOLD);
    p = {};
    reach_ = 0.18f;
    chime(1, 988.f, 0.07f, 0.08f);
    sys_->apu.noiseBurst(0.16f, 2800.f, 0.035f);
    sys_->rumble(0.15f, 0.35f, 70);
    if (plated_ >= TICKETS) {
        mode_ = Mode::Win;
        won_ = true;
        over_ = true;
        fanT_ = 0;
    }
    return true;
}

void Game::tryDrop() {
    if (next_ >= TICKETS || dropCd_ > 0) return;
    int slot = -1;
    for (int i = 0; i < PANS; i++)
        if (pans_[i].ticket < 0) {
            slot = i;
            break;
        }
    if (slot < 0) return;
    pans_[slot].ticket = next_++;
    pans_[slot].heat = 0;
    pans_[slot].dinged = false;
    dropCd_ = DROP_GAP;
}

void Game::advance(float dt) {
    for (int i = 0; i < PANS; i++) {
        Pan& p = pans_[i];
        if (p.ticket < 0) continue;
        p.heat += dt / MENU[p.ticket].cook;
        if (!p.dinged && p.heat >= READY) {
            p.dinged = true;
            chime(2, 1174.f, 0.06f, 0.1f);
        }
        if (p.heat >= 1.f) {
            p.heat = 1.f;
            burned_ = i;
            mode_ = Mode::Lose;
            over_ = true;
            won_ = false;
            streak_ = 0;
            shake_ = 0.55f;
            std::snprintf(fail_, sizeof fail_, "burned %s", MENU[p.ticket].name);
            sys_->apu.noiseBurst(0.6f, 420.f, 0.4f);
            chime(0, 70.f, 0.12f, 0.45f);
            sys_->rumble(0.9f, 1.f, 220);
            return;
        }
    }
    if (dropCd_ > 0) dropCd_ -= dt;
    tryDrop();
}

void Game::pop(int pan, const char* text, int pal) {
    Pop p;
    p.t = 0.9f;
    p.row = 12;
    p.pal = pal;
    std::snprintf(p.text, sizeof p.text, "%s", text);
    p.col = int(std::lround(PX[pan] / 8.f)) - int(std::strlen(p.text)) / 2;
    pops_.push_back(p);
    if (pops_.size() > 6) pops_.erase(pops_.begin());
}

void Game::tickPops(float dt) {
    for (Pop& p : pops_) p.t -= dt;
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());
}

void Game::chime(int ch, float freq, float vol, float hold) {
    if (ch < 0 || ch > 2) return;
    sys_->apu.tone(ch, freq, vol);
    chime_[ch] = hold;
}

void Game::audio(float dt) {
    for (int i = 0; i < 2; i++) {
        if (chime_[i] > 0) {
            chime_[i] -= dt;
            if (chime_[i] <= 0) sys_->apu.tone(i, 0, 0);
        }
    }
    if (mode_ == Mode::Win) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f, 1318.f};
        int step = int(fanT_ / 0.11f);
        if (step < 6) sys_->apu.tone(2, notes[step], 0.075f);
        else sys_->apu.tone(2, 0, 0);
        fanT_ += dt;
    } else if (chime_[2] > 0) {
        chime_[2] -= dt;
        if (chime_[2] <= 0) sys_->apu.tone(2, 0, 0);
    }
    bool hot = false;
    if (mode_ == Mode::Play)
        for (int i = 0; i < PANS; i++)
            if (pans_[i].ticket >= 0 && pans_[i].heat > 0.04f) hot = true;
    float vol = hot ? 0.04f : (mode_ == Mode::Title ? 0.018f : 0.f);
    sys_->apu.noise(vol, hot ? 1900.f : 900.f, false);
}

void Game::lights() {
    bool hot = false, go = false;
    for (int i = 0; i < PANS; i++) {
        if (pans_[i].ticket < 0) continue;
        if (pans_[i].heat >= 0.90f) hot = true;
        else if (pans_[i].heat >= READY) go = true;
    }
    if (mode_ == Mode::Win) sys_->setLight(40, 180, 70);
    else if (mode_ == Mode::Lose || hot) sys_->setLight(190, 30, 20);
    else if (go) sys_->setLight(220, 160, 40);
    else sys_->setLight(170, 80, 40);
}

void Game::glide(float dt) {
    float goal = PX[std::clamp(sel_, 0, PANS - 1)];
    chefX_ += (goal - chefX_) * std::min(1.f, dt * 10.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += DT;
    if (reach_ > 0) reach_ -= DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        sel_ = 1;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) ||
                      pad.pressed(gs::BTN_TURBO))) {
            chime(1, 880.f, 0.06f, 0.06f);
            beginService();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) mode_ = Mode::Pause;
        else {
            if (bot_) botAct();
            else humanAct(DT);
            if (mode_ == Mode::Play) advance(DT);
            t_ += DT;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) beginService();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) toTitle();
    }

    glide(DT);
    tickPops(DT);
    audio(DT);
    lights();
    draw();
}

float Game::demoHeat(int i) const {
    float s = 0.5f + 0.5f * std::sin(anim_ * 0.85f + float(i) * 1.7f);
    return 0.28f + 0.55f * s;
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.solid.lv[0];
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::bar(float x, float y, float w, float heat) {
    float fw = w * std::clamp(heat, 0.f, 1.f);
    if (fw >= 1.f) {
        int pal = PAL_RAW;
        if (heat >= 0.90f) pal = ((int(anim_ * 12.f) & 1) ? PAL_RED : PAL_HOT);
        else if (heat >= READY) pal = PAL_OK;
        solid(x, y, fw, 6, pal);
    }
    solid(x + w * READY, y, w * (1.f - READY), 6, PAL_ZONE);
    solid(x, y, w, 6, PAL_TRACK);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool lose = mode_ == Mode::Lose;
    v.setFogColor(lose ? gs::rgb4(10, 1, 0) : gs::rgb4(2, 1, 1));
    uint8_t fog = lose ? 4 : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 16) c = (y > 7 && y < 13) ? gs::rgb4(10, 3, 2) : gs::rgb4(4, 1, 1);
        else if (y < 120) c = gs::rgb4(14, 11, 9);
        else if (y < 168) c = gs::rgb4(9, 10, 11);
        else c = gs::rgb4(5, 3, 2);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = fog;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float ox = 0;
    if (shake_ > 0) ox = std::sin(anim_ * 90.f) * 3.f * shake_;
    const bool service = mode_ != Mode::Title;
    const int flame = int(anim_ * 9.f) % 3;
    const int demoDish[PANS] = {0, 1, 4};

    if (mode_ == Mode::Title) {
        spr(art_.word[WORD_TITLE], 160, 24, float(art_.word[WORD_TITLE].h), PAL_HUD);
        spr(art_.word[WORD_SUB], 160, 48, float(art_.word[WORD_SUB].h), PAL_GOLD);
        for (int i = 0; i < TICKETS; i++) solid(115.f + float(i) * 9.f, 56, 7, 5, PAL_GOLD);
        solid(0, 4, 320, 64, PAL_TRACK);
        solid(0, 196, 320, 28, PAL_TRACK);
    } else if (mode_ == Mode::Win) {
        spr(art_.word[WORD_WIN], 160, 46, float(art_.word[WORD_WIN].h), PAL_GREEN);
        solid(40, 30, 240, 34, PAL_TRACK);
        for (int i = 0; i < 14; i++) {
            float ph = anim_ * (18.f + float(i % 5) * 7.f) + float(i) * 30.f;
            float x = std::fmod(ph * 3.f + float(i) * 22.f, 320.f);
            if (x < 0) x += 320.f;
            float y = std::fmod(ph, 170.f);
            if (y < 0) y += 170.f;
            int pal = (i % 3 == 0) ? PAL_OK : (i % 3 == 1) ? PAL_GREEN : PAL_HOT;
            solid(x, y, (i & 1) ? 4.f : 3.f, (i & 1) ? 3.f : 5.f, pal);
        }
    } else if (mode_ == Mode::Lose) {
        spr(art_.word[WORD_BURN], 160, 46, float(art_.word[WORD_BURN].h), PAL_RED);
        solid(50, 30, 220, 34, PAL_TRACK);
    }

    if (service && mode_ != Mode::Win) {
        int pal = PAL_GOLD;
        if (pans_[sel_].ticket >= 0 && pans_[sel_].heat >= READY) pal = (pans_[sel_].heat >= 0.90f) ? PAL_HOT : PAL_OK;
        spr(art_.bracket, PX[sel_] + ox, 128, 76, pal);
    }

    for (int i = 0; i < PANS; i++) {
        bool show = false;
        float heat = 0;
        if (!service) {
            show = true;
            heat = demoHeat(i);
        } else if (pans_[i].ticket >= 0) {
            show = true;
            heat = pans_[i].heat;
        }
        if (!show) continue;
        if (heat >= READY) {
            float bob = std::sin(anim_ * 9.f + float(i)) * 2.f;
            spr(art_.bell, PX[i] + 24 + ox, 86 + bob, 14, PAL_GOLD);
            spr(art_.steam, PX[i] - 8 + ox, 100 + bob, 14, PAL_HUD);
        }
    }

    float bob = std::sin(anim_ * 6.f) * 1.3f;
    if (mode_ == Mode::Win) bob = std::sin(anim_ * 12.f) * 3.f;
    int pose = (reach_ > 0 || (mode_ == Mode::Win && (int(anim_ * 3.f) & 1))) ? 1 : 0;
    spr(art_.chef[pose], chefX_ + ox, CHEF_Y + bob, CHEF_H, PAL_CHEF);
    spr(art_.bottle, 16, 154, float(art_.bottle.h), PAL_DECOR);
    spr(art_.shaker, 304, 156, float(art_.shaker.h), PAL_DECOR);

    for (int i = 0; i < PANS; i++) {
        bool show = false;
        int dish = demoDish[i];
        bool burnt = false;
        float lift = 0;
        if (!service) show = true;
        else if (pans_[i].ticket >= 0) {
            show = true;
            dish = pans_[i].ticket;
            burnt = mode_ == Mode::Lose && i == burned_;
            if (i == sel_) lift = -3.f;
            if (pans_[i].heat >= READY) lift -= 2.f + std::sin(anim_ * 8.f + float(i)) * 1.5f;
        }
        if (!show) continue;
        const gs::Mipped& pic = burnt ? art_.burnt : art_.dish[dish];
        spr(pic, PX[i] + ox, DISH_Y + lift, DISH_H, PAL_FOOD);
    }

    for (int i = 0; i < PANS; i++) {
        float heat = -1;
        if (!service) heat = demoHeat(i);
        else if (pans_[i].ticket >= 0) heat = pans_[i].heat;
        if (heat < 0) continue;
        bar(PX[i] - BAR_W * 0.5f + ox, BAR_Y, BAR_W, heat);
    }

    if (service) {
        solid(16, 16, 288, 4, PAL_TRACK);
        for (int i = 0; i < PANS; i++) {
            if (pans_[i].ticket < 0) continue;
            float lift = (i == sel_) ? -4.f : 0.f;
            if (pans_[i].heat >= READY) lift -= 2.f;
            int pal = pans_[i].heat >= READY ? PAL_READY : PAL_PAPER;
            spr(art_.ticket, PX[i] + ox, TICKET_Y + lift, TICKET_H, pal);
        }
        for (int i = 0; i < TICKETS; i++) {
            int pal = PAL_ZONE;
            if (done_[i]) pal = PAL_GREEN;
            else {
                for (int p = 0; p < PANS; p++)
                    if (pans_[p].ticket == i) pal = pans_[p].heat >= READY ? PAL_OK : PAL_RAW;
            }
            solid(115.f + float(i) * 9.f, 3, 7, 5, pal);
        }
    }

    for (int i = 0; i < PANS; i++) {
        float flicker = (i == (flame % PANS)) ? 1.f : 0.f;
        spr(art_.pan, PX[i] + ox, PAN_Y, PAN_H, PAL_STEEL);
        spr(art_.flame[flame % 3], PX[i] + ox, FLAME_Y - flicker, FLAME_H, PAL_FIRE);
        spr(art_.shade, PX[i] + ox, PAN_Y + 8, 10, PAL_HUD, false, true);
        spr(art_.lamp, PX[i], LAMP_Y, LAMP_H, PAL_FIRE);
    }
    spr(art_.shade, chefX_ + ox, CHEF_Y + 22, 12, PAL_HUD, false, true);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(25, "PLATE ALL TEN BEFORE THEY BURN", PAL_HUD);
        hudC(26, "ARROWS MOVE   Z PLATES", PAL_GOLD);
        if ((int(anim_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_GOLD);
    } else {
        hud(1, 0, "S3 CHEF", PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(40 - int(std::strlen(buf)), 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "PLATED %d/10", plated_);
        hud(1, 1, buf, plated_ >= TICKETS ? PAL_GREEN : PAL_HUD);
        if (streak_ >= 2) {
            std::snprintf(buf, sizeof buf, "STREAK %d", streak_);
            hud(40 - int(std::strlen(buf)), 1, buf, PAL_GOLD);
        }
        for (int i = 0; i < PANS; i++) {
            if (pans_[i].ticket < 0) continue;
            int id = pans_[i].ticket;
            int col = int(std::lround(PX[i] / 8.f));
            char num[4];
            std::snprintf(num, sizeof num, "%d", id + 1);
            hud(col - int(std::strlen(num)) / 2, 3, num, PAL_HUD);
            hud(col - int(std::strlen(MENU[id].name)) / 2, 5, MENU[id].name, PAL_TRACK);
            if (pans_[i].heat >= 0.90f) hud(col - 1, 12, "HOT", PAL_RED);
            else if (pans_[i].heat >= READY) hud(col - 1, 12, "GO", PAL_GREEN);
        }
        for (const Pop& p : pops_) hud(p.col, p.row, p.text, p.pal);
        if (mode_ == Mode::Play && plated_ == 0) hudC(27, "ARROWS MOVE   Z PLATES", PAL_HUD);
        else if (mode_ == Mode::Pause) {
            if ((int(anim_ * 2.f) & 1) == 0) hudC(14, "PAUSED", PAL_GOLD);
            hudC(27, "START RESUMES", PAL_HUD);
        } else if (mode_ == Mode::Win) {
            hudC(16, "TEN PLATED", PAL_GREEN);
            hudC(27, "START SERVES AGAIN", PAL_HUD);
        } else if (mode_ == Mode::Lose) {
            std::snprintf(buf, sizeof buf, "%s BURNED", burned_ >= 0 ? MENU[pans_[burned_].ticket].name : "TICKET");
            hudC(16, buf, PAL_RED);
            hudC(27, "START RETRIES", PAL_HUD);
        }
    }
}

}  // namespace chef
