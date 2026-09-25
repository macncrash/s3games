// S3 ROCK — one minute on a rock rapid. The hull you still have is the score.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace rock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ROCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    // 0 title, 1 the rapid, 2 held the minute, 3 hull opened
    int marker() const;

    // One line on stdout. Returns 0 only when the minute was held.
    int report(int frames) const;

private:
    enum class Mode { Title, Run, Pause, End };

    struct Rock {
        float x = 0, y = 0, vy = 0, r = 0;
        int kind = 0, var = 0, pal = 0;
        bool flip = false;
        bool live = true;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0, max = 1;
    };
    struct Pop {
        float x = 0, y = 0, life = 0;
        int dmg = 0;
    };
    struct Decor {
        float x = 0, y = 0, vy = 0;
        int kind = 0, var = 0, pal = 0;
        bool flip = false;
    };

    void castOff();
    void layoutDecor();
    void update();
    void steer();
    void spawnRock();
    bool trySpawn(int kind);
    void collide();
    void chip(int dmg, float x, float y, int kind);
    void tickFx();
    void finish(bool win);
    void audio();
    void quiet();
    void draw();
    void scenery();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal, int align);
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int hull_ = 100;
    int best_ = 0;
    int playFrame_ = 0;
    int lived_ = 0;
    int spawned_ = 0;
    int inv_ = 0;
    int shake_ = 0;
    int scrape_ = 0;
    int thud_ = 0;
    int spawnSide_ = 0;
    int hitFrame_ = -1;
    int fan_ = 0;
    int fanFrame_ = 0;
    float boatX_ = 160;
    float vel_ = 0;
    float scroll_ = 0;
    float hitDx_ = 0, hitDy_ = 0;
    uint32_t rng_ = 1;
    std::vector<Rock> rocks_;
    std::vector<Mote> motes_;
    std::vector<Pop> pops_;
    Decor decor_[8]{};
};

}  // namespace rock
