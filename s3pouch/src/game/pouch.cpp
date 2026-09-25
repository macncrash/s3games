#include "game/pouch.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace pouch {
namespace {

struct Lane {
    float y0, y1, speed, spacing, length, phase;
    int kind;
    int pal0, pal1, pal2;
};

const Lane kLanes[] = {
    {S1_Y0, S1_Y1, 1.20f, 190.f, float(SEDAN_W), 60.f, 0, PAL_RED, PAL_BLUE, PAL_TAXI},
    {S2_Y0, S2_Y1, -1.60f, 220.f, float(VAN_W), 30.f, 1, PAL_VAN, PAL_VAN2, PAL_VAN},
    {S3_Y0, S3_Y1, 2.05f, 240.f, float(BUS_W), 30.f, 2, PAL_BUS, PAL_BUS2, PAL_BUS},
};

}  // namespace

float Game::stepToward(float y, float dest) const {
    float d = dest - y;
    if (std::fabs(d) <= SPEED) return dest;
    return y + (d < 0 ? -SPEED : SPEED);
}

bool Game::hits(float x, float y, int frame) const {
    for (int i = 0; i < 3; i++) {
        const auto& L = kLanes[i];
        if (!overlapsStreet(y, L.y0, L.y1)) continue;
        float rel = std::fmod(x - (L.phase + L.speed * float(frame)), L.spacing);
        if (rel < 0) rel += L.spacing;
        float dist = std::min(rel, L.spacing - rel);
        if (dist < L.length * 0.5f + HW) return true;
    }
    return false;
}

bool Game::pathClear(float x, float y, float dest, int frame0) const {
    int f = 0;
    while (std::fabs(y - dest) > 0.001f && f < 500) {
        y = stepToward(y, dest);
        if (hits(x, y, frame0 + f)) return false;
        f++;
    }
    return std::fabs(y - dest) <= 0.001f;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 6;
}

void Game::mix() {
    if (beep_ > 0 && --beep_ == 0) sys_->apu.tone(0, 0, 0);
    if (fan_ >= 0 && ++fanT_ >= 7) {
        fanT_ = 0;
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        if (fan_ < 4) sys_->apu.tone(1, notes[fan_], 0.075f);
        else sys_->apu.tone(1, 0, 0);
        if (++fan_ > 6) fan_ = -1;
    }
    float bed = (mode_ == Mode::Run || mode_ == Mode::Title) ? 0.028f : 0.012f;
    sys_->apu.noise(bed, 720.f, false);
}

void Game::begin() {
    mode_ = Mode::Run;
    px_ = HOME_X;
    py_ = POS_START;
    dest_ = py_;
    commit_ = false;
    held_ = true;
    cleared_ = 0;
    tick_ = 0;
    wait_ = 0;
    seconds_ = 0;
    showSec_ = 0;
    over_ = false;
    won_ = false;
    faceRight_ = true;
    shake_ = 0;
    endT_ = 0;
    satVx_ = satVy_ = 0;
    blip(880.f);
}

void Game::finish(bool win) {
    if (over_) return;
    showSec_ = seconds_;
    over_ = true;
    won_ = win;
    commit_ = false;
    if (win) {
        cleared_ = 3;
        held_ = true;
        mode_ = Mode::Home;
        fan_ = 0;
        fanT_ = 0;
        if (bot_) std::printf("pouch across three streets\n");
        sys_->rumble(0.15f, 0.4f, 160);
    } else {
        held_ = false;
        mode_ = Mode::Dropped;
        shake_ = 10;
        satX_ = px_ + 6.f;
        satY_ = py_ + 1.f;
        satVx_ = 1.7f;
        satVy_ = -2.1f;
        sys_->apu.tone(0, 120.f, 0.09f);
        beep_ = 12;
        sys_->apu.noiseBurst(0.5f, 640.f, 0.28f);
        if (bot_) std::printf("pouch dropped\n");
        sys_->rumble(0.75f, 0.95f, 220);
    }
}

void Game::flyPouch() {
    satX_ += satVx_;
    satY_ += satVy_;
    satVy_ += 0.22f;
    if (satY_ > py_ + 16.f && satVy_ > 0) {
        satY_ = py_ + 16.f;
        satVy_ = -satVy_ * 0.35f;
        satVx_ *= 0.65f;
        if (std::fabs(satVy_) < 0.45f) satVy_ = 0;
    }
}

void Game::botThink() {
    if (commit_) return;
    int b = bandAt(py_);
    if (b < 0 || b > 2) return;
    float dest = hopUp(b);
    if (pathClear(px_, py_, dest, tick_)) {
        dest_ = dest;
        commit_ = true;
        wait_ = 0;
        blip(620.f);
        return;
    }
    if (++wait_ >= 360) {
        px_ = std::clamp(px_ + (px_ < HOME_X ? 18.f : -18.f), 28.f, 292.f);
        wait_ = 0;
    }
}

void Game::humanThink() {
    const gs::Pad& pad = sys_->pad;
    if (!commit_) {
        float vx = 0;
        if (pad.down(gs::BTN_LEFT)) vx -= SLIDE;
        if (pad.down(gs::BTN_RIGHT)) vx += SLIDE;
        if (vx != 0) {
            faceRight_ = vx > 0;
            px_ = std::clamp(px_ + vx, 18.f, 302.f);
        }
        int b = bandAt(py_);
        bool go = pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
        bool back = pad.pressed(gs::BTN_DOWN);
        if (go && !back && b >= 0 && b < 3) {
            dest_ = hopUp(b);
            commit_ = true;
            blip(620.f);
        } else if (back && !go && b > 0 && b < 3) {
            dest_ = hopDown(b);
            commit_ = true;
            blip(420.f);
        }
    }
}

void Game::updateRun() {
    seconds_ += 1.f / 60.f;
    if (bot_) botThink();
    else humanThink();
    if (commit_) {
        py_ = stepToward(py_, dest_);
        if (py_ == dest_) commit_ = false;
    }
    if (hits(px_, py_, tick_)) finish(false);
    else if (bandAt(py_) == 3) finish(true);
    else {
        int b = bandAt(py_);
        if (b > cleared_) {
            cleared_ = b;
            if (b > 0 && b < 3) blip(780.f);
        }
    }
    if (mode_ == Mode::Run) tick_++;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (shake_ > 0 && !shadow) {
        s.x = int16_t(s.x + int(std::sin(float(shake_) * 1.9f) * 2.f));
        s.y = int16_t(s.y + ((shake_ & 1) ? 1 : -1));
    }
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::hudText(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hud() {
    char buf[40];
    if (mode_ == Mode::Title) {
        hudText(1, 27, "UP OR C CROSSES", PAL_HUD);
        hudText(22, 27, "ENTER STARTS", PAL_GOLD);
        return;
    }
    int n = std::min(cleared_, 3);
    std::snprintf(buf, sizeof buf, "CROSSED %d/3", n);
    hudText(1, 0, buf, PAL_HUD);
    hudText(16, 0, held_ ? "HELD" : "DROPPED", held_ ? PAL_SAFE : PAL_ALARM);
    std::snprintf(buf, sizeof buf, "%.1f", over_ ? showSec_ : seconds_);
    hudText(32, 0, buf, PAL_GOLD);
}

void Game::cars(int frame) {
    const gs::Mipped* pic[3] = {&art_.sedan, &art_.van, &art_.bus};
    for (int n = 0; n < 3; n++) {
        const auto& L = kLanes[n];
        float origin = L.phase + L.speed * float(frame);
        float S = L.spacing;
        int i0 = int(std::floor((-160.f - origin) / S)) - 1;
        int i1 = int(std::ceil((480.f - origin) / S)) + 1;
        float cy = (L.y0 + L.y1) * 0.5f;
        float h = float(pic[L.kind]->h);
        for (int i = i0; i <= i1; i++) {
            float cx = origin + float(i) * S;
            if (cx < -100.f || cx > 420.f) continue;
            int k = (i < 0 ? -i : i) % 3;
            int pal = k == 1 ? L.pal1 : k == 2 ? L.pal2 : L.pal0;
            spr(art_.shadow, cx, cy + h * 0.38f, 8, 0, false, true);
            spr(*pic[L.kind], cx, cy, h, pal, L.speed < 0, false);
        }
    }
}

void Game::props() {
    spr(art_.lamp, 22, POS_MED1, 22, PAL_PROP, false, false);
    spr(art_.lamp, 298, POS_MED1, 22, PAL_PROP, false, false);
    spr(art_.lamp, 22, POS_MED2, 22, PAL_PROP, false, false);
    spr(art_.lamp, 298, POS_MED2, 22, PAL_PROP, false, false);
    spr(art_.tree, 62, POS_MED1, 16, PAL_PROP, false, false);
    spr(art_.tree, 258, POS_MED1, 16, PAL_PROP, false, false);
    spr(art_.tree, 62, POS_MED2, 16, PAL_PROP, false, false);
    spr(art_.tree, 258, POS_MED2, 16, PAL_PROP, false, false);
}

void Game::actor(int frame) {
    float x = px_;
    float y = py_;
    if (mode_ == Mode::Title) {
        x = HOME_X;
        y = POS_START + std::sin(frame * 0.08f) * 1.2f;
    } else if (mode_ == Mode::Home) {
        y -= std::fabs(std::sin(endT_ * 0.28f)) * 3.2f;
    }
    int step = (commit_ && ((frame / 5) & 1)) ? 1 : 0;
    const gs::Mipped& body = held_ ? art_.hero[step] : art_.bare[step];
    spr(body, x, y, float(body.h), PAL_HERO, !faceRight_, false);
    if (!held_) {
        spr(art_.satchel, satX_, satY_, 12, PAL_HERO, false, false);
        if (endT_ < 16) spr(art_.puff, px_, py_, 8.f + endT_ * 0.8f, PAL_FX, false, false);
    }
}

void Game::banners(int frame) {
    if (mode_ == Mode::Title) {
        text("S3 POUCH", 160, 18, 1.15f, PAL_TITLE);
        text("THREE STREETS", 160, 42, 0.62f, PAL_GOLD);
        text("DROP IT AND THE RUN IS OVER", 160, 164, 0.46f, PAL_ALARM);
        if ((frame / 30) % 2 == 0) text("ENTER TO CROSS", 160, 184, 0.5f, PAL_SAFE);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 108, 1.1f, PAL_TITLE);
    } else if (mode_ == Mode::Dropped) {
        text("DROPPED", 160, 100, 1.15f, PAL_ALARM);
        text("THE RUN IS OVER", 160, 124, 0.55f, PAL_GOLD);
    } else if (mode_ == Mode::Home) {
        text("ACROSS", 160, 96, 1.15f, PAL_GOLD);
        text("THE POUCH IS HOME", 160, 120, 0.55f, PAL_SAFE);
    }
}

void Game::draw(int frame) {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    banners(frame);
    actor(frame);
    cars(frame);
    props();
    hud();
    if (shake_ > 0) shake_--;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    for (int y = 0; y < gs::SCREEN_H; y++) sys.vdp.lineBackdrop[y] = gs::rgb4(2, 3, 6);
    sys.apu.setMaster(0.78f);
    sys.apu.setEcho(0.14f, 0.22f, 0.14f);
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) {
            begin();
            draw(tick_);
            mix();
            return;
        }
        if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        int frameN = tick_++;
        draw(frameN);
        mix();
        return;
    }

    if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            commit_ = false;
            over_ = false;
            won_ = false;
        }
        draw(tick_);
        mix();
        return;
    }

    if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            draw(tick_);
            mix();
            return;
        }
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            commit_ = false;
            over_ = false;
            won_ = false;
            draw(tick_);
            mix();
            return;
        }
        int shown = tick_;
        updateRun();
        draw(shown);
        mix();
        return;
    }

    endT_++;
    if (mode_ == Mode::Dropped) flyPouch();
    if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        begin();
        draw(tick_);
        mix();
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        mode_ = Mode::Title;
        over_ = false;
        won_ = false;
    }
    draw(tick_ + endT_);
    mix();
}

}  // namespace pouch
