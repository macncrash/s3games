#include "game/metro.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace metro {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int NSTOP = 5;
constexpr float BOT_K = 0.78f;  // brake early so the mark dies at the middle of the box

struct Station {
    const char* name;
    const char* rail;
    float approach, speed, box, brake, power, coast;
    int theme;
};

const Station STATIONS[NSTOP] = {
    {"HARBOR", "DRY", 62.f, 10.5f, 4.0f, 1.70f, 0.65f, 0.12f, 0},
    {"MARKET", "DRY", 58.f, 12.0f, 2.8f, 1.85f, 0.70f, 0.10f, 1},
    {"CINDER", "WET", 66.f, 11.0f, 2.4f, 1.15f, 0.50f, 0.06f, 2},
    {"VIADUCT", "DRY", 50.f, 13.0f, 1.8f, 2.05f, 0.75f, 0.10f, 3},
    {"TERMINAL", "DRY", 46.f, 12.5f, 1.4f, 1.90f, 0.60f, 0.05f, 4},
};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto mix = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(mix(8), mix(4), mix(0));
}

char up(char c) {
    if (c >= 'a' && c <= 'z') return char(c - 32);
    return c;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    tries_ = 4;
    score_ = 0;
    won_ = false;
    over_ = false;
    t_ = 0;
    reason_ = "unfinished";
    if (bot_) startLine();
    else {
        mode_ = Mode::Title;
        titlePose();
    }
    sys.apu.setMaster(0.85f);
}

void Game::titlePose() {
    station_ = 0;
    boxW_ = 4.f;
    boxL_ = -2.f;
    boxR_ = 2.f;
    nose_ = 0.f;
    speed_ = 0.f;
    hold_ = 0;
    theme_ = 0;
    wasIn_ = true;
    braking_ = false;
    powering_ = false;
}

void Game::armStation() {
    const Station& s = STATIONS[station_];
    boxW_ = s.box;
    boxL_ = -s.box * 0.5f;
    boxR_ = s.box * 0.5f;
    nose_ = boxL_ - s.approach;
    speed_ = s.speed;
    hold_ = 0;
    joint_ = 0;
    wasIn_ = false;
    braking_ = false;
    powering_ = false;
    theme_ = s.theme;
}

void Game::startLine() {
    score_ = 0;
    tries_ = 4;
    station_ = 0;
    won_ = false;
    over_ = false;
    shake_ = 0;
    reason_ = "unfinished";
    armStation();
    mode_ = Mode::Brief;
    bannerT_ = 0;
}

bool Game::goPressed() const {
    if (bot_) return false;
    return sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A);
}

int Game::botCmd() const {
    const Station& s = STATIONS[station_];
    float dist = -nose_;
    float vdes = dist > 0.f ? std::sqrt(2.f * s.brake * dist * BOT_K) : 0.f;
    int cmd = 0;
    if (speed_ > vdes + 0.015f) cmd = -1;
    else if (dist > 0.12f && speed_ < 0.22f && nose_ < boxL_ + 0.02f) cmd = 1;
    else if (dist > 6.f && speed_ < vdes * 0.72f) cmd = 1;
    if (nose_ >= boxL_ && nose_ <= boxR_ && std::fabs(dist) <= s.box * 0.5f && speed_ < 0.15f && dist < 0.35f)
        cmd = speed_ > 0.04f ? -1 : 0;
    if (cmd > 0 && nose_ > boxR_ - 0.35f) cmd = -1;
    return cmd;
}

void Game::physics(float power, float brake) {
    const Station& s = STATIONS[station_];
    power = std::clamp(power, 0.f, 1.f);
    brake = std::clamp(brake, 0.f, 1.f);
    if (brake >= power && brake > 0.02f) power = 0.f;
    else brake = 0.f;
    float a;
    if (power > 0.f) a = s.power * power;
    else if (brake > 0.f) a = -s.brake * brake;
    else a = speed_ > 0.f ? -s.coast : 0.f;
    braking_ = brake > 0.02f;
    powering_ = power > 0.02f;
    speed_ = std::max(0.f, speed_ + a * DT);
    if (speed_ > 28.f) speed_ = 28.f;
    nose_ += speed_ * DT;
    odo_ += speed_ * DT;
    joint_ += speed_ * DT;
    if (joint_ >= 6.f) {
        joint_ -= 6.f;
        if (speed_ > 1.f) sys_->apu.noiseBurst(0.08f, 1400.f, 0.028f);
    }
}

void Game::enterDock() {
    int errCm = int(std::lround(std::fabs(nose_) * 100.f));
    int halfCm = std::max(1, int(std::lround(boxW_ * 50.f)));
    int bonus = (halfCm - errCm) * 500 / halfCm;
    if (bonus < 0) bonus = 0;
    lastAward_ = 1000 + bonus;
    score_ += lastAward_;
    errCm_ = errCm;
    mode_ = Mode::Dock;
    bannerT_ = 0;
    speed_ = 0;
    if (!sys_->headless) sys_->rumble(0.18f, 0.32f, 110);
}

void Game::enterOvershoot() {
    mode_ = Mode::Overshoot;
    tries_--;
    bannerT_ = 0;
    shake_ = 1.f;
    speed_ = 0;
    reason_ = "overshoot";
    if (!sys_->headless) sys_->rumble(0.9f, 0.45f, 220);
}

void Game::update() {
    t_ += DT;
    if (shake_ > 0.f) {
        shakeX_ = std::sin(t_ * 46.f) * 3.2f * shake_;
        shakeY_ = std::cos(t_ * 37.f) * 2.0f * shake_;
        shake_ *= 0.90f;
        if (shake_ < 0.04f) shake_ = 0;
    } else {
        shakeX_ = shakeY_ = 0;
    }

    if (!bot_ && sys_->pad.pressed(gs::BTN_MODE)) {
        if (mode_ == Mode::Title) {
            if (sys_->hasHome()) sys_->eject();
        } else {
            mode_ = Mode::Title;
            titlePose();
            sys_->apu.silence();
        }
        return;
    }

    switch (mode_) {
    case Mode::Title:
        if (goPressed()) startLine();
        break;
    case Mode::Brief:
        bannerT_++;
        if (bannerT_ > 90 || goPressed()) {
            mode_ = Mode::Run;
            runT_ = 0;
            hold_ = 0;
            bannerT_ = 0;
        }
        break;
    case Mode::Pause:
        if (!bot_ && sys_->pad.pressed(gs::BTN_START)) mode_ = held_;
        break;
    case Mode::Run: {
        if (!bot_ && sys_->pad.pressed(gs::BTN_START)) {
            held_ = Mode::Run;
            mode_ = Mode::Pause;
            break;
        }
        runT_++;
        float power = 0, brake = 0;
        if (bot_) {
            int c = botCmd();
            if (c > 0) power = 1.f;
            if (c < 0) brake = 1.f;
        } else {
            if (sys_->pad.down(gs::BTN_DOWN) || sys_->pad.down(gs::BTN_LEFT) || sys_->pad.down(gs::BTN_B) ||
                sys_->pad.down(gs::BTN_TURBO))
                brake = 1.f;
            if (sys_->pad.down(gs::BTN_UP) || sys_->pad.down(gs::BTN_RIGHT) || sys_->pad.down(gs::BTN_A)) power = 1.f;
            brake = std::max(brake, sys_->pad.brake);
            power = std::max(power, sys_->pad.accel);
        }
        physics(power, brake);
        // The far edge is a hard fail, even at a crawl. Stopping short is not.
        if (nose_ > boxR_ + 0.001f) {
            enterOvershoot();
            break;
        }
        bool in = nose_ >= boxL_ - 0.001f && nose_ <= boxR_ + 0.001f;
        if (in && !wasIn_) {
            sys_->apu.noiseBurst(0.12f, 2200.f, 0.02f);
            if (!sys_->headless) sys_->rumble(0.12f, 0.08f, 40);
        }
        wasIn_ = in;
        if (in && speed_ < 0.10f) {
            if (++hold_ >= 15) enterDock();
        } else {
            hold_ = 0;
        }
        break;
    }
    case Mode::Dock:
        bannerT_++;
        braking_ = false;
        powering_ = false;
        if (bannerT_ > 80 || goPressed()) {
            if (station_ + 1 >= NSTOP) {
                mode_ = Mode::Victory;
                won_ = true;
                over_ = true;
                reason_ = nullptr;
                bannerT_ = 0;
            } else {
                station_++;
                armStation();
                mode_ = Mode::Brief;
                bannerT_ = 0;
            }
        }
        break;
    case Mode::Overshoot:
        bannerT_++;
        braking_ = false;
        powering_ = false;
        if (bannerT_ > 80 || goPressed()) {
            if (tries_ <= 0) {
                mode_ = Mode::Over;
                won_ = false;
                over_ = true;
                bannerT_ = 0;
            } else {
                armStation();
                mode_ = Mode::Brief;
                bannerT_ = 0;
            }
        }
        break;
    case Mode::Victory:
    case Mode::Over:
        bannerT_++;
        if (!bot_ && (sys_->pad.pressed(gs::BTN_START) || sys_->pad.pressed(gs::BTN_A))) startLine();
        break;
    }
    sound();
}

void Game::sound() {
    gs::APU& a = sys_->apu;
    bool run = mode_ == Mode::Run;
    if (run && (speed_ > 0.15f || powering_)) {
        float f = 46.f + speed_ * 6.5f + (powering_ ? 14.f : 0.f);
        float v = 0.022f + std::min(speed_ * 0.004f, 0.05f) + (powering_ ? 0.028f : 0.f);
        a.tone(0, f, v);
    } else if (mode_ == Mode::Title) {
        a.tone(0, 72.f, 0.012f);
    } else {
        a.tone(0, 0, 0);
    }
    if (run && braking_ && speed_ > 0.4f) a.tone(1, 190.f + speed_ * 24.f, 0.04f);
    else a.tone(1, 0, 0);

    if (mode_ == Mode::Dock || mode_ == Mode::Victory) {
        static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f};
        int step = bannerT_ / 10;
        if (step >= 0 && step < 5) a.tone(2, notes[step], 0.05f);
        else a.tone(2, 0, 0);
    } else if (mode_ == Mode::Overshoot) {
        a.tone(2, 92.f, bannerT_ < 28 ? 0.06f : 0.f);
    } else {
        a.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    update();
    draw();
}

int Game::marker() const {
    switch (mode_) {
    case Mode::Title: return 0;
    case Mode::Run: return 1;
    case Mode::Dock: return 2;
    case Mode::Overshoot: return 3;
    case Mode::Victory: return 4;
    default: return 0;
    }
}

std::string Game::report() const {
    char b[180];
    if (won_)
        std::snprintf(b, sizeof b, "S3 METRO  WIN  stopped in the box on every stop  score %d", score_);
    else
        std::snprintf(b, sizeof b, "S3 METRO  FAIL  %s  score %d", reason_ ? reason_ : "unfinished", score_);
    return b;
}

float Game::sx(float meters, float par) const {
    return NOSE_X + (meters - nose_) * PPM * par;
}

void Game::blit(const gs::Mipped& m, float left, float top, int pal, bool flip) {
    if (m.h < 1 || m.w < 1) return;
    left += shakeX_;
    top += shakeY_;
    float w = float(m.w), h = float(m.h);
    if (left > gs::SCREEN_W + 4 || top > gs::SCREEN_H + 4 || left + w < -8 || top + h < -8) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(left));
    s.y = int16_t(std::lround(top));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 17.f * scale;
    float width = float(s.size()) * adv;
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(up(s[i]));
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float gh = g.h * scale;
        float gw = gh * float(g.w) / float(std::max(1, g.h));
        float cx = x + float(i) * adv + gw * 0.5f + shakeX_;
        float cy = y + shakeY_;
        if (gh < 1.f) continue;
        gs::Sprite sp;
        sp.w = int16_t(std::max(1L, std::lround(gw)));
        sp.h = int16_t(std::max(1L, std::lround(gh)));
        sp.x = int16_t(std::lround(cx - sp.w * 0.5f));
        sp.y = int16_t(std::lround(cy - sp.h * 0.5f));
        sp.img = g.pick(gh);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(up(s[i]));
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::backdrop() {
    const Theme& th = themeFor(theme_);
    gs::VDP& v = sys_->vdp;
    v.B.enabled = !th.open;
    float par = 0.5f;
    int hs = -int(std::lround(nose_ * PPM * par));
    v.B.scroll(hs, 0);
    v.setFogColor(th.pit);
    bool flash = mode_ == Mode::Overshoot && bannerT_ < 18;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c = y < 150 ? lerpC(th.top, th.mid, y / 150.f) : lerpC(th.mid, th.pit, (y - 150) / 74.f);
        if (flash && (y & 1)) c = lerpC(c, gs::rgb4(12, 1, 1), 0.55f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    v.setColor(PAL_WALL * 16 + 1, th.tile);
    v.setColor(PAL_WALL * 16 + 2, th.tileDk);
    v.setColor(PAL_WALL * 16 + 4, th.postA);
    v.setColor(PAL_WALL * 16 + 5, th.postB);
    float pulse = 0.72f + 0.28f * std::sin(t_ * 3.1f);
    v.setColor(PAL_LAMP * 16 + 2, lerpC(th.glow, gs::rgb4(15, 15, 14), pulse * 0.35f));

    bool in = nose_ + 0.001f >= boxL_ && nose_ <= boxR_ + 0.001f;
    v.setColor(PAL_PLAT * 16 + 4, in ? gs::rgb4(4, 14, 5) : gs::rgb4(15, 13, 2));
    v.setColor(PAL_PLAT * 16 + 5, in ? gs::rgb4(2, 8, 3) : gs::rgb4(11, 8, 1));
    v.setColor(PAL_POST * 16 + 1, in ? gs::rgb4(6, 15, 5) : gs::rgb4(15, 13, 2));
    v.setColor(PAL_POST * 16 + 3, in ? gs::rgb4(2, 9, 3) : gs::rgb4(10, 8, 1));

    int aspect = 1;
    if (mode_ == Mode::Title || mode_ == Mode::Dock || mode_ == Mode::Victory || (in && speed_ < 0.45f)) aspect = 2;
    else if ((in && speed_ >= 0.45f) || (nose_ > boxL_ - 8.f && speed_ > 7.f && mode_ == Mode::Run)) aspect = 0;
    uint16_t lens = aspect == 0 ? gs::rgb4(15, 2, 2) : aspect == 2 ? gs::rgb4(3, 14, 4) : gs::rgb4(15, 11, 2);
    v.setColor(PAL_SIGNAL * 16 + 2, lens);

    if (!sys_->headless) {
        if (mode_ == Mode::Overshoot || mode_ == Mode::Over) sys_->setLight(230, 30, 24);
        else if (in) sys_->setLight(40, 170, 70);
        else if (braking_) sys_->setLight(230, 130, 30);
        else sys_->setLight(40, 90, 180);
    }
}

void Game::banner() {
    char buf[64];
    bool plate = false;
    if (mode_ == Mode::Title) {
        text("S3 METRO", 160, 24, 1.2f, PAL_AMBER);
        text("STOP IN THE BOX", 160, 50, 0.72f, PAL_WHITE);
        text("OVERSHOOT IS A FAIL", 160, 72, 0.55f, PAL_RED);
        plate = true;
    } else if (mode_ == Mode::Brief) {
        const Station& s = STATIONS[station_];
        text(s.name, 160, 22, 1.05f, PAL_AMBER);
        std::snprintf(buf, sizeof buf, "%s RAIL", s.rail);
        text(buf, 160, 46, 0.62f, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "BOX %.1fM", s.box);
        text(buf, 160, 66, 0.55f, PAL_GREEN);
        text("OVERSHOOT IS A FAIL", 160, 84, 0.5f, PAL_RED);
        plate = true;
    } else if (mode_ == Mode::Dock) {
        text("IN THE BOX", 160, 26, 1.0f, PAL_GREEN);
        if (errCm_ <= 0) std::snprintf(buf, sizeof buf, "ON THE MARK");
        else std::snprintf(buf, sizeof buf, "OFF BY %d CM", errCm_);
        text(buf, 160, 52, 0.6f, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "+%d", lastAward_);
        text(buf, 160, 74, 0.7f, PAL_AMBER);
        plate = true;
    } else if (mode_ == Mode::Overshoot) {
        text("OVERSHOOT", 160, 30, 1.15f, PAL_RED);
        text("PAST THE BOX", 160, 56, 0.6f, PAL_WHITE);
        if (tries_ > 0) std::snprintf(buf, sizeof buf, "%d TRIES LEFT", tries_);
        else std::snprintf(buf, sizeof buf, "NO TRIES LEFT");
        text(buf, 160, 78, 0.55f, PAL_AMBER);
        plate = true;
    } else if (mode_ == Mode::Victory) {
        text("LINE CLEAR", 160, 28, 1.05f, PAL_AMBER);
        text("EVERY STOP IN THE BOX", 160, 54, 0.52f, PAL_GREEN);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        text(buf, 160, 76, 0.6f, PAL_WHITE);
        plate = true;
    } else if (mode_ == Mode::Over) {
        text("LINE CLOSED", 160, 30, 1.0f, PAL_RED);
        text("TOO MANY OVERSHOOTS", 160, 56, 0.5f, PAL_WHITE);
        text("ENTER TO RIDE AGAIN", 160, 78, 0.5f, PAL_AMBER);
        plate = true;
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 48, 1.1f, PAL_WHITE);
        plate = true;
    } else if (mode_ == Mode::Run && runT_ < 36) {
        text("GO", 160, 40, 1.0f, PAL_AMBER);
    }
    if (plate) blit(art_.plate, 40, 4, PAL_WALL);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    if (mode_ == Mode::Title) theme_ = 0;
    else if (station_ >= 0 && station_ < NSTOP) theme_ = STATIONS[station_].theme;
    const Theme& th = themeFor(theme_);
    backdrop();
    banner();

    if (th.wet && mode_ == Mode::Run) {
        for (int i = 0; i < 14; i++) {
            float rx = std::fmod(i * 47.f + t_ * 90.f, 360.f) - 24.f;
            float ry = std::fmod(i * 31.f + t_ * 160.f, 250.f) - 16.f;
            blit(art_.rain, rx, ry, PAL_DIM);
        }
    }

    bool in = nose_ + 0.001f >= boxL_ && nose_ <= boxR_ + 0.001f;
    blit(art_.chevron, NOSE_X - 11.f, TRAIN_Y + 26.f, PAL_FX);

    if (sx(boxL_) > 312.f && (int(t_ * 3.f) & 1)) blit(art_.chevron, 300.f, TRAIN_Y + 28.f, PAL_AMBER);

    float edgeY = TRAIN_Y + 8.f;
    blit(art_.edge, sx(boxL_) - 1.f, edgeY, in ? PAL_GREEN : PAL_AMBER);
    blit(art_.edge, sx(boxR_) - 1.f, edgeY, PAL_RED);
    blit(art_.post, sx(boxL_) - 3.f, 78.f, PAL_POST);
    blit(art_.post, sx(boxR_) - 3.f, 78.f, PAL_RED);

    float x0 = sx(boxL_);
    float x1 = sx(boxR_);
    for (float x = x0; x < x1 - 0.5f; x += float(art_.boxSeg.w)) blit(art_.boxSeg, x, PLAT_Y + 2.f, PAL_PLAT);
    blit(art_.mark, sx(0.f) - 1.f, PLAT_Y + 1.f, PAL_WHITE);

    static const float peepAt[] = {-18.f, -12.f, -7.5f, 7.f, 12.f, 18.f};
    for (int i = 0; i < 6; i++) {
        float bob = std::sin(t_ * 2.4f + i * 1.3f) * 1.3f;
        blit(art_.person[i & 3], sx(peepAt[i]) - 9.f, 178.f + bob, PAL_PEEP, i == 2 || i == 4);
    }

    int wframe = int(std::floor(odo_ * 3.f)) % 3;
    if (wframe < 0) wframe += 3;
    float leadLeft = NOSE_X - float(LEAD_TIP);
    auto bogies = [&](float left, int b0, int b1) {
        if (braking_ && speed_ > 0.8f) {
            blit(art_.spark, left + b0 - 4.f, TRAIN_Y + 60.f, PAL_FX);
            blit(art_.spark, left + b1 - 4.f, TRAIN_Y + 60.f, PAL_FX);
        }
        blit(art_.wheel[wframe], left + b0 - 11.f, TRAIN_Y + 50.f, PAL_TRAIN);
        blit(art_.wheel[(wframe + 1) % 3], left + b1 - 11.f, TRAIN_Y + 50.f, PAL_TRAIN);
    };
    bogies(leadLeft, LEAD_BOGIE0, LEAD_BOGIE1);
    float midLeft = leadLeft - 6.f - float(art_.mid.w);
    bogies(midLeft, MID_BOGIE0, MID_BOGIE1);
    blit(art_.lead, leadLeft, TRAIN_Y, PAL_TRAIN);
    blit(art_.mid, midLeft, TRAIN_Y, PAL_TRAIN);

    const char* name = mode_ == Mode::Title ? "HARBOR" : STATIONS[station_].name;
    blit(art_.sign, sx(-5.5f) - 48.f, 34.f, PAL_WALL);
    text(name, sx(-5.5f), 46.f, 0.62f, PAL_AMBER);
    blit(art_.signal, sx(boxR_ + 2.2f) - 7.f, TRAIN_Y + 8.f, PAL_SIGNAL);

    static const float lampAt[] = {-22.f, -10.f, 9.f, 20.f};
    for (float m : lampAt) blit(art_.lamp, sx(m) - 8.f, 118.f, PAL_LAMP);

    float mLeft = nose_ + (-20.f - NOSE_X) / PPM;
    float mRight = nose_ + (340.f - NOSE_X) / PPM;
    int i0 = int(std::floor(mLeft / 8.f)) - 1;
    int i1 = int(std::ceil(mRight / 8.f)) + 1;
    for (int i = i0; i <= i1; i++) {
        blit(art_.rail, sx(i * 8.f), RAIL_Y, PAL_PLAT);
        blit(art_.slab, sx(i * 8.f), PLAT_Y, PAL_PLAT);
    }

    float span0 = nose_ + (-40.f - NOSE_X) / (PPM * 0.5f);
    float span1 = nose_ + (380.f - NOSE_X) / (PPM * 0.5f);
    int p0 = int(std::floor(span0 / 7.f)) - 1;
    int p1 = int(std::ceil(span1 / 7.f)) + 1;
    for (int i = p0; i <= p1; i++) {
        float x = NOSE_X + (i * 7.f - nose_) * PPM * 0.5f;
        if (!th.open && (i % 3 != 0)) blit(art_.poster[(i < 0 ? -i : i) % 3], x, 48.f, PAL_WALL);
    }
    if (!th.open) {
        float q0 = nose_ + (-30.f - NOSE_X) / (PPM * 0.32f);
        float q1 = nose_ + (360.f - NOSE_X) / (PPM * 0.32f);
        int a = int(std::floor(q0 / 5.f)) - 1;
        int b = int(std::ceil(q1 / 5.f)) + 1;
        for (int i = a; i <= b; i++) {
            float x = NOSE_X + (i * 5.f - nose_) * PPM * 0.32f;
            blit(art_.pipe, x, 14.f, PAL_WALL);
        }
    } else {
        float g0 = nose_ + (-20.f - NOSE_X) / (PPM * 0.75f);
        float g1 = nose_ + (360.f - NOSE_X) / (PPM * 0.75f);
        int a = int(std::floor(g0 / 3.5f)) - 1;
        int b = int(std::ceil(g1 / 3.5f)) + 1;
        for (int i = a; i <= b; i++) {
            float x = NOSE_X + (i * 3.5f - nose_) * PPM * 0.75f;
            blit(art_.girder, x, 8.f, PAL_WALL);
        }
        for (int i = 0; i < 4; i++) {
            float m = nose_ * 0.15f + i * 18.f;
            float x = std::fmod(m, 72.f);
            if (x < 0) x += 72.f;
            blit(art_.cloud, x * 5.f - 30.f, 18.f + (i & 1) * 16.f, PAL_WHITE);
        }
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hud(2, 24, "UP/Z POWER    DOWN/X BRAKE", PAL_WHITE);
        hud(4, 25, "LEFT BRAKE    RIGHT POWER", PAL_DIM);
        if ((int(t_ * 2.f) & 1) == 0) hud(8, 26, "ENTER STARTS THE LINE", PAL_AMBER);
        hud(4, 27, "PAST THE RED EDGE FAILS", PAL_RED);
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause || mode_ == Mode::Brief) {
        const Station& s = STATIONS[station_];
        std::snprintf(buf, sizeof buf, "%s", s.name);
        hud(1, 0, buf, PAL_AMBER);
        std::snprintf(buf, sizeof buf, "%d/%d", station_ + 1, NSTOP);
        hud(36, 0, buf, PAL_WHITE);
        int kmh = int(std::lround(speed_ * 3.6f));
        std::snprintf(buf, sizeof buf, "%3d KM/H", kmh);
        hud(1, 1, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "%s", s.rail);
        hud(12, 1, buf, s.rail[0] == 'W' ? PAL_GREEN : PAL_DIM);
        std::snprintf(buf, sizeof buf, "TRIES %d", tries_);
        hud(30, 1, buf, tries_ > 1 ? PAL_WHITE : PAL_RED);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hud(1, 2, buf, PAL_DIM);

        const char* act = "COAST";
        int actPal = PAL_DIM;
        if (powering_) {
            act = "POWER";
            actPal = PAL_AMBER;
        } else if (braking_) {
            act = "BRAKE";
            actPal = PAL_RED;
        } else if (mode_ == Mode::Run && speed_ < 0.25f && nose_ < boxL_ - 0.05f) {
            act = "POWER UP";
            actPal = PAL_AMBER;
        }
        hud(22, 2, act, actPal);

        if (in && speed_ >= 0.4f) hud(1, 3, "BRAKE NOW", PAL_RED);
        else if (in) hud(1, 3, "IN THE BOX", PAL_GREEN);
        else if (nose_ < boxL_) {
            float m = boxL_ - nose_;
            if (m >= 10.f) std::snprintf(buf, sizeof buf, "BOX %dM", int(std::lround(m)));
            else std::snprintf(buf, sizeof buf, "BOX %.1fM", m);
            hud(1, 3, buf, PAL_AMBER);
        }
    } else if (mode_ == Mode::Victory) {
        hud(8, 26, "ENTER RIDES AGAIN", PAL_AMBER);
    } else if (mode_ == Mode::Over) {
        hud(7, 26, "ENTER TRIES THE LINE", PAL_AMBER);
    }
}

}  // namespace metro
