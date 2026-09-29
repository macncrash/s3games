// S3 VIADUCT WELL — keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "art.h"
#include "console/system.h"

namespace well {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 VIADUCT WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int stones() const { return stones_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 a wave, 2 the well still stands
    int marker() const;

private:
    enum class Mode { Title, Watch, Victory, Over };

    struct Foe {
        float x = 0;
        float spd = 0;
        int dir = 1;
        int hp = 1;
        int kind = 0;
        int age = 0;
        bool alive = true;
    };
    struct Bolt {
        float x = 0;
        float vx = 0;
        bool on = false;
    };

    void beginWatch();
    void update(float dt);
    void spawnDue();
    void fireAt(float tx);
    void winWatch();
    void loseWatch(const char* why);
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int nearest() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE WELL FELL";
    int score_ = 0;
    int wave_ = 0;
    int stones_ = 8;
    int spawned_ = 0;
    int toSpawn_ = 0;
    int face_ = 1;
    int step_ = 0;
    float px_ = 160;
    float clock_ = 0;
    float cool_ = 0;
    float lull_ = 0;
    float modeT_ = 0;
    float shake_ = 0;
    bool between_ = false;
    std::vector<Foe> foes_;
    Bolt bolts_[2];
};

}  // namespace well
