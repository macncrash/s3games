// S3 WHARFWELL — one wharf, one well, three waves.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace wharf {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WHARFWELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int well() const { return wellHp_; }
    int wave() const { return wave_; }
    int score() const { return score_; }

private:
    enum class Mode { Title, Banner, Play, End };
    enum Kind { Crab, Barrel, Brute };

    struct Spawn {
        float t;
        Kind kind;
        int side;  // -1 left, +1 right
    };
    struct Foe {
        Kind kind;
        float x;
        int hp;
        float spd;
        int dmg;
        int side;
        bool alive;
        int hit;
    };

    void resetRun();
    void queueWave();
    void update(float dt);
    void draw();
    void backdrop();
    void blit(const gs::Image& img, float x, float y, int pal, bool flip = false);
    void text(const std::string& s, float x, float y, int pal);
    void textC(const std::string& s, float y, int pal);
    void blip(bool high);
    void hitSound();
    void fanfare();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int wellHp_ = 10;
    int score_ = 0;
    float t_ = 0;
    float bannerT_ = 0;
    float endT_ = 0;
    float waveT_ = 0;
    float px_ = 160;
    int face_ = 1;
    float cool_ = 0;
    float swingT_ = 0;
    int swingId_ = 0;
    float hurt_ = 0;
    float walk_ = 0;
    std::vector<Spawn> queue_;
    int spawned_ = 0;
    std::vector<Foe> foes_;
    bool music_ = false;
};

}  // namespace wharf
