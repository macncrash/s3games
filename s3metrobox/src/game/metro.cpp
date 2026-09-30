#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace metro {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kLen = 96.f;
constexpr float kAccel = 78.f;
constexpr float kBrake = 148.f;
constexpr float kRev = 42.f;
constexpr float kDrag = 0.35f;
constexpr float kMax = 104.f;
constexpr float kStop = 3.2f;
constexpr float kDwell = 0.7f;
constexpr float kRival = 58.f;
constexpr int kStopsN = 4;

}  // namespace

const Game::St& Game::stop() const {
    static const St kStops[kStopsN] = {
        {"GATE", 560.f, 156.f},
        {"ARCH", 1380.f, 144.f},
        {"DOCK", 2140.f, 138.f},
        {"YARD", 2920.f, 164.f},
    };
    int i = next_;
    if (i < 0) i = 0;
    if (i >= kStopsN) i = kStopsN - 1;
    return kStops[i];
}

float Game::cam() const {
    if (mode_ == Mode::Title) return 470.f;
    return nose_ - 108.f;
}

bool Game::insideBox() const {
    const St& s = stop();
    float tail = nose_ - kLen;
    return tail >= s.left - 0.4f && nose_ <= s.left + s.width + 0.4f;
}

float Game::crewLeft() const {
    const St& s = stop();
    return std::max(0.f, (s.left - rival_) / kRival);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int t = y < 48 ? 1 : (y < 150 ? 2 : 1);
        sys.vdp.lineBackdrop[y] = gs::rgb4(t, t, t + 1);
    }
    showTitle();
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    next_ = 0;
    score_ = 0;
    time_ = 0;
    dwell_ = 0;
    speed_ = 0;
    nose_ = 560.f + (156.f + kLen) * 0.5f;
    rival_ = 180.f;
    msgT_ = 0;
    msg_[0] = 0;
    line_[0] = 0;
    why_ = nullptr;
    song_ = -1;
    songT_ = 0;
    if (sys_) sys_->apu.tone(0, 0, 0);
}

void Game::begin() {
    mode_ = Mode::Drive;
    over_ = false;
    won_ = false;
    next_ = 0;
    score_ = 0;
    time_ = 0;
    dwell_ = 0;
    speed_ = 0;
    nose_ = 96.f;
    rival_ = 24.f;
    msgT_ = 0;
    msg_[0] = 0;
    line_[0] = 0;
    why_ = nullptr;
    song_ = -1;
    std::snprintf(msg_, sizeof msg_, "HOLD RIGHT  STOP IN THE BOX");
    msgT_ = 2.4f;
}

void Game::finish(bool win, const char* why) {
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return;
    won_ = win;
    over_ = true;
    why_ = why;
    mode_ = win ? Mode::Win : Mode::Fail;
    dwell_ = 0;
    speed_ = 0;
    if (win) {
        std::snprintf(line_, sizeof line_,
                      "S3 METROBOX  WIN  stopped inside the box  beat the other crew  score %d  (%.1f s)",
                      score_, time_);
        if (sys_) sys_->apu.tone(0, 523.f, 0.18f);
    } else {
        std::snprintf(line_, sizeof line_, "S3 METROBOX  FAIL  %s  score %d  cleared %d/%d  (%.1f s)",
                      why ? why : "lost the clock", score_, next_, kStopsN, time_);
        if (sys_) sys_->apu.tone(0, 90.f, 0.2f);
    }
}

Game::In Game::human() {
    In in;
    const gs::Pad& p = sys_->pad;
    float accel = std::max(p.accel, p.down(gs::BTN_RIGHT) || p.down(gs::BTN_C) ? 1.f : 0.f);
    bool brake = p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.15f || p.down(gs::BTN_LEFT);
    bool back = p.down(gs::BTN_DOWN) || (p.down(gs::BTN_LEFT) && speed_ <= kStop);
    if (back && speed_ <= kStop + 1.f) {
        in.reverse = 1.f;
    } else if (brake) {
        in.brake = 1.f;
    }
    if (accel > 0.f && in.reverse <= 0.f) in.throttle = accel;
    return in;
}

Game::In Game::pilot() const {
    In in;
    if (dwell_ > 0.f || next_ >= kStopsN) return in;
    const St& s = stop();
    float target = s.left + (s.width + kLen) * 0.5f;
    float dist = target - nose_;
    float v = speed_;
    if (dist < -2.5f) {
        in.reverse = 1.f;
        return in;
    }
    float stopD = v > 0.f ? (v * v) / (2.f * kBrake * 0.92f) : 0.f;
    if (dist < 36.f && v < 24.f) {
        if (dist > 2.2f) in.throttle = 0.42f;
        else in.brake = 1.f;
        return in;
    }
    if (v > 0.8f && dist <= stopD + 10.f) {
        in.brake = 1.f;
        return in;
    }
    in.throttle = dist > 160.f ? 1.f : 0.72f;
    return in;
}

Game::In Game::readInput() {
    if (bot_ || (sys_ && sys_->headless && !sys_->scripted)) return pilot();
    return human();
}

void Game::physics(const In& in, float dt) {
    if (dwell_ > 0.f) {
        speed_ = 0.f;
        dwell_ -= dt;
        if (dwell_ <= 0.f) {
            dwell_ = 0.f;
            int spare = int(crewLeft() * 10.f);
            score_ += 1000 + std::max(0, spare);
            next_++;
            if (sys_) sys_->apu.tone(1, 660.f, 0.12f);
            if (next_ >= kStopsN) {
                finish(true, nullptr);
                return;
            }
            std::snprintf(msg_, sizeof msg_, "CLEAR  NEXT %s", stop().name);
            msgT_ = 1.6f;
        }
        rival_ += kRival * dt;
        return;
    }

    if (in.throttle > 0.f) speed_ += kAccel * in.throttle * dt;
    if (in.reverse > 0.f) speed_ -= kRev * in.reverse * dt;
    if (in.brake > 0.f) {
        if (speed_ > 0.f) speed_ = std::max(0.f, speed_ - kBrake * in.brake * dt);
        else if (speed_ < 0.f) speed_ = std::min(0.f, speed_ + kBrake * in.brake * dt);
    }
    speed_ -= speed_ * kDrag * dt;
    if (speed_ > kMax) speed_ = kMax;
    if (speed_ < -28.f) speed_ = -28.f;
    nose_ += speed_ * dt;
    if (nose_ < kLen * 0.5f) {
        nose_ = kLen * 0.5f;
        speed_ = 0;
    }
    rival_ += kRival * dt;
}

void Game::judge() {
    if (mode_ != Mode::Drive || next_ >= kStopsN || dwell_ > 0.f) return;
    const St& s = stop();
    float tail = nose_ - kLen;
    if (tail > s.left + s.width + 6.f) {
        finish(false, "ran past the box");
        return;
    }
    if (rival_ >= s.left && !insideBox()) {
        finish(false, "the other crew took the box");
        return;
    }
    if (insideBox() && std::fabs(speed_) <= kStop) {
        dwell_ = kDwell;
        speed_ = 0;
        int slack = int(std::fabs((s.left + (s.width + kLen) * 0.5f) - nose_));
        score_ += std::max(0, 80 - slack);
        std::snprintf(msg_, sizeof msg_, "IN THE BOX  %s", s.name);
        msgT_ = 1.2f;
        if (sys_) sys_->apu.noiseBurst(0.16f, 1800.f, 0.08f);
    }
}

void Game::audio(float dt) {
    if (!sys_) return;
    gs::APU& apu = sys_->apu;
    if (mode_ == Mode::Drive && dwell_ <= 0.f) {
        float spd = std::fabs(speed_);
        apu.tone(2, 48.f + spd * 1.3f, spd > 2.f ? 0.05f : 0.02f);
        apu.noise(spd > 8.f ? 0.03f : 0.f, 900.f + spd * 8.f, true);
    } else if (mode_ != Mode::Fail) {
        apu.tone(2, 0, 0);
        apu.noise(0, 0);
    }
    songT_ -= dt;
    if (songT_ > 0.f) return;
    static const float notes[] = {196, 247, 294, 247, 220, 294, 330, 294};
    song_ = (song_ + 1) % 8;
    float vol = (mode_ == Mode::Title || mode_ == Mode::Win) ? 0.07f : 0.035f;
    apu.tone(0, notes[song_], vol);
    songT_ = (song_ % 2) ? 0.28f : 0.18f;
}

void Game::lights() {
    if (!sys_) return;
    if (mode_ == Mode::Win) sys_->setLight(40, 180, 80);
    else if (mode_ == Mode::Fail) sys_->setLight(180, 30, 30);
    else if (dwell_ > 0.f) sys_->setLight(220, 180, 40);
    else if (crewLeft() < 3.f && mode_ == Mode::Drive) sys_->setLight(200, 60, 20);
    else sys_->setLight(40, 70, 140);
}

void Game::blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (!sys_) return;
    if (cx + w * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8) return;
    if (cy + h * 0.5f < -8 || cy - h * 0.5f > gs::SCREEN_H + 8) return;
    gs::Sprite s;
    s.img = m.pick(h);
    s.x = int16_t(cx - w * 0.5f);
    s.y = int16_t(cy - h * 0.5f);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!sys_ || !s) return;
    for (; *s && col < 40; ++s, ++col) {
        unsigned char c = static_cast<unsigned char>(*s);
        if (c < 32 || c > 127) c = ' ';
        sys_->vdp.HUD.set(col, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    int col = std::max(0, (40 - n) / 2);
    hudText(col, row, s, pal);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float c = cam();
    vdp.B.scroll(int(c * 0.45f), 0);
    vdp.A.scroll(int(c), 4);

    auto sx = [&](float wx) { return wx - c; };

    for (int i = 0; i < kStopsN; i++) {
        float left = 560.f, width = 156.f;
        const char* name = "GATE";
        if (i == 1) { left = 1380.f; width = 144.f; name = "ARCH"; }
        if (i == 2) { left = 2140.f; width = 138.f; name = "DOCK"; }
        if (i == 3) { left = 2920.f; width = 164.f; name = "YARD"; }
        (void)name;
        St s{name, left, width};
        float x0 = sx(s.left);
        float x1 = sx(s.left + s.width);
        if (x1 < -20 || x0 > gs::SCREEN_W + 20) continue;
        float mid = (x0 + x1) * 0.5f;
        blit(art_.post, x0, 148, 10, 52, PAL_BOX, false);
        blit(art_.post, x1, 148, 10, 52, PAL_BOX, false);
        int dashes = std::max(2, int(s.width / 18.f));
        for (int d = 0; d < dashes; d++) {
            float u = (d + 0.5f) / dashes;
            float x = x0 + (x1 - x0) * u;
            gs::Sprite mark;
            mark.img = art_.post.pick(8);
            mark.x = int16_t(x - 5);
            mark.y = 168;
            mark.w = 10;
            mark.h = 6;
            mark.pal = PAL_BOX;
            vdp.sprite(mark);
        }
        blit(art_.lamp, x0 - 28, 132, 16, 40, PAL_LAMP, false);
        blit(art_.lamp, x1 + 28, 132, 16, 40, PAL_LAMP, true);
        blit(art_.signal, mid, 96, 16, 24, (i == next_ && mode_ == Mode::Drive) ? PAL_BOX : PAL_LAMP, false);
        blit(art_.person, x0 - 18, 176, 14, 24, PAL_SIGN, false);
        blit(art_.person, x1 + 16, 176, 14, 24, PAL_SIGN, true);
        blit(art_.person, mid + 36, 178, 12, 20, PAL_SIGN, false);
    }

    float rx = sx(rival_);
    blit(art_.rival, rx - 50, 78, 92, 30, PAL_RIVAL, false);
    float tx = sx(nose_) - kLen * 0.5f;
    blit(art_.train, tx, 140, 112, 42, PAL_TRAIN, false);

    blit(art_.clock, 18, 16, 18, 18, PAL_CLOCK, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(3, "S3 METROBOX", PAL_HUD);
        hudC(6, "STOP INSIDE THE BOX", PAL_HUD);
        hudC(8, "THE CLOCK IS THE OTHER CREW", PAL_HUD);
        hudC(12, "RIGHT GO   LEFT BRAKE", PAL_HUD);
        hudC(14, "DOWN BACK UP THE TRAIN", PAL_HUD);
        hudC(18, "ENTER TO ROLL", PAL_HUD);
        return;
    }

    int sec = int(time_);
    std::snprintf(buf, sizeof buf, "SCORE %d", score_);
    hudText(1, 25, buf, PAL_HUD);
    std::snprintf(buf, sizeof buf, "%d:%02d", sec / 60, sec % 60);
    hudText(16, 25, buf, PAL_HUD);
    int spd = int(std::fabs(speed_));
    std::snprintf(buf, sizeof buf, speed_ < -1.f ? "SPD %02d REV" : "SPD %02d", spd);
    hudText(24, 25, buf, PAL_HUD);

    if (next_ < kStopsN && mode_ != Mode::Win) {
        float left = crewLeft();
        std::snprintf(buf, sizeof buf, "CREW %4.1f", left);
        hudText(28, 1, buf, left < 3.f ? PAL_HUD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d/4 %s", next_ + 1, stop().name);
        hudText(1, 1, buf, PAL_HUD);
        if (insideBox()) hudText(16, 1, "BOX", PAL_HUD);
    }

    if (mode_ == Mode::Pause) {
        hudC(10, "PAUSED", PAL_HUD);
        hudC(12, "ENTER RUN   ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(8, "BOX HELD", PAL_HUD);
        hudC(10, "THE OTHER CREW IS LATE", PAL_HUD);
        hudC(13, "ENTER RIDES AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(8, "LOST THE HEADWAY", PAL_HUD);
        hudC(10, why_ ? why_ : "FAIL", PAL_HUD);
        hudC(13, "ENTER RIDES AGAIN", PAL_HUD);
    } else if (msgT_ > 0.f && msg_[0]) {
        hudC(10, msg_, PAL_HUD);
    } else if (dwell_ > 0.f) {
        hudC(10, "DOORS  HOLD THE BOX", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    if (msgT_ > 0.f) msgT_ -= kDt;

    if (bot_ && mode_ == Mode::Title) begin();

    if (mode_ == Mode::Title) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        rival_ += kRival * 0.25f * kDt;
        if (rival_ > 520.f) rival_ = 160.f;
        audio(kDt);
        lights();
        draw();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Drive;
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) showTitle();
        audio(kDt);
        lights();
        draw();
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) showTitle();
        audio(kDt);
        lights();
        draw();
        return;
    }

    if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
        showTitle();
        audio(kDt);
        lights();
        draw();
        return;
    }
    if (!bot_ && dwell_ <= 0.f && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        audio(kDt);
        lights();
        draw();
        return;
    }

    In in = readInput();
    physics(in, kDt);
    if (mode_ == Mode::Drive) {
        time_ += kDt;
        judge();
    }
    audio(kDt);
    lights();
    draw();
}

}  // namespace metro
