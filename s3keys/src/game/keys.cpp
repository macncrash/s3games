#include "game/keys.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "version.h"

namespace keys {
namespace {

constexpr double kBpm = 96.0;
constexpr double kBeat = 60.0 / kBpm;
constexpr double kEighth = kBeat * 0.5;
constexpr double kCount = kBeat * 3.0;
constexpr double kWindow = 0.14;
constexpr double kApproach = 1.20;
constexpr double kPerfect = 0.05;

// Original tune in A minor. Step is an eighth-note index; the lane follows the pitch.
constexpr int kMel[] = {
    0, 57,  2, 60,  4, 64,  6, 69,
    8, 67, 10, 65, 12, 64, 14, 60,
   16, 65, 18, 69, 20, 67, 22, 64,
   24, 67, 26, 72, 28, 69, 30, 64,
   32, 62, 34, 65, 36, 69, 38, 65,
   40, 64, 42, 60, 44, 67, 46, 69,
   48, 71, 50, 67, 52, 64, 54, 62,
   56, 60, 58, 64, 60, 67, 62, 69,
   64, 65, 65, 69, 66, 72, 67, 69, 68, 67, 69, 64, 70, 60, 71, 57,
   72, 64, 74, 67, 76, 72, 78, 69,
   80, 67, 84, 64, 88, 60, 92, 57,
};
constexpr int kMelN = int(sizeof(kMel) / sizeof(kMel[0]) / 2);
constexpr int kRoot[12] = {45, 45, 41, 48, 50, 45, 43, 45, 41, 48, 45, 45};

int laneOf(int midi) {
    if (midi < 60) return 0;
    if (midi < 64) return 1;
    if (midi < 69) return 2;
    return 3;
}

float hz(int midi) { return float(440.0 * std::pow(2.0, (midi - 69) / 12.0)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch bassPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.35f;
    p.op[0] = {1, 1, 0.008f, 0.18f, 0.35f, 0.1f};
    p.op[1] = {1, 0.8f, 0.008f, 0.22f, 0.3f, 0.12f};
    p.op[2] = {0.5f, 0.7f, 0.01f, 0.3f, 0.4f, 0.14f};
    p.op[3] = {2, 0.2f, 0.008f, 0.16f, 0.1f, 0.08f};
    p.vol = 0.2f;
    p.tone = 900;
    p.echo = 0.08f;
    return p;
}

gs::FMPatch padPatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.1f;
    p.op[0] = {1, 0.6f, 0.04f, 0.4f, 0.7f, 0.3f};
    p.op[1] = {2, 0.25f, 0.05f, 0.4f, 0.5f, 0.3f};
    p.op[2] = {3, 0.15f, 0.06f, 0.5f, 0.4f, 0.3f};
    p.op[3] = {1, 0.3f, 0.05f, 0.4f, 0.6f, 0.3f};
    p.vol = 0.08f;
    p.tone = 1400;
    p.echo = 0.35f;
    return p;
}

gs::FMPatch pianoPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1, 1, 0.004f, 0.28f, 0.12f, 0.08f};
    p.op[1] = {2, 0.45f, 0.004f, 0.16f, 0.0f, 0.06f};
    p.op[2] = {3, 0.22f, 0.003f, 0.1f, 0.0f, 0.05f};
    p.op[3] = {1, 0.4f, 0.005f, 0.32f, 0.08f, 0.1f};
    p.vol = 0.24f;
    p.tone = 3200;
    p.echo = 0.22f;
    return p;
}

const gs::Button kBtn[kLanes] = {gs::BTN_LEFT, gs::BTN_DOWN, gs::BTN_UP, gs::BTN_RIGHT};
const gs::Button kAlt[kLanes] = {gs::BTN_A, gs::BTN_B, gs::BTN_C, gs::BTN_Y};
const int kNotePal[kLanes] = {PAL_N0, PAL_N1, PAL_N2, PAL_N3};

}  // namespace

void Game::noteOn(int ch, int midi, float vol) { sys_->apu.keyOn(ch, hz(midi), vol); }

void Game::click(bool accent) {
    sys_->apu.tone(0, accent ? 988.0f : 740.0f, accent ? 0.07f : 0.045f);
    clickLeft_ = 0.05;
}

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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal);
    }
}

void Game::miss(int lane) {
    if (mode_ != Mode::Play || misses_ >= kLamps) return;
    misses_++;
    combo_ = 0;
    if (pops_.size() < 12) pops_.push_back(Pop{float(kLaneX[lane]), float(kHitY - 20), clock_, 2});
    sys_->apu.noiseBurst(0.16f, 280.0f, 0.09f);
    if (!bot_) sys_->rumble(0.55f, 0.2f, 70);
    if (misses_ >= kLamps) die();
}

void Game::die() {
    mode_ = Mode::Dead;
    won_ = false;
    over_ = true;
    deadAt_ = clock_;
    sys_->apu.silence();
    sys_->apu.keyOn(2, 110.0f, 0.24f);
    sys_->apu.noiseBurst(0.45f, 90.0f, 0.45f);
    if (!bot_) sys_->setLight(180, 20, 20);
}

void Game::win() {
    mode_ = Mode::Clear;
    won_ = true;
    over_ = true;
    clearAt_ = clock_;
    fanStep_ = -1;
    sys_->apu.silence();
    if (!bot_) sys_->setLight(40, 180, 60);
}

void Game::startSong() {
    notes_.clear();
    notes_.reserve(kMelN);
    chartOk_ = true;
    int prevStep = -1;
    double prevLane[kLanes];
    for (int i = 0; i < kLanes; i++) prevLane[i] = -1000;
    for (int i = 0; i < kMelN; i++) {
        Note n;
        n.step = kMel[i * 2];
        n.midi = kMel[i * 2 + 1];
        n.lane = laneOf(n.midi);
        n.time = kCount + n.step * kEighth;
        if (n.step <= prevStep || n.lane < 0 || n.lane >= kLanes) chartOk_ = false;
        if (n.time - prevLane[n.lane] < kWindow * 2.0 + 1.0 / 60.0) chartOk_ = false;
        prevStep = n.step;
        prevLane[n.lane] = n.time;
        notes_.push_back(n);
    }
    if (notes_.empty()) chartOk_ = false;
    hits_ = perfect_ = misses_ = combo_ = bestCombo_ = 0;
    songFrame_ = 0;
    t_ = 0;
    clickN_ = eighthN_ = 0;
    fanStep_ = -1;
    melOn_ = bassOn_ = false;
    pops_.clear();
    for (int i = 0; i < kLanes; i++) keyLit_[i] = 0;
    over_ = false;
    won_ = false;
    sys_->apu.silence();
    if (!chartOk_) {
        std::fprintf(stderr, "s3keys: chart windows overlap\n");
        mode_ = Mode::Dead;
        over_ = true;
        won_ = false;
        return;
    }
    mode_ = Mode::Play;
}

void Game::music(double t) {
    if (mode_ != Mode::Play) return;
    while (clickN_ < 3 && t + 1e-9 >= clickN_ * kBeat) {
        click(clickN_ == 2);
        clickN_++;
    }
    if (t + 1e-9 < kCount) return;
    int guard = 0;
    while (eighthN_ <= 96 && t + 1e-9 >= kCount + eighthN_ * kEighth) {
        int bar = std::clamp(eighthN_ / 8, 0, 11);
        if (eighthN_ % 2 == 0) {
            noteOn(0, kRoot[bar], 0.2f);
            bassOn_ = true;
            bassOff_ = t + 0.16;
            if (eighthN_ % 8 == 0) noteOn(1, kRoot[bar] + 12, 0.07f);
            sys_->apu.noiseBurst(eighthN_ % 8 == 0 ? 0.16f : 0.07f, 150.0f, 0.06f);
        } else {
            sys_->apu.noiseBurst(0.035f, 2600.0f, 0.03f);
        }
        eighthN_++;
        if (++guard > 8) break;
    }
}

void Game::updatePlay() {
    const double t = t_;
    bool press[kLanes] = {};
    if (bot_) {
        for (const Note& n : notes_) {
            if (n.done) continue;
            double err = t - n.time;
            if (err >= -1.0 / 60.0 && err <= kWindow) press[n.lane] = true;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        for (int i = 0; i < kLanes; i++) press[i] = pad.pressed(kBtn[i]) || pad.pressed(kAlt[i]);
    }
    for (int i = 0; i < kLanes; i++)
        if (press[i]) keyLit_[i] = 0.12f;

    if (melOn_ && t >= melOff_) {
        sys_->apu.keyOff(2);
        melOn_ = false;
    }
    if (bassOn_ && t >= bassOff_) {
        sys_->apu.keyOff(0);
        bassOn_ = false;
    }

    const bool armed = !notes_.empty() && t + 1e-6 >= notes_.front().time - kWindow;
    if (armed && mode_ == Mode::Play) {
        int best[kLanes];
        double bestAbs[kLanes];
        for (int i = 0; i < kLanes; i++) {
            best[i] = -1;
            bestAbs[i] = 1e9;
        }
        for (int i = 0; i < int(notes_.size()); i++) {
            if (notes_[i].done) continue;
            double err = t - notes_[i].time;
            double a = std::fabs(err);
            int lane = notes_[i].lane;
            if (a <= kWindow && a < bestAbs[lane]) {
                bestAbs[lane] = a;
                best[lane] = i;
            }
        }
        bool dropped[kLanes] = {};
        for (int i = 0; i < int(notes_.size()); i++) {
            if (mode_ != Mode::Play) break;
            if (notes_[i].done) continue;
            if (t - notes_[i].time > kWindow) {
                dropped[notes_[i].lane] = true;
                notes_[i].done = true;
                miss(notes_[i].lane);
            }
        }
        if (mode_ == Mode::Play) {
            const double last = notes_.back().time;
            for (int lane = 0; lane < kLanes; lane++) {
                if (!press[lane]) continue;
                if (best[lane] >= 0) {
                    Note& n = notes_[best[lane]];
                    if (n.done) continue;
                    n.done = true;
                    hits_++;
                    double a = std::fabs(t - n.time);
                    bool perfect = a <= kPerfect;
                    if (perfect) perfect_++;
                    combo_++;
                    if (combo_ > bestCombo_) bestCombo_ = combo_;
                    noteOn(2, n.midi, perfect ? 0.28f : 0.22f);
                    melOn_ = true;
                    melOff_ = t + (perfect ? 0.22 : 0.16);
                    keyLit_[lane] = 0.16f;
                    if (pops_.size() < 12)
                        pops_.push_back(Pop{float(kLaneX[lane]), float(kHitY - 22), clock_, perfect ? 0 : 1});
                    if (!bot_) sys_->rumble(0.12f, perfect ? 0.45f : 0.25f, 30);
                } else if (!dropped[lane] && t <= last + kWindow) {
                    miss(lane);
                }
            }
        }
    }

    if (mode_ == Mode::Play && !notes_.empty()) {
        bool pending = false;
        for (const Note& n : notes_)
            if (!n.done) pending = true;
        if (!pending && t >= notes_.back().time + 1.25) win();
        else if (mode_ == Mode::Play) music(t);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    const bool dead = mode_ == Mode::Dead;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        uint16_t sky = lerpC(gs::rgb4(1, 1, 5), gs::rgb4(7, 3, 9), u);
        if (dead) sky = lerpC(sky, gs::rgb4(2, 0, 1), 0.72f);
        else if (mode_ == Mode::Clear) sky = lerpC(sky, gs::rgb4(2, 3, 7), 0.35f);
        v.lineBackdrop[y] = sky;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        text("S3 KEYS", 160, 46, 1.3f, PAL_GOLD);
        text("LANTERN", 160, 82, 0.95f, PAL_WHITE);
    } else if (mode_ == Mode::Dead) {
        text("THE TUNE DIES", 160, 70, 0.85f, PAL_BAD);
    } else if (mode_ == Mode::Clear) {
        text("THE TUNE HOLDS", 160, 64, 0.78f, PAL_GOOD);
    } else if (mode_ == Mode::Play && t_ < kCount) {
        int idx = int(t_ / kBeat);
        if (idx < 0) idx = 0;
        if (idx > 2) idx = 2;
        text(std::to_string(3 - idx), 160, 96, 1.5f, PAL_GOLD);
    }
    for (const Pop& p : pops_) {
        float age = float(clock_ - p.born);
        const char* word = p.kind == 0 ? "PERFECT" : p.kind == 1 ? "GOOD" : "MISS";
        int pal = p.kind == 0 ? PAL_GOLD : p.kind == 1 ? PAL_GOOD : PAL_BAD;
        text(word, p.x, p.y - age * 22.0f, 0.62f, pal);
    }
    if (mode_ == Mode::Play) {
        for (int lane = 0; lane < kLanes; lane++)
            if (keyLit_[lane] > 0.04f)
                spr(art_.flash, float(kLaneX[lane]), float(kHitY), 18 + keyLit_[lane] * 20, PAL_FX, 0);
        for (const Note& n : notes_) {
            if (n.done) continue;
            float lead = float(n.time - t_);
            float y = float(kHitY) - lead / float(kApproach) * kTravel;
            if (y < -20 || y > 210) continue;
            int fog = lead > 0 ? std::clamp(int(lead / float(kApproach) * 10.0f), 0, 10) : 0;
            spr(art_.note, float(kLaneX[n.lane]), y, 22, kNotePal[n.lane], fog);
        }
        for (int lane = 0; lane < kLanes; lane++) {
            bool hot = keyLit_[lane] > 0;
            if (!hot) {
                for (const Note& n : notes_) {
                    if (n.done || n.lane != lane) continue;
                    if (std::fabs(t_ - n.time) <= kWindow) {
                        hot = true;
                        break;
                    }
                }
            }
            spr(art_.ring, float(kLaneX[lane]), float(kHitY), 16, hot ? PAL_GOLD : PAL_DIM);
        }
    }
    for (int lane = 0; lane < kLanes; lane++) {
        float y = 180.0f + (keyLit_[lane] > 0 ? 3.0f : 0.0f);
        spr(art_.key[lane], float(kLaneX[lane]), y, 26, keyLit_[lane] > 0 ? PAL_KEYLIT : PAL_KEY);
    }
    for (int i = 0; i < kLamps; i++) {
        bool lit = !dead && i >= misses_;
        spr(art_.lamp, float(kLampX[i]), 10, 16, lit ? PAL_LAMP : PAL_DIM);
    }
    float sway = std::sin(float(clock_) * 1.4f) * 3.0f;
    int lampPal = dead ? PAL_DIM : PAL_LAMP;
    spr(art_.lantern, 30, 58 + sway, 42, lampPal);
    spr(art_.lantern, gs::SCREEN_W - 30, 58 - sway, 42, lampPal);

    if (mode_ == Mode::Title) {
        hudC(14, "ONE SONG", PAL_GOLD);
        hudC(16, "MISS FIVE AND THE TUNE DIES", PAL_WHITE);
        hudC(18, "ARROWS OR Z X C W", PAL_WHITE);
        if ((int(clock_ * 2) & 1) == 0) hudC(21, "PRESS START", PAL_GOLD);
        const char* ver = S3_VERSION_STRING;
        hud(40 - int(std::strlen(ver)), 27, ver, PAL_WHITE);
    } else if (mode_ == Mode::Play) {
        std::snprintf(buf, sizeof buf, "HITS %d", hits_);
        hud(1, 0, buf, PAL_WHITE);
        if (misses_ >= 4) hud(31, 0, "ONE LEFT", PAL_BAD);
        else {
            std::snprintf(buf, sizeof buf, "MISS %d", misses_);
            hud(32, 0, buf, misses_ ? PAL_BAD : PAL_WHITE);
        }
        if (combo_ >= 4) {
            std::snprintf(buf, sizeof buf, "X%d", combo_);
            hud(18, 0, buf, PAL_GOLD);
        }
        if (t_ < kCount) hudC(26, "HIT THE GOLD LINE", PAL_WHITE);
    } else if (mode_ == Mode::Dead) {
        std::snprintf(buf, sizeof buf, "HITS %d", hits_);
        hudC(14, buf, PAL_WHITE);
        hudC(16, "FIVE MISSES", PAL_BAD);
        if ((int(clock_ * 2) & 1) == 0) hudC(20, "START", PAL_GOLD);
    } else if (mode_ == Mode::Clear) {
        std::snprintf(buf, sizeof buf, "HITS %d", hits_);
        hudC(13, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "PERFECT %d", perfect_);
        hudC(15, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "BEST X%d", std::max(bestCombo_, combo_));
        hudC(17, buf, PAL_WHITE);
        if ((int(clock_ * 2) & 1) == 0) hudC(21, "START", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.22f, 0.34f, 0.2f);
    sys.apu.setPatch(0, bassPatch());
    sys.apu.setPatch(1, padPatch());
    sys.apu.setPatch(2, pianoPatch());
    sys.apu.setPan(0, -0.35f);
    sys.apu.setPan(1, 0.15f);
    sys.apu.setPan(2, 0.05f);
    mode_ = Mode::Title;
    if (bot_) startSong();
    draw();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += 1.0 / 60.0;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && pad.pressed(gs::BTN_START)) startSong();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        t_ = songFrame_ / 60.0;
        updatePlay();
        songFrame_++;
    } else if (mode_ == Mode::Dead) {
        double age = clock_ - deadAt_;
        if (age < 1.5) sys.apu.setFreq(2, 110.0f * std::pow(0.5f, float(age) * 0.85f));
        if (!bot_ && pad.pressed(gs::BTN_START)) startSong();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            sys.apu.silence();
        }
    } else if (mode_ == Mode::Clear) {
        double age = clock_ - clearAt_;
        int step = int(age / 0.12);
        static const int fan[4] = {69, 72, 76, 81};
        if (step != fanStep_ && step >= 0 && step < 4) {
            fanStep_ = step;
            noteOn(2, fan[step], 0.22f);
        }
        if (!bot_ && pad.pressed(gs::BTN_START)) startSong();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            sys.apu.silence();
        }
    }

    if (clickLeft_ > 0) {
        clickLeft_ -= 1.0 / 60.0;
        if (clickLeft_ <= 0) sys.apu.tone(0, 0, 0);
    }
    for (int i = 0; i < kLanes; i++)
        if (keyLit_[i] > 0) keyLit_[i] = std::max(0.0f, keyLit_[i] - float(1.0 / 60.0));
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(),
                               [&](const Pop& p) { return clock_ - p.born > 0.55; }),
                pops_.end());
    if (!bot_) {
        int g = std::max(20, 170 - misses_ * 30);
        sys.setLight(220, mode_ == Mode::Dead ? 24 : g, mode_ == Mode::Clear ? 90 : 36);
    }
    draw();
}

}  // namespace keys
