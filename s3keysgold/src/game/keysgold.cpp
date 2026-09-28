#include "game/keysgold.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace keysgold {
namespace {

constexpr double kEighth = 0.22;
constexpr double kCount = 0.55;
constexpr double kWindow = 0.10;
constexpr double kApproach = 0.95;

// step, lane, gold (1/0), midi. Eight golds: 160 on the double, 80 bare.
// Seven golds are 140, still under the line. Cream is between them and does not score.
constexpr int kChart[] = {
    0, 0, 1, 60,  1, 2, 0, 67,  2, 1, 1, 64,  3, 3, 0, 72,
    4, 2, 1, 67,  5, 0, 0, 57,  6, 3, 1, 69,  7, 1, 0, 62,
    8, 0, 1, 64,  9, 2, 0, 67, 10, 1, 1, 65, 11, 3, 0, 71,
   12, 2, 1, 69, 13, 0, 0, 60, 14, 3, 1, 72,
};
constexpr int kChartN = int(sizeof(kChart) / sizeof(kChart[0]) / 4);

static_assert(kGoldsNeeded * kFace * 2 >= kLine, "gold double clears the line");
static_assert(kGoldsNeeded * kFace < kLine, "bare gold faces stay short");
static_assert((kGoldsNeeded - 1) * kFace * 2 < kLine, "one gold short of the double");

const gs::Button kBtn[kLanes] = {gs::BTN_LEFT, gs::BTN_DOWN, gs::BTN_UP, gs::BTN_RIGHT};
const gs::Button kAlt[kLanes] = {gs::BTN_A, gs::BTN_B, gs::BTN_C, gs::BTN_Y};

constexpr int countGold() {
    int n = 0;
    for (int i = 0; i < kChartN; i++)
        if (kChart[i * 4 + 2]) n++;
    return n;
}
static_assert(countGold() == kGoldsNeeded, "chart golds");

}  // namespace

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Image& img, float cx, float cy, float h, int pal) {
    if (h < 1.2f || img.h < 1) return;
    float w = h * float(img.w) / float(img.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::miss() {
    if (mode_ != Mode::Play) return;
    misses_++;
    sys_->apu.noiseBurst(0.14f, 240.f, 0.08f);
    if (misses_ >= kLamps) die();
}

void Game::die() {
    mode_ = Mode::Dead;
    won_ = false;
    over_ = true;
    sys_->apu.tone(0, 110.f, 0.2f);
    beep_ = 0.35f;
    if (!bot_) sys_->setLight(160, 30, 20);
}

void Game::win() {
    mode_ = Mode::Clear;
    won_ = leave_ && finisherGold_ && score_ >= kLine && bare_ < kLine && golds_ > 0 && cream_ == 0;
    over_ = true;
    if (!won_) {
        die();
        return;
    }
    sys_->apu.tone(0, 523.f, 0.12f);
    sys_->apu.tone(1, 659.f, 0.08f);
    sys_->apu.tone(2, 784.f, 0.07f);
    beep_ = 0.4f;
    if (!bot_) sys_->setLight(255, 190, 40);
}

void Game::startSong() {
    noteN_ = kChartN;
    chartOk_ = noteN_ > 0 && noteN_ <= 16;
    double prevLane[kLanes];
    for (int i = 0; i < kLanes; i++) prevLane[i] = -1000;
    int prevStep = -1;
    int golds = 0;
    for (int i = 0; i < noteN_; i++) {
        Note& n = notes_[i];
        n.time = kCount + kChart[i * 4] * kEighth;
        n.lane = kChart[i * 4 + 1];
        n.gold = kChart[i * 4 + 2] != 0;
        n.midi = kChart[i * 4 + 3];
        n.done = false;
        if (n.gold) golds++;
        if (kChart[i * 4] <= prevStep || n.lane < 0 || n.lane >= kLanes) chartOk_ = false;
        if (n.time - prevLane[n.lane] < kWindow * 2.0 + 1.0 / 60.0) chartOk_ = false;
        prevStep = kChart[i * 4];
        prevLane[n.lane] = n.time;
    }
    if (golds != kGoldsNeeded) chartOk_ = false;
    score_ = bare_ = golds_ = cream_ = hits_ = misses_ = 0;
    finisherGold_ = leave_ = false;
    songFrame_ = 0;
    t_ = 0;
    for (int i = 0; i < kLanes; i++) keyLit_[i] = 0;
    over_ = won_ = false;
    sys_->apu.silence();
    if (!chartOk_) {
        std::fprintf(stderr, "s3keysgold: chart windows overlap\n");
        mode_ = Mode::Dead;
        over_ = true;
        won_ = false;
        return;
    }
    mode_ = Mode::Play;
}

void Game::updatePlay() {
    const double t = t_;
    bool press[kLanes] = {};
    if (bot_) {
        for (int i = 0; i < noteN_; i++) {
            const Note& n = notes_[i];
            if (n.done || !n.gold) continue;
            double err = t - n.time;
            if (err >= -1.0 / 60.0 && err <= kWindow) press[n.lane] = true;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        for (int i = 0; i < kLanes; i++) press[i] = pad.pressed(kBtn[i]) || pad.pressed(kAlt[i]);
    }
    for (int i = 0; i < kLanes; i++)
        if (press[i]) keyLit_[i] = 0.12f;

    int best[kLanes];
    double bestAbs[kLanes];
    for (int i = 0; i < kLanes; i++) {
        best[i] = -1;
        bestAbs[i] = 1e9;
    }
    for (int i = 0; i < noteN_; i++) {
        if (notes_[i].done) continue;
        double a = std::fabs(t - notes_[i].time);
        int lane = notes_[i].lane;
        if (a <= kWindow && a < bestAbs[lane]) {
            bestAbs[lane] = a;
            best[lane] = i;
        }
    }

    for (int i = 0; i < noteN_ && mode_ == Mode::Play; i++) {
        if (notes_[i].done) continue;
        if (t - notes_[i].time <= kWindow) continue;
        notes_[i].done = true;
        if (notes_[i].gold) miss();
    }

    if (mode_ == Mode::Play) {
        for (int lane = 0; lane < kLanes; lane++) {
            if (!press[lane] || best[lane] < 0) continue;
            Note& n = notes_[best[lane]];
            if (n.done) continue;
            n.done = true;
            if (n.gold) {
                hits_++;
                golds_++;
                score_ += kFace * 2;
                bare_ += kFace;
                sys_->apu.tone(2, float(440.0 * std::pow(2.0, (n.midi - 69) / 12.0)), 0.16f);
                beep_ = 0.12f;
                if (score_ >= kLine && bare_ < kLine) {
                    leave_ = true;
                    finisherGold_ = true;
                }
            } else {
                // Only the gold counts double. Cream cannot buy the line.
                cream_++;
                sys_->apu.noiseBurst(0.12f, 500.f, 0.06f);
            }
        }
    }

    if (mode_ == Mode::Play && leave_) win();
    else if (mode_ == Mode::Play) {
        bool pendingGold = false;
        for (int i = 0; i < noteN_; i++)
            if (!notes_[i].done && notes_[i].gold) pendingGold = true;
        if (!pendingGold && t >= notes_[noteN_ - 1].time + 0.4) {
            won_ = false;
            over_ = true;
            mode_ = Mode::Dead;
        }
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int shade = 1 + y / 40;
        v.lineBackdrop[y] = gs::rgb4(shade, shade, shade + 2);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    if (mode_ == Mode::Clear) {
        for (int y = 0; y < gs::SCREEN_H; y++) v.lineBackdrop[y] = gs::rgb4(3, 2, 1);
    } else if (mode_ == Mode::Dead) {
        for (int y = 0; y < gs::SCREEN_H; y++) v.lineBackdrop[y] = gs::rgb4(2, 0, 1);
    }

    for (int lane = 0; lane < kLanes; lane++) {
        spr(art_.rail.lv[0], float(kLaneX[lane]), 70, 8, PAL_WOOD);
        spr(art_.rail.lv[0], float(kLaneX[lane]), 110, 8, PAL_WOOD);
    }
    for (int lane = 0; lane < kLanes; lane++)
        spr(art_.rail.lv[0], float(kLaneX[lane]), float(kHitY), 10, keyLit_[lane] > 0 ? PAL_GOLD : PAL_LINE);

    if (mode_ == Mode::Title) {
        spr(art_.gold.lv[0], 160, 78, 36, PAL_GOLD);
    } else if (mode_ == Mode::Play || mode_ == Mode::Clear) {
        for (int i = 0; i < noteN_; i++) {
            const Note& n = notes_[i];
            if (n.done) continue;
            float lead = float(n.time - t_);
            float y = float(kHitY) - lead / float(kApproach) * kTravel;
            if (y < -24 || y > 210) continue;
            const gs::Image& img = n.gold ? art_.gold.lv[0] : art_.cream.lv[0];
            spr(img, float(kLaneX[n.lane]), y, n.gold ? 24 : 18, n.gold ? PAL_GOLD : PAL_CREAM);
        }
    }

    for (int lane = 0; lane < kLanes; lane++) {
        float y = 186.f + (keyLit_[lane] > 0 ? 2.f : 0.f);
        spr(art_.key[lane].lv[0], float(kLaneX[lane]), y, 28, keyLit_[lane] > 0 ? PAL_KEYLIT : PAL_KEY);
    }
    for (int i = 0; i < kLamps; i++) {
        bool lit = mode_ != Mode::Dead && i >= misses_;
        spr(art_.lamp.lv[0], 136.f + i * 16.f, 14, 14, lit ? PAL_GOLD : PAL_BAD);
    }

    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 KEYS GOLD", PAL_GOLD);
        hudC(12, "A SHORT KEYS", PAL_WHITE);
        hudC(14, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        hudC(16, "CREAM DOES NOT BUY THE LINE", PAL_CREAM);
        hudC(18, "ARROWS OR Z X C V", PAL_WHITE);
        if ((int(clock_ * 2) & 1) == 0) hudC(22, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 27, ver, PAL_HUD);
    } else if (mode_ == Mode::Play) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BARE %d", bare_);
        hud(14, 0, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "LINE %d", kLine);
        hud(28, 0, buf, PAL_HUD);
        hudC(26, "HIT GOLD  LET CREAM PASS", PAL_WHITE);
    } else if (mode_ == Mode::Clear) {
        hudC(8, "ONLY THE GOLD COUNTS DOUBLE", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(12, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(14, buf, PAL_GOLD);
        if ((int(clock_ * 2) & 1) == 0) hudC(20, "START", PAL_GOLD);
    } else {
        hudC(8, "THE DOUBLE WAS SHORT", PAL_BAD);
        std::snprintf(buf, sizeof buf, "SCORE %d  BARE %d", score_, bare_);
        hudC(12, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "GOLDS %d  CREAM %d", golds_, cream_);
        hudC(14, buf, PAL_CREAM);
        if ((int(clock_ * 2) & 1) == 0) hudC(20, "START", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    mode_ = Mode::Title;
    clock_ = 0;
    if (bot_) startSong();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += 1.0 / 60.0;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) startSong();
    } else if (mode_ == Mode::Play) {
        t_ = songFrame_ / 60.0;
        updatePlay();
        songFrame_++;
    } else if (!bot_ && (mode_ == Mode::Dead || mode_ == Mode::Clear)) {
        if (pad.pressed(gs::BTN_START)) startSong();
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            sys.apu.silence();
        }
    }

    if (beep_ > 0) {
        beep_ -= float(1.0 / 60.0);
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    for (int i = 0; i < kLanes; i++)
        if (keyLit_[i] > 0) keyLit_[i] = std::max(0.f, keyLit_[i] - float(1.0 / 60.0));
    draw();
}

}  // namespace keysgold
