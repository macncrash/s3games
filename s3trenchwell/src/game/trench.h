// S3 TRENCH WELL — the duckboards are yours. The well is not optional.
// Three waves come down the slope. If the well falls, the trench is lost.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace trench {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int well() const { return well_; }
    // 0 title, 1 the line, 2 a wave is down, 3 the well still stands
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Won, Lost };

    struct Spawn {
        int frame;
        int kind;
        float x;
        float speed;
    };
    struct Foe {
        int kind = 0;
        int hp = 1;
        int points = 100;
        int age = 0;
        float x = 0;
        float y = 0;
        float speed = 0.8f;
        bool alive = true;
    };
    struct Bolt {
        float x = 0, y = 0;
        int life = 0;
    };

    void begin();
    void loadWave();
    void input();
    void playTick();
    void act(float& move, bool& fire);
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
    int toneLeft_ = 0;
    float px_ = 96.f;
    uint32_t rng_ = 0x7E11u;
    std::vector<Spawn> spawns_;
    std::vector<Foe> foes_;
    Bolt bolts_[8] = {};
};

}  // namespace trench
