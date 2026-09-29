// S3 REDOUBT DOOR — hold the door for three minutes. If it gives, the watch is over.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace door {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 REDOUBT DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the door is failing, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Hold, Dead, Victory };

    struct Foe {
        int lane;
        float dist;
        float speed;
        int kind;  // 0 runner, 1 ram
        float flash;
    };

    void begin();
    void update(float dt);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    float rnd();
    void botThink();
    int laneX(int lane) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE DOOR GAVE WAY";
    int score_ = 0;
    int door_ = 100;
    int lane_ = 1;
    int want_ = 1;
    float slide_ = 0;
    float clock_ = 0;
    float strike_ = 0;
    float cool_ = 0;
    float spawn_ = 1.2f;
    float shake_ = 0;
    float titleT_ = 0;
    int lastLane_ = -1;
    uint32_t rng_ = 0xD007u;
    std::vector<Foe> foes_;
};

}  // namespace door
