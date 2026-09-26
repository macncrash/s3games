#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace dbann {
namespace {

constexpr float kMax = 2.35f;
constexpr float kAcc = 0.16f;
constexpr float kBody = 16.f;
constexpr float kMap = 2048.f;
constexpr float kXLo = 72.f;
constexpr float kXHi = 1780.f;
constexpr float kPocket = 188.f;
constexpr float kMastX = 112.f;
constexpr float kBannerHome = 1660.f;
constexpr int kBannerTrack = 2;
constexpr int kShift = 120 * 60;
constexpr float kRailY[3] = {127.f, 159.f, 191.f};
constexpr int kRailRow[3] = {15, 19, 23};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 4;
    if (carrying_ && px_ < 980.f) return 3;
    if (carrying_) return 2;
    if (mode_ == Mode::Play) return 1;
    return 0;
}

void Game::tune() {
    gs::FMPatch p;
    p.alg = 7;
    p.vol = 0.18f;
    p.fb = 0.04f;
    p.glide = 0.02f;
    p.tone = 900;
    for (int i = 0; i < 4; i++) {
        p.op[i].mul = i == 0 ? 1.f : 2.f;
        p.op[i].level = i == 0 ? 1.f : 0.18f;
        p.op[i].ar = 0.004f;
        p.op[i].dr = 0.18f;
        p.op[i].sl = 0.2f;
        p.op[i].rr = 0.14f;
    }
    for (int ch = 0; ch < 4; ch++) sys_->apu.setPatch(ch, p);
    sys_->apu.setEcho(0.14f, 0.22f, 0.1f);
}

void Game::layout() {
    cuts_[0] = {1380.f, 0.62f, 1220.f, 1540.f, 50.f, 0, PAL_CAR};
    cuts_[1] = {1040.f, -0.55f, 800.f, 1160.f, 50.f, 1, PAL_TEAL};
    cuts_[2] = {500.f, -0.48f, 380.f, 640.f, 50.f, 2, PAL_CAR};
    bannerX_ = kBannerHome;
    bannerTrack_ = kBannerTrack;
    atSiding_ = true;
    raised_ = false;
    carrying_ = false;
    px_ = 230.f;
    vx_ = 0;
    track_ = 1;
    py_ = kRailY[1];
    face_ = 1;
    lives_ = 3;
    clock_ = kShift;
    switchCd_ = 0;
    inv_ = 0;
    phase_ = Phase::Out;
    song_ = -1;
    songKind_ = 0;
    reason_ = "THE BANNER IS STILL OUT";
    for (auto& p : puffs_) p.t = 0;
}

void Game::paint() {
    gs::VDP& v = sys_->vdp;
    v.A.resize(256, 32);
    v.B.resize(64, 32);
    v.A.clear();
    v.B.clear();
    for (int cy = 0; cy < 12; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            uint32_t h = uint32_t(cx) * 374761393u + uint32_t(cy) * 668265263u;
            if (cy < 8 && (h % 47u) == 0) v.B.set(cx, cy, gs::entry(art_.star, PAL_NIGHT));
            if (cy == 9 || cy == 10) v.B.set(cx, cy, gs::entry(art_.hill, PAL_NIGHT, 0, cy == 10));
        }
    }
    for (int cx = 0; cx < 256; cx++) {
        uint32_t h = uint32_t(cx) * 2246822519u;
        for (int row = 13; row <= 27; row++) {
            bool rail = row == kRailRow[0] || row == kRailRow[1] || row == kRailRow[2];
            int tile = rail ? art_.rail : art_.ballast[int(h + row) % 3];
            v.A.set(cx, row, gs::entry(tile, PAL_YARD));
        }
        bool pocket = cx >= 8 && cx <= 24;
        v.A.set(cx, 12, gs::entry(pocket ? art_.hazard : art_.apron, PAL_YARD));
        bool office = cx >= 2 && cx <= 28;
        bool shed = cx >= 214 && cx <= 248;
        if (office || shed) {
            int wall = shed ? art_.shed : art_.brick;
            int win = shed ? art_.shedWin : art_.window;
            int pal = PAL_BRICK;
            v.A.set(cx, 7, gs::entry(art_.roof, pal));
            for (int row = 8; row <= 11; row++) {
                int tile = wall;
                if (row == 9 && ((cx + (shed ? 1 : 0)) % 4) == 0) tile = win;
                if (!shed && row == 10 && (cx == 8 || cx == 18)) tile = art_.door;
                v.A.set(cx, row, gs::entry(tile, pal));
            }
        }
    }
}

void Game::begin() {
    layout();
    paint();
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    horn_ = false;
    t_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    tune();
    layout();
    paint();
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    reason_ = "THE BANNER IS STILL OUT";
    sys.setLight(90, 70, 30);
}

bool Game::safe(int tr, float x, float margin) const {
    if (tr < 0 || tr > 2) return false;
    for (const auto& c : cuts_) {
        if (c.track != tr) continue;
        if (std::fabs(x - c.x) < c.half + kBody + margin) return false;
    }
    return true;
}

void Game::trySwitch(int dir) {
    if (dir == 0 || switchCd_ > 0) return;
    int nt = track_ + dir;
    if (nt < 0 || nt > 2) return;
    if (!safe(nt, px_, 8.f)) {
        if (!bot_) blip(140.f, 0.08f);
        return;
    }
    track_ = nt;
    switchCd_ = 6;
    blip(620.f, 0.07f);
}

void Game::bot(float& throttle, int& sw) {
    throttle = 0;
    sw = 0;
    auto hold = [&](float x, float slow) {
        float d = x - px_;
        if (d > slow) throttle = 1.f;
        else if (d > 10.f) throttle = 0.35f;
        else if (d < -slow) throttle = -1.f;
        else if (d < -10.f) throttle = -0.35f;
    };
    auto toward = [&](int tr) {
        if (track_ == tr) return;
        int step = tr > track_ ? 1 : -1;
        if (safe(track_ + step, px_, 18.f)) sw = step;
    };

    if (!carrying_) {
        if (phase_ == Phase::Out) {
            if (track_ != 1 && px_ < 700.f) toward(1);
            else hold(630.f, 70.f);
            if (track_ == 1 && px_ > 590.f && px_ < 690.f) phase_ = Phase::Wait;
        } else if (phase_ == Phase::Wait) {
            if (track_ == 2) {
                phase_ = Phase::Fetch;
                return;
            }
            hold(630.f, 40.f);
            const Cut& c = cuts_[2];
            float right = c.x + c.half;
            bool room = right < px_ - 64.f && (c.vx <= 0.f || right < px_ - 150.f);
            if (track_ == 1 && room && safe(2, px_, 36.f)) sw = 1;
        } else {
            if (track_ != kBannerTrack) toward(kBannerTrack);
            else hold(bannerX_, 90.f);
        }
        return;
    }

    if (phase_ != Phase::Return && phase_ != Phase::Fork && phase_ != Phase::Home) phase_ = Phase::Return;
    if (phase_ == Phase::Return) {
        if (track_ != 2) toward(2);
        else if (px_ > 1200.f) throttle = -1.f;
        else if (px_ > 1000.f) throttle = -0.5f;
        else if (px_ > 900.f) throttle = -0.28f;
        else if (px_ > 830.f) throttle = 0.f;
        else throttle = 0.35f;
        if (track_ == 2 && px_ < 910.f && px_ > 800.f && std::fabs(vx_) < 0.75f) phase_ = Phase::Fork;
    } else if (phase_ == Phase::Fork) {
        if (px_ < 800.f) throttle = 0.4f;
        else if (px_ > 900.f) throttle = -0.4f;
        if (track_ == 2 && px_ >= 800.f && px_ <= 900.f && safe(1, px_, 52.f)) sw = -1;
        else if (track_ == 1 && safe(0, px_, 16.f)) sw = -1;
        else if (track_ == 0) phase_ = Phase::Home;
    } else {
        if (track_ != 0) toward(0);
        else hold(120.f, 80.f);
    }
}

void Game::moveCuts() {
    for (auto& c : cuts_) {
        c.x += c.vx;
        if (c.x > c.maxX) {
            c.x = c.maxX;
            c.vx = -std::fabs(c.vx);
        } else if (c.x < c.minX) {
            c.x = c.minX;
            c.vx = std::fabs(c.vx);
        }
    }
}

void Game::collide() {
    if (inv_ > 0) {
        inv_--;
        return;
    }
    for (const auto& c : cuts_) {
        if (c.track != track_) continue;
        if (std::fabs(px_ - c.x) >= c.half + kBody) continue;
        lives_--;
        if (carrying_) {
            carrying_ = false;
            bannerX_ = std::clamp(px_, kXLo, kXHi);
            bannerTrack_ = track_;
            atSiding_ = false;
        }
        px_ = (px_ < c.x) ? c.x - c.half - kBody - 6.f : c.x + c.half + kBody + 6.f;
        px_ = std::clamp(px_, kXLo, kXHi);
        vx_ = 0;
        inv_ = 80;
        shake_ = 7;
        horn_ = true;
        phase_ = carrying_ ? Phase::Return : (px_ > 760.f && track_ == 2 ? Phase::Fetch : Phase::Out);
        if (lives_ <= 0) lose("THE LOCO LEFT THE RAIL");
        else blip(90.f, 0.12f);
        sys_->rumble(0.6f, 0.3f, 120);
        return;
    }
}

void Game::couple() {
    if (!carrying_) {
        if (track_ == bannerTrack_ && std::fabs(px_ - bannerX_) < 28.f && inv_ == 0) {
            carrying_ = true;
            atSiding_ = false;
            blip(740.f, 0.12f);
            sys_->rumble(0.2f, 0.45f, 80);
        }
        return;
    }
    if (px_ < kPocket) {
        carrying_ = false;
        raised_ = true;
        win();
    }
}

void Game::tickPlay(const gs::Pad& pad) {
    float throttle = 0;
    int sw = 0;
    if (bot_) {
        bot(throttle, sw);
    } else {
        throttle = pad.axisX;
        if (pad.down(gs::BTN_RIGHT)) throttle = 1.f;
        if (pad.down(gs::BTN_LEFT)) throttle = -1.f;
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_Y)) sw = -1;
        if (pad.pressed(gs::BTN_DOWN) || pad.pressed(gs::BTN_X)) sw = 1;
        if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) horn_ = true;
    }
    trySwitch(sw);
    if (switchCd_ > 0) switchCd_--;

    float want = std::clamp(throttle, -1.f, 1.f) * kMax;
    if (vx_ < want) vx_ = std::min(want, vx_ + kAcc);
    else vx_ = std::max(want, vx_ - kAcc);
    float nx = px_ + vx_;
    if (bot_ && safe(track_, px_, 0.f) && !safe(track_, nx, 0.f)) {
        vx_ = 0;
        nx = px_;
    }
    px_ = std::clamp(nx, kXLo, kXHi);
    if (vx_ > 0.2f) face_ = 1;
    else if (vx_ < -0.2f) face_ = -1;
    float goalY = kRailY[track_];
    py_ += std::clamp(goalY - py_, -6.f, 6.f);

    if (std::fabs(vx_) > 0.45f && (sys_->frame & 3) == 0) {
        for (auto& p : puffs_) {
            if (p.t > 0) continue;
            p.x = px_ - float(face_) * 18.f;
            p.y = py_ - 16.f;
            p.vx = -float(face_) * 0.3f;
            p.t = 16.f;
            break;
        }
    }

    moveCuts();
    if (mode_ != Mode::Play) return;
    collide();
    if (mode_ != Mode::Play) return;
    couple();
    if (mode_ != Mode::Play) return;
    if (--clock_ <= 0) lose("THE SHIFT RAN OUT");
    t_ += 1.f;
}

void Game::win() {
    if (mode_ != Mode::Play || !raised_) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = 0;
    reason_ = "HOME";
    song_ = 0;
    songKind_ = 2;
    sys_->rumble(0.35f, 0.7f, 180);
    sys_->setLight(40, 170, 60);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    vx_ = 0;
    reason_ = why;
    song_ = 0;
    songKind_ = 3;
    sys_->apu.noiseBurst(0.16f, 420.f, 0.4f);
    sys_->setLight(170, 30, 20);
}

void Game::blip(float freq, float vol) { sys_->apu.keyOn(1, freq, vol); }

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = std::clamp(float(y) / 150.f, 0.f, 1.f);
        int r = int(1 + u * 6);
        int g = int(1 + u * 4);
        int b = int(6 - u * 3);
        if (y > 96) {
            r = 3;
            g = 3;
            b = 2;
        }
        v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::spr(const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet, bool shadow) {
    if (!sys_ || h < 1.f || m.h <= 0) return;
    float w = h * float(m.w) / float(m.h);
    float sx = wx - cam_ - w * 0.5f;
    float sy = feet ? foot - h : foot - h * 0.5f;
    if (sx > 330.f || sx + w < -10.f || sy > gs::SCREEN_H || sy + h < -10.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(sx));
    s.y = int16_t(std::lround(sy));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();
    if (shake_ > 0) shake_ -= 1.f;

    float focus = px_;
    if (mode_ == Mode::Title) focus = 200.f;
    if (mode_ == Mode::Victory) focus = 150.f;
    cam_ = std::clamp(focus - 128.f, 0.f, kMap - float(gs::SCREEN_W));
    if (shake_ > 0) cam_ += std::sin(t_ * 1.7f) * shake_;
    int cam = int(std::lround(cam_));
    v.A.scroll(-cam, 0);
    v.B.scroll(-cam / 3, 0);

    int wave = int(sys_->frame / 8) & 1;
    const gs::Mipped& cloth = art_.cloth[wave];

    if (carrying_ && (inv_ == 0 || (sys_->frame & 2))) {
        spr(cloth, px_ + float(face_) * 6.f, py_ - 30.f, 18.f, PAL_BANN, face_ < 0, false);
    }
    if (inv_ == 0 || (sys_->frame & 2) || mode_ != Mode::Play) {
        float bob = std::sin(t_ * 0.7f) * std::min(1.f, std::fabs(vx_)) * 0.6f;
        spr(art_.loco, px_, py_ + bob, 30.f, PAL_LOCO, face_ < 0, true);
    }
    spr(art_.shadow, px_, py_ + 2.f, 8.f, PAL_LOCO, false, true, true);

    spr(art_.flat, kBannerHome, kRailY[kBannerTrack], 16.f, PAL_WOOD, false, true);
    if (raised_) spr(cloth, kMastX + 8.f, 78.f, 20.f, PAL_BANN, false, false);
    else if (!carrying_) {
        float by = atSiding_ ? kRailY[bannerTrack_] - 26.f : kRailY[bannerTrack_] - 6.f;
        spr(cloth, bannerX_, by, atSiding_ ? 20.f : 16.f, PAL_BANN, false, false);
    }

    for (int tr = 2; tr >= 0; --tr) {
        for (const auto& c : cuts_) {
            if (c.track != tr) continue;
            spr(art_.car, c.x - 22.f, kRailY[tr], 32.f, c.pal, c.vx < 0, true);
            spr(art_.car, c.x + 22.f, kRailY[tr], 32.f, c.pal, c.vx < 0, true);
            spr(art_.shadow, c.x, kRailY[tr] + 2.f, 8.f, PAL_LOCO, false, true, true);
        }
    }

    spr(art_.mast, kMastX, 127.f, 70.f, PAL_BANN, false, true);
    static const float lamps[] = {200.f, 480.f, 760.f, 1080.f, 1360.f, 1640.f};
    for (float x : lamps) {
        bool hot = (int(x + sys_->frame) / 20) % 5 != 0;
        spr(art_.lamp, x, 104.f, hot ? 46.f : 44.f, PAL_LAMP, false, true);
    }
    spr(art_.tower, 360.f, 104.f, 56.f, PAL_LAMP, false, true);
    spr(art_.tower, 1480.f, 104.f, 52.f, PAL_LAMP, false, true);
    spr(art_.crate, 250.f, 104.f, 16.f, PAL_WOOD, false, true);
    spr(art_.crate, 276.f, 104.f, 14.f, PAL_CAR, false, true);
    spr(art_.crate, 1588.f, 104.f, 16.f, PAL_WOOD, false, true);

    for (auto& p : puffs_) {
        if (p.t <= 0) continue;
        p.x += p.vx;
        p.t -= 1.f;
        spr(art_.puff, p.x, p.y - (16.f - p.t) * 0.3f, 6.f + (16.f - p.t) * 0.35f, PAL_SMOKE, false, false);
    }

    spr(art_.moon, cam_ + 286.f, 22.f, 16.f, PAL_NIGHT, false, false);
    float c1 = std::fmod(cam_ * 0.25f + float(sys_->frame) * 0.15f, 400.f);
    spr(art_.cloud, cam_ + 40.f + c1, 30.f, 12.f, PAL_NIGHT, false, false);
    spr(art_.cloud, cam_ + 180.f + c1 * 0.6f, 18.f, 10.f, PAL_NIGHT, false, false);

    if (mode_ == Mode::Title) {
        spr(art_.logo, cam_ + 160.f, 30.f, float(art_.logo.h), PAL_TITLE, false, false);
        spr(art_.tag, cam_ + 160.f, 50.f, float(art_.tag.h), PAL_TITLE, false, false);
    } else if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        const gs::Mipped& word = mode_ == Mode::Victory ? art_.win : art_.lose;
        spr(word, cam_ + 160.f, 36.f, float(word.h), PAL_TITLE, false, false);
        spr(art_.plate, cam_ + 160.f, 36.f, float(art_.plate.h), PAL_HUD, false, false);
    }

    for (int x = 0; x < 40; x++) {
        v.HUD.set(x, 0, gs::entry(art_.bar, PAL_HUD));
        if (mode_ != Mode::Play) v.HUD.set(x, 26, gs::entry(art_.bar, PAL_HUD));
        if (mode_ == Mode::Fail) v.HUD.set(x, 25, gs::entry(art_.bar, PAL_HUD));
    }
    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 DEPOT BANN", PAL_HUD);
        const char* blink = ((sys_->frame / 30) & 1) ? "ENTER" : "FETCH";
        hud(40 - int(std::strlen(blink)), 0, blink, PAL_HUD);
        hudC(26, "ARROWS DRIVE AND SWITCH", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(0, "PAUSED", PAL_HUD);
        hudC(26, "ENTER RESUMES", PAL_HUD);
    } else {
        char left[24];
        std::snprintf(left, sizeof left, "RAIL %d", std::max(0, lives_));
        hud(0, 0, left, lives_ <= 1 ? PAL_ALERT : PAL_HUD);
        const char* job = carrying_ ? "ABOARD" : (raised_ ? "HOME" : "FETCH");
        if (mode_ == Mode::Fail) job = "LOST";
        hud(8, 0, job, carrying_ ? PAL_ALERT : PAL_HUD);
        int sec = clock_ > 0 ? (clock_ + 59) / 60 : 0;
        char clock[16];
        std::snprintf(clock, sizeof clock, "SHIFT %d:%02d", sec / 60, sec % 60);
        int pal = (mode_ == Mode::Play && sec <= 10 && ((sys_->frame / 8) & 1)) ? PAL_ALERT : PAL_HUD;
        hud(40 - int(std::strlen(clock)), 0, clock, pal);
        if (mode_ == Mode::Victory) hudC(26, "THE BANNER IS ON THE MAST", PAL_HUD);
        if (mode_ == Mode::Fail) {
            hudC(25, reason_, PAL_ALERT);
            hudC(26, "ENTER TRIES THE YARD AGAIN", PAL_HUD);
        }
    }

    if (mode_ == Mode::Victory) sys_->setLight(40, 170, 60);
    else if (mode_ == Mode::Fail) sys_->setLight(170, 30, 20);
    else if (carrying_) sys_->setLight(220, 150, 40);
    else sys_->setLight(80, 90, 130);
}

void Game::serviceAudio() {
    float rpm = std::min(1.f, std::fabs(vx_) / kMax);
    bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    sys_->apu.tone(0, 48.f + rpm * 42.f, live ? 0.012f + rpm * 0.03f : 0.f);
    sys_->apu.tone(1, 96.f + rpm * 20.f, live ? 0.006f + rpm * 0.012f : 0.f);
    sys_->apu.noise(live ? 0.01f + rpm * 0.03f : 0.f, 280.f + rpm * 900.f, false);
    if (horn_) {
        horn_ = false;
        sys_->apu.noiseBurst(0.08f, 1100.f, 0.07f);
        sys_->apu.keyOn(2, 220.f, 0.1f);
    }
    if (song_ >= 0) {
        struct N {
            int t;
            float f;
        };
        static const N winN[] = {{0, 392.f}, {8, 523.f}, {16, 659.f}, {24, 784.f}, {36, 1046.f}};
        static const N loseN[] = {{0, 392.f}, {12, 330.f}, {24, 262.f}, {40, 196.f}};
        const N* ns = songKind_ == 2 ? winN : loseN;
        int count = songKind_ == 2 ? 5 : 4;
        int end = 70;
        for (int i = 0; i < count; i++)
            if (song_ == ns[i].t) sys_->apu.keyOn(0, ns[i].f, 0.16f);
        if (++song_ > end) song_ = -1;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        } else if (start || bot_) {
            begin();
        }
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        if (!bot_ && start) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) mode_ = Mode::Pause;
        else tickPlay(pad);
    }
    draw();
    serviceAudio();
}

}  // namespace dbann
