#include "game/rail.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rail {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float MAX_V = 210.f;
constexpr float ACCEL = 78.f;
constexpr float BRAKE = 125.f;
constexpr float DRAG = 18.f;
constexpr float STOP_V = 14.f;
constexpr float NOSE_X = 120.f;
constexpr float START_POS = 80.f;
constexpr int NSTOP = 4;
constexpr int RAIL_Y = 168;
constexpr int STATION_Y = 116;
constexpr int LOCO_Y = 112;
constexpr int NSTOPS = NSTOP;

struct Stop {
    const char* name;
    float x;
    float limit;
};

// Deadlines have slack over a full-throttle stop. They are not a target minute.
constexpr Stop STOPS[NSTOP] = {
    {"MILL", 1050.f, 12.0f},
    {"PIER", 2450.f, 13.5f},
    {"FORK", 4150.f, 15.0f},
    {"TERM", 5350.f, 12.5f},
};

float brakeReach() { return (MAX_V * MAX_V) / (2.f * BRAKE); }

float boardX(const Stop& s) { return s.x + float(BOX_W) - 28.f - brakeReach(); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch dieselPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.55f;
    p.op[0] = {0.5f, 0.9f, 0.03f, 0.4f, 0.85f, 0.3f};
    p.op[1] = {1.f, 0.7f, 0.02f, 0.35f, 0.7f, 0.25f};
    p.op[2] = {2.f, 0.28f, 0.04f, 0.3f, 0.45f, 0.2f};
    p.op[3] = {3.f, 0.16f, 0.02f, 0.25f, 0.3f, 0.2f};
    p.vol = 0.16f;
    p.drive = 0.45f;
    p.tone = 800.f;
    return p;
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.2f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.18f, 0.7f, 0.12f};
    p.op[1] = {2.f, 0.45f, 0.01f, 0.16f, 0.45f, 0.1f};
    p.op[2] = {3.f, 0.22f, 0.01f, 0.2f, 0.3f, 0.1f};
    p.op[3] = {4.5f, 0.12f, 0.01f, 0.12f, 0.2f, 0.1f};
    p.vol = 0.2f;
    return p;
}

}  // namespace

void Game::blit(const gs::Image& img, float x, float y, int pal, int w, int h, int fog) {
    if (img.w < 1 || img.h < 1) return;
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(w > 0 ? w : img.w);
    s.h = int16_t(h > 0 ? h : img.h);
    if (s.w < 1 || s.h < 1) return;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    if (s.x > gs::SCREEN_W + 40 || s.y > gs::SCREEN_H + 20 || s.x + s.w < -40 || s.y + s.h < -30) return;
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
    sys_->apu.keyOn(1, 740.f, 0.16f);
    whistle_ = 0.42f;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    blip_ = 0.06f;
}

void Game::audio(float dt) {
    if (!engineOn_) {
        sys_->apu.setPatch(0, dieselPatch());
        sys_->apu.setPatch(1, hornPatch());
        sys_->apu.keyOn(0, 36.f, 0.05f);
        engineOn_ = true;
    }
    float hz = 32.f + speed_ * 0.2f;
    float vol = 0.05f + (speed_ / MAX_V) * 0.1f;
    if (mode_ == Mode::Title) {
        hz = 34.f;
        vol = 0.045f;
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
    if (mode_ == Mode::Roll) {
        const Stop& s = STOPS[leg_];
        float left = s.limit - legTime_;
        if (left < 4.f) sys_->setLight(255, 40, 30);
        else sys_->setLight(40, 160, 70);
    } else if (mode_ == Mode::Won) sys_->setLight(40, 255, 90);
    else if (mode_ == Mode::Lost) sys_->setLight(255, 20, 20);
    else sys_->setLight(40, 70, 160);
}

void Game::botControls(float& thr, float& brk) const {
    thr = 0;
    brk = 0;
    const Stop& s = STOPS[leg_];
    float safe = s.x + float(BOX_W) - 28.f;
    bool shortOf = pos_ < s.x;
    float bd = (speed_ * speed_) / (2.f * BRAKE);
    bool need = (safe - pos_) <= bd + 16.f;
    if (!shortOf) brk = 1;
    else if (need && speed_ > 8.f) brk = 1;
    else thr = 1;
}

void Game::startRun() {
    pos_ = START_POS;
    speed_ = 0;
    leg_ = 0;
    made_ = 0;
    late_ = 0;
    legTime_ = 0;
    spare_ = 0;
    thr_ = brk_ = 0;
    won_ = false;
    over_ = false;
    note_ = "";
    boardPassed_ = false;
    wasBraking_ = false;
    fanStep_ = -1;
    for (Puff& p : puffs_) p.age = 0;
    mode_ = Mode::Roll;
    whistle();
}

void Game::arrive() {
    spare_ = std::max(0.f, STOPS[leg_].limit - legTime_);
    made_++;
    speed_ = 0;
    thr_ = brk_ = 0;
    mode_ = Mode::Dwell;
    dwell_ = 0.85f;
    whistle();
    blip(220.f);
    sys_->rumble(0.35f, 0.15f, 90);
}

void Game::fail(const char* why) {
    if (mode_ == Mode::Lost || mode_ == Mode::Won) return;
    note_ = why;
    mode_ = Mode::Lost;
    won_ = false;
    speed_ = 0;
    if (std::strcmp(why, "LATE ARRIVAL") == 0) late_ = 1;
    if (bot_) over_ = true;
    sys_->apu.keyOff(1);
    sys_->apu.noiseBurst(0.5f, 500.f, 0.35f);
    sys_->rumble(0.8f, 0.5f, 180);
}

void Game::winRun() {
    if (mode_ == Mode::Won) return;
    note_ = "ON TIME";
    mode_ = Mode::Won;
    won_ = true;
    speed_ = 0;
    fanStep_ = 0;
    fanT_ = 0;
    if (bot_) over_ = true;
    sys_->rumble(0.25f, 0.4f, 160);
}

void Game::updateTitle() {
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) startRun();
    else if (pad.pressed(gs::BTN_MODE)) sys_->quit();
}

void Game::updatePause() {
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_START)) mode_ = Mode::Roll;
    else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
}

void Game::updateEnd(float dt) {
    if (fanStep_ >= 0) {
        fanT_ += dt;
        if (fanT_ >= 0.14f) {
            fanT_ = 0;
            static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
            if (fanStep_ < 4) sys_->apu.keyOn(1, notes[fanStep_], 0.18f);
            else sys_->apu.keyOff(1);
            if (++fanStep_ > 6) fanStep_ = -1;
        }
    }
    if (bot_) return;
    const gs::Pad& pad = sys_->pad;
    if (pad.pressed(gs::BTN_START)) startRun();
    else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
}

void Game::updateDwell(float dt) {
    dwell_ -= dt;
    if (dwell_ > 0) return;
    if (leg_ + 1 >= NSTOPS) winRun();
    else {
        leg_++;
        legTime_ = 0;
        boardPassed_ = false;
        mode_ = Mode::Roll;
        whistle();
    }
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
        if (pad.down(gs::BTN_UP) || pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_A)) thr = 1;
        if (pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_B) || pad.down(gs::BTN_TURBO)) brk = 1;
        thr = std::max(thr, pad.accel);
        brk = std::max(brk, pad.brake);
        if (brk > 0.12f) thr = 0;
    }
    thr_ = thr;
    brk_ = brk;
    if (brk > 0.2f && !wasBraking_ && speed_ > 40.f) sys_->apu.noiseBurst(0.18f, 2200.f, 0.12f);
    wasBraking_ = brk > 0.2f;

    if (brk > 0.f) speed_ -= BRAKE * brk * dt;
    else if (thr > 0.f) speed_ += ACCEL * thr * dt;
    else speed_ -= DRAG * dt;
    speed_ = std::clamp(speed_, 0.f, MAX_V);
    pos_ += speed_ * dt;
    legTime_ += dt;

    const Stop& s = STOPS[leg_];
    if (!boardPassed_ && pos_ >= boardX(s)) {
        boardPassed_ = true;
        blip(680.f);
    }
    float far = s.x + float(BOX_W);
    bool inBox = pos_ >= s.x && pos_ <= far;
    if (inBox && speed_ <= STOP_V) {
        arrive();
        return;
    }
    if (pos_ > far) {
        fail("RAN THE BOX");
        return;
    }
    if (legTime_ >= s.limit) fail("LATE ARRIVAL");

    puffT_ += dt;
    float gap = speed_ > 50.f ? 0.09f : 0.2f;
    if (speed_ > 8.f && puffT_ >= gap) {
        puffT_ = 0;
        int slot = 0;
        for (int i = 0; i < 8; i++)
            if (puffs_[i].age <= 0) {
                slot = i;
                break;
            }
        Puff& p = puffs_[slot];
        p.x = pos_ - 46.f;
        p.age = 0.01f;
    }
    for (Puff& p : puffs_) {
        if (p.age <= 0) continue;
        p.age += dt;
        if (p.age > 0.85f) p.age = 0;
    }
}

void Game::sky() {
    const uint16_t top = gs::rgb4(3, 6, 12);
    const uint16_t hor = gs::rgb4(13, 11, 9);
    const uint16_t field = gs::rgb4(4, 8, 3);
    const uint16_t bed = gs::rgb4(5, 4, 3);
    const uint16_t bedLo = gs::rgb4(3, 3, 2);
    sys_->vdp.setFogColor(hor);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 142) c = lerpC(top, hor, y / 142.f);
        else if (y < RAIL_Y) c = lerpC(hor, field, (y - 142) / float(RAIL_Y - 142));
        else c = lerpC(bed, bedLo, (y - RAIL_Y) / float(gs::SCREEN_H - RAIL_Y));
        sys_->vdp.lineBackdrop[y] = c;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
}

void Game::drawPoster() {
    blit(art_.logo, (gs::SCREEN_W - art_.logo.w) * 0.5f, 18, PAL_AMBER);
    blit(art_.tag, (gs::SCREEN_W - art_.tag.w) * 0.5f, 52, PAL_RED);
    float age = std::fmod(t_, 0.8f);
    const float locoX = -16.f;
    blit(art_.puff, locoX + 64.f, LOCO_Y - age * 28.f, PAL_FX, 8 + int(age * 14), 8 + int(age * 14), int(age * 8));
    int phase = int(t_ * 6.f) % 2;
    blit(art_.loco[phase], locoX, float(LOCO_Y), PAL_CAB);
    blit(art_.board, 148, float(RAIL_Y - 36), PAL_STATION);
    blit(art_.signal, 166, float(RAIL_Y - 52), PAL_GO);
    blit(art_.stop[0], 192, float(STATION_Y), PAL_STATION);
    int seg = art_.rail.w;
    for (int x = -seg; x < gs::SCREEN_W + seg; x += seg) blit(art_.rail, float(x), float(RAIL_Y), PAL_SCENERY);
    blit(art_.tree, 250, 122, PAL_SCENERY, 28, 46, 3);
    blit(art_.hill, 150, 134, PAL_SCENERY, 0, 0, 6);
    blit(art_.hill, -10, 128, PAL_SCENERY, 170, 48, 8);
    float drift = std::fmod(t_ * 10.f, 400.f);
    blit(art_.cloud, drift - 80.f, 62, PAL_SCENERY);
    blit(art_.cloud, drift - 260.f, 48, PAL_SCENERY, 0, 0, 4);
    blit(art_.sun, 250, 34, PAL_FX);
}

void Game::drawRun() {
    float cam = pos_ - NOSE_X;
    blit(art_.panel, 0, 0, PAL_PANEL, gs::SCREEN_W, 26);
    int phase = int(pos_ / 8.f) % 2;
    if (phase < 0) phase = 0;
    blit(art_.loco[phase], NOSE_X - art_.loco[phase].w, float(LOCO_Y), PAL_CAB);
    for (const Puff& p : puffs_) {
        if (p.age <= 0) continue;
        int sz = 8 + int(p.age * 16);
        blit(art_.puff, p.x - cam, LOCO_Y - p.age * 34.f, PAL_FX, sz, sz, int(p.age * 10));
    }
    for (int i = 0; i < NSTOPS; i++) {
        const Stop& s = STOPS[i];
        float left = s.limit - legTime_;
        bool hot = (i == leg_ && mode_ == Mode::Roll && left < 4.f);
        blit(art_.signal, s.x - 28.f - cam, float(RAIL_Y - 54), hot ? PAL_STOP : PAL_GO);
        blit(art_.board, boardX(s) - cam, float(RAIL_Y - 40), PAL_STATION);
        blit(art_.stop[i], s.x - cam, float(STATION_Y), PAL_STATION);
    }
    int seg = std::max(1, int(art_.rail.w));
    int i0 = int(std::floor(cam / seg)) - 1;
    for (int i = i0; i < i0 + 14; i++) blit(art_.rail, float(i * seg) - cam, float(RAIL_Y), PAL_SCENERY);

    int drift = int(t_ * 6.f);
    auto band = [&](int spacing, int y, const gs::Image& img, int pal, int fog, float parallax, int dw, int dh) {
        float c = cam * parallax - drift * (parallax < 0.5f ? 0.15f : 0.f);
        int i0 = int(std::floor(c / spacing)) - 2;
        for (int i = i0; i < i0 + 12; i++) {
            float sx = float(i * spacing) - c;
            int bob = int((unsigned(i) * 17u) % 7u) - 3;
            blit(img, sx, float(y + bob), pal, dw, dh, fog);
        }
    };
    band(210, RAIL_Y - 58, art_.pole, PAL_SCENERY, 0, 0.92f, 0, 0);
    band(170, 118, art_.tree, PAL_SCENERY, 3, 0.72f, 0, 0);
    band(260, 118, art_.hill, PAL_SCENERY, 9, 0.35f, 160, 44);
    band(340, 46, art_.cloud, PAL_SCENERY, 2, 0.15f, 0, 0);
    blit(art_.sun, 236, 30, PAL_FX);
}

void Game::drawBanner() {
    const gs::Image* img = nullptr;
    int pal = PAL_TEXT;
    if (mode_ == Mode::Dwell) {
        img = &art_.banOn;
        pal = PAL_GREEN;
    } else if (mode_ == Mode::Won) {
        img = &art_.banMade;
        pal = PAL_GREEN;
    } else if (mode_ == Mode::Lost) {
        img = std::strcmp(note_, "RAN THE BOX") == 0 ? &art_.banRan : &art_.banLate;
        pal = PAL_RED;
    }
    if (!img) return;
    blit(*img, (gs::SCREEN_W - img->w) * 0.5f, 72, pal);
}

void Game::hud() {
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) {
        hudCenter(9, "A CAB. A LINE. STOP BOXES.", PAL_TEXT);
        hudCenter(11, "Z/UP THROTTLE    X/DOWN BRAKE", PAL_DIM);
        hudCenter(12, "C HORN", PAL_DIM);
        if (int(t_ * 2.f) % 2 == 0) hudCenter(24, "ENTER TO ROLL", PAL_AMBER);
        hudCenter(26, "LATE ARRIVAL COSTS THE RUN", PAL_RED);
        return;
    }

    const Stop& s = STOPS[std::clamp(leg_, 0, NSTOPS - 1)];
    char line[48];
    std::snprintf(line, sizeof line, "%s %d/%d", s.name, leg_ + 1, NSTOPS);
    hudText(1, 0, "WEST LINE", PAL_DIM);
    hudText(12, 0, line, PAL_TEXT);

    if (mode_ == Mode::Roll || mode_ == Mode::Pause) {
        float left = std::max(0.f, s.limit - legTime_);
        std::snprintf(line, sizeof line, "DUE %4.1f", left);
        int timePal = left < 4.f ? PAL_RED : left < 7.f ? PAL_AMBER : PAL_TEXT;
        hudText(28, 0, line, timePal);
        int toBox = int(std::max(0.f, s.x - pos_) + 0.5f);
        if (pos_ >= s.x && pos_ <= s.x + BOX_W) std::snprintf(line, sizeof line, "IN BOX");
        else std::snprintf(line, sizeof line, "BOX %4d", toBox);
        hudText(1, 1, line, pos_ >= s.x ? PAL_AMBER : PAL_DIM);
        std::snprintf(line, sizeof line, "SPD %3d", int(speed_ * 0.5f + 0.5f));
        hudText(14, 1, line, PAL_TEXT);
        bool showBrake = mode_ == Mode::Roll && pos_ >= boardX(s) - 6.f && speed_ > STOP_V && pos_ < s.x + BOX_W;
        if (showBrake && int(t_ * 8.f) % 2 == 0) hudText(30, 1, "BRAKE", PAL_RED);
        else if (pos_ >= s.x && speed_ > STOP_V) hudText(30, 1, "FAST", PAL_RED);
        else if (pos_ > s.x - 100.f && pos_ < s.x && speed_ < 22.f && legTime_ > 0.8f) hudText(30, 1, "CREEP", PAL_AMBER);

        int tb = std::clamp(int(thr_ * 10.f + 0.5f), 0, 10);
        int bb = std::clamp(int(brk_ * 10.f + 0.5f), 0, 10);
        char bar[16];
        std::snprintf(bar, sizeof bar, "%.*s%.*s", tb, "##########", 10 - tb, "----------");
        hudText(1, 2, "THR", PAL_DIM);
        hudText(5, 2, bar, PAL_GREEN);
        std::snprintf(bar, sizeof bar, "%.*s%.*s", bb, "##########", 10 - bb, "----------");
        hudText(20, 2, "BRK", PAL_DIM);
        hudText(24, 2, bar, PAL_RED);
        if (mode_ == Mode::Pause) hudCenter(6, "PAUSED", PAL_AMBER);
        else if (leg_ == 0 && legTime_ < 3.f) hudCenter(5, "BRAKE AT THE YELLOW BOARD", PAL_DIM);
    } else if (mode_ == Mode::Dwell) {
        std::snprintf(line, sizeof line, "EARLY %4.1f", spare_);
        hudText(26, 0, line, PAL_GREEN);
        hudText(1, 1, "STOPPED", PAL_GREEN);
    } else if (mode_ == Mode::Won) {
        hudText(26, 0, "ON TIME", PAL_GREEN);
        hudCenter(12, "EVERY STOP MADE", PAL_TEXT);
        if (!bot_) hudCenter(16, "ENTER FOR ANOTHER RUN", PAL_DIM);
    } else if (mode_ == Mode::Lost) {
        hudText(24, 0, note_, PAL_RED);
        hudCenter(12, "THE RUN IS LOST", PAL_RED);
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
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.14f, 0.22f, 0.14f);
    engineOn_ = false;
    if (bot_) startRun();
    else mode_ = Mode::Title;
    audio(DT);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    audio(DT);
    if (mode_ == Mode::Title) updateTitle();
    else if (mode_ == Mode::Pause) updatePause();
    else if (mode_ == Mode::Won || mode_ == Mode::Lost) updateEnd(DT);
    else if (mode_ == Mode::Dwell) updateDwell(DT);
    else updateRoll(DT);
    draw();
}

}  // namespace rail
