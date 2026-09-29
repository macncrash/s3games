#include "game/maga.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace palisade {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LINE_Y = 168.f;
constexpr float BREACH_Y = 196.f;
constexpr float RAID_LEN = 32.f;
constexpr int MAG = 16;
constexpr int WALL = 6;

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Raid) return 1;
    if (mode_ == Mode::Won) return 2;
    return 3;
}

void Game::hudText(int col, int row, const char* s) {
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 127) c = '?';
        sys_->vdp.HUD.set(col + i, row, gs::entry(art_.font[c - 32], Pal::PAL_HUD));
    }
}

void Game::spr(const gs::Image& img, float x, float y, int w, int h, int pal, bool flip) {
    gs::Sprite s;
    float jx = shake_ > 0 ? std::sin(t_ * 48.f) * shake_ : 0;
    s.x = int16_t(x + jx);
    s.y = int16_t(y);
    s.w = int16_t(w);
    s.h = int16_t(h);
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.hudEnabled = true;
    sys.apu.setMaster(0.75f);
    mode_ = Mode::Title;
}

void Game::beginRaid() {
    score_ = 0;
    kills_ = 0;
    rounds_ = MAG;
    wall_ = WALL;
    px_ = 160;
    face_ = 1;
    cool_ = 0;
    shake_ = 0;
    t_ = 0;
    raid_ = RAID_LEN;
    spawnIx_ = 0;
    fail_ = 0;
    won_ = false;
    over_ = false;
    foes_.clear();
    slugs_.clear();
    script_.clear();
    float t = 0.7f;
    auto add = [&](float gap, float x, int kind) {
        t += gap;
        script_.push_back({t, x, kind});
    };
    // Eleven who mean the wall. Grey coats turn short of the stakes.
    const float xs[] = {90, 150, 210, 70, 250, 120, 40, 190, 100, 230, 160};
    for (int i = 0; i < 11; i++) {
        add(1.85f, xs[i], 0);
        if (i == 1 || i == 4 || i == 7) add(0.55f, xs[i] + (i % 2 ? -36.f : 40.f), 1);
    }
    mode_ = Mode::Raid;
}

void Game::shoot() {
    if (cool_ > 0 || rounds_ <= 0) return;
    rounds_--;
    cool_ = 0.22f;
    slugs_.push_back({px_, LINE_Y - 8.f});
    sys_->apu.tone(0, 620.f, 0.1f);
}

void Game::stick(float& dir, bool& fire) {
    dir = 0;
    fire = false;
    const gs::Pad& pad = sys_->pad;
    if (bot_) {
        const Foe* best = nullptr;
        for (const Foe& f : foes_) {
            if (!f.live || f.kind != 0) continue;
            if (!best || f.y > best->y) best = &f;
        }
        if (best) {
            float dx = best->x - px_;
            if (std::fabs(dx) > 4.f) dir = dx > 0 ? 1.f : -1.f;
            bool sent = false;
            for (const Slug& s : slugs_)
                if (std::fabs(s.x - best->x) < 16.f) sent = true;
            fire = !sent && std::fabs(dx) < 7.f && best->y > 40.f && rounds_ > 1;
        }
        return;
    }
    if (pad.down(gs::BTN_LEFT)) dir = -1.f;
    if (pad.down(gs::BTN_RIGHT)) dir = 1.f;
    if (std::fabs(pad.axisX) > 0.3f) dir = pad.axisX > 0 ? 1.f : -1.f;
    fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_B);
}

void Game::finish(bool hold) {
    if (hold) {
        score_ += rounds_ * 40 + wall_ * 25;
        mode_ = Mode::Won;
        won_ = true;
        sys_->apu.tone(0, 523.f, 0.16f);
        sys_->apu.tone(1, 659.f, 0.16f);
        sys_->apu.tone(2, 784.f, 0.16f);
    } else {
        mode_ = Mode::Lost;
        won_ = false;
        sys_->apu.tone(0, 98.f, 0.22f);
    }
    over_ = true;
}

void Game::update(float dt) {
    t_ += dt;
    raid_ -= dt;
    if (cool_ > 0) cool_ -= dt;
    if (shake_ > 0) shake_ -= dt * 8.f;

    while (spawnIx_ < int(script_.size()) && script_[spawnIx_].t <= t_) {
        const Order& o = script_[spawnIx_++];
        Foe f;
        f.x = o.x;
        f.y = 16.f;
        f.flash = 0;
        f.kind = o.kind;
        f.phase = 0;
        f.speed = o.kind == 0 ? 34.f : 30.f;
        f.live = true;
        foes_.push_back(f);
    }

    float dir = 0;
    bool fire = false;
    stick(dir, fire);
    if (dir != 0) face_ = dir;
    px_ = std::clamp(px_ + dir * 130.f * dt, 18.f, 302.f);
    if (fire) shoot();

    for (Slug& s : slugs_) s.y -= 240.f * dt;
    for (Slug& s : slugs_) {
        if (s.y < -8) continue;
        for (Foe& f : foes_) {
            if (!f.live) continue;
            if (std::fabs(s.x - f.x) < 14.f && s.y <= f.y + 8.f && s.y >= f.y - 26.f) {
                f.live = false;
                f.flash = 0.12f;
                s.y = -40;
                kills_++;
                score_ += f.kind == 0 ? 100 : 10;
                sys_->apu.tone(1, f.kind == 0 ? 180.f : 240.f, 0.12f);
                break;
            }
        }
    }
    slugs_.erase(std::remove_if(slugs_.begin(), slugs_.end(), [](const Slug& s) { return s.y < -12.f; }), slugs_.end());

    for (Foe& f : foes_) {
        if (!f.live) continue;
        if (f.flash > 0) f.flash -= dt;
        if (f.kind == 0) {
            f.y += f.speed * dt;
            if (f.y >= BREACH_Y) {
                f.live = false;
                wall_ = std::max(0, wall_ - 1);
                shake_ = 5.f;
                sys_->apu.tone(1, 70.f, 0.2f);
            }
        } else if (f.phase == 0) {
            f.y += f.speed * dt;
            if (f.y >= 92.f) f.phase = 1;
        } else {
            f.y -= f.speed * dt;
            if (f.y < 8.f) f.live = false;
        }
    }
    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.live && f.flash <= 0; }),
                foes_.end());

    if (wall_ <= 0) {
        fail_ = 2;
        finish(false);
        return;
    }
    if (rounds_ <= 0 && raid_ > 0.05f) {
        fail_ = 1;
        finish(false);
        return;
    }
    if (raid_ <= 0.f) {
        fail_ = rounds_ > 0 ? 0 : 1;
        finish(rounds_ > 0 && wall_ > 0);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    bool dusk = mode_ != Mode::Title && raid_ < 12.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r = int((dusk ? 4 : 8) + (2 - (dusk ? 4 : 8)) * u);
        int g = int((dusk ? 2 : 5) + (3 - (dusk ? 2 : 5)) * u);
        int b = int((dusk ? 2 : 3) + (6 - (dusk ? 2 : 3)) * (1.f - u));
        r = std::clamp(r, 0, 15);
        g = std::clamp(g, 0, 15);
        b = std::clamp(b, 0, 15);
        if (fail_ == 2 && y > 140) r = std::min(15, r + 3);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        v.lineFog[y] = y < 28 ? uint8_t((28 - y) / 5) : 0;
        v.road[y].on = false;
    }

    for (int i = 0; i < 16; i++) {
        float x = 8.f + i * 20.f;
        int h = 44 + (i % 3) * 6;
        spr(art_.stake, x, 128.f - (h - 44) * 0.15f, 10, h, Pal::PAL_WOOD);
    }

    std::vector<Foe> order = foes_;
    std::sort(order.begin(), order.end(), [](const Foe& a, const Foe& b) { return a.y < b.y; });
    for (int i = int(order.size()) - 1; i >= 0; i--) {
        const Foe& f = order[i];
        bool flash = f.flash > 0;
        if (f.kind == 0) {
            int h = 26 + int(std::clamp(f.y / 200.f, 0.f, 1.f) * 8);
            spr(art_.raider, f.x - 9, f.y - h, 18, h, flash ? Pal::PAL_FX : Pal::PAL_RAIDER, f.x > px_);
        } else {
            int h = 24;
            spr(art_.feint, f.x - 9, f.y - h, 18, h, flash ? Pal::PAL_FX : Pal::PAL_FEINT, f.phase == 1);
        }
    }
    for (const Slug& s : slugs_) spr(art_.slug, s.x - 2, s.y - 8, 4, 8, Pal::PAL_FX);

    if (mode_ == Mode::Title || mode_ == Mode::Raid)
        spr(art_.sentry, px_ - 11, LINE_Y - 36, 22, 36, Pal::PAL_PLAYER, face_ < 0);

    char buf[64];
    if (mode_ == Mode::Title) {
        hudText(8, 3, "PALISADE MAGA");
        hudText(4, 6, "MAKE THE MAGAZINE LAST");
        hudText(6, 8, "LONGER THAN THE RAID");
        hudText(5, 11, "GREY COATS TURN BACK");
        hudText(6, 14, "ARROWS MOVE   A FIRE");
        hudText(10, 22, "PRESS START");
    } else if (mode_ == Mode::Won) {
        hudText(7, 4, "THE MAGAZINE HELD");
        hudText(8, 6, "THE RAID IS DONE");
        std::snprintf(buf, sizeof buf, "ROUNDS LEFT %d", rounds_);
        hudText(8, 9, buf);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudText(10, 11, buf);
    } else if (mode_ == Mode::Lost) {
        if (fail_ == 1) {
            hudText(5, 4, "THE MAGAZINE IS SPENT");
            hudText(6, 6, "THE RAID STILL RUNS");
        } else {
            hudText(6, 4, "THE PALISADE IS DOWN");
            hudText(7, 6, "THE WATCH IS OVER");
        }
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudText(10, 9, buf);
    }

    if (mode_ != Mode::Title) {
        int sec = std::max(0, int(std::ceil(raid_)));
        std::snprintf(buf, sizeof buf, "RAID %02d", sec);
        hudText(1, 1, buf);
        std::snprintf(buf, sizeof buf, "WALL %d", wall_);
        hudText(24, 1, buf);
        hudText(1, 26, "MAG");
        int show = std::min(rounds_, MAG);
        for (int i = 0; i < show; i++) spr(art_.round, 36.f + i * 8.f, 204.f, 6, 12, i < 4 ? Pal::PAL_FX : Pal::PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START)) beginRaid();
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    } else {
        update(DT);
    }
    if (cool_ <= 0.02f && mode_ == Mode::Raid) sys.apu.tone(0, 0, 0);
    draw();
}

}  // namespace palisade
