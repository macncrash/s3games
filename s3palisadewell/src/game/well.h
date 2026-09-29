#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace palisade {

class Game : public gs::Cart {
public:
    const char* title() const override { return "PALISADE WELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int wave() const { return wave_; }
    int wellHp() const { return wellHp_; }
    // 0 title, 1 the watch, 2 between waves, 3 the well stands, 4 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Watch, Banner, Won, Lost };

    struct Foe {
        float x, y, hp, flash;
        int kind;
        float speed;
    };
    struct Bolt {
        float x, y;
    };
    struct Slot {
        float t;
        float x;
        int kind;
    };

    void beginWatch();
    void openWave();
    void update(float dt);
    void draw();
    void hudText(int col, int row, const char* s);
    void spr(const gs::Image& img, float x, float y, int w, int h, int pal, bool flip = false, int fog = 0);
    void shoot();
    void blip(float freq);
    void stick(float& dir, bool& fire);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int score_ = 0;
    int wellHp_ = 100;
    int left_ = 0;
    float t_ = 0;
    float px_ = 160;
    float face_ = 1;
    float cool_ = 0;
    float shake_ = 0;
    float bannerT_ = 0;
    float hurt_ = 0;
    int spawnIx_ = 0;
    uint32_t rng_ = 0x5A11u;
    std::vector<Slot> script_;
    std::vector<Foe> foes_;
    std::vector<Bolt> bolts_;
};

}  // namespace palisade
