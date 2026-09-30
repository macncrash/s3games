#include "game/rail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace railbox {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float MAX_V = 210.f;
constexpr float ACCEL = 82.f;
constexpr float BRAKE = 128.f;
constexpr float DRAG = 14.f;
constexpr float STOP_V = 8.f;
constexpr float CREW = 16.f;
constexpr float START_POS = 90.f;
constexpr float BOX_X = 1080.f;
constexpr float AIM = BOX_X + 72.f;
constexpr float NOSE_X = 168.f;
constexpr int RAIL_Y = 168;
constexpr int CAB_Y = 118;
constexpr int BOX_Y = 112;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dieselPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.5f;
    p.op[0] = {0.5f, 0.85f, 0.03f, 0.35f, 0.8f, 0.25f};
    p.op[1] = {1.f, 0.65f, 0.02f, 0.3f, 0.65f, 0.2f};
    p.op[2] = {2.f, 0.25f, 0.04f, 0.28f, 0.4f, 0.18f};
    p.op[3] = {3.f, 0.14f, 0.02f, 0.22f, 0.28f, 0.18f};
    p.vol = 0.15f;
    p.drive = 0.4f;
    p.tone = 760.f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.16f, 0.65f, 0.1f};
    p.op[1] = {2.f, 0.4f, 0.01f, 0.14f, 0.4f, 0.1f};
    p.op[2] = {3.f, 0.2f, 0.01f, 0.18f, 0.28f, 0.1f};
    p.op[3] = {4.f, 0.1f, 0.01f, 0.1f, 0.18f, 0.1f};
    p.vol = 0.18f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (held_) return 3;
    if (pos_ >= BOX_X && pos_ <= BOX_X + BOX_W) return 2;
    if (mode_ == Mode::Roll || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blit(const gs::Image& img, float x, float y, int pal, int w, int h, int fog) {
    if (img.w < 1 || img.h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w > 0 ? w : img.w);
    s.h = int16_t(h > 0 ? h : img.h);
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    if (s.x > gs::SCREEN_W + 48 || s.y > gs::SCREEN_H + 24 || s.x + s.w < -48 || s.y + s.h < -32) return;
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c > 95) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudCenter(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hudText(20 - n / 2, row, s, pal);
}

void Game::whistle() {
    sys_->apu.keyOn(1, 698.f, 0.15f);
    whistle_ = 0.38f;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    blip_ = 0.06f;
}

void Game::audio(float dt) {
    if (!engineOn_) {
        sys_->apu.setPatch(0, dieselPatch());
        sys_->apu.setPatch(1, hornPatch());
        sys_->apu.keyOn(0, 34.f, 0.05f);
        engineOn_ = true;
    }
    float hz = 30.f + speed_ * 0.22f;
    float vol = 0.045f + (speed_ / MAX_V) * 0.1f;
    if (mode_ == Mode::Title) {
        hz = 32.f;
        vol = 0.04f;
    }
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, vol);
    if (whistle_ > 0 && mode_ != Mode::Won) {
        whistle_ -= dt;
        if (whistle_ <= 0) sys_->apu.keyOff(1);
    }
    if (blip_ > 0) {
        blip_ -= dt;
        if (blip_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (fanStep_ >= 0) {
        fanT_ += dt;
        if (fanT_ >= 0.14f) {
            fanT_ = 0;
            static const float notes[] = {392.f, 523.f, 659.f, 784.f};
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.16f);
            else sys_->apu.keyOff(1);
            if (++fanStep_ > 6) fanStep_ = -1;
        }
    }
    float left = CREW - time_;
    if (mode_ == Mode::Roll && left < 4.f) sys_->setLight(255, 40, 30);
    else if (mode_ == Mode::Won) sys_->setLight(40, 230, 80);
    else if (mode_ == Mode::Lost) sys_->setLight(255, 30, 30);
    else sys_->setLight(40, 80, 170);
}

void Game::botControls(float& thr, float& brk) const {
    thr = 0;
    brk = 0;
    float bd = (speed_ * speed_) / (2.f * BRAKE);
    bool inside = pos_ >= BOX_X && pos_ <= BOX_X + float(BOX_W);
    if (pos_ > AIM) brk = 1;
    else if (inside && speed_ < 28.f && pos_ + bd >= AIM - 4.f) brk = 1;
    else if (pos_ + bd >= AIM - 6.f && speed_ > 12.f) brk = 1;
    else thr = 1;
}

void Game::startRun() {
    pos_ = START_POS;
    speed_ = 0;
    time_ = 0;
    hold_ = 0;
    thr_ = brk_ = 0;
    won_ = false;
    over_ = false;
    held_ = false;
    wasBraking_ = false;
    fanStep_ = -1;
    why_[0] = 0;
    for (Puff& p : puffs_) p.age = 0;
    mode_ = Mode::Roll;
    whistle();
}

void Game::arrive() {
    speed_ = 0;
    thr_ = brk_ = 0;
    held_ = true;
    if (hold_ < 0.35f) return;
    winRun();
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Lost || mode_ == Mode::Won) return;
    std::snprintf(why_, sizeof why_, "%s", why);
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    speed_ = 0;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.45f, 480.f, 0.32f);
    sys_->rumble(0.75f, 0.45f, 160);
}

void Game::winRun() {
    if (mode_ == Mode::Won) return;
    std::snprintf(why_, sizeof why_, "IN THE BOX");
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    speed_ = 0;
    fanStep_ = 0;
    fanT_ = 0;
    sys_->rumble(0.25f, 0.4f, 140);
}

void Game::updateRoll(float dt) {
    const gs::Pad& pad = sys_->pad;
    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_C)) whistle();

    float thr = 0, brk = 0;
    if (bot_) botControls(thr, brk);
    else {
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A) || pad.down(gs::BTN_Z)) thr = 1;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.down(gs::BTN_X) ||
            pad.down(gs::BTN_TURBO))
            brk = 1;
        thr = std::max(thr, pad.accel);
        brk = std::max(brk, pad.brake);
        if (brk > 0.12f) thr = 0;
    }
    thr_ = thr;
    brk_ = brk;
    if (brk > 0.2f && !wasBraking_ && speed_ > 36.f) sys_->apu.noiseBurst(0.16f, 2000.f, 0.1f);
    wasBraking_ = brk > 0.2f;

    if (brk > 0.f) speed_ -= BRAKE * brk * dt;
    else if (thr > 0.f) speed_ += ACCEL * thr * dt;
    else speed_ -= DRAG * dt;
    speed_ = std::clamp(speed_, 0.f, MAX_V);
    pos_ += speed_ * dt;
    time_ += dt;

    float far = BOX_X + float(BOX_W);
    bool inBox = pos_ >= BOX_X && pos_ <= far;
    if (inBox && speed_ <= STOP_V) {
        hold_ += dt;
        held_ = hold_ > 0.12f;
        if (hold_ >= 0.28f) {
            arrive();
            return;
        }
    } else {
        hold_ = 0;
        held_ = false;
    }
    if (pos_ > far) {
        fail("RAN THE BOX");
        return;
    }
    if (speed_ <= STOP_V && pos_ < BOX_X && time_ > 1.2f && thr_ < 0.2f) {
        fail("STOPPED SHORT");
        return;
    }
    if (time_ >= CREW) fail("OTHER CREW");

    puffT_ += dt;
    float gap = speed_ > 40.f ? 0.08f : 0.18f;
    if (speed_ > 6.f && puffT_ >= gap) {
        puffT_ = 0;
        int slot = 0;
        for (int i = 0; i < 8; i++)
            if (puffs_[i].age <= 0) {
                slot = i;
                break;
            }
        puffs_[slot].x = pos_ - 96.f;
        puffs_[slot].age = 0.01f;
    }
    for (Puff& p : puffs_) {
        if (p.age <= 0) continue;
        p.age += dt;
        if (p.age > 0.8f) p.age = 0;
    }
}

void Game::sky() {
    const uint16_t top = gs::rgb4(2, 5, 11);
    const uint16_t hor = gs::rgb4(13, 10, 8);
    const uint16_t field = gs::rgb4(3, 7, 3);
    const uint16_t bed = gs::rgb4(5, 4, 3);
    sys_->vdp.setFogColor(hor);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 140) c = lerpC(top, hor, y / 140.f);
        else if (y < RAIL_Y) c = lerpC(hor, field, (y - 140) / float(RAIL_Y - 140));
        else c = lerpC(bed, gs::rgb4(3, 3, 2), (y - RAIL_Y) / float(gs::SCREEN_H - RAIL_Y));
        sys_->vdp.lineBackdrop[y] = c;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawPoster() {
    blit(art_.logo, (gs::SCREEN_W - art_.logo.w) * 0.5f, 16, PAL_AMBER);
    blit(art_.tag, (gs::SCREEN_W - art_.tag.w) * 0.5f, 48, PAL_RED);
    float age = std::fmod(t_, 0.7f);
    blit(art_.puff, 40.f, float(CAB_Y) - age * 24.f, PAL_FX, 8 + int(age * 12), 8 + int(age * 12), int(age * 8));
    blit(art_.cab[int(t_ * 6.f) % 2], 8, float(CAB_Y), PAL_CAB);
    blit(art_.box, 150, float(BOX_Y), PAL_BOX);
    blit(art_.clock, 286, float(RAIL_Y - 46), PAL_CREW);
    int seg = std::max(1, int(art_.rail.w));
    for (int x = -seg; x < gs::SCREEN_W + seg; x += seg) blit(art_.rail, float(x), float(RAIL_Y), PAL_LAND);
    blit(art_.tree, 250, 118, PAL_LAND, 26, 42, 2);
    blit(art_.hill, 40, 128, PAL_LAND, 0, 0, 6);
    blit(art_.sun, 270, 28, PAL_FX);
    float drift = std::fmod(t_ * 12.f, 360.f);
    blit(art_.cloud, drift - 40.f, 64, PAL_LAND);
}

void Game::drawRun() {
    float cam = pos_ - NOSE_X;
    int phase = int(pos_ / 7.f) % 2;
    if (phase < 0) phase = 0;
    blit(art_.cab[phase], NOSE_X - art_.cab[phase].w, float(CAB_Y), PAL_CAB);
    for (const Puff& p : puffs_) {
        if (p.age <= 0) continue;
        int sz = 8 + int(p.age * 14);
        blit(art_.puff, p.x - cam, CAB_Y - p.age * 30.f, PAL_FX, sz, sz, int(p.age * 10));
    }
    blit(art_.box, BOX_X - cam, float(BOX_Y), PAL_BOX);
    blit(art_.clock, BOX_X + BOX_W + 8.f - cam, float(RAIL_Y - 46), PAL_CREW);
    int seg = std::max(1, int(art_.rail.w));
    int i0 = int(std::floor(cam / seg)) - 1;
    for (int i = i0; i < i0 + 16; i++) blit(art_.rail, float(i * seg) - cam, float(RAIL_Y), PAL_LAND);

    auto band = [&](int spacing, int y, const gs::Image& img, int fog, float parallax) {
        float c = cam * parallax;
        int a = int(std::floor(c / spacing)) - 2;
        for (int i = a; i < a + 10; i++) {
            float sx = float(i * spacing) - c;
            int bob = int((unsigned(i) * 13u) % 5u) - 2;
            blit(img, sx, float(y + bob), PAL_LAND, 0, 0, fog);
        }
    };
    band(180, 116, art_.tree, 3, 0.7f);
    band(250, 122, art_.hill, 8, 0.32f);
    band(320, 42, art_.cloud, 1, 0.12f);
    blit(art_.sun, 248, 26, PAL_FX);
}

void Game::drawBanner() {
    const gs::Image* img = nullptr;
    int pal = PAL_TEXT;
    if (mode_ == Mode::Won) {
        img = &art_.banIn;
        pal = PAL_GREEN;
    } else if (mode_ == Mode::Lost) {
        if (std::strcmp(why_, "RAN THE BOX") == 0) img = &art_.banRan;
        else if (std::strcmp(why_, "STOPPED SHORT") == 0) img = &art_.banShort;
        else img = &art_.banCrew;
        pal = PAL_RED;
    } else if (held_) {
        img = &art_.banIn;
        pal = PAL_AMBER;
    }
    if (!img) return;
    blit(*img, (gs::SCREEN_W - img->w) * 0.5f, 64, pal);
}

void Game::hud() {
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) {
        hudCenter(10, "TAKE THE RAIL", PAL_TEXT);
        hudCenter(12, "Z THROTTLE    X BRAKE    C HORN", PAL_DIM);
        if (int(t_ * 2.f) % 2 == 0) hudCenter(22, "ENTER TO ROLL", PAL_AMBER);
        hudCenter(25, "THE CLOCK IS THE OTHER CREW", PAL_RED);
        return;
    }
    char line[48];
    float left = std::max(0.f, CREW - time_);
    std::snprintf(line, sizeof line, "CREW %4.1f", left);
    int timePal = left < 4.f ? PAL_RED : left < 7.f ? PAL_AMBER : PAL_TEXT;
    hudText(1, 0, "WEST BOX", PAL_DIM);
    hudText(28, 0, line, timePal);
    if (mode_ == Mode::Roll || mode_ == Mode::Pause) {
        if (pos_ >= BOX_X && pos_ <= BOX_X + BOX_W) std::snprintf(line, sizeof line, "IN BOX");
        else std::snprintf(line, sizeof line, "BOX %4d", int(std::max(0.f, BOX_X - pos_) + 0.5f));
        hudText(1, 1, line, pos_ >= BOX_X ? PAL_AMBER : PAL_DIM);
        std::snprintf(line, sizeof line, "SPD %3d", int(speed_ * 0.5f + 0.5f));
        hudText(14, 1, line, PAL_TEXT);
        if (mode_ == Mode::Pause) hudCenter(6, "PAUSED", PAL_AMBER);
        else if (time_ < 2.4f) hudCenter(4, "STOP THE NOSE IN THE BOX", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        std::snprintf(line, sizeof line, "BEAT CREW  %4.1f", time_);
        hudCenter(12, line, PAL_GREEN);
        if (!bot_) hudCenter(16, "ENTER FOR ANOTHER RUN", PAL_DIM);
    } else if (mode_ == Mode::Lost) {
        hudCenter(12, why_, PAL_RED);
        if (!bot_) hudCenter(16, "ENTER TO TRY AGAIN", PAL_DIM);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sky();
    if (mode_ == Mode::Title) drawPoster();
    else {
        drawBanner();
        drawRun();
    }
    hud();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.resize(64, 32);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.2f, 0.12f);
    engineOn_ = false;
    if (bot_) startRun();
    else mode_ = Mode::Title;
    audio(DT);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    audio(DT);
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Roll;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && pad.pressed(gs::BTN_START)) startRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
    } else updateRoll(DT);
    draw();
}

}  // namespace railbox
