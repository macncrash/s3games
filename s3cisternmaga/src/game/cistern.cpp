#include "game/cistern.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cmaga {
namespace {

constexpr float RAID_LEN = 27.f;
constexpr int MAG = 7;
constexpr float DT = 1.f / 60.f;
constexpr float SPEED = 0.15f;
constexpr float WIN_LO = 0.50f;
constexpr float WIN_HI = 0.68f;
constexpr float BREACH = 0.90f;

struct Beat {
    float t;
    int sector;
    int wade;
};

// Five raiders stay on the coping. Waders climb the water and turn back.
constexpr Beat kBeat[] = {
    {1.1f, 1, 0}, {3.4f, 1, 1}, {5.6f, 0, 0}, {7.8f, 2, 1}, {9.8f, 2, 0},
    {12.2f, 0, 1}, {14.4f, 1, 0}, {16.6f, 1, 1}, {18.6f, 0, 0}, {21.0f, 2, 1},
};
constexpr int NBEAT = int(sizeof(kBeat) / sizeof(kBeat[0]));

}  // namespace

const char* Game::result() const {
    if (won_) return "THE MAGAZINE OUTLASTS THE RAID";
    if (fail_ == Fail::Spent) return "MAGAZINE SPENT";
    if (fail_ == Fail::Rim) return "THEY TOOK THE RIM";
    return "THE RAID IS NOT DONE";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Raid || mode_ == Mode::Pause) return 1;
    return 2;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c = y < 36 ? gs::rgb4(1, 1, 2) : y < 100 ? gs::rgb4(2, 3, 4) : gs::rgb4(1, 2, 3);
        if (y > 168) c = gs::rgb4(3, 3, 2);
        sys.vdp.lineBackdrop[y] = c;
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.4f);
}

void Game::beginRaid() {
    mode_ = Mode::Raid;
    fail_ = Fail::None;
    over_ = false;
    won_ = false;
    rounds_ = MAG;
    stopped_ = 0;
    misses_ = 0;
    sector_ = 1;
    next_ = 0;
    raidT_ = 0;
    cool_ = 0.4f;
    flash_ = 0;
    shake_ = 0;
    lastHit_ = false;
    foes_.clear();
}

void Game::lose(Fail why) {
    if (mode_ != Mode::Raid) return;
    fail_ = why;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    sys_->apu.noiseBurst(0.4f, 700.f, 0.28f);
}

void Game::win() {
    if (mode_ != Mode::Raid) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    sys_->apu.tone(0, 494.f, 0.12f);
    sys_->apu.tone(1, 740.f, 0.08f);
}

Game::Foe* Game::onCoping(int sector) {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Commit || f.phase != Phase::Climb || f.sector != sector) continue;
        if (f.along < WIN_LO || f.along > WIN_HI) continue;
        if (!best || f.along > best->along) best = &f;
    }
    return best;
}

void Game::pull() {
    if (mode_ != Mode::Raid || cool_ > 0) return;
    cool_ = 0.36f;
    flash_ = 0.09f;
    shake_ = 3.2f;
    if (rounds_ <= 1) {
        rounds_ = 0;
        lose(Fail::Spent);
        return;
    }
    rounds_--;
    if (Foe* f = onCoping(sector_)) {
        f->phase = Phase::Sunk;
        f->age = 0;
        stopped_++;
        lastHit_ = true;
        sys_->apu.noiseBurst(0.5f, 2100.f, 0.07f);
        sys_->apu.tone(2, 180.f, 0.12f);
    } else {
        misses_++;
        lastHit_ = false;
        sys_->apu.noiseBurst(0.28f, 900.f, 0.1f);
        sys_->apu.tone(2, 90.f, 0.1f);
    }
}

void Game::botAct() {
    Foe* best = nullptr;
    for (Foe& f : foes_) {
        if (!f.on || f.kind != Kind::Commit || f.phase != Phase::Climb) continue;
        if (f.along < WIN_LO + 0.02f || f.along > WIN_HI - 0.02f) continue;
        if (!best || f.along > best->along) best = &f;
    }
    if (!best || rounds_ <= 1) return;
    sector_ = best->sector;
    pull();
}

void Game::humanAct() {
    gs::Pad& p = sys_->pad;
    if (p.pressed(gs::BTN_LEFT)) sector_ = (sector_ + 2) % 3;
    if (p.pressed(gs::BTN_RIGHT)) sector_ = (sector_ + 1) % 3;
    if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C)) pull();
}

void Game::updateRaid(float dt) {
    if (bot_) botAct();
    else humanAct();
    cool_ = std::max(0.f, cool_ - dt);
    flash_ = std::max(0.f, flash_ - dt);
    shake_ *= 0.86f;

    while (next_ < NBEAT && raidT_ >= kBeat[next_].t) {
        Foe f;
        f.kind = kBeat[next_].wade ? Kind::Wade : Kind::Commit;
        f.sector = kBeat[next_].sector;
        f.along = 0.02f;
        foes_.push_back(f);
        next_++;
    }

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.age += dt;
        float sp = f.kind == Kind::Commit ? SPEED : SPEED * 0.82f;
        if (f.phase == Phase::Climb) {
            f.along += sp * dt;
            if (f.kind == Kind::Wade && f.along >= 0.56f) f.phase = Phase::Turn;
            if (f.kind == Kind::Commit && f.along >= BREACH) {
                lose(Fail::Rim);
                return;
            }
        } else if (f.phase == Phase::Turn) {
            f.along -= sp * 1.25f * dt;
            if (f.along < -0.05f) f.on = false;
        } else if (f.age > 0.65f) {
            f.on = false;
        }
    }

    raidT_ += dt;
    if (mode_ != Mode::Raid) return;
    if (raidT_ >= RAID_LEN && rounds_ > 0) win();
    else if (raidT_ >= RAID_LEN) lose(Fail::Spent);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (mode_ == Mode::Title) {
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) beginRaid();
    } else if (mode_ == Mode::Raid) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else updateRaid(DT);
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Raid;
    } else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
        beginRaid();
        over_ = false;
    }
    draw();
}

void Game::place(const Foe& f, float& x, float& y, float& h) const {
    float n = std::clamp(f.along, 0.f, 1.f);
    float spread = 16.f + n * 78.f;
    x = 160.f + (f.sector - 1) * spread;
    y = 72.f + n * 78.f;
    h = 12.f + n * 34.f;
    if (f.kind == Kind::Wade) y += 6.f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 2.f || m.h <= 0) return;
    float s = h / float(m.h);
    gs::Sprite sp;
    sp.img = m.pick(h);
    sp.w = std::max(1, int(m.w * s));
    sp.h = std::max(1, int(h));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(feet ? cy - sp.h : cy - sp.h * 0.5f));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float pen = x;
    float sc = std::max(1.f, scale);
    for (char ch : s) {
        unsigned c = unsigned(ch);
        if (c < 32 || c > 95) c = 32;
        gs::Image img = art_.glyph[c];
        int w = std::max(1, art_.gw[c]);
        gs::Sprite sp;
        sp.img = img;
        sp.w = int(w * sc);
        sp.h = int(art_.gh * sc);
        sp.x = int(pen);
        sp.y = int(y);
        sp.pal = uint8_t(pal);
        sys_->vdp.sprite(sp);
        pen += (w + 1) * sc;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    float sx = (shake_ > 0.4f) ? std::sin(t_ * 80.f) * shake_ : 0.f;

    text("MAG", 12, 8, 1.f, PAL_HUD);
    for (int i = 0; i < MAG; i++) {
        if (i < rounds_) spr(art_.brass, 48.f + i * 10.f, 14, 14, PAL_BRASS, false, false);
    }
    int secs = std::max(0, int(std::ceil(RAID_LEN - raidT_ - 0.001f)));
    char clock[16];
    std::snprintf(clock, sizeof clock, "RAID %02d", mode_ == Mode::Title ? int(RAID_LEN) : secs);
    text(clock, 232, 8, 1.f, secs <= 5 && mode_ == Mode::Raid ? PAL_ALERT : PAL_HUD);

    float aimX = 160.f + (sector_ - 1) * 72.f + sx;
    spr(art_.notch, aimX, 168, 10, PAL_HUD, false, false);
    if (flash_ > 0) spr(art_.splash, aimX, 150, 18, lastHit_ ? PAL_FX : PAL_WATER, false, false);

    if (mode_ == Mode::Title) {
        text("S3 CISTERN MAGA", 92, 36, 1.f, PAL_HUD);
        text("MAKE THE MAGAZINE LAST", 70, 52, 1.f, PAL_OK);
        text("A ROUND MUST OUTLIVE THE RAID", 52, 66, 1.f, PAL_HUD);
        text("A MISS STILL LEAVES THE MAG", 64, 80, 1.f, PAL_BRASS);
        text("LEFT RIGHT AIM    A FIRE", 68, 196, 1.f, PAL_HUD);
        if (int(t_ * 2) & 1) text("START", 140, 112, 1.f, PAL_BRASS);
    } else if (mode_ == Mode::Pause) {
        text("HELD", 144, 40, 1.f, PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("THE MAGAZINE OUTLASTS THE RAID", 46, 36, 1.f, PAL_OK);
    } else if (mode_ == Mode::Lost) {
        text(fail_ == Fail::Spent ? "MAGAZINE SPENT" : "THEY TOOK THE RIM", 88, 36, 1.f, PAL_ALERT);
        text("THE WATCH IS OVER", 96, 50, 1.f, PAL_ALERT);
    } else if (rounds_ <= 2) {
        text("LAST ROUNDS", 116, 196, 1.f, PAL_ALERT);
    } else {
        text("HOLD THE CISTERN", 100, 196, 1.f, PAL_HUD);
    }

    spr(art_.lamp, 28 + sx, 48, 22, PAL_BRASS, false, false);
    spr(art_.lamp, 292 + sx, 48, 22, PAL_BRASS, true, false);
    for (int i = 0; i < 8; i++) spr(art_.block, 24.f + i * 40.f + sx, 28, 16, PAL_VAULT, false, false);

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].along > foes_[b].along; });

    for (int i : order) {
        const Foe& f = foes_[i];
        float x, y, h;
        place(f, x, y, h);
        x += sx;
        int pal = f.kind == Kind::Commit ? PAL_RAIDER : PAL_WADER;
        if (f.phase == Phase::Sunk) {
            spr(art_.sunk, x, y, h * 0.45f, pal, false, true);
            spr(art_.splash, x, y - 6, 16, PAL_WATER, false, false);
        } else {
            const gs::Mipped& img = (f.kind == Kind::Commit ? art_.raider : art_.wader)[int(f.age * 6) & 1];
            bool flip = f.phase == Phase::Turn ? (f.sector != 0) : (f.sector == 0);
            spr(img, x, y, h, pal, flip, true);
        }
    }

    spr(art_.stair, 160 + sx, 86, 36, PAL_LIME, false, false);
    spr(art_.water, 160 + sx, 124, 52, PAL_WATER, false, false);
    spr(art_.drip, 118 + std::sin(t_ * 2.f) * 4.f, 108, 10, PAL_WATER, false, false);
    spr(art_.drip, 206, 116 + std::sin(t_ * 1.7f) * 3.f, 8, PAL_WATER, false, false);

    for (int i = 0; i < 7; i++) spr(art_.lip, 40.f + i * 40.f + sx, 186, 18, PAL_LIME, i & 1, false);
    spr(art_.lip, 46 + sx, 150, 14, PAL_LIME, false, false);
    spr(art_.lip, 274 + sx, 150, 14, PAL_LIME, true, false);
}

}  // namespace cmaga
