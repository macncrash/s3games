#include "game/lantern.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace lantern {
namespace {

constexpr int LAMPS = 6;
constexpr int GOAL = 8;
constexpr int LEAD = 20;
constexpr int SHOW = 24;
constexpr int GAP = 14;
constexpr int GROW = 26;
constexpr int HOT = 12;
constexpr int LOCK = 10;
constexpr int MISS_T = 46;
constexpr int SNUFF = 22;
constexpr int WIN_T = 64;
constexpr int BOT_AT = 24;
constexpr float LAMP_H = 54.f;
constexpr float BEAM_Y = 48.f;

const float kX[LAMPS] = {30, 82, 134, 186, 238, 290};
const float kDrop[LAMPS] = {0, 7, 12, 12, 7, 0};
const float kPitch[LAMPS] = {349.23f, 392.00f, 440.00f, 493.88f, 523.25f, 587.33f};
const int kPal[LAMPS] = {PAL_L0, PAL_L1, PAL_L2, PAL_L3, PAL_L4, PAL_L5};
const float kFan[6] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f, 1046.5f};

struct Star {
    int x, y, t;
};
const Star kStars[] = {{10, 10, 0},  {26, 26, 3},  {48, 8, 1},   {64, 18, 5}, {258, 8, 2}, {276, 16, 4},
                       {300, 8, 6},  {308, 22, 1}, {12, 40, 2},  {292, 34, 7}, {150, 6, 3}, {188, 5, 5},
                       {214, 8, 1},  {96, 6, 4}};

const int kWinX[5] = {33, 97, 161, 225, 289};
const int kWinY = 135;

uint16_t mix(uint16_t a, uint16_t b, float t) {
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto L = [&](int x, int y) { return int(std::lround(x + (y - x) * t)); };
    return gs::rgb4(L(ar, br), L(ag, bg), L(ab, bb));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Watch) return 1;
    if (mode_ == Mode::Play) return 2;
    if (mode_ == Mode::Win) return 4;
    return 3;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    mode_ = Mode::Title;
    phase_ = Phase::Lead;
    over_ = false;
    won_ = false;
    hotBad_ = false;
    cursor_ = 0;
    handed_ = 0;
    run_ = 1;
    step_ = 0;
    show_ = 0;
    timer_ = 0;
    lock_ = 0;
    hold_ = 0;
    hot_ = -1;
    hotT_ = 0;
    pending_ = -1;
    toneT_ = 0;
    fanStep_ = 0;
    fanT_ = 0;
    mothX_ = 36;
    mothY_ = 70;
    faceLeft_ = false;
    rng_ = 1;
    seq_.clear();
    for (int i = 0; i < LAMPS; i++) wave_[i] = false;
    sys.vdp.A.enabled = true;
    sys.vdp.B.enabled = true;
}

int Game::roll() {
    int n = 0;
    do {
        rng_ = rng_ * 1664525u + 1013904223u;
        n = int((rng_ >> 16) % LAMPS);
    } while (!seq_.empty() && n == seq_.back());
    return n;
}

void Game::quiet() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    toneT_ = 0;
}

void Game::chime(int lamp, float vol) {
    if (lamp < 0 || lamp >= LAMPS) return;
    float f = kPitch[lamp];
    sys_->apu.tone(0, f, vol);
    sys_->apu.tone(1, f * 2.f, vol * 0.35f);
    toneT_ = 14;
    static const int R[6] = {255, 255, 80, 90, 190, 255};
    static const int G[6] = {200, 80, 220, 130, 80, 150};
    static const int B[6] = {40, 80, 90, 255, 255, 30};
    sys_->setLight(R[lamp], G[lamp], B[lamp]);
}

void Game::thud() {
    sys_->apu.tone(0, 98.f, 0.09f);
    sys_->apu.tone(1, 73.f, 0.05f);
    sys_->apu.noiseBurst(0.42f, 680.f, 0.22f);
    toneT_ = 16;
    sys_->setLight(80, 0, 0);
    sys_->rumble(0.55f, 0.25f, 100);
}

void Game::begin() {
    rng_ = 0xA11E4E01u ^ uint32_t(run_) * 0x85EBCA6Bu;
    run_++;
    seq_.clear();
    seq_.push_back(roll());
    handed_ = 0;
    won_ = false;
    over_ = false;
    pending_ = -1;
    cursor_ = 0;
    hot_ = -1;
    hotT_ = 0;
    hotBad_ = false;
    step_ = 0;
    fanStep_ = 0;
    fanT_ = 0;
    quiet();
    enterWatch();
}

void Game::enterWatch() {
    mode_ = Mode::Watch;
    phase_ = Phase::Lead;
    show_ = 0;
    timer_ = LEAD;
    pending_ = -1;
    lock_ = 0;
}

void Game::enterPlay() {
    mode_ = Mode::Play;
    step_ = 0;
    lock_ = 8;
    pending_ = -1;
}

void Game::enterGrow() {
    seq_.push_back(roll());
    mode_ = Mode::Grow;
    timer_ = GROW;
    pending_ = -1;
}

void Game::enterWin() {
    mode_ = Mode::Win;
    timer_ = WIN_T;
    won_ = true;
    fanStep_ = 0;
    fanT_ = 0;
    pending_ = -1;
    sys_->rumble(0.25f, 0.55f, 180);
    sys_->setLight(255, 180, 60);
}

void Game::enterMiss() {
    mode_ = Mode::Miss;
    timer_ = MISS_T;
    handed_++;
    hotBad_ = true;
    hotT_ = 22;
    pending_ = -1;
    lock_ = 0;
    thud();
}

void Game::nudge(int dir) {
    cursor_ = (cursor_ + dir + LAMPS) % LAMPS;
}

void Game::light(int lamp) {
    if (lamp < 0 || lamp >= LAMPS) return;
    if (mode_ == Mode::Title) {
        hot_ = lamp;
        hotT_ = HOT;
        hotBad_ = false;
        chime(lamp, 0.05f);
        return;
    }
    if (mode_ != Mode::Play) return;
    if (lock_ > 0 || hotT_ > 0) {
        pending_ = lamp;
        return;
    }
    if (step_ < 0 || step_ >= (int)seq_.size()) return;
    hot_ = lamp;
    hotT_ = HOT;
    hotBad_ = false;
    if (lamp == seq_[step_]) {
        chime(lamp, 0.08f);
        sys_->rumble(0.f, 0.16f, 28);
        step_++;
        if (step_ >= (int)seq_.size()) {
            if ((int)seq_.size() >= GOAL) enterWin();
            else enterGrow();
        } else {
            lock_ = LOCK;
        }
    } else {
        enterMiss();
    }
}

void Game::readHuman() {
    gs::Pad& p = sys_->pad;
    if (mode_ == Mode::Title || mode_ == Mode::Play) {
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
            } else if (++hold_ >= 16 && hold_ % 5 == 0) {
                nudge(dir);
            }
        }
    }
    if ((mode_ == Mode::Title || mode_ == Mode::Win) && p.pressed(gs::BTN_START)) {
        begin();
        return;
    }
    if (mode_ == Mode::Win && p.pressed(gs::BTN_TURBO)) {
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

void Game::updateTitle() {
    if (bot_) {
        if (sys_->frame >= BOT_AT) begin();
        return;
    }
    readHuman();
    for (int i = 0; i < LAMPS; i++) {
        float w = std::sin((float(sys_->frame) + i * 22) * 0.05f);
        bool on = w > 0.25f;
        if (on && !wave_[i]) chime(i, 0.028f);
        wave_[i] = on;
    }
}

void Game::updateWatch() {
    if (timer_ > 0) timer_--;
    if (timer_ > 0) return;
    if (phase_ == Phase::Lead) {
        phase_ = Phase::Show;
        show_ = 0;
        timer_ = SHOW;
        if (!seq_.empty()) chime(seq_[0], 0.08f);
    } else if (phase_ == Phase::Show) {
        phase_ = Phase::Gap;
        timer_ = GAP;
        quiet();
    } else {
        show_++;
        if (show_ >= (int)seq_.size()) enterPlay();
        else {
            phase_ = Phase::Show;
            timer_ = SHOW;
            chime(seq_[show_], 0.08f);
        }
    }
}

void Game::updatePlay() {
    if (!bot_) readHuman();
    else sys_->typed.clear();
    if (mode_ != Mode::Play) return;
    if (lock_ == 0 && hotT_ == 0 && pending_ >= 0) {
        int p = pending_;
        pending_ = -1;
        light(p);
        return;
    }
    if (bot_ && lock_ == 0 && hotT_ == 0 && step_ < (int)seq_.size()) light(seq_[step_]);
}

void Game::updateMiss() {
    if (timer_ == SNUFF) {
        sys_->apu.noiseBurst(0.18f, 380.f, 0.14f);
        sys_->apu.tone(0, 160.f, 0.04f);
        toneT_ = 8;
    }
    if (timer_ > 0) timer_--;
    if (timer_ > 0) return;
    // The miss gives the newest lamp back. One lamp is the floor: it is dealt again.
    if ((int)seq_.size() > 1) seq_.pop_back();
    enterWatch();
}

void Game::updateGrow() {
    if (timer_ > 0) timer_--;
    if (timer_ > 0) return;
    enterWatch();
}

void Game::updateWin() {
    sys_->typed.clear();
    if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_TURBO))) {
        begin();
        return;
    }
    if (fanT_ > 0) fanT_--;
    if (fanT_ == 0 && fanStep_ < 6) {
        float f = kFan[fanStep_++];
        sys_->apu.tone(0, f, 0.08f);
        sys_->apu.tone(1, f * 0.5f, 0.04f);
        toneT_ = 10;
        fanT_ = 9;
        sys_->setLight(255, 200, 80);
    }
    if (timer_ > 0) timer_--;
    if (timer_ == 0) over_ = true;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (hotT_ > 0) hotT_--;
    if (lock_ > 0 && mode_ == Mode::Play) lock_--;
    switch (mode_) {
        case Mode::Title: updateTitle(); break;
        case Mode::Watch: updateWatch(); break;
        case Mode::Play: updatePlay(); break;
        case Mode::Miss: updateMiss(); break;
        case Mode::Grow: updateGrow(); break;
        case Mode::Win: updateWin(); break;
    }
    if (toneT_ > 0 && --toneT_ == 0) {
        sys.apu.tone(0, 0, 0);
        sys.apu.tone(1, 0, 0);
    }
    draw();
}

bool Game::lampOn(int i, int& pal) const {
    if (hotT_ > 0 && hot_ == i) {
        pal = hotBad_ ? PAL_NO : kPal[i];
        return true;
    }
    if (mode_ == Mode::Title) {
        float w = std::sin((float(sys_->frame) + i * 22) * 0.05f);
        if (w > 0.25f) {
            pal = kPal[i];
            return true;
        }
        return false;
    }
    if (mode_ == Mode::Win) {
        pal = kPal[i];
        return true;
    }
    if (mode_ == Mode::Watch && phase_ == Phase::Show && show_ >= 0 && show_ < (int)seq_.size() && seq_[show_] == i) {
        pal = kPal[i];
        return true;
    }
    if (mode_ == Mode::Miss && timer_ > SNUFF && !seq_.empty() && seq_.back() == i) {
        pal = kPal[i];
        return true;
    }
    return false;
}

void Game::sky() {
    const uint16_t top = gs::rgb4(1, 1, 5);
    const uint16_t mid = gs::rgb4(2, 3, 8);
    const uint16_t hor = gs::rgb4(7, 4, 7);
    const uint16_t ground = gs::rgb4(2, 2, 4);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t;
        uint16_t c;
        if (y < 90) {
            t = y / 90.f;
            c = mix(top, mid, t);
        } else if (y < 150) {
            t = (y - 90) / 60.f;
            c = mix(mid, hor, t);
        } else {
            t = (y - 150) / 74.f;
            c = mix(hor, ground, std::min(1.f, t));
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

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

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    const uint64_t fr = sys_->frame;
    int showWick = -1;
    if (mode_ == Mode::Title || mode_ == Mode::Play) showWick = cursor_;
    else if (mode_ == Mode::Watch && phase_ != Phase::Lead && !seq_.empty())
        showWick = seq_[std::min(show_, (int)seq_.size() - 1)];
    else if (mode_ == Mode::Miss && hot_ >= 0) showWick = hot_;

    float lx[LAMPS], cy[LAMPS], cordH[LAMPS];
    int onPal[LAMPS];
    bool on[LAMPS];
    for (int i = 0; i < LAMPS; i++) {
        float swing = std::sin((float(fr) + i * 19) * 0.04f);
        float lift = (hotT_ > 0 && hot_ == i) ? 3.f : 0.f;
        lx[i] = kX[i] + swing * 2.4f;
        float top = 50.f + kDrop[i] + std::fabs(swing) * 1.2f - lift;
        cy[i] = top + LAMP_H * 0.5f;
        cordH[i] = std::max(2.f, top - (BEAM_Y + 2.f));
        onPal[i] = PAL_DARK;
        on[i] = lampOn(i, onPal[i]);
    }
    // Earlier sprites sit on top of later ones.
    if (mode_ == Mode::Title) {
        const char* logo = "S3 LANTERN";
        float adv = 18.f;
        float x0 = 160.f - (10 * adv) * 0.5f;
        for (int i = 0; logo[i]; i++) {
            unsigned char c = static_cast<unsigned char>(logo[i]);
            if (c <= 32 || c >= 128) continue;
            const gs::Mipped& g = art_.glyph[c - 32];
            spr(g, x0 + i * adv + adv * 0.5f, 16.f, float(g.h), PAL_GOLD);
        }
    }
    if (showWick >= 0 && showWick < LAMPS) {
        float x = kX[showWick] + std::sin((float(fr) + showWick * 19) * 0.04f) * 2.4f;
        float bob = std::sin(float(fr) * 0.2f) * 1.5f;
        spr(art_.wick, x, 34.f + bob, 11, kPal[showWick]);
    }
    int follow = cursor_;
    if (mode_ == Mode::Watch && phase_ != Phase::Lead && !seq_.empty())
        follow = seq_[std::min(show_, (int)seq_.size() - 1)];
    else if (mode_ == Mode::Win) follow = int(fr / 18) % LAMPS;
    float tx = kX[std::clamp(follow, 0, LAMPS - 1)];
    float nx = mothX_ + (tx - mothX_) * 0.05f;
    faceLeft_ = nx < mothX_ - 0.2f;
    mothX_ = nx;
    mothY_ += (62.f - mothY_) * 0.05f;
    spr(art_.moth[int((fr / 5) & 1)], mothX_, mothY_, 14, PAL_SCENE, faceLeft_);
    for (int i = 0; i < 5; i++) {
        float speed = 0.32f + i * 0.05f;
        float x = std::fmod(16.f + i * 68.f + float(fr) * speed, 300.f) + 8.f;
        float y = 104.f + std::sin(float(fr) * 0.045f + i * 1.7f) * 12.f;
        float tw = 0.5f + 0.5f * std::sin(float(fr) * 0.22f + i * 2.f);
        if (tw < 0.35f) continue;
        spr(art_.fly, x, y, 3.f + tw * 2.f, kPal[i % LAMPS]);
    }
    for (int i = 0; i < LAMPS; i++)
        if (on[i]) spr(art_.flame[int((fr / 6 + i) & 1)], lx[i], cy[i] - 2.f, 16, onPal[i]);
    for (int i = 0; i < LAMPS; i++) spr(art_.lamp[i], lx[i], cy[i], LAMP_H, on[i] ? onPal[i] : PAL_DARK);
    for (int i = 0; i < LAMPS; i++) spr(art_.cord, lx[i], BEAM_Y + 2.f + cordH[i] * 0.5f, cordH[i], PAL_SCENE);
    for (int i = 0; i < LAMPS; i++)
        if (on[i]) spr(art_.glow, lx[i], cy[i] - 2.f, 78, onPal[i]);
    for (int i = 0; i < LAMPS; i++) spr(art_.shade, lx[i], cy[i] + LAMP_H * 0.42f, 10, PAL_DARK, false, true);
    for (int i = 0; i < 5; i++) {
        if (((fr + i * 13) % 50) < 7) continue;
        spr(art_.pane, float(kWinX[i]), float(kWinY), 10, PAL_SCENE);
    }
    spr(art_.post, 12, 96, 100, PAL_SCENE);
    spr(art_.post, 308, 96, 100, PAL_SCENE);
    spr(art_.beam, 160, BEAM_Y, 10, PAL_SCENE);
    spr(art_.moon, 274, 22, 34, PAL_SCENE);
    for (const Star& s : kStars) {
        if (((fr + s.t * 5) % 28) < 4) continue;
        spr(art_.star, float(s.x), float(s.y), 5, PAL_SCENE);
    }

    if (mode_ == Mode::Title) {
        hudC(19, "LIGHT THEM IN ORDER", PAL_HUD);
        hudC(20, "A MISS HANDS ONE BACK", PAL_HUD);
        hudC(22, "ARROWS MOVE THE WICK", PAL_DIM);
        hudC(23, "Z OR C LIGHTS IT", PAL_DIM);
        hudC(24, "1 TO 6 LIGHTS THAT LAMP", PAL_DIM);
        hudC(25, "ENTER BEGINS THE NIGHT", PAL_GOLD);
        hudC(27, S3_VERSION_STRING, PAL_DIM);
    } else {
        int n = (int)seq_.size();
        int pips = n;
        if (mode_ == Mode::Miss && timer_ <= SNUFF) pips = std::max(0, n - 1);
        char line[40];
        std::snprintf(line, sizeof line, "ORDER %d OF %d", std::min(n, GOAL), GOAL);
        hud(1, 0, line, PAL_HUD);
        std::snprintf(line, sizeof line, "BACK %d", handed_);
        hud(1, 1, line, mode_ == Mode::Miss ? PAL_ROSE : PAL_DIM);
        std::string marks(GOAL, '.');
        for (int i = 0; i < GOAL && i < pips; i++) marks[i] = '*';
        if (mode_ == Mode::Miss && timer_ > SNUFF && n > 0 && n <= GOAL) marks[n - 1] = '+';
        hud(28, 1, marks, mode_ == Mode::Win ? PAL_JADE : PAL_GOLD);

        const char* banner = "WATCH THE ORDER";
        int bpal = PAL_HUD;
        if (mode_ == Mode::Play) banner = "YOUR TURN";
        else if (mode_ == Mode::Miss) {
            banner = "HANDED BACK TO THE DARK";
            bpal = PAL_ROSE;
        } else if (mode_ == Mode::Grow) banner = "THE CHAIN HOLDS";
        else if (mode_ == Mode::Win) {
            banner = "THE NIGHT IS LIT";
            bpal = PAL_JADE;
        }
        hudC(25, banner, bpal);
        if (mode_ == Mode::Win) hudC(26, "ENTER PLAYS AGAIN", PAL_DIM);
        else if (mode_ == Mode::Play) hudC(26, "ARROWS  Z  OR  1-6", PAL_DIM);
    }
}

}  // namespace lantern
