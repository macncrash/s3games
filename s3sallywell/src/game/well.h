// At the sally, keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace sally {

class Game : public gs::Cart {
public:
    const char* title() const override { return "SALLY WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_ + 1; }
    int well() const { return well_; }

private:
    enum class Mode { Title, Brief, Watch, Banner, Victory, Defeat };

    struct Spawn {
        float t, x, spd;
    };
    struct Raider {
        float x, z, spd, flash;
        bool alive;
    };
    struct Toss {
        float x, z;
    };
    struct Spark {
        float x, y, t;
    };

    void startWatch();
    void beginWave();
    void layCourt();
    void update();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float feet, float h, int pal, bool flip, int fog, bool shadow);
    void project(float wx, float z, float& sx, float& sy, float& sc) const;
    void botIntent(bool& left, bool& right, bool& strike, bool& toss);
    void killAt(float x, float z, float rx, float rz, int pts);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int score_ = 0;
    int well_ = 8;
    int spawned_ = 0;
    float clock_ = 0;
    float px_ = 0;
    float strikeCd_ = 0;
    float tossCd_ = 0;
    float swing_ = 0;
    float banner_ = 0;
    float shake_ = 0;
    float march_ = 0;
    std::vector<Spawn> spawns_;
    std::vector<Raider> raiders_;
    std::vector<Toss> tosses_;
    std::vector<Spark> sparks_;
};

}  // namespace sally
