// S3 MILLWELL — one mill. Keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mill {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILLWELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int well() const { return wellHp_; }
    // 0 title, 1 yard, 2 between waves, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Fight, Banner, Victory, Defeat };
    enum class Kind { Crow, Boar, Raider };

    struct Foe {
        Kind kind;
        float x;
        int lane;
        int hp;
        bool left;
        float flash;
    };
    struct Spawn {
        float t;
        Kind kind;
        int lane;
        bool left;
    };
    struct Sack {
        float x, vx;
        int lane;
    };
    struct Puff {
        float x, y, t;
    };

    void begin();
    void armWave();
    void update(float dt);
    void botThink();
    void swing();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void tone(float f, float v, float hold);
    float rnd();
    const char* waveName() const;
    static float laneY(int lane);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int score_ = 0;
    int wellHp_ = 9;
    int wellMax_ = 9;
    float px_ = 160;
    int lane_ = 1;
    int face_ = 1;
    float swingT_ = 0;
    float cool_ = 0;
    float throwC_ = 0;
    float t_ = 0;
    float clock_ = 0;
    float beep_ = 0;
    float noteT_ = 0;
    int noteI_ = 0;
    bool fan_ = false;
    uint32_t rng_ = 0x51a11u;
    bool wantSwing_ = false;
    bool wantThrow_ = false;
    int wantLane_ = 0;

    std::vector<Foe> foes_;
    std::vector<Spawn> spawns_;
    std::vector<Sack> sacks_;
    std::vector<Puff> puffs_;
    int spawned_ = 0;
};

}  // namespace mill
