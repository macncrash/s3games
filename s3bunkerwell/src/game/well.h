// S3 BUNKER WELL — one yard, one stone well.
// Orbit the coping. Drop anyone who reaches the water. Three waves.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace bwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUNKER WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int well() const { return well_; }
    // 0 title, 1 the yard, 2 a wave is down, 3 the well still stands
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Pause, Won, Lost };

    struct Spawn {
        int frame;
        int kind;
        float ang;
        float speed;
    };
    struct Foe {
        int kind = 0;
        int hp = 1;
        int points = 100;
        int age = 0;
        float ang = 0;
        float r = 150;
        float speed = 0.5f;
        bool alive = true;
    };
    struct Bolt {
        float x = 0, y = 0, vx = 0, vy = 0;
        int life = 0;
    };

    void begin();
    void loadWave();
    void input();
    void playTick();
    void act(float& turn, bool& fire);
    void shoot();
    void blip(int ch, float freq);
    void draw();
    void sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    bool anyAlive() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int score_ = 0;
    int wave_ = 0;
    int well_ = 8;
    int waveTime_ = 0;
    int spawnIx_ = 0;
    int calm_ = 0;
    int banner_ = 0;
    int cool_ = 0;
    int flash_ = 0;
    int wonAge_ = 0;
    float aim_ = -1.2f;
    uint32_t rng_ = 0xB077u;
    std::vector<Spawn> spawns_;
    std::vector<Foe> foes_;
    Bolt bolts_[12] = {};
};

}  // namespace bwell
